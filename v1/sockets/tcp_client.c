/*
 * TCP client — smallest demo. Connects to 127.0.0.1:7777.
 *
 * htons = host to network short. Multi-byte integers on the wire are
 * big-endian ("network order"). x86_64 and the Raspberry Pi (aarch64)
 * are both little-endian, so a port written as a plain integer would
 * be byte-swapped and the peer would listen on a different port.
 * 7777 in memory is 0x1E61; on the wire it must be 0x1E 0x61.
 *
 * inet_pton already writes the address in network order. Do not also
 * call htonl on its result. inet_addr is worse: its error value is
 * 255.255.255.255, so a real broadcast address looks like a failure.
 *
 * The same off-by-one as the old server lived here too:
 * Buffer[1024] = '/0' writes past the array and is not a NUL.
 *
 * Build: gcc -std=c11 -D_POSIX_C_SOURCE=200809L -o tcp_client tcp_client.c
 * Run the server first, then: ./tcp_client
 */

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 7777

int main(void)
{
    int fd;
    struct sockaddr_in addr;
    char buf[1024];
    const char *msg = "hello from client";
    ssize_t n;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) != 1) {
        fprintf(stderr, "inet_pton failed\n");
        close(fd);
        return EXIT_FAILURE;
    }

    if (connect(fd, (struct sockaddr *)&addr, sizeof addr) < 0) {
        perror("connect");
        close(fd);
        return EXIT_FAILURE;
    }

    if (send(fd, msg, strlen(msg), 0) < 0) {
        perror("send");
        close(fd);
        return EXIT_FAILURE;
    }

    n = recv(fd, buf, sizeof buf - 1, 0);
    if (n < 0) {
        perror("recv");
        close(fd);
        return EXIT_FAILURE;
    }
    buf[n] = '\0';
    printf("client: got '%s'\n", buf);

    close(fd);
    return EXIT_SUCCESS;
}
