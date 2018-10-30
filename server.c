#include "header.h"

int main(int argc, char *argv[])
{
    int clientsUDPListenPort = atoi(argv[1]);
    int clientsUDPBroadcastPort = atoi(argv[2]);

    fd_set socketsToReadFDSet, masterFDSet;
    int myUDPSocket, myTCPSocket;
    struct sockaddr_in clientsUDPAddress, myTCPAddress, clientTCPAddress;
    socklen_t clientsUDPAddressLen = sizeof(clientTCPAddress);

    int bytesSent, bytesReceived, broadcast = 1, fdMax;
    clock_t sysTime = clock();
    struct PendingRequest pendingRequests[MAX_CLIENTS];
    int numOfPRs = 0;
    struct Match** matches;
    int numOfMatches = 0;
    matches = (Match**)malloc(numOfMatches*sizeof(Match*));

    char buf[MAXBUFLEN];
    char message[MAXBUFLEN];


    // Setting up the TCP listener socket
    // TODO: functionalize
    if ((myTCPSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    myTCPAddress.sin_family = AF_INET;
    myTCPAddress.sin_port = htons(TCP_LISTEN_PORT);
    myTCPAddress.sin_addr.s_addr = INADDR_ANY;
    memset(myTCPAddress.sin_zero, '\0', sizeof myTCPAddress.sin_zero);
    if (bind(myTCPSocket, (struct sockaddr *) &myTCPAddress, sizeof myTCPAddress) == -1) {
        perror("bind");
        exit(1);
    }
    if (listen(myTCPSocket, MAX_CLIENTS) == -1) {
        perror("listen");
        exit(1);
    }

    if ((myUDPSocket = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    if (setsockopt(myUDPSocket, SOL_SOCKET, SO_BROADCAST, &broadcast,
        sizeof broadcast) == -1) {
        perror("setsockopt (SO_BROADCAST)");
        exit(1);
    }
    clientsUDPAddress.sin_family = AF_INET;
    clientsUDPAddress.sin_port = htons(clientsUDPListenPort);
    clientsUDPAddress.sin_addr.s_addr = inet_addr(BROADCAST_GROUP_IP);
    memset(clientsUDPAddress.sin_zero, '\0', sizeof clientsUDPAddress.sin_zero);

    FD_ZERO(&socketsToReadFDSet);
    FD_ZERO(&masterFDSet);
    FD_SET(myTCPSocket, &masterFDSet);
    FD_SET(0, &masterFDSet);
    fdMax = myTCPSocket;
    for(;;) {
        if ((double)(clock() - sysTime) / CLOCKS_PER_SEC > 1)
        {
            bytesSent = sendHeartBeat(myUDPSocket, &myTCPAddress, &clientsUDPAddress);
            sysTime = clock();
            printByWrite("Sent Heartbeat.\n");
        }
        bzero(buf, MAXBUFLEN);
        socketsToReadFDSet = masterFDSet;
        struct timeval timeout;
        timeout.tv_usec = timeout.tv_sec = 0;
        if (select(fdMax+1, &socketsToReadFDSet, NULL, NULL, &timeout) == -1) {
            perror("select");
        }
        
        for(int i = 0; i <= fdMax; i++)
        {   
            if (FD_ISSET(0, &socketsToReadFDSet)) {
                char input[MAXBUFLEN];
                bytesReceived = read(0, input, sizeof(input));
                input[bytesReceived - 1] = '\0';
                if (strcmp(input, "status") == 0) {
                    printMatchesHistory(matches, numOfMatches);
                } 
                break;
            }
            if (FD_ISSET(i, &socketsToReadFDSet) && i > 2) {                // New connection request
                if(i == myTCPSocket) {
                    // TODO: Handle new connections
                    int new = accept(myTCPSocket, (struct sockaddr*)&clientTCPAddress, &clientsUDPAddressLen);
                    FD_SET(new, &masterFDSet);
                    if (new > fdMax) {    // keep track of the max
                        fdMax = new;
                    }
                    Match* match;
                    match = (Match*)malloc(sizeof(Match));
                    match->state = WAITING_FOR_USERNAME;
                    match->hostSocket = new;
                    match->hostAddress = clientTCPAddress;
                    match->hostStatus = ONLINE;
                    matches = (Match**)realloc(matches, sizeof(Match*) * numOfMatches);
                    matches[numOfMatches] = match;
                    numOfMatches++;

                    printByWrite("client connected to  me.\n");
                } else { // Handle data from clients
                    if ((bytesReceived = recv(i, buf, sizeof buf, 0)) <= 0) {
                        // got error or connection closed by client
                        if (bytesReceived == 0) {
                            Match* client = findClientMatchBySocket(i, matches, numOfMatches);
                            if (client->hostSocket == i) {
                                client->hostStatus = OFFLINE;
                            }
                            else {
                                client->guestStatus = OFFLINE;
                            }
                            
                            // connection closed
                            printByWrite("socketserver: socket ");
                            printByWrite(intToString(i));
                            printByWrite(" hung up.\n");
                        } else {
                            perror("recv");
                        }
                        close(i); // bye!
                        FD_CLR(i, &masterFDSet); // remove from master set
                    } else {
                        // TODO: The main block to handle all the logic
                        Message message = unpackMessage(buf);
                        printByWrite("Type: ");
                        printByWrite(intToString(message.type));
                        printByWrite("\n");
                        switch (message.type)
                        {
                            case SEND_USERNAME: {
                                Match* match;
                                match = findClientBySocket(i, matches, numOfMatches);
                                match->hostUsername = message.content;
                                match->state = WAITING_FOR_PORT;
                                printByWrite("New client. Username: ");
                                printByWrite(message.content);
                                printByWrite("\n");
                                break;
                            }
                            case SEND_PORT: {
                                Match* client = findClientBySocket(i, matches, numOfMatches);
                                printByWrite("Client username: ");
                                printByWrite(client->hostUsername);
                                printByWrite("\n");
                                client->state = WAITING_FOR_RIVAL_REQUEST;
                                client->hostAddress.sin_port = htons(*(int*)message.content);
                                printByWrite("Peer Port Number: ");
                                printByWrite(intToString(ntohs(client->hostAddress.sin_port)));
                                printByWrite("\n");
                                for(int i = 0; i < numOfPRs; i++) {
                                    if (pendingRequests[i].rivalAddress.sin_port == client->hostAddress.sin_port) {
                                        bzero(buf, MAXBUFLEN);
                                        int port = ntohs(client->hostAddress.sin_port);
                                        packMessageByLength(SEND_HOST_PORT, (char*)&(port), sizeof(int), buf);
                                        send(pendingRequests[i].requesterSocket, buf, MAXBUFLEN, 0);
                                        pendingRequests[i].rivalAddress.sin_port = 0;
                                    }
                                }
                                break;
                            }
                            case REQUEST_SPECIFIC_RIVAL: {
                                Match* client = findClientBySocket(i, matches, numOfMatches);
                                Match* rival = findRivalByUsername(matches, numOfMatches, message.content);
                                printByWrite("Found someone not NULL.\n");
                                if (rival != NULL) {
                                    if (strcmp(rival->hostUsername, message.content) == 0) {  
                                        if (rival->hostStatus == ONLINE) {
                                            Match* guest;
                                            guest = findRivalByStateAndUsername(matches, numOfMatches, message.content, WAITING_FOR_RIVAL);
                                            if (guest != NULL) {
                                                guest->guestUsername = client->hostUsername;
                                                guest->guestSocket = client->hostSocket;
                                                guest->guestAddress = client->hostAddress;
                                                guest->state = PLAYING;
                                                guest->guestStatus = client->hostStatus;
                                            }
                                            bzero(buf, MAXBUFLEN);
                                            int port = ntohs(rival->hostAddress.sin_port);
                                            packMessageByLength(SEND_HOST_PORT, (char*)&(port), sizeof(int), buf);
                                            send(i, buf, MAXBUFLEN, 0);
                                        } else {
                                            PendingRequest request;
                                            request.requesterSocket = i;
                                            request.rivalAddress = rival->hostAddress;
                                            pendingRequests[numOfPRs] = request;
                                            numOfPRs++;
                                            packMessage(RIVAL_OFFLINE, NULL, buf);
                                            send(i, buf, MAXBUFLEN, 0);
                                        }
                                    }
                                    else {
                                        if (rival->guestStatus == ONLINE) {
                                            Match* guest;
                                            guest = findRivalByStateAndUsername(matches, numOfMatches, message.content, WAITING_FOR_RIVAL);
                                            if (guest != NULL) {
                                                guest->guestUsername = client->hostUsername;
                                                guest->guestSocket = client->hostSocket;
                                                guest->guestAddress = client->hostAddress;
                                                guest->state = PLAYING;
                                                guest->guestStatus = client->hostStatus;
                                            }
                                            bzero(buf, MAXBUFLEN);
                                            int port = ntohs(rival->guestAddress.sin_port);
                                            packMessageByLength(SEND_HOST_PORT, (char*)&(port), sizeof(int), buf);
                                            send(i, buf, MAXBUFLEN, 0);
                                        } else {
                                            PendingRequest request;
                                            request.requesterSocket = i;
                                            request.rivalAddress = rival->guestAddress;
                                            pendingRequests[numOfPRs] = request;
                                            numOfPRs++;
                                            packMessage(RIVAL_OFFLINE, NULL, buf);
                                            send(i, buf, MAXBUFLEN, 0);
                                        }
                                    }
                                }
                                else {
                                    // TODO: send response
                                    printByWrite("No rival found with the provided username.\n");
                                }
                                break;
                            }
                            case REQUEST_RANDOM_RIVAL: {
                                Match* host = findClientBySocket(i, matches, numOfMatches);
                                printByWrite("Host username: ");
                                printByWrite(host->hostUsername);
                                printByWrite("\n");
                                host->state = WAITING_FOR_RIVAL;
                                // host->hostAddress.sin_port = htons(*(int*)message.content);
                                printByWrite("Peer Port Number: ");
                                printByWrite(intToString(ntohs(host->hostAddress.sin_port)));
                                printByWrite("\n");
                                Match* guest;
                                guest = findRival(matches, numOfMatches, host->hostUsername);
                                if (guest != NULL) {
                                    // Clients are matched
                                    // TODO: Send host address to guest
                                    // TODO: Delete the guest Match
                                    printByWrite("Guest username: ");
                                    printByWrite(guest->hostUsername);
                                    printByWrite("\n");
                                    guest->guestUsername = host->hostUsername;
                                    guest->guestSocket = host->hostSocket;
                                    guest->guestAddress = host->hostAddress;
                                    guest->state = PLAYING;
                                    guest->guestStatus = host->hostStatus;
                                    printByWrite("Clients ");
                                    printByWrite(guest->hostUsername);
                                    printByWrite(" and ");
                                    printByWrite(guest->guestUsername);
                                    printByWrite(" are matched.\n");
                                    bzero(buf, MAXBUFLEN);
                                    int port = ntohs(guest->hostAddress.sin_port);
                                    packMessageByLength(SEND_HOST_PORT, (char*)&(port), sizeof(int), buf);
                                    send(i, buf, MAXBUFLEN, 0);
                                    deleteGuest(matches, numOfMatches, guest->guestUsername);
                                    numOfMatches--;
                                } else {
                                    // Inform client that nobody's found
                                    printByWrite("No rival found. Wait.\n");
                                    bzero(buf, MAXBUFLEN);
                                    packMessage(WAIT_FOR_RIVAL, NULL, buf);
                                    send(i, buf, MAXBUFLEN, 0);
                                }
                                break;
                            }
                            case SUBMIT_RESULT: {
                                Match* host = findMatchToSetResult(i, matches, numOfMatches);
                                if (host->hostSocket == i) {
                                    printByWrite(host->hostUsername);
                                    printByWrite(" won against ");
                                    printByWrite(host->guestUsername);
                                    printByWrite(".\n");
                                    host->result = HOST_WON;
                                    host->state = FINISHED;
                                }
                                else {
                                    printByWrite(host->guestUsername);
                                    printByWrite(" won against ");
                                    printByWrite(host->hostUsername);
                                    printByWrite(".\n");
                                    host->result = GUEST_WON;
                                    host->state = FINISHED;
                                }
                                // close(host->hostSocket);
                                // FD_CLR(host->hostSocket, &masterFDSet);
                                // close(host->guestSocket);
                                // FD_CLR(host->guestSocket, &masterFDSet);
                                break;
                            }
                            default:
                                break;
                        }
                    }
                }
            }
        }
    }
    
    close(myUDPSocket);
    close(myTCPSocket);

    return 0;
}