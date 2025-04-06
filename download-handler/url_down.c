#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h> 
#include "url_down.h"

long maxProcesses;

//Array to track PIds to line #
ProcessInfo processInfoArray[DEFAULT_SIZE];
int processInfoCount = 0;

int main(int argc, char *argv[])
{
    limit_fork(50);

    //initialize and startup program and check arguments
    FILE *fp = initialize_program(argc, argv);

    // Use getline, for every line spawn a child process
    // For each getline, assume the format: 
    // file to write to, url, max time (optional)

    char *lineptr = NULL;
    size_t len = 0;
    int currentProcesses = 0;
    int lineCount = 1;
    
    //read the file in a loop
    while (getline(&lineptr, &len, fp) != -1)
    {
        //Use strtok to easily get segments of the input string in file
        char *tokens[MAX_TOKENS] = {0};
        int token_count = 0;

        char *token = strtok(lineptr, " ");

        // go through other tokens
        while (token != NULL && token_count < MAX_TOKENS) 
        {
            tokens[token_count++] = token;
            token = strtok(NULL, " ");
        }

        //fork program
        pid_t pid = fork();

        if (pid == -1)
        {
            perror("fork failed");
            exit(1);
        }
        //Child process does this
        else if (pid == 0)
        {
            handle_child(tokens, token_count);
        }
        else // Parent Process
        {
            //Add pid/line count to array
            processInfoArray[processInfoCount].pid = pid;
            processInfoArray[processInfoCount].num = lineCount;
            processInfoCount++;
            
            printf("Process %d processing line #%d\n", pid, lineCount);
            lineCount++;
            currentProcesses++;
            //Wait until = to max. 
            if (currentProcesses >= maxProcesses)
            {
                currentProcesses = wait_for_child(currentProcesses);
            }
        }

    }

    //finish off proccesses remaining
    while (currentProcesses > 0)
    {
        currentProcesses = wait_for_child(currentProcesses);
    }

    free(lineptr);
    fclose(fp);
    return 0;
}

// Given Function so that we dont fork bomb the server.
void limit_fork(rlim_t max_procs)
{
    struct rlimit rl;
    if (getrlimit(RLIMIT_NPROC, &rl))
    {
        perror("getrlimit");
        exit(-1);
    }
    rl.rlim_cur = max_procs;
    if (setrlimit(RLIMIT_NPROC, &rl))
    {
        perror("setrlimit");
        exit(-1);
    }
}

//removes trailing newline on string from getline
void remove_newline(char *str)
{
    int i;
    int strlength = strlen(str);
    for (i=0; i<strlength; i++)
    {
        if ((str[i] == '\n'))
        {
            str[i] = '\0';
            break;
        }
    }
    return;
}

//deals w/ child process
void handle_child(char **tokens, int token_count)
{
    // Deal with newly unparsed data
    // If no optional maxtime param
    if (token_count == 2)
    {
        // -o (filename) -s (url)

        //remove newline that is on last element from getline()
        remove_newline(tokens[1]);

        execl("/usr/bin/curl", "curl", "-o", tokens[0], "-s", tokens[1], NULL);
        perror("Execl Failure\n");
        exit(1);
    }
    // If optional maxtime param
    else if (token_count == 3)
    {
        // -o (filename) -s (url), -m (seconds)

        //remove newline that is on last element from getline()
        remove_newline(tokens[2]);

        execl("/usr/bin/curl", "curl", "-o", tokens[0], "-s", tokens[1], "-m", 
        tokens[2], NULL);
        perror("Execl Failure\n");
        exit(1);
    }
    else // if downloads is corrupted or something
    {
        printf("Given File seems to have a bad format\n");
        exit(1);
    }
}

//basic initial checks of program
FILE* initialize_program(int argc, char *argv[])
{
    //initialize and startup program and check arguments
    if (argc != 3)
    {
        printf("Usage: Filename, integer [max # of download processes]\n");
        exit(1);
    }

    FILE *fp = fopen(argv[1], "r");
    if (fp == NULL)
    {
        printf("Invalid Filename\n");
        exit(1);
    }

    char *endptr;
    maxProcesses = strtol(argv[2], &endptr, 10);
    if (endptr == argv[2])
    {
        printf("Invalid second argument (Must be positive int)\n");
        exit(1);
    }

    if (maxProcesses <= 0)
    {
        printf("Second argument (max proccesses) must be greater than 0\n");
        exit(1);
    }
    return fp;
}

//wait and print child info
int wait_for_child(int currentProcesses)
{
    int status;
    pid_t childPid = wait(&status);
    currentProcesses--;
    int i;
    for (i=0; i<processInfoCount; i++)
    {
        if (processInfoArray[i].pid == childPid)
        {
            if (WIFEXITED(status)) 
            {
            printf("Process %d [line #%d] exited with status %d\n",
            childPid, processInfoArray[i].num, WEXITSTATUS(status));
            }
        }
    }
    return currentProcesses;
}
