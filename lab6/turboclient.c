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
int sd;
struct sockaddr_in server_addr;

int payload_sz;
int last_checked = -1;
int last_received = -1;
int * received_packets;
char * window;

void sigalrm_handl_nack(int sig) {
    int i, j, offset;
    char message[MAX_BUFF + 1];
    char payload[payload_sz + 1];
    for (i = last_checked + 1; offset < last_received; offset++) {
        if (received_packets[i] == 0) {
            // send NACK
            offset = i * payload_sz;
            for (j = 0; j < payload_sz; j++) {
                payload[j] = window[offset + i];
            }
            payload[j] = '\0';
            snprintf(message, sizeof(message), "$NACK$%d", i);
            sendto(sd, message, strlen(message), 0,
                   (struct sockaddr *)&server_addr, sizeof(server_addr));
            last_checked = i;
        }
    }
}

// this had to be modified according to protocol
int receive_file(char * filepath, int num_bytes, socklen_t addrlen) {
    FILE *f_dwnld;
    struct timeval start_time, end_time;
    char buffer[MAX_BUFF + 1];
    char message[MAX_BUFF + 1];
    int bytes_read, total_read, seqnumber, total_bytes, offset, prev_seqnumber;
    double start_sec, end_sec, elapsed_sec, reliable_throughput;
    char * seqnumber_str;
    char * payload;
    char * payload_sz_str;
    char * total_bytes_str;

    //stuff to handle alarm signals
    struct sigaction sa_alarm;
    memset(&sa_alarm, 0, sizeof sa_alarm);
    //sa_alarm.sa_flags = SA_NODEFER;
    sa_alarm.sa_handler = sigalrm_handl_nack;
    sigaction(SIGALRM, &sa_alarm, 0);

    // open file to write downloaded contents
    f_dwnld = fopen(filepath, "ab");

    // read from the socket per specified bytes and write to local file
    gettimeofday(&start_time, 0);
    total_read = 0;
    prev_seqnumber = -1;

    // set alarm to check not received packets
    ualarm(100000, 100000);

    while ((bytes_read = recvfrom(sd, buffer, sizeof(buffer), 0,
                                  (struct sockaddr *)&server_addr,
                                  &addrlen)) > 0) {
        buffer[bytes_read] = '\0';

        // parse message
        seqnumber_str = strtok(buffer, "$");

        seqnumber = atoi(seqnumber_str);

        if (seqnumber == -1) {
            // parse command
            total_bytes_str = strtok(NULL, "$");
            payload_sz_str = strtok(NULL, "$");
            // transform data
            total_bytes = atoi(total_bytes_str);
            payload_sz = atoi(payload_sz_str);
            // set window and packet tracker
            window = malloc(total_bytes + 1);
            received_packets = malloc(ceil(total_bytes / payload_sz)
                                       * sizeof(int));
            memset(received_packets, 0, total_bytes);
        } else if (seqnumber == -2) {
            // this means the transmission ended (for now)
            break;
        } else {
            payload = strtok(NULL, "$");
            received_packets[seqnumber] = 1;
            //fputs(payload, f_dwnld);
            offset = strlen(payload) * seqnumber;
            for (int i = 0; i < strlen(payload); i++) {
                window[offset + i] = payload[i];
            }
            total_read += bytes_read;
            last_received = seqnumber;
        }

    }
    gettimeofday(&end_time, 0);
    fclose(f_dwnld);

    // calculate sececonds we took
    start_sec = ((start_time.tv_sec) * 1000.0 + \
            (start_time.tv_usec)/ 1000.0)/1000.0 ;
    end_sec = ((end_time.tv_sec) * 1000.0 + \
            (end_time.tv_usec) / 1000.0)/1000.0 ;
    elapsed_sec = end_sec - start_sec;

     // trasnform bytes to Megabits
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
    int port, num_bytes, bytes_read, write_smth, total_read;
    FILE *f_config;
    struct hostent *ipv4address;
    char message[MAX_BUFF + 1];
    char buffer[MAX_BUFF + 1];

    // get port and host
    port = atoi(argv[2]);
    ipv4address = gethostbyname(argv[1]);
    if (!ipv4address) {
        perror("unknown host");
        return -1;
    }

    // set server info
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    bcopy(ipv4address->h_addr, &(server_addr.sin_addr.s_addr),
          ipv4address->h_length);
    server_addr.sin_port = htons(port);

    // create socket
    if ((sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("socket error");
        return -1;
    }

    // create the message to be sent and send it to the server
    snprintf(message, sizeof(message), "$%s$%s", argv[3], argv[4]);
    sendto(sd, message, strlen(message), 0, (struct sockaddr *) &server_addr,
           sizeof(server_addr));

    // read configuration file to know how many bytes to read at a time
    f_config = fopen(argv[5], "r");
    fscanf(f_config, "%s", buffer);
    num_bytes = atoi(buffer);
    fclose(f_config);

    receive_file(argv[4], num_bytes, sizeof(server_addr));



}
