/*
 * This file is called "server_functions.h",
 * but it also contains a several client defines and functions
*/
#ifndef SERVER_FUNCTIONS_C_
#define SERVER_FUNCTIONS_C_


#include <signal.h>
#include <netinet/in.h>

#define TRUE              1
#define FALSE             0
#define MSG_BUFFER_SIZE   256

/* Server specific variables */
#ifdef SERVER_C_
#define MAX_CLIENTS       10
#define LISTEN_BACKLOG    5
#endif

/* Global variable used by server and the client */
volatile sig_atomic_t run_loop;


/* Prints error message with perror() */
void error (const char* msg);

int check_command (const char* command, const char* rec_msg);

/* Signal functions */
void sigint_handler (int sig);
void init_signal_handler ();


/* Server specific functions */
int make_socket (const int port, struct sockaddr_in *name);
void send_to_sockets (const int* sockets, const char msg[MSG_BUFFER_SIZE],
                      const int num_of_sockets);

/* Client specific functions */
int connect_to_server_sock (const char* server_name, const int port);


#endif
