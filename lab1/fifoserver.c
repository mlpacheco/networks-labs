#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#define CMDFIFO "cmdfifo"

/**
 * Maria Pacheco - pachecog@purdue.edu
 */

// signal handler to not block the server waiting
void childsig_handler(int signum) {
    int status;
    wait(&status);
}

int main(int argc, char *argv[]) {

    pid_t k;
    int fd_in, fd_out, status;
    char buf_in[100];
    char buf_out[100];
    char cfifoPID[100];
    char * pid;
    char * cmd;

    // crete pipe to receive requests
    mkfifo(CMDFIFO, 0666);
    fd_in = open(CMDFIFO, O_RDONLY);

    if (fd_in < 0) {
        perror("couldn't open server FIFO");
        exit(-1);
    }

    // stuff needed to handle the signals
    struct sigaction sigchld_action;
    memset (&sigchld_action, 0, sizeof (sigchld_action));
    sigchld_action.sa_handler = &childsig_handler;
    sigaction (SIGCHLD, &sigchld_action, NULL);

    while(1) {
        // constantly read the fifo to find new requests
        int read_smth = read(fd_in, buf_in, 100);
        buf_in[read_smth] = '\0';
        if (read_smth > 0) {   // if a request was received, fork and have the child execute
            k = fork();
            if (k == 0) {
                // break down the message into pid and command
                pid = strtok(buf_in, "$");
                cmd = strtok(NULL, "$");
                snprintf(cfifoPID, sizeof(cfifoPID), "cfifo%s", pid);
                fd_out = open(cfifoPID, O_WRONLY);

                // redirect output to pipe
                if (fd_out > 0) {
                    dup2(fd_out, 1);
                    if (execlp(cmd, cmd, NULL) == -1) {
                        close(fd_out);
                        exit(-1);
                    }
                    close(fd_out);
                }
            }
        }
    }

    close(fd_in);
    unlink(CMDFIFO);
    return 0;
}
