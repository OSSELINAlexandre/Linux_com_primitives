/*
 * TCP server — smallest demo, one client, then exit. Loopback only.
 *
 * See tcp_client.c for htons. This file shows the matching accept path
 * and the buffer bug from the first draft of this repo:
 *
 *     char Buffer[1024];
 *     recv(..., 1023, ...);
 *     Buffer[1024] = '/0';
 *
 * Two separate mistakes:
 *   - index 1024 is past the last valid slot of a 1024-byte array
 *   - '/0' is not the NUL byte. The NUL byte is '\0'
 * recv does not write a terminator. Store '\0' at index `n`, and pass
 * sizeof(buf) - 1 so that index stays inside the array.
 *
 * TCP is a stream. One recv can return fewer bytes than the peer sent.
 * V1 assumes a short message that arrives in one recv. v2/sockets loops.
 *
 * Build: gcc -std=c11 -D_POSIX_C_SOURCE=200809L -o tcp_server tcp_server.c
 * Run:   ./tcp_server          (127.0.0.1:7777)
 *        ./tcp_client
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
    int listen_fd, client_fd;
    struct sockaddr_in addr;
    struct sockaddr_in peer;
    socklen_t peer_len;
    char buf[1024];
    ssize_t n;

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    /*
     * Without SO_REUSEADDR, restarting the server while the previous
     * connection is still in TIME_WAIT fails with EADDRINUSE. v2/sockets
     * explains the option; it is set here so the demo can be run twice.
     */
    {
        int yes = 1;
        if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) < 0) {
            perror("setsockopt");
            close(listen_fd);
            return EXIT_FAILURE;
        }
    }

    /*
     * Zero the address. sockaddr_in has padding; leftover stack bytes
     * make bind fail on some stacks with EINVAL.
     * 127.0.0.1 is this machine only. v2/sockets takes the address as a
     * parameter so a Raspberry Pi can listen on the LAN.
     * inet_pton stores the address already in network order.
     */
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) != 1) {
        fprintf(stderr, "inet_pton failed\n");
        close(listen_fd);
        return EXIT_FAILURE;
    }

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof addr) < 0) {
        perror("bind");
        close(listen_fd);
        return EXIT_FAILURE;
    }

    /* listen() marks the socket as passive. The backlog is the pending-accept queue. */
    if (listen(listen_fd, 1) < 0) {
        perror("listen");
        close(listen_fd);
        return EXIT_FAILURE;
    }

    printf("server: listening on 127.0.0.1:%d\n", PORT);
    fflush(stdout);

    /*
     * peer_len is in-out. You pass the size of the buffer; the kernel
     * writes the size it actually used. Starting from 0 drops the address.
     */
    peer_len = sizeof peer;
    client_fd = accept(listen_fd, (struct sockaddr *)&peer, &peer_len);
    if (client_fd < 0) {
        perror("accept");
        close(listen_fd);
        return EXIT_FAILURE;
    }

    n = recv(client_fd, buf, sizeof buf - 1, 0);
    if (n < 0) {
        perror("recv");
        close(client_fd);
        close(listen_fd);
        return EXIT_FAILURE;
    }
    buf[n] = '\0';
    printf("server: got '%s'\n", buf);

    {
        const char *reply = "hello from server";
        if (send(client_fd, reply, strlen(reply), 0) < 0)
            perror("send");
    }

    close(client_fd);
    close(listen_fd);
    return EXIT_SUCCESS;
}
