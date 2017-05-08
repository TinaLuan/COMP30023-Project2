/* A simple server in the internet domain using TCP
The port number is passed as an argument


 To compile: gcc server.c -o server
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>

#define NUM_THREADS 2

void *work_function(void *portno_from_argv);

int main(int argc, char **argv)
{


	if (argc < 2)
	{
		fprintf(stderr,"ERROR, no port provided\n");
		exit(1);
	}

	pthread_t tids[NUM_THREADS]; // 2

	//long int portno_from_argv = atoi(argv[1]);

	pthread_create(&tids[0], NULL, work_function, (void *)(argv[1]));
    pthread_create(&tids[1], NULL, work_function, (void *)(argv[1]));

	pthread_join(tids[0], NULL);
	pthread_join(tids[1], NULL);

	return 0;
}

void *work_function(void *portno_from_argv) {
	int sockfd, newsockfd, portno, clilen;
	char buffer[256];
	struct sockaddr_in serv_addr, cli_addr;
	int n;

	/* Create TCP socket */

   sockfd = socket(AF_INET, SOCK_STREAM, 0);

   if (sockfd < 0)
   {
	   perror("ERROR opening socket");
	   exit(1);
   }


   bzero((char *) &serv_addr, sizeof(serv_addr));

   portno = atoi((char *) portno_from_argv);
//printf("%d, %d\n", strlen(portno_from_argv), portno);
   /* Create address we're going to listen on (given port number)
	- converted to network byte order & any IP address for
	this machine */

   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = INADDR_ANY;
   serv_addr.sin_port = htons(portno);  // store in machine-neutral format

	/* Bind address to the socket */

   if (bind(sockfd, (struct sockaddr *) &serv_addr,
		   sizeof(serv_addr)) < 0)
   {
	   perror("ERROR on binding");
	   exit(1);
   }

   /* Listen on socket - means we're ready to accept connections -
	incoming connection requests will be queued */

   listen(sockfd,5);

   clilen = sizeof(cli_addr);

   /* Accept a connection - block until a connection is ready to
	be accepted. Get back a new file descriptor to communicate on. */

   newsockfd = accept(	sockfd, (struct sockaddr *) &cli_addr,
					   &clilen);

   if (newsockfd < 0)
   {
	   perror("ERROR on accept");
	   exit(1);
   }

   bzero(buffer,256);

   /* Read characters from the connection,
	   then process */
//while (1){
   n = read(newsockfd,buffer,255);

   if (n < 0)
   {
	   perror("ERROR reading from socket");
	   exit(1);
   }

   //printf("Here is the message: %s\n",buffer);

   // Get rid of the new line character in the end
   buffer[strlen(buffer)-1] = '\0';

   if (strcmp(buffer, "PING") == 0) {
	   n = write(newsockfd,"PONG",4);

   } else if (strcmp(buffer, "PONG") == 0) {
	   char erro_msg[] = "ERRO   'PONG' is reserved for the server";
	   n = write(newsockfd, erro_msg, strlen(erro_msg));

   } else if (strcmp(buffer, "OK") == 0) {
	   char erro_msg[] = "ERRO   It's not okay to send 'OK'";
	   n = write(newsockfd, erro_msg, strlen(erro_msg));
   }
   //n = write(newsockfd,"I got your message",18);

   if (n < 0)
   {
	   perror("ERROR writing to socket");
	   exit(1);
   }

   /* close socket */

   close(sockfd);
//}

}
