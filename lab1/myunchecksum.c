#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/stat.h>

int main(int argc, char *argv[]) {

    if (argc != 3) {
        printf("Correct call: ./myunchecksum file1 file2");
        return(-1);
    }

    int fd1; int fd2;

    if ((fd1 = open(argv[1], O_RDONLY)) < 0) {
        printf("File %s couldn't be opened\n", argv[1]);
        return(-1);
    }

    if ((fd2 = open(argv[2], O_CREAT | O_WRONLY)) < 0) {
        printf("File %s couldn't be opened\n", argv[2]);
        return(-1);
    }

    uint64_t checksum, sum;
    unsigned byte;
    struct stat st;
    int num_bytes;

    // find the length of the file in bytes
    if (stat(argv[1], &st) == 0)
        num_bytes = st.st_size;

    // read everything but the last 8 bytes
    // sum byte by byte and write to file2
    int count = 0;
    while (read(fd1, &byte, 1) == 1 && count < num_bytes - 8) {
        sum += byte;
        count++;
        write(fd2, &byte, 1);
    }

    // pull the last 8 bytes to see checksum
    lseek(fd1, -8L, SEEK_END);
    int read_byte = read(fd1, &checksum, 8);

    // compare checksum with file contents
    int match = checksum == sum;
    if (match) {
        printf("match: 0x%llX 0x%llX\n", checksum, sum);
    } else {
        printf("doesn't match: 0x%llX 0x%llX\n", checksum, sum);
    }
}
