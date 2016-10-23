#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#define MAX_BUFF 2000

int main(int argc, char *argv[]) {

    // check that we have all needed params
    if (argc != 5) {
        printf("Run: %s [vpn-IP] [vpn-port] [server-IP] [server-port]", argv[0]);
        return -1;
    }

    int sd_tunnel, sd_server;
    int port_tunnel, port_server;
    int recv_len;
    struct sockaddr_in addr_tunnel;
    struct sockaddr_in addr_server;
    char buffer[MAX_BUFF + 1];

    struct in_addr host_addr;
    struct hostent *ipv4address;


    socklen_t addrsize = sizeof(addr_tunnel);
    port_tunnel = atoi(argv[2]);
    port_server = atoi(argv[4]);

    // set socket to receive info from client
    memset(&addr_tunnel, 0, sizeof(addr_tunnel));
    addr_tunnel.sin_family = AF_INET;
    addr_tunnel.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_tunnel.sin_port = htons(port_tunnel);

    // set socket to send info to server
    memset(&addr_server, 0, sizeof(addr_server));
    addr_server.sin_family = AF_INET;
    bcopy(ipv4address->h_addr, &(addr_server.sin_addr.s_addr), ipv4address->h_length);
    addr_server.sin_port = htons(port_server);

    // set address to forward message to final server
    inet_pton(AF_INET, argv[3], &host_addr);
    ipv4address = gethostbyaddr(&host_addr, sizeof host_addr, AF_INET);
    if (!ipv4address) {
        perror("unknown address");
        return -1;
    }


    // create socket for tunnel
    if ((sd_tunnel = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket error");
        return -1;
    }

    // bind tunnel information to the socket
    if ((bind(sd_tunnel, (struct sockaddr *)&addr_tunnel, sizeof(addr_tunnel))) < 0) {
        perror("bind error");
        return -1;
    }

    // create socket for final server
    if ((sd_server = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket error");
        return -1;
    }

    // listen to requests from the client
    while(1) {
        // receive message from client
        recv_len = recvfrom(sd_tunnel, buffer, sizeof(buffer), 0, (struct sockaddr *) &addr_tunnel, &addrsize);
        if (recv_len > 0) {
            buffer[recv_len] = '\0';
            // tunnel client message to server
            sendto(sd_server, buffer, strlen(buffer), 0, (struct sockaddr *) &addr_server, sizeof(addr_server));
            // receive message from server
            recv_len = recvfrom(sd_server, buffer, sizeof(buffer), 0, (struct sockaddr *) &addr_server, &addrsize);
            if (recv_len > 0) {
                buffer[recv_len] = '\0';
                // tunnel server response to client
                sendto(sd_tunnel, buffer, strlen(buffer), 0, (struct sockaddr *) &addr_tunnel, sizeof(addr_tunnel));
            }
        }
    }


}
