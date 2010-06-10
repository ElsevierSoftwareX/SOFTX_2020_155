/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include "rleap.h"

#ifndef SIG_PF
#define SIG_PF void(*)(int)
#endif
#define _RPC_SVC
#include "dtt/rpcinc.h"

int
_leapquery_1 (void  *argp, void *result, struct svc_req *rqstp)
{
	return (leapquery_1_svc(result, rqstp));
}

void
rleapprog_1(struct svc_req *rqstp, register SVCXPRT *transp)
{
	union {
		int fill;
	} argument;
	union {
		resultLeapQuery_r leapquery_1_res;
	} result;
	bool_t retval;
	xdrproc_t _xdr_argument, _xdr_result;
	bool_t (*local)(char *, void *, struct svc_req *);

	switch (rqstp->rq_proc) {
	case NULLPROC:
		(void) svc_sendreply (transp, (xdrproc_t) xdr_void, (char *)NULL);
		return;

	case LEAPQUERY:
		_xdr_argument = (xdrproc_t) xdr_void;
		_xdr_result = (xdrproc_t) xdr_resultLeapQuery_r;
		local = (bool_t (*) (char *, void *,  struct svc_req *))_leapquery_1;
		break;

	default:
		svcerr_noproc (transp);
		return;
	}
	memset ((char *)&argument, 0, sizeof (argument));
	if (!svc_getargs (transp, (xdrproc_t) _xdr_argument, (caddr_t) &argument)) {
		svcerr_decode (transp);
		return;
	}
	retval = (bool_t) (*local)((char *)&argument, (void *)&result, rqstp);
	if (retval > 0 && !svc_sendreply(transp, (xdrproc_t) _xdr_result, (char *)&result)) {
		svcerr_systemerr (transp);
	}
	if (!svc_freeargs (transp, (xdrproc_t) _xdr_argument, (caddr_t) &argument)) {
		fprintf (stderr, "%s", "unable to free arguments");
		exit (1);
	}
	if (!rleapprog_1_freeresult (transp, _xdr_result, (caddr_t) &result))
		fprintf (stderr, "%s", "unable to free results");

	return;
}
