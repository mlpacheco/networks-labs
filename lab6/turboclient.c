#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>
#include <signal.h>

#define MAX_BUFF 10000

// variables needed in signal
int udp_sd;
struct sockaddr_in server_addr_udp;

int payload_sz;
int last_checked = -1;
int last_received = -1;
int * received_packets;
char * window;
size_t received_packets_sz;

void sigalrm_handl_nack(int sig) {
    //printf("signal handler\n");
    int i, j, offset;
    char message[MAX_BUFF + 1];
    char payload[payload_sz + 1];
    for (i = last_checked + 1; i < last_received; i++) {
        if (received_packets[i] == 0) {
            // send NACK
            snprintf(message, sizeof(message), "$NACK$%d", i);
            sendto(udp_sd, message, strlen(message), 0,
                   (struct sockaddr *)&server_addr_udp, sizeof(server_addr_udp));
            printf("Sent: %s\n", message);
            last_checked = i;
        }
    }
}

int received_all() {
    int i;
    for (i = 0; i < received_packets_sz; i++) {
        if (received_packets[i] == 0) {
            return 0;
        }
    }
    return 1;
}

// this had to be modified according to protocol
int receive_file(char * filepath, int num_bytes, socklen_t addrlen) {
    FILE *f_dwnld;
    struct timeval start_time, end_time;
    char buffer[MAX_BUFF + 1];
    char message[MAX_BUFF + 1];
    int bytes_read, total_read, seqnumber, total_bytes, offset, prev_seqnumber;
    double start_sec, end_sec, elapsed_sec, reliable_throughput;
    char seqnumber_str[MAX_BUFF + 1];
    char payload[MAX_BUFF + 1];
    char * payload_sz_str;
    char * total_bytes_str;

    //stuff to handle alarm signals
    struct sigaction sa_alarm;
    memset(&sa_alarm, 0, sizeof sa_alarm);
    //sa_alarm.sa_flags = SA_NODEFER;
    sa_alarm.sa_handler = sigalrm_handl_nack;
    sigaction(SIGALRM, &sa_alarm, 0);

    // read from the socket per specified bytes and write to local file
    gettimeofday(&start_time, 0);
    total_read = 0;
    prev_seqnumber = -1;

    // set alarm to check not received packets
    //ualarm(1000, 1000);

    int all = 0;
    while (all == 0) {
        bytes_read = recvfrom(udp_sd, buffer, sizeof(buffer), 0,
                              (struct sockaddr *)&server_addr_udp,
                               &addrlen);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            // parse message
            // get sequence number before '$'
            int sep_pos = strcspn(buffer, "$");
            if (sep_pos > 0) {
                memcpy(seqnumber_str, &buffer[0], sep_pos);
                seqnumber_str[sep_pos] = '\0';
                // get payload after $
                memcpy(payload, &buffer[sep_pos + 1], bytes_read - (sep_pos + 1));
                payload[bytes_read - (sep_pos + 1)] = '\0';

                seqnumber = atoi(seqnumber_str);
                printf("Received seqnumber %d\n", seqnumber);
                if (seqnumber == -1) {
                    // parse command

                    total_bytes_str = strtok(payload, "$");
                    payload_sz_str = strtok(NULL, "$");
                    // transform data
                    total_bytes = atoi(total_bytes_str);
                    payload_sz = atoi(payload_sz_str);
                    // set window and packet tracker
                    window = malloc(total_bytes + 1);
                    received_packets_sz = ceil((total_bytes * 1.0) / payload_sz);
                    printf("number of packets: %zu\n", received_packets_sz);
                    received_packets = malloc(received_packets_sz
                                               * sizeof(int));
                    memset(received_packets, 0, received_packets_sz);
                } else {
                    received_packets[seqnumber] = 1;
                    printf("Payload size: %d, bytes_read: %d\n", bytes_read - (sep_pos + 1), bytes_read);
                    //fputs(payload, f_dwnld);
                    offset = payload_sz * seqnumber;
                    printf("offset: %d\n", offset);
                    int i;
                    for (i = 0; i < bytes_read - (sep_pos + 1); i++) {
                        window[offset + i] = payload[i];
                    }
                    total_read += bytes_read;
                    if (seqnumber > last_received) {
                        last_received = seqnumber;
                    }
                }

                all = received_all();
            }
        }

    }

    // unset alarm and send a message to signal the end
    //ualarm(0,0);
    sendto(udp_sd, "DONE", 4, 0, (struct sockaddr *)&server_addr_udp,
           sizeof server_addr_udp);
    printf("Sent done\n");

    gettimeofday(&end_time, 0);

    // write to file
    printf("%s\n", filepath);
    f_dwnld = fopen(filepath, "wb");
    fwrite(window, 1, total_bytes, f_dwnld);
    fclose(f_dwnld);

    // calculate sececonds we took
    start_sec = ((start_time.tv_sec) * 1000.0 + \
            (start_time.tv_usec)/ 1000.0)/1000.0 ;
    end_sec = ((end_time.tv_sec) * 1000.0 + \
            (end_time.tv_usec) / 1000.0)/1000.0 ;
    elapsed_sec = end_sec - start_sec;

     // transform bytes to Megabits
    double total_Mb = total_read / 131072.0;

    // calculate reliable throughput = msg_len / RTT
    reliable_throughput = total_Mb / elapsed_sec;

    // print results
    printf("read = %d bytes\ntime = %f sec\nreliable_throughput = %f Mbps\n",
            total_read, elapsed_sec, reliable_throughput);

    return 0;
}

