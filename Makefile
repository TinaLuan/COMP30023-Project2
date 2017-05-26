
CC = gcc -std=gnu99
CFLAG = -pthread


server: server.o sha256.o helper.o
	$(CC) $(CFLAG) -o server server.o sha256.o helper.o
server.o: sha256.h uint256.h helper.h
sha256.o: sha256.h
helper.o: helper.h
