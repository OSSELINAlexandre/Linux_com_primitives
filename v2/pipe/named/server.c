/*
 * Named pipe server — multiply two integers, one client at a time.
 *
 * Two FIFOs, because a FIFO is one direction:
 *   <prefix>.req   client -> server
 *   <prefix>.rep   server -> client
 *
 * open() for only one direction blocks until the other direction is open.
 * POSIX leaves opening a FIFO O_RDWR undefined. On Linux it succeeds at
 * once, and because this process then holds a write end, a client that
 * disconnects does not look like end-of-file. That is why the request
 * FIFO is opened O_RDWR and kept open: read() waits for the next line
 * instead of returning 0 every time a client exits.
 *
 * The reply FIFO stays one-direction. Order, or both sides block forever:
 *
 *   server: read a line from the request FIFO
 *   client: open request O_WRONLY, write the line, THEN open reply O_RDONLY
 *   server: open reply O_WRONLY, write the product, close that end
 *
 * If the client opens the reply FIFO before writing, the server is stuck
 * in read() and the client is stuck in open(). That is a deadlock.
 *
 * SIGPIPE: the default action is to kill the process when write() hits
 * a FIFO (or a socket) whose read end is already gone. Ignoring it turns
 * that into write() == -1 and errno == EPIPE, which we can print.
 *
 * No SA_RESTART on SIGINT/SIGTERM: a blocked open/read returns EINTR
 * instead of resuming, so the stop flag is actually observed.
 *
 * mkfifo's mode is masked by umask (0666 & ~022 = 0644 with the usual umask).
 *
 * Usage: ./server [prefix]
 * Default prefix: /tmp/linux_com_v2
 * One line is "12 34" or "quit". The newline is the record: a FIFO is a
 * byte stream and does not preserve write() boundaries.
 */

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define DEFAULT_PREFIX "/tmp/linux_com_v2"

static volatile sig_atomic_t stop_flag = 0;

static void on_stop(int signo)
{
    (void)signo;
    stop_flag = 1;
}

static void die(const char *what)
{
    perror(what);
    exit(EXIT_FAILURE);
}

static void install_signals(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof sa);
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGPIPE, &sa, NULL) < 0)
        die("sigaction SIGPIPE");

    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_stop;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, NULL) < 0)
        die("sigaction SIGINT");
    if (sigaction(SIGTERM, &sa, NULL) < 0)
        die("sigaction SIGTERM");
}

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
            if (errno == EINTR) {
                if (stop_flag)
                    return 0;
                continue;
            }
            return -1;
        }
        buf[n++] = c;
        if (c == '\n')
            break;
    }
    buf[n] = '\0';
    return 1;
}

static void strip_nl(char *s)
{
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r'))
        s[--n] = '\0';
}

static int write_all(int fd, const char *buf, size_t len)
{
    size_t off = 0;

    while (off < len) {
        ssize_t n = write(fd, buf + off, len - off);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        off += (size_t)n;
    }
    return 0;
}

int main(int argc, char **argv)
{
    const char *prefix = argc > 1 ? argv[1] : DEFAULT_PREFIX;
    char req_path[512];
    char rep_path[512];
    int req_fd;

    if (argc > 2) {
        fprintf(stderr, "usage: %s [prefix]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (snprintf(req_path, sizeof req_path, "%s.req", prefix) >= (int)sizeof req_path ||
        snprintf(rep_path, sizeof rep_path, "%s.rep", prefix) >= (int)sizeof rep_path) {
        fprintf(stderr, "prefix too long\n");
        return EXIT_FAILURE;
    }

    install_signals();

    unlink(req_path);
    unlink(rep_path);
    /*
     * mkfifo applies ~umask. A typical umask of 022 would turn 0666 into
     * 0644, and another user on the Pi could not write the request FIFO.
     * Clear umask for the two calls, then put it back. errno from mkfifo
     * must be saved across umask(), which itself can succeed and clear it.
     */
    {
        mode_t previous = umask(0);
        int rc1 = mkfifo(req_path, 0666);
        int err1 = errno;
        int rc2 = (rc1 == 0) ? mkfifo(rep_path, 0666) : 0;
        int err2 = errno;
        umask(previous);
        if (rc1 < 0) {
            errno = err1;
            die("mkfifo req");
        }
        if (rc2 < 0) {
            errno = err2;
            die("mkfifo rep");
        }
    }

    printf("server: request FIFO %s\n", req_path);
    printf("server: reply   FIFO %s\n", rep_path);
    fflush(stdout);

    /*
     * O_RDWR: see the comment at the top. Linux does not block, and our
     * own write end keeps read() from returning 0 when a client closes.
     */
    req_fd = open(req_path, O_RDWR);
    if (req_fd < 0)
        die("open req");

    printf("server: waiting for a client\n");
    fflush(stdout);

    while (!stop_flag) {
        char line[128];
        char out[128];
        int rep_fd;
        int x, y;
        int rc;

        rc = read_line(req_fd, line, sizeof line);
        if (rc <= 0) {
            if (rc < 0)
                perror("read");
            break;
        }
        strip_nl(line);

        rep_fd = open(rep_path, O_WRONLY);
        if (rep_fd < 0) {
            if (errno == EINTR && stop_flag)
                break;
            perror("open rep");
            break;
        }

        if (strcmp(line, "quit") == 0)
            snprintf(out, sizeof out, "bye\n");
        else if (sscanf(line, "%d %d", &x, &y) != 2)
            snprintf(out, sizeof out, "error\n");
        else
            snprintf(out, sizeof out, "%d\n", x * y);

        if (write_all(rep_fd, out, strlen(out)) < 0)
            perror("write");
        else
            printf("server: '%s' -> %s", line, out);
        close(rep_fd);
        fflush(stdout);

        if (strcmp(line, "quit") == 0)
            break;
    }

    close(req_fd);

    unlink(req_path);
    unlink(rep_path);
    printf("server: stopped\n");
    return EXIT_SUCCESS;
}
