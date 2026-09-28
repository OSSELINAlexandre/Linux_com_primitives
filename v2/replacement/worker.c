/*
 * Image loaded by exec_demo via execve.
 *
 * execve replaces the environment as well as the code. Variables that
 * were in the parent are gone unless the parent put them in envp.
 * DEMO is set only because exec_demo passed it. PATH is not, on purpose.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    const char *demo = getenv("DEMO");

    printf("worker: pid=%ld ppid=%ld argc=%d argv1=%s DEMO=%s\n",
           (long)getpid(),
           (long)getppid(),
           argc,
           argc > 1 ? argv[1] : "(none)",
           demo != NULL ? demo : "(null)");
    return 42;
}
