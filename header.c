#include "header.h"

Message unpackMessage(char* data) {
    Message message;
    message.type = *(int*)data;
	message.length = *(int*)(data + sizeof(message.type));
	message.content = (char*)malloc((message.length - (sizeof(message.type) + sizeof(message.length)) * sizeof(char)));
	memcpy(message.content, data + sizeof(message.length) + sizeof(message.type),
        message.length - sizeof(message.length) - sizeof(message.type));
	return message;
}

int sendHeartBeat(int myUDPSocket, struct sockaddr_in *myTCPAddress, struct sockaddr_in *clientsUDPAddress) {
    int bytesSent;
    if ((bytesSent = sendto(myUDPSocket, (char*)myTCPAddress, sizeof *myTCPAddress, 0,
             (struct sockaddr *)clientsUDPAddress, sizeof *clientsUDPAddress)) == -1) {
        perror("sendto");
        return -1;
    }
    return bytesSent;
}

Match* findClientBySocket(int socket, Match** matches, int numOfMatches) {
    for(int i = 0; i < numOfMatches; i++)
    {   
        if (matches[i]->hostSocket == socket)
            return matches[i];
    }
    return NULL;
}

Match* findClientMatchBySocket(int socket, Match** matches, int numOfMatches) {
    for(int i = 0; i < numOfMatches; i++)
    {   
        if (matches[i]->hostSocket == socket || matches[i]->guestSocket == socket)
            return matches[i];
    }
    return NULL;
}

Match* findMatchToSetResult(int socket, Match** matches, int numOfMatches) {
    
    for(int i = 0; i < numOfMatches; i++)
    {
        if ((matches[i]->hostSocket == socket || matches[i]->guestSocket == socket) && matches[i]->state == PLAYING) {
            return matches[i];
        }
    }
    return NULL;
}

Match* findRival(Match** matches, int numOfMatches, char* username) {
    printByWrite("findRival: ");
    for(int i = 0; i < numOfMatches; i++)
    {
        if (matches[i]->state == WAITING_FOR_RIVAL && strcmp(matches[i]->hostUsername, username) != 0 && matches[i]->hostStatus == ONLINE) {
            printByWrite("Rival with username ");
            printByWrite(matches[i]->hostUsername);
            printByWrite(" is found.\n");
            return matches[i];
        }
    }
    return NULL;
}

Match* findRivalByUsername(Match** matches, int numOfMatches, char* rivalUsername) {
    printByWrite("findRivalByUsername: \n");
    for(int i = 0; i < numOfMatches; i++)
    {
        if (matches[i]->hostUsername != NULL && strcmp(matches[i]->hostUsername, rivalUsername) == 0) {
            // if (matches[i]->hostStatus == ONLINE) {
                printf("%s, %s\n", matches[i]->hostUsername, matches[i]->guestUsername);
                return matches[i];
            // }
        } else if (matches[i]->guestUsername != NULL && strcmp(matches[i]->guestUsername, rivalUsername) == 0) {
            // if (matches[i]->guestStatus == ONLINE) {
                printf("%s, %s\n", matches[i]->hostUsername, matches[i]->guestUsername);                
                return matches[i];
            // }
        }
    }
    printByWrite("No rival found with the provided username.\n");
    return NULL;
}

Match* findRivalByStateAndUsername(Match** matches, int numOfMatches, char* rivalUsername, int state) {
    for(int i = 0; i < numOfMatches; i++)
    {
        if (strcmp(matches[i]->hostUsername, rivalUsername) == 0) { 
            if (matches[i]->state == state) {
                return matches[i];
            }
        }
    }
    return NULL;
}

void packMessage(int type, char* data, char* out) {
    Message message;
    message.type = type;
    message.length = sizeof(message.type) + sizeof(message.length);
    if (data != NULL) {
        message.length = message.length + strlen(data);
        memcpy(out, &message, sizeof(message.type) + sizeof(message.length));
        message.content = (char*)malloc(MAXBUFLEN * sizeof(char));
        strcpy(message.content, data);
        memcpy(out + sizeof(message.type) + sizeof(message.length),
			message.content, message.length - sizeof(message.type) + sizeof(message.length));

    } else {
        memcpy(out, &message, sizeof(message.type) + sizeof(message.length));
        message.content = NULL;
    }
}

