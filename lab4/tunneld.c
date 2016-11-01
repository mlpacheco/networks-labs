#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <signal.h>
#include <string.h>

#define MAX_BUFF 2000

// signal to deal with concurrency
void child_sig_handler(int signum) {
    int status;
    wait(&status);
}

// alarm handler to continue when the server
// doesnt respond
void alarm_sig_handler(int signum) {
    //
}

// tunneling procedure
int tunnel(char * server_IP, char * server_port, int port_client) {
    int sd_server, port_server;
    int sd_client;
    int num_bytes;

    struct sockaddr_in addr_client;
    struct sockaddr_in addr_server;

    struct in_addr host_addr;
    struct hostent *ipv4address;

    char buffer[MAX_BUFF + 1];

    socklen_t addrsize = sizeof(addr_client);
    port_server = atoi(server_port);

    // set info needed to listen to client
    memset(&addr_client, 0, sizeof(addr_client));
    addr_client.sin_family = AF_INET;
    addr_client.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_client.sin_port = htons(port_client);

    if ((sd_client = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket error");
        return -1;
    }

    if ((bind(sd_client, (struct sockaddr *)&addr_client,
            sizeof(addr_client))) < 0) {
        perror("bind error");
        return -1;
    }

    // set info to tunnel to server
    inet_pton(AF_INET, server_IP, &host_addr);
    ipv4address = gethostbyaddr(&host_addr, sizeof host_addr, AF_INET);
    if (!ipv4address) {
        perror("unknown address");
        return -1;
    }

    memset(&addr_server, 0, sizeof(addr_server));
    addr_server.sin_family = AF_INET;
    bcopy(ipv4address->h_addr, &(addr_server.sin_addr.s_addr),
          ipv4address->h_length);
    addr_server.sin_port = htons(port_server);

    if ((sd_server = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket error");
        return -1;
    }

    // stuff to handle signals
    struct sigaction act;
    memset(&act, 0, sizeof (act));
    act.sa_handler = alarm_sig_handler;
    sigaction (SIGALRM, &act, 0);

    // listen to client
    num_bytes = recvfrom(sd_client, buffer, sizeof(buffer), 0,
                (struct sockaddr *) &addr_client, &addrsize);
    while(num_bytes > 0) {
        buffer[num_bytes] = '\0';

        sendto(sd_server, buffer, strlen(buffer), 0,
              (struct sockaddr *)&addr_server, sizeof(addr_server));

        // recv from server / send to client
        alarm(1);
        num_bytes = recvfrom(sd_server, buffer, strlen(buffer), 0,
                  (struct sockaddr *)&addr_server, &addrsize);
        alarm(0);

        if (num_bytes > 0) {
            buffer[num_bytes] = '\0';
            sendto(sd_client, buffer, strlen(buffer), 0,
                  (struct sockaddr *)&addr_client, sizeof(addr_client));
        }

        alarm(1);
        num_bytes = recvfrom(sd_client, buffer, sizeof(buffer), 0,
                (struct sockaddr *) &addr_client, &addrsize);
        alarm(0);
    }

    close(sd_client);
    close(sd_server);
}

int main(int argc, char *argv[]) {

    // check that we have all needed params
    if (argc != 2) {
        printf("Run: %s [vpn-port_vpnclient]\n", argv[0]);
        return -1;
    }

    pid_t k;
    int sd_vpnclient, port_vpnclient, n_bytes;
    struct sockaddr_in addr_vpnclient;
    char buffer[MAX_BUFF + 1];
    char msg[MAX_BUFF + 1];

    char * server_port;
    char * server_IP;

    socklen_t addrsize = sizeof(addr_vpnclient);
    port_vpnclient = atoi(argv[1]);

    // stuff to handle concurrency signals
    struct sigaction sigchld_action;
    memset (&sigchld_action, 0, sizeof (sigchld_action));
    sigchld_action.sa_handler = child_sig_handler;
    sigaction (SIGCHLD, &sigchld_action, NULL);

    // set info
    memset(&addr_vpnclient, 0, sizeof(addr_vpnclient));
    addr_vpnclient.sin_family = AF_INET;
    addr_vpnclient.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_vpnclient.sin_port = htons(port_vpnclient);

    // create socket
    if ((sd_vpnclient = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket error");
        return -1;
    }

    // bind information to the socket
    if ((bind(sd_vpnclient, (struct sockaddr *)&addr_vpnclient,
              sizeof(addr_vpnclient))) < 0) {
        perror("bind error");
        return -1;
    }

    int num_clients = 0;
    while(1) {
        // receive client request through tunnel request
        n_bytes = recvfrom(sd_vpnclient, buffer, sizeof(buffer), 0,
                          (struct sockaddr *) &addr_vpnclient, &addrsize);

        // fork a process to deal with this VPN client and actual client
        // this is implemented to allow multiple clients to use this proxy
        if (n_bytes > 0) {

            k = fork();

            if (k == 0) {
                buffer[n_bytes] = '\0';
                // read data
                server_IP = strtok(buffer, "$");
                server_port = strtok(NULL, "$");
                // allocate an arbitrary port number
                snprintf(msg, sizeof(msg), "$%d$%s", 12345 + num_clients,
                         server_IP);
                // send second port to VPN client
                sendto(sd_vpnclient, msg, strlen(msg), 0,
                      (struct sockaddr *) &addr_vpnclient,
                      sizeof(addr_vpnclient));
                // proceed with tunneling
                tunnel(server_IP, server_port, 12345 + num_clients);
                exit(1);

            } else {
                num_clients++;
            }
      }
    }


}
