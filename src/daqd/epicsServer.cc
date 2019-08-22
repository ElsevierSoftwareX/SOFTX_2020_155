#include <sys/types.h>
#include <sys/syscall.h>
#include "cadef.h"
#include "envDefs.h"
#include "fdManager.h"
#include "exServer.h"
#include "epicsServer.hh"
#include "debug.h"
#include "shutdown.h"
#include "daqd.hh"

void *
epicsServer::epics_main ()
{
  // Set thread parameters
    daqd_c::set_thread_priority("EPICS IOC","dqepics",0,0); 

    exServer *pCAS;
    try {
      pCAS = new exServer (prefix.c_str(), prefix1.c_str(), prefix2.c_str(), 0, 1);
    }
    catch (...) {
      system_log(1, "Epics server initialization error");
    }
    pCAS->setDebugLevel(0);
    system_log(1, "Epics server started");
    for(;!server_is_shutting_down;) fileDescriptorManager.process(1000.0);
    return (void*)0;
}

