//
// Network Helper implementation

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include "nethelper.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "printHelpers.h"

#define LISTEN_QUEUE_DEPTH 16

/** 
 * @brief Helper function to create a server socket, bind to it, and beginning
 * listening. 
 *
 * @param port Port to listen on
 * @returns Socket connection
 */
int getServerSocket(char *port)
{
    // Create a socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
    {
		printLastError("Error at socket(): %s\n");
		return -1;
	}

	// Bind to the created socket
	struct sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = (unsigned long)htonl(INADDR_ANY);
	serv_addr.sin_port = (unsigned short)htons(atoi(port));

	int bindRes = bind(sock, (const struct sockaddr*)&serv_addr, sizeof(serv_addr));

	if (bindRes < 0)
	{
	    printLastError("Error at bind(): %s\n");
		close(sock);
		return -1;
    }

	//
    int listenRes = listen(sock, LISTEN_QUEUE_DEPTH);
	if (listenRes != 0)
    {
		printLastError("Error at listen(): %s\n");
		close(sock);
		return -1;
    }


    return sock;
}

/** 
 * @brief Helper to create a client socket and connect to the host and port.
 *
 * @param hostname Hostname of the server
 * @param port Port of the server to connect to
 * @returns Socket connection
 */
int getClientSocket(char *hostname, char *port)
{
    int sock;
    struct addrinfo hints, *servinfo, *p;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // use AF_INET6 to force IPv6
    hints.ai_socktype = SOCK_STREAM;

    rv = getaddrinfo(hostname, port, &hints, &servinfo);

    if (rv != 0)
    {
		// Linux has a special gai_strerror for these errors
		fprintf(stderr, "Error at getaddrinfo(): %s\n", gai_strerror(rv));
		return -1;
    }

    // loop through all the results and connect to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
		if ((sock = socket(p->ai_family, p->ai_socktype,
					p->ai_protocol)) == -1)
		{
			continue;
		}

		if (connect(sock, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(sock);
			continue;
		}

		break; // if we get here, we must have connected successfully
    }

    if (p == NULL)
    {
		// looped off the end of the list with no connection
		fprintf(stderr, "Failed to connect\n");
		return -1;
    }

    freeaddrinfo(servinfo); // all done with this structure

    return sock;
}
