CC=gcc
CFLAGS=-Wall -pedantic -std=c99 -o3 -D_POSIX_C_SOURCE=199309L -D_GNU_SOURCE
LIB=ar rcs
COBJ=thread_process_creation.c semaphore_mutex_release.c semaphore_mutex_empty_open.c file_copy.c file_transfer.c

all: main 

main: main.c $(COBJ)
	$(CC) $(CFLAGS) -o main main.c $(COBJ) -lrt -lpthread

clean:
	rm -f main *.o *.a *.log