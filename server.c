//
// Server Interface implementation
 
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils/transport.h"
#include "utils/nethelper.h"
#include "utils/printHelpers.h"
#include "collections/linkedList.h"
#include "collections/hashTable.h"
#include "chatServer.h"


/* Mutex lock to guard the runtime thread buffer */
pthread_mutex_t connectionsMutex = PTHREAD_MUTEX_INITIALIZER;
/* Signaling condition for awaiting threads on the runtime thread buffer */
pthread_cond_t connectionsCond = PTHREAD_COND_INITIALIZER;

/* Linked List for all pthread connections */
LinkedList *connections;
/* Hash table for the chat room sessions */
HashTable *sessions;
/* Hash table for username/passwords */
HashTable *users;

/**
 * @brief Function delcaration for the thread function calloc
 */
void *threadCall(void *args);

/**
 * @brief Returns a new ThreadInfo
 */
ThreadInfo *getThreadInfo();

/**
 * @brief Releases the thread back into the thread pool
 */
void releaseThread(ThreadInfo *thread);

/* Maximum number of simultaneous connections */
#define MAX_CONNECTIONS 16
#define MAX_USERS_PER_SESSION 32

int main(int argc, char **argv) {
	if(argc != 2) {
		printf("Usage: server <port>\n");
		return 0;
	}

	int sock = getServerSocket(argv[1]);

	if (sock < 0) {
	    return 0;
	}

	// Init the storage structures
	connections = ll_init();
	sessions = ht_init(64);
	users = ht_init(128);

	// Init the passwords
	FILE *fp = fopen("passwords.txt", "r");
	if(fp == NULL) {
		printLastError("Error at fopen(): %s\n");
	}
	
	// Parse the passwords file
	char *str = NULL;
	size_t len;
	ssize_t read;
	while ((read = getline(&str, &len, fp)) != -1) {
		int parts;
	    char **tokens = parseTokens(str, "\t", &parts);

	    char *username = (char *)calloc(strlen(tokens[0]) + 1, sizeof(char));
	    char *password = (char *)calloc(strlen(tokens[1]), sizeof(char));
	    strcpy(username, tokens[0]);
	    memcpy(password, tokens[1], strlen(tokens[1]) - 1);
	    password[strlen(tokens[1])] = '\0';

	    ht_insert(users, username, password);

	    fflush(stdin);

	    free(tokens);
	}

	do {
		struct sockaddr clientInfo;
		socklen_t clientLen = sizeof(clientInfo);

		// Get an available thread, or sleep until a thread is free
		ThreadInfo *thread = getThreadInfo();
		thread->clientAddrLen = sizeof(clientInfo);
		
		// Block and accept a new client connection
		threadaccept:
		thread->socket = accept(sock, &clientInfo, &clientLen);
		if(thread->socket < 0) {
			//System interrupted go back to accept it
			if(errno == EINTR) {
			    goto threadaccept;
			}
		}
		thread->sessions = sessions;
		thread->connections = connections;
		thread->users = users;
		// Init the socket's lock
		pthread_mutex_init(&thread->socketLock, NULL);

		printf("Connected client on socket: %d\n", thread->socket);

		// Detach the thread
		pthread_create(&thread->thread, NULL, threadCall, thread);
		pthread_detach(thread->thread);
	} while(1);

	return 0;
}

void* threadCall(void *args) {
	ThreadInfo *threadInfo = (ThreadInfo *)args;

	// Loop until the client exists and handle the command
	threadInfo->clientConnected = 1;
	while(threadInfo->clientConnected) {
		// Begin reading from the socket
		unsigned char buf[MAX_PACKET_SIZE];
		memset(buf, 0, MAX_PACKET_SIZE);
		int bytes = recv(threadInfo->socket, buf, MAX_PACKET_SIZE, 0);

		if (bytes <= 0) {
			threadInfo->clientConnected = 0;
			continue;
		}

		printf("INFO: RECV %d bytes: %.*s\n", bytes, bytes, buf);

		// Convert the request to a packet
		Packet *requestPacket = bytesToPacket(buf, bytes);
		Packet *responsePacket;

		fflush(stdout);
		switch(requestPacket->type) {
			case LOGIN:
			    responsePacket = chatServer_login(threadInfo, requestPacket);
			    break;
			case EXIT:
			    responsePacket = chatServer_exit(threadInfo, requestPacket);
			    break;
			case JOIN:
			    responsePacket = chatServer_sessionJoin(threadInfo, requestPacket);
			    break;
			case LEAVE_SESS:
			    responsePacket = chatServer_sessionLeave(threadInfo, requestPacket);
			    break;
			case NEW_SESS:
			    responsePacket = chatServer_sessionCreate(threadInfo, requestPacket);
			    break;
			case QUERY:
				responsePacket = chatServer_sessionQuery(threadInfo, requestPacket);
				break;
			case MESSAGE:
			    responsePacket = chatServer_message(threadInfo, requestPacket, buf, bytes);
			    break;
			default:
				responsePacket = (Packet *)calloc(1, sizeof(Packet));
				responsePacket->type = UNKNOWN;
				char* unknownMessage = "Unknown request.";
				memcpy(responsePacket->data, unknownMessage, strlen(unknownMessage));
				responsePacket->size = strlen(unknownMessage);
				break;
		}
		free(requestPacket);

		if (responsePacket != NULL) {
			int responseLength;
			unsigned char *response = packetToByteArray(responsePacket, &responseLength);
			send(threadInfo->socket, response, responseLength, 0);
			free(response);
			response = NULL;
			free(responsePacket);
			responsePacket = NULL;
		}
	}

	close(threadInfo->socket);

	releaseThread(args);
	return NULL;
}

/**
 * @brief Semaphore implementation to prevent the server from exceeding the Maximum
 * number of connections. Server thread sleeps until another thread signals it
 */
ThreadInfo *getThreadInfo() {
	// Lock the connections list
	pthread_mutex_lock(&connectionsMutex);

	// Wait on the condition if there are no available connections
	while (connections->count >= MAX_CONNECTIONS) {
		pthread_cond_wait(&connectionsCond, &connectionsMutex);
	}

	// Lock is available, and threads available here. Take the current and 
	// update the circular buffer
	ThreadInfo *currInfo = (ThreadInfo *)calloc(1, sizeof(ThreadInfo));
	//currInfo->sessionIDs = (char **)calloc(MAX_SIMUL_SESSIONS_PER_CLIENT, sizeof(char *));
	ll_insert(connections, (void *)currInfo);

	// Release the mutex lock again so others can enter this critical section
	pthread_mutex_unlock(&connectionsMutex);

	return currInfo;
}

void releaseThread(ThreadInfo *thread) {
	// Lock the thread for the critical section
	pthread_mutex_lock(&connectionsMutex);
	
	// Find the current connection in the linked list, remove it
	Node *elem = ll_find(connections, thread, &threadInfoComparer);
	ll_remove(connections, elem);
	// Finally free the data
	free(elem->data);
	free(elem);

	// Signal to other threads
	pthread_cond_signal(&connectionsCond);

	// Release the lock
	pthread_mutex_unlock(&connectionsMutex);
}
