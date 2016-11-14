#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define MAX_BUFF 10000

// signal handler to not block the server waiting
// for child processes
void childsig_handler(int signum) {
    int status;
    wait(&status);
}

int stream(char * filename, int udp_port, int payload_size,
           double packet_spacing, int mode, char * logfile) {

    int num_bytes;
    char buffer[MAX_BUFF + 1];
    char message[MAX_BUFF + 1];
    FILE *f_stream;

    // four byte sequence number
    uint32_t seq;

    f_stream = fopen(filename, "rb");
    while ((num_bytes = fread(buffer, 1, payload_size, f_stream)) > 0) {
        buffer[num_bytes] = '\0';
        sprintf(message, "%d%s", seq, buffer);

    }
    // stream
    return 1;
}

int main(int argc, char *argv[]) {

    if (argc != 7) {
        printf("Run: %s %s %s %s %s %s %s\n", argv[0], "[tcp-port]",
               "[upd-port]", "[payload-size]", "[packet-spacing]", "[mode]",
               "[logfile-s]");
        return -1;
    }

    pid_t k;
    struct sockaddr_in tcp_addr;
    int tcp_sd, tcp_port, tcp_conn;
    int num_bytes;
    char buffer[MAX_BUFF + 1];
    char message[MAX_BUFF + 1];

    // incomming client info
    struct sockaddr_in client_addr;

    // other input params
    int udp_port, payload_size, mode;
    double packet_spacing;
    char * logfile;

    udp_port = atoi(argv[2]);
    payload_size = atoi(argv[3]);
    packet_spacing = atof(argv[4]);
    mode = atoi(argv[5]);
    logfile = argv[6];

    // to tokenize received request
    char * portnumber;
    char * filename;

    // set info to listen
    tcp_port = atoi(argv[1]);
    memset(&tcp_addr, 0, sizeof(tcp_addr));
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    tcp_addr.sin_port = htons(tcp_port);

    // create socket to listen to requests
    if ((tcp_sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        return -1;
    }

    // bind listening socket
    if ((bind(tcp_sd, (struct sockaddr *)&tcp_addr, sizeof(tcp_addr))) < 0) {
        perror("bind error");
        return -1;
    }

    // start listening
    listen(tcp_sd, 5);

    // stuff needed to handle the signals to handle waiting for children
    struct sigaction sigchld_action;
    memset (&sigchld_action, 0, sizeof (sigchld_action));
    sigchld_action.sa_handler = &childsig_handler;
    sigaction (SIGCHLD, &sigchld_action, NULL);


    while (1) {
        tcp_conn = accept(tcp_sd, (struct sockaddr *)&client_addr, 0);
        char * client_ip = inet_ntoa(client_addr.sin_addr);

        num_bytes = read(tcp_conn, buffer, MAX_BUFF);
        if (num_bytes > 0) {
            buffer[num_bytes] = '\0';
            portnumber = strtok(buffer, " ");
            filename = strtok(NULL, " ");

            // check if file exists or not
            if (access(filename, F_OK) == -1) {
                write(tcp_conn, "KO", 2);
            } else {
                sprintf(message, "OK %s", portnumber);
                write(tcp_conn, message, strlen(message));

                // fork a child to perform the streaming
                k = fork();
                if (k == 0) {
                    // what is the different between the two ports ?
                    udp_port = atoi(portnumber);
                    stream(filename, udp_port, payload_size, packet_spacing,
                           mode, logfile);

                } else {
                    close(tcp_conn);
                }
            }
        }
        close(tcp_conn);
    }

}
