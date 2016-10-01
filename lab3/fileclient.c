#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BUFF 500

int main(int argc, char *argv[]) {

     // check that we have all needed params
    if (argc != 6) {
        printf("Run: %s [hostname] [portnumber] [secretkey] [filename] [configfile]\n", argv[0]);
        return -1;
    }

    // check that secret key has the appropriate format
    if (strlen(argv[3]) < 10 || strlen(argv[3]) > 20) {
        perror("Secret key should be of length [10,20]");
        return -1;
    }

    // check if file already exists
    if (access(argv[4], F_OK) != -1) {
        perror("File already exists");
        return -1;
    }

    int sd, port, num_bytes, bytes_read, write_smth;
    FILE *f_dwnld;
    FILE *f_config;
    struct sockaddr_in server_addr;
    struct hostent *ipv4address;
    char message[MAX_BUFF + 1];
    char buffer[MAX_BUFF + 1];

    // get port and host
    port = atoi(argv[2]);
    ipv4address = gethostbyname(argv[1]);
    if (!ipv4address) {
        perror("unknown host");
        return -1;
    }

    // set server info
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    bcopy ( ipv4address->h_addr, &(server_addr.sin_addr.s_addr), ipv4address->h_length);
    server_addr.sin_port = htons(port);

    // create socket
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        return -1;
    }

    // connect to socket
    if (connect(sd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect error");
        close(sd);
        return -1;
    }

    // create the message to be sent
    snprintf(message, sizeof(message), "$%s$%s", argv[3], argv[4]);
    write_smth = write(sd, message, strlen(message));

    // read configuration file to know bytes to read
    f_config = fopen(argv[5], "r");
    fscanf(f_config, "%s", buffer);
    num_bytes = atoi(buffer);

    // open file to write downloaded contents
    f_dwnld = fopen(argv[4], "ab");

    // write the message and read the file
    while ((bytes_read = read(sd, buffer, num_bytes)) > 0) {
        buffer[bytes_read] = '\0';
        fputs(buffer, f_dwnld);
    }

    fclose(f_dwnld);

}
