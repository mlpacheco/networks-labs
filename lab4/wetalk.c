#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <signal.h>
#include <pthread.h>

#define MAX_BUFF 1000

volatile sig_atomic_t keep_prompting = 0;
struct sockaddr_in addr_rcv;
struct sockaddr_in addr_snd;
int sd_snd, sd_rcv;
int chatting = 0;
int quit = 0;
int was_asked = 0;


// signal handler to terminate client if no response
void catch_alarm (int sig) {
    printf("No response\n");
    keep_prompting = 1;
}

void *listen_connections(void *threadid) {
    char buffer[MAX_BUFF + 1];
    socklen_t addrsize = sizeof(addr_rcv);

    while(1) {
        if (chatting)
            pthread_exit(NULL);
        long tid;
        tid = (long) threadid;
        int len_rcv = recvfrom(sd_rcv, buffer, sizeof(buffer), 0, (struct sockaddr *) &addr_rcv, &addrsize);
        if (len_rcv > 0) {
            buffer[len_rcv] = '\0';
            if (strcmp(buffer, "wannatalk") == 0) {
                printf("\n| chat request from %s %d\n", inet_ntoa(addr_rcv.sin_addr), ntohs(addr_rcv.sin_port));
                was_asked = 1;
            }
        }
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {

    // check that we have all needed params
    if (argc != 2) {
        printf("Run: %s [my-port-number]\n", argv[0]);
        return -1;
    }

    // variables needed
    int port_rcv, port_snd, len_rcv;
    char buffer[MAX_BUFF + 1];
    char * inp_hostname;
    char * inp_port;
    struct hostent *ipv4address;
    struct in_addr host_addr;

    // parse input to appropriate types
    port_rcv = atoi(argv[1]);
    socklen_t addrsize = sizeof(addr_rcv);

    // set port info
    memset(&addr_rcv, 0, sizeof(addr_rcv));
    addr_rcv.sin_family = AF_INET;
    addr_rcv.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_rcv.sin_port = htons(port_rcv);

    // create sockets
    if ((sd_rcv = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("socket error");
        return -1;
    }

    if ((sd_snd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("socket error");
        return -1;
    }

    // bind listening socket to command line port
    if ((bind(sd_rcv, (struct sockaddr *)&addr_rcv, sizeof(addr_rcv))) < 0) {
        perror("bind error");
        return -1;
    }

    // stuff to handle signals
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = catch_alarm;
    sigaction (SIGALRM, &sa, 0);

    pthread_t thread;
    pthread_create(&thread, NULL, listen_connections, 0);

    while (!chatting) {
        // print prompt
        printf("? ");
        fgets(buffer, sizeof buffer, stdin);
        //printf("buffer %s buffer_size %lu\n", buffer, strlen(buffer));
        if (strlen(buffer) == 2) {
            if (strcmp(buffer, "q\n") == 0) {
                quit = 1;
                break;
            } else if (strcmp(buffer, "c\n") == 0 && was_asked) {
                sendto(sd_rcv, "OK", 2, 0, (struct sockaddr *) &addr_rcv, sizeof(addr_rcv));
                chatting = 1;
                continue;
            } else if (strcmp(buffer, "n\n") == 0 && was_asked) {
                sendto(sd_rcv, "KO", 2, 0, (struct sockaddr *) &addr_rcv, sizeof(addr_rcv));
                continue;
            } else {
                printf("Unknown command\n");
                continue;
            }

        } else {
            // parse hostname and port
            inp_hostname = strtok(buffer, " ");
            ipv4address = gethostbyname(inp_hostname);
            if (!ipv4address) {
                printf("Unknown host\n");
                continue;
            }

            inp_port = strtok(NULL, "\n");
            port_snd = atoi(inp_port);

            // set info to communicate with specified host and port
            memset(&addr_snd, 0, sizeof(addr_snd));
            addr_snd.sin_family = AF_INET;
            bcopy ( ipv4address->h_addr, &(addr_snd.sin_addr.s_addr), ipv4address->h_length);
            addr_snd.sin_port = htons(port_snd);

            // send message
            ualarm(7000000, 0);
            sendto(sd_snd, "wannatalk", 9, 0, (struct sockaddr *) &addr_snd, sizeof(addr_snd));
            len_rcv = recvfrom(sd_snd, buffer, sizeof(buffer), 0, (struct sockaddr *) &addr_snd, &addrsize);
            // cancel timeout
            ualarm(0, 0);

            // break out of the loop if user wants to chat
            if (len_rcv > 0) {
                buffer[len_rcv] = '\0';
                if (strcmp(buffer, "OK") == 0) {
                    chatting = 1;
                    continue;
                } else if (strcmp(buffer, "KO") == 0) {
                    printf("| doesn't want to chat\n");
                    continue;
                }
            }

        }

    }

    if (quit) {
        printf("Goodbye!\n");
    }

    if (chatting) {
        printf("we are chatting!\n");
    }

}
