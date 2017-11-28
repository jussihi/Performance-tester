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
    pthread_barrier_t* barr = args->barr;

    while(*status != -1)
    {
        pthread_barrier_wait(barr);
        if(*status == -1) break;
        if(sem_wait(id) != 0) perror("sem_wait child");
        pthread_barrier_wait(barr);
        clock_gettime(CLOCK_REALTIME, start);
        if(sem_post(id) != 0) printf("sem_post child");
    }
    return NULL;
}

void semaphore_mutex_not_empty(int w_iterations, int w_drop_cache)
{
    // start = malloc(sizeof(struct timespec));
    start  = mmap(NULL, sizeof(struct timespec), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    finish = mmap(NULL, sizeof(struct timespec), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);

    sem_t* id;

    int* status = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    *status = 4;

    // HUOM HUOM
    sem_unlink("releasesem");

    if((id = sem_open("releasesem", O_CREAT | O_EXCL, 0600, 0)) == SEM_FAILED)
    {
        perror("sem_open");
    }

    if(sem_post(id)) printf("asdloldaa");

    struct sema_mutex_args_struct args;
    args.id = id;
    args.status = status;
    args.barr = malloc(sizeof(pthread_barrier_t));

    pthread_barrier_init(args.barr, NULL, 2);

    pthread_t thread_creation;

    pthread_create(&thread_creation, NULL, semaphore_keeper, (void*)&args);

    double totSemaphoreCached = 0.0;

    for(int i = 0; i < w_iterations; i++)
    {
        pthread_barrier_wait(args.barr);
        pthread_barrier_wait(args.barr);
        if(sem_wait(id)) printf("WAIT FAILED ON PARENT");
        clock_gettime(CLOCK_REALTIME, finish);
        //double thisTime = (finish->tv_nsec - start->tv_nsec);
        if(i >= (w_iterations/5))
        {
            totSemaphoreCached += (finish->tv_nsec - start->tv_nsec);
        }
        //printf("%f\n", thisTime);
        if(sem_post(id)) printf("POST FAILED ON PARENT");
    }
    // very ugly
    *status = -1;
    pthread_barrier_wait(args.barr);
    pthread_join(thread_creation, NULL);

    printf("\nCached\t\tmean semaphore acquisition time after release with %d samples: %f ns\n", w_iterations - (w_iterations / 5), totSemaphoreCached / (w_iterations - (w_iterations / 5)));
    
    // calculate mutex times here
    pthread_create(&thread_creation, NULL, mutex_keeper, NULL);

    pthread_join(thread_creation, NULL);

    sem_close(id);
}