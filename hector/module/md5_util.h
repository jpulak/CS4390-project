#ifndef MD5_UTIL_H
#define MD5_UTIL_H

char* compute_file_md5(const char* filepath);
char* compute_buffer_md5(const char* buf, int len);


#endif