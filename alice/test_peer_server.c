#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <windows.h>
  #include <direct.h>
  #pragma comment(lib, "ws2_32.lib")
  #define sleep_ms(ms)  Sleep(ms)
  #define sock_close(s) closesocket(s)
  typedef SOCKET sock_t;
  #define INVALID_SOCK  INVALID_SOCKET
#else
  #include <unistd.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #define sleep_ms(ms)  usleep((ms)*1000)
  #define sock_close(s) close(s)
  typedef int sock_t;
  #define INVALID_SOCK  (-1)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include "peer_server.h"
#include "file_utils.h"

#define TEST_PORT      7777
#define TEST_FILE      "hello.txt"
#define TEST_FILE_SIZE 2048

static char TEST_SHARED_FOLDER[512];
static char TEST_CONFIG[512];
static char TEST_FILE_PATH[512];

static void init_paths(void) {
    const char *tmp = getenv("TEMP");
    if (!tmp) tmp = getenv("TMP");
    if (!tmp) tmp = ".";
    snprintf(TEST_SHARED_FOLDER, sizeof(TEST_SHARED_FOLDER), "%s\\test_shared",    tmp);
    snprintf(TEST_CONFIG,        sizeof(TEST_CONFIG),        "%s\\test_server.cfg", tmp);
    snprintf(TEST_FILE_PATH,     sizeof(TEST_FILE_PATH),     "%s\\%s", TEST_SHARED_FOLDER, TEST_FILE);
}

static void make_dir(const char *path) {
#ifdef _WIN32
    _mkdir(path);
#else
    mkdir(path, 0755);
#endif
}

static void remove_dir_recursive(const char *path) {
#ifdef _WIN32
    char cmd[600];
    snprintf(cmd, sizeof(cmd), "rmdir /s /q \"%s\"", path);
    system(cmd);
#else
    char cmd[600];
    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", path);
    system(cmd);
#endif
}

static void setup(void) {
    make_dir(TEST_SHARED_FOLDER);

    FILE *f = fopen(TEST_CONFIG, "w");
    assert(f != NULL);
    fprintf(f, "port=%d\n",          TEST_PORT);
    fprintf(f, "shared_folder=%s\n", TEST_SHARED_FOLDER);
    fclose(f);

    f = fopen(TEST_FILE_PATH, "wb");
    assert(f != NULL);
    for (int i = 0; i < TEST_FILE_SIZE; i++)
        fputc(i % 256, f);
    fclose(f);
}

static void teardown(void) {
    remove_dir_recursive(TEST_SHARED_FOLDER);
    remove(TEST_CONFIG);
}

static sock_t connect_to_server(void) {
    sock_t fd = socket(AF_INET, SOCK_STREAM, 0);
    assert(fd != INVALID_SOCK);

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(TEST_PORT);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    for (int i = 0; i < 10; i++) {
        if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0)
            return fd;
        sleep_ms(100);
    }
    fprintf(stderr, "Could not connect to server\n");
    exit(1);
}

static int do_request(const char *request, char *buf, int buflen) {
    sock_t fd = connect_to_server();
    send(fd, request, (int)strlen(request), 0);

    int total = 0, n;
    while ((n = recv(fd, buf + total, buflen - total - 1, 0)) > 0)
        total += n;

    buf[total] = '\0';
    sock_close(fd);
    return total;
}

void test_valid_small_request(void) {
    char buf[256] = {0};
    int  n = do_request("GET hello.txt 0 10\n", buf, sizeof(buf));
    assert(strncmp(buf, "<GET ok 10>\n", 12) == 0);
    assert(n == 12 + 10);
    unsigned char *data = (unsigned char *)(buf + 12);
    for (int i = 0; i < 10; i++)
        assert(data[i] == (unsigned char)i);
    printf("PASS: valid request, first 10 bytes\n");
}

void test_valid_max_chunk(void) {
    char buf[2048] = {0};
    int  n = do_request("GET hello.txt 0 1024\n", buf, sizeof(buf));
    assert(strncmp(buf, "<GET ok 1024>\n", 14) == 0);
    assert(n == 14 + 1024);
    printf("PASS: valid request, 1024 bytes\n");
}

void test_valid_nonzero_offset(void) {
    char buf[256] = {0};
    int  n = do_request("GET hello.txt 100 10\n", buf, sizeof(buf));
    assert(strncmp(buf, "<GET ok 10>\n", 12) == 0);
    assert(n == 12 + 10);
    unsigned char *data = (unsigned char *)(buf + 12);
    for (int i = 0; i < 10; i++)
        assert(data[i] == (unsigned char)(100 + i));
    printf("PASS: valid request, offset 100\n");
}

void test_valid_second_chunk(void) {
    char buf[2048] = {0};
    int  n = do_request("GET hello.txt 1024 1024\n", buf, sizeof(buf));
    assert(strncmp(buf, "<GET ok 1024>\n", 14) == 0);
    assert(n == 14 + 1024);
    printf("PASS: valid request, second chunk\n");
}

void test_over_limit(void) {
    char buf[64] = {0};
    do_request("GET hello.txt 0 1025\n", buf, sizeof(buf));
    assert(strncmp(buf, "<GET invalid>\n", 14) == 0);
    printf("PASS: chunk > 1024 rejected\n");
}

