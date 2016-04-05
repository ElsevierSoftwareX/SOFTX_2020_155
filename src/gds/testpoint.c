static char *versionId = "Version $Id$" ;
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: testpoint						*/
/*                                                         		*/
/* Module Description: implements functions for handling test points	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

#ifndef __EXTENSIONS__
#define __EXTENSIONS__
#endif
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include "dtt/gdsutil.h"
#ifndef _TESTPOINT_DIRECT
#ifdef OS_VXWORKS
#undef _NO_KEEP_ALIVE
#undef _CONFIG_DYNAMIC
#define _NO_KEEP_ALIVE

#if (IFO == GDS_IFO1)
#define _TESTPOINT_DIRECT	1
#elif (IFO == GDS_IFO2) 
#define _TESTPOINT_DIRECT	2
#elif (IFO == GDS_PEM)
#define _TESTPOINT_DIRECT	3
#else
#define _TESTPOINT_DIRECT	0
#endif
#else
#define _TESTPOINT_DIRECT	0
#endif
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Includes: 								*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#ifdef OS_VXWORKS
#include <vxWorks.h>
#include <semLib.h>
#include <taskLib.h>
#include <inetLib.h>
#include <hostLib.h>
#include <sysLib.h>
#include <timers.h>

#else
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

#include "dtt/rmorg.h"
#include "dtt/testpoint.h"
#ifndef _NO_TESTPOINTS
#include "dtt/rtestpoint.h"
#if (_TESTPOINT_DIRECT != 0)
#include "dtt/rmapi.h"
#endif
#if (_TESTPOINT_DIRECT != 0) || !defined (_NO_KEEP_ALIVE)
#include "dtt/gdssched.h"
#endif
#endif
#if defined (_CONFIG_DYNAMIC)
#include "dtt/confinfo.h" 
#endif

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Constants: _NETID		  net protocol used for rpc		*/
/*            _MAX_INDEX_CACHE	  size of tp index cache		*/
/*            _KEEP_ALIVE_RATE	  keep alive rate in sec		*/
/*            _TP_CLEAR_ALL	  clear all test points			*/
/*            PRM_FILE		  parameter file name			*/
/*            PRM_SECTION	  parameter file section heading	*/
/*            PRM_ENTRY1	  parameter file host name entry	*/
/*            PRM_ENTRY2	  parameter file rpc program # entry	*/
/*            PRM_ENTRY3	  parameter file rpc version # entry	*/
/*            PRM_ENTRY4	  parameter file controls sysem name    */
/*                                                         		*/
/*----------------------------------------------------------------------*/
#define _NETID			"tcp"
#define _MAX_INDEX_CACHE	3
#define _KEEP_ALIVE_RATE	5
#if !defined (_TP_DAQD) && !defined (_CONFIG_DYNAMIC)
#define PRM_FILE		gdsPathFile ("/param", "testpoint.par")
#define PRM_SECTION		gdsSectionSite ("node%i")
#define PRM_ENTRY1		"hostname"
#define PRM_ENTRY2		"prognum"
#define PRM_ENTRY3		"progver"
#define PRM_ENTRY4		"system"
#endif
#define _HELP_TEXT	"Test point interface commands:\n" \
			"  show 'node': show active test points\n" \
			"  set 'node' 'number': set a test point\n" \
			"  clear 'node' 'number': clear a test point, " \
			   " use * for wildcards\n"


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Types: tpNode_t - node type for rpc client				*/
/*        tpIndex_t - test point index chache				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   struct tpNode_t {
      int		valid;
      int		duplicate;
      int		id;
      char		hostname[80];
      unsigned long	prognum;
      unsigned long	progver;
   };
   typedef struct tpNode_t tpNode_t;

#if (_TESTPOINT_DIRECT != 0)
   struct tpIndex_t {
      testpoint_t	tp[TP_MAX_NODE][TP_MAX_INTERFACE][TP_MAX_INDEX];
      taisec_t		time;
      int		epoch;
   };
   typedef struct tpIndex_t tpIndex_t;
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Globals: tp_init		whether client was already init.	*/
/*          tpNodes		test point nodes		 	*/
/*          tpNum		Number of reachable test point nodes 	*/
/*          tpmux		mutex to protect index cache		*/
/*          tpindexcur		cursor into index cache			*/
/*          tpindexlist		test point index cache (organized as a	*/
/*          			ring buffer)				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#ifndef _NO_TESTPOINTS
   static int			tp_init = 0;
   static tpNode_t		tpNode[TP_MAX_NODE];
   static int			tpNum = 0;
#if (_TESTPOINT_DIRECT != 0)
   static mutexID_t		tpmux;
   static int			tpindexcur = 0;
   static tpIndex_t		tpindexlist[_MAX_INDEX_CACHE];
#endif
#if (_TESTPOINT_DIRECT != 0) || !defined (_NO_KEEP_ALIVE)
   static scheduler_t*		tpsched;
#endif
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Forward declarations: 						*/
/*	initTestpoint		init of test point interface		*/
/*	finiTestpoint		cleanup of test point interface		*/
/*      tpMakeHandle		make a rpc client handle		*/
/*      tpGetIndexDirect	gets the specified test point index	*/
/*      readTestpoints		reads the test point indexes from RM	*/
/*      								*/
/*----------------------------------------------------------------------*/
   __init__(initTestpoint);
#ifndef __GNUC__
#pragma init(initTestpoint)
#endif
   __fini__(finiTestpoint);
#ifndef __GNUC__
#pragma fini(finiTestpoint)
#endif
#ifndef _NO_TESTPOINTS
   static CLIENT* tpMakeHandle (int node);
#if (_TESTPOINT_DIRECT != 0)
   static int readTestpoints (schedulertask_t* info, taisec_t time, 
                     int epoch, void* arg);
#endif
#ifndef _NO_KEEP_ALIVE
   static int keepAlive (schedulertask_t* info, taisec_t time, 
                     int epoch, void* arg);
