/*
 * fork — three things the one-call demo cannot show at once.
 *
 * 1. Memory is copied. The child and the parent each have their own `value`.
 * 2. File descriptors are copied, but the open-file description (the offset)
 *    is shared. A write in the child advances the parent's offset too.
 * 3. waitpid's status word is not the exit code. WIFEXITED / WEXITSTATUS
 *    take it apart. A child that dies on a signal does not "exit 0".
 *
 * Return value of fork, same as v1:
 *   -1 error (errno), 0 child, >0 parent and the value is the child pid.
 *
 * Build: gcc -std=c11 -D_POSIX_C_SOURCE=200809L -o fork_demo fork_demo.c
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static void die(const char *what)
{
    perror(what);
    exit(EXIT_FAILURE);
}

static int wait_child(pid_t pid)
{
    int status = 0;

    if (waitpid(pid, &status, 0) < 0)
        die("waitpid");
    if (!WIFEXITED(status)) {
        fprintf(stderr, "child did not exit normally (status=%d)\n", status);
        exit(EXIT_FAILURE);
    }
    return WEXITSTATUS(status);
}

/* Section 1: the child's assignment does not change the parent's variable. */
static void show_memory_copy(void)
{
    int value = 1;
    pid_t pid;
    int child_value = -1;
    int pipefd[2];

    if (pipe(pipefd) < 0)
        die("pipe");

    fflush(stdout);
    pid = fork();
    if (pid < 0)
        die("fork");

    if (pid == 0) {
        close(pipefd[0]);
        value = 100;
        if (write(pipefd[1], &value, sizeof value) < 0)
            _exit(EXIT_FAILURE);
        close(pipefd[1]);
        _exit(EXIT_SUCCESS);
    }

    close(pipefd[1]);
    if (read(pipefd[0], &child_value, sizeof child_value) != (ssize_t)sizeof child_value)
        die("read child value");
    close(pipefd[0]);
    (void)wait_child(pid);

    printf("section memory: parent value=%d child value=%d\n", value, child_value);
    if (value == child_value) {
        fprintf(stderr, "memory was not copied\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * Section 2: inherited fd, shared offset.
 * Parent writes "before\n", forks, child writes "child\n", parent lseeks
 * back to 0. The child's write moved the shared offset, which is why the
 * parent must lseek instead of assuming it is still after "before\n".
 * The file is unlinked while still open so /tmp stays clean; the inode
 * lives as long as the descriptor does.
 */
static void show_fd_inheritance(void)
{
    char path[] = "/tmp/linux_com_fork_XXXXXX";
    char buf[64];
    ssize_t n;
    int fd;
    pid_t pid;

    fd = mkstemp(path);
    if (fd < 0)
        die("mkstemp");
    if (unlink(path) < 0)
        die("unlink");

    if (write(fd, "before\n", 7) != 7)
        die("write before");

    fflush(stdout);
    pid = fork();
    if (pid < 0)
        die("fork");

    if (pid == 0) {
        if (write(fd, "child\n", 6) != 6)
            _exit(EXIT_FAILURE);
        _exit(EXIT_SUCCESS);
    }

    if (wait_child(pid) != 0)
        exit(EXIT_FAILURE);

    if (lseek(fd, 0, SEEK_SET) < 0)
        die("lseek");

    n = read(fd, buf, sizeof buf - 1);
    if (n < 0)
        die("read");
    buf[n] = '\0';
    close(fd);

    printf("section fd: file='%s'\n", buf);
    if (strcmp(buf, "before\nchild\n") != 0) {
        fprintf(stderr, "unexpected file contents\n");
        exit(EXIT_FAILURE);
    }
}

/* Section 3: the status word. The child exits 7 on purpose. */
static void show_wait_status(void)
{
    pid_t pid;
    int code;

    fflush(stdout);
    pid = fork();
    if (pid < 0)
        die("fork");
    if (pid == 0)
        _exit(7);

    code = wait_child(pid);
    printf("section wait: exit status=%d\n", code);
    if (code != 7)
        exit(EXIT_FAILURE);
}

int main(void)
{
    show_memory_copy();
    show_fd_inheritance();
    show_wait_status();
    return EXIT_SUCCESS;
}
