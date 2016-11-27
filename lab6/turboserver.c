#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BUFF 10000

// keep variables we need to access in signal
int sd, window_sz, payload_sz;
char * window;

// signal handler to not block the server waiting
// for child processes
void childsig_handler(int signum) {
    int status;
    wait(&status);
}

int dropsendto(int sd, const char * message, int size,
               const struct sockaddr * addr, int addr_size, int totalnum,
               int lossnum) {
    return -1;
}

void sigpoll_handl_nack(int sig) {
    char buffer[MAX_BUFF + 1];
    struct sockaddr_in their_addr;
    socklen_t addr_len = sizeof(their_addr);
    int num_bytes, seqnumber, start;
    char * nack;
    char * seqnumber_str;
    char payload[payload_sz + 1];
    char message[MAX_BUFF + 1];

    num_bytes = recvfrom(sd, buffer, sizeof(buffer), 0,
                        (struct sockaddr *)&their_addr, &addr_len);

    if (num_bytes > 0) {
        buffer[num_bytes] = '\0';
        nack = strtok(buffer, "$");
        seqnumber_str = strtok(NULL, "$");

        if (strcmp(nack, "NACK") == 0) {
            seqnumber = atoi(seqnumber_str);

            printf("NACK %d\n", seqnumber);

            int i;
            start = seqnumber * payload_sz;
            for (i = 0; i < payload_sz; i++) {
                payload[i] = window[start + i];
            }
            payload[i] = '\0';

            snprintf(message, sizeof(message), "$%d$%s", seqnumber, payload);
            sendto(sd, message, strlen(message), 0,
                   (struct sockaddr *)&their_addr, addr_len);
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

    // open file to read content to upload to client
    f_dwnld = fopen(filepath, "rb");
    fseek(f_dwnld, 0L, SEEK_END);
    file_sz = ftell(f_dwnld);
    rewind(f_dwnld);

    int current = 0;

    // signaling beginning and letting client know the size of the file
    snprintf(message, sizeof(message), "$-1$%d", file_sz);
    sendto(sd, message, strlen(message), 0, addr, addrsize);

    // need to dinamically allocate size of window
    // I am stating by keeping a window of the size of the whole file
    window = malloc(file_sz + 1);

    // if the file exists, write to client per bytes specified
    if (f_dwnld != NULL) {
        while((bytes_read = fread(buffer, 1, numbytes, f_dwnld)) > 0) {
            buffer[bytes_read] = '\0';

            int i;
            for (i = 0; i < strlen(buffer); i++) {
                window[current] = buffer[i];
                current++;
            }

            // create the message with sequence number and payload
            snprintf(message, sizeof(message), "$%d$%s", seqnumber, buffer);
            //printf("message: %s\n", message);

            // change this call with the wrapper
            sendto(sd, message, strlen(message), 0, addr, addrsize);
            seqnumber++;
        }

        // signaling the end of the transmission
        snprintf(message, sizeof(message), "$-2$%d", bytes_read);
        sendto(sd, message, strlen(message), 0, addr, addrsize);
    }
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
    struct sockaddr_in addr;
    int port, conn, read_smth, num_bytes;
    char buffer[MAX_BUFF + 1];
    char filepath[MAX_BUFF + 1];
    char * secretkey;
    char * filename;
    FILE *f_config;

    port = atoi(argv[1]);
    socklen_t addrsize = sizeof(addr);

    // set server info
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);


    // create socket
    if ((sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("socket error");
        return -1;
    }

    // bind server information to the server socket
    if ((bind(sd, (struct sockaddr *)&addr, sizeof(addr))) < 0) {
        perror("bind error");
        return -1;
    }

    // stuff needed to handle the signals
    struct sigaction sigchld_action;
    memset (&sigchld_action, 0, sizeof (sigchld_action));
    sigchld_action.sa_handler = &childsig_handler;
    sigaction (SIGCHLD, &sigchld_action, NULL);

    // accept connections from clients constantly
    while(1) {
        read_smth = recvfrom(sd, buffer, sizeof(buffer), 0,
                            (struct sockaddr *) &addr, &addrsize);
        k = fork();

        // child process to manage file transfer
        // if parent, go back to wait to receive another request
        if (k == 0) {
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

                transfer_file(filepath, num_bytes, sd,
                             (struct sockaddr *)&addr, addrsize);

            }
            exit(0);
        }

    }



}