#endif
#endif
#if defined (_CONFIG_DYNAMIC) && !defined (_TP_DAQD)
   static int tpSetHostAddress (int node, const char* hostname, 
                     unsigned long prognum, unsigned long progver);
#endif

#ifndef _NO_TESTPOINTS
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* internal Procedure Name: tpMakeHandle				*/
/*                                                         		*/
/* Procedure Description: makes a rpc client handle for a TP node	*/
/*                                                         		*/
/* Procedure Arguments: node ID						*/
/*                                                         		*/
/* Procedure Returns: client handle if successful, NULL when failed	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static CLIENT* tpMakeHandle (int node)
   {
      CLIENT*		clnt;		/* client handle */
   
      /* printf("making handle tp node %d, max is %d\n", node, TP_MAX_NODE); */

      /* check node */
      if ((node < 0) || (node >= TP_MAX_NODE)) {
         return NULL;
      }
      /* check validity */
      if (!tpNode[node].valid) {
	 //printf("tp node %d invalid\n", node);
         return NULL;
      }
   
      /* create handle */
      clnt = clnt_create (tpNode[node].hostname, tpNode[node].prognum, 
                         tpNode[node].progver, _NETID);
      if (clnt == NULL) {
	 printf("couldn't create test point handle\n");
	 printf("hostname=%s, prognum=%d, progver=%d\n",
	 	tpNode[node].hostname, (int)tpNode[node].prognum,
		                         (int)tpNode[node].progver);
         gdsError (GDS_ERR_MEM, 
                  "couldn't create test point handle");
      }
   
      return clnt;
   }
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: tpRequest					*/
/*                                                         		*/
/* Procedure Description: requests a test point				*/
/*                                                         		*/
/* Procedure Arguments: node ID, test point list & length, timeout,	*/
/*                      active time & epoch				*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 when failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int tpRequest (int node, const testpoint_t tp[], int tplen,
                 tainsec_t timeout, taisec_t* time, int* epoch)
   {
   #ifdef _NO_TESTPOINTS
      return -10;
   #else
   
      TP_r		testpoints;	/* test point list */
      resultRequestTP_r	result;		/* result of rpc call */
      int		retval;		/* return value */
      CLIENT*		clnt;		/* client rpc handle */
   
   #ifdef OS_VXWORKS
      rpcTaskInit();
   #endif
      gdsDebug ("request test point");
   
      /* intialize interface */
      if (testpoint_client() < 0) {
         return -2;
      }
   
       /* check test point list */
      if ((tp == NULL) || (tplen == 0)) {
         return 0;
      }
   
      /* make test point list */
      testpoints.TP_r_len = tplen;
      testpoints.TP_r_val = (testpoint_t*) tp;
   
      /* make client handle */
      clnt = tpMakeHandle (node);
      if (clnt == NULL) {
         return -3;
      }
   
      /* call remote procedure */
      memset (&result, 0, sizeof (resultRequestTP_r));
      if ((requesttp_1 (tpNode[node].id, node, testpoints, timeout, 
         &result, clnt) == RPC_SUCCESS) && (result.status >= 0)) {
      	 /* set return arguments */
         if (time != NULL) {
            *time = result.time;
         }
         if (epoch != NULL) {
            *epoch = result.epoch;
         }
         retval = result.status;
      }
      else {
         gdsError (GDS_ERR_PROG, "unable to set test points");
         retval = -4;
      }
   
      /* free handle and memory of return argument */
      xdr_free ((xdrproc_t)xdr_resultRequestTP_r, (char*) &result);
      clnt_destroy (clnt);
      return retval;
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: tpRequestName				*/
/*                                                         		*/
/* Procedure Description: requests a test point	by name			*/
/*                                                         		*/
/* Procedure Arguments: test point name(s), timeout,			*/
/*                      active time & epoch				*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 when failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int tpRequestName (const char* tpNames,
                     tainsec_t timeout, taisec_t* time, int* epoch)
   {
   #ifdef _NO_TESTPOINTS
      return -10;
   #else
      int		node;		/* test point node */
      resultRequestTP_r	result;		/* result of rpc call */
      int		retval;		/* return value */
      int		k;		/* node index */
      int		temp;		/* temporary var. */
      CLIENT*		clnt;		/* client rpc handle */
   
   #ifdef OS_VXWORKS
      rpcTaskInit();
   #endif
      gdsDebug ("request test point by name");
   
      /* intialize interface */
      if (testpoint_client() < 0) {
         return -2;
      }
   
       /* check test point list */
      if (tpNames == NULL) {
         return 0;
      }
   
      /* send to all nodes which aren't duplicates */
      for (node = 0; node < TP_MAX_NODE; node++) {
         if (!tpNode[node].valid || tpNode[node].duplicate) {
            continue;
         }
      
         /* make client handle */
         clnt = tpMakeHandle (node);
         if (clnt == NULL) {
            continue;
            /* return -3; */
         }
      
         /* call remote procedure */
         memset (&result, 0, sizeof (resultRequestTP_r));
         if ((requesttpname_1 (tpNode[node].id, (char*) tpNames, timeout, 
            &result, clnt) == RPC_SUCCESS) && (result.status >= 0)) {
         /* set return arguments */
            if (time != NULL) {
               *time = result.time;
            }
            if (epoch != NULL) {
               *epoch = result.epoch;
            }
            retval = result.status;
         }
         else {
            gdsError (GDS_ERR_PROG, "unable to set test points");
            retval = -4;
         }
      
         /* free handle and memory of return argument */
         xdr_free ((xdrproc_t)xdr_resultRequestTP_r, (char*) &result);
         clnt_destroy (clnt);
      
         /* cleanup on error */
         if (retval < 0) {
            for (k = node - 1; k >= 0; k--) {
               if (!tpNode[k].valid || tpNode[k].duplicate) {
                  continue;
               }
               /* make client handle */
               clnt = tpMakeHandle (k);
               if (clnt == NULL) {
                  return -3;
               }
               /* call remote procedure */
               cleartpname_1 (tpNode[k].id, (char*) tpNames, &temp, 
                             clnt);
               /* free handle */
               clnt_destroy (clnt);
            }
            return retval;
         }
      }
   
      return 0;
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: tpClear					*/
/*                                                         		*/
/* Procedure Description: clears a test point				*/
/*                                                         		*/
/* Procedure Arguments: request ID, test point list			*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 when failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int tpClear (int node, const testpoint_t tp[], int tplen)
   {
   #ifdef _NO_TESTPOINTS
      return -10;
   #else
   
      static testpoint_t 	all = _TP_CLEAR_ALL;
      TP_r		testpoints;	/* test point list */
      int		result;		/* result of rpc call */
      CLIENT*		clnt;		/* client rpc handle */
   
   #ifdef OS_VXWORKS
      rpcTaskInit();
   #endif
      gdsDebug ("clear test point");
   
      /* intialize interface */
      if (testpoint_client() < 0) {
         return -2;
      }
   
      /* make test point list */
      if (tp == NULL) {
         testpoints.TP_r_len = 1;
         testpoints.TP_r_val = &all;
      }
      else if (tplen == 0) {
         return 0;
      }
      else {
         testpoints.TP_r_len = tplen;
         testpoints.TP_r_val = (testpoint_t*) tp;
      }
   
      /* make client handle */
      clnt = tpMakeHandle (node);
      if (clnt == NULL) {
         return -3;
      }
   
      /* call remote procedure */
      if ((cleartp_1 (tpNode[node].id, node, testpoints, &result, 
         clnt) != RPC_SUCCESS) || (result < 0)) {
         gdsError (GDS_ERR_PROG, "unable to clear test points");
         result = -4;
      }
   
      /* free handle */
      clnt_destroy (clnt);
      return result;
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: tpClearName					*/
/*                                                         		*/
/* Procedure Description: clears a test point by name			*/
/*                                                         		*/
/* Procedure Arguments: test point names				*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 when failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int tpClearName (const char* tpNames)
   {
   #ifdef _NO_TESTPOINTS
      return -10;
   #else
   
      int		node;		/* node ID */
      int		result;		/* result of rpc call */
      int		retval;		/* return value */
      CLIENT*		clnt;		/* client rpc handle */
   
   #ifdef OS_VXWORKS
      rpcTaskInit();
   #endif
      gdsDebug ("clear test point by name");
   
      /* intialize interface */
      if (testpoint_client() < 0) {
         return -2;
      }
   
      /* send to all nodes which are not duplicates */
      retval = 0;
      for (node = 0; node < TP_MAX_NODE; node++) {
         if (!tpNode[node].valid || tpNode[node].duplicate) {
            continue;
         }
      
         /* make client handle */
         clnt = tpMakeHandle (node);
         if (clnt == NULL) {
            return -3;
         }
      
         /* call remote procedure */
         if ((cleartpname_1 (tpNode[node].id, (char*) tpNames, &result, 
            clnt) != RPC_SUCCESS) || (result < 0)) {
            gdsError (GDS_ERR_PROG, "unable to clear test points");
            result = -4;
         }
      
         /* free handle */
         clnt_destroy (clnt);
         if (result < 0) {
            retval = result;
         }
      }
   
      return retval;
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: tpQuery					*/
/*                                                         		*/
/* Procedure Description: queries the test point interface		*/
/*                                                         		*/
/* Procedure Arguments: node ID, test point list, max. length, 		*/
/*                      time and epoch of query request     		*/
/*                                                         		*/
/* Procedure Returns: # of entries, if successful, <0 when failed	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int tpQuery (int node, int tpinterface, testpoint_t tp[], int tplen, 
               taisec_t time, int epoch)
   {
   #ifdef _NO_TESTPOINTS
      return -10;
   #else
   
      resultQueryTP_r	result;		/* result of rpc call */
      int		retval;		/* return value */
      CLIENT*		clnt;		/* client rpc handle */
      int		i;		/* index into test point list */
   
      gdsDebug ("query test point");
   
      /* intialize interface */
      if (testpoint_client() < 0) {
         return -2;
      }
   
      /* check node */
      if ((node < 0) || (node >= TP_MAX_NODE)) {
         return -2;
      }
   
      /* check interface */
      if ((tpinterface < 0) || (tpinterface >= TP_MAX_INTERFACE)) {
         return -2;
      }
   
      /* check test point list */
      if (tplen < 0) {
         return -2;
      }
   
   #if (_TESTPOINT_DIRECT != 0)
      /* try to access reflective memory directly */ 
      retval = tpGetIndexDirect (node, tpinterface, tp, tplen, 
                                 time, epoch);
      /* printf ("tpGetIndexDirect = %i\n", retval);*/
      if (retval >= 0) {
         /* direct access successful */
         return retval;
      }
      if (retval != -1) {
         /* direct access failed due to unrecoverable error */
         return -3;
      }
      /* direct access failed because node is not directly accessible, 
         try to read from server */
   #endif
   
      /* do remote procedure call */
   #ifdef OS_VXWORKS
      rpcTaskInit();
   #endif
   
      /* make client handle */
      clnt = tpMakeHandle (node);
      if (clnt == NULL) {
         return -3;
      }
   
      /* call remote procedure */
      memset (&result, 0, sizeof (resultQueryTP_r));
      if ((querytp_1 (tpNode[node].id, node, tpinterface, tplen, time, 
         epoch, &result, clnt) == RPC_SUCCESS) && (result.status >= 0)) {
         /* copy result */
         if (tp != NULL) {
            for (i = 0; i < result.tp.TP_r_len; i++) {
               tp[i] = result.tp.TP_r_val[i];
            }
         }
         retval = result.tp.TP_r_len;
      }
      else {
         retval = -4;
      }
   
      /* free handle and memory of return array */
      xdr_free ((xdrproc_t)xdr_resultQueryTP_r, (char*) &result); 
      clnt_destroy (clnt);
      return retval;
   #endif
   }

