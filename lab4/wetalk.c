#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include <termios.h>

#define MAX_BUFF 1000

// socket info that we need to share with signals
int socket_listen, socket_send;
struct sockaddr_in their_addr;

// flags to control program in and out of signals
volatile sig_atomic_t chatting = 0;
volatile sig_atomic_t is_client = 0;

// log of partially writen message
unsigned char my_log[MAX_BUFF + 1];

// signal handler to timeout if waiting for peer response
void alarm_handl(int sig) {
    printf("No response\n");
}

// signal handler for SIGPOLL at connection time
// react to incoming connections requests
void sigpoll_handl_connections(int sig) {
    if (!chatting) {
        int num_bytes;
        char buffer[MAX_BUFF + 1];
        int addr_len = sizeof(their_addr);
        struct hostent *ipv4address;
        struct in_addr host_addr;

        num_bytes = recvfrom(socket_listen, buffer, sizeof buffer, 0,
                            (struct sockaddr *)&their_addr, &addr_len);
        if (num_bytes > 0) {
            buffer[num_bytes] = '\0';
            // check if message is a request and read terminal response
            if (strcmp(buffer, "wannatalk") == 0) {
                char * peer_address = inet_ntoa(their_addr.sin_addr);
                int  peer_port = ntohs(their_addr.sin_port);
                printf("\n| chat request from %s %d\n",
                       peer_address, peer_port);
                fputs("? ", stdout);
                fgets(buffer, sizeof buffer, stdin);
                // send response message or finish execution
                if (strcmp(buffer, "c\n") == 0) {
                    sendto(socket_listen, "OK", 2, 0,
                          (struct sockaddr *)&their_addr, addr_len);
                    chatting = 1;
                } else if (strcmp(buffer, "n\n") == 0) {
                    sendto(socket_listen, "KO", 2, 0,
                          (struct sockaddr *)&their_addr, addr_len);
                } else if (strcmp(buffer, "q\n") == 0) {
                    exit(0);
                }
            }
        }
    }
}

// signal handler for SIGPOLL at chat time
// displays incoming message or disable chatting flag
void sigpoll_handl_chat(int sig) {
    //printf("sigpoll_handl_chat\n");
    if (chatting) {
        int num_bytes;
        struct sockaddr_in their_addr2;
        char buffer[MAX_BUFF + 1];
        int addr_len = sizeof(their_addr2);

        if (!is_client) {
            num_bytes = recvfrom(socket_listen, buffer, sizeof buffer, 0,
                                (struct sockaddr *)&their_addr2, &addr_len);
        } else {
            num_bytes = recvfrom(socket_send, buffer, sizeof buffer, 0,
                                (struct sockaddr *)&their_addr2, &addr_len);
        }
        if (num_bytes > 0) {
            buffer[num_bytes] = '\0';
            if (buffer[0] == 'D') {
                char subbuff[strlen(buffer) - 1];
                memcpy(subbuff, &buffer[1], strlen(buffer) - 1);
                subbuff[strlen(buffer) - 1] = '\0';
                printf("\n| %s\n", subbuff);
                printf("> %s", my_log);
            } else if (num_bytes == 1 && buffer[0] == 'E') {
                printf("\n| chat terminated\n");
                chatting = 0;
            }
        }
    }
}

