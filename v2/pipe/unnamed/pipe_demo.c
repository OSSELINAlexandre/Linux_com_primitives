/*
 * unnamed pipes — request in one pipe, answer in the other.
 *
 * A single pipe is one direction. Two pipes give a request/response
 * without the processes stepping on each other.
 *
 *   parent --p2c--> child
 *   parent <--c2p-- child
 *
 * After fork, close the end you do not use in EACH process. The EOF on
 * a pipe is "no write ends remain". A forgotten write end in the reader
 * turns into a permanent block, which looks like a deadlock.
 *
 * Pipes are byte streams, same idea as TCP. One write of 8 bytes can be
 * read back as 8, or in theory as two shorter reads. A newline frames
 * one message so the reader knows when to stop. Writes smaller than
 * PIPE_BUF (4096 on Linux) are not interleaved with other writers; that
 * does not mean one read returns the whole write.
 *
 * Usage: ./pipe_demo [a] [b]     (default 3 and 4, prints the sum)
 *
 * Build: gcc -std=c11 -D_POSIX_C_SOURCE=200809L -o pipe_demo pipe_demo.c
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static void die(const char *what)
{
    perror(what);
    exit(EXIT_FAILURE);
}

static int parse_int(const char *s, int *out)
{
    char *end = NULL;
    long v;

    errno = 0;
    v = strtol(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0')
        return -1;
    *out = (int)v;
    return 0;
}

/* Read one line. Returns 1 on a line, 0 on EOF with no data, -1 on error. */
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
    int a = 3;
    int b = 4;
    int p2c[2];
    int c2p[2];
    pid_t pid;
    char line[64];
    int sum = 0;

    if (argc == 3) {
        if (parse_int(argv[1], &a) != 0 || parse_int(argv[2], &b) != 0) {
            fprintf(stderr, "usage: %s [a b]\n", argv[0]);
            return EXIT_FAILURE;
        }
    } else if (argc != 1) {
        fprintf(stderr, "usage: %s [a b]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (pipe(p2c) < 0 || pipe(c2p) < 0)
        die("pipe");

    fflush(stdout);
    pid = fork();
    if (pid < 0)
        die("fork");

    if (pid == 0) {
        int x = 0;
        int y = 0;
        int rc;

        close(p2c[1]);
        close(c2p[0]);

        rc = read_line(p2c[0], line, sizeof line);
        if (rc <= 0)
            _exit(EXIT_FAILURE);
        if (sscanf(line, "%d %d", &x, &y) != 2)
            _exit(EXIT_FAILURE);

        snprintf(line, sizeof line, "%d\n", x + y);
        if (write(c2p[1], line, strlen(line)) < 0)
            _exit(EXIT_FAILURE);

        close(p2c[0]);
        close(c2p[1]);
        _exit(EXIT_SUCCESS);
    }

    close(p2c[0]);
    close(c2p[1]);

    snprintf(line, sizeof line, "%d %d\n", a, b);
    if (write(p2c[1], line, strlen(line)) < 0)
        die("write");
    close(p2c[1]); /* child read_line can finish; we are done sending */

    if (read_line(c2p[0], line, sizeof line) <= 0)
        die("read_line");
    close(c2p[0]);

    if (sscanf(line, "%d", &sum) != 1) {
        fprintf(stderr, "bad reply '%s'\n", line);
        return EXIT_FAILURE;
    }

    {
        int status = 0;
        if (waitpid(pid, &status, 0) < 0)
            die("waitpid");
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
            return EXIT_FAILURE;
    }

    printf("sum=%d\n", sum);
    return sum == a + b ? EXIT_SUCCESS : EXIT_FAILURE;
}
