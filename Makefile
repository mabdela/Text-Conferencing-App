# Build targets
TARGET = server client

# Compiler
CC = gcc

# Source files
SRCS = client.c server.c utils/nethelper.c utils/transport.c chatClient.c utils/printHelpers.c collections/linkedList.c collections/hashTable.c chatServer.c

# Compile flags.
CFLAGS = -g -Wall -O3
LDFLAGS = -g -Wall -pthread

# Default targets.
all: $(TARGET)

# Build the server.
server: server.o utils/nethelper.o utils/transport.o utils/printHelpers.o collections/linkedList.o collections/hashTable.o chatServer.o
	$(CC) $(LDFLAGS) $^ -o $@

# Build the client.
client: client.o utils/nethelper.o chatClient.o utils/transport.o utils/printHelpers.o
	$(CC) $(LDFLAGS) $^ -o $@

# Compile a .c source file to a .o object file.
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Delete generated files.
clean:
	-rm -rf $(TARGET) *.o **/*.o