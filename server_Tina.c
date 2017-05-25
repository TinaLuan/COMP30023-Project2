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
#include <time.h>
#include <arpa/inet.h>

#include "uint256.h"
#include "sha256.h"
#include "helper.h"

//#define NUM_THREADS 2
pthread_mutex_t mutex;
int client_num = 0;
int work_num = 0;

typedef struct  {
	int newsockfd;
	struct sockaddr_in cli_addr;
} client_info_t;

void *work_function(void *newsockfd_ptr);
int stage_A(char buffer[], client_info_t client_info);
int stage_B(char buffer[], client_info_t client_info);
int stage_C(char buffer[], client_info_t client_info);

bool verify(BYTE concat[], BYTE target[]);
void concatenate(char buffer[], BYTE concat[]);
void calculate_target(char buffer[], BYTE target[]);
BYTE hex_to_byte(char buffer[], int start);
void generate_log(struct sockaddr_in addr, int newsockfd, char *msg);



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

	FILE *log_fp = fopen("./log.txt", "w");
	fclose(log_fp);
	//pthread_t tids[NUM_THREADS]; // 2
	client_info_t client_info;

while (1){
	/* Accept a connection - block until a connection is ready to
	be accepted. Get back a new file descriptor to communicate on. */
	newsockfd = accept(	sockfd, (struct sockaddr *) &cli_addr, &clilen);
	if (newsockfd < 0) {
	   perror("ERROR on accept");
	   exit(1);
	}
	client_num++;
	if (client_num >110) {
		close(newsockfd);
		client_num--;
		continue;
	}

	generate_log( cli_addr,newsockfd, "CONNECTION\n");

	pthread_t tid;

	client_info.newsockfd = newsockfd;
	client_info.cli_addr = cli_addr;
	//pthread_create(&tid, NULL, work_function, (void *)&newsockfd);
	pthread_create(&tid, NULL, work_function, (void *)&client_info);

	//pthread_join(tid, NULL);
	pthread_detach(tid);
}

	close(sockfd);

	return 0;
}


