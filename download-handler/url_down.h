#ifndef URL_DOWN_H
#define URL_DOWN_H

#define MAX_TOKENS 3
#define DEFAULT_SIZE 1024

void limit_fork(rlim_t max_procs);
void remove_newline(char *str);
void handle_child(char **tokens, int token_count);
FILE* initialize_program(int argc, char *argv[]);
int wait_for_child(int currentProcesses);

//Struct to match pids to line #
typedef struct
{
    pid_t pid;
    int num;
} ProcessInfo;

#endif