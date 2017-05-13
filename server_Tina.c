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
#include "code/crypto/sha256.c"

//#define NUM_THREADS 2

void *work_function(void *newsockfd_ptr);
int stage_A(char buffer[], int newsockfd);
int stage_B(char buffer[], int newsockfd);
int stage_C(char buffer[], int newsockfd);

bool verify(BYTE concat[], BYTE target[]);
void concatenate(char buffer[], BYTE concat[]);
void calculate_target(char buffer[], BYTE target[]);
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
	int n, newsockfd = *((int *)newsockfd_ptr);
	char *erro_msg; // BYTE????
	bzero(buffer,256);

	/* Read characters from the connection,
	   then process */
	n = read( newsockfd ,buffer,255);
	if (n < 0) {
	   perror("ERROR reading from socket");
	   exit(1);
	}

	n= stage_A(buffer, newsockfd);
	if (n < 0)
	{
	   perror("ERROR writing to socket");
	   exit(1);
	}

	char header[5];
	strncpy(header, buffer, 4);
	header[4] = '\0';

	if (strcmp(header, "SOLN") == 0) {
		n = stage_B(buffer, newsockfd);
	}

	if (n < 0)
	{
	   perror("ERROR writing to socket");
	   exit(1);
	}

	if (strcmp(header, "WORK") == 0) {

		//n = stage_C(buffer, newsockfd);
	}

 //	close( *((int *)sockfd_ptr) );
	return NULL; // ??????
}

int stage_C(char buffer[], int newsockfd) {
	BYTE target[32];
	calculate_target(buffer, target);
	return 0;
}

BYTE hex_to_byte(char buffer[], int start) {
	char one_byte[3];
	strncpy(one_byte, buffer + start, 2);
	one_byte[2] = '\0';
	BYTE result_byte = strtoul(one_byte, NULL, 16);
	//printf("%s  %x\n", one_byte, result_byte);
	return result_byte;
}

void calculate_target(char buffer[], BYTE target[]) {
	int i, j;
	uint32_t alpha, expo;
	BYTE beta[32], base[32], res[32];

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
	base[31] = 0x2;
	uint256_exp(res, base, expo);
	uint256_mul(target, beta, res);
	//print_uint256(target);
}

void concatenate(char buffer[], BYTE concat[]) {
	int i, j = 0;
	// The seed starts from index 14, with a length of 64 hex digits(2 * 32 BYTE)
	// Then a space is between seed and solution
	// The solution starts from index 14+64+1, with a length of 16 hex(64 bits/4)
	// Each pair of two hex digits (char) is a BYTE
	for (i = 14 + 0; i < 14 + 64 + 1 + 16; i+=2) {
		// skip the space
		if (i == 14 + 64) i++;
		concat[j++] = hex_to_byte(buffer, i);
	}
}

int stage_B(char buffer[], int newsockfd) {
	char *erro_msg= "ERRO It is not a valid proof-of-work.\n";
	int n = 0;

	BYTE target[32];
	calculate_target(buffer, target);

	// Concatenate the seed and the solution together
	BYTE concat[40]; // 32-BYTE seed + 8-BYTE(64bits/8) solution
	concatenate(buffer, concat);

	bool is_correct = verify(concat, target);
	if (is_correct) {
		n = write(newsockfd, "OK", 3);
	} else {
		n = write(newsockfd, erro_msg, 40);
	}
	return n;
}

bool verify(BYTE concat[], BYTE target[]) {
	SHA256_CTX ctx;
	BYTE hash1[SHA256_BLOCK_SIZE], hash2[SHA256_BLOCK_SIZE]; // 32
	sha256_init(&ctx);
	sha256_update(&ctx, concat, 40);
	sha256_final(&ctx, hash1);

	sha256_init(&ctx);
	sha256_update(&ctx, hash1, SHA256_BLOCK_SIZE);
	sha256_final(&ctx, hash2);
	//print_uint256(hash1);
	//print_uint256(hash2);

	int i = 0;
	while (i<32) {
		if (hash2[i] < target[i]) return true;
		if (hash2[i] > target[i]) return false;
		if (hash2[i] == target[i]) i++;
	}
	return false;
}

int stage_A(char buffer[], int newsockfd) {
	int n = 0;
	char *erro_msg;
	//BYTE *erro_msg; // ??????

	if (strcmp(buffer, "PING\n") == 0 || strcmp(buffer, "PING\r\n") == 0 ) {
    n = write(newsockfd,"PONG",5);

   } else if (strcmp(buffer, "PONG\n") == 0 || strcmp(buffer, "PONG\r\n") == 0 ) {
    erro_msg = "ERRO   'PONG' is reserved for the server";
    n = write(newsockfd, erro_msg, 40);

   } else if (strcmp(buffer, "OK\n") == 0 || strcmp(buffer, "OK\r\n") == 0 ) {
    erro_msg= "ERRO   It's not okay to send 'OK'";
    n = write(newsockfd, erro_msg, 40);
	}
	return n;
}
