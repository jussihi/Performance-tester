#ifndef TESTS_HH_
#define TESTS_HH_

// Function definitions

/*
 * Function to drop the instruction cache from CPU
 * slows down syscalls (intentionally!)
 */
void drop_cache();


/*
 * Thread and process creation time calculation function definitions
 */
void measure_creation_overhead(int w_iterations, int w_dropCache);


/*
 * Timing of opening an empty semaphore,
 * Timing of getting an already released semaphore (which nobody owns)
 */
void semaphore_mutex_open(int w_iterations, int w_dropCache);

void semaphore_mutex_empty(int w_iterations, int w_dropCache);


/*
 * Timing of released semaphore / mutex
 */
void semaphore_mutex_not_empty(int w_iterations, int w_dropCache);


/*
 * File copy tests
 * file size in MiB
 */
void file_copy(int w_numFiles, int w_fileSize, int w_dropCache);




/*
 * File transfer tests
 * file size in MiB
 * buffer size in bytes
 */

void file_transfer(int w_fileSize, int w_bufferSize, int w_dropCache);

#endif