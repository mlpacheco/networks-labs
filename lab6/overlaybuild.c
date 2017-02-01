#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

#define MAX_BUFF 2000

int main(int argc, char *argv[]) {

    // check that we have all needed params
    if (argc < 5) {
        printf("Run: %s [dst-IP] [dst-port] [routerk-IP, ...] [overlay-port] [build-port]\n", argv[0]);
        return -1;
    }

    int sd_router1, port_router1, n_bytes, my_port;
    struct sockaddr_in addr_router1, addr_my;
    char buffer[MAX_BUFF + 1];
    char message[MAX_BUFF + 1];
    struct in_addr host_addr;
    struct hostent *ipv4address;

    my_port = atoi(argv[argc - 1]);

    inet_pton(AF_INET, argv[argc - 3], &host_addr);
    printf("Router1 Hostname: %s\n", argv[argc - 3]);
    ipv4address = gethostbyaddr(&host_addr, sizeof host_addr, AF_INET);
    if (!ipv4address) {
        perror("unknown address");
        return -1;
    }

    socklen_t addrsize = sizeof(addr_router1);
    printf("Router1 port: %s\n", argv[argc - 2]);
    port_router1 = atoi(argv[argc - 2]);

    // set router1 info
    memset(&addr_router1, 0, sizeof(addr_router1));
    addr_router1.sin_family = AF_INET;
    bcopy(ipv4address->h_addr, &(addr_router1.sin_addr.s_addr), ipv4address->h_length);
    addr_router1.sin_port = htons(port_router1);

    // set my info
    memset(&addr_my, 0, sizeof(addr_my));
    addr_my.sin_family = AF_INET;
    addr_my.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_my.sin_port = htons(my_port);

    if ((sd_router1 = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket error");
        return -1;
    }

    if (bind(sd_router1, (struct sockaddr *) &addr_my, sizeof(addr_my)) < 0) {
        perror("bind error");
        return -1;
    }

    message[0] = '\0';
    snprintf(buffer, sizeof(buffer), "$%s$%s$", argv[1], argv[2]);
    strcat(message, buffer);

    int i;
    for (i = 3; i < argc - 2; i++) {
        snprintf(buffer, sizeof(buffer), "%s$", argv[i]);
        strcat(message, buffer);
    }

    printf("%s\n", message);

    n_bytes = sendto(sd_router1, message, strlen(message), 0, (struct sockaddr *) &addr_router1,
                    sizeof(addr_router1));
    printf("Sent: %d bytes\n", n_bytes);

    n_bytes = recvfrom(sd_router1, buffer, sizeof(buffer), 0,
                       (struct sockaddr *) &addr_router1, &addrsize);

    if (n_bytes > 0) {
        buffer[n_bytes] = '\0';
        printf("%s\n", buffer);
    }

}
