#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <fcntl.h>

#define CHAR_BIT 8
#define INT_DECIMAL_STRING_SIZE(int_type) ((CHAR_BIT*sizeof(int_type)-1)*10/33+3)

#define TABLE_ROWS 10
#define TABLE_COLUMNS 10

#define BROADCAST_GROUP_IP "225.0.0.37"
#define MAXBUFLEN 1024
#define TCP_LISTEN_PORT 9005
#define MAX_CLIENTS 16

#define HOST 1
#define GUEST 2

#define WON 1
#define LOST 2

#define HIT 300
#define MISS 0
#define CONTINUE 100
#define I_LOST 200

#define SEND_USERNAME 1
#define SEND_PORT 2
#define SEND_HOST_PORT 3
#define SUBMIT_RESULT 4
#define REQUEST_RANDOM_RIVAL 5
#define REQUEST_SPECIFIC_RIVAL 6

#define WAITING_FOR_USERNAME 10
#define WAITING_FOR_PORT 11
#define WAITING_FOR_RIVAL 12
#define PLAYING 13
#define IDLE 14
#define WAIT_FOR_RIVAL 15
#define HOST_WON 16
#define GUEST_WON 17
#define FINISHED 18
#define DONE 19
#define ONLINE 20
#define OFFLINE 21
#define WAITING_FOR_RIVAL_REQUEST 22
#define RIVAL_OFFLINE 23

typedef enum { false, true } bool;

typedef struct Message {
    int type;
    int length;
    char* content;
} Message;

typedef struct PendingRequest {
    int requesterSocket;
    struct sockaddr_in rivalAddress;
} PendingRequest;

typedef struct Match {
    int state;
    int hostSocket;
    char* hostUsername;
    int hostStatus;
    struct sockaddr_in hostAddress;
    struct sockaddr_in guestAddress;
    int guestStatus;
    char* guestUsername;
    int guestSocket;
    int result;
} Match;

Message unpackMessage(char* data);
int sendHeartBeat(int myUDPSocket, struct sockaddr_in *myTCPAddress, struct sockaddr_in *clientsUDPAddress);
Match* findClientBySocket(int socket, Match** matches, int numOfMatches);
Match* findClientMatchBySocket(int socket, Match** matches, int numOfMatches);
Match* findRival(Match** matches, int numOfMatches, char* username);
Match* findRivalByUsername(Match** matches, int numOfMatches, char* rivalUsername);
Match* findRivalByStateAndUsername(Match** matches, int numOfMatches, char* rivalUsername, int state);
void packMessage(int type, char* data, char* out);
void packMessageByLength(int type, char* data, int size, char* out);
void requestRival(int myTCPSocket, struct sockaddr_in serverTCPAddress, char* username, int myTCPPort);
void deleteGuest(Match** matches, int numOfMatches, char* guestUsername);
int getIntInput();
int applyMove(int coordinates,  int table[TABLE_ROWS][TABLE_COLUMNS]);
int checkTable( int table[TABLE_ROWS][TABLE_COLUMNS]);
void parseMap(char* fileName, int table[TABLE_ROWS][TABLE_COLUMNS]);
void printByWrite(char* input);
Match* findMatchToSetResult(int socket, Match** matches, int numOfMatches);
int checkServerAvailability(int myUDPListenSocket, struct sockaddr_in* serverTCPAddress);
void printMatchesHistory(Match** matches, int numOfMatches);
char* intToString(int x);
int play(int socket, int myTurn, char* mapName);