/*
 * fork — smallest demo.
 *
 * Look at the return value. fork() returns twice, once in each process:
 *   -1  error, no child exists (errno is EAGAIN or ENOMEM, typically)
 *    0  this process is the child
 *   >0  this process is the parent; the value is the child's pid
 *
 * The child is a copy of the parent at the moment of the call (the kernel
 * uses copy-on-write). Changing `value` in the child does not change it
 * in the parent.
 *
 * Build: gcc -std=c11 -D_POSIX_C_SOURCE=200809L -o fork_demo fork_demo.c
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    int value = 1;
    pid_t pid;

    /*
     * stdout is block-buffered when it is not a terminal. Data still
     * sitting in that buffer is copied by fork and printed twice.
     * Flush before the split.
     */
    printf("parent: about to fork, value=%d\n", value);
    fflush(stdout);

    pid = fork();
    if (pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        value = 2;
        printf("child: pid=%ld ppid=%ld value=%d\n",
               (long)getpid(), (long)getppid(), value);
        fflush(stdout);
        /*
         * _exit does not flush stdio and does not run atexit handlers
         * registered by the parent. exit() would. We already flushed.
         */
        _exit(EXIT_SUCCESS);
    }

    printf("parent: pid=%ld child=%ld value=%d\n",
           (long)getpid(), (long)pid, value);

    /*
     * Without wait/waitpid the child stays a zombie until the parent
     * exits: it is dead, but its pid entry is kept so the parent can
     * still collect the status.
     */
    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
        return EXIT_FAILURE;
    }

    if (WIFEXITED(status)) {
        printf("parent: child exit status=%d value=%d (still the parent's copy)\n",
               WEXITSTATUS(status), value);
    } else {
        printf("parent: child did not exit normally\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