void packMessageByLength(int type, char* data, int size, char* out) {
    Message message;
    message.type = type;
    message.length = sizeof(message.type) + sizeof(message.length);
    if (data != NULL) {
        message.length = message.length + strlen(data);
        memcpy(out, &message, sizeof(message.type) + sizeof(message.length));
        message.content = (char*)malloc(size * sizeof(char));
        memcpy(message.content, data, size);
        memcpy(out + sizeof(message.type) + sizeof(message.length),
            message.content, message.length - sizeof(message.type) + sizeof(message.length));
    } else {
        memcpy(out, &message, sizeof(message.type) + sizeof(message.length));
        message.content = NULL;
    }   
}

void deleteGuest(Match** matches, int numOfMatches, char* guestUsername) {
    
    for(int i = 0; i < numOfMatches; i++)
    {
        if (strcmp(matches[i]->hostUsername, guestUsername) == 0) {
            free(matches[i]);
            
            for(int j = i; j < numOfMatches - 1; i++)
            {
                matches[j] = matches[j+1];
            }
            numOfMatches = numOfMatches - 1;
            matches = (Match**)realloc(matches, sizeof(Match*) * numOfMatches);
        }
    }
}

int getIntInput() {
    char buffer[10];
    int i = 0;
    char c = '\0';
    read(STDIN_FILENO, &c, 1);
    while(c != '\n')
    {
        buffer[i] = c;
        read(STDIN_FILENO, &c, 1);
        i++;
    }
    buffer[i] = '\0';
    return atoi(buffer);
}

int applyMove(int coordinates,  int table[TABLE_ROWS][TABLE_COLUMNS]) {
    int row, column;
    row = coordinates / 10;
    column = coordinates % 10;
    
    if (table[row][column] == 1) {
        table[row][column] = 0;
        return 1;
    } else {
        return 0;
    }
}

int checkTable( int table[TABLE_ROWS][TABLE_COLUMNS]) {
    for(int i = 0; i < TABLE_ROWS; i++)
    {
        for(int j = 0; j < TABLE_COLUMNS; j++)
        {
            if (table[i][j] == 1) {
                return 1;
            }
        }
    }
    return 0;
}

void parseMap(char* fileName, int table[TABLE_ROWS][TABLE_COLUMNS]) {
    int mapFD = open(fileName, O_RDONLY);
    int fileCharsNum = TABLE_COLUMNS * TABLE_ROWS + TABLE_COLUMNS - 1;
    char buffer[TABLE_COLUMNS + 1];
    for (int i = 0; i < TABLE_ROWS; i++)
    {
        read(mapFD, buffer, TABLE_COLUMNS + 1);
        for(int j = 0; j < TABLE_COLUMNS; j++)
        {
            if (buffer[j] == '1')
            {
                table[i][j] = 1;
            }
            else if (buffer[j] == '0')
            {
                table[i][j] = 0;
            }
        }
    }
    close(mapFD);
}

void printByWrite(char* input) {
    write(STDOUT_FILENO, input, strlen(input));
}

int checkServerAvailability(int myUDPListenSocket, struct sockaddr_in* serverTCPAddress) {
    int bytesReceived;
    char buf[MAXBUFLEN];
    bytesReceived = recvfrom(myUDPListenSocket, buf, MAXBUFLEN, 0, NULL, NULL);
    buf[bytesReceived] = '\0';
    
    
    if (bytesReceived > 0) {
        *serverTCPAddress = *(struct sockaddr_in*)buf;
        return 1;
    } else {
        return 0;
    }
    
}

