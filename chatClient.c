//
// ChatClient library implementation
// 

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "utils/nethelper.h"
#include "chatClient.h"
#include "utils/transport.h"
#include "utils/printHelpers.h"

#define TIMEOUT_RTT_MULT 3

typedef struct _ThreadSessionInfo {
    SessionInfo *sessionInfo;
    int listeningSession;
} ThreadSessionInfo;

/**
 * @brief Initializes an instance of a SessionInfo
 */
SessionInfo *chatclient_init()
{
	SessionInfo* sess =  (SessionInfo *)calloc(1, sizeof(SessionInfo));
	pthread_mutex_init(&sess->socketLock, NULL);

	sess->currSessionID = (char **)calloc(MAX_SIMUL_SESSIONS, sizeof(char *));
	return sess;
}

/**
 * @brief Listening thread function
 */
void *chatClient_listenThread(void *args)
{
    ThreadSessionInfo *threadSess = (ThreadSessionInfo *)args;
    SessionInfo *sess = threadSess->sessionInfo;

	while (sess->threadRun) {
		unsigned char buf[MAX_PACKET_SIZE];
		memset(buf, 0, MAX_PACKET_SIZE);
		
		// Lock before reading
		pthread_mutex_lock(&sess->socketLock);
		int bytes = recv(sess->socket, buf, MAX_PACKET_SIZE, 0);

		if (bytes == -1) {
			// No data received before timeout, release thread
			pthread_mutex_unlock(&sess->socketLock);
			usleep(1000);
			continue;
		}
		else if (bytes == 0) {
		    sess->threadRun = 0;
		    continue;
		}

		Packet *message = bytesToPacket(buf, bytes);

		char *buf2 = (char *)calloc(message->size + 1, sizeof(char));
		memcpy(buf2, message->data, message->size);
		buf2[message->size] = '\0';

		int i, j, k = 0;
		char buf3[256];
		for(i = 0; i < message->size && buf2[i] != ';'; i++) {
		    buf3[i] = buf2[i];
		}
		buf3[i] = '\0';

		for (j = 0; j < MAX_SIMUL_SESSIONS; j++) {
			if(sess->currSessionID[j] != NULL) {
				if(strcmp(buf3, sess->currSessionID[j]) == 0) {
				    k = j;
				}
			}
		}

		printf("\rSession %.64s: %.64s: %.*s\n\rTab %d '%.64s'> ", sess->currSessionID[k],
			   message->source, message->size - i - 2, message->data + i + 1, sess->currSession + 1, sess->currSessionID[sess->currSession]);
		fflush(stdout);
		free(message);
		free(buf2);

		pthread_mutex_unlock(&sess->socketLock);
	}

	free(args);
	return NULL;
}

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
					 char *serverIP, char *serverPort)
{
	// Check that the socket is initialized
	if (sess->socket <= 0) {
		sess->socket = getClientSocket(serverIP, serverPort);
	}

	// Lock the socket
	pthread_mutex_lock(&sess->socketLock);

	// Get a login packet
	Packet *loginPacket = getLoginPacket(clientId, password);

	// Convert it to a string
	int messageLen;
	unsigned char *message = packetToByteArray(loginPacket, &messageLen);

	// RTT clock
	struct timespec start, end;
	clock_gettime(CLOCK_REALTIME, &start);
	send(sess->socket, message, messageLen, 0);

	// Receive a response from the server
	unsigned char buf[MAX_PACKET_SIZE];
	int received = recv(sess->socket, buf, MAX_PACKET_SIZE, 0);
	// Stop RTT
	clock_gettime(CLOCK_REALTIME, &end);

	sess->waitPeriod.tv_sec = end.tv_sec - start.tv_sec;
	sess->waitPeriod.tv_usec = TIMEOUT_RTT_MULT *
					 ((end.tv_nsec - start.tv_nsec) / 1000);

	if (sess->waitPeriod.tv_usec == 0) sess->waitPeriod.tv_usec = 2500;

	// Set the socket timeout to a multiple of the RTT
	if (setsockopt(sess->socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&sess->waitPeriod, sizeof(struct timeval)) < 0)
	{
		printLastError("Error at setsockopt(): %s\n");
		return -1;
	}
	if (setsockopt(sess->socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&sess->waitPeriod, sizeof(struct timeval)) < 0)
	{
		printLastError("Error at setsockopt(): %s\n");
		return -1;
	} 

	// Free allocated packets and string!
	free(loginPacket);
	free(message);

	if (received < 0) {
	    fprintf(stderr, "No data received.\n");
	    pthread_mutex_unlock(&sess->socketLock);
	    return -1;
	}
	// Convert the string response back into a packet format
	Packet *responsePacket = bytesToPacket(buf, received);

	int returnVal;
	if (responsePacket->type == LO_ACK) {
		memcpy(sess->clientID, responsePacket->data, responsePacket->size);
		sess->currSessionID[sess->currSession] = NULL;
	    returnVal = 0;

		sess->threadRun = 1;
		ThreadSessionInfo *ti = (ThreadSessionInfo *)calloc(1, sizeof(ThreadSessionInfo));
		ti->sessionInfo = sess;
		ti->listeningSession = sess->currSession;
		pthread_create(&sess->listeningThread, NULL, chatClient_listenThread,
				ti);
		pthread_detach(sess->listeningThread);
	} else {
		printf("Login error: %.*s", responsePacket->size, responsePacket->data);
		//TODO: Handle error messages
		returnVal = -1;
	}

	free(responsePacket);
	// Unlock socket and return
	pthread_mutex_unlock(&sess->socketLock);
	return returnVal;
}

