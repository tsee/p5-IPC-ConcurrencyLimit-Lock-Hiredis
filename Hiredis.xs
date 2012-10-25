#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include <stdio.h>
#include <stdlib.h>

#include "ppport.h"
#include "heartbeat.h"
#include "hiredis/hiredis.h"
#include "pthread.h"

typedef struct hiredis_hb_data {
    redisContext *redis_cxt;
} hiredis_hb_data_t;


void
redis_reply_expect_str(redisReply *r, char **str, size_t *len)
{
    switch (r->type) {
    case REDIS_REPLY_STRING:
        *str = r->str;
        *len = r->len;
        break;
    case REDIS_REPLY_ERROR:
        fprintf(stderr, "Got Redis error: %.*s\n", r->len, r->str);
        *str = NULL;
        *len = 0;
        break;
    case REDIS_REPLY_INTEGER:
    case REDIS_REPLY_NIL:
    case REDIS_REPLY_STATUS:
    case REDIS_REPLY_ARRAY:
        fprintf(stderr, "Got unexpected Redis reply (%i)\n", r->type);
        *str = NULL;
        *len = 0;
        break;
    default:
        fprintf(stderr, "Invalid Redis reply!? (%i)\n", r->type);
        *str = NULL;
        *len = 0;
        break;
    }
}

void
redis_reply_expect_status(redisReply *r, char **str, size_t *len)
{
    switch (r->type) {
    case REDIS_REPLY_STATUS:
        *str = r->str;
        *len = r->len;
        break;
    case REDIS_REPLY_ERROR:
        fprintf(stderr, "Got Redis error: %.*s\n", r->len, r->str);
        *str = NULL;
        *len = 0;
        break;
    case REDIS_REPLY_INTEGER:
    case REDIS_REPLY_NIL:
    case REDIS_REPLY_STRING:
    case REDIS_REPLY_ARRAY:
        fprintf(stderr, "Got unexpected Redis reply (%i)\n", r->type);
        *str = NULL;
        *len = 0;
        break;
    default:
        fprintf(stderr, "Invalid Redis reply!? (%i)\n", r->type);
        *str = NULL;
        *len = 0;
        break;
    }
}

void
hiredis_app_callback(hb_app_state_ptr_t app_state, hb_command_t cmd)
{
    hiredis_hb_data_t *d = (hiredis_hb_data_t *)app_state;
    redisContext *redis = d->redis_cxt;
    redisReply *reply;
    char *strbuf = NULL;
    size_t str_len;

    switch (cmd) {
    case HBCMD_NONE:
    case HBCMD_HALT:
    case HBCMD_INIT:
        break;
    case HBCMD_TIMER:
        /* do heartbeat here */
        reply = redisCommand(redis, "SET foo bar");
        redis_reply_expect_status(reply, &strbuf, &str_len);
        if (strbuf == NULL || str_len != 2 || strncmp(strbuf, "OK", 2)) {
            freeReplyObject(reply);
            break;
        }
        freeReplyObject(reply);

        reply = redisCommand(redis, "GET foo");
        redis_reply_expect_str(reply, &strbuf, &str_len);
        if (strbuf != NULL) {
            printf("%.*s\n", str_len, strbuf);
        }
        freeReplyObject(reply);

        break;
    default:
        fprintf(stderr, "Invalid command received in heartbeat thread (%i)\n", cmd);
        break;
    }
}


MODULE = IPC::ConcurrencyLimit::Lock::Hiredis		PACKAGE = IPC::ConcurrencyLimit::Lock::Hiredis

heartbeat_t *
new(CLASS, char *ip, int port)
    char *CLASS;
  PREINIT:
    int rc;
    hiredis_hb_data_t *hb_data;
    redisContext *rcxt;
  CODE:
    rcxt = redisConnect(ip, port);
    if (rcxt->err) {
        char* str = strdup(rcxt->errstr);
        redisFree(rcxt);
        croak("Error connecting to Redis: %s", str);
    }
    hb_data = (hiredis_hb_data_t *)malloc(sizeof(hiredis_hb_data_t));
    if (hb_data == NULL) {
        redisFree(rcxt);
        croak("Out of memory");
    }
    hb_data->redis_cxt = rcxt;
    rc = hb_create(&RETVAL, 1, hb_data, &hiredis_app_callback);
    if (rc != 0)
        croak("Failed to create heartbeat");
    hb_execute(RETVAL);
  OUTPUT: RETVAL

void
ping_thread(hb)
    heartbeat_t *hb
  CODE:
    hb_send_cmd(hb, HBCMD_PING, HBCMD_OPT_NORM);

void
DESTROY(hb)
    heartbeat_t *hb;
  PREINIT:
    hiredis_hb_data_t *d;
  CODE:
    hb_finish(hb);
    d = (hiredis_hb_data_t *)hb->application_state;
    redisFree(d->redis_cxt);


