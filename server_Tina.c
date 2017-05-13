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
#include <stdbool.h>

#include "code/uint256.h"

//#define NUM_THREADS 2

void *work_function(void *newsockfd_ptr);
bool verify(char buffer[]);
BYTE hex_to_byte(char buffer[], int start);
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
   if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
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
	//buffer[strlen(buffer)-1] = '\0';

   //  (strcmp(buffer, "PING\n") == 0 || strcmp(buffer, "PING\r\n") == 0 ) {
   //  n = write(newsockfd,"PONG",4);
   //
   // } else if (strcmp(buffer, "PONG\n") == 0 || strcmp(buffer, "PONG\r\n") == 0 ) {
   //  char erro_msg[] = "ERRO   'PONG' is reserved for the server";
   //  n = write(newsockfd, erro_msg, strlen(erro_msg));
   //
   // } else if (strcmp(buffer, "OK\n") == 0 || strcmp(buffer, "OK\r\n") == 0 ) {
   //  char erro_msg[] = "ERRO   It's not okay to send 'OK'";
   //  n = write(newsockfd, erro_msg, strlen(erro_msg));
   //

	char header[5];
	strncpy(header, buffer, 4);
	header[4] = '\0';

	if (strcmp(header, "SOLN") == 0) {
		verify(buffer);

		n = write(newsockfd, "!!", 2);
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

BYTE hex_to_byte(char buffer[], int start) {
	char one_byte[2];
	strncpy(one_byte, buffer + start, 2);
	BYTE result_byte = strtoul(one_byte, NULL, 16);
	return result_byte;
}

bool verify(char buffer[]) {
	int i, j;
	uint32_t alpha, expo;
	BYTE beta[32], base[32], res[32], target[32];

	uint256_init(beta);
	uint256_init(base);
	uint256_init(res);
	uint256_init(target);

	alpha = (uint32_t)hex_to_byte(buffer, 5);

	// starts from 7th, and then 6 hex digits are beta
	j = 29;
	for (i = 5 + 2; i < 5 + 2 + 6; i+=2) {
		beta[j++] = hex_to_byte(buffer, i);
	}
	expo = alpha;
	expo -= 0x3;
	expo *= 0x8;
	printf("alpha %d\n", alpha);
	base[31] = 0x2;
	uint256_exp(res, base, expo);
	uint256_mul(target, beta, res);
	//print_uint256(target);

	BYTE result[40]; // 32-BYTE seed + 8-BYTE(64bits/8) solution
	j = 0;
	// The seed starts from index 14, with a length of 64 hex digits(2 * 32 BYTE)
	// Then a space is between seed and solution
	// The solution starts from index 14+64+1, with a length of 16 hex(64 bits/4)
	// Each pair of two hex digits (char) is a BYTE
	for (i = 14 + 0; i < 14 + 64 + 1 + 16; i+=2) {
		// skip the space
		if (i == 14 + 64) i++;
		result[j++] = hex_to_byte(buffer, i);
		printf("%hhu\n", result[j-1]);
	}


	for (i = 0; i < 32; i++) {
		if (result[i] > target[i]) {
			return false;
		}
	}
	return true;
}