#ifndef _TP_DAQD

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: tpAddr					*/
/*                                                         		*/
/* Procedure Description: returns a test point address in refl. mem.	*/
/*                                                         		*/
/* Procedure Arguments: node id, test point, time & epoch of request	*/
/*                                                         		*/
/* Procedure Returns: address if successful, <0 otherwise		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int tpAddr (int node, testpoint_t tp, taisec_t time, int epoch)
   {
   #ifdef _NO_TESTPOINTS
      return -10;
   #elif (_TESTPOINT_DIRECT != 0)
      int		tpinterface;	/* tp interface id */
      testpoint_t	indx[TP_MAX_INDEX];	/* test point index */
      int		len;		/* length of index */
      int		rmOfs;		/* rm offset */
      int		rmBlkSize;	/* rm block size */
      int		i;		/* index */
   
      /* first make sure node exists */
      if (((_TESTPOINT_DIRECT & (1 << node)) == 0)  || 
         (rmBaseAddress (node) == NULL)) {
         return -2;
      }
   
      /* get interface id */
      tpinterface = TP_ID_TO_INTERFACE (tp);
      if ((tpinterface < 0) || (tpinterface >= TP_MAX_INTERFACE)) {
         return -3;
      }
   
      /* read index */
      len = tpQuery (node, tpinterface, indx, TP_MAX_INDEX, time, epoch);
      if (len <= 0) {
         return -4;
      }
   
      /* now search through index */
      for (i = 0; i < len; i++) {
         if (indx[i] == tp) {
            /* found it: calculate address */
            rmOfs = 
               TP_NODE_INTERFACE_TO_DATA_OFFSET (node, tpinterface);
            rmBlkSize = 
               TP_NODE_INTERFACE_TO_DATA_BLOCKSIZE (node, tpinterface);
            if ((rmOfs < 0) || (rmBlkSize < 0)) {
               return -5;
            }
            rmOfs += TP_DATUM_LEN * i * 
                     TP_INTERFACE_TO_CHN_LEN (tpinterface);
            return CHN_ADDR (rmOfs, rmBlkSize, epoch);
         }
      }
      /* not in index */
      return -1;
   #else
      return -2;
   #endif
   }

