
//header guards
#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <stdio.h>
#include "protocol.h"

// download file from the tracker/peer
//host is the IP address of tracker/peer
//port is port number
// 0 on success -1 if failure

int download_file(const char* host, int port, const char* filename);

#endif //end guard