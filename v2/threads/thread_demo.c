/*
 * threads — shared memory is a race, until a mutex makes it one update.
 *
 * `counter++` is a load, an add, and a store. Two threads can load the
 * same value, both add one, and both store it: one increment is lost.
 * That is a data race. In C it is undefined behaviour; on a normal gcc
 * build you still observe a result lower than the number of increments.
 *
 * volatile does NOT fix it. volatile only tells the compiler not to keep
 * the variable in a register. The load/add/store is still not atomic,
 * and there is still no ordering with the other thread.
 *
 * pthread_mutex_lock / unlock make the increment one critical section.
 * The mutex must be taken on every path that touches the counter, including
 * the read that prints it if a writer might still be running. After join,
 * both writers are done, so the printf needs no lock.
 *
 * pthread calls return an error number. They do not set errno.
 *
 * Build: gcc -std=c11 -D_POSIX_C_SOURCE=200809L -pthread -o thread_demo thread_demo.c
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ITERS 200000
#define EXPECTED (ITERS * 2)

static volatile int counter = 0;
static pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;

static void *add_raw(void *arg)
{
    int i;
    (void)arg;
    for (i = 0; i < ITERS; i++)
        counter++;
    return NULL;
}

static void *add_safe(void *arg)
{
    int i;
    int rc;
    (void)arg;
    for (i = 0; i < ITERS; i++) {
        rc = pthread_mutex_lock(&mu);
        if (rc != 0) {
            fprintf(stderr, "pthread_mutex_lock: %s\n", strerror(rc));
            return NULL;
        }
        counter++;
        rc = pthread_mutex_unlock(&mu);
        if (rc != 0) {
            fprintf(stderr, "pthread_mutex_unlock: %s\n", strerror(rc));
            return NULL;
        }
    }
    return NULL;
}

static int run_pair(void *(*fn)(void *))
{
    pthread_t a, b;
    int rc;

    counter = 0;

    rc = pthread_create(&a, NULL, fn, NULL);
    if (rc != 0) {
        fprintf(stderr, "pthread_create: %s\n", strerror(rc));
        exit(EXIT_FAILURE);
    }
    rc = pthread_create(&b, NULL, fn, NULL);
    if (rc != 0) {
        fprintf(stderr, "pthread_create: %s\n", strerror(rc));
        exit(EXIT_FAILURE);
    }

    rc = pthread_join(a, NULL);
    if (rc != 0) {
        fprintf(stderr, "pthread_join: %s\n", strerror(rc));
        exit(EXIT_FAILURE);
    }
    rc = pthread_join(b, NULL);
    if (rc != 0) {
        fprintf(stderr, "pthread_join: %s\n", strerror(rc));
        exit(EXIT_FAILURE);
    }
    return counter;
}

int main(void)
{
    int raw;
    int safe;

    raw = run_pair(add_raw);
    safe = run_pair(add_safe);

    /*
     * raw is often smaller than EXPECTED. It is not a bug in the printer
     * if the two numbers differ: that difference is the lost updates.
     * On a very quiet run raw can accidentally match; safe must always match.
     */
    printf("raw counter=%d expected=%d\n", raw, EXPECTED);
    printf("safe counter=%d expected=%d\n", safe, EXPECTED);

    if (safe != EXPECTED)
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}
