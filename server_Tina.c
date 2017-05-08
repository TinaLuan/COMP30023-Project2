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

void *work_function(void *newsockfd_ptr);

int main(int argc, char **argv)
{


	if (argc < 2)
	{
		fprintf(stderr,"ERROR, no port provided\n");
		exit(1);
	}

	int sockfd, newsockfd, portno;//, clilen;
	socklen_t clilen;
	//char buffer[256];
	struct sockaddr_in serv_addr, cli_addr;



	/* Create TCP socket */

   sockfd = socket(AF_INET, SOCK_STREAM, 0);

   if (sockfd < 0)
   {
	   perror("ERROR opening socket");
	   exit(1);
   }


   bzero((char *) &serv_addr, sizeof(serv_addr));

   portno = atoi(argv[1]);
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

	//pthread_t tids[NUM_THREADS]; // 2
while (1){
	/* Accept a connection - block until a connection is ready to
	be accepted. Get back a new file descriptor to communicate on. */

	newsockfd = accept(	sockfd, (struct sockaddr *) &cli_addr, &clilen);
	if (newsockfd < 0)
	{
	   perror("ERROR on accept");
	   exit(1);
	}

	pthread_t tid;
	pthread_create(&tid, NULL, work_function, (void *)&newsockfd);
	//pthread_join(tid, NULL);
}
	close(sockfd);
	//pthread_exit(NULL);
	return 0;
}


void *work_function(void *newsockfd_ptr) {
	char buffer[256];
	int n;
	int newsockfd = *((int *)newsockfd_ptr);

	bzero(buffer,256);

	/* Read characters from the connection,
	   then process */
	n = read( newsockfd ,buffer,255);

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
	

	if (n < 0)
	{
	   perror("ERROR writing to socket");
	   exit(1);
	}

	/* close socket */

	//close(sockfd);
//	close( *((int *)sockfd_ptr) );

	return NULL; // ??????
}
