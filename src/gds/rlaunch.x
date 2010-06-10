
/* Launch server rpc interface */

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

/* launch information */
struct launch_info_r {
   /* Launch category */
   string    title<>;
   /* Launch program */
   string    prog<>;
};
   
/* launch information list */
typedef launch_info_r launch_infolist_r<>;

/* launch information request result */
struct resultLaunchInfoQuery_r {
      int		status;	/* return status */
      launch_infolist_r	list;   /* info list */
};

/* rpc interface */
program RLAUNCHPROG {
   version RLAUCNHVER {

      resultLaunchInfoQuery_r LAUNCHQUERY (void) = 1;
      int LAUNCH (string category<>, string prog<>, string display<>) = 2;

   } = 1;
} = 0x31001007;
