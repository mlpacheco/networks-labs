#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#define MAX_BUFF 2000

// signal to deal with concurrency
void child_sig_handler(int signum) {
    int status;
    wait(&status);
}

int main(int argc, char *argv[]) {

    // check that we have all needed params
    if (argc != 2) {
        printf("Run: %s [server-port]\n", argv[0]);
        return -1;
    }

    pid_t k;
    int sd_server, port_server, n_bytes;
    struct sockaddr_in addr_server;
    char buffer[MAX_BUFF + 1];
    char message[MAX_BUFF + 1];

    socklen_t addrsize = sizeof(addr_server);

    // stuff to handle concurrency signals
    struct sigaction sigchld_action;
    memset (&sigchld_action, 0, sizeof (sigchld_action));
    sigchld_action.sa_handler = child_sig_handler;
    sigaction (SIGCHLD, &sigchld_action, NULL);

    // set info
    memset(&addr_server, 0, sizeof(addrsize));
    addr_server.sin_family = AF_INET;
    addr_server.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_server.sin_port = htons(port_server);

    // create socket
    if ((sd_server = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket error");
        return -1;
    }

    // bind information to the socket
    if ((bind(sd_server, (struct sockaddr *)&addr_server,
              sizeof(addr_server))) < 0) {
        perror("bind error");
        return -1;
    }

    int num_clients = 0;
    while(1) {
        // receive client request through tunnel request
        n_bytes = recvfrom(sd_server, buffer, sizeof(buffer), 0,
                          (struct sockaddr *) &addr_server, &addrsize);


        // fork a process to deal with this client
        // this is implemented to allow multiple clients to use this router
        if (n_bytes > 0) {

            k = fork();

            if (k == 0) {
                buffer[n_bytes] = '\0';
                // read data
            }

        }

    }


}
