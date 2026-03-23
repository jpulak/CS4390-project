//test out protocol files to see if they work
// gcc protocol.c test_protocol.c -o test_protocol and ./test_protocol

#include <stdio.h>
#include <string.h>
#include "protocol.h"

int main() 
{
    //declare vars
    char buf[MAX_BUF];
    int count;
    char filename[256];
    char desc[256], md5[64], ip[32];
    int port;
    long filesize, start, end;

    printf("TEST all protocols \n\n");

    //list test
    printf("Testing LIST pack/parse...\n");
    pack_list(buf);
    printf("Packed LIST: %s", buf);

    //list response test
    sprintf(buf, "<REP LIST 2>\nfile1.txt\nfile2.txt\n<REP LIST END>\n");
    printf("Simulated LIST Response:\n%s", buf);

    //parse list begin
    parse_list_begin(buf, &count);
    printf("Parsed list count: %d\n", count);

    //parse file entries
    char* line = strtok(buf, "\n");
    while(line) {
        if(strstr(line, "<REP LIST") == NULL && strstr(line, "<REP LIST END>") == NULL) {
            parse_list_file(line, filename);
            printf("Parsed file: %s\n", filename);
        }
        line = strtok(NULL, "\n");
    }

    printf("\n"); // new line

    //get test
    printf("Testing GET pack/parse...\n");
    pack_get(buf, "file1.txt");
    printf("Packed GET: %s", buf);

    //get begin simulateed
    sprintf(buf, "<REP GET BEGIN file1.txt>\n");
    parse_get_begin(buf, filename);
    printf("Parsed GET BEGIN filename: %s\n", filename);

    //get end simulated
    sprintf(buf, "<REP GET END file1.txt>\n");
    parse_get_end(buf, filename);
    printf("Parsed GET END filename: %s\n", filename);

    printf("\n");

    //create tracker 
    printf("Testing CREATETRACKER pack/parse...\n");
    pack_createtracker(buf, "movie.avi", 102400, "Ghost_and_the_Darkness_DVD_Rip", 
                       "abcd1234", "127.0.0.1", 3490);
    printf("Packed CREATETRACKER: %s", buf);

    parse_createtracker(buf, filename, &filesize, desc, md5, ip, &port);
    printf("Parsed CREATETRACKER:\n  filename: %s\n  filesize: %ld\n  desc: %s\n  md5: %s\n  ip: %s\n  port: %d\n",
           filename, filesize, desc, md5, ip, port);

    printf("\n");

    //update tracker
    printf("Testing UPDATETRACKER pack/parse...\n");
    pack_updatetracker(buf, "movie.avi", "127.0.0.1", 3490, 0, 1024);
    printf("Packed UPDATETRACKER: %s", buf);

    parse_updatetracker(buf, filename, ip, &port, &start, &end);
    printf("Parsed UPDATETRACKER:\n  filename: %s\n  ip: %s\n  port: %d\n  start: %ld\n  end: %ld\n",
           filename, ip, port, start, end);

    printf("\n End of protocol testing\n");
    return 0;
}