CC = gcc
CFLAGS = -Wall
NCURSES_FLAGS = -Wall -lncurses
TARGETS = server client chat

debug: CFLAGS += -g
debug: SERVER_FLAGS += -g
debug: all
all: $(TARGETS)

server: server.c
	$(CC) $(CFLAGS) server.c -o server

client: client.c
	$(CC) $(CFLAGS) client.c -o client

chat: chat.c
	$(CC) $(NCURSES_FLAGS) chat.c -o chat

clean:
	rm -fr $(TARGETS)
