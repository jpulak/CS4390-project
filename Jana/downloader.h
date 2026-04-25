#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include "protocol.h"
#include "file_utils.h"
#include "md5_util.h"

#include <pthread.h>

// peer info
typedef struct {
    char ip[64];
    int port;
    long timestamp;
} PeerInfo;

// thread task
typedef struct {
    char filename[256];
    char ip[64];
    int port;
    long start;
    long end;
} DownloadTask;

// main coordinator (FINAL REQUIRED FUNCTION)
int multi_peer_download(const char* filename,
                        PeerInfo* peers,
                        int num_peers,
                        long filesize,
                        const char* state_file);

#endif
