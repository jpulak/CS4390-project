#include "file_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <errno.h>

//Keeps threads safe
static pthread_mutex_t file_mutex  = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;


static int ensure_file_exists(const char *filepath)
{
    FILE *f = fopen(filepath, "ab");
    if (!f) { perror("ensure_file_exists"); return -1; }
    fclose(f);
    return 0;
}

/* Comparator for qsort – sort ByteRanges by start offset */
static int cmp_ranges(const void *a, const void *b)
{
    const ByteRange *ra = (const ByteRange *)a;
    const ByteRange *rb = (const ByteRange *)b;
    if (ra->start < rb->start) return -1;
    if (ra->start > rb->start) return  1;
    return 0;
}


int read_chunk(const char *filepath, long offset, char *buf, int max_len)
{
    if (!filepath || !buf || max_len <= 0 || offset < 0) return -1;

    FILE *f = fopen(filepath, "rb");
    if (!f) { perror("read_chunk: fopen"); return -1; }

    if (fseek(f, offset, SEEK_SET) != 0) {
        perror("read_chunk: fseek");
        fclose(f);
        return -1;
    }

    int n = (int)fread(buf, 1, (size_t)max_len, f);
    fclose(f);
    return n;  
}

int write_chunk(const char *filepath, long offset, const char *data, int len)
{
    if (!filepath || !data || len <= 0 || offset < 0) return -1;

    pthread_mutex_lock(&file_mutex);

    if (ensure_file_exists(filepath) != 0) {
        pthread_mutex_unlock(&file_mutex);
        return -1;
    }

    FILE *f = fopen(filepath, "r+b");
    if (!f) {
        perror("write_chunk: fopen r+b");
        pthread_mutex_unlock(&file_mutex);
        return -1;
    }

    if (fseek(f, offset, SEEK_SET) != 0) {
        perror("write_chunk: fseek");
        fclose(f);
        pthread_mutex_unlock(&file_mutex);
        return -1;
    }

    int n = (int)fwrite(data, 1, (size_t)len, f);
    fflush(f);
    fclose(f);

    pthread_mutex_unlock(&file_mutex);
    return (n == len) ? 0 : -1;
}

//states 1 if file is complete, 0 otherwise
int is_file_complete(const char *filepath, long expected_size)
{
    struct stat st;
    if (stat(filepath, &st) != 0) return 0;
    return (st.st_size >= expected_size) ? 1 : 0;
}


int merge_chunks(const char *filepath)
{
    struct stat st;
    if (stat(filepath, &st) != 0) {
        fprintf(stderr, "merge_chunks: not found: %s\n", filepath);
        return -1;
    }
    if (st.st_size == 0) {
        fprintf(stderr, "merge_chunks: empty file: %s\n", filepath);
        return -1;
    }
    printf("[FILE] merge_chunks: %s – %ld bytes, looks complete\n",
           filepath, (long)st.st_size);
    return 0;
}



#define MAX_STATE_LINES 512
#define MAX_LINE_LEN    4096

int record_downloaded_range(const char *state_path, const char *filename,
                             long total_size, long range_start, long range_end)
{
    if (!state_path || !filename) return -1;

    pthread_mutex_lock(&state_mutex);

    /* ── 1. Read current state into memory ── */
    char lines[MAX_STATE_LINES][MAX_LINE_LEN];
    int  line_count = 0;
    int  found = 0;

    FILE *f = fopen(state_path, "r");
    if (f) {
        while (line_count < MAX_STATE_LINES &&
               fgets(lines[line_count], MAX_LINE_LEN, f)) {
            lines[line_count][strcspn(lines[line_count], "\n")] = '\0';
            if (strlen(lines[line_count]) == 0) continue;

            /* Does this line belong to our file? */
            char fname[MAX_FILENAME_LEN];
            long tsz;
            sscanf(lines[line_count], "%255s %ld", fname, &tsz);

            if (strcmp(fname, filename) == 0) {
                /* Append the new range to the existing line */
                char extra[64];
                snprintf(extra, sizeof(extra), ",%ld:%ld", range_start, range_end);
                strncat(lines[line_count], extra,
                        MAX_LINE_LEN - strlen(lines[line_count]) - 1);
                found = 1;
            }
            line_count++;
        }
        fclose(f);
    }

    // New entry if not seen before 
    if (!found && line_count < MAX_STATE_LINES) {
        snprintf(lines[line_count], MAX_LINE_LEN,
                 "%s %ld %ld:%ld", filename, total_size, range_start, range_end);
        line_count++;
    }

    // Rewrite state file
    f = fopen(state_path, "w");
    if (!f) {
        pthread_mutex_unlock(&state_mutex);
        return -1;
    }
    for (int i = 0; i < line_count; i++)
        fprintf(f, "%s\n", lines[i]);
    fclose(f);

    pthread_mutex_unlock(&state_mutex);
    return 0;
}


