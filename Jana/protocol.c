// implementations of funcs declared in protocol.h

//imports
#include <stdio.h>
#include <string.h>
#include "protocol.h" //call protocol defs

//pack funcs

//packs LIST requests
int pack_list(char* buf) 
{
    return sprintf(buf, "%s\n", CMD_LIST);
}

//packs GET requests
int pack_get(char* buf, const char* filename) 
{
    return sprintf(buf, "%s %s\n", CMD_GET, filename);
}

//build createtracker message based on what is needed
int pack_createtracker(char* buf, const char* filename, long filesize,
                       const char* desc, const char* md5,
                       const char* ip, int port) 
                       {
    return sprintf(buf, "%s %s %ld %s %s %s %d\n",
                   CMD_CREATETRACKER, filename, filesize, desc, md5, ip, port);
}

// build the updatetracker based on what it needs
int pack_updatetracker(char* buf, const char* filename,
                       const char* ip, int port,
                       long start_byte, long end_byte) 
                       {
    return sprintf(buf, "%s %s %s %d %ld %ld\n",
                   CMD_UPDATETRACKER, filename, ip, port, start_byte, end_byte);
}

//parse funcs

//read the number of files from rep list header
int parse_list_begin(const char* msg, int* count) 
{
    return sscanf(msg, "<REP LIST %d>", count);
}

//read a single filename from a list
int parse_list_file(const char* msg, char* filename) 
{
    return sscanf(msg, "%s", filename);
}

//red filename form get begin
int parse_get_begin(const char* msg, char* filename) 
{
    return sscanf(msg, "<REP GET BEGIN %s>", filename);
}

//read filename from get end
int parse_get_end(const char* msg, char* filename) 
{
    return sscanf(msg, "<REP GET END %s>", filename);
}

//parse createtrager message into the fields
int parse_createtracker(const char* msg, char* filename, long* filesize,
                        char* desc, char* md5,
                        char* ip, int* port) 
                        {
    return sscanf(msg, "<createtracker %s %ld %s %s %s %d>",
                  filename, filesize, desc, md5, ip, port);
}

//parse updatetracker messge into field
int parse_updatetracker(const char* msg, char* filename,
                        char* ip, int* port,
                        long* start_byte, long* end_byte) 
                        {
    return sscanf(msg, "<updatetracker %s %s %d %ld %ld>",
                  filename, ip, port, start_byte, end_byte);
}