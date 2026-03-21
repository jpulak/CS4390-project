#ifndef PEER_SERVER_H
#define PEER_SERVER_H
#include "file_utils.h"
#define CONFIG_FILE "serverThreadConfig.cfg"

typedef struct {
    int  port;
    char shared_folder[MAX_PATH_LEN];
} ServerConfig;

//R ads serverThreadConfig.cfg, spawns the listening thread. Call once from main(). Returns 0 on success, -1 on error
int start_peer_server(const char *config_path);

//Exposed in header so tests can call it directly
void *client_handler(void *arg);

#endif