#ifndef FILE_UTILS_H
#define FILE_UTILS_H
#define MAX_CHUNK_SIZE   1024
#define MAX_FILENAME_LEN 256
#define MAX_PATH_LEN     512
#define MAX_RANGES       1024

// A single byte range, inclusive: [start, end] 
typedef struct {
    long start;
    long end;
} ByteRange;

// Describes one incomplete download and its missing ranges
typedef struct {
    char      filename[MAX_FILENAME_LEN];
    long      total_size;
    ByteRange missing[MAX_RANGES];
    int       missing_count;
} IncompleteFile;

//─ Core I/O ─
int read_chunk (const char *filepath, long offset, char *buf, int max_len);
int write_chunk(const char *filepath, long offset, const char *data, int len);

// Completion checks
int is_file_complete(const char *filepath, long expected_size);
int merge_chunks    (const char *filepath);   /* verify file is whole */

// Resume support
int record_downloaded_range(const char *state_path, const char *filename,
                             long total_size, long range_start, long range_end);
int scan_incomplete_downloads(const char *shared_folder, const char *state_path,
                               IncompleteFile *out, int max_files);

#endif 