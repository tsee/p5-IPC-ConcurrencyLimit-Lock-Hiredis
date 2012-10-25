#ifndef HEARTBEAT_H_
#define HEARTBEAT_H_

#include <pthread.h>

typedef int hb_command_t;
#define HBCMD_NONE 0
#define HBCMD_TIMER 1
#define HBCMD_INIT 2
#define HBCMD_HALT 3
#define HBCMD_PING 4

#define HBCMD_LAST 15

typedef int hb_option_t;
#define HBCMD_OPT_NORM   0
#define HBCMD_OPT_FORCE  1

typedef void *hb_app_state_ptr_t;
typedef void (*hb_app_callback_t)(hb_app_state_ptr_t app_state, hb_command_t cmd);

typedef struct heartbeat {
    pthread_t heartbeat_thread;
    pthread_mutex_t cmd_mutex;
    pthread_cond_t cmd_condvar; 
    hb_command_t command;
    unsigned int interval; /* hb interval in s */
    hb_app_state_ptr_t application_state;
    hb_app_callback_t application_callback;
} heartbeat_t;

int hb_create(heartbeat_t **hb, unsigned int hb_interval_ms, hb_app_state_ptr_t app_state,
              hb_app_callback_t app_callback);
int hb_execute(heartbeat_t *hb);
int hb_finish(heartbeat_t *hb);
int hb_send_cmd(heartbeat_t *hb, hb_command_t cmd, hb_option_t flags);

#endif
