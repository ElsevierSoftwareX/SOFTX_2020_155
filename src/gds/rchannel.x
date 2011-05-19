/* Version $Id$ */

/* GDS channel database rpc interface */

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

/* channel information */
struct channelinfo_r {
      /* channel name. 32 characters maximum; always \0 terminated! 
         The name must be the first member of the structure.  */
      char		chName[MAX_CHNNAME_SIZE];
      /* interferometer id:
         H0, L0 -> 0; H1, L1 -> 1; H2 -> 2 */
      short		ifoId;
      /* reflective memory loop id: 0 or 1 (LHO only) */
      short		rmId;
      /* DCU id number */
      short		dcuId;
      /* channel number */
      short		chNum;
      /* data type as defined by the nds:
         1 - int16, 2 - int32, 3 - int64, 4 - float, 5 - double */
      short		dataType;
      /* data rate: specified in Hz; must be a power of two  */
#if defined(_ADVANCED_LIGO) 
      int 		dataRate;
#else
      short		dataRate;
#endif
      /* channel group */
      short 		chGroup;
      /* number of bytes per sample */
      short		bps;
      /** front-end gain */
      float		gain;
      /** calibration slope */
      float		slope;
      /** calibration offset */
      float		offset;
      /** unit name */
      char		unit[40];
      /* offset of channel in reflective memory */
      unsigned long	rmOffset;
      /* size of block channels belongs to */
      unsigned long	rmBlockSize;
};

typedef channelinfo_r channellist_r<>;

/* request result */
struct resultChannelQuery_r {
      int		status;	/* return status */
      channellist_r	chnlist;/* channel list */
};

/* rpc interface */
program RCHANNEL {
   version RCHNVERS {

      resultChannelQuery_r CHNQUERY (void) = 1;

   } = 1;
} = 0x31001005;
