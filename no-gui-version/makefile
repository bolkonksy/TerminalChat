CC = gcc
CFLAGS = -Wall
DEPS = server_functions.c
TARGETS = server client

debug: CFLAGS += -g
debug: SERVER_FLAGS += -g
debug: all
all: $(TARGETS)

server: server.c $(DEPS)
	$(CC) $(CFLAGS) $(DEPS) server.c -o server

client: client.c $(DEPS)
	$(CC) $(CFLAGS) $(DEPS) client.c -o client

clean:
	rm -fr $(TARGETS)
