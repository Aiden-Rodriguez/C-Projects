#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/wait.h> 
#include <sys/resource.h> 
#include <unistd.h>

#define PORT 3305

#define MIN_ARGS 2
#define MAX_ARGS 2
#define SERVER_ARG_IDX 1
#define READ_MAX 8196

#define USAGE_STRING "usage: %s <server address>\n"

char *line = NULL;
//to handle memory leak at exit
void quit_handler(int signo) 
{
   printf("\nInterrupt (code=%i)\n", signo);
    if (line)
    {
        free(line);
    }
   printf("Closing Client\n");
   exit(0);
}

void validate_arguments(int argc, char *argv[])
{
    if (argc == 0)
    {
        fprintf(stderr, USAGE_STRING, "client");
        exit(EXIT_FAILURE);
    }
    else if (argc < MIN_ARGS || argc > MAX_ARGS)
    {
        fprintf(stderr, USAGE_STRING, argv[0]);
        exit(EXIT_FAILURE);
    }
}

void send_request(int fd)
{
   size_t size;
   ssize_t num;
   ssize_t bytes_read;
   
   while ((num = getline(&line, &size, stdin)) >= 0)
   {
      //send request to server
      if (write(fd, line, num) == -1)
      {
         perror("write");
         free(line);
         return;
      }

      char response[READ_MAX];
      bytes_read = read(fd, response, sizeof(response) - 1);
      if (bytes_read > 0)
      {
         response[bytes_read] = '\0'; //null terminate the response
         printf("Received from host:\n%s", response);
      }
      else
      {
         perror("read");
         free(line);
         return;
      }
   }

   free(line);
}

int connect_to_server(struct hostent *host_entry)
{
   int fd;
   struct sockaddr_in their_addr;

   if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
   {
      return -1;
   }
   
   their_addr.sin_family = AF_INET;
   their_addr.sin_port = htons(PORT);
   their_addr.sin_addr = *((struct in_addr *)host_entry->h_addr);

   if (connect(fd, (struct sockaddr *)&their_addr,
      sizeof(struct sockaddr)) == -1)
   {
      close(fd);
      perror(0);
      return -1;
   }

   return fd;
}

struct hostent *gethost(char *hostname)
{
   struct hostent *he;

   if ((he = gethostbyname(hostname)) == NULL)
   {
      herror(hostname);
   }

   return he;
}

int main(int argc, char *argv[])
{
   validate_arguments(argc, argv);
   struct hostent *host_entry = gethost(argv[SERVER_ARG_IDX]);

	signal(SIGINT, quit_handler); 
	signal(SIGTERM, quit_handler); 
	signal(SIGQUIT, quit_handler);

   if (host_entry)
   {
      int fd = connect_to_server(host_entry);
      if (fd != -1)
      {
         send_request(fd);
         close(fd);
      }
   }

   return 0;
}
