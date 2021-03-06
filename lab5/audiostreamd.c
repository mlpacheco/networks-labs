#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/time.h>

#define MAX_BUFF 10000

int udp_sd, mode;
double tau;

// signal handler to not block the server waiting
// for child processes
void childsig_handler(int signum) {
    int status;
    wait(&status);
}

void sigpoll_handler(int signum) {
    struct sockaddr_in client_addr;
    int addr_len = sizeof(client_addr);
    char buffer[MAX_BUFF + 1];
    int num_bytes;

    char * response;
    char * Qt_str;
    char * Qopt_str;
    char * gamma_str;

    int Qt, Qopt;
    double gamma;

    double a, delta, eps, beta, curr_lambda, new_tau;

    num_bytes = recvfrom(udp_sd, buffer, sizeof buffer, 0,
                         (struct sockaddr *)&client_addr, &addr_len);

    if (num_bytes > 0) {
        buffer[num_bytes] = '\0';
        //printf("%s\n", buffer);
        if (buffer[0] == 'Q') {
            response = strtok(buffer, " ");
            Qt_str = strtok(NULL, " ");
            Qopt_str = strtok(NULL, " ");
            gamma_str = strtok(NULL, " ");
            Qt = atoi(Qt_str);
            Qopt = atoi(Qopt_str);
            gamma = atof(gamma_str);


            if (mode == 0) {
                // Method A
                a = 3;
                curr_lambda = 1.0/(tau/1000.0);
                if (Qt < Qopt) {
                    curr_lambda += a;
                } else if (Qt > Qopt) {
                    curr_lambda -= a;
                }
            } else if (mode == 1) {
                // Method B
                a = 3; delta = 0.5;
                curr_lambda = 1.0/(tau/1000.0);
                if (Qt < Qopt) {
                    curr_lambda += a;
                } else if (Qt > Qopt) {
                    curr_lambda = curr_lambda * delta;
                }
            } else if (mode == 2) {
                // Method C
                eps = 0.5;
                curr_lambda = curr_lambda + eps * (Qopt - Qt);
            } else {
                // Method D
                eps = 0.2; beta = 0.5;
                curr_lambda =
                  curr_lambda + eps * (Qopt - Qt) - beta * (curr_lambda - gamma);
            }

            // common update
            new_tau = (1.0/curr_lambda)*1000;
            if (new_tau < 0) {
                tau = 0.0;
            } else {
                tau = new_tau;
            }

        }
    }
}

int stream(char * filename, int udp_port, char * client_ip, int payload_size,
           char * logfile) {
    printf("Starting to stream...\n");
    int num_bytes;
    char buffer[MAX_BUFF + 1];
    char message[MAX_BUFF + 1];
    FILE *f_stream;
    FILE *f_log;

    // buffer to keep logs
    char * log_buff;
    log_buff = malloc(1);

    // four byte sequence number
    uint32_t seq;

    // vars for communication
    struct sockaddr_in client_addr;
    struct in_addr host_addr;
    struct hostent *ipv4address;
    socklen_t addrsize = sizeof(client_addr);

    // resolve address
    inet_pton(AF_INET, client_ip, &host_addr);
    ipv4address = gethostbyaddr(&host_addr, sizeof host_addr, AF_INET);

    // create udp socket for streaming
    if ((udp_sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket error");
        return -1;
    }

    // set info for streaming
    memset(&client_addr, 0, sizeof client_addr);
    client_addr.sin_family = AF_INET;
    bcopy(ipv4address->h_addr, &(client_addr.sin_addr.s_addr),
          ipv4address->h_length);
    client_addr.sin_port = htons(udp_port);

    // set signal to receive feedback
    struct sigaction sa_sigpoll;
    memset(&sa_sigpoll, 0, sizeof sa_sigpoll);
    sa_sigpoll.sa_handler = sigpoll_handler;
    sa_sigpoll.sa_flags = SA_NODEFER;
    sigaction(SIGPOLL, &sa_sigpoll, 0);

    fcntl(udp_sd, F_SETOWN, getpid());
    fcntl(udp_sd, F_SETFL, FASYNC);

    // open audiofile and read bytes payload_size by payload_size
    f_stream = fopen(filename, "rb");
    while ((num_bytes = fread(buffer, 1, payload_size, f_stream)) > 0) {
        buffer[num_bytes] = '\0';
        sprintf(message, "%d%s", seq, buffer);
        sendto(udp_sd, message, strlen(message), 0,
               (struct sockaddr *)&client_addr, sizeof client_addr);
        //printf("Sent packet %d\n", seq);
        // space packets, convert msec to microsec
        // for now its fixed, this must change
        // we might need a global variable
        double excess = tau;
        //printf("Waiting: %f\n", tau);
        while (excess > 1000) {
            usleep(1000 * 1000);
            excess -= 1000;
        }
        usleep(excess * 1000);
        seq += 1;

        char log_step[MAX_BUFF + 1];
        struct timeval t;
        gettimeofday(&t, NULL);
        double millis = (t.tv_sec * 1000) + (t.tv_usec / 1000.0);
        snprintf(log_step, sizeof log_step, "%f %f\n", millis, 1.0/(tau/1000.0));

        char * log_with_step;
        log_with_step = malloc((strlen(log_buff) + strlen(log_step) + 1) * sizeof(char));
        strcpy(log_with_step, log_buff);
        strcat(log_with_step, log_step);
        log_with_step[strlen(log_buff) + strlen(log_step)] = '\0';
        log_buff = log_with_step;
    }
    sendto(udp_sd, "F", 1, 0, (struct sockaddr *)&client_addr,
           sizeof client_addr);

    char child_file[MAX_BUFF + 1];
    sprintf(child_file, "%s_%d", logfile, udp_port);
    //printf("filename: %s\n", child_file);
    f_log = fopen(child_file, "w");
    fputs(log_buff, f_log);

    printf("Finished streaming\n");

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
    char * client_ip;
    socklen_t addrsize;

    // other input params
    int udp_port, payload_size;
    char * logfile;

    udp_port = atoi(argv[2]);
    payload_size = atoi(argv[3]);
    // initial packet spacing
    tau = atof(argv[4]);
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
    //printf("tcp port %d\n", tcp_port);

    // create socket to listen to requests
    if ((tcp_sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        return -1;
    }

    // bind listening socket
    //printf("bind tcp socket %d\n", tcp_sd);
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
        addrsize = sizeof(client_addr);
        tcp_conn = accept(tcp_sd, (struct sockaddr *)&client_addr, &addrsize);
        client_ip = inet_ntoa(client_addr.sin_addr);

        num_bytes = read(tcp_conn, buffer, MAX_BUFF);
        if (num_bytes > 0) {
            printf("Received request from: %s\n", client_ip);
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
                    stream(filename, udp_port, client_ip, payload_size,
                           logfile);
                    exit(0);

                } else {
                    close(tcp_conn);
                }
            }
        }
        close(tcp_conn);
    }

}
