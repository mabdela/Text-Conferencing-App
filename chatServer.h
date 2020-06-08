//
// Chat Server Interface header


#pragma once
#ifndef CHATSERVER_H_
#define CHATSERVER_H_

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

#define MAX_SIMUL_SESSIONS_PER_CLIENT 4

/**
 * @brief Data structure for storing relevant per-thread data
 */
typedef struct _ThreadInfo
{
	struct sockaddr_in clientAddr;
	socklen_t clientAddrLen;
	int socket;
	char clientID[MAX_NAME];
	char *sessionID;
	pthread_t thread;
	pthread_mutex_t socketLock;
	int clientConnected;
	LinkedList *connections;
	HashTable *sessions;
	HashTable *users;
} ThreadInfo;

/**
 * @brief Function for comparing two ThreadInfo elements
 *
 * @param t1 First element, should be a ThreadInfo
 * @param t2 Second element, should be a ThreadInfo
 * @returns 0 if they're equal, otherwise 1 or -1.
 */
int threadInfoComparer(void *t1, void *t2);

/**
 * @brief Checks the request packet for a valid login request, sets the threadInfo to logged in if successful
 *
 * @params threadInfo ThreadInfo struct
 * @params requestPacket Client request packet
 * @returns ResponsePacket to send to client
 */
Packet *chatServer_login(ThreadInfo *threadInfo, Packet *requestPacket);

/** 
 * @brief Closes the connection to the client and removes them from all connected sessions
 *
 * @params threadInfo ThreadInfo struct
 * @params requestPacket Client request packet
 * @returns ResponsePacket to send to client
 */
Packet *chatServer_exit(ThreadInfo *threadInfo, Packet *requestPacket);

/**
 * @brief Joins the client to the specified session
 *
 * @params threadInfo ThreadInfo struct
 * @params requestPacket Client request packet
 * @returns ResponsePacket to send to client
 */
Packet *chatServer_sessionJoin(ThreadInfo *threadInfo, Packet *requestPacket);

/**
 * @brief Removes the client from the specified session
 *
 * @params threadInfo ThreadInfo struct
 * @params requestPacket Client request packet
 * @returns ResponsePacket to send to client
 */
Packet *chatServer_sessionLeave(ThreadInfo *threadInfo, Packet *requestPacket);

/** 
 * @brief Creates a session for the client and joins them
 *
 * @params threadInfo ThreadInfo struct
 * @params requestPacket Client request packet
 * @returns ResponsePacket to send to client
 */
Packet *chatServer_sessionCreate(ThreadInfo *threadInfo, Packet *requestPacket);

/**
 * @brief Finds all the current sessions and returns it to the client
 *
 * @params threadInfo ThreadInfo struct
 * @params requestPacket Client request packet
 * @returns ResponsePacket to send to client
 */
Packet *chatServer_sessionQuery(ThreadInfo *threadInfo, Packet *requestPacket);

/**
 * @brief Broadcasts the client's message to the session
 *
 * @params threadInfo ThreadInfo struct
 * @params requestPacket Client request packet
 * @params buf Original byte buffer for the packet
 * @params bytes Size of the byte buffer
 * @returns ResponsePacket to send to client
 */
Packet *chatServer_message(ThreadInfo *threadInfo, Packet *requestPacket, unsigned char *buf, int bytes);

#endif
