/*
 * named pipe (FIFO) — writer, smallest demo.
 *
 * open(O_WRONLY) blocks until a reader has the FIFO open. Run the reader
 * first, or start this program first: both orders work, one side waits.
 *
 * Write a byte stream, not a C string. The reader adds the NUL itself.
 * Closing our write end is what makes the reader's read return 0.
 *
 * Build: gcc -std=c11 -D_POSIX_C_SOURCE=200809L -o fifo_writer fifo_writer.c
 * Run:   ./fifo_writer [/tmp/linux_com_v1.fifo] [message]
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define DEFAULT_PATH "/tmp/linux_com_v1.fifo"

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : DEFAULT_PATH;
    const char *msg = argc > 2 ? argv[2] : "hello";
    int fd;

    if (mkfifo(path, 0666) < 0 && errno != EEXIST) {
        perror("mkfifo");
        return EXIT_FAILURE;
    }

    printf("writer: sending to %s\n", path);
    fflush(stdout);

    fd = open(path, O_WRONLY);
    if (fd < 0) {
        perror("open");
        return EXIT_FAILURE;
    }

    if (write(fd, msg, strlen(msg)) < 0) {
        perror("write");
        close(fd);
        return EXIT_FAILURE;
    }

    close(fd); /* last writer gone -> reader sees EOF */
    printf("writer: sent '%s'\n", msg);
    return EXIT_SUCCESS;
}
