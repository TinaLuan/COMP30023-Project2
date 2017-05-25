
CC = gcc
CFLAG = -std=gnu99 -pthread

#all: server_Tina
#CC -o server_Tina server_Tina.c

#all: server
	#CC -o server server.c


server: server.o sha256.o
	$(CC) $(CFLAG) -o server server.o sha256.o helper.o
server.o: sha256.h uint256.h
sha256.o: sha256.h
string-helper-functions.o: string-helper-functions.h
