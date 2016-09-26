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
    if (argc != 3) {
        printf("Run: %s [port] [secretkey]\n", argv[0]);
        return -1;
    }

    // check that secret key has the appropiate format
    if (strlen(argv[2]) < 10 || strlen(argv[2]) > 20) {
        printf("Secret key should be of length [10,20]\n");
        return -1;
    }

    int sd, port, recv_len, msg_count;
    struct sockaddr_in addr;
    char buffer[MAX_BUFF + 1];
    char * secretkey;

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

    // set a counter to identify every 4th request
    msg_count = 1;

    while (1) {
        // receive client request
        recv_len = recvfrom(sd, buffer, sizeof(buffer), 0, (struct sockaddr *) &addr, &addrsize);

        // ignore every 4th client request
        if (msg_count == 4) {
            msg_count = 1;
            continue;
        }

        if (recv_len > 0) {
            buffer[recv_len] = '\0';
            int len = strlen(buffer);

            // break down the message into secretkey and pad
            secretkey = strtok(buffer, "$");

            // ignore packages that have a different secretkey or the wrong length
            if (strcmp(secretkey, argv[2]) != 0 || len != 1000) {
                continue;
            }

            // send message back to the client
            sendto(sd, "terve", 5, 0, (struct sockaddr *) &addr, sizeof(addr));
        }

        msg_count++;

    }

}
