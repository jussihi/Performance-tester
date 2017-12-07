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

#include "log.h"

static int log_fd;


void semaphore_mutex_open(int w_iterations, int w_drop_cache)
{
    log_fd = open("performance_tester.log", O_RDWR | O_APPEND | O_CREAT | O_NONBLOCK, S_IRWXU);
    log_write(log_fd, 1, "Free semaphore & mutex acquisition overhead measurement routine init.");
    printf("\nMEASURING SEMAPHORE AND MUTEX INIT & ACQUISITION OVERHEAD\n");

    start = malloc(sizeof(struct timespec));
    finish = malloc(sizeof(struct timespec));

    sem_t* id;

    double totSemaphoreCached = 0.0;

    for(int i = 0; i < w_iterations; i++)
    {
        sem_unlink("mysem");
        clock_gettime(CLOCK_REALTIME, start);
        if((id = sem_open("mysem", O_CREAT, 0600, 0)) == SEM_FAILED)    // check retval! / slows down???
        {
            perror("sem_open");
            return;
        }
        clock_gettime(CLOCK_REALTIME, finish);
        sem_post(id);
        sem_close(id);
        if(i >= (w_iterations / 5))
        {
            totSemaphoreCached += (finish->tv_nsec - start->tv_nsec);
        }
    }

    printf("\nCached\t\tmean init & acquisition time of a semaphore with %d samples: %f ns.\n", w_iterations - (w_iterations / 5), (totSemaphoreCached / (w_iterations - (w_iterations / 5))));
    
    if(w_drop_cache)
    {
        int noncache_iterations = w_iterations / 10;

        double totSemaphoreUncached = 0.0;

        for(int i = 0; i < noncache_iterations; i++)
        {
            sem_unlink("mysem");
            drop_cache();
            clock_gettime(CLOCK_REALTIME, start);
            if((id = sem_open("mysem", O_CREAT, 0600, 0)) == SEM_FAILED)    // slows down?
            {
                perror("sem_open");
                return;
            }
            clock_gettime(CLOCK_REALTIME, finish);
            sem_post(id);
            sem_close(id);
            totSemaphoreUncached += (finish->tv_nsec - start->tv_nsec);
        }

        printf("Non-cached\tmean init & acquisition time of a semaphore with %d samples: %f ns.\n", noncache_iterations, (totSemaphoreUncached / noncache_iterations));
    }

    // create mutex and init it
    pthread_mutex_t mutex;
    
    double totMutexCached = 0.0;

    // here also same problem as with the first for-loop, the first iteration seems to have huge time difference compared
    // to the other iterations
    for(int i = 0; i < w_iterations; i++)
    {
        clock_gettime(CLOCK_REALTIME, start);
        pthread_mutex_init(&mutex, NULL);    // once again check retval!
        pthread_mutex_lock(&mutex);
        clock_gettime(CLOCK_REALTIME, finish);
        pthread_mutex_unlock(&mutex);
        pthread_mutex_destroy(&mutex);
        if(i >= (w_iterations / 5))
        {
            totMutexCached += (finish->tv_nsec - start->tv_nsec);
        }
    }
        
    printf("\nCached\t\tmean init & acquisition time of a mutex with %d samples: %f ns.\n", w_iterations - (w_iterations / 5), (totMutexCached / (w_iterations - (w_iterations / 5))));

    if(w_drop_cache)
    {
        int noncache_iterations = w_iterations / 10;

        double totMutexUncached = 0.0;

        // here also same problem as with the first for-loop, the first iteration seems to have huge time difference compared
        // to the other iterations
        for(int i = 0; i < noncache_iterations; i++)
        {
            drop_cache();
            clock_gettime(CLOCK_REALTIME, start);
            pthread_mutex_init(&mutex,NULL);    // once again check retval!
            pthread_mutex_lock(&mutex);
            clock_gettime(CLOCK_REALTIME, finish);
            pthread_mutex_unlock(&mutex);
            pthread_mutex_destroy(&mutex);
            totMutexUncached += (finish->tv_nsec - start->tv_nsec);
        }

        printf("Non-cached\tmean init & acquisition time of a mutex with %d samples: %f ns.\n", noncache_iterations, (totMutexUncached / noncache_iterations));

    }
    close(log_fd);
    free(start);
    free(finish);
}

