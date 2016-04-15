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

#define TRUE              1
#define FALSE             0
#define RCV_BUFFER_SIZE   256
#define MSG_BUFFER_SIZE   RCV_BUFFER_SIZE + 4
#define MAX_CLIENTS       10
#define LISTEN_BACKLOG    5

volatile sig_atomic_t run_loop;

void error (char* msg)
{
    perror (msg);
    exit (EXIT_FAILURE);
}

void sigint_handler (int sig)
{
    write (STDOUT_FILENO, "\n\nSIGINT cought\n", 16);
    run_loop = FALSE;
}

int make_socket (int port, struct sockaddr_in *name)
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

int main(int argc, char *argv[])
{
    run_loop = TRUE;

    int     servsock_fd, clisock_fd[MAX_CLIENTS] = {0},
            port_num, cli_len, n, newfd;
    struct  sockaddr_in serv_addr, cli_addr;
    char    rcv_buffer[RCV_BUFFER_SIZE]; // For receiving messages
    char    msg_buffer[MSG_BUFFER_SIZE]; // For forwarding messsages

    // Set of filedescriptors for multple sockets
    fd_set  read_fd_set, active_fd_set;
    struct sigaction handler; // Signal handler

    /* Port error checking */
    if (argc < 2)
        error ("Port not provided.");
    for (n = 0; n < strlen (argv[1]); ++n) {
        if (! isdigit (argv[1][n])) 
            error ("Port should be a number.");
    }
    port_num = atoi (argv[1]);

    /* Setting the SIGINT handler so it could stop the while loop */
    handler.sa_handler = sigint_handler;
    handler.sa_flags = SA_RESTART;
    sigemptyset(&handler.sa_mask);
    if (sigaction(SIGINT, &handler, NULL) == -1) 
        error ("sigaction");

    /* Setting zeros for addr and creating socket */ 
    memset ((char*)&serv_addr, 0, sizeof (serv_addr));
    servsock_fd = make_socket (port_num, &serv_addr);

    if ( listen (servsock_fd, LISTEN_BACKLOG) == -1)
        error ("ERROR listening socket");
    puts ("Waiting for connections...");

    for (size_t i = 0; i < MAX_CLIENTS; i++)
        clisock_fd[i] = 0;
    cli_len = sizeof (cli_addr);
    FD_ZERO (&active_fd_set);
    FD_SET (servsock_fd, &active_fd_set);

    while (run_loop) {
        read_fd_set = active_fd_set;
        int ret = select (FD_SETSIZE, &read_fd_set, NULL, NULL, NULL);

        // If SIGINT is cought, run_loop will be set to FALSE
        if (errno == EINTR)
            continue;
        else if ( ret < 0 )
            error ("ERROR select");

        for (size_t fd = 0; fd < FD_SETSIZE; fd++) {
            if (FD_ISSET(fd, &read_fd_set) && errno != EINTR) {
                /* Incoming connection request */
                if (fd == servsock_fd) {
                    memset ((char*)&cli_addr, 0, sizeof (cli_addr));
                    newfd = accept (servsock_fd, (struct sockaddr *) &cli_addr,
                                    (socklen_t *)&cli_len);
                    if (newfd < 0)
                        error ("ERROR on accept");
                    /*printf("New connection: socket fd is %d, ip is : %s\n",
                            newfd, inet_ntoa(cli_addr.sin_addr));*/
                    printf("New connection: socket fd is %d\n",
                            newfd);

                    for (size_t i = 0; i < MAX_CLIENTS; i++) {
                        if (clisock_fd[i] == 0) {
                            clisock_fd[i] = newfd;
                            FD_SET(newfd, &active_fd_set);
                            break;
                        }
                    }
                }
                else {
                    memset (rcv_buffer, 0, RCV_BUFFER_SIZE);
                    memset (msg_buffer, 0, MSG_BUFFER_SIZE);
                    n = read (fd, rcv_buffer, RCV_BUFFER_SIZE);
                    if (n < 0)
                        error ("ERROR reading from socket");

                    // User disconnected
                    if (strlen(rcv_buffer) == 6 && rcv_buffer[0] == '/'
                             && rcv_buffer[1] == 'q' && rcv_buffer[2] == 'u'
                             && rcv_buffer[3] == 'i'  && rcv_buffer[4] == 't') {
                        FD_CLR (fd, &active_fd_set);
                        close (fd);
                        for (size_t i = 0; i < MAX_CLIENTS; i++) {
                            if (fd == clisock_fd[i]) {
                                clisock_fd[i] = 0;
                                break;
                            }       
                        }
                    }
                    // User sent a message
                    else {
                        sprintf (msg_buffer, "[%zu]", fd);
                        rcv_buffer[n] = '\0';
                        strcat(msg_buffer, rcv_buffer);

                        // serverside message print
                        fprintf (stdout, msg_buffer);

                        // send to all
                        for (size_t i = 0; i < MAX_CLIENTS; i++) {
                            if (clisock_fd[i] > 0 && fd != clisock_fd[i] ) {
                                n = write (clisock_fd[i], msg_buffer, MSG_BUFFER_SIZE);
                                if (n < 0)
                                    error ("ERROR writing to socket");
                            }
                        }
                    }
                }
            } // if FD_ISSET 
        } // fd loop
    } // run_loop

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

