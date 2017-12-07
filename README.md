# Performance tester

This program is a simple POSIX / linux interface tester. It is possible to test: 

 - Thread and process creation time overhead ( with  *fork()* and *pthread_create()* calls)
 - Semaphore and mutex init & acquisition time overhead with both empty and unitialized mutexes / semaphores.
 - Semaphore and mutex acquisition time overhead after some other process / thread has released it.
 - The time it takes to compile multiple different files with processes / threads. (Giving one process / thread per file)
 - Data transfer speed comparison between pipe transfer and transferring through memory mapped memory pool.

Some of the tests even support dropping the CPU instruction cache for more comprehensive testing (if you run the program with root privileges).

# How to compile and use

You should be able to compile the program with simply running 
```sh
# make
```
in the root directory. After it you can either run the program normally or with root privileges. The program takes 4 arguments, it is run either by
```sh
# ./performance_tester <iteration count> <file count> <file size> <buffer size>
```
or as root
```sh
$ ./performance_tester <iteration count> <file count> <file size> <buffer size>
```

Here is an example if you want to run 1000 iterations, file tests with 10 files, á 100MB and with buffer size of 4096 bytes:
```sh
$ ./performance_tester 1000 10 100 4096
```

# Known bugs

  - Sometimes the total time in useconds exceeds the maximum value of type *double*, and the test result seems to be a negative time. You can try to lower the iteration count/file sizes to get around it.
  - The file transfer algorithm doesn't always write the exact copy of the input file.
  - If you run the program once with root privileges, the program will segfault with normal user privileges until a reboot. This is a problem with POSIX semaphores and their privileges.

### Todos

 - Write comparison for data transfer through message queue
 - Fix the problem with semaphore privileges