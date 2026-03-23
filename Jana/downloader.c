
//implement of downloader
#include "downloader.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFFER_SIZE 1024

//set up tcp comms with server
int download_file(const char* host, int port, const char* filename) 
{
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    int writing = 0; //flag 

    //creates tcp socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) 
    {
        perror("socket failed");
        return -1; // if failed
    }

    //connect tracker or peer over tcp
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &server_addr.sin_addr);
    //call connect ot he t racker/peer
    if(connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) 
    {
        perror("connect failed");
        close(sockfd);
        return -1;
    }

    //send get request using protocol
    pack_get(buffer, filename);
    send(sockfd, buffer, strlen(buffer), 0);

    //open file and save content
    FILE *fp = fopen("downloaded.txt", "w");
    if(!fp) 
    {
        perror("fopen failed");
        close(sockfd);
        return -1;
    }

    //read data from socket into buffer
    //loop until connection close or no more data
    while(1) 
    {
        memset(buffer, 0, BUFFER_SIZE);
        int n = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
        if(n <= 0) break;
        buffer[n] = '\0';
        // find protocol markers and execute accordingly
        if(strstr(buffer, REP_GET_BEGIN) != NULL) 
        {
            writing = 1;
            continue;
        }

        if(strstr(buffer, REP_GET_END) != NULL) 
        {
            break;
        }

        if(writing) 
        {
            fprintf(fp, "%s", buffer);
        }
    }
    //clsoe file and socekt
    fclose(fp);
    close(sockfd);
    return 0; //success!
}