//void *work_function(void *newsockfd_ptr ) {
void *work_function(void *client_info_ptr ) {
	client_info_t client_info = *((client_info_t *)client_info_ptr);
	char buffer[256];
	//int n, newsockfd = *((int *)newsockfd_ptr);
	int n, newsockfd = client_info.newsockfd;
	char *erro_msg; // BYTE????

	while(1) {
		bzero(buffer,256);
		/* Read characters from the connection,
		   then process */
		n = read( newsockfd ,buffer,255);
		if (n < 0) {
		   generate_log(client_info.cli_addr, newsockfd, "DISCONNECTION\n");
		   break;
		}
		if (buffer[0] == '\0') {
			generate_log(client_info.cli_addr, newsockfd, "DISCONNECTION\n");
			break;
		}
//printf("111\n");
		char msg[270] = "READ: ";
		strcat(msg, buffer);
		generate_log(client_info.cli_addr, newsockfd, msg);

		char header[5] = "";
		if (strlen(buffer)>=4) {
			strncpy(header, buffer, 4);
			header[4] = '\0';
		}
//printf("222\n");
		if (strcmp(header, "SOLN") == 0) {
			n = stage_B(buffer,client_info);
		} else if (strcmp(header, "WORK") == 0) {
			n = stage_C(buffer, client_info);
		} else {
			n = stage_A(buffer, client_info);
		}

		if (n < 0){
		   //perror("ERROR writing to socket");
		   //exit(1);
		   generate_log(client_info.cli_addr, newsockfd, "DISCONNECTION\n");
		   break;
		}
	}
	close(newsockfd);
	pthread_exit(NULL);

	return NULL; // ??????
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

 	//starts from 7th, and then 6 hex digits are beta
	j = 29;
	for (i = 5 + 2; i < 5 + 2 + 6; i+=2) {
		beta[j++] = hex_to_byte(buffer, i);
	}
	expo = alpha;
	expo -= 0x3;
	expo *= 0x8;
	base[31] = 0x2;
	uint256_exp(res, base, expo);
	uint256_mul(target, beta,res);
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

//int stage_A(char buffer[], int newsockfd) {
int stage_A(char buffer[], client_info_t client_info) {
	int n = 0;
	char *msg;
	//BYTE *erro_msg; // ??????

	if (strcmp(buffer, "PING\n") == 0 || strcmp(buffer, "PING\r\n") == 0 ) {
    msg = "PONG\r\n";
	n = write(client_info.newsockfd,msg,6);

   } else if (strcmp(buffer, "PONG\n") == 0 || strcmp(buffer, "PONG\r\n") == 0 ) {
    msg = "ERRO   'PONG' is reserved for the server\r\n";
    n = write(client_info.newsockfd, msg, strlen(msg));

   } else if (strcmp(buffer, "OK\n") == 0 || strcmp(buffer, "OK\r\n") == 0 ) {
    msg= "ERRO   It's not okay to send 'OK'\r\n";
    n = write(client_info.newsockfd, msg, strlen(msg));
	} else {
		msg= "ERRO   don't understand\r\n";
	    n = write(client_info.newsockfd, msg, strlen(msg));

	}

	if (n >= 0) {
		char log_msg[270] = "WRITE: ";
		strcat(log_msg, msg);
		generate_log(client_info.cli_addr, client_info.newsockfd, log_msg);
	}
	return n;
}

int stage_B(char buffer[], client_info_t client_info) {
	char *msg;
	int n= 0;
	bool isError = false;
	// if (strlen(buffer) != 100) {
	// 	isError = true;
	// }
	/*
	int len = str_char_count(buffer, ' ') +1;
	printf("len %d\n", len);
	char **list = tokenizer(buffer);
	// if (sizeof(list)/sizeof(char*) != 4) {
	// 	isError = true;
	// 	printf("num %d\n", sizeof(list)/sizeof(char*));
	// }
	if (len != 4) {
		msg= "ERRO invalid message\r\n";
		n = write(client_info.newsockfd, msg, strlen(msg));
		return n;
	}
	if (strlen(list[0]) != 4 || strlen(list[1]) != 8 || strlen(list[2]) != 64
	|| strlen(list[3])-2 != 16) {
		isError = true;
		printf("%d %d %d %d \n", strlen(list[0]), strlen(list[1]),
		strlen(list[2]), strlen(list[3]));
		msg= "ERRO invalid message\r\n";
		n = write(client_info.newsockfd, msg, strlen(msg));
		return n;
	}
*/

	BYTE target[32];
	calculate_target(buffer, target);

	// Concatenate the seed and the solution together
	BYTE concat[40]; // 32-BYTE seed + 8-BYTE(64bits/8) solution
	concatenate(buffer, concat);

	bool is_correct = verify(concat, target);
	if (is_correct) {
		msg = "OKAY\r\n";
		n = write(client_info.newsockfd, msg, 6);
	} else {
		msg = "ERRO It is not a valid proof-of-work.\r\n";
		n = write(client_info.newsockfd, msg, 40);
	}
	if (n >= 0) {
		char log_msg[270] = "WRITE: ";
		strcat(log_msg, msg);
		//log_msg[strlen(log_msg)] = '\0';
		generate_log(client_info.cli_addr, client_info.newsockfd, log_msg);
	}
	return n;
}
//int stage_C(char buffer[], int newsockfd) {
int stage_C(char buffer[], client_info_t client_info) {
	char *msg;
	int n= 0;
	//bool isError = false;
/*
	int len = str_char_count(buffer, ' ') +1;
	printf("len %d\n", len);
	char **list = tokenizer(buffer);
	if (len != 5) {
		msg= "ERRO invalid message\r\n";
		n = write(client_info.newsockfd, msg, strlen(msg));
		return n;
	}
	if (strlen(list[0]) != 4 || strlen(list[1]) != 8 || strlen(list[2]) != 64
	|| strlen(list[3]) != 16 || strlen(list[4])-2 != 2) {
		//isError = true;
		printf("%d %d %d %d %d\n", strlen(list[0]), strlen(list[1]),
		strlen(list[2]), strlen(list[3]), strlen(list[4]));
		msg= "ERRO invalid message\r\n";
		n = write(client_info.newsockfd, msg, strlen(msg));
		return n;
	}
*/

	work_num++;
	//if (work_num > 11) {
	if (work_num > 13) {
		msg= "ERRO too many works\r\n";
		n = write(client_info.newsockfd, msg, strlen(msg));
		work_num--;
		return n;
	}
	BYTE target[32];
	calculate_target(buffer, target);

	BYTE concat[40]; // 32-BYTE seed + 8-BYTE(64bits/8) solution
	concatenate(buffer, concat);

	BYTE nonce[32];
	uint256_init(nonce);
	int i, j=24;
	for (i=32;i<40;i++) {
		nonce[j++] = concat[i];
	}
//print_uint256(nonce);
	BYTE adder[32];
	uint256_init(adder);
	adder[31] = 0x1;

//	print_uint256(nonce);
	bool is_correct = false;
	while(!is_correct) {
		uint256_add(nonce, nonce, adder);
		j = 32;
		for (i = 24; i<32; i++) {
			concat[j++] = nonce[i];
		}
		is_correct = verify(concat, target);
	}
//printf("find sl\n");
	// According to the spec, there's always a solution
	// so no need to consider when is_correct is false
	char solution_msg[97] = "SOLN ";
	// difficulty 8 hex digits, 1 space, seed 64 hex, and 1 space
	strncpy(solution_msg + 5, buffer + 5, 8+1+64+1);
	//printf("%s\n", solution_msg);

	// solution 16 hex
	j= strlen(solution_msg);
	for (i = 32; i<40; i++) {
		sprintf(solution_msg+j, "%02x", concat[i]);
		//printf("%s\n", msg);
		j+=2;
	}
	solution_msg[95] = '\r';
	solution_msg[96] = '\n';

	//printf("%s\n", solution_msg);
	//int n =0;
	if (is_correct) {
		n = write(client_info.newsockfd, solution_msg, 97); // not including \0
	}
	if (n >= 0) {
		char log_msg[270] = "WRITE: ";
		strcat(log_msg, solution_msg);
		generate_log(client_info.cli_addr, client_info.newsockfd, log_msg);
	}
	return n;
}

void generate_log(struct sockaddr_in addr, int newsockfd, char *msg) {
	//pthread_mutex_lock(&mutex);
	FILE *fp = fopen("./log.txt", "a");
	time_t t = time(NULL);
    struct tm tm = *localtime(&t);

	fprintf(fp, "%02d/%02d/%d %02d:%02d:%02d  ", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,
	 tm.tm_hour, tm.tm_min, tm.tm_sec);

	char ip[16];
	inet_ntop(AF_INET, &(addr.sin_addr), ip, 15);
	ip[15] = '\0';
	fprintf(fp, "%s  %d  ", ip, newsockfd);

	fprintf(fp, "%s", msg);

	fclose(fp);

	printf("%02d/%02d/%d %02d:%02d:%02d  ", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,
	tm.tm_hour, tm.tm_min, tm.tm_sec);
	printf("%s  %d  ", ip, newsockfd);
	printf("%s", msg);
	//pthread_mutex_unlock(&mutex);
}
