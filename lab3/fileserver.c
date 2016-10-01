#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BUFF 500

int main(int argc, char *argv[]) {

    // check that we have all needed params
    if (argc != 4) {
        printf("Run: %s [portnumber] [secretkey] [configfile]\n", argv[0]);
        return -1;
    }

    // check that secret key has the appropriate format
    if (strlen(argv[2]) < 10 || strlen(argv[2]) > 20) {
        printf("Secret key should be of length [10,20]\n");
        return -1;
    }

    pid_t k;
    struct sockaddr_in addr;
    int sd, port, conn, read_smth, bytes_read, num_bytes;
    char buffer[MAX_BUFF + 1];
    char * secretkey;
    char * filename;
    char * filepath;

    FILE *f_dwnld;
    FILE *f_config;

    port = atoi(argv[1]);

    printf("port %d\n", port);

    // create socket
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        return -1;
    }

    // bind server information to the server socket
    if ((bind(sd, (struct sockaddr *)&addr, sizeof(addr))) < 0) {
        perror("bind error");
        return -1;
    }

    // start listening
    printf("listening...\n");
    listen(sd, 5);

    printf("accepting connections...\n");
    conn = accept(sd, 0, 0);
    read_smth = read(conn, buffer, MAX_BUFF);

    if (read_smth > 0) {
        buffer[read_smth] = '\0';
        secretkey = strtok(buffer, "$");
        filename = strtok(NULL, "$");

        snprintf(filepath, sizeof(filepath), "%s%s", "./filedeposit/", filename);

        // ignore packages that have a different secretkey
        if (strcmp(secretkey, argv[2]) != 0) {
            close(conn);
            exit(-1);
        }

        // read configuration file to know bytes to write
        f_config = fopen(argv[3], "r");
        fscanf(f_config, "%s", buffer);
        num_bytes = atoi(buffer);

        // open file to read content to send
        f_dwnld = fopen(argv[4], "rb");
        if (f_dwnld != NULL) {
            while((bytes_read = fread(buffer, 1, num_bytes, f_dwnld)) > 0) {
                buffer[bytes_read] = '\0';
                write(conn, buffer, bytes_read);
            }
        }
        close(conn);

    }

    close(sd);


}
