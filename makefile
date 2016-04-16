CC = gcc
CFLAGS = -Wall
NCURSES_FLAGS = -Wall -lncurses
DEPS = server_functions.c
TARGETS = server client chat

debug: CFLAGS += -g
debug: SERVER_FLAGS += -g
debug: all
all: $(TARGETS)

server: server.c $(DEPS)
	$(CC) $(CFLAGS) $(DEPS) server.c -o server

client: client.c $(DEPS)
	$(CC) $(CFLAGS) $(DEPS) client.c -o client

chat: chat.c
	$(CC) $(NCURSES_FLAGS) chat.c -o chat

clean:
	rm -fr $(TARGETS)
