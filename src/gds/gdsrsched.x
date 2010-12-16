/* Version $Id$ */
/* GDS remote scheduler rpc interface */

/* fix include problems with VxWorks */
#ifdef RPC_HDR
%#define		_RPC_HDR
#endif
#ifdef RPC_XDR
%#define		_RPC_XDR
#endif
#ifdef RPC_SVC		
%#define		_RPC_SVC
#endif
#ifdef RPC_CLNT		
%#define		_RPC_CLNT
#endif
%#include "dtt/rpcinc.h"

/* tai nsec */
typedef hyper tainsec_r; 

/* number of bytes reserved for an address on the remote machine */
const ADDRLEN = 8;

/* length of time tag, must be identical to TIMETAG_LENGTH in 
   gdssched.h */
const RTIMETAG_LENGTH = 17;

/* scheduler descriptor */
struct scheduler_r {
      opaque	scheduler_r[ADDRLEN];
};

/* scheduler task information; this structure has to correspond 
   to the one defined in gdssched.h
   It is reordered due to transport efficiency
   and functions are represented as interger values */
struct schedulertask_r {
      int	flag;
      int	priority;
      int	tagtype; 
      int	synctype;
      int	syncval;
      int	waittype;
      int	repeattype;
      int	repeatval;
      int	repeatratetype;
      int	repeatrate;
      int	repeatsynctype;
      int	repeatsyncval;
      long 	func;
      int	arg_sizeof;
      tainsec_r	timeout;
      tainsec_r	waitval;
      char	timetag [RTIMETAG_LENGTH];	/* using opaque would be */
      char	waittag [RTIMETAG_LENGTH];	/* more efficient */
      opaque	arg<>;
};


/* return information from getScheduledtask */
struct resultGetScheduledTask_r {
      int		status; /* return status */
      schedulertask_r   task;	/* returned task */
};

/* return information from connect scheduler */
struct remotesched_r {
      int		status; /* return status */
      scheduler_r	sd;	/* remote scheduler descriptor */
};


/* rpc interface */
program GDSSCHEDULER {
   version SCHEDVER {

      int CLOSESCHEDULER (scheduler_r sd, tainsec_r timeout) = 1;
      int SCHEDULETASK (scheduler_r sd, 
      		schedulertask_r newtask) = 2;
      resultGetScheduledTask_r GETSCHEDULEDTASK (scheduler_r sd, int id) = 3;
      int REMOVESCHEDULEDTASK (scheduler_r sd, int id, 
		int terminate) = 4;
      int WAITFORSCHEDULERTOFINISH (scheduler_r sd, 
		tainsec_r timeout) = 5;
      int SETTAGNOTIFY (scheduler_r sd, string tag<RTIMETAG_LENGTH>, 
 		tainsec_r time) = 6;
      remotesched_r CONNECTSCHEDULER (scheduler_r callbacksd, 
		unsigned int callbackprogram, 
		unsigned int callbackversion) = 7;

   } = 1;
} = 0x30000000;


/* callback interface */
program GDSSCHEDULERCALLBACK {
   version SCHEDVERCALLBACK {

      int SETTAGCALLBACK (scheduler_r sd, scheduler_r bsd, 
                string tag<RTIMETAG_LENGTH>, tainsec_r time) = 1;

   } = 1;
} = 0x40000000;

