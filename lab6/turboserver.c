#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include "dropsendto.h"

#define MAX_BUFF 10000

// keep variables we need to access in signal
int udp_sd, window_sz, payload_sz;
char * window;

volatile sig_atomic_t transmission_incomplete = 1;

// signal handler to not block the server waiting
// for child processes
void childsig_handler(int signum) {
    int status;
    wait(&status);
}


void sigpoll_handl_nack(int sig) {
    printf("sigpoll handler\n");
    char buffer[MAX_BUFF + 1];
    struct sockaddr_in their_addr;
    socklen_t addr_len = sizeof(their_addr);
    int bytes_read, seqnumber, start;
    char * nack;
    char * seqnumber_str;
    char payload[payload_sz + 1];
    char message[MAX_BUFF + 1];

    bytes_read = recvfrom(udp_sd, buffer, sizeof(buffer), 0,
                        (struct sockaddr *)&their_addr, &addr_len);

    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        nack = strtok(buffer, "$");
        seqnumber_str = strtok(NULL, "$");

        if (strcmp(nack, "NACK") == 0) {
            seqnumber = atoi(seqnumber_str);

            printf("NACK %d\n", seqnumber);

            snprintf(message, sizeof(message), "%d$", seqnumber);

            int i;
            int payload_curr = strlen(message);
            int msg_len = strlen(message) + payload_sz;
            start = seqnumber * payload_sz;
            for (i = 0; i < payload_sz; i++) {
                message[payload_curr] = window[start + i];
                payload_curr++;
            }

            sendto(udp_sd, message, msg_len, 0,
                   (struct sockaddr *)&their_addr, addr_len);
        } else if (strcmp(nack, "DONE") == 0) {
            transmission_incomplete = 0;
        }
    }
}


// Here we want to implement the new protocol
int transfer_file(char * filepath, int numbytes, int sd,
                  const struct sockaddr * addr, int addrsize) {
    FILE *f_dwnld;
    int bytes_read, file_sz;
    char buffer[MAX_BUFF + 1];
    char message[MAX_BUFF + 1];
    int seqnumber = 0;

    //stuff to handle sigpoll signals
    struct sigaction sa_sigpoll;
    memset(&sa_sigpoll, 0, sizeof sa_sigpoll);
    sa_sigpoll.sa_handler = sigpoll_handl_nack;
    sa_sigpoll.sa_flags = SA_NODEFER;
    sigaction(SIGPOLL, &sa_sigpoll, 0);

    fcntl(sd, F_SETOWN, getpid());
    fcntl(sd, F_SETFL, FASYNC);

    // open file to read content to upload to client
    f_dwnld = fopen(filepath, "rb");
    fseek(f_dwnld, 0L, SEEK_END);
    file_sz = ftell(f_dwnld);
    rewind(f_dwnld);

    int current = 0;

    // signaling beginning and letting client know the size of the file
    snprintf(message, sizeof(message), "-1$%d$%d", file_sz, numbytes);
    sendto(sd, message, strlen(message), 0, addr, addrsize);
    printf("Sent: %s\n", message);

    // need to dinamically allocate size of window
    // I am stating by keeping a window of the size of the whole file
    window = malloc(file_sz + 1);
    payload_sz = numbytes;

    // if the file exists, write to client per bytes specified
    if (f_dwnld != NULL) {
        while((bytes_read = fread(buffer, 1, numbytes, f_dwnld)) > 0) {
            buffer[bytes_read] = '\0';
            // create the message with sequence number and payload
            snprintf(message, sizeof(message), "%d$", seqnumber, buffer);
            int i;
            int curr_payload = strlen(message);
            int msg_len = strlen(message) + bytes_read;
            for (i = 0; i < bytes_read; i++) {
                window[current] = buffer[i];
                message[curr_payload] = buffer[i];
                current++;
                curr_payload++;
            }

            printf("Seqnumber: %d, payload_sz: %d, message_sz: %d\n", seqnumber, bytes_read, msg_len);

            // change this call with the wrapper
            dropsendto(sd, message, msg_len, addr, addrsize, 10, 1);
            seqnumber++;
            //exit(0);
        }

        // signaling the end of the transmission
        //snprintf(message, sizeof(message), "$-2$%d", bytes_read);
        //sendto(sd, message, strlen(message), 0, addr, addrsize);
    }

    while(transmission_incomplete == 1) {
    }

    printf("Done\n");
    fclose(f_dwnld);

    return 0;
}

