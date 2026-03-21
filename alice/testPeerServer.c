#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "peer_server.h"
#include "file_utils.h"
#define TEST_PORT          7777
#define TEST_SHARED_FOLDER "/tmp/test_shared"
#define TEST_CONFIG        "/tmp/test_server.cfg"
#define TEST_FILE          "hello.txt"
#define TEST_FILE_PATH     TEST_SHARED_FOLDER "/" TEST_FILE

static void setup() {
    
    FILE *f = fopen(TEST_CONFIG, "w");
    assert(f);
    fprintf(f, "port=%d\n", TEST_PORT);
    fprintf(f, "shared_folder=%s\n", TEST_SHARED_FOLDER);
    fclose(f);

    system("mkdir -p " TEST_SHARED_FOLDER);
    f = fopen(TEST_FILE_PATH, "wb");
    assert(f);

    for (int i = 0; i < 2048; i++)
        fputc(i % 256, f);
    fclose(f);
}

static void teardown() {
    system("rm -rf " TEST_SHARED_FOLDER);
    remove(TEST_CONFIG);
}

static int connect_to_server() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    assert(fd >= 0);

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(TEST_PORT);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    /* Retry a few times — server thread may still be starting */
    for (int i = 0; i < 10; i++) {
        if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0)
            return fd;
        usleep(100000);  /* 100 ms */
    }
    fprintf(stderr, "Could not connect to server\n");
    exit(1);
}

//Send a request, read back the full response into buf. Returns bytes read. 
static int do_request(const char *request, char *buf, int buflen) {
    int fd = connect_to_server();
    send(fd, request, strlen(request), 0);

    int total = 0;
    int n;
    while ((n = recv(fd, buf + total, buflen - total - 1, 0)) > 0)
        total += n;

    buf[total] = '\0';
    close(fd);
    return total;
}

//valid request, first 10 bytes
void testValidSmalll() {
    char buf[256] = {0};
    int  n = do_request("GET hello.txt 0 10\n", buf, sizeof(buf));

    // Response: "<GET ok 10>\n" + 10 bytes of data
    assert(strncmp(buf, "<GET ok 10>\n", 12) == 0);
    assert(n == 12 + 10);

    // Verify the actual bytes
    unsigned char *data = (unsigned char *)(buf + 12);
    for (int i = 0; i < 10; i++)
        assert(data[i] == (unsigned char)i);

    printf("PASS: valid request, first 10 bytes\n");
}

//valid request, max chunk size (1024 bytes)
void testValidMaxChunk() {
    char buf[2048] = {0};
    int  n = do_request("GET hello.txt 0 1024\n", buf, sizeof(buf));

    assert(strncmp(buf, "<GET ok 1024>\n", 14) == 0);
    assert(n == 14 + 1024);
    printf("PASS: valid request, 1024 bytes\n");
}

//non-zero offset
void testRequestOffset() {
    char buf[256] = {0};
    int  n = do_request("GET hello.txt 100 10\n", buf, sizeof(buf));

    assert(strncmp(buf, "<GET ok 10>\n", 12) == 0);
    assert(n == 12 + 10);

    /* bytes at offset 100: should be 100, 101, ..., 109 */
    unsigned char *data = (unsigned char *)(buf + 12);
    for (int i = 0; i < 10; i++)
        assert(data[i] == (unsigned char)(100 + i));

    printf("PASS: valid request, offset 100\n");
}

//chunk size over 1024 → <GET invalid> 
void testOverLimit() {
    char buf[64] = {0};
    do_request("GET hello.txt 0 1025\n", buf, sizeof(buf));

    assert(strncmp(buf, "<GET invalid>\n", 14) == 0);
    printf("PASS: chunk > 1024 rejected\n");
}

//chunk size of 0 → <GET invalid>
void testZeroLength() {
    char buf[64] = {0};
    do_request("GET hello.txt 0 0\n", buf, sizeof(buf));

    assert(strncmp(buf, "<GET invalid>\n", 14) == 0);
    printf("PASS: chunk size 0 rejected\n");
}

//file does not exist → <GET invalid>
void testMissingFile() {
    char buf[64] = {0};
    do_request("GET ghost.txt 0 10\n", buf, sizeof(buf));

    assert(strncmp(buf, "<GET invalid>\n", 14) == 0);
    printf("PASS: missing file rejected\n");
}

// path traversal attempt → <GET invalid>
void tesPathTraversal() {
    char buf[64] = {0};
    do_request("GET ../etc/passwd 0 10\n", buf, sizeof(buf));

    assert(strncmp(buf, "<GET invalid>\n", 14) == 0);
    printf("PASS: path traversal rejected\n");
}

//garbage/malformed request → <GET invalid> 
void testMalformedRequest() {
    char buf[64] = {0};
    do_request("HELLO WORLD\n", buf, sizeof(buf));

    assert(strncmp(buf, "<GET invalid>\n", 14) == 0);
    printf("PASS: malformed request rejected\n");
}

// multiple concurrent connections
#define NUM_THREADS 20

typedef struct { 
    int id; 
    int passed; 
} ThreadResult;

static void *concurrent_worker(void *arg) {
    ThreadResult *r = (ThreadResult *)arg;
    char buf[2048] = {0};
    do_request("GET hello.txt 0 10\n", buf, sizeof(buf));
    r->passed = (strncmp(buf, "<GET ok 10>\n", 12) == 0);
    return NULL;
}

void test_concurrent_connections() {
    pthread_t      threads[NUM_THREADS];
    ThreadResult   results[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++) {
        results[i].id     = i;
        results[i].passed = 0;
        pthread_create(&threads[i], NULL, concurrent_worker, &results[i]);
    }
    for (int i = 0; i < NUM_THREADS; i++)
        pthread_join(threads[i], NULL);

    int passed = 0;
    for (int i = 0; i < NUM_THREADS; i++)
        passed += results[i].passed;

    assert(passed == NUM_THREADS);
    printf("PASS: %d concurrent connections all served correctly\n", NUM_THREADS);
}

//request at EOF boundary
void testEofBoundary() {
    /* File is 2048 bytes. Request bytes 2040-2048 — only 8 bytes remain. */
    char buf[256] = {0};
    int  n = do_request("GET hello.txt 2040 1024\n", buf, sizeof(buf));

    assert(strncmp(buf, "<GET ok 8>\n", 11) == 0);
    assert(n == 11 + 8);
    printf("PASS: EOF boundary — partial chunk returned\n");
}

int main() {
    printf("=== peer_server tests ===\n\n");

    setup();

    /* Start the server — give it 300ms to bind and listen */
    assert(start_peer_server(TEST_CONFIG) == 0);
    usleep(300000);

    test_valid_request_small();
    test_valid_request_max_chunk();
    test_valid_request_offset();
    test_over_limit();
    test_zero_length();
    test_missing_file();
    test_path_traversal();
    test_malformed_request();
    test_concurrent_connections();
    test_eof_boundary();

    teardown();

    printf("\nAll peer_server tests passed.\n");
    return 0;
}