#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: getIndexDirect				*/
/*                                                         		*/
/* Procedure Description: gets a test point index			*/
/*                                                         		*/
/* Procedure Arguments: node, interface, time, epoch, tp list & length	*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if not			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int tpGetIndexDirect (int node, int tpinterface, testpoint_t tp[], 
                     int tplen, taisec_t time, int epoch)
   {
   #if (_TESTPOINT_DIRECT == 0) || defined (_NO_TESTPOINTS)
      return -10;
   #else
      int		len;		/* length of index */
      int 		cur;		/* index into index cache */
   
      /* check node */
      if ((node < 0) || (node >= TP_MAX_NODE)) {
         return -2;
      }
   
      /* check interface */
      if ((tpinterface < 0) || (tpinterface >= TP_MAX_INTERFACE)) {
         return -2;
      }
   
      /* test if node is included in test point direct */
      if ((_TESTPOINT_DIRECT & (1 << node)) == 0) {
         return -1;
      }
   
      /* get length and address of index */
      len = TP_INTERFACE_TO_INDEX_LEN (tpinterface);
      if (len > tplen) {
         len = tplen;
      }
      if (len < 0) {
         return -3;
      }
   
      /* get mutex */
      MUTEX_GET (tpmux);
   
      /* search for appropriate index */
      cur = tpindexcur;
      do {
         if (time >= tpindexlist[cur].time) {
            break;
         }
         cur = (cur + _MAX_INDEX_CACHE - 1) % _MAX_INDEX_CACHE;
      } while (cur != tpindexcur);
      /* request too far in the past */
      if (time < tpindexlist[cur].time) {
         printf ("tp request time %li error for %li\n", time, 
                 tpindexlist[cur].time);
         MUTEX_RELEASE (tpmux);
         return -4;
      }
   
      /* copy list */
      if (tp != NULL) {
         memcpy (tp, tpindexlist[cur].tp[node][tpinterface], 
                len * sizeof (testpoint_t));
      }
   
      /* release mutex and return */
      MUTEX_RELEASE (tpmux);
      return len;
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: cmdreply					*/
/*                                                         		*/
/* Procedure Description: command reply					*/
/*                                                         		*/
/* Procedure Arguments: string						*/
/*                                                         		*/
/* Procedure Returns: newly allocated char*				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/   
   static char* cmdreply (const char* m)
   {
      if (m == 0) {
         return 0;
      }
      else {
         char* p = (char*) malloc (strlen (m));
         if (p != 0) {
            strcpy (p, m);
         }
         return p;
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: queryCmd					*/
/*                                                         		*/
/* Procedure Description: queries tp's and returns a description	*/
/*                                                         		*/
/* Procedure Arguments: buffer, node					*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void queryCmd (char* buf, int node)
   {
      int		i;
      char*		p;
      testpoint_t	tp[TP_MAX_INDEX]; /* test points */
      int		num;	/* number of test points */
   
      sprintf (buf, "Test points for node %i\n", node);
      /* query lsc exc */
      num = tpQuery (node, TP_LSC_EX_INTERFACE, tp, 
                    TP_MAX_INDEX, 0, 0);
      p = strend (buf);
      sprintf (p, "LSC EX:");
      p = strend (p);
      if (num < 0) { 
         sprintf (p, " invalid\n");
         return;
      }
      for (i = 0; i < num; i++) {
         sprintf (p, " %i", tp[i]);
         p = strend (p);
      }
      sprintf (p++, "\n");
      /* query lsc tp */
      num = tpQuery (node, TP_LSC_TP_INTERFACE, tp, 
                    TP_MAX_INDEX, 0, 0);
      p = strend (buf);
      sprintf (p, "LSC TP:");
      p = strend (p);
      if (num < 0) { 
         sprintf (p, " invalid\n");
         return;
      }
      for (i = 0; i < num; i++) {
         sprintf (p, " %i", tp[i]);
         p = strend (p);
      }
      sprintf (p++, "\n");
      /* query asc exc */
      num = tpQuery (node, TP_ASC_EX_INTERFACE, tp, 
                    TP_MAX_INDEX, 0, 0);
      p = strend (buf);
      sprintf (p, "ASC EX:");
      p = strend (p);
      if (num < 0) { 
         sprintf (p, " invalid\n");
         return;
      }
      for (i = 0; i < num; i++) {
         sprintf (p, " %i", tp[i]);
         p = strend (p);
      }
      sprintf (p++, "\n");
      /* query asc tp */
      num = tpQuery (node, TP_ASC_TP_INTERFACE, tp, 
                    TP_MAX_INDEX, 0, 0);
      p = strend (buf);
      sprintf (p, "ASC TP:");
      p = strend (p);
      if (num < 0) { 
         sprintf (p, " invalid\n");
         return;
      }
      for (i = 0; i < num; i++) {
         sprintf (p, " %i", tp[i]);
         p = strend (p);
      }
      sprintf (p++, "\n");
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: tpCommand					*/
/*                                                         		*/
/* Procedure Description: command line interface			*/
/*                                                         		*/
/* Procedure Arguments: command string					*/
/*                                                         		*/
/* Procedure Returns: reply string					*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   char* tpCommand (const char* cmd)
   {
   #ifdef _NO_TESTPOINTS
      return NULL;
   #else
      int		i;
      char*		p;
      int		node;	/* node */
      char*		buf;
      testpoint_t	tp[TP_MAX_INDEX];
   
      /* help */
      if (gds_strncasecmp (cmd, "help", 4) == 0) {
         return cmdreply (_HELP_TEXT);
      }
      /* show */
      else if (gds_strncasecmp (cmd, "show", 4) == 0) {
         p = (char*) (cmd + 4);
         while (*p == ' ') {
            p++;
         }
         if (*p == '*') {
            buf = malloc (TP_MAX_NODE * 2000);
            p = buf;
            for (node = 0; node < TP_MAX_NODE; node++) {
               if (tpNode[node].valid) {
                  queryCmd (p, node);
                  p = strend (p);
               }
            }
         }
         else {
            node = *p - (int) '0';
            if ((node < 0) || (node >= TP_MAX_NODE) ||
               (!tpNode[node].valid)) {
               return cmdreply ("error: invalid node number");
            }
            buf = malloc (2000);
            queryCmd (buf, node);
         }
         buf = realloc (buf, strlen (buf) + 1);
         return buf;
      }
      /* set */
      else if (gds_strncasecmp (cmd, "set", 3) == 0) {
         p = (char*) (cmd + 3);
         while (*p == ' ') {
            p++;
         }
         node = *p - (int) '0';
         if ((node < 0) || (node >= TP_MAX_NODE)) {
            /* assume channel names are specified */
            if (tpRequestName (p, -1, NULL, NULL) < 0) {
               return cmdreply ("error: unable to set test point");
            }
            else {
               return cmdreply ("test point set");
            }
         }
         else {
            /* assume test point numbers are specified */
            if (!tpNode[node].valid) {
               return cmdreply ("error: invalid node number");
            }
            /* read testpoint numbers */
            i = 0;
            do {
               p++;
               while (*p == ' ') {
                  p++;
               }
               tp [i++] = strtol (p, &p, 10);
            } while ((tp[i-1] != 0) && (i < TP_MAX_INDEX));
            /* set test point */
            if (tpRequest (node, tp, i, -1, NULL, NULL) < 0) {
               return cmdreply ("error: unable to set test point");
            }
            else {
               return cmdreply ("test point set");
            }
         }
      }
      /* clear */
      else if (gds_strncasecmp (cmd, "clear", 5) == 0) {
         p = (char*) (cmd + 5);
         while (*p == ' ') {
            p++;
         }
      	 /* read node */
         if (*p == '*') {
            for (node = 0; node < TP_MAX_NODE; node++) {
               if (tpNode[node].valid) {
                  tpClear (node, NULL, 0);
               }
            }
            return cmdreply ("test point cleared");
         }
         /* try reading node */
         node = *p - (int) '0';
         if ((node < 0) || (node >= TP_MAX_NODE)) {
            /* assume channel names are specified */
            if (tpClearName (p) < 0) {
               return cmdreply ("error: unable to clear test point");
            }
            else {
               return cmdreply ("test point cleared");
            }
         }
         else {
            /* assume test point numbers are specified */
            if (!tpNode[node].valid) {
               return cmdreply ("error: invalid node number");
            }
             /* read testpoint numbers */
            i = 0;
            do {
               p++;
               while (*p == ' ') {
                  p++;
               }
               if (*p == '*') {
                  tp[i++] = _TP_CLEAR_ALL;
               }
               else {
                  tp [i++] = strtol (p, &p, 10);
               }
            } while ((tp[i-1] != 0) && (i < TP_MAX_INDEX));
            /* clear test point */
            if (tpClear (node, tp, i) < 0) {
               return cmdreply ("error: unable to clear test point");
            }
            else {
               return cmdreply ("test point cleared");
            }
         }
      }
      else {
         return cmdreply ("error: unrecognized command\n"
                         "use help for further information");
      }
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: tpcmdline					*/
/*                                                         		*/
/* Procedure Description: command line interface			*/
/*                                                         		*/
/* Procedure Arguments: command string					*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 on error			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int tpcmdline (const char* cmd)
   {
      char* 		p;
      int		ret;
   
      p = tpCommand (cmd);
      if (p == NULL) {
         printf ("error: testpoints not supported\n");
         return -2;
      }
      ret = (strncmp (p, "error:", 6) == 0) ? -1 : 0;
      printf ("%s\n", p);
   
      free (p);
      return ret;
   }


#if !defined(_NO_KEEP_ALIVE) && !defined(_NO_TESTPOINTS)
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: keepAlive					*/
/*                                                         		*/
/* Procedure Description: sends keep alive				*/
/*                                                         		*/
/* Procedure Arguments: scheduler task, time, epoch, argument		*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if not			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int keepAlive (schedulertask_t* info, taisec_t time, 
                     int epoch, void* arg)
   {
      int		node;		/* node index */
      CLIENT*		clnt;		/* client rpc handle */
      int		ret;		/* return value */
   
   /*printf("keep alive\n");*/
   #ifdef OS_VXWORKS
      rpcTaskInit();
   #endif
      gdsDebug ("send keep alive");
   
      /* send keep alive to every test point node which is not
         directly accessible */
      for (node = 0; node < TP_MAX_NODE; node++) {
         if ((!tpNode[node].valid) || (tpNode[node].duplicate) ||
            ((_TESTPOINT_DIRECT & (1 << node)) != 0)) {
            continue;
         }
         /* make client handle */
         clnt = tpMakeHandle (node);
         if (clnt == NULL) {
            continue;
         }
         /* call remote procedure */
         if ((keepalive_1 (tpNode[node].id, &ret, clnt) != RPC_SUCCESS) || 
            (ret < 0)) {
            /* connection lost; try to reconnect */
         #ifdef _NO_KEEP_ALIVE
            tpNode[node].id = -2;
         #else
            tpNode[node].id = -1;
         #endif
            /* call remote procedure */
            if ((keepalive_1 (tpNode[node].id, &tpNode[node].id, clnt) !=
               RPC_SUCCESS) || (tpNode[node].id < 0)) {
               /*tpNode[node].valid = 0;*/
            }
         } else {
	    /* Assign returned ID number; otherwise we'd go into infinite
		loop here, constantly reallocating handle on server */
	    if (ret >= 0) tpNode[node].id = ret;
	 }
         clnt_destroy (clnt);
      }
      return 0;
   }