static void compute_missing_ranges(IncompleteFile *out, const char *range_str)
{
    ByteRange have[MAX_RANGES];
    int have_count = 0;

    /* ── Parse "s:e,s:e,..." ── */
    char buf[MAX_LINE_LEN];
    strncpy(buf, range_str, sizeof(buf) - 1);
    char *tok = strtok(buf, ",");
    while (tok && have_count < MAX_RANGES) {
        long s, e;
        if (sscanf(tok, "%ld:%ld", &s, &e) == 2) {
            have[have_count].start = s;
            have[have_count].end   = e;
            have_count++;
        }
        tok = strtok(NULL, ",");
    }

    qsort(have, have_count, sizeof(ByteRange), cmp_ranges);

    // Walk from byte 0 to total_size, find gaps in file
    long cursor = 0;
    out->missing_count = 0;

    for (int i = 0; i < have_count && out->missing_count < MAX_RANGES; i++) {
        if (have[i].start > cursor) {
            out->missing[out->missing_count].start = cursor;
            out->missing[out->missing_count].end   = have[i].start - 1;
            out->missing_count++;
        }
        if (have[i].end + 1 > cursor)
            cursor = have[i].end + 1;
    }

    if (cursor < out->total_size && out->missing_count < MAX_RANGES) {
        out->missing[out->missing_count].start = cursor;
        out->missing[out->missing_count].end   = out->total_size - 1;
        out->missing_count++;
    }
}

int scan_incomplete_downloads(const char *shared_folder, const char *state_path,
                               IncompleteFile *out, int max_files)
{
    FILE *f = fopen(state_path, "r");
    if (!f) return 0;   // No state file = nothing to resume

    int count = 0;
    char line[MAX_LINE_LEN];

    while (fgets(line, sizeof(line), f) && count < max_files) {
        line[strcspn(line, "\n")] = '\0';
        if (strlen(line) == 0) continue;

        char fname[MAX_FILENAME_LEN];
        long tsz;
        char range_str[MAX_LINE_LEN] = {0};

        int parsed = sscanf(line, "%255s %ld %4095s", fname, &tsz, range_str);
        if (parsed < 2) continue;

        // Build full path to check actual file on disk
        char fullpath[MAX_PATH_LEN * 2];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", shared_folder, fname);

        if (is_file_complete(fullpath, tsz)) continue; 

        strncpy(out[count].filename, fname, MAX_FILENAME_LEN - 1);
        out[count].filename[MAX_FILENAME_LEN - 1] = '\0';
        out[count].total_size    = tsz;
        out[count].missing_count = 0;

        if (parsed == 3 && strlen(range_str) > 0) {
            compute_missing_ranges(&out[count], range_str);
        } else {
            //no file found
            out[count].missing[0].start = 0;
            out[count].missing[0].end   = tsz - 1;
            out[count].missing_count    = 1;
        }

        // adds if something is missing
        if (out[count].missing_count > 0) {
            printf("[RESUME] %s: %d missing range(s) to re-request\n",
                   fname, out[count].missing_count);
            count++;
        }
    }
    fclose(f);
    return count;
}