void test_zero_length(void) {
    char buf[64] = {0};
    do_request("GET hello.txt 0 0\n", buf, sizeof(buf));
    assert(strncmp(buf, "<GET invalid>\n", 14) == 0);
    printf("PASS: chunk size 0 rejected\n");
}

void test_negative_length(void) {
    char buf[64] = {0};
    do_request("GET hello.txt 0 -1\n", buf, sizeof(buf));
    assert(strncmp(buf, "<GET invalid>\n", 14) == 0);
    printf("PASS: negative chunk size rejected\n");
}

void test_missing_file(void) {
    char buf[64] = {0};
    do_request("GET ghost.txt 0 10\n", buf, sizeof(buf));
    assert(strncmp(buf, "<GET invalid>\n", 14) == 0);
    printf("PASS: missing file rejected\n");
}

void test_path_traversal_dotdot(void) {
    char buf[64] = {0};
    do_request("GET ../etc/passwd 0 10\n", buf, sizeof(buf));
    assert(strncmp(buf, "<GET invalid>\n", 14) == 0);
    printf("PASS: path traversal (..) rejected\n");
}

void test_path_traversal_slash(void) {
    char buf[64] = {0};
    do_request("GET /etc/passwd 0 10\n", buf, sizeof(buf));
    assert(strncmp(buf, "<GET invalid>\n", 14) == 0);
    printf("PASS: path traversal (/) rejected\n");
}

void test_malformed_request(void) {
    char buf[64] = {0};
    do_request("HELLO WORLD\n", buf, sizeof(buf));
    assert(strncmp(buf, "<GET invalid>\n", 14) == 0);
    printf("PASS: malformed request rejected\n");
}

void test_eof_boundary(void) {
    char buf[256] = {0};
    int  n = do_request("GET hello.txt 2040 1024\n", buf, sizeof(buf));
    assert(strncmp(buf, "<GET ok 8>\n", 11) == 0);
    assert(n == 11 + 8);
    printf("PASS: EOF boundary — partial chunk returned\n");
}

void test_past_eof(void) {
    char buf[64] = {0};
    do_request("GET hello.txt 9999 10\n", buf, sizeof(buf));
    assert(strncmp(buf, "<GET invalid>\n", 14) == 0);
    printf("PASS: request past EOF rejected\n");
}

#define NUM_THREADS 20
typedef struct { int id; int passed; } ThreadResult;

static void *concurrent_worker(void *arg) {
    ThreadResult *r = (ThreadResult *)arg;
    char buf[256] = {0};
    do_request("GET hello.txt 0 10\n", buf, sizeof(buf));
    r->passed = (strncmp(buf, "<GET ok 10>\n", 12) == 0);
    return NULL;
}

void test_concurrent_connections(void) {
    pthread_t    threads[NUM_THREADS];
    ThreadResult results[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        results[i].id = i; results[i].passed = 0;
        pthread_create(&threads[i], NULL, concurrent_worker, &results[i]);
    }
    for (int i = 0; i < NUM_THREADS; i++)
        pthread_join(threads[i], NULL);
    int passed = 0;
    for (int i = 0; i < NUM_THREADS; i++) passed += results[i].passed;
    assert(passed == NUM_THREADS);
    printf("PASS: %d concurrent connections all served correctly\n", NUM_THREADS);
}

void test_data_integrity_two_chunks(void) {
    char buf1[2048] = {0}, buf2[2048] = {0};
    int n1 = do_request("GET hello.txt 0 1024\n",    buf1, sizeof(buf1));
    int n2 = do_request("GET hello.txt 1024 1024\n", buf2, sizeof(buf2));
    assert(strncmp(buf1, "<GET ok 1024>\n", 14) == 0);
    assert(strncmp(buf2, "<GET ok 1024>\n", 14) == 0);
    assert(n1 == 14 + 1024);
    assert(n2 == 14 + 1024);
    unsigned char *d1 = (unsigned char *)(buf1 + 14);
    unsigned char *d2 = (unsigned char *)(buf2 + 14);
    for (int i = 0; i < 1024; i++) {
        assert(d1[i] == (unsigned char)(i % 256));
        assert(d2[i] == (unsigned char)((1024 + i) % 256));
    }
    printf("PASS: two-chunk data integrity verified\n");
}

int main(void) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }
#endif

    init_paths();
    printf("=== peer_server tests ===\n\n");
    setup();

    if (start_peer_server(TEST_CONFIG) != 0) {
        fprintf(stderr, "Failed to start peer server\n");
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }
    sleep_ms(300);

    test_valid_small_request();
    test_valid_max_chunk();
    test_valid_nonzero_offset();
    test_valid_second_chunk();
    test_over_limit();
    test_zero_length();
    test_negative_length();
    test_missing_file();
    test_path_traversal_dotdot();
    test_path_traversal_slash();
    test_malformed_request();
    test_eof_boundary();
    test_past_eof();
    test_concurrent_connections();
    test_data_integrity_two_chunks();

    teardown();
    printf("\nAll peer_server tests passed.\n");
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
