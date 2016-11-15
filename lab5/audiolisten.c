#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <semaphore.h>
#include <pthread.h>

#define MAX_BUFF 1000

// info that we need to share with handlers
// (threads in this case)
int udp_sd, buf_sz, fd, payload_size, target_buf;
double gamma_;
int last_read = -1;
int last_written = -1;
int current = 0;
int occupied = 0;
int finished_reading = 0;
int finished_writing = 0;

// flags to control program in and out of signals
volatile sig_atomic_t sigpoll_signal = 0;
volatile sig_atomic_t alarm_signal = 0;

// shared buffer and semaphore to control shared access
char * shared_buff;
sem_t mutex;

void alarm_handl(int sig) {
    //printf("Got alarm signal\n");
    alarm_signal = 1;
}

void sigpoll_handl(int sig) {
    //printf("Got sigpoll signal\n");
    sigpoll_signal = 1;
}

void *playback(void *threadid) {
    // read from buffer, write to dev/audio
    // shared buff is a circular buffer
    char * out;
    int num_bytes;

    if (finished_reading) {
      finished_writing = 1;
    } else {

        sem_wait(&mutex);
        if (last_written > last_read) {
            num_bytes = last_written - last_read;
            out = malloc((num_bytes + 1) * sizeof(char));
            int i;
            int j = 0;
            for(i = last_read + 1; i <= last_written; i++) {
                out[j] = shared_buff[i];
                j++;
                last_read = i;
            }
        } else if (last_written < last_read) {
            num_bytes = buf_sz - last_read + last_written;
            out = malloc((num_bytes + 1)* sizeof(char));
            int i;
            int j = 0;
            for (i = last_read + 1; i < buf_sz; i++) {
                out[j] = shared_buff[i];
                j++;
                last_read = i;
            }

            for (i = 0; i <= last_written; i++) {
                out[j] = shared_buff[i];
                j++;
                last_read = i;
            }
        } else {
            num_bytes = 0;
            out = malloc((num_bytes + 1) * sizeof(char));
        }
        // if last written is last read there is nothing to read
        sem_post(&mutex);

        out[num_bytes] = '\0';
        if (num_bytes > 0) {
            printf("last read: %d, num bytes: %d out len: %lu\n",
                    last_read, num_bytes, strlen(out));
            write(fd, out, strlen(out));
        }
    }
    pthread_exit(NULL);
}

void *read_stream(void *threadid) {
    struct sockaddr_in server_addr;
    int addr_len = sizeof(server_addr);
    char buffer[MAX_BUFF + 1];
    char sequence[4 + 1];
    char payload[payload_size + 1];
    int num_bytes;

    num_bytes = recvfrom(udp_sd, buffer, sizeof buffer, 0,
                         (struct sockaddr *)&server_addr, &addr_len);
    if (num_bytes > 0) {
        buffer[num_bytes] = '\0';
        if (buffer[0] == 'F') {
            finished_reading = 1;
        } else {
            printf("received %d bytes\n", num_bytes);
            // strip sequence number
            // what to do with the sequence
            // how to do resequencing?
            memcpy(sequence, &buffer[0], 4);
            sequence[4] = '\0';
            // get payload
            memcpy(payload, &buffer[4], payload_size);
            payload[payload_size] = '\0';

            int free_space;
            //printf("payload %lu bytes\n", strlen(payload));

            if (last_written > last_read) {
                free_space = buf_sz - last_written + last_read;
            } else if (last_written < last_read) {
                free_space = last_read - last_written;
            } else {
                free_space = buf_sz;
            }

            occupied = buf_sz - free_space;

            sem_wait(&mutex);
            printf("last_read: %d, last_written: %d\n",
                    last_read, last_written);
            int j;
            for (j = 0; j < num_bytes; j++) {
                if (j >= strlen(payload))
                    break;
                shared_buff[current] = payload[j];
                last_written = current;
                occupied++;
                current = (current + 1) % buf_sz;
            }
            sem_post(&mutex);

            // send feedback
            char feedback[MAX_BUFF + 1];

            snprintf(feedback, sizeof feedback, "Q %d %d %f",
                     occupied, target_buf, gamma_);
            printf("%s\n", feedback);
            printf("last written: %d\n", last_written);
            sendto(udp_sd, feedback, strlen(feedback), 0,
                   (struct sockaddr *)&server_addr, sizeof server_addr);
        }
    }

    pthread_exit(NULL);

}

