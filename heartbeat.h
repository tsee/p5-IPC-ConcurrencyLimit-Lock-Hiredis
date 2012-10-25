#ifndef HEARTBEAT_H_
#define HEARTBEAT_H_

#include <pthread.h>

/* The various heartbeat-built-in commands */
typedef int hb_command_t;
#define HBCMD_NONE 0
/* fired after every internal */
#define HBCMD_TIMER 1
/* fired once after thread initialization */
#define HBCMD_INIT 2
/* fired when the thread is about to be joined */
#define HBCMD_HALT 3
/* just tests that the hb thread is not stuck */
#define HBCMD_PING 4

/* The highest HB-internal cmd. All above are app-specific. */
#define HBCMD_LAST 15

/* HBCMD_OPT_NORM => don't set command if one is already queued */
/* HBCMD_OPT_FORCE => set command, possibly overriding queued command */
typedef int hb_option_t;
#define HBCMD_OPT_NORM   0
#define HBCMD_OPT_FORCE  1

/* custom application state */
typedef void *hb_app_state_ptr_t;
/* custom application callback */
typedef void (*hb_app_callback_t)(hb_app_state_ptr_t app_state, hb_command_t cmd);

typedef struct heartbeat {
    pthread_t heartbeat_thread; /* the actual heartbeat pthread */

    pthread_mutex_t cmd_mutex; /* the mutex for transmitting commands */
    pthread_cond_t cmd_condvar;  /* the condvar for triggering command execution */
    hb_command_t command; /* command storage (default HBCMD_NONE) */

    unsigned int interval; /* hb interval in s, firing HBCMD_TIMER every so often */

    hb_app_state_ptr_t application_state; /* user-defined app state */
    hb_app_callback_t application_callback; /* user-defined hb app callback */
} heartbeat_t;

int hb_create(heartbeat_t **hb,
              unsigned int hb_interval_ms,
              hb_app_state_ptr_t app_state,
              hb_app_callback_t app_callback);
int hb_execute(heartbeat_t *hb);
int hb_finish(heartbeat_t *hb);
int hb_send_cmd(heartbeat_t *hb, hb_command_t cmd, hb_option_t flags);

#endif
