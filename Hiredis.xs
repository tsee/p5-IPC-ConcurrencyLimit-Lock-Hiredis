#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "ppport.h"
#include "r_heartbeat.h"
#include "hiredis/hiredis.h"
#include "pthread.h"

MODULE = IPC::ConcurrencyLimit::Lock::Hiredis		PACKAGE = IPC::ConcurrencyLimit::Lock::Hiredis

rheartbeat_t *
new(CLASS)
    char *CLASS;
  PREINIT:
    int rc;
  CODE:
    rc = rhb_create(&RETVAL, 1);
    if (rc != 0)
      croak("Failed to create heartbeat");
    rhb_execute(RETVAL);
  OUTPUT: RETVAL

void
ping_thread(hb)
    rheartbeat_t *hb
  CODE:
    rhb_send_cmd(hb, RCMD_PING, RCMD_OPT_NORM);

void
DESTROY(hb)
    rheartbeat_t *hb;
  CODE:
    /*warn("foo");*/
    rhb_finish(hb);


