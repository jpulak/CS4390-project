// run tracker before running test_downloader
//cc downloader.c protocol.c test_downloader.c -o test_downloader -lpthread ./test_downloader
#include "downloader.h"
#include <stdio.h>

int main() 
{
    //call download file to download the testfile from local prot 3490
    printf("Downloading TEST_FILE.txt from tracker...\n");
    if(download_file("127.0.0.1", 3490, "TEST_FILE.txt") == 0) 
    {
        printf("Download succeeded! Check downloaded.txt\n"); //successful
    } 
    else 
    {
        printf("Download failed!\n"); //not successful
    }
    return 0;
}