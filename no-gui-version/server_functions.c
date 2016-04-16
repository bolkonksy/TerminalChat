#include "server_functions.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>


void error (const char* msg)
{
    perror (msg);
    exit (EXIT_FAILURE);
}

int check_command (const char* command, const char* rec_msg)
{
    if (command != NULL && rec_msg != NULL
        && strlen(command) > 0 && strlen(rec_msg) > 0)
        if (strcmp (command, rec_msg) == 0)
            return TRUE;
    return FALSE;
}

void sigint_handler (int sig)
{
    write (STDOUT_FILENO, "\nSIGINT cought\n", 16);
    run_loop = FALSE;
}

void init_signal_handler () 
{
    struct sigaction    handler;

    handler.sa_handler = sigint_handler;
    handler.sa_flags = SA_RESTART;
    sigemptyset (&handler.sa_mask);
    if (sigaction (SIGINT, &handler, NULL) == -1) 
        error ("sigaction");
}

int make_socket (const int port, struct sockaddr_in *name)
{
    int sock, opt = TRUE;

    sock = socket (AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error ("ERROR opening socket");
    puts ("Socket created...");

    name->sin_family = AF_INET;
    name->sin_port = htons (port);
    name->sin_addr.s_addr = INADDR_ANY;

    if( setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
                   sizeof(opt)) < 0 )
        error ("ERROR setsockopt");

    if (bind (sock, (struct sockaddr *) name, 
                sizeof (struct sockaddr_in)) == -1)
        error ("ERROR binding");
    puts ("Binding done...");

    return sock;
}

void send_to_sockets (const int* sockets, const char msg[MSG_BUFFER_SIZE],
                      const int num_of_sockets)
{
    for (size_t i = 0; i < num_of_sockets; i++) {
        if (sockets[i] > 0) {
            int ret = write (sockets[i], msg, MSG_BUFFER_SIZE);
            if (ret < 0)
                error ("ERROR writing to socket");
        }
    }
}

int connect_to_server_sock (const char* server_name, const int port)
{
    int                sock;
    struct sockaddr_in serv_addr;
    struct hostent*    server;

    sock = socket (AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error ("ERROR opening socket");
    puts ("Socket created...");

    server = gethostbyname (server_name);
    if (server == NULL)
        error ("ERROR resloving host");

    memset ((char*)&serv_addr, 0, sizeof (serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons (port);
    memcpy ((char*)&serv_addr.sin_addr.s_addr, 
            (char*)server->h_addr, 
            server->h_length);

    if (connect (sock, (struct sockaddr*)&serv_addr,
                 sizeof (serv_addr)) == -1)
        error ("ERROR connecting");
    puts ("Connection established...");

    return sock;
}
