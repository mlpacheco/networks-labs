#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PORT 10001
#define MAX_BUFF 500

// signal handler to not block the server waiting
void childsig_handler(int signum) {
    int status;
    wait(&status);
}

int main(int argc, char *argv[]) {

    if (argc != 3) {
        printf("Run: %s [portnumber] [secretkey]", argv[0]);
        return -1;
    }

    pid_t k;
    struct sockaddr_in addr; //socket info about the client
    int sd, recv_len, port;
    char buffer[MAX_BUFF + 1];

    port = atoi(argv[1]);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    sd = socket(AF_INET, SOCK_STREAM, 0);

    // bind server information to the server socket
    bind(sd, (struct sockaddr *)&addr, sizeof(addr));

    // start listening
    listen(sd, 5);

    // stuff needed to handle the signals
    struct sigaction sigchld_action;
    memset (&sigchld_action, 0, sizeof (sigchld_action));
    sigchld_action.sa_handler = &childsig_handler;
    sigaction (SIGCHLD, &sigchld_action, NULL);

    while(1) {
        int conn = accept(sd, 0, 0);
        int read_smth = read(conn, buffer, MAX_BUFF);
        if (read_smth > 0) {
            buffer[read_smth] = '\0';
            k = fork();
            if (k == 0) {
                dup2(conn, 1);
                if (execlp(buffer, buffer, NULL) == -1) {
                    exit(-1);
                }
            }
        }
        close(conn);
    }

}
