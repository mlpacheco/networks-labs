#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define MAX_BUFF 1000

volatile sig_atomic_t keep_going = 1;

// signal handler to terminate client if no response
void catch_alarm (int sig) {
    printf("No response\n");
    keep_going = 0;
}

int main(int argc, char *argv[]) {

    // check that we have all needed params
    if (argc != 2) {
        printf("Run: %s [my-port-number]\n", argv[0]);
        return -1;
    }

    // variables needed
    int port_rcv, port_snd, sd_rcv, sd_snd, len_rcv;
    struct sockaddr_in addr_rcv;
    struct sockaddr_in addr_snd;
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

    signal(SIGALRM, catch_alarm);

    while (1) {
        // print prompt
        printf("? ");
        fgets(buffer, sizeof buffer, stdin);
        //printf("%s", buffer);

        // parse input hostname and port
        inp_hostname = strtok(buffer, " ");
        inp_port = strtok(NULL, "\n");

        //printf("hostname: %s\n", inp_hostname);
        //printf("port: %s\n", inp_port);

        // resolve IP address and port
        ipv4address = gethostbyname(inp_hostname); // if host unkown print msg
        port_snd = atoi(inp_port);

        // set info to communicate with specified host and port
        memset(&addr_snd, 0, sizeof(addr_snd));
        addr_snd.sin_family = AF_INET;
        bcopy ( ipv4address->h_addr, &(addr_snd.sin_addr.s_addr), ipv4address->h_length);
        addr_snd.sin_port = htons(port_snd);

        // send message
        printf("sending message\n");
        ualarm(7000000, 0);
        sendto(sd_snd, "wannatalk", 9, 0, (struct sockaddr *) &addr_snd, sizeof(addr_snd));
        printf("before recv\n");

        len_rcv = recvfrom(sd_rcv, buffer, sizeof(buffer), 0, (struct sockaddr *) &addr_rcv, &addrsize);

        printf("after recv\n");

    }

}
