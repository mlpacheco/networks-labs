#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define MAX_BUFF 10000

int main(int argc, char *argv[]) {

     // check that we have all needed params
    if (argc != 6) {
        printf("Run: %s [hostname] [portnumber] [secretkey] [filename] [configfile]\n", argv[0]);
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
    int sd, port, num_bytes, bytes_read, write_smth, total_read;
    FILE *f_dwnld;
    FILE *f_config;
    struct sockaddr_in server_addr;
    struct hostent *ipv4address;
    char message[MAX_BUFF + 1];
    char buffer[MAX_BUFF + 1];
    struct timeval start_time, end_time;
    double start_sec, end_sec, elapsed_sec, reliable_throughput;

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
    bcopy ( ipv4address->h_addr, &(server_addr.sin_addr.s_addr), ipv4address->h_length);
    server_addr.sin_port = htons(port);

    // create socket
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        return -1;
    }

    // connect to socket
    if (connect(sd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect error");
        close(sd);
        return -1;
    }

    // create the message to be sent and send it to the server
    snprintf(message, sizeof(message), "$%s$%s", argv[3], argv[4]);
    write_smth = write(sd, message, strlen(message));

    // read configuration file to know how many bytes to read at a time
    f_config = fopen(argv[5], "r");
    fscanf(f_config, "%s", buffer);
    num_bytes = atoi(buffer);

    // open file to write downloaded contents
    f_dwnld = fopen(argv[4], "ab");

    // read from the socket per specified bytes and write to local file
    gettimeofday(&start_time, 0);
    while ((bytes_read = read(sd, buffer, num_bytes)) > 0) {
        buffer[bytes_read] = '\0';
        fputs(buffer, f_dwnld);
        total_read += bytes_read;
    }
    gettimeofday(&end_time, 0);

    // close connection and file
    close(sd);
    fclose(f_dwnld);

    // calculate sececonds we took
    start_sec = ((start_time.tv_sec) * 1000.0 + (start_time.tv_usec) / 1000.0)/1000.0 ;
    end_sec = ((end_time.tv_sec) * 1000.0 + (end_time.tv_usec) / 1000.0)/1000.0 ;
    elapsed_sec = end_sec - start_sec;

    // trasnform bytes to Megabits
    double total_Mb = total_read / 131072;

    // calculate reliable throughput = msg_len / RTT
    reliable_throughput = total_Mb / elapsed_sec;

    // print results
    printf("read = %d bytes\ntime = %f sec\nreliable_throughput = %f Mbps\n", total_read, elapsed_sec, reliable_throughput);

}
