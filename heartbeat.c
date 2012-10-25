#include "heartbeat.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <error.h>
#include <errno.h>

#define STMT_START	do
#define STMT_END	while (0)

#define APP_CALLBACK(hb, cmd)                                           \
    STMT_START {                                                        \
        pthread_mutex_lock(&(hb)->app_state_mutex);                     \
        (*(hb)->application_callback)((hb)->application_state, (cmd));  \
        pthread_mutex_unlock(&(hb)->app_state_mutex);                   \
    } STMT_END

static void *
hb_thread_loop(void *arg)
{
    heartbeat_t *hb = (heartbeat_t *)arg;
    struct timespec time_to_wait;
    hb_command_t cmd;
    int rc;

    APP_CALLBACK(hb, HBCMD_INIT);

    time_to_wait.tv_sec = 0;
    while (1) {
        if (time_to_wait.tv_sec == 0) {
            clock_gettime(CLOCK_REALTIME, &time_to_wait);
            time_to_wait.tv_sec  += hb->interval;
            /* time_to_wait.tv_nsec +=  */
        }

        pthread_mutex_lock(&hb->cmd_mutex);
        rc = pthread_cond_timedwait(&hb->cmd_condvar, &hb->cmd_mutex, &time_to_wait);
        if (rc == ETIMEDOUT) { /* timeout => perform heartbeat */
            pthread_mutex_unlock(&hb->cmd_mutex);
            printf("Performing heartbeat\n");
            APP_CALLBACK(hb, HBCMD_TIMER);
            time_to_wait.tv_sec = 0; /* trigger resetting the timer */
        }
        else if (rc == 0) { /* command received */
            cmd = hb->command;
            hb->command = HBCMD_NONE;
            pthread_mutex_unlock(&hb->cmd_mutex);

            switch (cmd) {
            case HBCMD_NONE:
                /* printf("Got NONE cmd!\n"); */
                break;
            case HBCMD_HALT:
                printf("Got HALT cmd!\n");
                APP_CALLBACK(hb, HBCMD_HALT);
                goto cleanup;
                break;
            case HBCMD_PING:
                printf("Got PING cmd!\n");
                break;
            default:
                if (cmd <= HBCMD_LAST) {
                    printf("Got cmd reserved for HB internals! ERROR.\n");
                    break;
                }
                printf("Got OTHER cmd (%i)!\n", cmd);
                APP_CALLBACK(hb, cmd);
                break;
            }
        }
        else {
            printf("Error waiting for condvar\n");
            break;
        }
    }

  cleanup:
    pthread_exit(NULL);
    return NULL;
}


int
hb_create( heartbeat_t **hb,
            unsigned int hb_interval_ms,
            hb_app_state_ptr_t app_state,
            hb_app_callback_t app_callback)
{
    heartbeat_t *h;
    h = (heartbeat_t *)malloc(sizeof(heartbeat_t));
    if (h == NULL)
        return 1;
    *hb = h;
    h->interval = hb_interval_ms;
    h->command  = HBCMD_NONE;
    h->application_state = app_state;
    h->application_callback = app_callback;

    pthread_mutex_init(&h->cmd_mutex, NULL);
    pthread_mutex_init(&h->app_state_mutex, NULL);
    pthread_cond_init(&h->cmd_condvar, NULL);

    return 0;
}


int
hb_execute(heartbeat_t *hb)
{
    int rc = 0;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    rc = pthread_create(&hb->heartbeat_thread, &attr, &hb_thread_loop, (void *)hb);
    pthread_attr_destroy(&attr);
    return rc;
}


int
hb_finish(heartbeat_t *hb)
{
    int rc = 0;
    hb_send_cmd(hb, HBCMD_HALT, 1);
    rc = pthread_join(hb->heartbeat_thread, NULL);
    pthread_mutex_destroy(&hb->cmd_mutex);
    pthread_mutex_destroy(&hb->app_state_mutex);
    pthread_cond_destroy(&hb->cmd_condvar);
    return rc;
}


int
hb_send_cmd(heartbeat_t *hb, hb_command_t cmd, hb_option_t flags)
{
    pthread_mutex_lock(&hb->cmd_mutex);
    if (!(flags & HBCMD_OPT_FORCE) && hb->command != HBCMD_NONE) {
        printf("hb command is %i\n", hb->command);
        pthread_mutex_unlock(&hb->cmd_mutex);
        return 1;
    }
    hb->command = cmd;
    pthread_cond_signal(&hb->cmd_condvar);
    pthread_mutex_unlock(&hb->cmd_mutex);

    return 0;
}

