// peer.c - peer client program
// hector gonzalez - CS 4390 group 1
// connects to the tracker server and lets you send commands

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAXBUF 4096

// config stuff - gets filled in from the config file
int tracker_port;
char tracker_ip[64];
int update_interval;
char peer_id[32] = "Peer";

// reads clientThreadConfig.cfg
// line 1 = tracker port, line 2 = tracker ip, line 3 = update interval
int read_config(const char* path) {
    FILE* fp = fopen(path, "r");
    if (!fp) {
        printf("cant open config: %s\n", path);
        return -1;
    }

    char line[256];
    int field = 0;

    while (fgets(line, sizeof(line), fp) && field < 3) {
        line[strcspn(line, "\n")] = '\0';
        if (line[0] == '#' || line[0] == '\0') continue;

        if (field == 0) tracker_port = atoi(line);
        else if (field == 1) strncpy(tracker_ip, line, sizeof(tracker_ip) - 1);
        else if (field == 2) update_interval = atoi(line);
        field++;
    }

    fclose(fp);
    printf("%s: config loaded - tracker=%s:%d, interval=%ds\n",
           peer_id, tracker_ip, tracker_port, update_interval);
    return 0;
}

// connects to the tracker, returns socket fd or -1
int connect_to_tracker() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("%s: socket failed\n", peer_id);
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(tracker_port);
    inet_pton(AF_INET, tracker_ip, &addr.sin_addr);

    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        printf("%s: cant connect to tracker\n", peer_id);
        close(sockfd);
        return -1;
    }

    return sockfd;
}

// sends msg to tracker, puts response in resp buffer
// returns bytes received or -1
int send_recv(const char* msg, char* resp, int resp_size) {
    int sockfd = connect_to_tracker();
    if (sockfd < 0) return -1;

    write(sockfd, msg, strlen(msg));

    memset(resp, 0, resp_size);
    int total = 0, n;
    while ((n = read(sockfd, resp + total, resp_size - total - 1)) > 0) {
        total += n;
    }

    close(sockfd);
    return total;
}

// --- command handlers ---

void do_list() {
    char resp[MAXBUF];
    if (send_recv("<REQ LIST>\n", resp, sizeof(resp)) > 0) {
        printf("%s: LIST response:\n%s", peer_id, resp);
    } else {
        printf("%s: LIST failed\n", peer_id);
    }
}

void do_get(char* args) {
    char trackfile[256];
    if (sscanf(args, "%s", trackfile) != 1) {
        printf("usage: GET <filename.track>\n");
        return;
    }

    char msg[MAXBUF];
    snprintf(msg, sizeof(msg), "<GET %s>\n", trackfile);

    char resp[MAXBUF];
    printf("%s: GET %s\n", peer_id, trackfile);

    if (send_recv(msg, resp, sizeof(resp)) > 0) {
        printf("%s: response:\n%s", peer_id, resp);

        // save tracker file locally
        char path[512];
        snprintf(path, sizeof(path), "shared/%s", trackfile);
        FILE* fp = fopen(path, "w");
        if (fp) {
            fprintf(fp, "%s", resp);
            fclose(fp);
            printf("%s: saved to %s\n", peer_id, path);
        }
    } else {
        printf("%s: GET failed\n", peer_id);
    }
}

void do_createtracker(char* args) {
    char fname[256], desc[256], md5[64], ip[64];
    long fsize;
    int port;

    if (sscanf(args, "%s %ld %s %s %s %d",
               fname, &fsize, desc, md5, ip, &port) != 6) {
        printf("usage: createtracker <file> <size> <desc> <md5> <ip> <port>\n");
        return;
    }

    char msg[MAXBUF];
    snprintf(msg, sizeof(msg), "<createtracker %s %ld %s %s %s %d>\n",
             fname, fsize, desc, md5, ip, port);

    char resp[MAXBUF];
    printf("%s: createtracker %s\n", peer_id, fname);

    if (send_recv(msg, resp, sizeof(resp)) > 0)
        printf("%s: %s", peer_id, resp);
    else
        printf("%s: createtracker failed\n", peer_id);
}

void do_updatetracker(char* args) {
    char fname[256], ip[64];
    long start, end;
    int port;

    if (sscanf(args, "%s %ld %ld %s %d",
               fname, &start, &end, ip, &port) != 5) {
        printf("usage: updatetracker <file> <start> <end> <ip> <port>\n");
        return;
    }

    char msg[MAXBUF];
    snprintf(msg, sizeof(msg), "<updatetracker %s %ld %ld %s %d>\n",
             fname, start, end, ip, port);

    char resp[MAXBUF];
    printf("%s: updatetracker %s\n", peer_id, fname);

    if (send_recv(msg, resp, sizeof(resp)) > 0)
        printf("%s: %s", peer_id, resp);
    else
        printf("%s: updatetracker failed\n", peer_id);
}

// background thread - sends periodic updates
void* update_timer(void* arg) {
    while (1) {
        sleep(update_interval);
        printf("%s: [update tick]\n", peer_id);
        // TODO: loop thru shared files and send updatetracker for each
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc >= 2)
        strncpy(peer_id, argv[1], sizeof(peer_id) - 1);

    printf("=== %s starting ===\n", peer_id);

    if (read_config("clientThreadConfig.cfg") != 0) {
        printf("bad config, exiting\n");
        exit(1);
    }

    // start background update timer
    pthread_t tid;
    pthread_create(&tid, NULL, update_timer, NULL);
    pthread_detach(tid);

    // command loop
    printf("%s> ", peer_id);
    fflush(stdout);

    char input[MAXBUF];
    while (fgets(input, sizeof(input), stdin)) {
        input[strcspn(input, "\n")] = '\0';

        if (strlen(input) == 0) {
            printf("%s> ", peer_id);
            fflush(stdout);
            continue;
        }

        if (strcmp(input, "quit") == 0) break;
        else if (strcmp(input, "LIST") == 0) do_list();
        else if (strncmp(input, "GET ", 4) == 0) do_get(input + 4);
        else if (strncmp(input, "createtracker ", 14) == 0) do_createtracker(input + 14);
        else if (strncmp(input, "updatetracker ", 14) == 0) do_updatetracker(input + 14);
        else printf("unknown command: %s\n", input);

        printf("%s> ", peer_id);
        fflush(stdout);
    }

    printf("%s terminated\n", peer_id);
    return 0;
}
