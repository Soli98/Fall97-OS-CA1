COMPILER = gcc

all : Server.c Client.c
	${COMPILER} -o Server Server.c && ${COMPILER} -o Client Client.c

Server : Server.c
	${COMPILER} -o Server erver.c

Client : Client.c
	${COMPILER} -o Client Client.c

clean:
	rm *.o Server Client
