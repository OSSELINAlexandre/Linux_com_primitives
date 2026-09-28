/*
 * TCP server for two machines on the same LAN (laptop <-> Raspberry Pi).
 *
 *   ./tcp_server [bind_addr] [port]
 *   BIND_ADDR=0.0.0.0 PORT=7777 ./tcp_server
 *
 * bind_addr chooses the local interface:
 *   0.0.0.0      every IPv4 interface (use this on the Pi)
 *   127.0.0.1    loopback only; another machine cannot connect
 *   192.168.x.x  that one address
 *
 * argv overrides the environment. Default is 0.0.0.0:7777.
 *
 * htons: the port is a 16-bit integer. On the wire multi-byte values are
 * big-endian. x86_64 and aarch64 (the Pi) are little-endian, so a bare
 * assignment of 7777 would publish a different port. htons does the swap
 * when the host needs it, and is a no-op on a big-endian host.
 * inet_pton writes the address ALREADY in network order. Do not htonl it.
 *
 * SO_REUSEADDR: after a previous server died, the port can sit in
 * TIME_WAIT and bind fails with EADDRINUSE. This option allows the new
 * listen socket to bind anyway. It is not a mutex and it does not make
 * two servers share a port safely; it makes a restart work.
 *
 * TCP is a stream. One send can arrive as several recv, and several sends
 * can arrive as one recv. A newline is the record boundary. recv does not
 * NUL-terminate; we do, and we leave one byte for that NUL
 * (the old Buffer[1024] = '/0' wrote past the array and was not a NUL).
 *
 * The accept loop is the server. SIGINT/SIGTERM set a flag. Handlers stay
 * async-signal-safe (flag only). SA_RESTART is off so accept/recv return
 * EINTR and the flag is noticed; with SA_RESTART the call would resume
 * and Ctrl+C would look ignored.
 *
 * SIGPIPE is ignored so a client that disconnects mid-reply becomes
 * EPIPE on send, not a dead server.
 */

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define DEFAULT_ADDR "0.0.0.0"
#define DEFAULT_PORT "7777"

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

static const char *env_or(const char *key, const char *fallback)
{
    const char *v = getenv(key);
    if (v == NULL || v[0] == '\0')
        return fallback;
    return v;
}

static int parse_port(const char *s, unsigned short *out)
{
    char *end = NULL;
    long port;

    errno = 0;
    port = strtol(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0' || port <= 0 || port > 65535)
        return -1;
    *out = (unsigned short)port;
    return 0;
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
        ssize_t r = recv(fd, &c, 1, 0);
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

static int send_all(int fd, const char *buf, size_t len)
{
    size_t off = 0;

    while (off < len) {
        ssize_t n = send(fd, buf + off, len - off, 0);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (n == 0)
            return -1;
        off += (size_t)n;
    }
    return 0;
}

static void serve_client(int client_fd, const struct sockaddr_in *peer)
{
    char ip[INET_ADDRSTRLEN];
    char line[1024];
    char reply[1200];
    int rc;

    if (inet_ntop(AF_INET, &peer->sin_addr, ip, sizeof ip) == NULL) {
        perror("inet_ntop");
        return;
    }

    /*
     * ntohs is the inverse of htons: the port in sockaddr_in is in
     * network order, and printing it raw on a little-endian machine
     * shows the byte-swapped number.
     */
    printf("server: connection from %s:%u\n", ip, (unsigned)ntohs(peer->sin_port));

    rc = read_line(client_fd, line, sizeof line);
    if (rc <= 0) {
        if (rc < 0)
            perror("recv");
        return;
    }

    /* Drop the framing newline so the log and the echo are one clean line. */
    {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n')
            line[len - 1] = '\0';
    }

    printf("server: got '%s'\n", line);
    snprintf(reply, sizeof reply, "echo: %s\n", line);
    if (send_all(client_fd, reply, strlen(reply)) < 0)
        perror("send");
}

int main(int argc, char **argv)
{
    const char *bind_addr = env_or("BIND_ADDR", DEFAULT_ADDR);
    const char *port_s = env_or("PORT", DEFAULT_PORT);
    unsigned short port = 0;
    int listen_fd;
    int yes = 1;
    struct sockaddr_in addr;

    if (argc >= 2)
        bind_addr = argv[1];
    if (argc >= 3)
        port_s = argv[2];
    if (argc > 3) {
        fprintf(stderr, "usage: %s [bind_addr] [port]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (parse_port(port_s, &port) != 0) {
        fprintf(stderr, "bad port '%s'\n", port_s);
        return EXIT_FAILURE;
    }

    install_signals();

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0)
        die("socket");

    /*
     * The option argument is a pointer to an int, not the int itself.
     * Set it before bind, or it does not affect this bind.
     */
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) < 0)
        die("setsockopt SO_REUSEADDR");

    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, bind_addr, &addr.sin_addr) != 1) {
        fprintf(stderr, "bad bind address '%s' (IPv4 dotted quad, or 0.0.0.0)\n", bind_addr);
        close(listen_fd);
        return EXIT_FAILURE;
    }

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof addr) < 0)
        die("bind");
    if (listen(listen_fd, 16) < 0)
        die("listen");

    printf("server: listening on %s:%u\n", bind_addr, (unsigned)port);
    fflush(stdout);

    while (!stop_flag) {
        struct sockaddr_in peer;
        socklen_t peer_len = sizeof peer;
        int client_fd = accept(listen_fd, (struct sockaddr *)&peer, &peer_len);

        if (client_fd < 0) {
            if (errno == EINTR)
                continue;
            perror("accept");
            break;
        }

        serve_client(client_fd, &peer);
        close(client_fd);
    }

    close(listen_fd);
    printf("server: stopped\n");
    return EXIT_SUCCESS;
}
