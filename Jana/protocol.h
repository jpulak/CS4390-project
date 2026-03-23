//header file for protocol
//define consts, func signatures and structures

//header guard
#ifndef PROTOCOL_H
#define PROTOCOL_H

//define buffer size
#define MAX_BUF 1024

//commands to send from peer to tracker/peer to peer

#define CMD_CREATETRACKER "<createtracker"
#define CMD_UPDATETRACKER "<updatetracker"
#define CMD_LIST "<REQ LIST>"
#define CMD_GET "<GET"

//responses- succes, failure, file error
#define RESP_SUCC "succ"
#define RESP_FAIL "fail"
#define RESP_FERR "ferr"

//messages from teacker/peer
#define REP_LIST_BEGIN "<REP LIST"
#define REP_LIST_END "<REP LIST END>"
#define REP_GET_BEGIN "<REP GET BEGIN>"
#define REP_GET_END "<REP GET END>"

//pack funcs, so build protocol strings to send per instructions
int pack_list(char* buf);
int pack_get(char* buf, const char* filename);
int pack_createtracker(char* buf, const char* filename, long filesize,
                       const char* desc, const char* md5,
                       const char* ip, int port);
int pack_updatetracker(char* buf, const char* filename,
                       const char* ip, int port,
                       long start_byte, long end_byte);

//parse funcs, read protocol messges from str into fields

int parse_list_begin(const char* msg, int* count);
int parse_list_file(const char* msg, char* filename);
int parse_get_begin(const char* msg, char* filename);
int parse_get_end(const char* msg, char* filename);
int parse_createtracker(const char* msg, char* filename, long* filesize,
                        char* desc, char* md5,
                        char* ip, int* port);
int parse_updatetracker(const char* msg, char* filename,
                        char* ip, int* port,
                        long* start_byte, long* end_byte);

#endif //end header