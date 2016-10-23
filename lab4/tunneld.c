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
    if (argc != 2) {
        printf("Run: %s [vpn-port]", argv[0]);
        return -1;
    }

    int sd, port, recv_len;
    struct sockaddr_in addr;
    char buffer[MAX_BUFF + 1];

    socklen_t addrsize = sizeof(addr);
    port = atoi(argv[1]);

    // set server info
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    // create socket
    if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket error");
        return -1;
    }

    // bind server information to the socket
    if ((bind(sd, (struct sockaddr *)&addr, sizeof(addr))) < 0) {
        perror("bind error");
        return -1;
    }

    while(1) {
        // receive client request through tunnel request
        recv_len = recvfrom(sd, buffer, sizeof(buffer), 0, (struct sockaddr *) &addr, &addrsize);

        if (recv_len > 0) {
            buffer[recv_len] = '\0';
            sendto(sd, "ACK", 3, 0, (struct sockaddr *) &addr, sizeof(addr));
        }
    }


}