#endif


#ifndef _TP_DAQD
#if (_TESTPOINT_DIRECT != 0) && !defined (_NO_TESTPOINTS)
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: readTestpoints				*/
/*                                                         		*/
/* Procedure Description: reads test point indexes			*/
/*                                                         		*/
/* Procedure Arguments: scheduler task, time, epoch, argument		*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if not			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int readTestpoints (schedulertask_t* info, taisec_t time, 
                     int epoch, void* arg)
   {
      int		node;		/* node index */
      int		i;		/* interface index */
      int		j;		/* index into test point list */
      int		len;		/* length of index */
      int		addr;		/* RM ddress of tp index */
   
      /*printf("read testpoints\n");*/
      /* get mutex and increase cursor */
      MUTEX_GET (tpmux);
      tpindexcur = (tpindexcur + 1) % _MAX_INDEX_CACHE;
   
      /* set time and epoch */
      tpindexlist[tpindexcur].time = time + 1;
      tpindexlist[tpindexcur].epoch = 0;
      /*printf ("read TP at %li (%i)\n", time, epoch);*/
   
      for (node = 0; node < TP_MAX_NODE; node++) {
         /* test if node is included in test point direct */
         if ((_TESTPOINT_DIRECT & (1 << node)) == 0) {
            continue;
         }
      	 /* loop over interfaces */
         for (i = 0; i < TP_MAX_INTERFACE; i++) {
            /* get length and address of index */
            len = TP_INTERFACE_TO_INDEX_LEN (i);
            addr = TP_NODE_INTERFACE_TO_INDEX_OFFSET (node, i);
            if ((len <= 0) || (addr < 0)) {
               continue;
            }
	    /* read index */
	    /* debug */
	    /*printf("read testpoint cache for node %d\n", node);*/
            rmRead (TP_NODE_ID_TO_RFM_ID (node), 
                   (char*) tpindexlist[tpindexcur].tp[node][i], 
                   addr, 2*(int)((len+1)/2) * sizeof (testpoint_t), 0);
#ifndef __linux__
            /* swap on little-endian machines */
	    j = 0;
            *(char*)&j = 1;
            if (j == 1) {
               testpoint_t* tp = tpindexlist[tpindexcur].tp[node][i];
               for (j = 0; j < len; j += 2) {
                  testpoint_t temp = tp[j];
                  tp[j] = tp[j+1]; tp[j+1] = temp;
                  /* tp[j] = ((tp[j]&0xFF)<<8) | ((tp[j]&0xFF00)>>8); */
               }
            }
#endif
            /* debug */
            /*if ((node == 0) && (i == 0)) {
               static int counttpr = 0;
               if (++counttpr % 10 == 0) {
               printf ("read tps %i %i %i from node %i from 0x%x\n",
                    tpindexlist[tpindexcur].tp[node][i][0],
                    tpindexlist[tpindexcur].tp[node][i][1],
                    tpindexlist[tpindexcur].tp[node][i][2],
                    TP_NODE_ID_TO_RFM_ID (node), addr);
               }
            }*/
         }
      }
      MUTEX_RELEASE (tpmux);
      return 0;
   }
