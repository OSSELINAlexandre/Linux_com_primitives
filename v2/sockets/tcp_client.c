/*
 * TCP client for the v2 server. Same machine or another machine on the LAN.
 *
 *   ./tcp_client [host] [port] [message]
 *   HOST=192.168.1.42 PORT=7777 ./tcp_client
 *
 * argv overrides the environment. Default is 127.0.0.1:7777 and the
 * message "hello from client".
 *
 * host is an IPv4 address, not a DNS name. inet_pton is used on purpose
 * so the htons below stays visible. inet_pton returns 1 on success,
 * 0 if the string is not an address, -1 if the address family is bad.
 * inet_addr is not used: its error code is 255.255.255.255, so you cannot
 * tell a failure from the broadcast address.
 *
 * The message is one line. We add the newline if the caller did not,
 * because the server reads until '\n'. send() may send only part of the
 * buffer; send_all loops until every byte is queued or an error happens.
 * recv has the same short-read rule, so the reply is read until '\n' too.
 *
 * On the Pi, 127.0.0.1 is the Pi itself. Point HOST at the other machine.
 */

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT "7777"

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
    const char *host = env_or("HOST", DEFAULT_HOST);
    const char *port_s = env_or("PORT", DEFAULT_PORT);
    const char *message = "hello from client";
    unsigned short port = 0;
    int fd;
    struct sockaddr_in addr;
    char payload[1024];
    char reply[1200];
    int n;

    if (argc >= 2)
        host = argv[1];
    if (argc >= 3)
        port_s = argv[2];
    if (argc >= 4)
        message = argv[3];
    if (argc > 4) {
        fprintf(stderr, "usage: %s [host] [port] [message]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (parse_port(port_s, &port) != 0) {
        fprintf(stderr, "bad port '%s'\n", port_s);
        return EXIT_FAILURE;
    }

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
        fprintf(stderr, "bad IPv4 address '%s'\n", host);
        close(fd);
        return EXIT_FAILURE;
    }

    if (connect(fd, (struct sockaddr *)&addr, sizeof addr) < 0) {
        perror("connect");
        close(fd);
        return EXIT_FAILURE;
    }

    n = snprintf(payload, sizeof payload, "%s", message);
    if (n < 0 || (size_t)n >= sizeof payload) {
        fprintf(stderr, "message too long\n");
        close(fd);
        return EXIT_FAILURE;
    }
    if (n == 0 || payload[n - 1] != '\n') {
        if ((size_t)n + 1 >= sizeof payload) {
            fprintf(stderr, "message too long\n");
            close(fd);
            return EXIT_FAILURE;
        }
        payload[n++] = '\n';
        payload[n] = '\0';
    }

    if (send_all(fd, payload, (size_t)n) < 0) {
        perror("send");
        close(fd);
        return EXIT_FAILURE;
    }

    /* Half-close the write side so a server that reads until EOF still finishes. */
    if (shutdown(fd, SHUT_WR) < 0) {
        perror("shutdown");
        close(fd);
        return EXIT_FAILURE;
    }

    if (read_line(fd, reply, sizeof reply) <= 0) {
        perror("recv");
        close(fd);
        return EXIT_FAILURE;
    }

    /* The server's newline is the frame, not part of the text we print. */
    {
        size_t len = strlen(reply);
        if (len > 0 && reply[len - 1] == '\n')
            reply[len - 1] = '\0';
    }

    printf("client: reply='%s'\n", reply);
    close(fd);
    return EXIT_SUCCESS;
}
