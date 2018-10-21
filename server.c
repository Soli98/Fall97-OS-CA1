/*
** broadcaster.c -- a datagram "client" like talker.c, except
**                  this one can broadcast
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>

int main(int argc, char *argv[])
{
    int sockfd, mysocket;
    struct sockaddr_in their_addr, my_addr; // connector's address information
    clock_t sys_time = clock();
    mysocket = socket(AF_INET, SOCK_STREAM, 0);
    if ((mysocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    my_addr.sin_family = AF_INET;     // host byte order
    my_addr.sin_port = htons(9005); // short, network byte order
    my_addr.sin_addr.s_addr = INADDR_ANY;
    struct hostent *he;
    int numbytes;
    int broadcast = 1;
    //char broadcast = '1'; // if that doesn't work, try this

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast,
        sizeof broadcast) == -1) {
        perror("setsockopt (SO_BROADCAST)");
        exit(1);
    }

    their_addr.sin_family = AF_INET;     // host byte order
    their_addr.sin_port = htons(atoi(argv[3])); // short, network byte order
    their_addr.sin_addr.s_addr = inet_addr("225.0.0.37");
    memset(their_addr.sin_zero, '\0', sizeof their_addr.sin_zero);
    printf("%s\n", inet_ntoa(my_addr.sin_addr));
    if ((numbytes=sendto(sockfd, (char*)&my_addr, strlen(argv[2]), 0,
             (struct sockaddr *)&their_addr, sizeof their_addr)) == -1) {
        perror("sendto");
        exit(1);
    }
    while(1) {
        if ((double)(clock() - sys_time) / CLOCKS_PER_SEC > 1)
        {
            sendto(sockfd, (char*)&my_addr, strlen(argv[2]), 0,
             (struct sockaddr *)&their_addr, sizeof their_addr);
            sys_time = clock();
        }

    }
    

    printf("sent %d bytes to %s\n", numbytes,
        inet_ntoa(their_addr.sin_addr));

    bind(mysocket, (struct sockaddr *) &my_addr, sizeof my_addr);
    listen(mysocket, 16);
    int acc = accept(mysocket, NULL, NULL);
    printf("%d", acc);

    close(sockfd);
    close(mysocket);

    return 0;
}