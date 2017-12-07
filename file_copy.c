#include <stdlib.h>         // NULL, exit
#include <stdio.h>          // printf, sprintf
#include <sys/types.h>      // O_RDONLY, O_WRONLY
#include <sys/stat.h>       // mkdir
#include <fcntl.h>          // open
#include <unistd.h>         // close
#include <sys/sendfile.h>   // sendfile
#include <pthread.h>        // pthread
#include <sys/wait.h>       // wait
#include "testutils.h"      // start, finish time structs

#include "log.h"

static int log_fd;

int generate_files(int w_numFiles, int w_fileSize)
{
    if(mkdir("dummyfiles", S_IRWXU | S_IRWXO))
    {
        perror("mkdir");
        return -1;
    }
    
    // generate dummy files for copying
    for(int i = 0; i < w_numFiles; i++)
    {
        char filename[50] = { 0 };
        int sprintfret = sprintf(filename, "%s%d", "dummyfiles/dummy", i);
        if(sprintfret < 0)
        {
            perror("sprintf");
            return -2;
        }
        FILE *fp=fopen(filename, "w");
        if(!fp)
        {
            perror("fopen");
            return -3;
        }
        int trunc = ftruncate(fileno(fp), 1000*1000*100);
        if(trunc)
        {
            perror("ftruncate");
            return -4;
        }
        int fcloseret = fclose(fp);
        if(fcloseret)
        {
            perror("fclose");
            return -5;
        }
    }
    return 0;
}

void* thread_copier(void* w_filename)
{
    char* filename = w_filename;
    int fdin, fdout;

    char outFile[26] = {0};
    int sprintfret = sprintf(outFile, "%s%s", filename, "_thread");
    if(sprintfret < 0)
    {
        perror("sprintf");
        return NULL;
    }

    if((fdin = open(filename, O_RDONLY)) == -1)
    {
        perror("open");
        printf("the file was: %s\n", filename);
        return NULL;
    }
    
    if((fdout = open(outFile, O_WRONLY | O_CREAT, S_IRWXU | S_IRWXO | S_IRWXG)) == -1)
    {
        perror("open");
        printf("the file was: %s\n", outFile);
        // this return value does not need to be checked, right? :)
        close(fdin);
        return NULL;
    }
    off_t startOffset = 0;
    struct stat fileinfo = {0};
    fstat(fdin, &fileinfo);
    int resbytes = sendfile(fdout, fdin, &startOffset, fileinfo.st_size);

    if(resbytes < fileinfo.st_size)
    {
        perror("sendfile");
        return NULL;
    }

    close(fdin);
    close(fdout);
    free(filename);

    return NULL;
}

