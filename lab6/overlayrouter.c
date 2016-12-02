#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>

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
    port_server = atoi(argv[1]);

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
                char last_router[MAX_BUFF + 1];
                char * ptr;
                int beg_ip, count;
                struct ifreq ifr;
                char iface[] = "eth0";
                char * localaddr;

                ifr.ifr_addr.sa_family = AF_INET;
                strncpy(ifr.ifr_name , iface , IFNAMSIZ-1);
                ioctl(sd_server, SIOCGIFADDR, &ifr);
                localaddr = inet_ntoa(( (struct sockaddr_in *)&ifr.ifr_addr )->sin_addr);

                printf("local address: %s\n", localaddr);

                buffer[n_bytes] = '\0';
                printf("%s\n", buffer);

                buffer[n_bytes - 1] = '\0';
                printf("%s\n", buffer);

                ptr = strrchr(buffer, '$');
                beg_ip = ptr - buffer + 1;
                printf("beg_ip: %d\n", beg_ip);

                memcpy(last_router, &buffer[beg_ip], strlen(buffer) - beg_ip);
                last_router[strlen(buffer) - beg_ip] = '\0';
                printf("last_router=%s\n", last_router);

                if (strcmp(last_router, localaddr) == 0) {

                    // first I need to return the ACK
                    num_clients++;
                    snprintf(message, sizeof(message), "%d", port_server + num_clients);
                    sendto(sd_server, message, strlen(message), 0,
                           (struct sockaddr *)&addr_server, sizeof(addr_server));

                    printf("ACK: %s\n", message);

                    // now I need to forward to the next in line
                    memcpy(message, &buffer[0], strlen(buffer) - strlen(last_router));
                    message[strlen(buffer) - strlen(last_router)] = '\0';
                    printf("message: %s, length: %lu\n", message, strlen(message));

                    // count separators to see if we are at the end of the chain
                    int i;
                    for(i = 0; i < strlen(message); i++) {
                        if(message[i] == '$') {
                            count++;
                        }
                    }

                    printf("count of $: %d\n", count);
                    if (count <= 3) {
                        // i am the last router
                    } else {
                        memcpy(buffer, &message[0], strlen(message));
                        buffer[strlen(message) - 1] = '\0';
                        ptr = strrchr(buffer, '$');
                        beg_ip = ptr - buffer + 1;
                        printf("beg_ip: %d\n", beg_ip);

                        memcpy(last_router, &buffer[beg_ip], strlen(buffer) - beg_ip);
                        last_router[strlen(buffer) - beg_ip] = '\0';
                        printf("next_router=%s\n", last_router);

                        
                    }

                }
            }
        }

    }


}
