/*
 * Tiny program loaded by exec_demo. After a successful exec the child's
 * pid does not change: only the code, data and stack are replaced.
 *
 * The exit status (42) is what the parent reads with waitpid. It is the
 * proof the new image ran to the end.
 */

#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    printf("worker: replaced image, same pid=%ld arg=%s\n",
           (long)getpid(), argc > 1 ? argv[1] : "(none)");
    return 42;
}
