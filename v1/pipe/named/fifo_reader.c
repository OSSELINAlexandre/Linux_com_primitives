/*
 * named pipe (FIFO) — reader, smallest demo.
 *
 * A FIFO is a pipe with a path. Unrelated processes can open it.
 * mkfifo() only creates the directory entry. The data does not live in
 * the filesystem: open/read/write go through a kernel pipe.
 *
 * Opening O_RDONLY blocks until some process opens the same FIFO for
 * writing (and the other way around). That is not a deadlock by itself;
 * it is the rendezvous. Start either side first.
 *
 * The mode passed to mkfifo is filtered by umask. 0666 with a typical
 * umask of 022 becomes 0644. The call is not "ignoring" the mode.
 *
 * Build: gcc -std=c11 -D_POSIX_C_SOURCE=200809L -o fifo_reader fifo_reader.c
 * Run:   ./fifo_reader [/tmp/linux_com_v1.fifo]
 *        then, in another terminal: ./fifo_writer
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#define DEFAULT_PATH "/tmp/linux_com_v1.fifo"

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : DEFAULT_PATH;
    char buf[256];
    size_t total = 0;
    int fd;

    if (mkfifo(path, 0666) < 0 && errno != EEXIST) {
        perror("mkfifo");
        return EXIT_FAILURE;
    }

    printf("reader: waiting on %s\n", path);
    fflush(stdout);

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return EXIT_FAILURE;
    }

    while (total < sizeof buf - 1) {
        ssize_t n = read(fd, buf + total, sizeof buf - 1 - total);
        if (n == 0)
            break;
        if (n < 0) {
            if (errno == EINTR)
                continue;
            perror("read");
            close(fd);
            return EXIT_FAILURE;
        }
        total += (size_t)n;
    }
    buf[total] = '\0';
    close(fd);

    /* The directory entry survives the close. Remove it so the next run starts clean. */
    if (unlink(path) < 0)
        perror("unlink");

    printf("reader: got '%s'\n", buf);
    return EXIT_SUCCESS;
}