#endif
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: testpoint_client				*/
/*                                                         		*/
/* Procedure Description: installs test point client interface		*/
/*                                                         		*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/           
   int testpoint_client (void)
   {
   #ifdef _NO_TESTPOINTS
      return -10;
   #else
      int		node;		/* node id */
      struct timeval	timeout;	/* timeout for probe */
      CLIENT*		clnt;		/* client rpc handle */
      int		status;		/* rpc status */
   #if !defined(_NO_KEEP_ALIVE)
      int		keepAliveNum;	/* # of keep alives */
   #endif
   #if defined (_CONFIG_DYNAMIC)
      const char* const* cinfo;		/* configuration info */
      confinfo_t	crec;		/* conf. info record */
   #endif
   
      /* already initialized */
      if (tp_init == 2) {
         return tpNum;
      }
   
   #ifdef OS_VXWORKS
      rpcTaskInit();
   #endif
      gdsDebug ("start test point client");
   
      /* intialize interface first */
      if (tp_init == 0) {
         initTestpoint();
         if (tp_init == 0) {
            gdsError (GDS_ERR_MEM, "failed to initialze test points");
            return -1;
         }
	 /* Log the version ID */
	 printf("testpoint_client %s\n", versionId) ;
      }
   
      /* dynamic configuration */
   #if defined (_CONFIG_DYNAMIC)
      for (cinfo = getConfInfo (0, 0); cinfo && *cinfo; cinfo++) {
         if ((parseConfInfo (*cinfo, &crec) == 0) &&
            (gds_strcasecmp (crec.interface, 
                             CONFIG_SERVICE_TP) == 0) &&
            (crec.ifo >= 0) && (crec.ifo < TP_MAX_NODE) &&
            (crec.port_prognum > 0) && (crec.progver > 0)) {
            tpSetHostAddress (crec.ifo, crec.host, 
                             crec.port_prognum, crec.progver);
         }
      }
   #endif
   
      /* install heartbeat */
   #if (_TESTPOINT_DIRECT != 0) || !defined (_NO_KEEP_ALIVE)
      if (installHeartbeat (NULL) < 0) {
         gdsError (GDS_ERR_MEM, "failed to install heartbeat");
         return -2;
      }
   
      /* create scheduler */
      tpsched = createScheduler (0, NULL, NULL);
      if (tpsched == NULL) {
         gdsError (GDS_ERR_MEM, 
                  "failed to create test point scheduler");
         return -3;
      }
   #endif
   
      timeout.tv_sec = RPC_PROBE_WAIT;
      timeout.tv_usec = 0;
      for (node = 0; node < TP_MAX_NODE; node++) {
      #ifdef OS_VXWORKS
         /* avoid long timeouts under VxWorks */
         if (tpNode[node].valid) {
            tpNum++;
         }
      #else
         if ((tpNode[node].valid) &&
            (rpcProbe (tpNode[node].hostname, tpNode[node].prognum, 
            tpNode[node].progver, _NETID, &timeout, NULL))) {
            tpNum++;
         }
         else {

#ifdef _TP_DAQD
		// Do not fail this one if we are in the frame builder
		tpNum++;
#else
            tpNode[node].valid = 0;
	    /* printf("failed to find testpoint manager on node %d\n", node); */
#endif
         }
      #endif
      }
   
