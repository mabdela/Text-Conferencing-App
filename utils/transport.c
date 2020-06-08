//
// Transport format library implementation


#include "transport.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/** 
 * @brief Converts a Packet into a serialized byte array for transport
 *
 * @param packet Packet to Convert
 * @param len Returns the length of the byte array created
 */
unsigned char *
packetToByteArray(Packet *packet, int *len)
{
	char sprintfBuf[128];

	// sprintf always adds a null, causing size issues, but it returns the
	// actual number of bytes written. Write to a temporary buffer. 
	int chars = sprintf(sprintfBuf, "%d:%d:%s:", packet->type, packet->size,
						(char *)packet->source);
	unsigned char *buf = (unsigned char *)calloc(packet->size, sizeof(unsigned char) + chars);
	memcpy(buf, sprintfBuf, chars);
	memcpy(buf + chars, packet->data, packet->size);

	*len = chars + packet->size;

	return buf;
}

/**
 * @brief Convert a serialized packet back into a Packet
 *
 * @param string Byte array of the packet
 * @param packetLength Length of the byte array
 */
Packet *
bytesToPacket(unsigned char *string, int packetLength)
{
	Packet *packet = (Packet *)calloc(1, sizeof(Packet));
	char content[MAX_DATA];
	char clientId[MAX_NAME];
	int colon_counter = 0, i = 0, content_start = 0, content_size = 0;

	if (packetLength == 0) {
		return packet;
	}

	while (1) {
		if (string[i] == ':') {
			colon_counter++;
		}
		i++;

		if (colon_counter == 3) {
			content_start = i;
			content_size = packetLength - i;
			memcpy(content, string + i, content_size);
			break;
		}
	}
		
	// Parse the header
	int type, size;
	// Create a temporary header
	char *header = (char *)calloc(content_start+1, sizeof(char));
	memcpy(header, string, content_start - 1);
	header[content_start] = '\0';

	// Parse out the type, size, and clientId strings	
	sscanf(header, "%d:%d:%s:%*s", &type, &size, clientId);
	int clientIdLen = strlen(clientId);

	// Free allocated header
	free(header);

	// Build the packet
	packet->type = type;
	packet->size = size;
	memcpy(packet->source, clientId, clientIdLen);
	memcpy(packet->data, content, size);

	return packet;
}

/**
 * @brief Helper to create a login packet
 *
 * @param clientID ClientID string
 * @param password Password string
 * @returns Formatted login packet
 */
Packet *
getLoginPacket(char *clientID, char *password)
{
	Packet *packet = (Packet *)calloc(1, sizeof(Packet));
	char buf[MAX_NAME+MAX_DATA+2];
	int bytes = sprintf(buf, "%s,%s", clientID, password);
	int clientIdLen = strlen(clientID);

	packet->type = LOGIN;
	packet->size = bytes;
	memcpy(packet->source, clientID, clientIdLen);
	memcpy(packet->data, buf, bytes);

	return packet;
}

/**
 * @brief Helper to create a logout packet
 *
 * @param clientID ClientID string
 * @returns Formatted logout packet
 */
Packet *
getLogoutPacket(char *clientID)
{
	Packet *packet = (Packet *)calloc(1, sizeof(Packet));
	int clientIdLen = strlen(clientID);

	packet->type = EXIT;
	packet->size = 0;
	memcpy(packet->source, clientID, clientIdLen);
	memset(packet->data, 0, MAX_DATA);

	return packet;
}

/**
 * @brief Helper to create a query packet
 *
 * @param clientID ClientID string
 * @returns Formatted query packet
 */
Packet *
getListPacket(char *clientID)
{
	Packet *packet = (Packet *)calloc(1, sizeof(Packet));
	char buf[MAX_NAME+MAX_DATA+2];
	int bytes = sprintf(buf, "%s", clientID);
	int clientIdLen = strlen(clientID);

	packet->type = QUERY;
	packet->size = bytes;
	memcpy(packet->source, clientID, clientIdLen);
	memcpy(packet->data, buf, bytes);

	return packet;
}

/**
 * @brief Helper to create a message packet
 *
 * @param clientID ClientID string
 * @param contents Message contents
 * @returns Formatted query packet
 */
Packet *
getMessagePacket(char *clientID, char *sessionName, char *contents)
{
	Packet *packet = (Packet *)calloc(1, sizeof(Packet));

	int size = strlen(contents) + 1 + strlen(sessionName);
	char *buf = (char *)calloc(size, sizeof(char));

	sprintf(buf, "%s;%s", sessionName, contents);

	packet->type = MESSAGE;
	packet->size = size;
	memcpy(packet->source, clientID, strlen(clientID));
	memcpy(packet->data, buf, packet->size);

	free(buf);

	return packet;
}

/**
 * @brief Helper to create a new session packet
 *
 * @param clientID ClientID string
 * @param sessionID SessionID string
 * @returns Formatted query packet
 */
Packet *
getNewSessionPacket(char *clientID, char *sessionID) 
{
	Packet *packet = (Packet *)calloc(1, sizeof(Packet));

	packet->type = NEW_SESS;
	packet->size = strlen(sessionID);
	memcpy(packet->source, clientID, strlen(clientID));
	memcpy(packet->data, sessionID, strlen(sessionID));

	return packet;
}

/**
 * @brief Helper to create a join session packet
 *
 * @param clientID ClientID string
 * @param sessionID SessionID string
 * @returns Formatted query packet
 */
Packet *
getJoinSessionPacket(char *clientID, char *sessionID) 
{
    Packet *packet = (Packet *)calloc(1, sizeof(Packet));

    packet->type = JOIN;
    packet->size = strlen(sessionID);
    memcpy(packet->source, clientID, strlen(clientID));
    memcpy(packet->data, sessionID, strlen(sessionID));

	return packet;
}

/**
 * @brief Helper to create a leave session packet
 *
 * @param clientID ClientID string
 * @param sessionID Session to leave
 * @returns Formatted query packet
 */
Packet *
getLeaveSessionPacket(char *clientID, char *sessionID) 
{
	Packet *packet = (Packet *)calloc(1, sizeof(Packet));
	int len = strlen(sessionID);

	packet->type = LEAVE_SESS;
	packet->size = len;
	memcpy(packet->source, clientID, strlen(clientID));
	memcpy(packet->data, sessionID, len);	
	
	return packet;
}