/**
 * @brief Logs the client out of the current server
 * 
 * @param sess SessionInfo struct
 * @returns 0 if successful, -1 otherwise
 */
int chatclient_logout(SessionInfo *sess)
{
	if (sess->socket <= 0) {
		// Socket not initialized, nothing to do
		return -1;
	}
	if (strcmp(sess->clientID, "") == 0) {
		// No session value, nothing to do
		return -1;
	}
	// Lock the socket
	pthread_mutex_lock(&sess->socketLock);

	// Get a logout packet
	Packet *logoutPacket = getLogoutPacket(sess->clientID);

	// Convert it to a string
	int messageLen;
	unsigned char *message = packetToByteArray(logoutPacket, &messageLen);

	send(sess->socket, message, messageLen, 0);

	// Free allocated packets and string!
	free(logoutPacket);
	free(message);

	sess->threadRun = 0;

	chatclient_finish(sess);
	// Unlock the socket
	pthread_mutex_unlock(&sess->socketLock);
	return 0;
}

/**
 * @brief Joins the client into the specified session
 * 
 * @param sess SessionInfo struct
 * @param sessionID Session ID to join
 * @returns 0 if successful, -1 otherwise
 */
int chatclient_joinSession(SessionInfo *sess, char *sessionID)
{
	if (sess->socket <= 0) {
		// Socket not initialized, nothing to do
		return -1;
	}
	if (strcmp(sess->clientID, "") == 0) {
		// No session value, nothing to do
		return -1;
	}
	// Lock the socket
	pthread_mutex_lock(&sess->socketLock);

	Packet *joinSessionPacket = getJoinSessionPacket(sess->clientID, sessionID);

	// Convert it to a string
	int messageLen;
	unsigned char *message = packetToByteArray(joinSessionPacket, &messageLen);

	send(sess->socket, message, messageLen, 0);

	// Free allocated packets and strings
	free(joinSessionPacket);
	free(message);

	// Receive a response from the server
	unsigned char buf[MAX_PACKET_SIZE];
	int received = recv(sess->socket, buf, MAX_PACKET_SIZE, 0);

	if (received < 0) {
	    fprintf(stderr, "No data received.\n");
	    pthread_mutex_unlock(&sess->socketLock);
	    return -1;
	}

	// Convert the response back into a packet
	Packet *responsePacket = bytesToPacket(buf, received);

	int returnVal;
	if (responsePacket->type == JN_ACK) {
		if(sess->currSessionID[sess->currSession] != NULL) {
		    free(sess->currSessionID[sess->currSession]);
		}
		sess->currSessionID[sess->currSession] = (char *)calloc(responsePacket->size, sizeof(char));
		memcpy(sess->currSessionID[sess->currSession], responsePacket->data, responsePacket->size);
		printf("Joined session: %s\n", sess->currSessionID[sess->currSession]);
		returnVal = 0;
	} 
	else {
		printf("Join session error: %.*s", responsePacket->size, 
			   responsePacket->data);
		returnVal = -1;
	}

	free(responsePacket);
	pthread_mutex_unlock(&sess->socketLock);
	return returnVal;
}

