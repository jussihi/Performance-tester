#include <stdio.h>      // printf, 
#include <time.h>       // struct timespec, clock_gettime()
#include <unistd.h>     // sysconf
#include <stdlib.h>     // malloc
#include <sys/mman.h>   // mmap
#include <sys/wait.h>   // wait
#include <pthread.h>    // threading
#include <semaphore.h>  // semaphore
#include <fcntl.h>      // O_CREAT

#include "tests.h"
#include "testutils.h"

//static struct timespec start, finish;
struct timespec* start;
struct timespec* finish;

double gettime_cached;
double gettime_nocached;


void drop_cache()
{
	sync();
	int fd = open("/proc/sys/vm/drop_caches", O_WRONLY);
	write(fd, "3", sizeof(char));
	close(fd);
}

double measure_gettime_overhead(int w_iterations, int w_drop_cache)
{
	printf("\nMEASURING GETTIME CALL OVERHEAD\n");
    start = malloc(sizeof(struct timespec));
    finish = malloc(sizeof(struct timespec));

    double total = 0.0;

    int noncache_iterations = w_iterations / 10;

    // get the cached gettime syscall overhead
    for(int i = 0; i < w_iterations; i++)
    {
        clock_gettime(CLOCK_REALTIME, start);
        clock_gettime(CLOCK_REALTIME, finish);
        //double timeused = (finish->tv_nsec - start->tv_nsec);
        //printf("%f\n", timeused);
        if(i >= (w_iterations / 5))
        {
        	total += (finish->tv_nsec - start->tv_nsec);
        }
    }

    gettime_cached = total / (w_iterations - (w_iterations / 5));

    printf("\nCached\t\tgettime() call mean overhead with %d samples: %f ns.\n", w_iterations - (w_iterations / 5), gettime_cached);

    if(w_drop_cache)
    {
    	total = 0.0;

	    // get the non-cached gettime syscall overhead
	    for(int i = 0; i < noncache_iterations; i++)
	    {
	    	drop_cache();
	        clock_gettime(CLOCK_REALTIME, start);
	        clock_gettime(CLOCK_REALTIME, finish);
	        //double timeused = (finish->tv_nsec - start->tv_nsec);
	        //printf("%f\n", timeused);
	        total += (finish->tv_nsec - start->tv_nsec);
	    }

	    gettime_nocached = total / noncache_iterations;

	    printf("Non-cached\tgettime() call mean overhead with %d samples: %f ns.\n", noncache_iterations, gettime_nocached);
    }
    
    free(start);
    free(finish);

    return total;
}

int main(void)
{
	int has_root;
	if(geteuid() != 0)
	{
		// normal run
		printf("\nYou did not run this program as root. Cache will not be dropped during tests.\n");
		has_root = 0;
	}
	else
	{
		// got root! we can drop caches
		printf("\nThe application has root privileges. Cache will be dropped for more comprehensive testing.\n");
		has_root = 1;
	}

	drop_cache();

	measure_gettime_overhead(1000, has_root);
    
    //printf("10 000 clock_gettime calls takes: %f nanoseconds\n", measure_gettime_overhead(1000));
    
    // printf("So the sysconf return value was %d\n", sysconf(_SC_MONOTONIC_CLOCK));

    measure_creation_overhead(1000, has_root);

    semaphore_mutex_empty(1000, has_root);

    semaphore_mutex_open(1000, has_root);

    semaphore_mutex_not_empty(500, has_root);

    // clock_gettime(CLOCK_REALTIME, &finish);

    // double total = /*(finish.tv_msec - start.tv_msec) * 1000.0 +*/ ((finish.tv_nsec - start.tv_nsec));

    // printf("%f\n", total);

    return 0;
}