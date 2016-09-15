#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define CMDFIFO "cmdfifo"
#define CMD "ls"
/**
 * Maria Pacheco - pachecog@purdue.edu
 */

int main(int argc, char *argv[]) {
    int fd_in, fd_out;
    char buf_out[100];
    char buf_in[100];
    char cfifoPID[100];

    fd_out = open(CMDFIFO, O_WRONLY);

    if (fd_out < 0) {
       perror("couldn't open server FIFO");
       exit(-1);
    }

    // create the request by the specified format
    // and write it to the server
    snprintf(buf_out, sizeof(buf_out), "$%d$%s", getpid(), CMD);
    write(fd_out, buf_out, sizeof(buf_out));

    // create the pipe to read from
    snprintf(cfifoPID, sizeof(cfifoPID), "cfifo%d", getpid());
    mkfifo(cfifoPID, 0666);

    fd_in = open(cfifoPID, O_RDONLY);

    if (fd_in < 0) {
        perror("couldn't open client FIFO");
        exit(-1);
    }

    // read the response from the server and print it out
    int read_smth = read(fd_in, buf_in, sizeof(buf_in));
    if (read_smth > 0)
        printf("Received: \n%s\n", buf_in);
    unlink(cfifoPID);
    return 0;
}
