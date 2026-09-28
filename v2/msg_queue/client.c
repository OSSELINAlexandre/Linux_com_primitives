/*
 * System V message queue client. Start the server first so the keyfile
 * and the queue exist.
 *
 *   ./client [keyfile] [message]
 *   default keyfile: /tmp/linux_com_v2_mq.key
 *   default message: hello
 *
 * reply_to is our pid. The server sends the answer with that mtype, and
 * we receive only that type. Two clients can share the queue without
 * reading each other's replies. mtype 0 is illegal in msgsnd and, in
 * msgrcv, means "any type" — do not use it here.
 *
 * The size passed to msgsnd/msgrcv is the payload after mtype.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

#include "protocol.h"

#define DEFAULT_KEYFILE "/tmp/linux_com_v2_mq.key"

int main(int argc, char **argv)
{
    const char *path = DEFAULT_KEYFILE;
    const char *text = "hello";
    key_t key;
    int qid;
    struct mq_request req;
    struct mq_reply rep;

    if (argc >= 2)
        path = argv[1];
    if (argc >= 3)
        text = argv[2];
    if (argc > 3) {
        fprintf(stderr, "usage: %s [keyfile] [message]\n", argv[0]);
        return EXIT_FAILURE;
    }

    key = ftok(path, 'M');
    if (key == (key_t)-1) {
        perror("ftok");
        fprintf(stderr, "start the server first (it creates %s)\n", path);
        return EXIT_FAILURE;
    }

    qid = msgget(key, 0666);
    if (qid < 0) {
        perror("msgget");
        fprintf(stderr, "start the server first\n");
        return EXIT_FAILURE;
    }

    memset(&req, 0, sizeof req);
    req.mtype = MQ_REQUEST;
    req.reply_to = (long)getpid();
    snprintf(req.text, sizeof req.text, "%s", text);

    if (msgsnd(qid, &req, sizeof req - sizeof req.mtype, 0) < 0) {
        perror("msgsnd");
        return EXIT_FAILURE;
    }

    memset(&rep, 0, sizeof rep);
    if (msgrcv(qid, &rep, sizeof rep.text, req.reply_to, 0) < 0) {
        perror("msgrcv");
        return EXIT_FAILURE;
    }
    rep.text[MQ_TEXT - 1] = '\0';

    printf("client: reply='%s'\n", rep.text);
    return EXIT_SUCCESS;
}
