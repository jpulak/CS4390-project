#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 5001
#define BUFFER_SIZE 1024

int main() {
    int server_fd, client_sock;
    struct sockaddr_in address;
    char buffer[BUFFER_SIZE];

    FILE *fp = fopen("test.txt", "rb"); // file to serve
    if (!fp) {
        perror("file not found");
        return 1;
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 5);

    printf("Peer server running on port %d...\n", PORT);

    while (1) {
        client_sock = accept(server_fd, NULL, NULL);

        memset(buffer, 0, BUFFER_SIZE);
        recv(client_sock, buffer, BUFFER_SIZE, 0);

        // parse request: <GET filename start end>
        char filename[256];
        long start, end;
        sscanf(buffer, "<GET %s %ld %ld>", filename, &start, &end);

        fseek(fp, start, SEEK_SET);

        long bytes_to_send = end - start;
        while (bytes_to_send > 0) {
            int n = fread(buffer, 1, BUFFER_SIZE, fp);
            if (n <= 0) break;

            send(client_sock, buffer, n, 0);
            bytes_to_send -= n;
        }

        close(client_sock);
        rewind(fp);
    }

    fclose(fp);
    return 0;
}
