//
// Network Helper header

#pragma once
#ifndef NETHELPER_H_
#define NETHELPER_H_

/** 
 * @brief Helper function to create a server socket, bind to it, and beginning
 * listening. 
 *
 * @param port Port to listen on
 * @returns Socket connection
 */
int getServerSocket(char *port);

/** 
 * @brief Helper to create a client socket and connect to the host and port.
 *
 * @param hostname Hostname of the server
 * @param port Port of the server to connect to
 * @returns Socket connection
 */
int getClientSocket(char * hostname, char * port);

#endif