int main(int argc, char *argv[]) {

    // check that we have all needed params
    if (argc != 4) {
        printf("Run: %s [portnumber] [secretkey] [configfile]\n", argv[0]);
        return -1;
    }

    // check that secret key has the appropriate format
    if (strlen(argv[2]) < 10 || strlen(argv[2]) > 20) {
        printf("Secret key should be of length [10,20]\n");
        return -1;
    }

    // all the variables that we will need
    pid_t k;
    struct sockaddr_in addr_tcp;
    struct sockaddr_in addr_udp;
    int tcp_sd, tcp_port, udp_port, conn, read_smth, num_bytes;
    char buffer[MAX_BUFF + 1];
    char filepath[MAX_BUFF + 1];
    char * secretkey;
    char * filename;
    FILE *f_config;

    tcp_port = atoi(argv[1]);
    socklen_t addrsize = sizeof(addr_tcp);

    // set server info
    memset(&addr_tcp, 0, sizeof(addr_tcp));
    addr_tcp.sin_family = AF_INET;
    addr_tcp.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_tcp.sin_port = htons(tcp_port);


    // create socket
    if ((tcp_sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        return -1;
    }

    // bind server information to the server socket
    if ((bind(tcp_sd, (struct sockaddr *)&addr_tcp, sizeof(addr_tcp))) < 0) {
        perror("bind error");
        return -1;
    }

    // stuff needed to handle the signals
    // to avoid zombie processes
    struct sigaction sigchld_action;
    memset (&sigchld_action, 0, sizeof (sigchld_action));
    sigchld_action.sa_handler = &childsig_handler;
    sigaction (SIGCHLD, &sigchld_action, NULL);


    // start listening
    listen(tcp_sd, 5);

    // accept connections from clients constantly
    int client_count = 1;
    while(1) {
        struct sockaddr_in client_addr;
        socklen_t clilen = sizeof(client_addr);
        struct in_addr host_addr;
        struct hostent *ipv4address;

        conn = accept(tcp_sd, (struct sockaddr *) &client_addr, &clilen);
        char * client_ip = inet_ntoa(client_addr.sin_addr);

        k = fork();

        // child process to manage file transfer
        // if parent, go back to wait to receive another request
        if (k == 0) {
            read_smth = read(conn, buffer, MAX_BUFF);
            if (read_smth > 0) {
                buffer[read_smth] = '\0';

                // parse message
                secretkey = strtok(buffer, "$");
                filename = strtok(NULL, "$");
                // construct filepath
                snprintf(filepath, sizeof(filepath), "%s%s", "./filedeposit/",
                         filename);

                // ignore packages that have a different secretkey
                if (strcmp(secretkey, argv[2]) != 0) {
                    printf("Different keys\n");
                    exit(-1);
                }

                // read configuration file to know bytes to write
                f_config = fopen(argv[3], "r");
                fscanf(f_config, "%s", buffer);
                num_bytes = atoi(buffer);
                fclose(f_config);

                // send the port to be used for UDP transfer of packages
                char udp_port_str[MAX_BUFF + 1];
                snprintf(udp_port_str, sizeof(udp_port_str), "%d", tcp_port + client_count);
                write(conn, udp_port_str, strlen(udp_port_str));
                printf("wrote: %s\n", udp_port_str);

                udp_port = atoi(udp_port_str);
                socklen_t addrsize = sizeof(addr_udp);

                // set UDP info
                inet_pton(AF_INET, client_ip, &host_addr);
                ipv4address = gethostbyaddr(&host_addr, sizeof host_addr, AF_INET);
                memset(&addr_udp, 0, sizeof(addr_udp));
                addr_udp.sin_family = AF_INET;
                bcopy(ipv4address->h_addr, &(addr_udp.sin_addr.s_addr), ipv4address->h_length);
                addr_udp.sin_port = htons(udp_port);

                // create UDP socket
                if ((udp_sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
                    perror("socket error");
                    return -1;
                }

                transfer_file(filepath, num_bytes, udp_sd,
                             (struct sockaddr *)&addr_udp, addrsize);
            }

            close(conn);
            exit(0);

        } else {
            client_count++;
        }

    }



}


