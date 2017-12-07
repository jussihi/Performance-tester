#include <sys/types.h>      // O_RDONLY, O_WRONLY
#include <time.h>           // struct timespec, clock_gettime()
#include <sys/stat.h>       // mkdir
#include <sys/mman.h>       // mmap
#include <fcntl.h>          // open
#include <unistd.h>         // close
#include <limits.h>         // PIPE_BUF
#include <stdio.h>          // printf, sprintf
#include <stdlib.h>         // NULL, exit
#include <pthread.h>        // threading, barriers
#include <string.h>         // memcpy

#include "testutils.h"      // start, finish time structs

#include "log.h"

static int log_fd;

int remove_dummyfile()
{
    char filename[17] = {0};
    int sprintfret = sprintf(filename, "%s", "dummyfiles/dummy");
    if(sprintfret < 0)
    {
        perror("sprintf");
        return -1;
    }
    if(remove(filename))
    {
        perror("remove");
        return -2;
    }
    if(remove("dummyfiles"))
    {       
        perror("remove");
        return -3;
    }
    return 0;
}

int generate_file(int w_fileSize)
{
    if(mkdir("dummyfiles", S_IRWXU | S_IRWXO))
    {
        perror("mkdir");
        return -1;
    }
    
    char filename[17] = { 0 };
    int sprintfret = sprintf(filename, "%s", "dummyfiles/dummy");
    if(sprintfret < 0)
    {
        perror("sprintf");
        return -2;
    }
    FILE *fp=fopen(filename, "w");
    if(!fp)
    {
        perror("fopen");
        return -3;
    }
    int trunc = ftruncate(fileno(fp), 1000*1000*100);
    if(trunc)
    {
        perror("ftruncate");
        return -4;
    }
    int fcloseret = fclose(fp);
    if(fcloseret)
    {
        perror("fclose");
        return -5;
    }

    return 0;
}