#if (_TESTPOINT_DIRECT != 0) || !defined (_NO_KEEP_ALIVE)
      /* initialize keep alive for all test point interfaces
         which are not accessible directly */
      for (node = 0; node < TP_MAX_NODE; node++) {
         if ((!tpNode[node].valid) ||
            ((_TESTPOINT_DIRECT & (1 << node)) != 0)) {
            continue;
         } 
         else if (_TESTPOINT_DIRECT != 0) {
            tpNode[node].valid = 0;
            continue;
         }
         /* handle duplicates */
         if (tpNode[node].duplicate) {
            tpNode[node].id = tpNode[tpNode[node].id].id;
            continue;
         }
      
         /* make client handle */
         clnt = tpMakeHandle (node);
         if (clnt == NULL) {
            gdsError (GDS_ERR_MEM, 
                     "failed to create test point rpc handle");
            return -4;
         }
      #ifdef _NO_KEEP_ALIVE
         tpNode[node].id = -2;
      #else
         tpNode[node].id = -1;
      #endif

         /* call remote procedure */
         status = keepalive_1 (tpNode[node].id, &tpNode[node].id, clnt);
         if ((status != RPC_SUCCESS) || (tpNode[node].id < 0)) {
            closeScheduler (tpsched, 3 * _EPOCH);
            tpsched = NULL;
            gdsError (GDS_ERR_MEM, 
                     "contact with test point manager failed");
            tpNode[node].valid = 0;
	    tpNum--;
         }
         clnt_destroy (clnt);
      }
