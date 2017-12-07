#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#include "log.h"

void log_write(int fd, int count, ...)
{
    va_list list;

    va_start(list, count);

    time_t t = time(NULL);
    struct tm systime = *localtime(&t);

    // create a buffer and store the parameters sent to 
    // this function inside it, after it write the buffer
    // to the log file
    char log[1000] = { 0 };

    sprintf(log, "[ %6d  %d:%d:%d ] ", getpid(), systime.tm_hour, systime.tm_min, systime.tm_sec);

    for(int i = 0; i < count; i++)
    {
        sprintf(log + strlen(log), "%s", va_arg(list, const char*));
    }
    sprintf(log + strlen(log), "\n");

    va_end(list);

    int ret;
    while ((ret = write(fd, log, strlen(log))) == -1)
    {
        // check if the write would block,
        // retry if needed
        if (ret == EAGAIN || EWOULDBLOCK)
        {
            continue;
        }
        // some error
        else
        {
            return;
        }
    }
}