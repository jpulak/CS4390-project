#ifndef PROTOCOL_H
#define PROTOCOL_H

// Buffer size
#define MAX_BUF 1024

// pack funcs

// createtracker
int pack_createtracker(char* buf, const char* filename, long filesize,
                       const char* desc, const char* md5,
                       const char* ip, int port);

// list
int pack_list(char* buf);

// get
int pack_get(char* buf, const char* filename);

// updatetracker
int pack_updatetracker(char* buf, const char* filename,
                       long start, long end,
                       const char* ip, int port);

//pase funcs

// createtracker
int parse_createtracker(const char* msg, char* filename, long* filesize,
                        char* desc, char* md5, char* ip, int* port);

#endif