int main(int argc, char *argv[]) {

     // check that we have all needed params
    if (argc != 6) {
        printf("Run: %s [hostname] [portnumber] [secretkey] [filename] [configfile]\n",
                argv[0]);
        return -1;
    }

    // check that secret key has the appropriate format
    if (strlen(argv[3]) < 10 || strlen(argv[3]) > 20) {
        perror("Secret key should be of length [10,20]");
        return -1;
    }

    // check if file already exists
    if (access(argv[4], F_OK) != -1) {
        perror("File already exists");
        return -1;
    }

    // all variables that we will need
    int tcp_sd, tcp_port, udp_port, num_bytes, bytes_read;
    struct sockaddr_in server_addr_tcp;
    FILE *f_config;
    struct hostent *ipv4address;
    char message[MAX_BUFF + 1];
    char buffer[MAX_BUFF + 1];

    // get port and host
    tcp_port = atoi(argv[2]);
    ipv4address = gethostbyname(argv[1]);
    if (!ipv4address) {
        perror("unknown host");
        return -1;
    }

    // set server info
    memset(&server_addr_tcp, 0, sizeof(server_addr_tcp));
    server_addr_tcp.sin_family = AF_INET;
    bcopy(ipv4address->h_addr, &(server_addr_tcp.sin_addr.s_addr),
          ipv4address->h_length);
    server_addr_tcp.sin_port = htons(tcp_port);

    // create socket
    if ((tcp_sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        return -1;
    }

    // connect to socket
    if (connect(tcp_sd, (struct sockaddr *)&server_addr_tcp, sizeof(server_addr_tcp)) < 0) {
        perror("connect error");
        close(tcp_sd);
        return -1;
    }

    // create the message to be sent and send it to the server
    // then read port that will be used for UDP transfer
    snprintf(message, sizeof(message), "$%s$%s", argv[3], argv[4]);
    write(tcp_sd, message, strlen(message));
    printf("Sent: %s\n", message);
    bytes_read = read(tcp_sd, buffer, MAX_BUFF);

    // we no longer need this
    close(tcp_sd);

    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        udp_port = atoi(buffer);
        printf("Received: %d\n", udp_port);

        // set server info
        memset(&server_addr_udp, 0, sizeof(server_addr_udp));
        server_addr_udp.sin_family = AF_INET;
        server_addr_udp.sin_addr.s_addr = htonl(INADDR_ANY);
        server_addr_udp.sin_port = htons(udp_port);
        printf("port: %d\n", udp_port);
        // create UDP socket

        if ((udp_sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
            perror("socket error");
            return -1;
        }

        // bind socket to listen to file packets
        if ((bind(udp_sd, (struct sockaddr *)&server_addr_udp, sizeof(server_addr_udp))) < 0) {
            perror("bind error");
            return -1;
        }

        // read configuration file to know how many bytes to read at a time
        f_config = fopen(argv[5], "r");
        fscanf(f_config, "%s", buffer);
        num_bytes = atoi(buffer);
        fclose(f_config);

        receive_file(argv[4], num_bytes, sizeof(server_addr_udp));
    }

}
