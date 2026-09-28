/*
 * replacement — fork + execve, the call a shell actually makes.
 *
 * execve(path, argv, envp):
 *   - path is where the kernel loads the file from. It is not searched in PATH.
 *   - argv is what the new image sees. argv[0] is a convention (the name),
 *     not the path. The array MUST end with NULL. A missing NULL makes
 *     exec read off the end of the array.
 *   - envp replaces the environment. It is not merged with the parent's.
 *     A NULL entry ends it, same rule as argv.
 *
 * On success execve does not return: the bytes after the call are gone.
 * On failure it returns -1 and sets errno (ENOENT, EACCES, ENOEXEC, ...).
 * The child then exits 127, the conventional "could not exec" status,
 * so the parent can tell a failed exec from the worker's status 42.
 *
 * Open file descriptors survive exec unless FD_CLOEXEC / O_CLOEXEC is set.
 * This demo does not leave extra descriptors open.
 *
 * Build both binaries; worker must sit next to exec_demo.
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
    int status = 0;

    if (sibling_path(path, sizeof path, argc > 0 ? argv[0] : "exec_demo", "worker") != 0) {
        fprintf(stderr, "worker path is too long\n");
        return EXIT_FAILURE;
    }

    printf("parent: pid=%ld will exec %s\n", (long)getpid(), path);
    fflush(stdout);

    pid = fork();
    if (pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        char *args[] = {"worker", "from-parent", NULL};
        char *envp[] = {"DEMO=raspberry", NULL};

        execve(path, args, envp);
        perror("execve");
        _exit(127);
    }

    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
        return EXIT_FAILURE;
    }

    if (!WIFEXITED(status)) {
        fprintf(stderr, "parent: child did not exit normally\n");
        return EXIT_FAILURE;
    }

    printf("parent: worker exit status=%d\n", WEXITSTATUS(status));
    if (WEXITSTATUS(status) == 127) {
        fprintf(stderr, "parent: exec failed inside the child\n");
        return EXIT_FAILURE;
    }
    return WEXITSTATUS(status) == 42 ? EXIT_SUCCESS : EXIT_FAILURE;
}
