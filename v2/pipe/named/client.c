/*
 * Named pipe client — one product, or "quit" to stop the server.
 *
 * Order matters. The server reads the request before it opens the reply
 * FIFO for writing. This program must therefore:
 *   1. open the request FIFO O_WRONLY
 *   2. write the line (the kernel buffers it)
 *   3. only then open the reply FIFO O_RDONLY
 * Opening the reply FIFO first deadlocks: this process sits in that open,
 * and the server sits in read() waiting for a line that was never written.
 *
 * Usage: ./client [prefix] [x y | quit]
 * Default prefix: /tmp/linux_com_v2
 * Default request: 6 7
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEFAULT_PREFIX "/tmp/linux_com_v2"

static int read_line(int fd, char *buf, size_t cap)
{
    size_t n = 0;

    while (n + 1 < cap) {
        char c;
        ssize_t r = read(fd, &c, 1);
        if (r == 0) {
            buf[n] = '\0';
            return n == 0 ? 0 : 1;
        }
        if (r < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        buf[n++] = c;
        if (c == '\n')
            break;
    }
    buf[n] = '\0';
    return 1;
}

int main(int argc, char **argv)
{
    const char *prefix = DEFAULT_PREFIX;
    const char *payload = "6 7";
    char req_path[512];
    char rep_path[512];
    char line[128];
    char reply[128];
    int req_fd, rep_fd;

    if (argc >= 2)
        prefix = argv[1];
    if (argc == 3)
        payload = argv[2];
    else if (argc == 4) {
        snprintf(line, sizeof line, "%s %s", argv[2], argv[3]);
        payload = line;
    } else if (argc > 4) {
        fprintf(stderr, "usage: %s [prefix] [x y | quit]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (snprintf(req_path, sizeof req_path, "%s.req", prefix) >= (int)sizeof req_path ||
        snprintf(rep_path, sizeof rep_path, "%s.rep", prefix) >= (int)sizeof rep_path) {
        fprintf(stderr, "prefix too long\n");
        return EXIT_FAILURE;
    }

    req_fd = open(req_path, O_WRONLY);
    if (req_fd < 0) {
        perror("open req");
        return EXIT_FAILURE;
    }

    snprintf(reply, sizeof reply, "%s\n", payload);
    if (write(req_fd, reply, strlen(reply)) < 0) {
        perror("write");
        close(req_fd);
        return EXIT_FAILURE;
    }

    /* The server opens the reply FIFO only after it has read the line above. */
    rep_fd = open(rep_path, O_RDONLY);
    if (rep_fd < 0) {
        perror("open rep");
        close(req_fd);
        return EXIT_FAILURE;
    }

    if (read_line(rep_fd, reply, sizeof reply) <= 0) {
        perror("read");
        close(req_fd);
        close(rep_fd);
        return EXIT_FAILURE;
    }

    printf("client: product=%s", reply);
    close(req_fd);
    close(rep_fd);
    return EXIT_SUCCESS;
}
