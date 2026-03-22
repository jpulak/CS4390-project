#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "file_utils.h"

static void tmp_path(char *buf, size_t bufsz, const char *filename) {
    const char *tmp = getenv("TEMP");
    if (!tmp) tmp = getenv("TMP");
    if (!tmp) tmp = ".";        
    snprintf(buf, bufsz, "%s\\%s", tmp, filename);
}

void test_write_read(void) {
    char path[512];
    tmp_path(path, sizeof(path), "test_chunk.bin");

    const char *data = "Hello, chunk world!";
    int len = (int)strlen(data);

    assert(write_chunk(path, 0, data, len) == 0);

    char buf[64] = {0};
    int n = read_chunk(path, 0, buf, len);
    assert(n == len);
    assert(memcmp(buf, data, len) == 0);
    printf("PASS: write/read at offset 0\n");

    assert(write_chunk(path, 50, "OFFSET", 6) == 0);
    char buf2[64] = {0};
    int n2 = read_chunk(path, 50, buf2, 6);
    assert(n2 == 6);
    assert(memcmp(buf2, "OFFSET", 6) == 0);
    printf("PASS: write/read at offset 50\n");
}

void test_is_file_complete(void) {
    char path[512];
    tmp_path(path, sizeof(path), "test_complete.bin");

    char data[512] = {0};
    write_chunk(path, 0, data, 512);

    assert(is_file_complete(path, 512) == 1);
    assert(is_file_complete(path, 1024) == 0);

    char nopath[512];
    tmp_path(nopath, sizeof(nopath), "nonexistent.bin");
    assert(is_file_complete(nopath, 10) == 0);

    printf("PASS: file complete\n");
}

void test_resume(void) {
    char state[512], tmpdir[512];
    tmp_path(state,  sizeof(state),  "test_state.txt");
    tmp_path(tmpdir, sizeof(tmpdir), "");  

    size_t len = strlen(tmpdir);
    if (len > 0 && (tmpdir[len-1] == '\\' || tmpdir[len-1] == '/'))
        tmpdir[len-1] = '\0';

    remove(state); 

    record_downloaded_range(state, "movie.avi", 8192, 0,    1023);
    record_downloaded_range(state, "movie.avi", 8192, 2048, 3071);

    IncompleteFile out[4];
    int n = scan_incomplete_downloads(tmpdir, state, out, 4);

    assert(n == 1);
    printf("Missing ranges for movie.avi: %d\n", out[0].missing_count);
    for (int i = 0; i < out[0].missing_count; i++)
        printf("  [%ld - %ld]\n", out[0].missing[i].start,  
                                  out[0].missing[i].end);
    printf("PASS: resume scan\n");
}

int main(void) {
    test_write_read();
    test_is_file_complete();
    test_resume();
    printf("\nAll file_utils tests passed.\n");
    return 0;
}
