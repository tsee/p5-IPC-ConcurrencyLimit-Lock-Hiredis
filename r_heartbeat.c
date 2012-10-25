#include "r_heartbeat.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <error.h>
#include <errno.h>

static void *
rhb_thread_loop(void *arg)
{
    rheartbeat_t *hb = (rheartbeat_t *)arg;
    struct timespec time_to_wait;
    r_command_t cmd;
    int rc;

    time_to_wait.tv_sec = 0;
    while (1) {
        if (time_to_wait.tv_sec == 0) {
            clock_gettime(CLOCK_REALTIME, &time_to_wait);
            time_to_wait.tv_sec  += hb->interval;
            /* time_to_wait.tv_nsec +=  */
        }

        pthread_mutex_lock(&hb->access_mutex);
        rc = pthread_cond_timedwait(&hb->cmd_cond_var, &hb->access_mutex, &time_to_wait);
        if (rc == ETIMEDOUT) { /* timeout => perform heartbeat */
            pthread_mutex_unlock(&hb->access_mutex);
            printf("Performing heartbeat\n");
            time_to_wait.tv_sec = 0; /* trigger resetting the timer */
        }
        else if (rc == 0) { /* command received */
            cmd = hb->command;
            hb->command = RCMD_NONE;
            pthread_mutex_unlock(&hb->access_mutex);

            switch (cmd) {
            case RCMD_NONE:
                printf("Got NONE cmd!\n");
                break;
            case RCMD_HALT:
                printf("Got HALT cmd!\n");
                goto cleanup;
                break;
            case RCMD_PING:
                printf("Got PING cmd!\n");
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
rhb_create(rheartbeat_t **hb, unsigned int hb_interval_ms)
{
    rheartbeat_t *h;
    h = (rheartbeat_t *)malloc(sizeof(rheartbeat_t));
    if (h == NULL)
        return 1;
    *hb = h;
    h->interval = hb_interval_ms;
    h->command  = RCMD_NONE;

    pthread_mutex_init(&h->access_mutex, NULL);
    pthread_cond_init(&h->cmd_cond_var, NULL);

    return 0;
}


int
rhb_execute(rheartbeat_t *hb)
{
    int rc = 0;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    rc = pthread_create(&hb->heartbeat_thread, &attr, &rhb_thread_loop, (void *)hb);
    pthread_attr_destroy(&attr);
    return rc;
}


int
rhb_finish(rheartbeat_t *hb)
{
    int rc = 0;
    rhb_send_cmd(hb, RCMD_HALT, 1);
    rc = pthread_join(hb->heartbeat_thread, NULL);
    pthread_mutex_destroy(&hb->access_mutex);
    pthread_cond_destroy(&hb->cmd_cond_var);
    return rc;
}


int
rhb_send_cmd(rheartbeat_t *hb, r_command_t cmd, int flags)
{
    pthread_mutex_lock(&hb->access_mutex);
    if (!(flags & RCMD_OPT_FORCE) && hb->command != RCMD_NONE) {
        printf("hb command is %i\n", hb->command);
        pthread_mutex_unlock(&hb->access_mutex);
        return 1;
    }
    hb->command = cmd;
    pthread_cond_signal(&hb->cmd_cond_var);
    pthread_mutex_unlock(&hb->access_mutex);

    return 0;
}

