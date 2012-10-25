#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "ppport.h"
#include "heartbeat.h"
#include "hiredis/hiredis.h"
#include "pthread.h"

typedef struct hiredis_hb_data {
    redisContext * redis_cxt;
} hiredis_hb_data_t;

void hiredis_app_callback(hb_app_state_ptr_t app_state, hb_command_t cmd)
{
    hiredis_hb_data_t *d = (hiredis_hb_data_t *)app_state;
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
  CODE:
    /*warn("foo");*/
    hb_finish(hb);


