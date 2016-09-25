#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BUFF 500
#define CMD "date"

int main(int argc, char *argv[]) {

    if (argc != 4) {
        printf("Run: %s [hostname] [portnumber] [secretkey]\n", argv[0]);
        return -1;
    }

    if (strlen(argv[3]) < 10 || strlen(argv[3]) > 20) {
        printf("Secret key should be of length [10,20]\n");
        return -1;
    }

    char buffer[MAX_BUFF + 1];
    int sd, n_recv, port;
    struct sockaddr_in server_addr;
    struct hostent *ipv4address;
    char message[MAX_BUFF + 1];

    port = atoi(argv[2]);
    ipv4address = gethostbyname(argv[1]);

    // create the message
    snprintf(message, sizeof(message), "$%s$%s", argv[3], CMD);

    sd = socket(AF_INET, SOCK_STREAM, 0);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    bcopy ( ipv4address->h_addr, &(server_addr.sin_addr.s_addr), ipv4address->h_length);
    server_addr.sin_port = htons(port);

    connect(sd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    int write_smth = write(sd, message, strlen(message));
    int read_smth = read(sd, buffer, MAX_BUFF);
    if (read_smth > 0) {
        buffer[read_smth] = '\0';
        printf("%s", buffer);
    }
    close(sd);

}
