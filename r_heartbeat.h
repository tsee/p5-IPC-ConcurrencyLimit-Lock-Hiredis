#ifndef R_HEARTBEAT_H_
#define R_HEARTBEAT_H_

#include <pthread.h>

typedef int r_command_t;
#define RCMD_NONE 0
#define RCMD_TIMER 1
#define RCMD_INIT 2
#define RCMD_HALT 3
#define RCMD_PING 4

typedef int r_option_t;
#define RCMD_OPT_NORM   0
#define RCMD_OPT_FORCE  1

typedef void *r_app_state_ptr_t;
typedef void (*r_app_callback_t)(r_app_state_ptr_t app_state, r_command_t cmd);

typedef struct rheartbeat {
    pthread_t heartbeat_thread;
    pthread_mutex_t cmd_mutex;
    pthread_cond_t cmd_condvar; 
    r_command_t command;
    unsigned int interval; /* hb interval in s */
    r_app_state_ptr_t application_state;
    r_app_callback_t application_callback;
} rheartbeat_t;

int rhb_create(rheartbeat_t **hb, unsigned int hb_interval_ms, r_app_state_ptr_t app_state,
               r_app_callback_t app_callback);
int rhb_execute(rheartbeat_t *hb);
int rhb_finish(rheartbeat_t *hb);
int rhb_send_cmd(rheartbeat_t *hb, r_command_t cmd, r_option_t flags);

#endif
