/*
 * threads — smallest demo.
 *
 * A thread is another line of execution inside the SAME process.
 * Same pid, same data segment: `shared` is not copied the way fork copies it.
 * The stack and the registers are per-thread.
 *
 * pthread_create's start routine must be `void *(*)(void *)`.
 * It returns 0 on success, or a positive error number. It does not return
 * -1 and it does not set errno (unlike fork). Comparing the result to -1
 * misses every real failure.
 *
 * Build: gcc -std=c11 -D_POSIX_C_SOURCE=200809L -pthread -o thread_demo thread_demo.c
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int shared = 0;

static void *worker(void *arg)
{
    (void)arg;
    printf("thread: pid=%ld shared_before=%d\n", (long)getpid(), shared);
    shared = 42;
    printf("thread: shared_after=%d\n", shared);
    return NULL;
}

int main(void)
{
    pthread_t thread;
    int rc;

    printf("main: pid=%ld shared=%d\n", (long)getpid(), shared);

    rc = pthread_create(&thread, NULL, worker, NULL);
    if (rc != 0) {
        fprintf(stderr, "pthread_create: %s\n", strerror(rc));
        return EXIT_FAILURE;
    }

    /*
     * join waits until the thread returns and reaps it.
     * Without join, main can exit first and kill the thread mid-update.
     * Like pthread_create, join returns an error number, not -1.
     */
    rc = pthread_join(thread, NULL);
    if (rc != 0) {
        fprintf(stderr, "pthread_join: %s\n", strerror(rc));
        return EXIT_FAILURE;
    }

    printf("main: after join shared=%d (same address space)\n", shared);
    return shared == 42 ? EXIT_SUCCESS : EXIT_FAILURE;
}
