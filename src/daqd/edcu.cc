#include <config.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <limits.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include "epics_pvs.hh"

using namespace std;

#include "circ.hh"
#include "daqd.hh"
#include "sing_list.hh"

extern daqd_c daqd;

#if __GNUC__ >= 3
extern long int altzone;
#endif

#if EPICS_EDCU==1

#include "cadef.h"

void connectCallback(struct connection_handler_args args) {
    unsigned int *channel_status = (unsigned int *)ca_puser(args.chid);
    *channel_status = (args.op == CA_OP_CONN_UP? 0: 0xbad);
	if (args.op == CA_OP_CONN_UP) daqd.edcu1.con_chans++; else daqd.edcu1.con_chans--;
	daqd.edcu1.con_events++;
    PV::set_pv(PV::PV_EDCU_CHANS, daqd.edcu1.num_chans);
    PV::set_pv(PV::PV_EDCU_CONN_CHANS, daqd.edcu1.con_chans);
}

void subscriptionHandler(struct event_handler_args args) {
//	system_log(1, "Epics Value Change callback for channel %d", (int)args.usr);
	daqd.edcu1.val_events++;
  	if (args.status != ECA_NORMAL) {
	  //system_log(1, "ECA Abnormal status=%d", args.status);
	  return;
        }
	if (args.type == DBR_FLOAT) {
	  float val = *((float *)args.dbr);
      float *channel_value = (float *)args.usr;
      *channel_value = val;
	} else {
//	  system_log(1, "args.type=%d", args.type);
	} 
}

void *
edcu::edcu_main ()
{
  // Set thread parameters
     daqd_c::set_thread_priority("EDCU","dqedcu",0,0); 

     ca_context_create(ca_enable_preemptive_callback);
     for (int i = fidx; i < (fidx + num_chans); i++) {
	chid chid1;
    int status = ca_create_channel(daqd.channels[i].name, connectCallback,
                                   (void *)&(daqd.edcu1.channel_status[i]), 0, &chid1);
	status = ca_create_subscription(DBR_FLOAT, 0, chid1, DBE_VALUE, 
                    subscriptionHandler,
                    (void *)&(daqd.edcu1.channel_value[i]), 0);
     }
     system_log(1, "EDCU has %d channels configured; first=%d\n", num_chans, fidx);
}

#endif /* if EPICS_EDCU != 1 */
