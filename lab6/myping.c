#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <signal.h>

#define MAX_BUFF 1000

// signal handler to terminate client if no response
void signal_handler (int x) {
    printf("no response from ping server\n");
    exit(-1);
}

int main(int argc, char *argv[]) {

    // check that we have all needed params
    if (argc != 5) {
        printf("Run: %s [IPaddress] [port] [secretkey] [my_port]\n", argv[0]);
        return -1;
    }

    // check that secret key has the appropiate format
    if (strlen(argv[3]) < 10 || strlen(argv[3]) > 20) {
        printf("Secret key should be of length [10,20]\n");
        return -1;
    }

    int sd, port, recv_len, my_port;
    struct sockaddr_in server_addr, my_addr;
    struct in_addr host_addr;
    struct hostent *ipv4address;
    char message[MAX_BUFF + 1];
    char buffer[MAX_BUFF + 1];
    struct timeval start_time, end_time;
    double start_millis, end_millis, elapsed_millis;

    // to stuff handle signals
    struct sigaction act;
    act.sa_handler = signal_handler;
    sigaction (SIGALRM, &act, 0);

    socklen_t addrsize = sizeof(server_addr);
    char * rndm_pool = "ABCDEFGHIJKLMNOPQRSTUVWXYZIabcdefghijklmnopqrstuvwxyz0123456789";
    port = atoi(argv[2]);
    my_port = atoi(argv[4]);

    inet_pton(AF_INET, argv[1], &host_addr);
    ipv4address = gethostbyaddr(&host_addr, sizeof host_addr, AF_INET);
    if (!ipv4address) {
        perror("unknown address");
        return -1;
    }

     // create the message
    sprintf(message, "$%s$", argv[3]);

    // create pad and append to the message
    while((strlen(message)) < 1000) {
        char rndm_char = rndm_pool[random() % strlen(rndm_pool)];
        int len = strlen(message);
        message[len] = rndm_char;
        message[len + 1] = '\0';
    }

    // create socket
    if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket error");
        return -1;
    }

    // set server info
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    bcopy(ipv4address->h_addr, &(server_addr.sin_addr.s_addr), ipv4address->h_length);
    server_addr.sin_port = htons(port);

    // set my info
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    my_addr.sin_port = htons(my_port);

    // force source port to be the specified one
    if (bind(sd, (struct sockaddr *) &my_addr, sizeof(my_addr)) < 0) {
        perror("bind error");
    }

    // set alarm
    ualarm(255000, 0);

    // send and receive packages, take time at both ends
    gettimeofday(&start_time, 0);
    sendto(sd, message, strlen(message), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
    recv_len = recvfrom(sd, buffer, sizeof(buffer), 0, (struct sockaddr *) &server_addr, &addrsize);
    gettimeofday(&end_time, 0);

    // calculate milliseconds
    start_millis = (start_time.tv_sec) * 1000.0 + (start_time.tv_usec) / 1000.0 ;
    end_millis = (end_time.tv_sec) * 1000.0 + (end_time.tv_usec) / 1000.0 ;
    elapsed_millis = end_millis - start_millis;

    if (recv_len > 0) {
        buffer[recv_len] = '\0';
        // if we get the expected message, print ping info
        if (strcmp("terve", buffer) == 0) {
            printf("ip_address=%s port=%d time=%f ms\n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port), elapsed_millis);
        }
    }
}