void file_copy(int w_numFiles, int w_fileSize, int w_drop_cache)
{
    log_fd = open("performance_tester.log", O_RDWR | O_APPEND | O_CREAT | O_NONBLOCK, S_IRWXU);
    log_write(log_fd, 1, "Multiple file copy measurement with threads & processes routine init.");
    // sanity check
    if(w_numFiles > 99 || w_numFiles < 1)
    {
        printf("File copying will not be tested because the user input was invalid. Please use 1-99 as number of files.\n");
        return;
    }

    if(generate_files(w_numFiles, w_fileSize))
    {
        printf("Generating dummy files failed. Please try to manually remove the \"dummyfiles\" folder and its insides.\n");
        return;
    }

    printf("\nMEASURING FILE TRANSFER SPEED IN THREADS VS FORKED PROCESSES\n");
    printf("USING %d FILES EACH %d MiB IN SIZE\n", w_numFiles, w_fileSize);
    printf("EACH FILE WILL HAVE ITS OWN THREAD/PROCESS PROCESSING IT\n");

    start = malloc(sizeof(struct timespec));
    finish = malloc(sizeof(struct timespec));
    long totCopyTime = 0;

    if(!start || !finish)
    {
        perror("malloc");
        return;
    }


    // THREADS
    pthread_t* threads = malloc(w_numFiles * sizeof(pthread_t));
    if(!threads)
    {
        perror("malloc");
        return;
    }

    // start timing for threads
    clock_gettime(CLOCK_REALTIME, start);

    // start copying of files using threads:
    for(int i = 0; i < w_numFiles; i++)
    {
        char* filename = calloc(300, sizeof(char));
        if(!filename)
        {
            perror("calloc");
            return;
        }
        int sprintfret = sprintf(filename, "%s%d", "dummyfiles/dummy", i);
        if(sprintfret < 0)
        {
            perror("sprintf");
            free(filename);
            return;
        }

        int ret = pthread_create(&threads[i], NULL, thread_copier, (void*)filename);
        if(ret)
        {
            perror("pthread_create");
            free(filename);
            return;
        }
    }

    for(int i = 0; i < w_numFiles; i++)
    {
        if(pthread_join(threads[i], NULL)) perror("pthread_join");
    }

    clock_gettime(CLOCK_REALTIME, finish);

    totCopyTime = (finish->tv_nsec - start->tv_nsec);
    printf("\nCopying of %d files (%d MiB each, total %d MiB) took %ld ns using threads.\n", w_numFiles, w_fileSize, w_numFiles * w_fileSize, totCopyTime);

    clock_gettime(CLOCK_REALTIME, start);

    // PROCESSES
    pid_t child;

    for(int i = 0; i < w_numFiles; i++)
    {
        if((child = fork()) == 0)
        {
            char filename[300] = {0};
            int sprintfret = sprintf(filename, "%s%d", "dummyfiles/dummy", i);
            char outFile[300] = {0};
            sprintfret = sprintf(outFile, "%s%s", filename, "_fork");
            if(sprintfret < 0)
            {
                perror("sprintf");
                exit(0);
            }

            int fdin, fdout;

            if((fdin = open(filename, O_RDONLY)) == -1)
            {
                perror("open");
                printf("the file was: %s\n", filename);
                exit(0);
            }
            
            if((fdout = open(outFile, O_WRONLY | O_CREAT, S_IRWXU | S_IRWXO | S_IRWXG)) == -1)
            {
                perror("open");
                printf("the file was: %s\n", outFile);
                // this return value does not need to be checked, right? :)
                close(fdin);
                exit(0);
            }

            off_t startOffset = 0;
            struct stat fileinfo = {0};
            fstat(fdin, &fileinfo);
            int resbytes = sendfile(fdout, fdin, &startOffset, fileinfo.st_size);

            if(resbytes < fileinfo.st_size)
            {
                perror("sendfile");
                exit(0);
            }

            close(fdin);
            close(fdout);

            exit(0);
        }
    }

    pid_t waited;
    while((waited = wait(NULL))> 0)
    {
        // waiting for the children to exit.
        // here we could also check the exit values of them,
        // but I didn't have time for that now.
    }

    clock_gettime(CLOCK_REALTIME, finish);
    totCopyTime = (finish->tv_nsec - start->tv_nsec);
    printf("\nCopying of %d files (%d MiB each, total %d MiB) took %ld ns using forks.\n", w_numFiles, w_fileSize, w_numFiles * w_fileSize, totCopyTime);


    // clean up files copied by threads
    for(int i = 0; i < w_numFiles; i++)
    {
        char filename[300] = {0};
        int sprintfret = sprintf(filename, "%s%d%s", "dummyfiles/dummy", i, "_thread");
        if(sprintfret < 0)
        {
            perror("sprintf");
            return;
        }
        if(remove(filename)) perror("remove");
    }

    // clean up files copied by forks
    for(int i = 0; i < w_numFiles; i++)
    {
        char filename[300] = {0};
        int sprintfret = sprintf(filename, "%s%d%s", "dummyfiles/dummy", i, "_fork");
        if(sprintfret < 0)
        {
            perror("sprintf");
            return;
        }
        if(remove(filename)) perror("remove");
    }


    // FILE AND FOLDER CLEAN UP
    for(int i = 0; i < w_numFiles; i++)
    {
        char filename[30] = {0};
        int sprintfret = sprintf(filename, "%s%d", "dummyfiles/dummy", i);
        if(sprintfret < 0)
        {
            perror("sprintf");
            return;
        }
        if(remove(filename)) perror("remove");
    }
    if(remove("dummyfiles")) perror("remove");

    close(log_fd);

    free(threads);
    free(start);
    free(finish);

}