int pipe_transfer(int w_bufferSize, int w_fileSize)
{
    int bufferSize = w_bufferSize;
    if(w_bufferSize > PIPE_BUF || w_bufferSize < 1)
    {
        printf("Buffer size of %d bytes was too large. Using maximum value of %d\n", w_bufferSize, PIPE_BUF);
        bufferSize = PIPE_BUF;
    }

    // create pipe
    int pipefd[2];
    
    if(pipe(pipefd)) return -1;

    start  = mmap(NULL, sizeof(struct timespec), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    finish = mmap(NULL, sizeof(struct timespec), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);

    if(start == MAP_FAILED || finish == MAP_FAILED) return -2;

    // set pipe buffer size
    if((fcntl(pipefd[0], F_SETPIPE_SZ, bufferSize)) != bufferSize)
    {
        munmap(start, sizeof(struct timespec));
        munmap(finish, sizeof(struct timespec));
        return -3;
    }

    pid_t child;

    if((child = fork()) == 0)
    {
        clock_gettime(CLOCK_REALTIME, start);
        // child, here we send the data to parent.
        FILE* fp = fopen("dummyfiles/dummy", "rb");
        if(!fp)
        {
            perror("File opening failed.");
            exit(0);
        }

        int charsRead = 0;

        char buffer[bufferSize];

        // write loop
        charsRead = fread(buffer, 1, bufferSize, fp);
        do
        {
            if((write(pipefd[1], buffer, charsRead)) == -1)
            {
                perror("write returned an error :)");
            }
        } while((charsRead = fread(buffer, 1, bufferSize, fp)) == bufferSize);

        if((write(pipefd[1], buffer, charsRead)) == -1)
        {
            perror("write");
            exit(0);
        }
        
        int fcloseret = fclose(fp);
        if(fcloseret)
        {
            perror("fclose");
            exit(0);
        }
        
        close(pipefd[1]);

        exit(0);
    }
    else
    {
        // parent, we receive the file here
        char buffer[bufferSize];

        FILE* fp = fopen("dummyfiles/dummywrite", "wb");
        if(!fp)
        {
            perror("File opening failed.");
            return -1;
        }

        int charsWritten = 0;

        int readret;
        
        if((readret = read(pipefd[0], buffer, bufferSize)) != bufferSize)
        {
            perror("read");
            return -1;
        }
        // reading loop, read until not getting bufferSize bytes
        do
        {
            if(readret != bufferSize) printf("Something went wrong, please restart the test.\n");
            if((fwrite(buffer, 1, bufferSize, fp)) != bufferSize)
            {
                perror("fwrite");
            }
        } while((readret = read(pipefd[0], buffer, bufferSize)) == bufferSize);
        
        if((charsWritten = fwrite(buffer, 1, readret, fp)) != readret)
        {
            perror("fwrite");
        }

        fclose(fp);

        clock_gettime(CLOCK_REALTIME, finish);
    }
    double total = (finish->tv_nsec - start->tv_nsec);
    printf("Pipe\t\ttransferring %d MiB file with %d byte-sized buffers takes %.2f us. That's %f MiB/s.\n", w_fileSize, w_bufferSize, (double)(total / 1000), (1 / ((double)total / 1000000000)) * w_fileSize);
    

    // remove the copied file
    if(remove("dummyfiles/dummywrite"))
    {
        perror("remove");
        munmap(start, sizeof(struct timespec));
        munmap(finish, sizeof(struct timespec));
        return -1;
    }

    munmap(start, sizeof(struct timespec));
    munmap(finish, sizeof(struct timespec));

    return 0;
}

void* shared_memory_sender(void* w_args)
{
    struct shared_memory_args_struct* args = (struct shared_memory_args_struct*)w_args;

    FILE* fp = fopen("dummyfiles/dummy", "rb");

    if(!fp)
    {
        perror("File opening failed.");
        return NULL;
    }

    int charsRead = 0;
    char buffer[*args->bufferSize];

    charsRead = fread(buffer, 1, *args->bufferSize, fp);
    do
    {
        pthread_barrier_wait(args->barr);
        *(args->bytesToRead) = charsRead;
        memcpy(args->filebuff, buffer, *args->bufferSize);
        pthread_barrier_wait(args->barr);
    } while((charsRead = fread(buffer, 1, *args->bufferSize, fp)) == *args->bufferSize);
    
    pthread_barrier_wait(args->barr);
    *(args->bytesToRead) = charsRead;
    memcpy(args->filebuff, buffer, charsRead);

    fclose(fp);

    return NULL;
}

int shared_memory_transfer(int w_bufferSize, int w_fileSize)
{
    int tempBuf = w_bufferSize;
    if(w_bufferSize < 1)
    {
        printf("Invalid buffer size for shared memory transfer. Using default size of %d bytes.\n", PIPE_BUF);
        tempBuf = PIPE_BUF;
    }

    pthread_barrier_t* barr;
    unsigned char* filebuff;
    unsigned int* bytesToRead;
    int* bufferSize;

    barr = mmap(NULL, sizeof(pthread_barrier_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    filebuff = mmap(NULL, tempBuf, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    start  = mmap(NULL, sizeof(struct timespec), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    finish = mmap(NULL, sizeof(struct timespec), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    bytesToRead = mmap(NULL, sizeof(unsigned int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    bufferSize = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);

    if(barr == MAP_FAILED || filebuff == MAP_FAILED || start == MAP_FAILED || finish == MAP_FAILED || bytesToRead == MAP_FAILED || bufferSize == MAP_FAILED)
    {
        return -1;
    }

    *bufferSize = tempBuf;

    pthread_barrier_init(barr, NULL, 2);

    struct shared_memory_args_struct args;
    args.barr = barr;
    args.bytesToRead = bytesToRead;
    args.filebuff = filebuff;
    args.bufferSize = bufferSize;

    pthread_t threadCreation;

    clock_gettime(CLOCK_REALTIME, start);
    pthread_create(&threadCreation, NULL, shared_memory_sender, (void*)&args);

    // in parent, we receive file here
    FILE* fp = fopen("dummyfiles/dummywrite", "wb");

    int readBytes;

    if(!fp)
    {
        perror("File opening failed.");
        return -1;
    }
    pthread_barrier_wait(barr);
    do
    {
        pthread_barrier_wait(barr);
        pthread_barrier_wait(barr);
    } while((readBytes = fwrite(filebuff, 1, *bytesToRead, fp)) == *bufferSize);

    fclose(fp);

    clock_gettime(CLOCK_REALTIME, finish);

    long total = (finish->tv_nsec - start->tv_nsec);
    printf("\nShared memory\ttransferring %d MiB file with %d byte-sized buffers takes %.2f us. That's %f MiB/s.\n", w_fileSize, w_bufferSize, (double)(total / 1000), (1 / ((double)total / 1000000000)) * w_fileSize);
    

    // remove the copied file
    if(remove("dummyfiles/dummywrite"))
    {
        perror("remove");
        munmap(start, sizeof(struct timespec));
        munmap(finish, sizeof(struct timespec));
        return -1;
    }

    pthread_join(threadCreation, NULL);

    munmap(barr, sizeof(pthread_barrier_t));
    munmap(filebuff, *bufferSize);
    munmap(start, sizeof(struct timespec));
    munmap(finish, sizeof(struct timespec));
    munmap(bytesToRead, sizeof(unsigned int));
    munmap(bufferSize, sizeof(int));

    return 0;
}

int message_queue_transfer()
{
    return 0;
}

void file_transfer(int w_fileSize, int w_BufferSize, int w_dropCache)
{
    log_fd = open("performance_tester.log", O_RDWR | O_APPEND | O_CREAT | O_NONBLOCK, S_IRWXU);
    log_write(log_fd, 1, "File transfer speed measurement with pipes & shared memory routine done.");
    printf("\nMEASURING PIPE vs SHARED MEMORY FILE TRANSFER SPEED\n");
    if(generate_file(w_fileSize))
    {
        perror("generating dummy file failed. Please try to remove the dummyfile folder by hand");
        return;
    }

    if(shared_memory_transfer(w_BufferSize, w_fileSize)) perror("shared_memory_transfer");

    if(pipe_transfer(w_BufferSize, w_fileSize)) perror("pipe_transfer");

    if(remove_dummyfile())
    {
        perror("generating dummy file failed. Please try to remove the dummyfile folder by hand");
        return;
    }
    close(log_fd);
    return;
}