#define _GNU_SOURCE
#include "net.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/wait.h> 
#include <sys/resource.h> 
#include <unistd.h>

#define PORT 3305
#define ERROR_TEXT_LEN 32

//Given function so we don't fork bomb the server
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

//handle termination of children (parent process does this)
int num_processes;
void sigchld_handler(int signo) 
{
   //handle sigchilds !
   int status;
   pid_t pid;

   while ((pid = waitpid(-1, &status, WNOHANG)) > 0) 
   {
      if (WIFEXITED(status)) 
      {
         printf("Child %d exited with status %d\n", pid, WEXITSTATUS(status));
         num_processes--;
         printf("%d child processes (clients) remain\n", num_processes);
      }  
   }
}

int fd;
void quit_handler(int signo) 
{
   printf("\nInterrupt (code=%i)\n", signo);
   close(fd);
   printf("Closing Server\n");
   exit(0);
}

void handle_request(int nfd)
{
   FILE *network = fdopen(nfd, "rw");
   char *line = NULL;
   size_t size;
   ssize_t num;

   if (network == NULL)
   {
      perror("fdopen");
      close(nfd);
      return;
   }

   while ((num = getline(&line, &size, network)) >= 0)
   {
      FILE *fp;
      //Look for get input
      if (strncmp(line, "GET ", 4) == 0)
      {
         printf("GET recieved (PID = %i)\n", getpid());

         //get filepath
         char *file_path = line + 4; 
         char *end_of_path = strchr(file_path, '\n');
         if (end_of_path != NULL)
         {
            *end_of_path = '\0'; //null terminate path
         }

         //open file, if not possible sned to client error
         fp = fopen(file_path, "r");
         if (fp != NULL)
         {
            char *file_line = NULL;
            size_t file_size = 0;
            //write GET contents back to client (cat)
            while (getline(&file_line, &file_size, fp) != -1)
            {
               if (write(nfd, file_line, strlen(file_line)) == -1)
               {
                  perror("write");
                  free(file_line);
                  fclose(fp);
                  close(nfd);
                  return;
               }
            }
            printf("Sucessfully sent GET info to client\n");
            free(file_line);
            fclose(fp);
         }
         else
         {
            //filename given to host did not work out
            printf("Bad file name. Sending that info to client...\n");
            printf("Recieved  '%s', (PID = %i)\n", file_path, getpid());
            if (write(nfd, "Bad File Name. Try Again\n", ERROR_TEXT_LEN) == -1)
            {
               perror("write");
               fclose(fp);
               close(nfd);
               return;
            }
         }
      }
      else
      {
         //GET was not used at all
         //this program only responds to GET so just send back msg
         if (write(nfd, "Use GET <filename>\n", ERROR_TEXT_LEN) == -1)
         {
            perror("write");
            close(nfd);
            return;
         }
      }
   }
   free(line);
   fclose(network);
}

void run_service(int fd)
{
   //parent keeps checking for connections
   //children do the process if a connection is made
   while (1)
   {
      int nfd = accept_connection(fd);
      if (nfd != -1)
      {
         pid_t pid;
         pid = fork();
         if (pid < 0)
         {
            perror("fork");
            exit(1);
         }

         if (pid == 0)
         {
            printf("Connection established (PID = %i)\n", getpid());
            handle_request(nfd);
            //Child will return once client closes up shop2
            return;
         }
         else //Parent continues looping
         {
            num_processes++;
         }
      }
   }
}

int main(void)
{
   //Limiting the fork so we don't fork bomb
   limit_fork(25);
   fd = create_service(PORT);

   if (fd == -1)
   {
      perror(0);
      exit(1);
   }

   //set up signal handler
   if (signal(SIGCHLD, sigchld_handler) == SIG_ERR)
   {
      perror("signal");
      exit(1);
   }

   //Set up signals so that we can terminate correctly afterwards.
	signal(SIGINT, quit_handler); 
	signal(SIGTERM, quit_handler); 
	signal(SIGQUIT, quit_handler);

   printf("listening on port: %d\n", PORT);
   run_service(fd);

   //child will close upon returning here
   close(fd);

   return 0;
}
