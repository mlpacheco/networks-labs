#include <sys/socket.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <signal.h>

#define ETHERNET_HEADER 22
#define ETHERNET_TRAILER 4
#define UPD_HEADER 8

int main(int argc, char *argv[]) {

    // check that we have all needed params
    if (argc != 6) {
        printf("Run: %s [IPaddress] [port] [payload_size] [packet_count] [packet_spacing]\n", argv[0]);
        return -1;
    }

    // variables needed
    int port, sd, payload_size, n_packets, len, bytes_sent;
    struct hostent *ipv4address;
    struct in_addr host_addr;
    struct sockaddr_in server_addr;
    struct timeval start_time, end_time;
    double interval, start_sec, end_sec, elapsed_sec;

    // parse input to appropiate types
    port = atoi(argv[2]);
    payload_size = atoi(argv[3]);
    n_packets = atoi(argv[4]);
    interval = atof(argv[5]);

    // the message of the size of the payload
    char msg[payload_size + 1];

    // resolve IP address
    inet_pton(AF_INET, argv[1], &host_addr);
    ipv4address = gethostbyaddr(&host_addr, sizeof host_addr, AF_INET);
    if (!ipv4address) {
        perror("unknown address");
        return -1;
    }

    // create socket
    if ((sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("socket error");
        return -1;
    }

    // set server info
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    bcopy(ipv4address->h_addr, &(server_addr.sin_addr.s_addr), ipv4address->h_length);
    server_addr.sin_port = htons(port);

    // create payload
    while((strlen(msg)) < payload_size) {
        len = strlen(msg);
        msg[len] = 'P';
        msg[len + 1] = '\0';
    }

    // send the specified number of packets with specified interval
    bytes_sent = 0;
    gettimeofday(&start_time, 0);
    int i;
    for (i = 0; i < n_packets; i++) {
        sendto(sd, msg, strlen(msg), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
        // add payload
        bytes_sent += strlen(msg);
        // add header/trailer overhead
        bytes_sent += UPD_HEADER + ETHERNET_HEADER + ETHERNET_TRAILER;

        // wait the specified interval
        usleep(interval);
    }
    gettimeofday(&end_time, 0);

    // calculate time spent
    start_sec = ((start_time.tv_sec) * 1000.0 + (start_time.tv_usec) / 1000.0)/1000.0 ;
    end_sec = ((end_time.tv_sec) * 1000.0 + (end_time.tv_usec) / 1000.0)/1000.0 ;
    elapsed_sec = end_sec - start_sec;

    // convert bytes to Megabits to report bit rate
    double sent_Mb = bytes_sent / 131072;


    // output info
    printf("sent=%d bytes (with overhead) | time=%f sec | bit_rate=%f Mbps\n",
            bytes_sent, elapsed_sec, (1.0 * sent_Mb)/elapsed_sec);
    printf("sent=%d packets | time=%f sec | bit_rate=%f pps\n",
            n_packets, elapsed_sec, (1.0 * n_packets)/elapsed_sec);

    // send 3 packets each of payload size 3 bytes back-to-back
    // to signal end of transmission
    for (i = 0; i < 3; i++) {
        sendto(sd, msg, 3,  0, (struct sockaddr *) &server_addr, sizeof(server_addr));
    }

    close(sd);

}

