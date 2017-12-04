#ifndef TESTUTILS_HH_
#define TESTUTILS_HH_

#include <semaphore.h>  // semaphore

extern struct timespec* start;
extern struct timespec* finish;

extern double gettime_cached;
extern double gettime_nocached;


struct sema_mutex_args_struct {
    sem_t* id;
    int* status;
    pthread_barrier_t* barr;
    pthread_mutex_t* mutex;
};


#endif