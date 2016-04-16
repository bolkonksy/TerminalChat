#define CLIENT_C_
#include "server_functions.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>

int main (int argc, char *argv[])
{
    int     sock_fd, port_num, n;
    char    msg_buffer[MSG_BUFFER_SIZE]; /* For sending messages */
    char    rcv_buffer[MSG_BUFFER_SIZE]; /* For receiving messages */
    fd_set  read_fd_set, active_fd_set;

    run_loop = TRUE;
    
    /* Port error checking */
    if (argc < 3) {
        fprintf (stderr, "Usage: %s [server-name] [port]\n", argv[0]);
        return -1;
    }
    for (n = 0; n < strlen (argv[2]); ++n) {
        if (! isdigit (argv[2][n])) {
            fputs ("Port should be a number\n", stderr);
            return -1;
        }   
    }
    port_num = atoi (argv[2]);

    /* Setting the SIGINT handler so it could stop the while loop */
    init_signal_handler ();

    sock_fd = connect_to_server_sock (argv[1], port_num); 

    FD_ZERO (&active_fd_set);
    FD_SET (sock_fd, &active_fd_set); /* For recieving from socket */
    FD_SET (STDIN_FILENO, &active_fd_set); /* Keyboard input */

    while (run_loop) {
        read_fd_set = active_fd_set;
        int ret = select (FD_SETSIZE, &read_fd_set, NULL, NULL, NULL);

        /* If SIGINT is cought, run_loop will be set to FALSE */
        if (errno == EINTR)
            continue;
        else if ( ret < 0 )
            error ("ERROR select");

        for (size_t fd = 0; fd < FD_SETSIZE; fd++) {
            if (FD_ISSET(fd, &read_fd_set) && errno != EINTR) {
                /* Incoming data from server */
                if (fd == sock_fd) {
                    memset (rcv_buffer, 0, MSG_BUFFER_SIZE);
                    n = read (sock_fd, rcv_buffer, MSG_BUFFER_SIZE);
                    if (n < 0)
                        error ("ERROR reading from socket");

                    /* Server shutting down */
                    if (check_command ("/ESC\n", rcv_buffer)) {
                        run_loop = FALSE;
                        break;
                    } else 
                        printf ("%s", rcv_buffer);
                }
                /* Keyboard input */
                else if (fd == STDIN_FILENO) {
                    memset (msg_buffer, 0, MSG_BUFFER_SIZE);
                    /* Reserving 4 characters for a socketId, which
                     * will later be added by the server */
                    fgets (msg_buffer, MSG_BUFFER_SIZE - 4, stdin);

                    /* User is quiting */
                    if (check_command ("/quit\n", msg_buffer)) {
                        run_loop = FALSE;
                        break;
                    } else {
                        n = write (sock_fd, msg_buffer, strlen (msg_buffer));
                        if (n < 0)
                            error ("ERROR writing to socket");
                    }
                }
            } /* if FD_ISSET */
        } /* fd for loop */
    } /* while run_loop */
    write (sock_fd, "/quit\n", 6);
    close (sock_fd);
    puts ("Quiting...");

    return 0;
}
