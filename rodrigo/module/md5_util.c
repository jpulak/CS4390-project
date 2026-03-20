#include <stdio.h>      
#include <stdlib.h>    
#include <string.h>     
#include <openssl/md5.h> 
#include "md5_util.h"


static char* md5_to_hex(const unsigned char* raw){
    char* result = malloc(33);
    if(result == NULL){
        return NULL; // memory could not be allcated
    }

    for(int i = 0;i<16;i++){
        sprintf(&result[i*2], "%02X", raw[i]);

    }

    result[32] = '\0'; //null ter



    return result; // MAKE SURE TO FREE AFTER USE !!!!!
}

char* compute_buffer_md5(const char* buf, int len){
   
    unsigned char raw[16]; // raw MD5 output
    MD5((unsigned char*)buf,len,raw);
    return md5_to_hex(raw);
}

char* compute_file_md5(const char* filepath){

    FILE* file = fopen(filepath, "rb");

    if(!file){
        return NULL; //file did not open
    }
    
    MD5_CTX ctx;
    MD5_Init(&ctx);

    unsigned char buffer[1024]; // read 1024 bytes at a time
    size_t bytesToRead;

    while((bytesToRead = fread(buffer,1, sizeof(buffer),file))> 0){
        MD5_Update(&ctx,buffer,bytesToRead);
    }


    fclose(file);

    unsigned char raw[16]; 
    MD5_Final(raw,&ctx);

    return md5_to_hex(raw);
}

