#include <sys/types.h>      // O_RDONLY, O_WRONLY
#include <sys/stat.h>       // mkdir
#include <fcntl.h>          // open
#include <unistd.h>         // close
#include <limits.h>			// PIPE_BUF
#include <stdio.h>          // printf, sprintf
#include <stdlib.h>         // NULL, exit

#include "testutils.h"      // start, finish time structs

int remove_dummyfile()
{
    char filename[17] = {0};
    int sprintfret = sprintf(filename, "%s", "dummyfiles/dummy");
    if(sprintfret < 0)
    {
        perror("sprintf");
        return -1;
    }
    if(remove(filename))
    {
    	perror("remove");
    	return -2;
    }
    if(remove("dummyfiles"))
	{		
		perror("remove");
		return -3;
	}
	return 0;
}

int generate_file(int w_fileSize)
{
	if(mkdir("dummyfiles", S_IRWXU | S_IRWXO))
    {
        perror("mkdir");
        return -1;
    }
    
    char filename[17] = { 0 };
    int sprintfret = sprintf(filename, "%s", "dummyfiles/dummy");
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

    return 0;
}

int pipe_transfer()
{
	// crete pipe, assign O_NONBLOCK to the writing end
	int pipefd[2];
	pipe(pipefd);

	//if(fcntl(pipefd[0], F_SETFL, O_NONBLOCK)) perror("fcntl O_NONBLOCK");
	if((fcntl(pipefd[0], F_SETPIPE_SZ, PIPE_BUF)) != PIPE_BUF) perror("fcntl F_SETPIPE_SZ");

	pid_t child;

	if((child = fork()) == 0)
    {
    	// child, here we send the data to parent.
    	FILE* fp = fopen("dummyfiles/dummy", "rb");
    	if(!fp)
    	{
    		perror("File opening failed.");
    		return -1;
    	}

    	int charsRead = 0;
    	char buffer[PIPE_BUF];

		charsRead = fread(buffer, 1, PIPE_BUF, fp);
		do
		{
    		if((write(pipefd[1], buffer, charsRead)) == -1)
    		{
    			perror("write returned an error :)");
    		}
		} while((charsRead = fread(buffer, 1, PIPE_BUF, fp)) == PIPE_BUF);
		printf("sent %d\n", charsRead);

		if((write(pipefd[1], buffer, charsRead)) == -1)
		{
			perror("write returned an error :)");
		}
		
    	int fcloseret = fclose(fp);
		if(fcloseret)
		{
		    perror("fclose");
		    return -5;
		}
		
		//printf("asd: %d\n", asd);
		printf("writing complete.\n");

		close(pipefd[1]);

    	exit(0);
    }
    else
    {
    	//close(pipefd[0]);

    	char buffer[PIPE_BUF];

    	FILE* fp = fopen("dummyfiles/dummywrite", "wb");
    	if(!fp)
    	{
    		perror("File opening failed.");
    		return -1;
    	}

    	int charsWritten = 0;

    	int readret;

    	int i = 0;
    	

    	if((readret = read(pipefd[0], buffer, PIPE_BUF)) != PIPE_BUF)
    	{
    		perror("read");
    		return -1;
    	}

    	do
    	{
    		printf("%d\n", ++i);
    		if(readret != PIPE_BUF) printf("ei tänne pitäisi tulla");
    		if((fwrite(buffer, 1, PIPE_BUF, fp)) != PIPE_BUF)
    		{
    			perror("fwrite");
    		}
    	} while((readret = read(pipefd[0], buffer, PIPE_BUF)) == PIPE_BUF);
    	printf("readret: %d\n", readret);
    	
    	if((charsWritten = fwrite(buffer, 1, readret, fp)) != readret)
		{
			perror("fwrite");
			printf("%d", charsWritten);
		}
		
    	printf("read complete.\n");
    }
    return 0;
}

void file_transfer(int w_fileSize)
{
	if(generate_file(w_fileSize))
	{
		perror("generating file failed.");
		return;
	}
	pipe_transfer();
}