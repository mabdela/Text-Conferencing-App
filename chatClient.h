//
// ChatClient library header

#pragma once
#ifndef CHATCLIENT_H_
#define CHATCLIENT_H_

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include "utils/transport.h"

#define MAX_SIMUL_SESSIONS 4

typedef struct _SessionInfo
{
	int socket;
	char clientID[MAX_NAME];
	pthread_mutex_t socketLock;
	int threadRun;
	char **currSessionID;
	pthread_t listeningThread;
	int currSession;
	struct timeval waitPeriod;
} SessionInfo;

/**
 * @brief Initializes an instance of a SessionInfo
 */
SessionInfo *chatclient_init();

/**
 * @brief Logs into the server with the given clientID, password, and host.
 * Initializes the socket as well and saves the info into sess
 * 
 * @params sess SessionInfo struct
 * @params clientId Client ID to login with
 * @params password Password to login with
 * @params serverIP Hostname or IP address of the server
 * @params serverPort Port of the server
 * @returns 0 if successful, -1 otherwise
 */
int chatclient_login(SessionInfo *sess, char *clientId, char *password,
					 char *serverIP, char *serverPort);

/**
 * @brief Logs the client out of the current server
 * 
 * @param sess SessionInfo struct
 * @returns 0 if successful, -1 otherwise
 */
int chatclient_logout(SessionInfo *sess);

/**
 * @brief Joins the client into the specified session
 * 
 * @param sess SessionInfo struct
 * @param sessionID Session ID to join
 * @returns 0 if successful, -1 otherwise
 */
int chatclient_joinSession(SessionInfo *sess, char *sessionID);

/**
 * @brief Removes the client from the current session
 * 
 * @param sess SessionInfo struct
 * @returns 0 if successful, -1 otherwise
 */
int chatclient_leaveSession(SessionInfo *sess);

/**
 * @brief Creates as session with the specified name
 * 
 * @param sess SessionInfo struct
 * @param sessionID Session ID to create
 * @returns 0 if successful, -1 otherwise
 */
int chatclient_createSession(SessionInfo *sess, char *sessionID);

/**
 * @brief Lists the sessions on the server and the users
 * 
 * @param sess SessionInfo struct
 * @returns 0 if successful, -1 otherwise
 */
int chatclient_list(SessionInfo *sess);

/**
 * @brief Sends a message to the server in the current session
 * 
 * @param sess SessionInfo struct
 * @param message Message to send
 * @returns 0 if successful, -1 otherwise
 */
int chatclient_sendMessage(SessionInfo *sess, char *message);

/**
 * @brief Cleans up the session
 * 
 * @param sess SessionInfo struct
 */
void chatclient_finish(SessionInfo *sess);

#endif