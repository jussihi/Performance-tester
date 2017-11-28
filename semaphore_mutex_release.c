#include <stdio.h>      // printf, 
#include <time.h>       // struct timespec, clock_gettime()
#include <stdlib.h>     // malloc
#include <sys/mman.h>   // mmap
#include <sys/wait.h>   // wait
#include <unistd.h>     // sysconf
#include <pthread.h>    // threading
#include <semaphore.h>  // semaphore
#include <fcntl.h>      // O_CREAT

#include "tests.h"
#include "testutils.h"

void* mutex_keeper(void* w_args)
{
    
    return NULL;
}

void* semaphore_keeper(void* w_args)
{
    struct sema_mutex_args_struct* args = (struct sema_mutex_args_struct*)w_args;
    sem_t* id = args->id;
    int* status = args->status;

    while(*status != -1)
    {
        //printf("waiting for semaphore in thread\n");
        sem_wait(id);
        clock_gettime(CLOCK_REALTIME, start);
        //printf("new clock\n");
        sem_post(id);
        //printf("posted\n");
        // give main thread some time to recover from calculations
        usleep(20);
        //printf("posted semaphore in thread\n");
    }
    return NULL;
}

void semaphore_mutex_not_empty(int w_iterations)
{
    // start = malloc(sizeof(struct timespec));
    start  = mmap(NULL, sizeof(struct timespec), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    finish = mmap(NULL, sizeof(struct timespec), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);

    sem_t* id;

    int* status = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    *status = 1;

    if((id = sem_open("threadsem", O_CREAT, 0600, 0)) == SEM_FAILED)
    {
        perror("sem_open");
    }

    sem_post(id);

    struct sema_mutex_args_struct args;
    args.id = id;
    args.status = status;

    pthread_t thread_creation;

    pthread_create(&thread_creation, NULL, semaphore_keeper, (void*)&args);

    double totSemaphore = 0.0;

    usleep(1000);

    for(int i = 0; i < w_iterations; i++)
    {
        //usleep(100);
        sem_wait(id);
        //printf("got sem\n");
        clock_gettime(CLOCK_REALTIME, finish);
        //double thisTime = (finish->tv_nsec - start->tv_nsec);
        totSemaphore += (finish->tv_nsec - start->tv_nsec);
        //printf("%f\n", thisTime);
        sem_post(id);
        usleep(10);
    }
    // very ugly
    *status = -1;
    pthread_join(thread_creation, NULL);
    printf("Total time getting released semaphore was: %f ms\n", totSemaphore / 1000000);
    printf("Mean time getting a released semaphore was: %f us\n", totSemaphore / (w_iterations * 1000));

    // calculate mutex times here
    pthread_create(&thread_creation, NULL, mutex_keeper, NULL);

    pthread_join(thread_creation, NULL);
}