/**
 * @brief Removes the client from the current session
 * 
 * @param sess SessionInfo struct
 * @returns 0 if successful, -1 otherwise
 */
int chatclient_leaveSession(SessionInfo *sess)
{
	if (sess->socket <= 0) {
		// Socket not initialized, nothing to do
		return -1;
	}
	if (strcmp(sess->clientID, "") == 0) {
		// No session value, nothing to do
		return -1;
	}
	// Lock network socket before processing
	pthread_mutex_lock(&sess->socketLock);

	Packet *leaveSessPacket = getLeaveSessionPacket(sess->clientID, sess->currSessionID[sess->currSession]);

	int messageLen;
	unsigned char *message = packetToByteArray(leaveSessPacket, &messageLen);

	send(sess->socket, message, messageLen, 0);

	free(leaveSessPacket);
	free(message);

	// Receive a response from the server
	unsigned char buf[MAX_PACKET_SIZE];
	int received = recv(sess->socket, buf, MAX_PACKET_SIZE, 0);

	if (received < 0) {
	    fprintf(stderr, "No data received.\n");
		pthread_mutex_unlock(&sess->socketLock);
	    return -1;
	}

	// Convert the string response back into a packet format
	Packet *responsePacket = bytesToPacket(buf, received);

	int returnVal;
	if(responsePacket->type == LS_ACK) {
		free(sess->currSessionID[sess->currSession]);
		sess->currSessionID[sess->currSession] = NULL;
		returnVal = 0;
	}
	else {
		fprintf(stderr, "Error leaving session: %.*s\n", responsePacket->size, responsePacket->data);
		returnVal = -1;
	}

	free(responsePacket);
	pthread_mutex_unlock(&sess->socketLock);
	return returnVal;
}

/**
 * @brief Creates as session with the specified name
 * 
 * @param sess SessionInfo struct
 * @param sessionID Session ID to create
 * @returns 0 if successful, -1 otherwise
 */
int chatclient_createSession(SessionInfo *sess, char *sessionID)
{
	if (sess->socket <= 0) {
		// Socket not initialized, nothing to do
		return -1;
	}
	if (strcmp(sess->clientID, "") == 0) {
		// No session value, nothing to do
		return -1;
	}
	if (sess->currSessionID[sess->currSession] != NULL) {
	    // User already in session
		return -1;
	}
	// Lock socket before processing
	pthread_mutex_lock(&sess->socketLock);

	Packet *newSessPacket = getNewSessionPacket(sess->clientID, sessionID);

	int messageLen;
	unsigned char *message = packetToByteArray(newSessPacket, &messageLen);

	send(sess->socket, message, messageLen, 0);

	free(newSessPacket);
	free(message);

	// Receive a response from the server
	unsigned char buf[MAX_PACKET_SIZE];
	int received = recv(sess->socket, buf, MAX_PACKET_SIZE, 0);

	if (received < 0) {
	    fprintf(stderr, "No data received.\n");
		pthread_mutex_unlock(&sess->socketLock);
	    return -1;
	}
	// Convert the string response back into a packet format
	Packet *responsePacket = bytesToPacket(buf, received);

	int returnVal;
	if (responsePacket->type == NS_ACK) {
		if (sess->currSessionID[sess->currSession] != NULL) {
			free(sess->currSessionID[sess->currSession]);
		}
		sess->currSessionID[sess->currSession] = (char *)calloc(responsePacket->size, sizeof(char));
		memcpy(sess->currSessionID[sess->currSession], responsePacket->data, responsePacket->size);
		printf("Session created: %s\n", sess->currSessionID[sess->currSession]);
	    returnVal = 0;
	} else {
		printf("Create session error: %.*s\n", responsePacket->size, 
				responsePacket->data);
		//TODO: Handle error messages
		returnVal = -1;
	}

	free(responsePacket);
	pthread_mutex_unlock(&sess->socketLock);
	return returnVal;
}

