#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>

#define MAX_BUFF 1000

int socket_listen, socket_send;
volatile sig_atomic_t chatting = 0;
volatile sig_atomic_t quit = 0;

// signal handler to terminate client if no response
void alarm_handl(int sig) {
    printf("No response\n");
}

// signal handler for SIGPOLL
void sigpoll_handl_connections(int sig) {
    if (!chatting) {
        int num_bytes;
        struct sockaddr_in their_addr;
        char buffer[MAX_BUFF];
        int addr_len = sizeof(their_addr);

        num_bytes = recvfrom(socket_listen, buffer, sizeof buffer, 0, (struct sockaddr *)&their_addr, &addr_len);
        if (num_bytes > 0) {
            buffer[num_bytes] = '\0';
            if (strcmp(buffer, "wannatalk") == 0) {
                printf("\n| chat request from %s %d\n", inet_ntoa(their_addr.sin_addr), ntohs(their_addr.sin_port));
                fputs("? ", stdout);
                fgets(buffer, sizeof buffer, stdin);
                if (strcmp(buffer, "c\n") == 0) {
                    sendto(socket_listen, "OK", 2, 0, (struct sockaddr *)&their_addr, addr_len);
                    chatting = 1;
                } else if (strcmp(buffer, "n\n") == 0) {
                    sendto(socket_listen, "KO", 2, 0, (struct sockaddr *)&their_addr, addr_len);
                } else if (strcmp(buffer, "q\n") == 0) {
                    exit(0);
                }
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
    //int quit = 0;
    //int was_asked = 0;
    //int send_on_listen = 0;

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

    int length = sizeof(addr_listen);
    if (getsockname(socket_listen, (struct sockaddr *)&addr_listen, &length) < 0) {
        perror("getting socket name");
        return -1;
    }

    // stuff to handle alarm signals
    struct sigaction sa_alarm;
    memset(&sa_alarm, 0, sizeof sa_alarm);
    sa_alarm.sa_handler = alarm_handl;
    sigaction (SIGALRM, &sa_alarm, 0);

    // stuff to handle sigpoll signals on server socket
    struct sigaction sa_sigpoll;
    memset(&sa_sigpoll, 0, sizeof sa_sigpoll);
    sa_sigpoll.sa_handler = sigpoll_handl_connections;
    sigaction (SIGPOLL, &sa_sigpoll, 0);
    fcntl(socket_listen, F_SETOWN, getpid());
    fcntl(socket_listen, F_SETFL, FASYNC);

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

        // set info to communicate with specified host and port
        memset(&addr_send, 0, sizeof(addr_send));
        addr_send.sin_family = AF_INET;
        bcopy ( ipv4address->h_addr, &(addr_send.sin_addr.s_addr), ipv4address->h_length);
        addr_send.sin_port = htons(port_send);

        // send message
        alarm(7);
        sendto(socket_send, "wannatalk", 9, 0, (struct sockaddr *) &addr_send, sizeof(addr_send));
        num_bytes = recvfrom(socket_send, buffer, sizeof(buffer), 0, (struct sockaddr *) &addr_send, &addrsize);
        // cancel timeout
        alarm(0);

        if (num_bytes > 0) {
            buffer[num_bytes] = '\0';
            if (strcmp(buffer, "OK") == 0) {
                chatting = 1;
            } else if (strcmp(buffer, "KO") == 0) {
                printf("| doesn't want to chat\n");
            }
        }
    }

    printf("> \n");

}
