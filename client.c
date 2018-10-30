#include "header.h"

int main(int argc, char *argv[])
{
    fd_set socketsToReadFDSet, masterFDSet;
    int myUDPListenPort = atoi(argv[1]);
    int clientsUDPListenPort = atoi(argv[2]);
    int myTCPGamePort;
    int myState = IDLE;
    int myUDPListenSocket, myTCPSocket, myTCPGameSocket, gamePlaySocket, myUDPBroadcastSocket, myClientUDPListenSocket;
    struct sockaddr_in serverTCPAddress, myTCPAddress, myUDPListenAddress, myTCPGameAddress, gamePlayAddress, clientsUDPListenAddress;
    int bytesReceived, bytesSent;
    struct sockaddr_storage theirAddr;
    char buf[MAXBUFLEN];
    char message[MAXBUFLEN];
    socklen_t addrLen;
    bool serverIsUp = true;
    bool loggedIn = false;
    int result;
    int fdMax;
    int broadcast = 1;
    clock_t sysTime = clock();
    gamePlaySocket = socket(AF_INET, SOCK_STREAM, 0);

    if ((myUDPBroadcastSocket = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    if (setsockopt(myUDPBroadcastSocket, SOL_SOCKET, SO_BROADCAST, &broadcast,
        sizeof broadcast) == -1) {
        perror("setsockopt (SO_BROADCAST)");
        exit(1);
    }
    clientsUDPListenAddress.sin_family = AF_INET;
    clientsUDPListenAddress.sin_port = htons(clientsUDPListenPort);
    clientsUDPListenAddress.sin_addr.s_addr = inet_addr(BROADCAST_GROUP_IP);
    memset(clientsUDPListenAddress.sin_zero, '\0', sizeof clientsUDPListenAddress.sin_zero);

    myUDPListenSocket = socket(AF_INET, SOCK_DGRAM, 0);
    myUDPListenAddress.sin_family = AF_INET;
    myUDPListenAddress.sin_port = htons(myUDPListenPort);
    myUDPListenAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    uint32_t reuseport = 1;
    setsockopt(myUDPListenSocket, SOL_SOCKET, SO_REUSEPORT, &reuseport, sizeof(reuseport));
    
    struct timeval read_timeout;
    read_timeout.tv_sec = 2;
    read_timeout.tv_usec = 0;
    setsockopt(myUDPListenSocket, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);

    if (bind(myUDPListenSocket, (struct sockaddr *) &myUDPListenAddress, sizeof myUDPListenAddress) == -1) {
        close(myUDPListenSocket);
        perror("listener: bind");
    }

    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(BROADCAST_GROUP_IP);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(myUDPListenSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &mreq, sizeof(mreq)) < 0){
        const char err[] ="setsockopt: mreq\n" ;
        write(STDERR_FILENO, err, sizeof(err)-1);
        exit(EXIT_FAILURE);
    }
    //
    myClientUDPListenSocket = socket(AF_INET, SOCK_DGRAM, 0);
    clientsUDPListenAddress.sin_family = AF_INET;
    clientsUDPListenAddress.sin_port = htons(clientsUDPListenPort);
    clientsUDPListenAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    setsockopt(myClientUDPListenSocket, SOL_SOCKET, SO_REUSEPORT, &reuseport, sizeof(reuseport));
    setsockopt(myClientUDPListenSocket, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);
    if (bind(myClientUDPListenSocket, (struct sockaddr *) &clientsUDPListenAddress, sizeof clientsUDPListenAddress) == -1) {
        close(myClientUDPListenSocket);
        perror("listener: bind");
    }
    if (setsockopt(myClientUDPListenSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &mreq, sizeof(mreq)) < 0){
        const char err[] ="setsockopt: mreq\n" ;
        write(STDERR_FILENO, err, sizeof(err)-1);
        exit(EXIT_FAILURE);
    }
    //
    char username[MAXBUFLEN];
    printByWrite("Please enter your username: ");
    bytesReceived = read(STDIN_FILENO, username, sizeof(username));
    username[bytesReceived - 1] = '\0';

    printByWrite("listener: waiting to receive from server...\n");
    
    // Setting up game port
    myTCPGameSocket = socket(AF_INET, SOCK_STREAM, 0);
    myTCPGameAddress.sin_family = AF_INET;
    myTCPGameAddress.sin_port = 0;
    myTCPGameAddress.sin_addr.s_addr = INADDR_ANY;
    bind(myTCPGameSocket, (struct sockaddr *) &myTCPGameAddress, sizeof myTCPGameAddress);
    listen(myTCPGameSocket, MAX_CLIENTS);
    

    // Connecting to server
    myTCPSocket = socket(AF_INET, SOCK_STREAM, 0);
    myTCPAddress.sin_family = AF_INET;
    myTCPAddress.sin_port = 0;
    myTCPAddress.sin_addr.s_addr = INADDR_ANY;
    bind(myTCPSocket, (struct sockaddr *) &myTCPAddress, sizeof myTCPAddress);
    struct sockaddr_in peerAddress;
    int mode = 0;
    for(;;) {
        if (mode == 0) {
            if (checkServerAvailability(myUDPListenSocket, &serverTCPAddress) == 0) {
                serverIsUp = false;
                printByWrite("Server is offline. You can:\n");
                printByWrite("0: Check server availability.\n");
                printByWrite("3: Find a random opponent.\n");
                printByWrite("4: Find a certain opponent with username.\n");
                printByWrite("Enter mode number: ");
                mode = getIntInput();
            } else {
                serverIsUp = true;
                if (!loggedIn) {
                    printByWrite("Server Port Number: ");
                    printByWrite(intToString(ntohs(serverTCPAddress.sin_port)));
                    printByWrite("\n");
                    if (connect(myTCPSocket, (struct sockaddr*)&serverTCPAddress, sizeof(serverTCPAddress)) == -1) {
                            perror("connect:");
                        } else {
                            printByWrite("connected to server\n");
                        }
                    socklen_t myTCPGameAddressLen = sizeof(myTCPGameAddress);
                    getsockname(myTCPGameSocket, (struct sockaddr*)&myTCPGameAddress, &myTCPGameAddressLen);
                    printByWrite("Game Port Number: ");
                    printByWrite(intToString(ntohs(peerAddress.sin_port)));
                    printByWrite("\n");
                    myTCPGamePort = ntohs(myTCPGameAddress.sin_port);
                    packMessage(SEND_USERNAME, username, message);
                    send(myTCPSocket, message, MAXBUFLEN, 0);
                    packMessageByLength(SEND_PORT, (char*)&myTCPGamePort, sizeof(int), message);            
                    send(myTCPSocket, message, MAXBUFLEN, 0);
                    loggedIn = true;
                }
                printByWrite("Server is online. You can:\n");
                printByWrite("1: Request server to match a random opponent.\n");
                printByWrite("2: Request server to match a certain opponent with username.\n");
                printByWrite("3: Find a random opponent.\n");
                printByWrite("4: Find a certain opponent with username.\n");
                printByWrite("Enter mode number: ");
                mode = getIntInput();
            }
        }
        if (mode == 1) {
            if (myState == IDLE) {
                packMessageByLength(REQUEST_RANDOM_RIVAL, (char*)&myTCPGamePort, sizeof(int), message);
                send(myTCPSocket, message, MAXBUFLEN, 0);
                bzero(buf, MAXBUFLEN);
                recv(myTCPSocket, buf, MAXBUFLEN, 0);
                Message serverResponse = unpackMessage(buf);
                myState = serverResponse.type;
                
                if (serverResponse.type == SEND_HOST_PORT) {
                    peerAddress.sin_addr.s_addr = INADDR_ANY;
                    peerAddress.sin_port = htons(*(int*)serverResponse.content);
                    peerAddress.sin_family = AF_INET;
                    printByWrite("Peer Port Number: ");
                    printByWrite(intToString(ntohs(peerAddress.sin_port)));
                    printByWrite("\n");
                }
            }
            if (myState == WAIT_FOR_RIVAL) {
                printByWrite("Waiting for rival.\n");
                int opponent = accept(myTCPGameSocket, NULL, NULL);
                result = play(opponent, HOST, "map");
                myState = DONE;
            }

            
            if (myState == SEND_HOST_PORT) {
                printByWrite("Waiting to connect to host rival...\n");
                int connection = connect(gamePlaySocket, (struct sockaddr*)&peerAddress, sizeof(peerAddress));
                if(connection != -1) printByWrite("Connected to host rival.\n");
                result = play(gamePlaySocket, GUEST, "map");
                // close(myTCPGameSocket);
                myState = DONE;
            }
            
            if (myState == DONE && result == WON) {
                packMessage(SUBMIT_RESULT, NULL, message);
                send(myTCPSocket, message, MAXBUFLEN, 0);
                // close(myTCPSocket);
                myState = IDLE;
            }
            
            if (myState == DONE) {
                myState = IDLE;
            }
            
            if (result == LOST) {
                myState = IDLE;
            }
            mode = 0;
        }
        if (mode == 2) {
            if (myState == IDLE) {
                char rivalUsername[MAXBUFLEN];
                printByWrite("Please enter opponent's username: ");
                bytesReceived = read(STDIN_FILENO, rivalUsername, sizeof(rivalUsername));
                rivalUsername[bytesReceived - 1] = '\0';
                packMessage(REQUEST_SPECIFIC_RIVAL, rivalUsername, message);
                send(myTCPSocket, message, MAXBUFLEN, 0);
                bzero(buf, MAXBUFLEN);
                recv(myTCPSocket, buf, MAXBUFLEN, 0);
                Message serverResponse = unpackMessage(buf);
                if (serverResponse.type == SEND_HOST_PORT) {
                    peerAddress.sin_addr.s_addr = INADDR_ANY;
                    peerAddress.sin_port = htons(*(int*)serverResponse.content);
                    peerAddress.sin_family = AF_INET;
                    printByWrite("Peer Port Number: ");
                    printByWrite(intToString(ntohs(peerAddress.sin_port)));
                    printByWrite("\n");
                    myState = SEND_HOST_PORT;
                    mode = 1;
                    continue;
                }
                
                if (serverResponse.type == RIVAL_OFFLINE) {
                    myState = WAIT_FOR_RIVAL;
                    mode = 1;
                    continue;
                }
                mode = 0;
            }
        }
        if (mode == 3) {
            socklen_t myTCPGameAddressLen = sizeof(myTCPGameAddress);
            getsockname(myTCPGameSocket, (struct sockaddr*)&myTCPGameAddress, &myTCPGameAddressLen);
            int host = 0;
            myTCPGamePort = ntohs(myTCPGameAddress.sin_port);            
            struct sockaddr_in rivalAddress;
            memset((char*)&rivalAddress, 0, sizeof(rivalAddress));
            FD_ZERO(&socketsToReadFDSet);
            FD_ZERO(&masterFDSet);
            FD_SET(myTCPGameSocket, &masterFDSet);
            FD_SET(myClientUDPListenSocket, &masterFDSet);
            int availability = 0;
            if (myTCPGameSocket > myClientUDPListenSocket) {
                fdMax = myTCPGameSocket;
            } else {
                fdMax = myClientUDPListenSocket;
            }
            int broke = 0;
            while (!broke) {
                if ((double)(clock() - sysTime) / CLOCKS_PER_SEC > 1)
                {
                    bytesSent = sendHeartBeat(myUDPBroadcastSocket, &myTCPGameAddress, &clientsUDPListenAddress);
                    printByWrite("Sent heartbeat.\n");
                    sysTime = clock();
                }
                socketsToReadFDSet = masterFDSet;
                struct timeval timeout;
                timeout.tv_usec = timeout.tv_sec = 0;
                if (select(fdMax+1, &socketsToReadFDSet, NULL, NULL, &timeout) == -1) {
                    perror("select");
                }
                for(int i = 0; i <= fdMax; i++) {
                    if (FD_ISSET(i, &socketsToReadFDSet) && i > 2) {
                        if (i == myTCPGameSocket) {
                            int new = accept(myTCPGameSocket, NULL, NULL);
                            result = play(new, HOST, "map");    
                            mode = 0;
                            myState = IDLE;
                            FD_SET(new, &masterFDSet);
                            if (new > fdMax) {    // keep track of the max
                                fdMax = new;
                            }
                            broke = 1;
                            break;
                        }
                        if (i == myClientUDPListenSocket) {
                            bytesReceived = recvfrom(myClientUDPListenSocket, buf, MAXBUFLEN, 0, NULL, NULL);
                            buf[bytesReceived] = '\0';
                            char temp[MAXBUFLEN];
                            strcpy(temp, buf);
                            
                            if (strcmp(strtok(temp, " "), "Username") == 0) {
                                strcpy(temp, buf);
                                if (strcmp(strtok(temp, " "), username) == 0) {
                                    availability = 1;
                                    rivalAddress.sin_port = htons(atoi(temp));
                                    rivalAddress.sin_addr.s_addr = INADDR_ANY;
                                    rivalAddress.sin_family = AF_INET;
                                } else {
                                    availability = 0;
                                }
                            } else {
                                rivalAddress = *(struct sockaddr_in*)buf;
                                if (bytesReceived > 0 && ntohs(rivalAddress.sin_port) != myTCPGamePort) {
                                    availability = 1;
                                } else {
                                    availability = 0;
                                }
                            }
                            if (availability == 1) {
                                int c = connect(gamePlaySocket, (struct sockaddr*)&rivalAddress, sizeof(rivalAddress));
                                if(c != -1) printByWrite("Connected to host rival.\n");
                                result = play(gamePlaySocket, GUEST, "map");
                                mode = 0;
                                myState = IDLE;
                                broke = 1;
                                myState = IDLE;
                            }
                        }
                    }
                }
            }
        }
        if (mode == 4) {
            char rivalUsername[MAXBUFLEN];
            printByWrite("Please enter opponent's username: ");
            bytesReceived = read(STDIN_FILENO, rivalUsername, sizeof(rivalUsername));
            rivalUsername[bytesReceived - 1] = '\0';
            socklen_t myTCPGameAddressLen = sizeof(myTCPGameAddress);
            getsockname(myTCPGameSocket, (struct sockaddr*)&myTCPGameAddress, &myTCPGameAddressLen);
            int host = 0;
            myTCPGamePort = ntohs(myTCPGameAddress.sin_port);            
            struct sockaddr_in rivalAddress;
            memset((char*)&rivalAddress, 0, sizeof(rivalAddress));
            FD_ZERO(&socketsToReadFDSet);
            FD_ZERO(&masterFDSet);
            FD_SET(myTCPGameSocket, &masterFDSet);
            FD_SET(myClientUDPListenSocket, &masterFDSet);
            int availability = 0;
            if (myTCPGameSocket > myClientUDPListenSocket) {
                fdMax = myTCPGameSocket;
            } else {
                fdMax = myClientUDPListenSocket;
            }
            int broke = 0;
            while (!broke) {
                if ((double)(clock() - sysTime) / CLOCKS_PER_SEC > 1)
                {
                    bzero(message, MAXBUFLEN);
                    strcat(message, "Username ");
                    strcat(message, rivalUsername);
                    strcat(message, " ");
                    strcat(message, intToString(myTCPGamePort));
                    sendto(myUDPBroadcastSocket , message, MAXBUFLEN, 0, (struct sockaddr*)&clientsUDPListenAddress, sizeof clientsUDPListenAddress);
                    printByWrite("Sent heartbeat.\n");
                    sysTime = clock();
                }
                socketsToReadFDSet = masterFDSet;
                struct timeval timeout;
                timeout.tv_usec = timeout.tv_sec = 0;
                if (select(fdMax+1, &socketsToReadFDSet, NULL, NULL, &timeout) == -1) {
                    perror("select");
                }
                for(int i = 0; i <= fdMax; i++) {
                    if (FD_ISSET(i, &socketsToReadFDSet) && i > 2) {
                        if (i == myTCPGameSocket) {
                            int new = accept(myTCPGameSocket, NULL, NULL);
                            result = play(new, HOST, "map");    
                            mode = 0;
                            myState = IDLE;
                            FD_SET(new, &masterFDSet);
                            if (new > fdMax) {    // keep track of the max
                                fdMax = new;
                            }
                            broke = 1;
                            break;
                        }
                        if (i == myClientUDPListenSocket) {
                            bytesReceived = recvfrom(myClientUDPListenSocket, buf, MAXBUFLEN, 0, NULL, NULL);
                            buf[bytesReceived] = '\0';
                            char temp[MAXBUFLEN];
                            strcpy(temp, buf);
                            
                            if (strcmp(strtok(temp, " "), "Username") == 0) {
                                strcpy(temp, buf);
                                if (strcmp(strtok(temp, " "), username) == 0) {
                                    availability = 1;
                                    rivalAddress.sin_port = htons(atoi(temp));
                                    rivalAddress.sin_addr.s_addr = INADDR_ANY;
                                    rivalAddress.sin_family = AF_INET;
                                } else {
                                    availability = 0;
                                }
                            } else {
                                rivalAddress = *(struct sockaddr_in*)buf;
                                if (bytesReceived > 0 && ntohs(rivalAddress.sin_port) != myTCPGamePort) {
                                    availability = 1;
                                } else {
                                    availability = 0;
                                }
                            }
                            if (availability == 1) {
                                int c = connect(gamePlaySocket, (struct sockaddr*)&rivalAddress, sizeof(rivalAddress));
                                if(c != -1) printByWrite("Connected to host rival.\n");
                                result = play(gamePlaySocket, GUEST, "map");
                                mode = 0;
                                myState = IDLE;
                                broke = 1;
                                myState = IDLE;
                            }
                        }
                    }
                }
            }
        }
    }
    close(myUDPListenSocket);
    return 0;
}
