#ifndef TESTUTILS_HH_
#define TESTUTILS_HH_

#include <semaphore.h>  // semaphore

extern struct timespec* start;
extern struct timespec* finish;

extern double gettimeCached;
extern double gettimeNoCached;

struct sema_mutex_args_struct {
    sem_t* id;
    int* status;
    pthread_barrier_t* barr;
    pthread_mutex_t* mutex;
};

struct shared_memory_args_struct {
    pthread_barrier_t* barr;
    unsigned int* bytesToRead;
    unsigned char* filebuff;
    int* bufferSize;
};


#endif