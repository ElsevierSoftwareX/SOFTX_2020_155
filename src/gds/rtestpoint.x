
/* GDS test point rpc interface */

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

/* list of test point */
#if defined(_ADVANCED_LIGO) && !defined(COMPAT_INITIAL_LIGO)
typedef unsigned int TP_r<>;
#else
typedef unsigned short TP_r<>;
#endif

/* request result */
struct resultRequestTP_r {
      int		status;	/* return status */
      unsigned long	time;	/* active time */
      int		epoch;	/* active epoch */
};

/* query result */
struct resultQueryTP_r {
      int 		status; /* return status */
      TP_r		tp;	/* return array of test points */
};


/* rpc interface */
program RTESTPOINT {
   version TPVERS {

      resultRequestTP_r REQUESTTP (int id, int node, TP_r tp, hyper timeout) = 1;
      resultRequestTP_r REQUESTTPNAME (int id, string tpnames, hyper timeout) = 2;
      int CLEARTP (int id, int node, TP_r TP) = 3;
      int CLEARTPNAME (int id, string tpnames) = 4;
      resultQueryTP_r QUERYTP (int id, int node, int tpinterface, int tplen,
                               unsigned long time, int epoch) = 5;
      int KEEPALIVE (int id) = 6;

   } = 1;
} = 0x31001001;