void printMatchesHistory(Match** matches, int numOfMatches) {
    
    for(int i = 0; i < numOfMatches; i++)
    {
        if (matches[i]->state == FINISHED) {
            printByWrite("###########\n");
            printByWrite("Match #");
            printByWrite(intToString(i + 1));
            printByWrite("\n");
            printByWrite("Host username: ");
            printByWrite(matches[i]->hostUsername);
            printByWrite("\n");
            printByWrite("Guest username: ");
            printByWrite(matches[i]->guestUsername);
            printByWrite("\n");
            printByWrite("Result: ");
            if (matches[i]->result == HOST_WON) {
                printByWrite(matches[i]->hostUsername);
                printByWrite("Won.\n");
            }
            if (matches[i]->result == GUEST_WON) {
                printByWrite(matches[i]->guestUsername);
                printByWrite("Won.\n");           
            }
            printByWrite("###########\n");
        }
    }
}

char* intToString(int x) {
  int i = x;
  char buf[INT_DECIMAL_STRING_SIZE(int)];
  char *p = &buf[sizeof buf - 1];
  *p = '\0';
  if (i >= 0) {
    i = -i;
  }
  do {
    p--;
    *p = (char) ('0' - i % 10);
    i /= 10;
  } while (i);
  if (x < 0) {
    p--;
    *p = '-';
  }
  size_t len = (size_t) (&buf[sizeof buf] - p);
  char *s = malloc(len);
  if (s) {
    memcpy(s, p, len);
  }
  return s;
}

int play(int socket, int myTurn, char* mapName) {
    int currentTurn = myTurn;
    int table[TABLE_ROWS][TABLE_COLUMNS];
    parseMap("map", table);
    char buffer[MAXBUFLEN];
    int status = PLAYING;
    int bytesReceived;
    while(status == PLAYING) {
            if (currentTurn == GUEST) {
                    printByWrite("Guest:\n");
                    if (write(socket, buffer, 0) == -1) {
                        status = WON;
                        break;
                    }
                    bytesReceived = recv(socket, buffer, MAXBUFLEN, 0);
                    buffer[bytesReceived] = '\0';
                    printByWrite("Received: ");
                    printByWrite(intToString(*(int*)buffer));
                    printByWrite("\n");
                    int input = *(int*)buffer;
                    int hit = applyMove(input, table);
                    if(checkTable(table) == 0) {
                        int message = I_LOST;
                        send(socket, (char*)&message, sizeof(int), 0);
                        printByWrite("Table is empty. Lost to Host.\n");
                        status = LOST;
                        break;
                    }
                    if (hit == 1) {
                        write(1, "Hit.\n", strlen("Hit.\n"));
                        int message = CONTINUE;
                        send(socket, (char*)&message, sizeof(int), 0);
                    }
                    if (hit == 0) {
                        write(1, "Not Hit.\n", strlen("Not Hit.\n"));
                        currentTurn = HOST;
                        int message = MISS;
                        send(socket, (char*)&message, sizeof(int), 0);
                        continue;
                    }

            }
            if (currentTurn == HOST) {
                    printByWrite("Host:\n");
                    if (checkTable(table) == 0) {
                        status = LOST;
                        int message = I_LOST;
                        send(socket, "Hello", sizeof("Hello"), 0);
                        break;
                    } else {
                        write(1, "Enter coordinates: ", strlen("Enter coordinates: "));
                        int myShot = getIntInput();
                        send(socket, (char*)&myShot, sizeof(int), 0);
                    }
                    bzero(buffer, MAXBUFLEN);
                    bytesReceived = recv(socket, buffer, MAXBUFLEN, 0);
                    buffer[bytesReceived] = '\0';
                    int input = *(int*)buffer;
                    int hit = input == MISS ? 0 : 1;
                    printByWrite("Received: ");
                    printByWrite(intToString(input));
                    printByWrite("\n");
                    if (input == I_LOST) {
                        status = WON;
                        close(socket);
                        break;
                    }
                    if (input == CONTINUE) {
                        write(1, "My turn again.\n", strlen("My turn again.\n"));
                        continue;
                    }
                    if (hit == 1) {
                        printByWrite("Hit.\n");
                        int message = CONTINUE;
                        send(socket, (char*)&message, sizeof(int), 0);
                        continue;
                    }
                    if (hit == 0) {
                        printByWrite("Missed.\n");
                        currentTurn = GUEST;
                        continue;
                    }
        }
    }
    return status;
}