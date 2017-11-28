#include <stdio.h>      // printf, 
#include <time.h>       // struct timespec, clock_gettime()
#include <stdlib.h>     // malloc
#include <sys/mman.h>   // mmap
#include <sys/wait.h>   // wait
#include <unistd.h>     // sysconf
#include <pthread.h>    // threading
#include <semaphore.h>  // semaphore

#include "tests.h"
#include "testutils.h"

void* thread_creation_overhead(void* voidparam)
{
    clock_gettime(CLOCK_REALTIME, finish);
    return NULL;
}

void measure_creation_overhead(int w_iterations, int w_drop_cache)
{
    printf("\nMEASURING PROCESS AND THREAD CREATION OVERHEAD\n");
    start = malloc(sizeof(struct timespec));
    finish = mmap(NULL, sizeof(struct timespec), PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0);

    double retProcessCached = 0.0;
    
    for(int i = 0; i < w_iterations; i++)
    {
        clock_gettime(CLOCK_REALTIME, start);
        if(!fork())
        {
            // child
            clock_gettime(CLOCK_REALTIME, finish);
            exit(0);
        }
        else
        {
            // parent
            wait(NULL);
            double total = (finish->tv_nsec - start->tv_nsec);
            //printf("%f\n", total);
            if(i >= (w_iterations / 5))
            {
                retProcessCached += total;
            }
        }
    }
    printf("\nCached\t\tmean process creation using fork() with %d samples: %f us.\n", w_iterations - (w_iterations / 5), (retProcessCached / (w_iterations - (w_iterations / 5))) / 1000.0);
    
    if(w_drop_cache)
    {
        int noncache_iterations = w_iterations / 10;

        double retProcessUncached = 0.0;
        
        for(int i = 0; i < noncache_iterations; i++)
        {
            drop_cache();
            clock_gettime(CLOCK_REALTIME, start);
            if(!fork())
            {
                // child
                clock_gettime(CLOCK_REALTIME, finish);
                exit(0);
            }
            else
            {
                // parent
                wait(NULL);
                double total = (finish->tv_nsec - start->tv_nsec);
                //printf("%f\n", total);
                retProcessUncached += total;
            }
        }
        printf("Non-cached\tmean process creation using fork() with %d samples: %f us.\n", noncache_iterations, (retProcessUncached / noncache_iterations) / 1000.0);
        
    }

    munmap(finish, sizeof(*finish));

    finish = malloc(sizeof(struct timespec));

    pthread_t thread_creation;

    double retThreadCached = 0.0;

    for(int i = 0; i < w_iterations; i++)
    {
        clock_gettime(CLOCK_REALTIME, start);
        pthread_create(&thread_creation, NULL, thread_creation_overhead, NULL); // bad practice, check retval!
        pthread_join(thread_creation, NULL);
        double total = (finish->tv_nsec - start->tv_nsec);
        if(i >= (w_iterations / 5))
        {
            retThreadCached += total;
        }
    }

    printf("\nCached\t\tmean thread creation time using pthread_create() with %d samples: %f us\n", w_iterations - (w_iterations / 5), (retThreadCached / (w_iterations - (w_iterations / 5))) / 1000.0);

    if(w_drop_cache)
    {
        double retThreadUncached = 0.0;

        int noncache_iterations = w_iterations / 10;

        for(int i = 0; i < noncache_iterations; i++)
        {
            drop_cache();
            clock_gettime(CLOCK_REALTIME, start);
            pthread_create(&thread_creation, NULL, thread_creation_overhead, NULL); // bad practice, check retval!
            pthread_join(thread_creation, NULL);
            double total = (finish->tv_nsec - start->tv_nsec);
            retThreadUncached += total;
        }

        printf("Non-cached\tmean thread creation time using pthread_create() with %d samples: %f us\n", noncache_iterations, (retThreadUncached / noncache_iterations) / 1000.0);

    }

    free(start);
    free(finish);
}