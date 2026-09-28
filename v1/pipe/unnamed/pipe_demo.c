/*
 * unnamed pipe — smallest demo.
 *
 * pipe() creates a one-way byte stream in the kernel and two descriptors:
 *   fd[0] is the read end, fd[1] is the write end.
 * The pipe has no name. The only way another process gets it is by
 * inheritance across fork (or by passing the fd another way).
 *
 * After fork, BOTH processes own BOTH ends. A read returns 0 (end of
 * file) only when EVERY write end is closed. If the reader forgets to
 * close its own unused write end, it never sees EOF and blocks forever,
 * even after the writer has closed its end. Close the end you do not use.
 *
 * read() may return fewer bytes than asked. It does not wait to fill the
 * buffer. Loop until EOF when the close is the end of the message.
 * A pipe is a byte stream, not a C string: the NUL is not sent for you.
 *
 * Build: gcc -std=c11 -D_POSIX_C_SOURCE=200809L -o pipe_demo pipe_demo.c
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    int fd[2];
    pid_t pid;
    const char *msg = "hello from parent";

    if (pipe(fd) < 0) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    fflush(stdout);
    pid = fork();
    if (pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        char buf[64];
        size_t total = 0;

        close(fd[1]); /* child only reads; drop the write end */

        while (total < sizeof buf - 1) {
            ssize_t n = read(fd[0], buf + total, sizeof buf - 1 - total);
            if (n == 0)
                break; /* every writer closed: that is the end of the message */
            if (n < 0) {
                if (errno == EINTR)
                    continue;
                perror("read");
                _exit(EXIT_FAILURE);
            }
            total += (size_t)n;
        }
        buf[total] = '\0';
        close(fd[0]);

        printf("child: received '%s'\n", buf);
        fflush(stdout);
        _exit(EXIT_SUCCESS);
    }

    close(fd[0]); /* parent only writes; drop the read end */

    if (write(fd[1], msg, strlen(msg)) < 0) {
        perror("write");
        return EXIT_FAILURE;
    }

    /*
     * Closing the last write end unblocks the child's read with EOF.
     * Leaving it open makes the child wait for more bytes that never come.
     */
    close(fd[1]);

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
        return EXIT_FAILURE;
    }

    printf("parent: child done\n");
    return (WIFEXITED(status) && WEXITSTATUS(status) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