/**
 * @brief Lists the sessions on the server and the users
 * 
 * @param sess SessionInfo struct
 * @returns 0 if successful, -1 otherwise
 */
int chatclient_list(SessionInfo *sess)
{
	if (sess->socket <= 0) {
		// Socket not initialized, nothing to do
		return -1;
	}
	if (strcmp(sess->clientID, "") == 0) {
		//No session value, nothing to do
		return -1;
	}
	// Lock network socket
	pthread_mutex_lock(&sess->socketLock);

	Packet *queryPacket = getListPacket(sess->clientID);

	int messageLen;
	unsigned char *ret = packetToByteArray(queryPacket, &messageLen);

	send(sess->socket, ret, messageLen, 0);

	free(queryPacket);
	free(ret);

	//Receive response
	unsigned char buf[MAX_PACKET_SIZE];
	int received = recv(sess->socket, buf, MAX_PACKET_SIZE, 0);

	if (received < 0) {
		fprintf(stderr, "No data received.\n");
		pthread_mutex_unlock(&sess->socketLock);
		return -1;
	}
	// Convert back to packet
	Packet *responsePacket = bytesToPacket(buf, received);
	int returnVal;
	if(responsePacket->type != QU_ACK) {
		printf("Error listing sessions: %.*s\n", responsePacket->size, responsePacket->data);
		returnVal = -1;
	}
	else {
		printf("%.*s\n", responsePacket->size, responsePacket->data);
		returnVal = 0;
	}

	free(responsePacket);
	pthread_mutex_unlock(&sess->socketLock);
	return returnVal;
}

/**
 * @brief Sends a message to the server in the current session
 * 
 * @param sess SessionInfo struct
 * @param message Message to send
 * @returns 0 if successful, -1 otherwise
 */
int chatclient_sendMessage(SessionInfo *sess, char *message)
{
	if (sess->socket <= 0) {
		// Socket not initialized, nothing to do
		return -1;
	}
	if (strcmp(sess->clientID, "") == 0) {
		//No session value, nothing to do
		return -1;
	}
	// Lock network socket
	pthread_mutex_lock(&sess->socketLock);

	Packet *messagePacket = getMessagePacket(sess->clientID, sess->currSessionID[sess->currSession], message);

	int messageLen;
	unsigned char *ret = packetToByteArray(messagePacket, &messageLen);

	send(sess->socket, ret, messageLen, 0);

	free(messagePacket);
	free(ret);

	// Receive a response from the server
	unsigned char buf[MAX_PACKET_SIZE];
	int received = recv(sess->socket, buf, MAX_PACKET_SIZE, 0);

	if (received < 0) {
	    fprintf(stderr, "No data received.\n");
		pthread_mutex_unlock(&sess->socketLock);
	    return -1;
	}
	// Convert the string response back into a packet format
	Packet *responsePacket = bytesToPacket(buf, received);

	int returnVal;
	if (responsePacket->type != MESSAGE_ACK) {
		printf("Error sending message: %.*s", responsePacket->size, responsePacket->data);
		//TODO: Handle error messages
		returnVal = -1;
	} 
	else {
	    returnVal = 0;
	}

	free(responsePacket);
	pthread_mutex_unlock(&sess->socketLock);
	return returnVal;
}

/**
 * @brief Cleans up the session
 * 
 * @param sess SessionInfo struct
 */
void chatclient_finish(SessionInfo *session)
{
	if(session->socket > 0) {
		close(session->socket);
	}
}
