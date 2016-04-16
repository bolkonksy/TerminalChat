#define SERVER_C_
#include "server_functions.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

int main (int argc, char *argv[])
{
    int     servsock_fd, clisock_fd[MAX_CLIENTS] = {0},
            port_num, cli_len, n, newfd;
    struct  sockaddr_in serv_addr, cli_addr;
    char    rcv_buffer[MSG_BUFFER_SIZE]; // For receiving messages */
    char    msg_buffer[MSG_BUFFER_SIZE]; // For forwarding messsages */
    /* Set of filedescriptors for multple sockets */
    fd_set  read_fd_set, active_fd_set;

    run_loop = TRUE;

    /* Port error checking */
    if (argc < 2) {
        fprintf (stderr, "Usage: %s [port]\n", argv[0]);
        return -1;
    }
    for (n = 0; n < strlen (argv[1]); ++n) {
        if (! isdigit (argv[1][n])) {
            fputs ("Port should be a number\n", stderr);
            return -1;
        }
    }
    port_num = atoi (argv[1]);

    /* Setting the SIGINT handler so it could stop the while loop */
    init_signal_handler ();

    /* Creating server socket */ 
    memset ((char*)&serv_addr, 0, sizeof (serv_addr));
    servsock_fd = make_socket (port_num, &serv_addr);

    if (listen (servsock_fd, LISTEN_BACKLOG) == -1)
        error ("ERROR listening socket");
    puts ("Waiting for connections...");

    cli_len = sizeof (cli_addr);
    FD_ZERO (&active_fd_set);
    FD_SET (servsock_fd, &active_fd_set);

    while (run_loop) {
        read_fd_set = active_fd_set;
        int ret = select (FD_SETSIZE, &read_fd_set, NULL, NULL, NULL);

        // If SIGINT is cought, run_loop will be set to FALSE */
        if (errno == EINTR)
            continue;
        else if (ret < 0)
            error ("ERROR select");

        for (size_t fd = 0; fd < FD_SETSIZE; fd++) {
            if (FD_ISSET (fd, &read_fd_set) && errno != EINTR) {
                /* Incoming connection request */
                if (fd == servsock_fd) {
                    memset ((char*)&cli_addr, 0, sizeof (cli_addr));
                    newfd = accept (servsock_fd,
                                    (struct sockaddr *) &cli_addr,
                                    (socklen_t *)&cli_len);
                    if (newfd < 0)
                        error ("ERROR on accept");
                    /*printf("New connection: socket fd is %d, ip is: %s\n",
                            newfd, inet_ntoa(cli_addr.sin_addr));*/
                    printf ("New connection: socket fd is %d\n", newfd);
                    for (size_t i = 0; i < MAX_CLIENTS; i++) {
                        if (clisock_fd[i] == 0) {
                            clisock_fd[i] = newfd;
                            FD_SET(newfd, &active_fd_set);
                            break;
                        }
                    }
                }
                else {
                    memset (rcv_buffer, 0, MSG_BUFFER_SIZE);
                    memset (msg_buffer, 0, MSG_BUFFER_SIZE);
                    n = read (fd, rcv_buffer, MSG_BUFFER_SIZE);
                    if (n < 0)
                        error ("ERROR reading from socket");

                    // User disconnected */
                    if (check_command ("/quit\n", rcv_buffer)) {
                        FD_CLR (fd, &active_fd_set);
                        close (fd);
                        for (size_t i = 0; i < MAX_CLIENTS; i++) {
                            if (fd == clisock_fd[i]) {
                                clisock_fd[i] = 0;
                                break;
                            }       
                        }
                        /* Inform other clients */
                        snprintf (msg_buffer, MSG_BUFFER_SIZE,
                                  "User [%zu] disconnected\n", fd);
                        send_to_sockets (clisock_fd, msg_buffer,
                                         MAX_CLIENTS);
                        printf ("User [%zu] disconnected\n", fd);
                    }
                    // User sent a message */
                    else {
                        snprintf (msg_buffer, MSG_BUFFER_SIZE, "[%zu]", fd);
                        rcv_buffer[n] = '\0';
                        strcat (msg_buffer, rcv_buffer);

                        // serverside message print */
                        fprintf (stdout, msg_buffer);

                        /* Send to everyone */
                        send_to_sockets (clisock_fd, msg_buffer,
                                         MAX_CLIENTS);
                    }
                }
            } // if FD_ISSET  */
        } // fd for loop */
    } // while run_loop */

    snprintf (msg_buffer, MSG_BUFFER_SIZE, "Server closing...\n");
    send_to_sockets (clisock_fd, msg_buffer, MAX_CLIENTS);
    /* Command to terminate running clients */
    snprintf (msg_buffer, MSG_BUFFER_SIZE, "/ESC\n");
    send_to_sockets (clisock_fd, msg_buffer, MAX_CLIENTS);
    usleep (500);

    puts ("Closing all sockets...");
    for (size_t i = 0; i < MAX_CLIENTS; i++) {
        if (clisock_fd[i] > 0) {
            printf ("Closing socket: %d \n", clisock_fd[i]);
            close (clisock_fd[i]);
        }       
    }
    printf ("Closing server socket: %d \n", servsock_fd);
    close (servsock_fd);

    puts ("Exiting...");
    return 0;
}
