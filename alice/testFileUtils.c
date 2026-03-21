#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "file_utils.h"

void testWriteRead() {
    const char *path = "/tmp/test_chunk.bin";
    const char *data = "Hello, chunk world!";
    int len = strlen(data);

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

void testFileComplete() {
    const char *path = "/tmp/test_complete.bin";
    char data[512] = {0};
    write_chunk(path, 0, data, 512);

    assert(is_file_complete(path, 512) == 1);
    assert(is_file_complete(path, 1024) == 0);  
    assert(is_file_complete("/tmp/nonexistent.bin", 10) == 0);
    printf("PASS: File Complete\n");
}

void testResume() {
    const char *state = "/tmp/test_state.txt";
    remove(state);  /* start fresh */

    record_downloaded_range(state, "movie.avi", 8192, 0,    1023);
    record_downloaded_range(state, "movie.avi", 8192, 2048, 3071);

    IncompleteFile out[4];
    int n = scan_incomplete_downloads("/tmp", state, out, 4);

    assert(n == 1);
    printf("Missing ranges for movie.avi: %d\n", out[0].missing_count);
    for (int i = 0; i < out[0].missing_count; i++)
        printf("  [%ld - %ld]\n", out[i].missing[i].start, out[i].missing[i].end);
    printf("PASS: resume scan\n");
}

int main() {
    test_write_read();
    test_is_file_complete();
    test_resume();
    printf("\nAll file_utils tests passed.\n");
    return 0;
}