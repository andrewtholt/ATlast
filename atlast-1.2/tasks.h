/*
 * tasks.h
 *
 *  Created on: Nov 15, 2016
 *      Author: andrew.holt
 */
#ifndef TASKS_H_   //1
#define TASKS_H_

#ifdef LINUX
#include <mqueue.h>
#define SENDER_SIZE 32
#endif
#ifdef FREERTOS  // 2
#include "cmsis_os.h"
#endif //2


#define MAX_CMD 8       // Length of longest command i.e. UNSUB
#define MAX_VALUE 32    // Length of longest value.
#define MAX_KEY   16    // Length of longest key.

#ifdef FREERTOS // 2
// QueueHandle_t getQid(const enum appTask tid) D or the given task
// void setQid(const enum appTask tid, const QueueHandle_t q) ;

struct taskData {
	osMutexId lock;
	QueueHandle_t iam;
};

enum appTask {
	NO_TASK=0,
	TST_HARNESS,
	DUMMY_TASK,
	TST_RX,
	LAST_TASK
};
#
#endif // 2
struct payload {
    uint8_t fields; // <= 3
    char cmd[MAX_CMD];
    char key[MAX_KEY];
    char value[MAX_VALUE];
};

#ifdef FREERTOS // 2
struct cmdMessage {
    QueueHandle_t sender;
    struct payload message;
};
#endif //2

#ifdef LINUX
struct cmdMessage {
    char sender[SENDER_SIZE];
    struct payload message;
};
#endif

#ifdef LINUX
struct client {
    char name[32];
    mqd_t pipe;
};
#else
struct client {
    QueueHandle_t pipe;
};
#endif

#endif
