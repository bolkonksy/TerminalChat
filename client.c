#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#define MSG_BUFFER_SIZE   256
#define RCV_BUFFER_SIZE   MSG_BUFFER_SIZE + 4 
#define TRUE              1
#define FALSE             0

volatile sig_atomic_t run_loop;

void sigint_handler (int sig)
{
    write (STDOUT_FILENO, "\n\nSIGINT cought\n", 16);
    run_loop = FALSE;
}

void error (char* msg)
{
    perror (msg);
    exit (EXIT_FAILURE);
}

int main (int argc, char *argv[])
{
    run_loop = TRUE;

    int     sock_fd, port_num, n;
    char    msg_buffer[MSG_BUFFER_SIZE]; // For sending messages
    char    rcv_buffer[RCV_BUFFER_SIZE]; // For receiving messages
    struct sockaddr_in serv_addr;
    struct hostent *server;

    fd_set  read_fd_set, active_fd_set;
    struct sigaction handler; // Signal handler
    
    if (argc < 3) {
        fputs ("Usage: %s [server-name] [port]\n", stderr);
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
    handler.sa_handler = sigint_handler;
    handler.sa_flags = SA_RESTART;
    sigemptyset(&handler.sa_mask);
    if (sigaction(SIGINT, &handler, NULL) == -1) 
        error ("sigaction");

    sock_fd = socket (AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1)
        error ("ERROR opening socket");
    puts ("Socket created...");

    server = gethostbyname (argv[1]);
    if (server == NULL)
        error ("ERROR resloving host");

    memset ((char*)&serv_addr, 0, sizeof (serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons (port_num);
    memcpy ((char*)&serv_addr.sin_addr.s_addr, 
            (char*)server->h_addr, 
            server->h_length);

    if (connect (sock_fd, (struct sockaddr*)&serv_addr,
                 sizeof (serv_addr)) == -1)
        error ("ERROR connecting");
    puts ("Connection established...");

    FD_ZERO (&active_fd_set);
    FD_SET (sock_fd, &active_fd_set); // For recieving from socket
    FD_SET (STDIN_FILENO, &active_fd_set); // Keyboard input

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
                /* Incoming data from server */
                if (fd == sock_fd) {
                    memset (rcv_buffer, 0, RCV_BUFFER_SIZE);
                    n = read (sock_fd, rcv_buffer, RCV_BUFFER_SIZE);
                    rcv_buffer[n] = '\n';
                    if (n < 0)
                        error ("ERROR reading from socket");

                    if (strlen(rcv_buffer) == 5 && rcv_buffer[0] == '/'
                             && rcv_buffer[1] == 'E' && rcv_buffer[2] == 'S'
                             && rcv_buffer[3] == 'C') {
                        run_loop = FALSE;
                        break;
                    } else 
                        printf ("%s", rcv_buffer);
                }
                /* Keyboard input */
                else if (fd == STDIN_FILENO) {
                    memset (msg_buffer, 0, MSG_BUFFER_SIZE);
                    fgets (msg_buffer, MSG_BUFFER_SIZE, stdin);

                    /* User is quiting */
                    if (strlen(msg_buffer) == 6 && msg_buffer[0] == '/'
                        && msg_buffer[1] == 'q' && msg_buffer[2] == 'u'
                        && msg_buffer[3] == 'i'  && msg_buffer[4] == 't') {
                        run_loop = FALSE;
                        break;
                    } else {
                        n = write (sock_fd, msg_buffer, strlen (msg_buffer));
                        if (n < 0)
                            error ("ERROR writing to socket");
                    }
                }
            } // if FD_ISSET
        } // fd for loop
    } // while run_loop
    write (sock_fd, "/quit\n", 6);
    close (sock_fd);
    puts ("Quiting...");

    return 0;
}
