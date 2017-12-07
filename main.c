#include <stdio.h>      // printf, 
#include <time.h>       // struct timespec, clock_gettime()
#include <unistd.h>     // sysconf
#include <stdlib.h>     // malloc
#include <sys/mman.h>   // mmap
#include <sys/wait.h>   // wait
#include <pthread.h>    // threading
#include <semaphore.h>  // semaphore
#include <fcntl.h>      // O_CREAT
#include <signal.h>     // signal handling

#include "tests.h"
#include "testutils.h"

#include "log.h"

static int log_fd;

struct timespec* start;
struct timespec* finish;

double gettimeCached;
double gettimeNoCached;

int run;

void sig_int(int w_signal)
{
    run = 0;
}

void drop_cache()
{
    sync();
    int fd = open("/proc/sys/vm/drop_caches", O_WRONLY);
    write(fd, "3", sizeof(char));
    close(fd);
}

double measure_gettime_overhead(int w_iterations, int w_dropCache)
{
    printf("\nMEASURING GETTIME CALL OVERHEAD\n");
    log_write(log_fd, 1, "Gettime overhead measurement routine init.");
    start = malloc(sizeof(struct timespec));
    finish = malloc(sizeof(struct timespec));

    double total = 0.0;

    int nonCacheIterations = w_iterations / 10;

    // get the cached gettime syscall overhead
    for(int i = 0; i < w_iterations; i++)
    {
        clock_gettime(CLOCK_REALTIME, start);
        clock_gettime(CLOCK_REALTIME, finish);
        if(i >= (w_iterations / 5))
        {
            total += (finish->tv_nsec - start->tv_nsec);
        }
    }

    gettimeCached = total / (w_iterations - (w_iterations / 5));

    printf("\nCached\t\tgettime() call mean overhead with %d samples: %f ns.\n", w_iterations - (w_iterations / 5), gettimeCached);

    if(w_dropCache)
    {
        total = 0.0;

        // get the non-cached gettime syscall overhead
        for(int i = 0; i < nonCacheIterations; i++)
        {
            drop_cache();
            clock_gettime(CLOCK_REALTIME, start);
            clock_gettime(CLOCK_REALTIME, finish);
            total += (finish->tv_nsec - start->tv_nsec);
        }

        gettimeNoCached = total / nonCacheIterations;

        printf("Non-cached\tgettime() call mean overhead with %d samples: %f ns.\n", nonCacheIterations, gettimeNoCached);
    }
    
    free(start);
    free(finish);

    return total;
}

int main(int argc, char** argv)
{
    log_fd = open("performance_tester.log", O_RDWR | O_APPEND | O_CREAT | O_NONBLOCK, S_IRWXU);

    log_write(log_fd, 1, "Performance tester init.");

    run = 1;

    struct sigaction sa;

    sigemptyset(&sa.sa_mask);

    sa.sa_handler = &sig_int;
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    
    int hasRoot;
    if(geteuid() != 0)
    {
        // normal run
        printf("\nYou did not run this program as root. Cache will not be dropped during tests.\n");
        hasRoot = 0;
    }
    else
    {
        // got root! we can drop caches
        printf("\nThe application has root privileges. Cache will be dropped for more comprehensive testing.\n");
        hasRoot = 1;
    }
    printf("\n!!!THIS APPLICATION USES CLOCK_REALTIME FOR TIMING!!!\n!!!Please don't change the system time while running this application!!!\n");

    if(run) measure_gettime_overhead(1000, hasRoot);
    log_write(log_fd, 1, "Gettime overhead measurement routine done.");

    if(run) measure_creation_overhead(1000, hasRoot);
    log_write(log_fd, 1, "Thread & process creation overhead measurement routine done.");

    if(run) semaphore_mutex_empty(1000, hasRoot);
    log_write(log_fd, 1, "Empty semaphore & mutex init and acquisition overhead measurement routine done.");

    if(run) semaphore_mutex_open(1000, hasRoot);
    log_write(log_fd, 1, "Free semaphore & mutex acquisition overhead measurement routine done.");

    if(run) semaphore_mutex_not_empty(500, hasRoot);
    log_write(log_fd, 1, "Semaphore & mutex acquisition after release overhead measurement routine done.");

    if(run) file_copy(10, 100, hasRoot);
    log_write(log_fd, 1, "Multiple file copy measurement with threads & processes routine done.");

    if(run) file_transfer(100, 4096, hasRoot);
    log_write(log_fd, 1, "File transfer speed measurement with pipes & shared memory routine done.");

    if(!run)
    {
        printf("Program was requested to exit by SIGINT. Exiting.\n");
        log_write(log_fd, 1, "Premature exit (SIGINT).");
    }
    log_write(log_fd, 1, "Program exit.");
    close(log_fd);
    return 0;
}