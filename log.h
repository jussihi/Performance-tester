#ifndef HH_LOG_C
#define HH_LOG_C 

#include <fcntl.h>      // O_CREAT
#include <unistd.h>     // sysconf

/*
 * Writes log entries into file descriptor.
 * This could've been done with same sort 
 * of extern variable but had no time any
 * more.
 */

void log_write(int fd, int count, ...);

#endif