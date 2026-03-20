#include <stdio.h>
#include <string.h>
#include "protocol.h"

int main() {
    char buf[1024];

    // =========================
    // TEST pack_createtracker
    // =========================
    pack_createtracker(buf, "movie1.avi", 1000, "Ghost", "abc123", "127.0.0.1", 5001);
    printf("PACK CREATE:\n%s\n", buf);

    // =========================
    // TEST parse_createtracker
    // =========================
    char filename[100], desc[100], md5[100], ip[50];
    long filesize;
    int port;

    parse_createtracker(buf, filename, &filesize, desc, md5, ip, &port);

    printf("PARSE CREATE:\n");
    printf("filename: %s\n", filename);
    printf("filesize: %ld\n", filesize);
    printf("desc: %s\n", desc);
    printf("md5: %s\n", md5);
    printf("ip: %s\n", ip);
    printf("port: %d\n\n", port);

    // =========================
    // TEST LIST
    // =========================
    pack_list(buf);
    printf("PACK LIST:\n%s\n", buf);

    // =========================
    // TEST GET
    // =========================
    pack_get(buf, "movie1.avi");
    printf("PACK GET:\n%s\n", buf);

    // =========================
    // TEST UPDATE
    // =========================
    pack_updatetracker(buf, "movie1.avi", 0, 1024, "127.0.0.1", 5001);
    printf("PACK UPDATE:\n%s\n", buf);

    return 0;
}