#ifndef R_HEARTBEAT_H_
#define R_HEARTBEAT_H_

#include <pthread.h>

typedef int r_command_t;
#define RCMD_NONE 0
#define RCMD_HALT 1
#define RCMD_PING 2

#define RCMD_OPT_NORM   0
#define RCMD_OPT_FORCE  1

typedef struct rheartbeat {
    pthread_t heartbeat_thread;
    pthread_mutex_t access_mutex;
    pthread_cond_t cmd_cond_var; 
    r_command_t command;
    unsigned int interval; /* hb interval in s */
} rheartbeat_t;

int rhb_create(rheartbeat_t **hb, unsigned int hb_interval_ms);
int rhb_execute(rheartbeat_t *hb);
int rhb_finish(rheartbeat_t *hb);
int rhb_send_cmd(rheartbeat_t *hb, r_command_t cmd, int flags);

#endif
