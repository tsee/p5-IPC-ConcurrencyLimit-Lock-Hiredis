#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "ppport.h"
#include "r_heartbeat.h"
#include "hiredis/hiredis.h"
#include "pthread.h"

void empty_app_callback (hb_app_state_ptr_t app_state, hb_command_t cmd)
{
}


MODULE = IPC::ConcurrencyLimit::Lock::Hiredis		PACKAGE = IPC::ConcurrencyLimit::Lock::Hiredis

heartbeat_t *
new(CLASS)
    char *CLASS;
  PREINIT:
    int rc;
  CODE:
    rc = hb_create(&RETVAL, 1, NULL, &empty_app_callback);
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


