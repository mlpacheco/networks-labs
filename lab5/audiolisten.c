#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_BUFF 1000

int main(int argc, char *argv[]) {

     if (argc != 11) {
        printf("Run: %s %s %s %s %s %s %s %s %s %s %s\n", argv[0],
               "[server-ip]", "[server-tcp-port]", "[client-udp-port]",
               "[payload-size]", "[playback-del]", "[gamma]", "[buf-sz]",
               "[target-buf]", "[logfile-c]", "[filename]");
        return -1;
    }

    int tcp_server_sd, tcp_server_port;
    struct sockaddr_in tcp_server_addr;
    struct in_addr host_addr;
    struct hostent *server_ipv4_addr;
    char message[MAX_BUFF + 1];
    char buffer[MAX_BUFF + 1];

    int num_bytes;

    tcp_server_port = atoi(argv[2]);
    printf("IP address %s\n", argv[1]);
    inet_pton(AF_INET, argv[1], &host_addr);
    server_ipv4_addr =
        gethostbyaddr(&host_addr, sizeof host_addr, AF_INET);
    if (!server_ipv4_addr) {
        perror("unknown IP address");
        return -1;
    }

    // set info to send message
    memset(&tcp_server_addr, 0, sizeof tcp_server_addr);
    tcp_server_addr.sin_family = AF_INET;
    bcopy(server_ipv4_addr->h_addr, &(tcp_server_addr.sin_addr.s_addr),
          server_ipv4_addr->h_length);
    tcp_server_addr.sin_port = htons(tcp_server_port);


    // create tcp socket
    if ((tcp_server_sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        return -1;
    }

    // connect to tcp socket
    if (connect(tcp_server_sd, (struct sockaddr *)&tcp_server_addr,
                sizeof tcp_server_addr) < 0) {
        perror("connect error");
        close(tcp_server_sd);
        return -1;
    }

    // create message
    sprintf(message, "%s %s", argv[3], argv[10]);
    printf("Message: %s\n", message);

    write(tcp_server_sd, message, strlen(message));
    num_bytes = read(tcp_server_sd, buffer, MAX_BUFF);
    if (num_bytes > 0) {
        buffer[num_bytes] = '\0';
        printf("Received: %s\n", buffer);
    }

    close(tcp_server_sd);

}
