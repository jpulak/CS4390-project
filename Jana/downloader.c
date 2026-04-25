#include "downloader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define CHUNK_SIZE 1024


//thread func
void* download_segment(void* arg)
{
    DownloadTask* task = (DownloadTask*)arg;

    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[CHUNK_SIZE];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) pthread_exit(NULL);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(task->port);
    inet_pton(AF_INET, task->ip, &server_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) 
    {
        close(sockfd);
        pthread_exit(NULL);
    }

    long offset = task->start;

    while (offset < task->end) 
    {

        long chunk_end = offset + CHUNK_SIZE - 1;
        if (chunk_end > task->end) chunk_end = task->end;

        // build request
        char req[256];
        snprintf(req, sizeof(req),
         "<GET %.180s %ld %ld>\n",
         task->filename, offset, chunk_end);

        send(sockfd, req, strlen(req), 0);

        int n = recv(sockfd, buffer, CHUNK_SIZE, 0);
        if (n <= 0) break;

        // write using alice's func
        write_chunk(task->filename, offset, buffer, n);

        offset += n;
    }

    close(sockfd);

    printf("[THREAD DONE] %s %ld-%ld from %s:%d\n",
           task->filename, task->start, task->end,
           task->ip, task->port);

    pthread_exit(NULL);
}


// maian coordinator
int multi_peer_download(const char* filename,
                        PeerInfo* peers,
                        int num_peers,
                        long filesize,
                        const char* state_file)
{
    pthread_t threads[num_peers];
    DownloadTask tasks[num_peers];

    long segment_size = filesize / num_peers;

    for (int i = 0; i < num_peers; i++)
    {
        strcpy(tasks[i].filename, filename);
        strcpy(tasks[i].ip, peers[i].ip);
        tasks[i].port = peers[i].port;

        tasks[i].start = i * segment_size;

        if (i == num_peers - 1)
            tasks[i].end = filesize;
        else
            tasks[i].end = (i + 1) * segment_size;

        pthread_create(&threads[i], NULL, download_segment, &tasks[i]);
    }

    for (int i = 0; i < num_peers; i++)
        pthread_join(threads[i], NULL);

    printf("[INFO] All threads complete. Verifying file...\n");

    // md5 check form rodrigo
    char* hash = compute_file_md5(filename);

    if (hash) {
        printf("[MD5] %s\n", hash);
    } else {
        printf("[MD5 ERROR]\n");
    }

    // merge check
    merge_chunks(filename);

    printf("[DONE] Download complete: %s\n", filename);

    return 0;
}
