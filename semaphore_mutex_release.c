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
    struct sema_mutex_args_struct* args = (struct sema_mutex_args_struct*)w_args;
    pthread_mutex_t* pmutex = args->mutex;
    int* status = args->status;
    pthread_barrier_t* barr = args->barr;

    while(*status != -1)
    {
        pthread_barrier_wait(barr);
        if(*status == -1) break;
        pthread_mutex_lock(pmutex);
        pthread_barrier_wait(barr);
        clock_gettime(CLOCK_REALTIME, start);
        pthread_mutex_unlock(pmutex);
    }
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
    printf("\nMEASURING SEMAPHORE AND MUTEX ACQUISITION TIME AFTER RELEASE\n");

    // start = malloc(sizeof(struct timespec));
    start  = mmap(NULL, sizeof(struct timespec), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    finish = mmap(NULL, sizeof(struct timespec), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);

    sem_t* id;

    int* status = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    *status = 1;

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
    
    if(w_drop_cache)
    {
        pthread_create(&thread_creation, NULL, semaphore_keeper, (void*)&args);

        int noncache_iterations = w_iterations / 10;

        double totSemaphoreUncached = 0.0;

        *status = 1;

        for(int i = 0; i < noncache_iterations; i++)
        {
            pthread_barrier_wait(args.barr);
            drop_cache();
            pthread_barrier_wait(args.barr);
            if(sem_wait(id)) printf("WAIT FAILED ON PARENT");
            clock_gettime(CLOCK_REALTIME, finish);
            totSemaphoreUncached += (finish->tv_nsec - start->tv_nsec);
            if(sem_post(id)) printf("POST FAILED ON PARENT");
        }

        *status = -1;
        pthread_barrier_wait(args.barr);

        pthread_join(thread_creation, NULL);

        printf("Non-cached\tmean semaphore acquisition time after release with %d samples: %f ns\n", noncache_iterations, totSemaphoreUncached / noncache_iterations);
    }

    sem_close(id);

    args.mutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(args.mutex, NULL);

    // calculate mutex times here
    *status = 1;
    pthread_create(&thread_creation, NULL, mutex_keeper, (void*)&args);

    double totMutexCached = 0.0;

    for(int i = 0; i < w_iterations; i++)
    {
        pthread_barrier_wait(args.barr);
        pthread_barrier_wait(args.barr);
        pthread_mutex_lock(args.mutex);
        clock_gettime(CLOCK_REALTIME, finish);
        if(i >= (w_iterations/5))
        {
            totMutexCached += (finish->tv_nsec - start->tv_nsec);
        }
        pthread_mutex_unlock(args.mutex);
    }
    // very ugly
    *status = -1;
    pthread_barrier_wait(args.barr);

    pthread_join(thread_creation, NULL);

    printf("\nCached\t\tmean mutex acquisition time after release with %d samples: %f ns\n", w_iterations - (w_iterations / 5), totMutexCached / (w_iterations - (w_iterations / 5)));
    
    if(w_drop_cache)
    {
        *status = 1;

        pthread_create(&thread_creation, NULL, mutex_keeper, (void*)&args);

        int noncache_iterations = w_iterations / 10;

        double totMutexUncached = 0.0;

        for(int i = 0; i < noncache_iterations; i++)
        {
            pthread_barrier_wait(args.barr);
            drop_cache();
            pthread_barrier_wait(args.barr);
            pthread_mutex_lock(args.mutex);
            clock_gettime(CLOCK_REALTIME, finish);
            totMutexUncached += (finish->tv_nsec - start->tv_nsec);  
            pthread_mutex_unlock(args.mutex);
        }

        *status = -1;
        pthread_barrier_wait(args.barr);

        pthread_join(thread_creation, NULL);

        printf("Non-cached\tmean mutex acquisition time after release with %d samples: %f ns\n", noncache_iterations, totMutexUncached / noncache_iterations);
    }
    
    munmap(status, sizeof(int));
    munmap(start, sizeof(struct timespec));
    munmap(finish, sizeof(struct timespec));
}