//
// Transport format header

#pragma once
#ifndef TRANSPORT_H_
#define TRANSPORT_H_

/* Configurable Packet Dimensions */
#define MAX_NAME 64
#define MAX_DATA 2048
#define MAX_PACKET_SIZE MAX_NAME+MAX_DATA+3*sizeof(unsigned int)

/* Packet Type Definitions */
#define LOGIN 1
#define LO_ACK 2
#define LO_NAK 3
#define EXIT 4
#define JOIN 5
#define JN_ACK 6
#define JN_NAK 7
#define LEAVE_SESS 8
#define LS_ACK 9
#define LS_NACK 10
#define NEW_SESS 11
#define NS_ACK 12
#define NS_NAK 13
#define MESSAGE 14
#define MESSAGE_ACK 15
#define MESSAGE_NCK 16
#define QUERY 17
#define QU_ACK 18
#define QU_NACK 19
#define UNKNOWN 20

/**
 * Transport packet, used to represent data sent via TCP
 */
typedef struct _Packet
{
	unsigned int type;
	unsigned int size;
	unsigned char source[MAX_NAME];
	unsigned char data[MAX_DATA];
} Packet;

/** 
 * @brief Converts a Packet into a serialized byte array for transport
 *
 * @param packet Packet to Convert
 * @param len Returns the length of the byte array created
 */
unsigned char *
packetToByteArray(Packet *packet, int* len);

/**
 * @brief Convert a serialized packet back into a Packet
 *
 * @param string Byte array of the packet
 * @param packetLength Length of the byte array
 */
Packet *
bytesToPacket(unsigned char *string, int packetLength);

/**
 * @brief Helper to create a login packet
 *
 * @param clientID ClientID string
 * @param password Password string
 * @returns Formatted login packet
 */
Packet *
getLoginPacket(char *clientID, char *password);

/**
 * @brief Helper to create a logout packet
 *
 * @param clientID ClientID string
 * @returns Formatted logout packet
 */
Packet *
getLogoutPacket(char *clientID);

/**
 * @brief Helper to create a query packet
 *
 * @param clientID ClientID string
 * @returns Formatted query packet
 */
Packet *
getListPacket(char *clientID);

/**
 * @brief Helper to create a message packet
 *
 * @param clientID ClientID string
 * @param contents Message contents
 * @returns Formatted query packet
 */
Packet *
getMessagePacket(char *clientID, char *sessionName, char *contents);

/**
 * @brief Helper to create a new session packet
 *
 * @param clientID ClientID string
 * @param sessionID SessionID string
 * @returns Formatted query packet
 */
Packet *
getNewSessionPacket(char *clientID, char *sessionID);

/**
 * @brief Helper to create a join session packet
 *
 * @param clientID ClientID string
 * @param sessionID SessionID string
 * @returns Formatted query packet
 */
Packet *
getJoinSessionPacket(char *clientID, char *sessionID);

/**
 * @brief Helper to create a leave session packet
 *
 * @param clientID ClientID string
 * @param sessionID Session to leave
 * @returns Formatted query packet
 */
Packet *
getLeaveSessionPacket(char *clientID, char *sessionID);

#endif