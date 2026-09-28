/*
 * System V message queue server.
 *
 *   ./server [keyfile]
 *   default keyfile: /tmp/linux_com_v2_mq.key
 *
 * ftok(path, proj) builds a key from the file's inode and a project id.
 * The file must exist. Delete it and create it again, and the inode
 * changes, so the client computes a different key and msgget does not
 * find this queue. The project id ('M') is just a char; it is not a mode.
 *
 * A crashed server leaves the queue in the kernel. On startup we remove
 * a queue that already has this key, then create a fresh one with
 * IPC_EXCL so we know we own it. IPC_RMID on the way out is what actually
 * frees it. The process exiting does not.
 *
 * Permissions 0666 let another user on the same Pi (or the same laptop)
 * open the queue. msgget's mode is still filtered by umask.
 *
 * msgrcv blocks. SIGINT/SIGTERM are installed without SA_RESTART, so the
 * call returns EINTR and we can IPC_RMID. msgctl is not async-signal-safe,
 * so the handler only sets a flag; the removal happens in main.
 *
 * Send "quit" as the text to stop the server after it replies "bye".
 */

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <unistd.h>

#include "protocol.h"

#define DEFAULT_KEYFILE "/tmp/linux_com_v2_mq.key"

static volatile sig_atomic_t stop_flag = 0;

static void on_stop(int signo)
{
    (void)signo;
    stop_flag = 1;
}

static void die(const char *what)
{
    perror(what);
    exit(EXIT_FAILURE);
}

static void install_signals(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_stop;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, NULL) < 0)
        die("sigaction SIGINT");
    if (sigaction(SIGTERM, &sa, NULL) < 0)
        die("sigaction SIGTERM");
}

static key_t make_key(const char *path)
{
    int fd;
    key_t key;

    fd = open(path, O_CREAT | O_RDWR, 0666);
    if (fd < 0)
        die("open keyfile");
    close(fd);

    key = ftok(path, 'M');
    if (key == (key_t)-1)
        die("ftok");
    return key;
}

static int create_queue(key_t key)
{
    int qid = msgget(key, 0666);
    if (qid >= 0) {
        if (msgctl(qid, IPC_RMID, NULL) < 0)
            die("msgctl remove leftover");
        printf("server: removed leftover queue %d\n", qid);
    } else if (errno != ENOENT) {
        die("msgget lookup");
    }

    /*
     * The low 9 bits of msgflg are masked by umask, same rule as open().
     * 0666 with umask 022 becomes 0644, so another user cannot msgsnd.
     */
    {
        mode_t previous = umask(0);
        qid = msgget(key, 0666 | IPC_CREAT | IPC_EXCL);
        if (qid < 0) {
            int saved = errno;
            umask(previous);
            errno = saved;
            die("msgget create");
        }
        umask(previous);
    }
    return qid;
}

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : DEFAULT_KEYFILE;
    key_t key;
    int qid;

    if (argc > 2) {
        fprintf(stderr, "usage: %s [keyfile]\n", argv[0]);
        return EXIT_FAILURE;
    }

    install_signals();
    key = make_key(path);
    qid = create_queue(key);
    printf("server: queue %d keyfile %s\n", qid, path);
    fflush(stdout);

    while (!stop_flag) {
        struct mq_request req;
        struct mq_reply rep;
        ssize_t n;

        n = msgrcv(qid, &req, sizeof req - sizeof req.mtype, MQ_REQUEST, 0);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            perror("msgrcv");
            break;
        }
        req.text[MQ_TEXT - 1] = '\0';

        memset(&rep, 0, sizeof rep);
        if (req.reply_to <= 0) {
            fprintf(stderr, "server: ignored request with reply_to=%ld\n", req.reply_to);
            continue;
        }
        rep.mtype = req.reply_to;

        if (strcmp(req.text, "quit") == 0)
            snprintf(rep.text, sizeof rep.text, "bye");
        else
            snprintf(rep.text, sizeof rep.text, "ack: %s", req.text);

        if (msgsnd(qid, &rep, sizeof rep.text, 0) < 0) {
            perror("msgsnd");
            break;
        }
        printf("server: '%s' -> '%s' (reply type %ld)\n", req.text, rep.text, rep.mtype);
        fflush(stdout);

        if (strcmp(req.text, "quit") == 0)
            break;
    }

    if (msgctl(qid, IPC_RMID, NULL) < 0)
        perror("msgctl IPC_RMID");
    else
        printf("server: queue removed\n");

    unlink(path);
    return EXIT_SUCCESS;
}
