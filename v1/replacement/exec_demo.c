/*
 * replacement (exec) — smallest demo of the shell pattern.
 *
 * A shell runs a command by forking, then the CHILD calls exec.
 * exec replaces the calling process image. On success it does not return.
 * The pid stays the same, which is why this is a replacement and not a
 * new process. Code after exec runs only when the call failed.
 *
 * The path passed to execv and argv[0] are different things. execv does
 * not search PATH (execvp does). argv must be a NULL-terminated array.
 *
 * An early draft of this repo also exec'd in the parent. That replaces
 * the only process left to wait() for the child, so the child becomes
 * a zombie and the "SHOULD NOT BE PRINTED" line is skipped for a
 * confusing reason. Only the child is replaced here.
 *
 * Build from this directory (worker must sit next to exec_demo):
 *   gcc -std=c11 -D_POSIX_C_SOURCE=200809L -o worker worker.c
 *   gcc -std=c11 -D_POSIX_C_SOURCE=200809L -o exec_demo exec_demo.c
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static int sibling_path(char *out, size_t outlen, const char *argv0, const char *name)
{
    const char *slash = strrchr(argv0, '/');
    int n;

    if (slash == NULL)
        n = snprintf(out, outlen, "%s", name);
    else
        n = snprintf(out, outlen, "%.*s/%s", (int)(slash - argv0), argv0, name);

    if (n < 0 || (size_t)n >= outlen)
        return -1;
    return 0;
}

int main(int argc, char **argv)
{
    char path[4096];
    pid_t pid;

    if (sibling_path(path, sizeof path, argc > 0 ? argv[0] : "exec_demo", "worker") != 0) {
        fprintf(stderr, "worker path is too long\n");
        return EXIT_FAILURE;
    }

    printf("parent: before exec, pid=%ld worker=%s\n", (long)getpid(), path);
    fflush(stdout);

    pid = fork();
    if (pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        char *args[] = {"worker", "replaced", NULL};

        printf("child: about to exec, pid=%ld\n", (long)getpid());
        fflush(stdout);

        execv(path, args);

        /* Reached only if execv failed. errno says why (ENOENT, EACCES, ...). */
        perror("execv");
        _exit(127);
    }

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
        return EXIT_FAILURE;
    }

    if (!WIFEXITED(status)) {
        fprintf(stderr, "parent: child did not exit normally\n");
        return EXIT_FAILURE;
    }

    printf("parent: worker exit status=%d\n", WEXITSTATUS(status));
    return WEXITSTATUS(status) == 42 ? EXIT_SUCCESS : EXIT_FAILURE;
}
