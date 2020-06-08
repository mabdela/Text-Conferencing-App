//
// Client interface implementation

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include "utils/nethelper.h"
#include "utils/printHelpers.h"
#include "chatClient.h"

enum STATE
{
	disconnected,
	loggedin,
	insession,
};

void show_help();

int main()
{
	enum STATE currentState = disconnected;
	SessionInfo *sess = chatclient_init();

	// Buffer for command inputs, set to OS maximum input
	char cmdBuffer[MAX_INPUT];

	// Loop indefinitely
	while (1)
	{
		//Read in a command, print the current session name if applicable
		if(sess->currSessionID[sess->currSession] != NULL) {
		    printf("Tab %d '%.64s'> ", sess->currSession + 1, sess->currSessionID[sess->currSession]);
		}
		else {
			printf("Tab %d> ", sess->currSession + 1);
		}
		//Clear the commandbuffer
		memset(cmdBuffer, 0, MAX_INPUT);
		char *read = fgets (cmdBuffer, MAX_INPUT, stdin);

		if(read == NULL) {
		    printf("Error at fgets()\n");
		}
		
		//Command start with a / character
		if (cmdBuffer[0] == '/')
		{
			int tokenCount;
			char **tokens = parseTokens(cmdBuffer, " \n", &tokenCount);
			char *token = tokens[0] + 1; // Advance by 1 to remove /

			if (strcmp(token, "login") == 0 && tokenCount == 5)
			{
				if (chatclient_login(sess, tokens[1], tokens[2], tokens[3],
									 tokens[4]) == 0)
				{
					currentState = loggedin;
					printf("Connected.\n");
				}
				else
				{
					printf("Log in error.\n");
				}
			}
			else if (strcmp(token, "logout") == 0 && tokenCount == 1)
			{
				int ret = chatclient_logout(sess);
				if (ret == 0) {
					currentState = disconnected;
				}
				else {
					printf("Error, cannot logout.\n");
				}
			}
			else if (strcmp(token, "joinsession") == 0 && tokenCount == 2)
			{
				int ret = chatclient_joinSession(sess, tokens[1]);
				if (ret == 0)
				{
					currentState = insession;
				}
				else
				{
					printf("Cannot join session.\n");
				}
			}
			else if (strcmp(token, "leavesession") == 0 && tokenCount == 1)
			{
				int ret = chatclient_leaveSession(sess);
				if (ret == 0) {
					currentState = loggedin;
				}
				else {
					printf("Cannot leave sesesion.\n");
				}
			}
			else if (strcmp(token, "createsession") == 0 && tokenCount == 2)
			{
				int ret = chatclient_createSession(sess, tokens[1]);
				if (ret == 0)
				{
					currentState = insession;
				}
				else
				{
					printf("Session creation error.\n");
				}
			}
			else if (strcmp(token, "list") == 0 && tokenCount == 1)
			{
				int ret = chatclient_list(sess);
				if (ret != 0) {
					printf("Error listing sessions\n");
				}
			}
			else if (strcmp(token, "switchtab") == 0 && (tokenCount == 1 || tokenCount == 2)) {
				// Accept either /switchtab to cycle through, or /switchtab <tab> to jump to specified
			    int sessionVal;
			    if (tokenCount == 1)
			    {
			    	sessionVal = (sess->currSession + 1) % MAX_SIMUL_SESSIONS;
				}
				else {
				    char *endPtr;
				    long temp = strtol(tokens[1], &endPtr, 10);

					if(endPtr == tokens[1]) {
						//No conversion happened
					    sessionVal = -1;
					} else {
					    sessionVal = (int)temp - 1;
					}
				}

				if(sessionVal >= 0 && sessionVal < MAX_SIMUL_SESSIONS) {
					sess->currSession = sessionVal;
					if (sess->currSessionID[sess->currSession] == NULL) {
				    	currentState = loggedin;
					} else {
					    currentState = insession;
					}

			    	printf("Switched to tab %d\n", sess->currSession + 1);
				}
				else {
				    printf("Invalid session number\n");
				}
			}
			else if (strcmp(token, "quit") == 0)
			{
				exit(0);
			}
			else
			{
				printf("Unrecognized command. ");
				show_help();
			}

			free(tokens);
		}
		else
		{
			if (currentState != insession)
			{
				show_help();
			}
			else
			{
				int ret = chatclient_sendMessage(sess, cmdBuffer);
				if(ret != 0) {
					printf("Error sending message.\n");
				}
			}
		}
	}

	// Should never reach here
	return 0;
}

// Outputs the command help text into stdout
void show_help()
{
	printf("Usage:\n ");
	printf("\t/login <clientID> <password> <serverIP> <serverPort>\n");
	printf("\t/logout\n");
	printf("\t/joinsession <sessionID>\n");
	printf("\t/leavesession\n");
	printf("\t/createsession <sessionID>\n");
	printf("\t/switchtab <tab (optional)>\n");
	printf("\t/list\n");
	printf("\t/quit\n");
}