int main(int argc, char *argv[]) {

    // check that we have all needed params
    if (argc != 2) {
        printf("Run: %s [my-port-number]\n", argv[0]);
        return -1;
    }

    // variables needed
    int port_listen, port_send, num_bytes;
    char buffer[MAX_BUFF + 1];
    char * inp_hostname;
    char * inp_port;
    struct hostent *ipv4address;
    struct in_addr host_addr;
    struct sockaddr_in addr_listen;
    struct sockaddr_in addr_send;
    struct sockaddr_in addr_source;

    // parse input to appropriate types
    port_listen = atoi(argv[1]);
    socklen_t addrsize = sizeof(addr_listen);

    // set port info
    memset(&addr_listen, 0, sizeof(addr_listen));
    addr_listen.sin_family = AF_INET;
    addr_listen.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_listen.sin_port = htons(port_listen);


    // create sockets
    if ((socket_listen = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket error");
        return -1;
    }

    if ((socket_send = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket error");
        return -1;
    }

    // bind listening socket to command line port
    if (bind(socket_listen, (struct sockaddr *)&addr_listen, sizeof addr_listen) < 0) {
        perror("bind error");
        return -1;
    }

    // stuff to handle alarm signals
    struct sigaction sa_alarm;
    memset(&sa_alarm, 0, sizeof sa_alarm);
    sa_alarm.sa_handler = alarm_handl;
    sigaction (SIGALRM, &sa_alarm, 0);

    // stuff to handle sigpoll signals on server socket

    while (1) {
        // signal to handle incomming connection requests
        struct sigaction sa_sigpoll;
        memset(&sa_sigpoll, 0, sizeof sa_sigpoll);
        sa_sigpoll.sa_handler = sigpoll_handl_connections;
        sa_sigpoll.sa_flags = SA_NODEFER;
        sigaction (SIGPOLL, &sa_sigpoll, 0);
        fcntl(socket_listen, F_SETOWN, getpid());
        fcntl(socket_listen, F_SETFL, FASYNC);

        is_client = 0;

        // set up a connection between two peers
        while (!chatting) {
            fputs("? ", stdout);
            fgets(buffer, sizeof buffer, stdin);
            //printf("buffer %s buffer_size %lu\n", buffer, strlen(buffer));

            if (strcmp(buffer, "q\n") == 0)
                exit(0);

            // parse hostname and port
            if ((inp_hostname = strtok(buffer, " ")) == NULL) {
                continue;
            }

            ipv4address = gethostbyname(inp_hostname);
            if (!ipv4address) {
                continue;
            }

            if ((inp_port = strtok(NULL, "\n")) == NULL) {
                continue;
            }
            port_send = atoi(inp_port);

            // set port info
            memset(&addr_source, 0, sizeof(addr_source));
            addr_source.sin_family = AF_INET;
            addr_source.sin_addr.s_addr = htonl(INADDR_ANY);
            addr_source.sin_port = htons(45678);

            if ((bind(socket_send, (struct sockaddr *) &addr_source, sizeof addr_source)) < 0) {
                perror("bind error");
                return -1;
            }

            // set info to communicate with specified host and port
            memset(&addr_send, 0, sizeof(addr_send));
            addr_send.sin_family = AF_INET;
            bcopy ( ipv4address->h_addr, &(addr_send.sin_addr.s_addr), ipv4address->h_length);
            addr_send.sin_port = htons(port_send);

            // send message
            alarm(7);
            sendto(socket_send, "wannatalk", 9, 0, (struct sockaddr *) &addr_send, sizeof(addr_send));
            num_bytes = recvfrom(socket_send, buffer, sizeof(buffer), 0, (struct sockaddr *) NULL, &addrsize);
            // cancel timeout
            alarm(0);

            if (num_bytes > 0) {
                buffer[num_bytes] = '\0';
                if (strcmp(buffer, "OK") == 0) {
                    printf("");
                    chatting = 1;
                    is_client = 1;
                } else if (strcmp(buffer, "KO") == 0) {
                    printf("| doesn't want to chat\n");
                }
            }
        }

        // signal to handle incoming messages
        struct sigaction sa_sigpoll_2;
        memset(&sa_sigpoll_2, 0, sizeof sa_sigpoll_2);
        sa_sigpoll_2.sa_handler = sigpoll_handl_chat;
        sa_sigpoll_2.sa_flags = SA_NODEFER;
        sigaction (SIGPOLL, &sa_sigpoll_2, 0);

        // depending on who is listening to the signal
        if (!is_client) {
            fcntl(socket_listen, F_SETOWN, getpid());
            fcntl(socket_listen, F_SETFL, FASYNC);
        } else  {
            fcntl(socket_send, F_SETOWN, getpid());
            fcntl(socket_send, F_SETFL, FASYNC);
        }

        // put the terminal to raw mode
        static struct termios oldt, newt;
        tcgetattr( STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON);
        tcsetattr( STDIN_FILENO, TCSANOW, &newt);

        char c;
        unsigned char message[MAX_BUFF + 1];
        my_log[0] = '\0';

        // start chatting
        while (chatting) {
            printf("> ");

            // construct message and keep log char by char
            message[0] = 'D';
            int i = 1;
            while ((c = getchar()) != '\n') {
                message[i] = c;
                my_log[i - 1] = c;
                my_log[i] = '\0';
                i++;
            }
            message[i] = '\0';

            //printf("message: %s\n", message);
            if (is_client) {
                sendto(socket_send, message, strlen(message), 0, (struct sockaddr *) &addr_send, sizeof(addr_send));
            } else {
                sendto(socket_listen, message, strlen(message), 0, (struct sockaddr *)&their_addr, sizeof(their_addr));
            }

            my_log[0] = '\0';
        }

        // go back to canonical mode
        tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
    }

}
