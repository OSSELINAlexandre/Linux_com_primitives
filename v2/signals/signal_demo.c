/*
 * signals — sigaction, a flag, and no lost wakeup.
 *
 * signal() is the old API. On some historical systems the handler is
 * reset to SIG_DFL as soon as the signal is delivered, so a second
 * SIGINT kills the process. sigaction keeps the handler and can block
 * other signals while it runs.
 *
 * A handler may only do async-signal-safe work. printf is not on that
 * list: it locks stdio, and if the main program was inside printf the
 * lock is already held. The safe pattern is to store a volatile
 * sig_atomic_t and do the real work back in main.
 *
 * Race: "if (!flag) pause();" loses the signal that arrives between the
 * test and pause(). sigprocmask blocks the signals first. sigsuspend
 * atomically installs a mask that allows them, and on return puts the
 * old mask back. The signal cannot land in the gap.
 *
 * SIGCHLD is how the parent learns a child exited. The child stays a
 * zombie until waitpid. Waiting inside the handler is allowed (waitpid
 * is async-signal-safe); waiting in main after the flag is easier to read.
 * WNOHANG is what you want when you do not know if a child is ready.
 *
 * A signal is delivered the next time the process is scheduled. It is
 * not a synchronous CPU exception.
 *
 * This program arms SIGALRM and forks a child that exits. It stops itself
 * once both have been observed. Ctrl+C is the same flag, if you send it.
 *
 * Build: gcc -std=c11 -D_POSIX_C_SOURCE=200809L -o signal_demo signal_demo.c
 */

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static volatile sig_atomic_t got_alrm = 0;
static volatile sig_atomic_t got_chld = 0;
static volatile sig_atomic_t got_int = 0;

static void on_alrm(int signo)
{
    (void)signo;
    got_alrm = 1;
}

static void on_chld(int signo)
{
    (void)signo;
    got_chld = 1;
}

static void on_int(int signo)
{
    (void)signo;
    got_int = 1;
}

static void install(int signo, void (*handler)(int))
{
    struct sigaction sa;

    memset(&sa, 0, sizeof sa);
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    /* No SA_RESTART: sigsuspend must be able to return when the signal arrives. */
    if (sigaction(signo, &sa, NULL) < 0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}

int main(void)
{
    sigset_t block;
    sigset_t suspended;
    pid_t pid;
    int status = 0;
    int code;

    install(SIGALRM, on_alrm);
    install(SIGCHLD, on_chld);
    install(SIGINT, on_int);

    sigemptyset(&block);
    sigaddset(&block, SIGALRM);
    sigaddset(&block, SIGCHLD);
    sigaddset(&block, SIGINT);
    if (sigprocmask(SIG_BLOCK, &block, NULL) < 0) {
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
        /*
         * The child inherited the blocked mask. Unblock so a stray signal
         * is not stuck pending in a process that is about to exit anyway.
         */
        if (sigprocmask(SIG_UNBLOCK, &block, NULL) < 0)
            _exit(EXIT_FAILURE);
        _exit(EXIT_SUCCESS);
    }

    /* alarm() delivers SIGALRM after one second. It is a process timer. */
    alarm(1);

    sigemptyset(&suspended);
    while (!got_alrm || !got_chld) {
        if (got_int)
            break;
        /*
         * sigsuspend replaces the mask for the duration of the wait.
         * An empty set means SIGALRM, SIGCHLD and SIGINT can be delivered.
         * On return the blocked mask from sigprocmask is restored, so the
         * handlers cannot run again until the next sigsuspend.
         */
        sigsuspend(&suspended);
    }

    /*
     * WNOHANG collects the zombie without blocking when the child has
     * already exited. If it is somehow still running, fall back to a
     * blocking wait so `status` is actually filled in. Do not call
     * waitpid a second time after a successful reap: that returns ECHILD.
     */
    {
        pid_t reaped = waitpid(pid, &status, WNOHANG);
        if (reaped == 0)
            reaped = waitpid(pid, &status, 0);
        if (reaped < 0) {
            perror("waitpid");
            return EXIT_FAILURE;
        }
    }
    if (!WIFEXITED(status)) {
        fprintf(stderr, "child did not exit normally\n");
        return EXIT_FAILURE;
    }
    code = WEXITSTATUS(status);

    printf("parent: alarm and child reaped, status=%d\n", code);
    return (got_alrm && got_chld && code == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