void semaphore_mutex_empty(int w_iterations, int w_drop_cache)
{
    log_fd = open("performance_tester.log", O_RDWR | O_APPEND | O_CREAT | O_NONBLOCK, S_IRWXU);
    log_write(log_fd, 1, "Empty semaphore & mutex init and acquisition overhead measurement routine init.");
    printf("\nMEASURING MUTEX AND SEMAPHORE ACQUISITION OVERHEAD WHEN THEY ARE FREE\n");

    start = malloc(sizeof(struct timespec));
    finish = malloc(sizeof(struct timespec));

    sem_t* id;
    // create new semaphore, it is also acquired here,
    sem_unlink("mysem");
    if((id = sem_open("mysem", O_CREAT, 0600, 0)) == SEM_FAILED)  
    {
        perror("sem_open");
        return;
    }
    // release semaphore as it was acquired in the sem_open
    sem_post(id);

    double totSemaphoreCached = 0.0;

    // problem with first iteration; the clock seems to be very off with the first one, it is 20x slower on the first run?
    // edit: this is figured out in the diary
    for(int i = 0; i < w_iterations; i++)
    {
        clock_gettime(CLOCK_REALTIME, start);
        // get semaphore, this should always be free
        if(sem_wait(id) < 0) perror("sem_wait");    
        clock_gettime(CLOCK_REALTIME, finish);
        sem_post(id);
        if(i >= (w_iterations / 5))
        {
            totSemaphoreCached += (finish->tv_nsec - start->tv_nsec);
        }
    }

    printf("\nCached\t\tmean acquisition time of a free semaphore with %d samples: %f ns.\n", w_iterations - (w_iterations / 5), (totSemaphoreCached / (w_iterations - (w_iterations / 5))));
    
    if(w_drop_cache)
    {
        int noncache_iterations = w_iterations / 10;

        double totSemaphoreUncached = 0.0;

        for(int i = 0; i < noncache_iterations; i++)
        {
            drop_cache();
            clock_gettime(CLOCK_REALTIME, start);
            // get semaphore, this should always be free
            if(sem_wait(id) < 0) perror("sem_wait");
            clock_gettime(CLOCK_REALTIME, finish);
            sem_post(id);
            totSemaphoreUncached += (finish->tv_nsec - start->tv_nsec);
        }

        printf("Non-cached\tmean acquisition time of a free semaphore with %d samples: %f ns.\n", noncache_iterations, (totSemaphoreUncached / noncache_iterations));
        
    }

    sem_close(id);

    // create mutex and init it
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex,NULL);

    double totMutexCached = 0.0;

    // here also same problem as with the first for-loop, the first iteration seems to have huge time difference compared
    // to the other iterations
    for(int i = 0; i < w_iterations; i++)
    {
        clock_gettime(CLOCK_REALTIME, start);
        pthread_mutex_lock(&mutex);
        clock_gettime(CLOCK_REALTIME, finish);
        pthread_mutex_unlock(&mutex);
        if(i >= (w_iterations / 5))
        {
            totMutexCached += (finish->tv_nsec - start->tv_nsec);
        }
    }
        
    printf("\nCached\t\tmean acquisition time of a free mutex with %d samples: %f ns.\n", w_iterations - (w_iterations / 5), (totMutexCached / (w_iterations - (w_iterations / 5))));

    if(w_drop_cache)
    {
        int noncache_iterations = w_iterations / 10;

        double totMutexUncached = 0.0;

        for(int i = 0; i < noncache_iterations; i++)
        {
            drop_cache();
            clock_gettime(CLOCK_REALTIME, start);
            pthread_mutex_lock(&mutex);
            clock_gettime(CLOCK_REALTIME, finish);
            pthread_mutex_unlock(&mutex);
            totMutexUncached += (finish->tv_nsec - start->tv_nsec);
        }

        printf("Non-cached\tmean acquisition time of a free mutex with %d samples: %f ns.\n", noncache_iterations, (totMutexUncached / noncache_iterations));

    }

    pthread_mutex_destroy(&mutex);
    close(log_fd);
    free(start);
    free(finish);
}