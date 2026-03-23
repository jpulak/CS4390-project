//sample tracker to test out downloader
// run tracker before downloader
// gcc tracker_test.c -o tracker_test -lpthread ./tracker_test
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 3490
#define BUFFER_SIZE 1024

void* handle_client(void* arg) 
{
    int client_fd = *(int*)arg;
    free(arg);

    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    int n = read(client_fd, buffer, BUFFER_SIZE - 1);
    if (n > 0) 
    {
        buffer[n] = '\0';
        printf("[TRACKER] Received: %s\n", buffer);

        //request list
        if (strstr(buffer, "<REQ LIST>")) 
        {
            char msg[BUFFER_SIZE];
            sprintf(msg, "<REP LIST 1>\nTEST_FILE.txt\n<REP LIST END>\n");
            write(client_fd, msg, strlen(msg));
            printf("[TRACKER] Sent file list\n");
        }

        //request get
        else if (strstr(buffer, "<GET")) 
        {
            char msg[BUFFER_SIZE];
            sprintf(msg, "<REP GET BEGIN>\n");

            write(client_fd, msg, strlen(msg));

            //send file contents
            FILE *fp = fopen("TEST_FILE.txt", "r");
            if (fp) {
                while (fgets(msg, BUFFER_SIZE, fp)) 
                {
                    write(client_fd, msg, strlen(msg));
                }
                fclose(fp);
            }

            //send of file
            sprintf(msg, "<REP GET END TEST_FILE.txt>\n");
            write(client_fd, msg, strlen(msg));
            printf("[TRACKER] Sent file contents\n");
        }
    }
    close(client_fd);
    return NULL;
}

int main() 
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("Socket creation failed"); return 1; }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) 
    {
        perror("Bind failed"); return 1;
    }

    if (listen(server_fd, 5) < 0) 
    {
        perror("Listen failed"); return 1;
    }

    printf("[TRACKER] Running on port %d\n", PORT);

    while (1) 
    {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) { perror("Accept failed"); continue; }

        printf("[TRACKER] Client connected!\n");

        int *pclient = malloc(sizeof(int));
        *pclient = client_fd;

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, pclient);
        pthread_detach(tid);
    }

    close(server_fd);
    return 0;
}