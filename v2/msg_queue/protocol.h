/*
 * One System V queue, two message types.
 *
 * The early theater draft in this repo did the same thing: consultation
 * and reservation shared a queue and were told apart by mtype. That draft
 * also started threads that all called msgrcv on the queue, so two threads
 * could race for the same message. Here the server is one loop, and the
 * reply type is the client's pid, so two clients do not steal each other's
 * answers.
 *
 * mtype must be > 0. msgrcv(..., type, ...) with type > 0 takes the first
 * message of that type. type 0 would take the oldest message of any type,
 * which is how a client would accidentally read another client's reply.
 *
 * The msgsnd/msgrcv size is the bytes after mtype, not sizeof(struct).
 */

#ifndef LINUX_COM_MQ_PROTOCOL_H
#define LINUX_COM_MQ_PROTOCOL_H

#define MQ_TEXT 128
#define MQ_REQUEST 1L

struct mq_request {
    long mtype;       /* MQ_REQUEST when sent to the server */
    long reply_to;    /* client's pid, used as the reply mtype */
    char text[MQ_TEXT];
};

struct mq_reply {
    long mtype;       /* reply_to */
    char text[MQ_TEXT];
};

#endif
