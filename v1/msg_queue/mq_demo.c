/*
 * System V message queue — smallest demo.
 *
 * A queue lives in the kernel, identified by an integer id, until someone
 * calls msgctl(IPC_RMID) or the machine reboots. It is not freed when the
 * process exits. Forgetting IPC_RMID leaks the queue (see ipcs -q / ipcrm -q).
 *
 * IPC_PRIVATE does not mean "only this process". It means "create a new
 * queue; do not look up an existing key". The id is just an integer, so
 * the child inherits it through the memory copy of fork and can use the
 * same kernel object. An unrelated process cannot name it.
 *
 * Every message starts with a long mtype that MUST be > 0.
 * msgrcv's size argument is the bytes AFTER mtype, not including it.
 * msgrcv with type 0 would take the oldest message of any type. Here the
 * parent asks for type 1, the type the child sends.
 *
 * Build: gcc -std=c11 -D_POSIX_C_SOURCE=200809L -o mq_demo mq_demo.c
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <unistd.h>

struct mq_msg {
    long mtype;
    char text[64];
};

int main(void)
{
    int qid;
    pid_t pid;
    struct mq_msg msg;

    qid = msgget(IPC_PRIVATE, 0600 | IPC_CREAT);
    if (qid < 0) {
        perror("msgget");
        return EXIT_FAILURE;
    }

    fflush(stdout);
    pid = fork();
    if (pid < 0) {
        perror("fork");
        msgctl(qid, IPC_RMID, NULL);
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        memset(&msg, 0, sizeof msg);
        msg.mtype = 1;
        snprintf(msg.text, sizeof msg.text, "hello from child");

        /*
         * Blocking send. The size excludes mtype. A size larger than the
         * queue's limit fails with EINVAL; a full queue waits.
         */
        if (msgsnd(qid, &msg, sizeof msg.text, 0) < 0) {
            perror("msgsnd");
            _exit(EXIT_FAILURE);
        }
        _exit(EXIT_SUCCESS);
    }

    memset(&msg, 0, sizeof msg);
    if (msgrcv(qid, &msg, sizeof msg.text, 1, 0) < 0) {
        perror("msgrcv");
        msgctl(qid, IPC_RMID, NULL);
        return EXIT_FAILURE;
    }

    printf("parent: queue id=%d text='%s'\n", qid, msg.text);

    if (waitpid(pid, NULL, 0) < 0)
        perror("waitpid");

    if (msgctl(qid, IPC_RMID, NULL) < 0) {
        perror("msgctl IPC_RMID");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
