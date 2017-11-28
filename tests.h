#ifndef TESTS_HH_
#define TESTS_HH_

// Function definitions

/*
 * Function to drop the instruction cache from CPU
 * slows down syscalls
 */
void drop_cache();


/*
 * Thread and process creation time calculation function definitions
 */
void* thread_creation_overhead(void* voidparam);

void measure_creation_overhead(int w_iterations, int w_drop_cache);


/*
 * Timing of opening an empty semaphore,
 * Timing of getting an already released semaphore (which nobody owns)
 */
void semaphore_mutex_open(int w_iterations, int w_drop_cache);

void semaphore_mutex_empty(int w_iterations, int w_drop_cache);


/*
 * Timing of released semaphore / mutex
 * OBS: this is very buggy still
 */
void* mutex_keeper(void* w_args);

void* semaphore_keeper(void* w_args);

void semaphore_mutex_not_empty(int w_iterations);




#endif