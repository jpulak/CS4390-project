#include "peer_server.h"
#include "file_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


static ServerConfig g_config;

static int parse_config(const char *path, ServerConfig *cfg)
{
    FILE *f = fopen(path, "r");
    if (!f) { perror("parse_config: fopen"); return -1; }

    char line[256];
    cfg->port = 0;
    cfg->shared_folder[0] = '\0';

    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = '\0';
        char key[128], val[256];

        if (sscanf(line, "%127[^=]=%255s", key, val) == 2) {
            if      (strcmp(key, "port")          == 0) cfg->port = atoi(val);
            else if (strcmp(key, "shared_folder") == 0)
                strncpy(cfg->shared_folder, val, MAX_PATH_LEN - 1);
        }
    }
    fclose(f);

    if (cfg->port <= 0 || cfg->shared_folder[0] == '\0') {
        fprintf(stderr, "parse_config: missing port or shared_folder in %s\n", path);
        return -1;
    }
    return 0;
}

static int recv_line(int fd, char *buf, int maxlen)
{
    int total = 0;
    while (total < maxlen - 1) {
        char c;
        int n = (int)recv(fd, &c, 1, 0);
        if (n < 0) return -1;
        if (n == 0) break;          /* EOF */
        buf[total++] = c;
        if (c == '\n') break;
    }
    buf[total] = '\0';
    return total;
}

static int send_all(int fd, const char *buf, int len)
{
    int sent = 0;
    while (sent < len) {
        int n = (int)send(fd, buf + sent, (size_t)(len - sent), 0);
        if (n <= 0) return -1;
        sent += n;
    }
    return 0;
}

typedef struct {
    int  fd;
    char ip[INET_ADDRSTRLEN];
    int  port;
} ClientArgs;

void *client_handler(void *arg)
{
    ClientArgs *ca  = (ClientArgs *)arg;
    int         fd  = ca->fd;
    char        ip[INET_ADDRSTRLEN];
    int         cli_port = ca->port;
    strncpy(ip, ca->ip, INET_ADDRSTRLEN);
    free(ca);

    //Receive the request line
    char req[512];
    if (recv_line(fd, req, sizeof(req)) <= 0) {
        close(fd);
        return NULL;
    }

    //Parse
    char filename[MAX_FILENAME_LEN];
    long offset;
    int  length;

    if (sscanf(req, "GET %255s %ld %d", filename, &offset, &length) != 3) {
        fprintf(stderr, "[SERVER] Bad request from %s:%d: %s", ip, cli_port, req);
        send_all(fd, "<GET invalid>\n", 14);
        close(fd);
        return NULL;
    }

    // Security: block path traversal
    if (strstr(filename, "..") != NULL || strchr(filename, '/') != NULL) {
        fprintf(stderr, "[SERVER] Path traversal attempt from %s:%d: %s\n",
                ip, cli_port, filename);
        send_all(fd, "<GET invalid>\n", 14);
        close(fd);
        return NULL;
    }

    //Enforce 1024-byte limit
    if (length <= 0 || length > MAX_CHUNK_SIZE) {
        fprintf(stderr, "[SERVER] Chunk size %d rejected from %s:%d\n",
                length, ip, cli_port);
        send_all(fd, "<GET invalid>\n", 14);
        close(fd);
        return NULL;
    }

    // Build full path
    char filepath[MAX_PATH_LEN * 2];
    snprintf(filepath, sizeof(filepath), "%s/%s",
             g_config.shared_folder, filename);

    // Read chunk from disk
    char data[MAX_CHUNK_SIZE];
    int  n = read_chunk(filepath, offset, data, length);
    if (n <= 0) {
        fprintf(stderr, "[SERVER] read_chunk failed for %s offset %ld\n",
                filename, offset);
        send_all(fd, "<GET invalid>\n", 14);
        close(fd);
        return NULL;
    }

    //Log it
     printf("[SERVER] Serving bytes %ld-%ld of %s to %s:%d\n",
           offset, offset + n - 1, filename, ip, cli_port);

    //Send response header then data
    char header[64];
    snprintf(header, sizeof(header), "<GET ok %d>\n", n);
    send_all(fd, header, (int)strlen(header));
    send_all(fd, data, n);

    close(fd);
    return NULL;
}

static void *server_thread(void *arg)
{
    (void)arg;

    int srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0) { perror("socket"); return NULL; }

    // Allow reuse so we can restart without waiting for TIME_WAIT 
    int yes = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in addr = {0};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons((uint16_t)g_config.port);

    if (bind(srv, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(srv); return NULL;
    }
    if (listen(srv, 16) < 0) {
        perror("listen"); close(srv); return NULL;
    }

    printf("[SERVER] Listening on port %d | shared: %s\n",
           g_config.port, g_config.shared_folder);

    while (1) {
        struct sockaddr_in cli_addr;
        socklen_t cli_len = sizeof(cli_addr);

        int cli_fd = accept(srv, (struct sockaddr *)&cli_addr, &cli_len);
        if (cli_fd < 0) {
            if (errno == EINTR) continue;   /* interrupted by signal, retry */
            perror("accept");
            break;
        }

        // Pack client info to pass to the handler thread
        ClientArgs *ca = malloc(sizeof(ClientArgs));
        if (!ca) { close(cli_fd); continue; }

        ca->fd   = cli_fd;
        ca->port = ntohs(cli_addr.sin_port);
        inet_ntop(AF_INET, &cli_addr.sin_addr, ca->ip, INET_ADDRSTRLEN);

        // Spawn a detached thread – no need to join it later
        pthread_t tid;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

        if (pthread_create(&tid, &attr, client_handler, ca) != 0) {
            perror("pthread_create");
            free(ca);
            close(cli_fd);
        }
        pthread_attr_destroy(&attr);
    }

    close(srv);
    return NULL;
}

int start_peer_server(const char *config_path)
{
    if (parse_config(config_path, &g_config) != 0) return -1;

    pthread_t tid;
    if (pthread_create(&tid, NULL, server_thread, NULL) != 0) {
        perror("start_peer_server: pthread_create");
        return -1;
    }
    pthread_detach(tid);   
    return 0;
}
