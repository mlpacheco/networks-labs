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
    if (argc != 3) {
        printf("Run: %s [portnumber] [secretkey]\n", argv[0]);
        return -1;
    }

    // check that secret key has the appropriate format
    if (strlen(argv[2]) < 10 || strlen(argv[2]) > 20) {
        printf("Secret key should be of length [10,20]\n");
        return -1;
    }

    pid_t k;
    struct sockaddr_in addr;
    int sd, recv_len, port, conn, read_smth;
    char buffer[MAX_BUFF + 1];
    char * secretkey;
    char * cmd;

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

    while(1) {
        // accept connections and read command
        conn = accept(sd, 0, 0);
        read_smth = read(conn, buffer, MAX_BUFF);

        // if something is read, fork a process to handle the request
        if (read_smth > 0) {
            buffer[read_smth] = '\0';
            k = fork();

            // child process code
            if (k == 0) {
                // break down the message into secretkey and command
                secretkey = strtok(buffer, "$");
                cmd = strtok(NULL, "$");

                // ignore packages that have a different secretkey
                if (strcmp(secretkey, argv[2]) != 0) {
                    close(conn);
                    exit(-1);
                }

                // check if the command is permitted, if so, execute & redirect output
                // if not, inform the client
                if (strcmp("ls", cmd) != 0 && strcmp("date", cmd) != 0 &&\
                    strcmp("host", cmd) != 0 && strcmp("cal", cmd) != 0) {
                    char * error = "command not accepted, use: [ls|date|host|cal]\n";
                    write(conn, error, strlen(error));
                    close(conn);
                    exit(-1);
                } else {
                    dup2(conn, 1);
                    if (execlp(cmd, cmd, NULL) == -1) {
                        close(conn);
                        exit(-1);
                    }
                }
            }
        } else {
            // if nothing was read, close conn and go back to wait for new connections
            close(conn);
        }

    }

}
