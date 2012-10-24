#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "ppport.h"
#include "r_heartbeat.h"
#include "hiredis/hiredis.h"
#include "pthread.h"

MODULE = IPC::ConcurrencyLimit::Lock::Hiredis		PACKAGE = IPC::ConcurrencyLimit::Lock::Hiredis