int listen_stream(int udp_port, double gamma_) {
    printf("listen_stream\n");
    struct sockaddr_in udp_addr;
    memset(&udp_addr, 0, sizeof udp_addr);
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    udp_addr.sin_port = htons(udp_port);
    printf("udp port %d\n", udp_port);

    // create socket for listening to stream
    if ((udp_sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket error");
        return -1;
    }

    // bind listening socket
    printf("bind udp socket %d\n", udp_sd);
    if (bind(udp_sd, (struct sockaddr *)&udp_addr, sizeof udp_addr) < 0) {
        perror("bind error");
        return -1;
    }

    //stuff to handle sigpoll signals
    struct sigaction sa_sigpoll;
    memset(&sa_sigpoll, 0, sizeof sa_sigpoll);
    sa_sigpoll.sa_handler = sigpoll_handl;
    //sa_sigpoll.sa_flags = SA_NODEFER;
    sigaction(SIGPOLL, &sa_sigpoll, 0);

    fcntl(udp_sd, F_SETOWN, getpid());
    fcntl(udp_sd, F_SETFL, FASYNC);

    //stuff to handle alarm signals
    struct sigaction sa_alarm;
    memset(&sa_alarm, 0, sizeof sa_alarm);
    //sa_alarm.sa_flags = SA_NODEFER;
    sa_alarm.sa_handler = alarm_handl;
    sigaction(SIGALRM, &sa_alarm, 0);

    // open audio dev
    fd = open("/dev/audio", O_RDWR);

    // create semaphore
    sem_init(&mutex, 0, 1);

    // msec to microsec and set alarm
    ualarm(1.0/gamma_ * 1000, 1.0/gamma_ * 1000);
    while (finished_writing == 0) {
        pthread_t t1;
        pthread_t t2;
        if (sigpoll_signal == 1) {
            sigpoll_signal = 0;
            pthread_create(&t1, NULL, read_stream, 0);
        } else if (alarm_signal == 1) {
            alarm_signal = 0;
            pthread_create(&t2, NULL, playback, 0);
        }
    }

    sem_destroy(&mutex); // destroy semaphore
    return 1;
}

int main(int argc, char *argv[]) {

     if (argc != 11) {
        printf("Run: %s %s %s %s %s %s %s %s %s %s %s\n", argv[0],
               "[server-ip]", "[server-tcp-port]", "[client-udp-port]",
               "[payload-size]", "[playback-del]", "[gamma_]", "[buf-sz]",
               "[target-buf]", "[logfile-c]", "[filename]");
        return -1;

    }
    int tcp_server_sd, tcp_server_port, udp_port;
    struct sockaddr_in tcp_server_addr;
    struct in_addr host_addr;
    struct hostent *server_ipv4_addr;
    char message[MAX_BUFF + 1];
    char buffer[MAX_BUFF + 1];

    int num_bytes;
    char * response;

    tcp_server_port = atoi(argv[2]);
    udp_port = atoi(argv[3]);

    // other input params
    double playback_del;
    char * logfile;

    // set size of shared buffer
    shared_buff = malloc(buf_sz * sizeof(char));

    payload_size = atoi(argv[4]);
    playback_del = atof(argv[5]);
    gamma_ = atof(argv[6]);
    buf_sz = atoi(argv[7]);
    target_buf = atoi(argv[8]);
    logfile = argv[9];

    printf("IP address %s\n", argv[1]);
    inet_pton(AF_INET, argv[1], &host_addr);
    server_ipv4_addr =
        gethostbyaddr(&host_addr, sizeof host_addr, AF_INET);
    if (!server_ipv4_addr) {
        perror("unknown IP address");
        return -1;
    }

    // set info to send message
    memset(&tcp_server_addr, 0, sizeof tcp_server_addr);
    tcp_server_addr.sin_family = AF_INET;
    bcopy(server_ipv4_addr->h_addr, &(tcp_server_addr.sin_addr.s_addr),
          server_ipv4_addr->h_length);
    tcp_server_addr.sin_port = htons(tcp_server_port);


    // create tcp socket
    if ((tcp_server_sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        return -1;
    }

    // connect to tcp socket
    if (connect(tcp_server_sd, (struct sockaddr *)&tcp_server_addr,
                sizeof tcp_server_addr) < 0) {
        perror("connect error");
        close(tcp_server_sd);
        return -1;
    }

    // create message
    sprintf(message, "%s %s", argv[3], argv[10]);
    printf("Message: %s\n", message);


    write(tcp_server_sd, message, strlen(message));
    num_bytes = read(tcp_server_sd, buffer, MAX_BUFF);
    if (num_bytes > 0) {
        buffer[num_bytes] = '\0';
        response = strtok(buffer, " ");
        if (strcmp(response, "OK") == 0) {
            listen_stream(udp_port, gamma_);
        } else if (strcmp(response, "KO") == 0) {
            printf("Request denied\n");
        } else {
            printf("Unexpected response\n");
        }
    }

    close(tcp_server_sd);

}
