#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#define MAX_BUFF 2000

int main(int argc, char *argv[]) {

    // check that we have all needed params
    if (argc != 5) {
        printf("Run: %s [vpn-IP] [vpn-port] [server-IP] [server-port]",
                argv[0]);
        return -1;
    }

    int sd;
    int port;
    int n_bytes;
    struct sockaddr_in addr;
    char buffer[MAX_BUFF + 1];

    struct in_addr host_addr;
    struct hostent *ipv4address;

    char * second_port;
    char * IP_confirm;


    socklen_t addrsize = sizeof(addr);
    port = atoi(argv[2]);

    // set address to forward message to final server
    inet_pton(AF_INET, argv[1], &host_addr);
    ipv4address = gethostbyaddr(&host_addr, sizeof host_addr, AF_INET);
    if (!ipv4address) {
        perror("unknown address");
        return -1;
    }


    // set tunneld info
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    bcopy(ipv4address->h_addr, &(addr.sin_addr.s_addr), ipv4address->h_length);
    addr.sin_port = htons(port);

    // create socket for tunnel
    if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket error");
        return -1;
    }


    // write the message
    snprintf(buffer, sizeof(buffer), "$%s$%s", argv[3], argv[4]);
    //printf("%s\n", buffer);
    sendto(sd, buffer, strlen(buffer), 0, (struct sockaddr *) &addr,
           sizeof(addr));

    n_bytes = recvfrom(sd, buffer, sizeof(buffer), 0,
                       (struct sockaddr *) &addr, &addrsize);
    if (n_bytes > 0) {
        buffer[n_bytes] = '\0';
        second_port = strtok(buffer, "$");
        IP_confirm = strtok(NULL, "$");
        // print received port to stdout
        if (strcmp(IP_confirm, argv[3]) == 0) {
            printf("Port: %s\n", second_port);
        }

        //printf("IP_confirm: %s\n", IP_confirm);
    }
}
