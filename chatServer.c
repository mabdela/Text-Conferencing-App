//
// Chat Server Interface implementation


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

/**
 * @brief Function for comparing two ThreadInfo elements
 *
 * @param t1 First element, should be a ThreadInfo
 * @param t2 Second element, should be a ThreadInfo
 * @returns 0 if they're equal, otherwise 1 or -1.
 */
int threadInfoComparer(void *t1, void *t2) {
    ThreadInfo *ti1 = (ThreadInfo *)t1;
    ThreadInfo *ti2 = (ThreadInfo *)t2;

    return (ti1->socket > ti2->socket) - (ti1->socket < ti2->socket);

}

/**
 * @brief Function for comparing two strings
 *
 * @param s1 First element, should be a char *
 * @param s2 Second element, should be a char *
 * @returns 0 if they're equal, otherwise 1 or -1
 */
int stringComparer(void *s1, void *s2) {
    return strcmp((char *)s1, (char *)s2);
}

const char *notAuthenticatedError = "Not logged in.";

/**
 * @brief Checks the request packet for a valid login request, sets the threadInfo to logged in if successful
 *
 * @params threadInfo ThreadInfo struct
 * @params requestPacket Client request packet
 * @returns ResponsePacket to send to client
 */
Packet *chatServer_login(ThreadInfo *threadInfo, Packet *requestPacket) {
	Packet *responsePacket = (Packet *)calloc(1, sizeof(Packet));

	//Create local string
	char *buf = (char *)calloc(requestPacket->size + 1, sizeof(char));
	memcpy(buf, requestPacket->data, requestPacket->size);
	buf[requestPacket->size] = '\0';

	int parts;
	char **tokens = parseTokens(buf, ",", &parts);

	char *password = (char *)ht_find(threadInfo->users, tokens[0]);

	// Compare their password with the passwords file
	if(password != NULL && strcmp(password, tokens[1]) == 0) {
		// Verify current user isn't logged in already
		if (ll_find(threadInfo->connections, tokens[0], &stringComparer) == NULL) {
			responsePacket->type = LO_ACK;
			memcpy(threadInfo->clientID, requestPacket->source, MAX_NAME);
			memcpy(responsePacket->data, requestPacket->source, MAX_NAME);
			responsePacket->size = MAX_NAME;
			ll_insert(threadInfo->connections, buf);
		}
		else {
		    responsePacket->type = LO_NAK;
		    responsePacket->size = 0;
			free(buf);
		}
	}
	else {
		responsePacket->type = LO_NAK;
		responsePacket->size = 0;
		free(buf);
	}

	free(tokens);

	return responsePacket;
}

/** 
 * @brief Closes the connection to the client and removes them from all connected sessions
 *
 * @params threadInfo ThreadInfo struct
 * @params requestPacket Client request packet
 * @returns ResponsePacket to send to client
 */
Packet *chatServer_exit(ThreadInfo *threadInfo, Packet *requestPacket) {
    threadInfo->clientConnected = 0;

    HashEntry *entry = threadInfo->sessions->head;
	while(entry != NULL) {
	    LinkedList *session = entry->data;
	    Node *client = ll_find(session, threadInfo, &threadInfoComparer);
		if (client != NULL) {
		    ll_remove(session, client);
		    free(client);

			if(session->count == 0) {
			    ht_remove(threadInfo->sessions, entry->key);
			}
		}
		entry = entry->next;
	}

    Node *client = ll_find(threadInfo->connections, threadInfo->clientID, &stringComparer);
    ll_remove(threadInfo->connections, client);

    return NULL;
}

/**
 * @brief Joins the client to the specified session
 *
 * @params threadInfo ThreadInfo struct
 * @params requestPacket Client request packet
 * @returns ResponsePacket to send to client
 */
Packet *chatServer_sessionJoin(ThreadInfo *threadInfo, Packet *requestPacket) {
	Packet *responsePacket = (Packet *)calloc(1, sizeof(Packet));

	// Ensure that the clientID is set for this request (logged in)
	if(memcmp(threadInfo->clientID, requestPacket->source, MAX_NAME) != 0) {
		responsePacket->type = NS_NAK;
		memcpy(responsePacket->data, notAuthenticatedError, strlen(notAuthenticatedError));
		responsePacket->size = strlen(notAuthenticatedError);
	}
	else {
		char* sessionName = (char *)calloc(requestPacket->size + 1, sizeof(char));
		memcpy(sessionName, requestPacket->data, requestPacket->size);
		sessionName[requestPacket->size] = '\0';
		// Check whether or not the session actually exists
		LinkedList *session = (LinkedList *)ht_find(threadInfo->sessions, sessionName);
		if (session == NULL) {
			responsePacket->type = JN_NAK;
			const char *sessNonexistent = "Session does not exist.";
			memcpy(responsePacket->data, sessNonexistent, strlen(sessNonexistent));
			responsePacket->size = strlen(sessNonexistent);
		}
		else {
			// Join to session
			threadInfo->sessionID = sessionName;
			ll_insert(session, (void *)threadInfo);

			responsePacket->type = JN_ACK;
			memcpy(responsePacket->data, sessionName, strlen(sessionName));
			responsePacket->size = strlen(sessionName);
			printf("Client at socket %d joined session %s\n", threadInfo->socket, sessionName);
			fflush(stdout);
		}
	}

	return responsePacket;
}

