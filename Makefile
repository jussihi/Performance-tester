CC=gcc
CFLAGS=-Wall -pedantic -std=c99 -O3 -D_POSIX_C_SOURCE=199309L -D_GNU_SOURCE
LIB=ar rcs
COBJ=thread_process_creation.c semaphore_mutex_release.c semaphore_mutex_empty_open.c file_copy.c file_transfer.c

all: performance_tester 

performance_tester: main.c $(COBJ) liblog.a
	$(CC) $(CFLAGS) -o performance_tester main.c $(COBJ) -L. -lrt -lpthread -llog

log.o: log.c
	$(CC) $(CFLAGS) -c log.c

liblog.a: log.o
	$(LIB) liblog.a log.o && rm -f log.o

clean:
	rm -f performance_tester *.o *.a *.log