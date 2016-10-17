#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define MAX_BUFF 10000

#define UDP_HEADER 8
#define ETHERNET_HEADER 22
#define ETHERNET_TRAILER 4

int main(int argc, char *argv[]) {

    // check that we have all needed params
    if (argc != 2) {
        printf("Run: %s [port]\n", argv[0]);
        return -1;
    }

    // variables that we will need
    int sd, port, recv_len, n_packets, n_bytes, first_pkg;
    struct sockaddr_in addr;
    char buffer[MAX_BUFF + 1];
    struct timeval start_time, end_time;
    double interval, start_sec, end_sec, elapsed_sec;


    socklen_t addrsize = sizeof(addr);
    port = atoi(argv[1]);

    // set server info
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    // create socket
    if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket error");
        return -1;
    }

    // bind server information to the socket
    if ((bind(sd, (struct sockaddr *)&addr, sizeof(addr))) < 0) {
        perror("bind error");
        return -1;
    }

    first_pkg = 1;
    n_bytes = 0;
    n_packets = 0;
    // need to add UDP header and Ethernet header/trailer overhead to received bytes
    while (1) {
        // receive a package from the sender
        recv_len = recvfrom(sd, buffer, sizeof(buffer), 0, (struct sockaddr *) &addr, &addrsize);

        // take the time after receiving first packet
        if (first_pkg) {
            gettimeofday(&start_time, 0);
            first_pkg = 0;
        }

        // server has signaled the end of transmission, capture time and break
        if (recv_len == 3) {
            gettimeofday(&end_time, 0);
            close(sd);
            break;
        } else {
            n_bytes += recv_len;
            // add header/trailer overhead
            n_bytes += UDP_HEADER + ETHERNET_HEADER + ETHERNET_TRAILER;
            n_packets += 1;
        }

    }

    // estimate duration
    start_sec = ((start_time.tv_sec) * 1000.0 + (start_time.tv_usec) / 1000.0)/1000.0 ;
    end_sec = ((end_time.tv_sec) * 1000.0 + (end_time.tv_usec) / 1000.0)/1000.0 ;
    elapsed_sec = end_sec - start_sec;

    double recv_Mb = n_bytes / 131072;

    // print outputs
    printf("received=%d bytes (with overhead) | time=%f sec | bit_rate=%f Mbps\n",
            n_bytes, elapsed_sec, (1.0 * recv_Mb)/elapsed_sec);
    printf("received=%d packets | time=%f sec | bit_rate=%f pps\n",
            n_packets, elapsed_sec, (1.0 * n_packets)/elapsed_sec);

}
