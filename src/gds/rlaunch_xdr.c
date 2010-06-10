/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include "rlaunch.h"
#define _RPC_XDR
#include "dtt/rpcinc.h"

bool_t
xdr_launch_info_r (XDR *xdrs, launch_info_r *objp)
{
	register int32_t *buf;

	 if (!xdr_string (xdrs, &objp->title, ~0))
		 return FALSE;
	 if (!xdr_string (xdrs, &objp->prog, ~0))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_launch_infolist_r (XDR *xdrs, launch_infolist_r *objp)
{
	register int32_t *buf;

	 if (!xdr_array (xdrs, (char **)&objp->launch_infolist_r_val, (u_int *) &objp->launch_infolist_r_len, ~0,
		sizeof (launch_info_r), (xdrproc_t) xdr_launch_info_r))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_resultLaunchInfoQuery_r (XDR *xdrs, resultLaunchInfoQuery_r *objp)
{
	register int32_t *buf;

	 if (!xdr_int (xdrs, &objp->status))
		 return FALSE;
	 if (!xdr_launch_infolist_r (xdrs, &objp->list))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_launch_1_argument (XDR *xdrs, launch_1_argument *objp)
{
	 if (!xdr_string (xdrs, &objp->category, ~0))
		 return FALSE;
	 if (!xdr_string (xdrs, &objp->prog, ~0))
		 return FALSE;
	 if (!xdr_string (xdrs, &objp->display, ~0))
		 return FALSE;
	return TRUE;
}
