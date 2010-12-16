/* Version $Id$ */

/* GDS leap second information rpc interface */

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

/* leap second information */
   struct leap_r {
      /** GPS sec when the leap seconds takes effect */
      hyper transition;
      /** Seconds of correction to apply. The correction is given as
          the new total difference between TAI (not GPS) and UTC. */
      int change;
   };

typedef leap_r leaplist_r<>;

/* request result */
struct resultLeapQuery_r {
      int		status;	/* return status */
      leaplist_r	leaplist;/* channel list */
};

/* rpc interface */
program RLEAPPROG {
   version RLEAPVER {

      resultLeapQuery_r LEAPQUERY (void) = 1;

   } = 1;
} = 0x31001006;
