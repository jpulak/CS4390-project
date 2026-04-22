#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>
#include "module/md5_util.h"

pthread_mutex_t tracker_lock = PTHREAD_MUTEX_INITIALIZER;

void handle_createtracker(int sock, char* buffer) {
    char filename[256], description[256], md5[64], ip[64];
    int filesize, port;

    if (sscanf(buffer,
        "<createtracker %s %d %s %s %s %d>",
        filename, &filesize, description, md5, ip, &port) != 6) {

        write(sock, "<createtracker fail>\n", 23);
        return;
    }

    char path[512];
    sprintf(path, "torrents/%s.track", filename);

    pthread_mutex_lock(&tracker_lock);

    // check if file exists
    if (access(path, F_OK) == 0) {
        pthread_mutex_unlock(&tracker_lock);
        write(sock, "<createtracker ferr>\n", 23);
        return;
    }

    FILE* fp = fopen(path, "w");
    if (!fp) {
        pthread_mutex_unlock(&tracker_lock);
        write(sock, "<createtracker fail>\n", 23);
        return;
    }

    
    fprintf(fp, "%s %d %s %s\n", filename, filesize, description, md5);

    
    fprintf(fp, "%s %d 0 %d %ld\n", ip, port, filesize, time(NULL));

    fclose(fp);
    pthread_mutex_unlock(&tracker_lock);

    write(sock, "<createtracker succ>\n", 22);
}

void handle_list(int sock) {
    pthread_mutex_lock(&tracker_lock);

    DIR* dir = opendir("torrents/");
    if (!dir) {
        pthread_mutex_unlock(&tracker_lock);
        write(sock, "<REP LIST 0>\n<REP LIST END>\n", 30);
        return;
    }

    struct dirent* entry;
    char response[1024];

    int count = 0;

    // first pass: count files
    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, ".track"))
            count++;
    }

    rewinddir(dir);

    sprintf(response, "<REP LIST %d>\n", count);
    write(sock, response, strlen(response));

    int index = 1;

    while ((entry = readdir(dir)) != NULL) {
        if (!strstr(entry->d_name, ".track")) continue;

        char filepath[512];
        sprintf(filepath, "torrents/%s", entry->d_name);

        FILE* fp = fopen(filepath, "r");
        if (!fp) continue;

        char fname[256], md5[64], desc[256];
        int filesize;

        fscanf(fp, "%s %d %s %s", fname, &filesize, desc, md5);

        fclose(fp);

        sprintf(response, "<%d %s %d %s>\n", index++, fname, filesize, md5);
        write(sock, response, strlen(response));
    }

    closedir(dir);
    pthread_mutex_unlock(&tracker_lock);

    write(sock, "<REP LIST END>\n", 15);
}


void handle_get(int sock, char* buffer) {
    char filename[256];

    if (sscanf(buffer, "<GET %[^>]>", filename) != 1) {
        write(sock, "Invalid GET\n", 12);
        return;
    }

    char path[512];
    sprintf(path, "torrents/%s", filename);

    pthread_mutex_lock(&tracker_lock);

    FILE* fp = fopen(path, "r");
    if (!fp) {
        pthread_mutex_unlock(&tracker_lock);
        write(sock, "File not found\n", 15);
        return;
    }

    write(sock, "<REP GET BEGIN>\n", 17);

    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        write(sock, line, strlen(line));
    }

    fclose(fp);

    
    char* md5 = compute_file_md5(path);

    char response[512];
    sprintf(response, "<REP GET END %s>\n", md5);
    write(sock, response, strlen(response));

    free(md5);
    pthread_mutex_unlock(&tracker_lock);
}

#define UPDATE_INTERVAL 60

void handle_updatetracker(int sock, char* buffer) {
    char filename[256], ip[64];
    int start, end, port;

    if (sscanf(buffer,
        "<updatetracker %s %d %d %s %d>",
        filename, &start, &end, ip, &port) != 5) {

        write(sock, "<updatetracker fail>\n", 23);
        return;
    }

    char path[512];
    sprintf(path, "torrents/%s.track", filename);

    pthread_mutex_lock(&tracker_lock);

    FILE* fp = fopen(path, "r");
    if (!fp) {
        pthread_mutex_unlock(&tracker_lock);
        write(sock, "<updatetracker filename ferr>\n", 33);
        return;
    }

    FILE* temp = fopen("temp.track", "w");

    char line[1024];

    
    fgets(line, sizeof(line), fp);
    fputs(line, temp);

    time_t now = time(NULL);
    int found = 0;

    while (fgets(line, sizeof(line), fp)) {
        char pip[64];
        int pport, pstart, pend;
        long timestamp;

        sscanf(line, "%s %d %d %d %ld",
               pip, &pport, &pstart, &pend, &timestamp);

        // remove stale peers
        if (now - timestamp > UPDATE_INTERVAL)
            continue;

        // update existing peer
        if (strcmp(pip, ip) == 0 && pport == port) {
            fprintf(temp, "%s %d %d %d %ld\n",
                    ip, port, start, end, now);
            found = 1;
        } else {
            fputs(line, temp);
        }
    }

    // if new peer
    if (!found) {
        fprintf(temp, "%s %d %d %d %ld\n",
                ip, port, start, end, now);
    }

    fclose(fp);
    fclose(temp);

    rename("temp.track", path);

    pthread_mutex_unlock(&tracker_lock);

    write(sock, "<updatetracker filename succ>\n", 33);
}
void* handle_client(void* arg){
    int client_fd = *(int*)arg;
    free(arg);

    char buffer[1024];

    int n = read(client_fd,buffer,sizeof(buffer) -1);

    if(n > 0){
        buffer[n] = '\0';
        if (strncmp(buffer, "<createtracker", 14) == 0) {
            handle_createtracker(client_fd, buffer);
        }
        else if (strncmp(buffer, "<updatetracker", 14) == 0) {
            handle_updatetracker(client_fd, buffer);
        }
        else if (strncmp(buffer, "<REQ LIST>", 10) == 0) {
            handle_list(client_fd);
        }
        else if (strncmp(buffer, "<GET", 4) == 0) {
            handle_get(client_fd, buffer);
        }
        else {
            write(client_fd, "Invalid command\n", 16);
}
    }

    close(client_fd);

    return NULL;

}



int main () {
    int port;
    char shared_dir[256];

    FILE *fp = fopen("sconfig","r");

    if(fp == NULL){
        perror("Error opeing sconfig");
        return 1;
    }

    fscanf(fp, "%d %s", &port,shared_dir);
    fclose(fp);

    //printf("Port: %d\n", port);
    //printf("Shared Dir: %s\n", shared_dir);

    int server_fd = socket(AF_INET,SOCK_STREAM,0);

    if(server_fd < 0){
        perror("Socket creation failed");
        return 1;
    }

    //printf("Socket created successfully\n");

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    

    if(bind(server_fd,(struct sockaddr*)&addr,sizeof(addr)) < 0){
        perror("Bind failed");
        return 1;
    }

    //printf("Bind successful on port %d\n", port);


    if(listen(server_fd, 10) < 0){
        perror("Listen failed");
        
        return 1;
    }

    printf("Server is listening...\n");

    while(1){
        int client_fd = accept(server_fd,NULL,NULL);

        if(client_fd < 0){
            perror("Accept failed");
            continue;
        }

        printf("Client connected!\n");


        int *pclient = malloc(sizeof(int));
        *pclient = client_fd;

        pthread_t tid;
        pthread_create(&tid,NULL,handle_client,pclient);
        pthread_detach(tid);
    }
    

    return 0;
}