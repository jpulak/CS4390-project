#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "md5_util.h"

int main() {
    // 🟦 Test 1: Buffer MD5
    const char* text = "hello";
    char* hash1 = compute_buffer_md5(text, strlen(text));

    if (hash1 != NULL) {
        printf("Buffer MD5 (\"hello\"): %s\n", hash1);
        free(hash1);
    } else {
        printf("Buffer MD5 failed\n");
    }

    // 🟦 Test 2: File MD5
    const char* filename = "test.txt";  // make sure this file exists
    char* hash2 = compute_file_md5(filename);

    if (hash2 != NULL) {
        printf("File MD5 (%s): %s\n", filename, hash2);
        free(hash2);
    } else {
        printf("File MD5 failed (could not open file)\n");
    }

    return 0;
}