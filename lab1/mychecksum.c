#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <inttypes.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Correct call: ./mychecksum file1 file2\n");
    }

    uint64_t sum = 0;
    int fd1; int fd2;

    if ((fd1 = open(argv[1], O_RDWR)) < 0) {
        printf("File %s couldn't be opened\n", argv[1]);
        return(-1);
    }

    if ((fd2 = open(argv[2], O_CREAT | O_WRONLY)) < 0) {
        printf("File %s couldn't be opened\n", argv[2]);
        return(-1);
    }

    unsigned c;
    while (read(fd1, &c, 1) == 1) {
        sum += c;
        write(fd2, &c, 1);
    }
    printf("%" PRId64 "\n", sum);
    write(fd2, &sum, 8);
}