/**
 * @brief Removes the client from the specified session
 *
 * @params threadInfo ThreadInfo struct
 * @params requestPacket Client request packet
 * @returns ResponsePacket to send to client
 */
Packet *chatServer_sessionLeave(ThreadInfo *threadInfo, Packet *requestPacket) {
	Packet *responsePacket = (Packet *)calloc(1, sizeof(Packet));

	// Ensure that the clientID is set for this request (logged in)
	if(memcmp(threadInfo->clientID, requestPacket->source, MAX_NAME) != 0) {
		responsePacket->type = NS_NAK;
		memcpy(responsePacket->data, notAuthenticatedError, strlen(notAuthenticatedError));
		responsePacket->size = strlen(notAuthenticatedError);
	}
	else {
		char* sessionName = (char *)calloc(requestPacket->size + 1, sizeof(char));
		memcpy(sessionName, requestPacket->data, requestPacket->size);
		sessionName[requestPacket->size] = '\0';

		//Check if the session exists
		if (ht_find(threadInfo->sessions, sessionName) == NULL) {
		    responsePacket->type = LS_NACK;
		    const char *sessNoExist = "Session does not exist.";
		    memcpy(responsePacket->data, sessNoExist, strlen(sessNoExist));
		    responsePacket->size = strlen(sessNoExist);
		}
		else {
		    LinkedList *currSession = (LinkedList *)ht_find(threadInfo->sessions, threadInfo->sessionID);
			Node *curr = ll_find(currSession, threadInfo, &threadInfoComparer);
			// TODO: Necessary to check curr == NULL?
			ll_remove(currSession, curr);
			free(curr);
			free(threadInfo->sessionID);
			threadInfo->sessionID = NULL;
			printf("Left. %d remaining.\n", currSession->count);
			//Delete if empty
			if (currSession->count == 0) {
				ht_remove(threadInfo->sessions, threadInfo->sessionID);
				free(currSession);
			}
			responsePacket->type = LS_ACK;
			responsePacket->size = 0;
		}

		free(sessionName);
	}

	return responsePacket;
}

/** 
 * @brief Creates a session for the client and joins them
 *
 * @params threadInfo ThreadInfo struct
 * @params requestPacket Client request packet
 * @returns ResponsePacket to send to client
 */
Packet *chatServer_sessionCreate(ThreadInfo *threadInfo, Packet *requestPacket) {
	Packet *responsePacket = (Packet *)calloc(1, sizeof(Packet));

	// Ensure that the clientID is set for this request (logged in)
	if(memcmp(threadInfo->clientID, requestPacket->source, MAX_NAME) != 0) {
		responsePacket->type = NS_NAK;
		memcpy(responsePacket->data, notAuthenticatedError, strlen(notAuthenticatedError));
		responsePacket->size = strlen(notAuthenticatedError);
	}
	else {
		char* sessionName = (char *)calloc(requestPacket->size + 1, sizeof(char));
		memcpy(sessionName, requestPacket->data, requestPacket->size);
		sessionName[requestPacket->size] = '\0';

		// Check if there's another session with the same name
		if (ht_find(threadInfo->sessions, sessionName) != NULL) {
			responsePacket->type = NS_NAK;
			const char *sessExists = "Session already exists.";
			memcpy(responsePacket->data, sessExists, strlen(sessExists));
			responsePacket->size = strlen(sessExists);
		}
		else {
			// Create the session
			LinkedList *connectedClients = ll_init();
			ht_insert(threadInfo->sessions, sessionName, (void *)connectedClients);

			// Join to session
			threadInfo->sessionID = sessionName;
			ll_insert(connectedClients, (void *)threadInfo);
						
			responsePacket->type = NS_ACK;
			memcpy(responsePacket->data, sessionName, requestPacket->size);
			responsePacket->size = requestPacket->size;

			printf("Created session %s from client at sock %d\n", sessionName, threadInfo->socket);
		}
	}
	return responsePacket;
}