#endif
   
      /* start keep alive task */
   #if !defined(_NO_KEEP_ALIVE)
      keepAliveNum = 0;
      for (node = 0; node < TP_MAX_NODE; node++) {
         if (tpNode[node].valid &&
            ((_TESTPOINT_DIRECT & (1 << node)) == 0)) {
            keepAliveNum++;
         }
      }
      if (keepAliveNum > 0) {
         schedulertask_t	task;	/* task info */
         /* setup task info structure for tp read task*/
         SET_TASKINFO_ZERO (&task);
         task.flag = SCHED_REPEAT | SCHED_WAIT | SCHED_ASYNC;
         task.waittype = SCHED_WAIT_IMMEDIATE;
         task.repeattype = SCHED_REPEAT_INFINITY;
         task.repeatratetype = SCHED_REPEAT_EPOCH;
         task.repeatrate = _KEEP_ALIVE_RATE * NUMBER_OF_EPOCHS;
         task.synctype = SCHED_SYNC_EPOCH;
         task.syncval = 0;
         task.func = keepAlive;
      
         /* schedule test point read task */
         if (scheduleTask (tpsched, &task) < 0) {
            closeScheduler (tpsched, 3 * _EPOCH);
            tpsched = NULL;
            gdsError (GDS_ERR_MEM, 
                     "failed to create test point read task");
            return -6;
         }
      }
   #endif
   
      /* install test point read task */
   #if (_TESTPOINT_DIRECT != 0)
      {
         schedulertask_t	task;	/* task info */
         
         /* setup task info structure for tp read task*/
         SET_TASKINFO_ZERO (&task);
         task.flag = SCHED_REPEAT | SCHED_WAIT;
         task.waittype = SCHED_WAIT_IMMEDIATE;
         task.repeattype = SCHED_REPEAT_INFINITY;
         task.repeatratetype = SCHED_REPEAT_EPOCH;
         task.repeatrate = NUMBER_OF_EPOCHS;
         task.synctype = SCHED_SYNC_EPOCH;
         task.syncval = TESTPOINT_VALID1;
         task.func = readTestpoints;
      
         /* schedule test point read task */
         if (scheduleTask (tpsched, &task) < 0) {
            closeScheduler (tpsched, 3 * _EPOCH);
            tpsched = NULL;
            gdsError (GDS_ERR_MEM, 
                     "failed to create test point read task");
            return -7;
         }
      }
   #endif
   
      tp_init = 2;
      return tpNum;
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: testpoint_cleanup				*/
/*                                                         		*/
/* Procedure Description: cleans up the test point client interface	*/
/*                                                         		*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/       
   void testpoint_cleanup (void) 
   {
   #ifndef _NO_TESTPOINTS
      if (tp_init <= 1) {
         return;
      }
   #if (_TESTPOINT_DIRECT != 0) || !defined (_NO_KEEP_ALIVE)
      if (tp_init == 2) {
         closeScheduler (tpsched, 0);
         tpsched = NULL;
      }
   #endif
      tp_init = 1;
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: initTestpoint				*/
/*                                                         		*/
/* Procedure Description: initializes test point interface		*/
/*                                                         		*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void initTestpoint (void)
   {
   #ifndef _NO_TESTPOINTS
      extern char system_name[32];	/* Control system name */
      int		node;		/* node id */
   #if !defined (_TP_DAQD) && !defined (_CONFIG_DYNAMIC)
      char 		remotehost[PARAM_ENTRY_LEN];
   				        /* remote hostname */
      char		section[30];	/* section name */
      struct in_addr 	addr;		/* host address */
      unsigned long	prognum;	/* rpc prog. num. */
      unsigned long	progver;	/* rpc prog. ver. */
      int		k;		/* node index */
   #endif
      /* test if already initialized */
      if (tp_init != 0) {
         return;
      }
   #if (_TESTPOINT_DIRECT != 0) || !defined (_NO_KEEP_ALIVE)
      tpsched = NULL;
   #endif
   
      tpNum = 0;
   #if (_TESTPOINT_DIRECT != 0)
      if (MUTEX_CREATE (tpmux) != 0) {
         gdsError (GDS_ERR_MEM, "unable to create test point mutex");
         return;
      }
      tpindexcur = 0;
      memset (tpindexlist, 0, sizeof (tpindexlist));
   #endif
   
      for (node = 0; node < TP_MAX_NODE; node++) {
         /* make section header */
         tpNode[node].valid = 0;
      /*#if (_TESTPOINT_DIRECT != 0)
         if ((_TESTPOINT_DIRECT & (1 << node)) != 0) {
            tpNode[node].valid = 1;*/
      #if !defined (_TP_DAQD) && !defined (_CONFIG_DYNAMIC)
         sprintf (section, "%s-node%i", site_prefix, node);
      
         /* get remote host from parameter file */
         strcpy (remotehost, "");
         loadStringParam (PRM_FILE, section, PRM_ENTRY1, 
                         remotehost);
         if ((strcmp (remotehost, "") == 0) ||
            (rpcGetHostaddress (remotehost, &addr) != 0)) {
            continue;
         }
         inet_ntoa_b (addr, tpNode[node].hostname);
      
         /* get rpc parameters from parameter file */
         prognum = RPC_PROGNUM_TESTPOINT;
         progver = RPC_PROGVER_TESTPOINT;
         loadNumParam (PRM_FILE, section, PRM_ENTRY2, &prognum);
         loadNumParam (PRM_FILE, section, PRM_ENTRY3, &progver);
         if ((prognum != 0) && (progver != 0)) {
            tpNode[node].prognum = prognum;
            tpNode[node].progver = progver;
            tpNode[node].valid = 1;
         }
         else {
            continue;
         }
      
         /* look for identical nodes */
         for (k = node - 1; k >= 0; k--) {
            if ((tpNode[k].valid) &&
               (gds_strcasecmp (tpNode[k].hostname, 
               tpNode[node].hostname) == 0) &&
               (tpNode[k].prognum == tpNode[node].prognum) &&
               (tpNode[k].progver == tpNode[node].progver)) {
               break;
            }
         }
         tpNode[node].duplicate = (k >= 0);
         if (tpNode[node].duplicate) {
            tpNode[node].id = k;
         }
      #endif
      }
   
      /* intialized */
      tp_init = 1;
   #endif
   }


#if defined (_TP_DAQD) || defined (_CONFIG_DYNAMIC)
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: tpSetHostAddress				*/
/*                                                         		*/
/* Procedure Description: cleans up test point interface		*/
/*                                                         		*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int tpSetHostAddress (int node, const char* hostname, 
                     unsigned long prognum, unsigned long progver)
   {
      int		k;
   
      if ((node < 0) || (node >= TP_MAX_NODE)) {
         return -1;
      }
      /* set node parameters */
      tpNode[node].valid = 1;
      strncpy (tpNode[node].hostname, hostname, 
              sizeof (tpNode[node].hostname));
      tpNode[node].hostname[sizeof(tpNode[node].hostname)-1] = 0;
      tpNode[node].prognum = (prognum > 0) ? 
                           prognum : RPC_PROGNUM_TESTPOINT;
      tpNode[node].progver = (progver > 0) ? 
                           progver : RPC_PROGVER_TESTPOINT;
   
      /* look for identical nodes */
      for (k = node - 1; k >= 0; k--) {
         if ((tpNode[k].valid) &&
            (gds_strcasecmp (tpNode[k].hostname, 
            tpNode[node].hostname) == 0) &&
            (tpNode[k].prognum == tpNode[node].prognum) &&
            (tpNode[k].progver == tpNode[node].progver)) {
            break;
         }
      }
      tpNode[node].duplicate = (k >= 0);
      if (tpNode[node].duplicate) {
         tpNode[node].id = k;
      }
      printf ("TP: node = %i, host = %s, dup = %i, prog = 0x%x, vers = %i\n",
              node, tpNode[node].hostname, tpNode[node].duplicate,
              (int)tpNode[node].prognum, (int)tpNode[node].progver);
      return 0;
   }
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: finiTestpoint				*/
/*                                                         		*/
/* Procedure Description: cleans up test point interface		*/
/*                                                         		*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void finiTestpoint (void)
   {
   #ifndef _NO_TESTPOINTS
      if (tp_init == 0) {
         return;
      }
      if (tp_init > 1) {
         /*tp_init = 3; make sure we don't call closeScheduler */
         testpoint_cleanup ();
      }
   
   #if (_TESTPOINT_DIRECT != 0)
      MUTEX_GET (tpmux);
      MUTEX_DESTROY (tpmux);
   #endif
   
      tp_init = 0;
   #endif
   }

