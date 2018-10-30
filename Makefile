COMPILER = gcc

all : header.h header.c Server.c Client.c
	${COMPILER} -o Server header.c Server.c && ${COMPILER} -o Client header.c Client.c

Server : header.h header.c Server.c
	${COMPILER} -o Server header.c Server.c

Client : header.c header.h Client.c
	${COMPILER} -o Client header.c Client.c

clean:
	rm *.o Server Client