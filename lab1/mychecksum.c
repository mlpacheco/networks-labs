#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <inttypes.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Correct call: ./mychecksum file1 file2\n");
        return(-1);
    }

    int fd1; int fd2;

    if ((fd1 = open(argv[1], O_RDWR)) < 0) {
        printf("File %s couldn't be opened\n", argv[1]);
        return(-1);
    }

    if ((fd2 = open(argv[2], O_CREAT | O_RDWR)) < 0) {
        printf("File %s couldn't be opened\n", argv[2]);
        return(-1);
    }

    // read byte per byte and copy to file2
    // sum bytes to create checksum
    uint64_t sum = 0;
    unsigned byte;
    while (read(fd1, &byte, 1) == 1) {
        sum += byte;
        write(fd2, &byte, 1);
    }

    // write checksum in last 8 bytes
    write(fd2, &sum, 8);
}
