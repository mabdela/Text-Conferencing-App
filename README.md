## Synopsis

This project implements a text conferencing client and serve by Mubarek Abdela and Jonathan Law. 

## Compiling
The program has been tested running on Ubuntu 14.04 on the Windows Bash Subsystem, and Debian

### Linux
Compile the program using `make`.

Run a server with `./server <port>`, and clients with `./client`.

Use `/help` in the client to view help information.

### Client
Clients are capable of running multiple tabs, where you can be in simulataneous sessions. Use `/switchtab` to cycle through them, or specify a number fo jump to it. The client can receive messages from all connected tabs, but can only send messages to one tab at a time. 

### Server
Username/passwords are stored in a `passwords.txt` file. The username/password is tab-delimited. Only one client can log in per credential, preventing two clients from logging in with the same credentials. 