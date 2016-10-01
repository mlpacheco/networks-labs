#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BUFF 500

// signal handler to not block the server waiting
// for child processes
void childsig_handler(int signum) {
    int status;
    wait(&status);
}

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
    char filepath[MAX_BUFF + 1];

    FILE *f_dwnld;
    FILE *f_config;

    port = atoi(argv[1]);

    // set server info
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

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
    listen(sd, 5);

    // stuff needed to handle the signals
    struct sigaction sigchld_action;
    memset (&sigchld_action, 0, sizeof (sigchld_action));
    sigchld_action.sa_handler = &childsig_handler;
    sigaction (SIGCHLD, &sigchld_action, NULL);

    // accept connections
    while(1) {
        conn = accept(sd, 0, 0);
        k = fork();

        if (k == 0) {
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
                f_dwnld = fopen(filepath, "rb");
                if (f_dwnld != NULL) {
                    while((bytes_read = fread(buffer, 1, num_bytes, f_dwnld)) > 0) {
                        buffer[bytes_read] = '\0';
                        write(conn, buffer, bytes_read);
                    }
                }
            }

            close(conn);
            exit(0);

        } else {
            close(conn);
        }

    }

    close(sd);

}