/**
 * @brief Finds all the current sessions and returns it to the client
 *
 * @params threadInfo ThreadInfo struct
 * @params requestPacket Client request packet
 * @returns ResponsePacket to send to client
 */
Packet *chatServer_sessionQuery(ThreadInfo *threadInfo, Packet *requestPacket) {
	Packet *responsePacket = (Packet *)calloc(1, sizeof(Packet));

	if (memcmp(threadInfo->clientID, requestPacket->source, MAX_NAME) != 0) {
	    responsePacket->type = QU_NACK;
		memcpy(responsePacket->data, notAuthenticatedError, strlen(notAuthenticatedError));
		responsePacket->size = strlen(notAuthenticatedError);
	}
	else {
		responsePacket->type = QU_ACK;
	    HashEntry *node = threadInfo->sessions->head;

	    const char *sessFormat = "'%s': %d users\n";
	    int len = 0;
		char buf[2048]; // TODO: Possible buffer overflow possible here
	    unsigned char *ptr = responsePacket->data;
		// Traverse the HashTable's linked list
	    while (node != NULL)
	    {
			// Traverse the session's linked list
			LinkedList *session = (LinkedList *)node->data;
			int bytes = sprintf(buf, sessFormat, node->key, session->count);

			Node *sessNode = session->head;
			while(sessNode != NULL) {
			    ThreadInfo *sessTi = (ThreadInfo *)sessNode->data;
			    bytes += sprintf(buf + bytes, "\t%.64s\n", sessTi->clientID);
			    sessNode = sessNode->next;
			}

			memcpy(ptr, buf, bytes);
			ptr += bytes;
			len += bytes;

			node = node->next;
		}
		responsePacket->size = len;
	}

	return responsePacket;
}

/**
 * @brief Broadcasts the client's message to the session
 *
 * @params threadInfo ThreadInfo struct
 * @params requestPacket Client request packet
 * @params buf Original byte buffer for the packet
 * @params bytes Size of the byte buffer
 * @returns ResponsePacket to send to client
 */
Packet *chatServer_message(ThreadInfo *threadInfo, Packet *requestPacket, unsigned char *buf, int bytes) {
	Packet *responsePacket = (Packet *)calloc(1, sizeof(Packet));

	if (memcmp(threadInfo->clientID, requestPacket->source, MAX_NAME) != 0) {
	    responsePacket->type = MESSAGE_NCK;
		memcpy(responsePacket->data, notAuthenticatedError, strlen(notAuthenticatedError));
		responsePacket->size = strlen(notAuthenticatedError);
	}
	else {
		char *string = (char *)calloc(requestPacket->size + 1, sizeof(char));
		memcpy(string, requestPacket->data, requestPacket->size);
		string[requestPacket->size] = '\0';

		// Parse out the session from the first part
		char buf2[256]; // TODO: Possible buffer overflow for large session names
		int i;
		for (i = 0; string[i] != ';' && string[i] != '\0'; i++) {
			buf2[i] = string[i];
		}
		buf2[i] = '\0';
		
		// Find the session for the sessionID
		LinkedList *session = ht_find(threadInfo->sessions, buf2);
		if (session == NULL || ll_find(session, threadInfo, NULL) == NULL) {
		    responsePacket->type = MESSAGE_NCK;
		    const char *notInSession = "Cannot send message, not in session";
		    memcpy(responsePacket->data, notInSession, strlen(notInSession));
		    responsePacket->size = strlen(notInSession);
		}
		else {
		    // Traverse the list, for each one forward the message
			// to the socket on the other side
			printf("Client %.64s sending size %d, \"%.*s\"\n", requestPacket->source, requestPacket->size, requestPacket->size, requestPacket->data);
		    fflush(stdout);
		    Node *curr = session->head;
		    while (curr != NULL)
		    {
				ThreadInfo *ti = (ThreadInfo *)curr->data;
				
				// Avoid sending the message back to self, will cause issues with the expected response
				if (ti->socket != threadInfo->socket)
				{
					printf("Sending %.*s to socket %d\n", bytes-i-1, string+i, ti->socket);
					pthread_mutex_lock(&ti->socketLock);
					send(ti->socket, buf, bytes, 0);
					pthread_mutex_unlock(&ti->socketLock);
				}
				curr = curr->next;
			}
			responsePacket->type = MESSAGE_ACK;
			responsePacket->size = 0;
		}
		free(string);
	}
	return responsePacket;
}
