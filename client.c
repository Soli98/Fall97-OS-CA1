/*
** listener.c -- a datagram sockets "server" demo
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

#define MAXCLIENTS 16
#define MAXBUFLEN 100

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    int udp_listen_port = atoi(argv[1]);
    int my_tcp_port = atoi(argv[2]);
    int udp_listen_socket, tcp_socket;
    struct sockaddr_in host_addr, my_tcp_addr, udp_listen_addr;
    tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];

    udp_listen_socket = socket(AF_INET, SOCK_DGRAM, 0);
    udp_listen_addr.sin_family = AF_INET;
    udp_listen_addr.sin_port = htons(udp_listen_port);
    udp_listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    uint32_t reuseport = 1;
    setsockopt(udp_listen_socket, SOL_SOCKET, SO_REUSEPORT, &reuseport, sizeof(reuseport));

    // struct timeval tv;
    // tv.tv_sec = 2;
    // tv.tv_usec = 1000;
    // if (setsockopt(udp_listen_socket, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
    //     const char err[] ="setsockopt: timeval\n" ;
    //     write(STDERR_FILENO, err, sizeof(err)-1);
    // }

    if (bind(udp_listen_socket, (struct sockaddr *) &udp_listen_addr, sizeof udp_listen_addr) == -1) {
        close(udp_listen_socket);
        perror("listener: bind");
    }
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr("225.0.0.37");
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    
    if (setsockopt(udp_listen_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &mreq, sizeof(mreq)) < 0){
        const char err[] ="setsockopt: mreq\n" ;
        write(STDERR_FILENO, err, sizeof(err)-1);
        exit(EXIT_FAILURE);
    }
    printf("listener: waiting to receive from server...\n");

    addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(udp_listen_socket, buf, MAXBUFLEN-1 , 0,
        (struct sockaddr *)&their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }
    host_addr = *(struct sockaddr_in*) buf;
    printf("listener: waiting to connect to server...\n");

    my_tcp_addr.sin_family = AF_INET;     // host byte order
    my_tcp_addr.sin_port = htons(my_tcp_port); // short, network byte order
    my_tcp_addr.sin_addr.s_addr = INADDR_ANY;
    bind(tcp_socket, (struct sockaddr *) &my_tcp_addr, sizeof my_tcp_addr);

    int c = connect(tcp_socket, (struct sockaddr *) &host_addr, sizeof host_addr);
    printf("CONNECTED TO HOST!!!!!!!!! %d\n", c);
    printf("listener: packet is %d bytes long\n", numbytes);
    buf[numbytes] = '\0';

    close(udp_listen_socket);
    close(tcp_socket);

    return 0;
}