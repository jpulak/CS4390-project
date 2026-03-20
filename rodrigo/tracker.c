#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>


void* handle_client(void* arg){
    int client_fd = *(int*)arg;
    free(arg);

    char buffer[1024];

    int n = read(client_fd,buffer,sizeof(buffer) -1);

    if(n > 0){
        buffer[n] = '\0';
        printf("Received from client: %s\n", buffer);
        
        write(client_fd, "ACK\n",4);
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
        perror("Listen failed");printf("Server is listening...\n");
        return 1;
    }

    //printf("Server is listening...\n");

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