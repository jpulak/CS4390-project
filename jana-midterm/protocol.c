#include <stdio.h>
#include <string.h>
#include "protocol.h"

//pack funcs

// createtracker
int pack_createtracker(char* buf, const char* filename, long filesize,
                       const char* desc, const char* md5,
                       const char* ip, int port)
{
    // build message
    return sprintf(buf, "<createtracker %s %ld %s %s %s %d>\n",
                   filename, filesize, desc, md5, ip, port);
}

// list
int pack_list(char* buf)
{
    return sprintf(buf, "<REQ LIST>\n");
}

// get
int pack_get(char* buf, const char* filename)
{
    return sprintf(buf, "<GET %s.track>\n", filename);
}

// updatetracker
int pack_updatetracker(char* buf, const char* filename,
                       long start, long end,
                       const char* ip, int port)
{
    return sprintf(buf, "<updatetracker %s %ld %ld %s %d>\n",
                   filename, start, end, ip, port);
}

//parse func

// createtracker
int parse_createtracker(const char* msg, char* filename, long* filesize,
                        char* desc, char* md5, char* ip, int* port)
{
    // extract values from message
    return sscanf(msg, "<createtracker %s %ld %s %s %s %d>",
                  filename, filesize, desc, md5, ip, port);
}