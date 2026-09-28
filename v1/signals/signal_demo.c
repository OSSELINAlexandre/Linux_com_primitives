/*
 * signals — smallest demo that does not race.
 *
 * A signal is a notification delivered to a process, not a shared-memory
 * message. The default action of SIGUSR1 is to kill the process. We block
 * it and take it with sigwait instead, so nothing dies.
 *
 * Why not pause()? The signal can arrive after you tested a flag and
 * before pause(). pause then sleeps forever: the signal was already
 * consumed. Blocking SIGUSR1 first keeps it pending, and sigwait takes
 * that pending signal even if the child sent it before we reached sigwait.
 *
 * sigwait returns 0 on success, or a positive error number. It does not
 * return -1 and it does not set errno (same family of API as pthread).
 *
 * signal() is the older API. On some systems it resets the handler to
 * SIG_DFL when the signal is delivered, so the next one kills you.
 * This demo does not use it.
 *
 * A signal is handled when the process is next scheduled. It is not a
 * synchronous CPU exception: if the process is not running, the handler
 * waits until the kernel elects it again.
 *
 * Build: gcc -std=c11 -D_POSIX_C_SOURCE=200809L -o signal_demo signal_demo.c
 */

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    sigset_t set;
    pid_t pid;
    int signo = 0;
    int rc;

    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);

    /* Block before fork so the child's kill cannot be lost or take the default action. */
    if (sigprocmask(SIG_BLOCK, &set, NULL) < 0) {
        perror("sigprocmask");
        return EXIT_FAILURE;
    }

    fflush(stdout);
    pid = fork();
    if (pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        /* The child inherits the blocked mask. kill still queues the signal on the parent. */
        if (kill(getppid(), SIGUSR1) < 0) {
            perror("kill");
            _exit(EXIT_FAILURE);
        }
        _exit(EXIT_SUCCESS);
    }

    rc = sigwait(&set, &signo);
    if (rc != 0) {
        errno = rc;
        perror("sigwait");
        return EXIT_FAILURE;
    }

    if (waitpid(pid, NULL, 0) < 0) {
        perror("waitpid");
        return EXIT_FAILURE;
    }

    printf("parent: sigwait delivered %d (SIGUSR1=%d)\n", signo, SIGUSR1);
    return signo == SIGUSR1 ? EXIT_SUCCESS : EXIT_FAILURE;
}
