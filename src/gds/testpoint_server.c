static char *versionId = "Version $Id$" ;
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: testpoint_server					*/
/*                                                         		*/
/* Module Description: implements server functions for handling the 	*/
/* test point interface							*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#ifndef DEBUG
/* Only uncomment this for debugging, do not leave uncommented in a live system */
/*#define DEBUG 3*/
#endif

#define RPC_SVC_FG

#if 0
#ifndef _TESTPOINT_DIRECT
#define _TESTPOINT_DIRECT 0
#endif
#endif

/* Header File List: */
#ifndef __EXTENSIONS__
#define __EXTENSIONS__
#endif
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#ifdef OS_VXWORKS
#include <vxWorks.h>
#include <semLib.h>
#include <taskLib.h>
#include <taskVarLib.h>
#include <sockLib.h>
#include <inetLib.h>
#include <hostLib.h>
#include <signal.h>
#include <sysLib.h>
#include <timers.h>

#else
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
/*#include <netdir.h>*/
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <pthread.h>
#endif

#include "dtt/gdsutil.h"
#include "dtt/rtestpoint.h"
#include "dtt/hardware.h"
#include "dtt/gdschannel.h"
#ifndef _NO_TESTPOINTS
#include "dtt/testpoint.h"
#include "dtt/testpointinfo.h"
#include "dtt/gdssched.h"
#include "dtt/rmorg.h"
#include "dtt/rmapi.h"
#include "dtt/confserver.h"
#endif
#include "PConfig.h"

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Constants: _KEEPALIVE_TIMEOUT  timeout for keep alive signal		*/
/* 	      _KEEP_AROUND	  time of keeping an expired client	*/
/* 	      _CHNNAME_SIZE	  size of channel name			*/
/* 	      _TP_MAX_USER	  max. number of clients/users		*/
/* 	      _MAX_TPNAMES	  max. number of tp names in one call	*/
/* 	      PRM_FILE		  parameter file name			*/
/*            PRM_SECTION	  parameter section name		*/
/*            PRM_ENTRY1	  entry name for host 			*/
/*            PRM_ENTRY2	  entry name for rpc prog num		*/
/*            PRM_ENTRY3	  entry name for rpc ver num		*/
/*            PRM_ENTRY4	  entry name for control system name	*/
/*            _NODE0_LSCX_UNIT_ID unit id of 4K LSC excitation		*/
/*            _NODE0_ASCX_UNIT_ID unit id of 4K ASC excitation		*/
/*            _NODE1_LSCX_UNIT_ID unit id of 2K LSC excitation		*/
/*            _NODE1_ASCX_UNIT_ID unit id of 2K ASC excitation		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#define _KEEPALIVE_TIMEOUT	60
#define _KEEP_AROUND		240
#define _CHNNAME_SIZE		MAX_CHNNAME_SIZE
#define _TP_MAX_USER		1000
#define _MAX_TPNAMES		1000
#define PRM_FILE		gdsPathFile ("/param", "testpoint.par")
#define PRM_SECTION		gdsSectionSite ("node%i")
#define PRM_PRESELECT		"preselect%i"
#define PRM_ENTRY1		"hostname"
#define PRM_ENTRY2		"prognum"
#define PRM_ENTRY3		"progver"
#define PRM_ENTRY4		"system"

#ifndef _NO_AWG_CHECK
#define _NODE0_LSCX_UNIT_ID	GDS_4k_LSC_EX_ID
#define _NODE0_ASCX_UNIT_ID	GDS_4k_ASC_EX_ID
#define _NODE1_LSCX_UNIT_ID	GDS_2k_LSC_EX_ID
#define _NODE1_ASCX_UNIT_ID	GDS_2k_ASC_EX_ID
#endif

#ifdef RPC_SVC_FG
#define _SVC_FG		1
#else
#define _SFC_FG		0
#endif
#if !defined (OS_VXWORKS) && !defined (PORTMAP)
// NEED FIXING, VERIFICATION
//#define _SVC_MODE	RPC_SVC_MT_AUTO
#define _SVC_MODE       0
#else
#define _SVC_MODE	0
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Types: tpClient_t		client entry (keep alive)		*/
/* 	  tpUsage_t		client (user) id			*/
/* 	  tpEntry_t		internal rep. of a test point		*/
/*        tpNode_t		test point node				*/
/*        tpwriter_t		info on ddcus which write into tp area	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#ifndef _NO_TESTPOINTS

   struct tpClient_t {
      /* valid */
      int		valid;
      /* time of last contact */
      tainsec_t		lastTime;
   };
   typedef struct tpClient_t tpClient_t;

   struct tpUsage_t {
      /* time of request */
      tainsec_t		reqTime;
      /* timeout */
      tainsec_t		timeout;
      /* client id */
      int		clntId;
   };
   typedef struct tpUsage_t tpUsage_t;

   struct tpEntry_t {
      /* testpoint ID */
      testpoint_t	id;
      /* in use count */
      int		inUse;
      /* user identification array */
      tpUsage_t		users[_TP_MAX_USER];
      /* channel name of testpint */
      char		name[_CHNNAME_SIZE];
   };
   typedef struct tpEntry_t tpEntry_t;

   struct tpNode_t {
      /* true if node is valid */
      int		valid;
      /* Node number of the node. */
      int		node ;
      /* rpc hostname */
      char		hostname[80];
      /* rpc prog num */
      unsigned long	prognum;
      /* rpc prog ver. */
      unsigned long	progver;
      /* list of preselected testpoints */
      testpoint_t	preselect[TP_MAX_PRESELECT];
      /* list of TP active TPs */
      tpEntry_t		indx[TP_MAX_INTERFACE][TP_MAX_INDEX];
      /* points directly into RM IPC area 
      rmIpcStr*		ipc[TP_MAX_INTERFACE]; */
      /* points directly into RM channel info area
      dataInfoStr*	chninfo[TP_MAX_INTERFACE]; */
      /* list of testpoint channel information */
      gdsChnInfo_t*	tplist;
      /* length of list */
      int		tplistlen;
      /* fast lookup table: tp ID -> channel info */
      gdsChnInfo_t*	tplookup[TP_HIGHEST_INDEX];
   };
   typedef struct tpNode_t tpNode_t;

#ifndef _NO_AWG_CHECK
   typedef struct rmIpcStr rmIpcStr;
   struct tpwriter_t {
      /* pointer to exc. engine IPC area */
      rmIpcStr*		IPC;
      /* awg alive if true */
      int		alive;
      /* old cycle count */
      int		oldCycleCount;
   };
   typedef struct tpwriter_t tpwriter_t;
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Globals: sd			scheduler				*/
/*	    servermux		protects globals			*/
/*          initServer		if 0 the server is not yet initialized	*/
/*          shutdownflag	shutdown flag				*/
/*          tplist		list of test points			*/
/*          tpclnts	        client list with keep alive   		*/
/*          tpclntID	        client ID for clients w/o keep alive	*/
/*          tpwriter		information on DCUs writing to tp	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static scheduler_t* 		sd = NULL;
   static mutexID_t		servermux;
   static int			initServer = 0;
   static int			shutdownflag = 0;
   static tpNode_t		tpNode ;
   static tpClient_t		tpclnts[_TP_MAX_USER];
   static int			tpclntID = _TP_MAX_USER + 1;
#ifndef _NO_AWG_CHECK
   static tpwriter_t		tpwriter[TP_MAX_INTERFACE];
#endif
#endif
   /* Global. Test point manager's node id */
   int 				testpoint_manager_node = -1;
   /* Global. Test point manager's RPC program number*/
   int 				testpoint_manager_rpc = -1;

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Forward declarations: 						*/
/*	initTestpointServer	initializes testpoint server		*/
/*	finiTestpointServer	termionates testpoint server		*/
/*	tpName2Index		translate tp names into id numbers	*/
/*	finiTestpointServer	cleans up testpoint server		*/
/*	initializeTestpoints	sets up testpoint server		*/
/*	cleanupTestpoints	garbage collector for test points	*/
/*	updateTestpoints	updates testpoints in the refl. mem.	*/
/*      rtestpoint_1		rpc dispatch function			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   __init__(initTestpointServer);
#pragma init(initTestpointServer);
   __fini__(finiTestpointServer);
#pragma fini(finiTestpointServer);
#ifndef _NO_TESTPOINTS
   void setPreselect (int node);
   static int tpName2Index (int node, const char* tpNames, TP_r* tp);
   static int initializeTestpoints (void);
   static int cleanupTestpoints (schedulertask_t* info, taisec_t time, 
                     int epoch, void* arg);
   static int updateTestpoints (schedulertask_t* info, taisec_t time, 
                     int epoch, void* arg);
   extern void rtestpoint_1 (struct svc_req* rqstp, 
                     register SVCXPRT* transp);
   extern int curDaqBlockSize;
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Remote Procedure Name: requesttp_1_svc				*/
/*                                                         		*/
/* Procedure Description: sets test points				*/
/*                                                         		*/
/* Procedure Arguments: node ID, test point list, timeout		*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if not (status)		*/
/*                    time and epoch of activation         		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   bool_t requesttp_1_svc (int id, int node, TP_r tp, quad_t timeout, 
                     resultRequestTP_r* result, struct svc_req* rqstp)
   {
   #ifdef _NO_TESTPOINTS
      result->status = -10;
      return TRUE;
   
   #else
      int		i,j,k;		/* index into test point list */
      int		tpinterface;	/* tp interface id */
      int		len;		/* length of test point list */
      tpEntry_t*	tpp;		/* test point entry */
      tainsec_t		time;		/* activation time */
   
      gdsDebug ("request test point");
   
      #ifdef DEBUG
      printf ("Request tp id=%d; node=%d; node_valid=%d\n",
    	       id, node, tpNode.valid);
   
      /* JCB */
      printf("Request %d testpoint(s)\n", tp.TP_r_len) ;
      for (i = 0; i < tp.TP_r_len; i++)
	 printf("request tp %d\n", tp.TP_r_val[i]) ;
      #endif

      /* test node ID */
      if ((id < 0) || (node < 0) || (node >= TP_MAX_NODE) ||
         !tpNode.valid || tpNode.node != node) {
         result->status = -2;
         return TRUE;
      }
   
      /* get server mutex */
      MUTEX_GET (servermux);
   
      /* go through supplied test point list */
      result->status = 0;
      #ifdef DEBUG
      printf ("Test point list length is %d\n", tp.TP_r_len);
      #endif


      // Loop through the list of testpoint to set and check for DAQ overload
      int projTpNum = 0; /* Projected test point number */
      for (i = 0; i < tp.TP_r_len; i++) {
         /* check if ID in list */
         if ((tp.TP_r_val[i] <= 0) || (tp.TP_r_val[i] >= TP_HIGHEST_INDEX) || (tpNode.tplookup[tp.TP_r_val[i]] == NULL)) {
            continue;
         }
      
         /* test whether valid test point */
         tpinterface = TP_ID_TO_INTERFACE (tp.TP_r_val[i]);
         /* use LSC EXC for DAC channels */
         if (tpinterface == TP_DAC_INTERFACE) {
            tpinterface = TP_LSC_EX_INTERFACE;
         }
         len = TP_INTERFACE_TO_INDEX_LEN (tpinterface);
         if ((tpinterface < 0) || (tpinterface >= TP_MAX_INTERFACE)) {
            continue;
         }
      
         /* test whether already in list */
         for (j = 0, k = -1; j < len; j++) {
            tpp = &tpNode.indx[tpinterface][j];
            /* check for duplicate */
            if (tpp->id == tp.TP_r_val[i]) {
               /* already in list; check in use */
               if (tpp->inUse < _TP_MAX_USER) {
                  tpp->users[tpp->inUse].reqTime = TAInow();
                  tpp->users[tpp->inUse].timeout = timeout;
                  tpp->users[tpp->inUse].clntId = id;
         //         tpp->inUse++;
                  k = -2;
                  break;
               }
               /* too many users */
               else {
                  k = -1;
                  break;
               }
            }
            if ((k == -1) && (tpp->id == 0)) {
               /* found an empty slot */
               k = j;
	       projTpNum++;
            }
         }
      }
      /* Check if the total number of testpoint has reached the limit */
      {
	   int tpiface;
	   int projTpNumNewEntry = -1;
           for (tpiface = 0; tpiface < TP_MAX_INTERFACE; tpiface++) {
            /* loop over selected test points */
            int idxlen = TP_INTERFACE_TO_INDEX_LEN (tpiface);
	    int tpidx;
            for (tpidx = 0; tpidx < idxlen; tpidx++) {
      		tpEntry_t*	tpent;		/* test point entry */
               	tpent = &tpNode.indx[tpiface][tpidx];
               	if (tpent->id != 0) projTpNum++;
	    }
	   }
        #ifdef DEBUG
	   printf("Projected tpnum=%d curDaqSize=%d\n", projTpNum, curDaqBlockSize);
	#endif
	   unsigned int projDaqSize = projTpNum * 4 * 16384 * sys_freq_mult + curDaqBlockSize;
	   if (projDaqSize > DAQ_DCU_SIZE) {
		printf ("Too many testpoints: projected DAQ size %d > %d (maximum)\n", projDaqSize, DAQ_DCU_SIZE);
		k = -1;
      		MUTEX_RELEASE (servermux);
		return TRUE;
	   }
      }

      for (i = 0; i < tp.TP_r_len; i++) {
         /* debug */
      #ifdef DEBUG
         printf ("Request tp %i\n", tp.TP_r_val[i]);
      #endif
         /* check if ID in list */
         if ((tp.TP_r_val[i] <= 0) || 
            (tp.TP_r_val[i] >= TP_HIGHEST_INDEX) ||
            (tpNode.tplookup[tp.TP_r_val[i]] == NULL)) {
      #ifdef DEBUG
            printf ("TP %i not in list\n", tp.TP_r_val[i]);
      #endif
            continue;
         }
      
         /* test whether valid test point */
         tpinterface = TP_ID_TO_INTERFACE (tp.TP_r_val[i]);
         /* use LSC EXC for DAC channels */
         if (tpinterface == TP_DAC_INTERFACE) {
            tpinterface = TP_LSC_EX_INTERFACE;
         }
         len = TP_INTERFACE_TO_INDEX_LEN (tpinterface);
      #ifdef DEBUG
	 printf("Max tp index is %d, tp interface is %d\n", len, tpinterface) ;
         printf ("TP %i has interface %d\n", tp.TP_r_val[i], tpinterface);
      #endif
         if ((tpinterface < 0) || (tpinterface >= TP_MAX_INTERFACE)) {
      #ifdef DEBUG
            printf ("TP %i has invalid interface %d\n", tp.TP_r_val[i], tpinterface);
      #endif
            continue;
         }
      
         /* test whether already in list */
         for (j = 0, k = -1; j < len; j++) {
            tpp = &tpNode.indx[tpinterface][j];
            /* check for duplicate */
            if (tpp->id == tp.TP_r_val[i]) {
               /* already in list; check in use */
               if (tpp->inUse < _TP_MAX_USER) {
                  tpp->users[tpp->inUse].reqTime = TAInow();
                  tpp->users[tpp->inUse].timeout = timeout;
                  tpp->users[tpp->inUse].clntId = id;
                  tpp->inUse++;
                  k = -2;
      #ifdef DEBUG
         	  printf ("TP %i already in list; inUse= %d\n", tp.TP_r_val[i], tpp->inUse);
      #endif
                  break;
               }
               /* too many users */
               else {
                  k = -1;
      #ifdef DEBUG
         	  printf ("Too many users\n");
      #endif
                  break;
               }
            }
            if ((k == -1) && (tpp->id == 0)) {
               /* found an empty slot */
               k = j;
            }
         }

         /* allocate a new test point */
	if (k >= 0) {

            /* debug */
         #ifdef DEBUG
            printf ("allocate new TP node/interface/slot %i/%i/%i\n", 
                   node, tpinterface, k);
         #endif
            tpp = &tpNode.indx[tpinterface][k];
            tpp->id = tp.TP_r_val[i];
            strncpy (tpp->name, 
                    tpNode.tplookup[tp.TP_r_val[i]]->chName,
                    _CHNNAME_SIZE);
            tpp->name[_CHNNAME_SIZE-1] = 0;
            tpp->inUse = 0;
            tpp->users[tpp->inUse].reqTime = TAInow();
            tpp->users[tpp->inUse].timeout = timeout;
            tpp->users[tpp->inUse].clntId = id;
            tpp->inUse++;
         }
         else if (k == -1) {
            /* no empty slot or too many users */
            result->status = - (i + 1);
            break;
         }
      }
   
      #ifdef DEBUG
      printf ("result status = %d\n", result->status);
      #endif
   
      /* if failed, cleanup */
      if (result->status < 0) {
         int		ret;
         TP_r		tpcln;
         tpcln.TP_r_len = -result->status;
         tpcln.TP_r_val = malloc (tpcln.TP_r_len * sizeof(testpoint_t));
         if (tpcln.TP_r_val != 0) {
            for (i = 0; i < tpcln.TP_r_len; i++) {
               tpcln.TP_r_val[i] = tp.TP_r_val[i];
            }
            cleartp_1_svc (id, node, tpcln, &ret, rqstp);
         }
      }

      /* release server mutex */
      MUTEX_RELEASE (servermux);

      /* set activation time */
      time = TAInow();
      result->time = time / _ONESEC + 1;
      if ((time % _ONESEC) / _EPOCH >= TESTPOINT_UPDATE_EPOCH) {
         result->time++;
      }
      result->epoch = 0;
   
      #ifdef DEBUG
      printf ("activation time %ld; time now %lld\n", result->time, (long long int) time);
      #endif
      return TRUE;
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Remote Procedure Name: requesttpname_1_svc				*/
/*                                                         		*/
/* Procedure Description: sets test points				*/
/*                                                         		*/
/* Procedure Arguments: test point names, timeout			*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if not (status)		*/
/*                    time and epoch of activation         		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   bool_t requesttpname_1_svc (int id, char* tpNames, quad_t timeout, 
                     resultRequestTP_r* result, struct svc_req* rqstp)
   {
   #ifdef _NO_TESTPOINTS
      result->status = -10;
      return TRUE;
   
   #else
      int		node;		/* node number */
      TP_r		tp;		/* list of test points */
   
      gdsDebug ("request test point by name");
      printf("Request tp by name %s\n", tpNames) ; /* JCB */
   
      if (tpNode.valid) 
      {
#ifdef DEBUG
         printf ("tp name (node %i) = %s\n", tpNode.node, tpNames);
#endif
      
         /* translate names into test points */
         if (tpName2Index (tpNode.node, tpNames, &tp) < 0) {
            result->status = -1;
            return TRUE;
         }
         /* request test points */
         requesttp_1_svc (id, tpNode.node, tp, timeout, result, rqstp);
         free (tp.TP_r_val);
         if (result->status < 0) {
            result->status = -2;
            return TRUE;
         }
      }
   
      return TRUE;
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Remote Procedure Name: cleartp_1_svc					*/
/*                                                         		*/
/* Procedure Description: clears test points				*/
/*                                                         		*/
/* Procedure Arguments: node ID, test point list			*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if not			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   bool_t cleartp_1_svc (int id, int node, TP_r tp, int* result,
                     struct svc_req* rqstp)
   {
   #ifdef _NO_TESTPOINTS
      *result = -10;
      return TRUE;
   
   #else
      int		i,j;		/* index into test point list */
      int		k;		/* index into user list */
      int		tpinterface;	/* tp interface id */
      int		len;		/* length of test point list */
      tpEntry_t*	tpp;		/* test point entry */
      int		clearall;	/* clear all */
   
      gdsDebug ("clear test point");
#ifdef DEBUG
      printf("Clear test points: len = %d\n", tp.TP_r_len) ;
      for (i = 0; i < tp.TP_r_len; i++)
	 printf("clear tp %d\n", tp.TP_r_val[i]) ; /* JCB */
#endif
   
      /* test node ID */
      if ((id < 0) || (node < 0) || (node >= TP_MAX_NODE) ||
         !tpNode.valid || tpNode.node != node) {
         *result = -2;
         return TRUE;
      }
   
      /* get server mutex */
      MUTEX_GET (servermux);
      clearall = 0;
   
      /* go through supplied test point list */
      for (i = 0; i < tp.TP_r_len; i++) {
         /* debug */
      #ifdef DEBUG
         printf ("Clear tp %i\n", tp.TP_r_val[i]);
      #endif
         if (tp.TP_r_val[i] == _TP_CLEAR_ALL) {
            /* clear all test points of this node */
            memset (tpNode.indx, 0, 
                   TP_MAX_INTERFACE * TP_MAX_INDEX * sizeof (tpEntry_t));
            clearall = 1;
            break;
         }
         /* else clear only specific test points */
         tpinterface = TP_ID_TO_INTERFACE (tp.TP_r_val[i]);
         /* use LSC EXC for DAC channels */
         if (tpinterface == TP_DAC_INTERFACE) {
            tpinterface = TP_LSC_EX_INTERFACE;
         }
         len = TP_INTERFACE_TO_INDEX_LEN (tpinterface);
         if ((tpinterface < 0) || (tpinterface >= TP_MAX_INTERFACE)) {
            continue;
         }
      
         /* search through list */
         for (j = 0; j < len; j++) {
            tpp = &tpNode.indx[tpinterface][j];
            if (tpp->id == tp.TP_r_val[i]) {
               /* found test point */
               for (k = 0; k < tpp->inUse; k++) {
                  if ((tpp->users[k].clntId == id) && 
                     (tpp->users[k].timeout <= 0)) {
                     /* found client identifier */
                     break;
                  } 
               }
               /* decrease in use count */
               if (k < tpp->inUse) {
                  tpp->inUse--;
                  if (tpp->inUse <= 0) {
                     tpp->id = 0;
                  }
               }
               /* shift array */
               if (k < tpp->inUse) {
                  memmove (tpp->users + k, tpp->users + k + 1,
                          (tpp->inUse - k) * sizeof (tpUsage_t));
               }
               break;
            }
         }
      }
   
      /* release server mutex */
      MUTEX_RELEASE (servermux);
   
      /* check if preselect */
      if (clearall) {
         setPreselect (node);
      }
   
      *result = 0;
      return TRUE;
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Remote Procedure Name: clearnametp_1_svc				*/
/*                                                         		*/
/* Procedure Description: clears test points by name			*/
/*                                                         		*/
/* Procedure Arguments: test point names				*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if not			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   bool_t cleartpname_1_svc (int id, char* tpNames, int* result, 
                     struct svc_req* rqstp)
   {
   #ifdef _NO_TESTPOINTS
      *result = -10;
      return TRUE;
   
   #else
      int		node;		/* node number */
      TP_r		tp;		/* list of test points */
   
      gdsDebug ("clear test point by name");
      printf("Clear test point by name %s\n", tpNames) ; /* JCB */
   
      /* translate names into test points */
      if (tpName2Index (tpNode.node, tpNames, &tp) >= 0) 
      {
         /* clear test points */
         cleartp_1_svc (id, tpNode.node, tp, result, rqstp);
         free (tp.TP_r_val);
      }
   
      *result = 0;
      return TRUE;
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Remote Procedure Name: querytp_1_svc					*/
/*                                                         		*/
/* Procedure Description: queries test point interface			*/
/*                                                         		*/
/* Procedure Arguments: request ID, max length of tp list		*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if not			*/
/*                    test point list		          		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   bool_t querytp_1_svc (int id, int node, int tpinterface, int tplen, 
                     u_long time, int epoch, resultQueryTP_r* result, 
                     struct svc_req* rqstp)
   {
   #ifdef _NO_TESTPOINTS
      memset (result, 0, sizeof (resultQueryTP_r));
      result->status = -10;
      return TRUE;
   
   #else
      gdsDebug ("query test point");
      memset (result, 0, sizeof (resultQueryTP_r));
      /* debug */
   #ifdef DEBUG
      printf ("Query node %i interface %i\n", node, tpinterface);
   #endif
      printf ("Query node %i interface %i\n", node, tpinterface); /* JCB */
   
      /* test node ID */
      if ((id < 0) || (node < 0) || (node >= TP_MAX_NODE) ||
         !tpNode.valid || tpNode.node != node) {
         result->status = -2;
         return TRUE;
      }
   
      /* allocate memory for result array */
      result->tp.TP_r_val = malloc (tplen * sizeof (testpoint_t));
      if (result->tp.TP_r_val == NULL) {
         result->status = -2;
         return FALSE;
      }
      result->tp.TP_r_len = tplen;
   
      /* fix time if needed */
      if (time == 0) {
         tainsec_t t = TAInow ();
         time = t / _ONESEC;
         epoch = (t % _ONESEC) / _EPOCH;
      }
   
      /* get index directly from test point client interface */
      result->status = 
         tpGetIndexDirect (node, tpinterface, result->tp.TP_r_val, 
                          result->tp.TP_r_len, time, epoch);
      if (result->status >= 0) {
         result->tp.TP_r_len = result->status;
      }
      else {
         free (result->tp.TP_r_val);
         result->tp.TP_r_val = NULL;
         result->tp.TP_r_len = 0;
      };
   
   #ifdef DEBUG
      {
         char		buf[256];
         tainsec_t	now;
         now = TAInow();
         sprintf (buf, "query test point status (node %i) %i\n", 
                 node, result->status);
         sprintf (strend (buf), "req. time = %li : %i\n", time, epoch);
         sprintf (strend (buf), "cur. time = %i : %i", (int) (now / _ONESEC), 
                 (int) ((now % _ONESEC + 1000000) / _EPOCH));
         gdsDebug (buf);
         printf ("%s\n", buf);
         if (result->status >= 4) {
            printf ("TPs: %i %i %i %i\n", result->tp.TP_r_val[0],
                   result->tp.TP_r_val[1],  result->tp.TP_r_val[2],  
                   result->tp.TP_r_val[3]);
         }
      }
   #endif
   
      return TRUE;
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Remote Procedure Name: keepalive_1_svc				*/
/*                                                         		*/
/* Procedure Description: keep alive function				*/
/* This function is used as follows: Clients which do not support a	*/
/* keep alive call it once during initializationwith the value -2;	*/
/* the function returns a handle greater than the max. index of the	*/
/* client list. Clients which support a keep alive call this function	*/
/* once during initialization with the value -1; and then every 15s	*/
/* with the returned handle from the first call. In this case the 	*/
/* the returned handle is an index into the client list. If the function*/
/* is not called fro more than 60s, all test points associated with this*/
/* handle are deleted. A negative return value	indicates an error	*/
/* and the client should shut down. Client list entries which have	*/
/* expired are kept around for another 240s before they are deleted.	*/
/* 									*/
/* Procedure Arguments: request ID, return status, service		*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if not			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   bool_t keepalive_1_svc (int id, int* ret, struct svc_req* rqstp)
   {
      int		i;		/* index into client list */
      struct in_addr	addr;		/* client address */
   
   #ifdef DEBUG
      char		buf[256];
      rpcGetClientaddress (rqstp->rq_xprt, &addr);
      sprintf (buf, "keep alive %i from %s", id, inet_ntoa (addr));
      gdsDebug (buf);
      printf ("%s\n", buf);
   #endif
   
      MUTEX_GET (servermux);
      if (id < -1) {
         *ret = tpclntID++;
      }
      else if (id < 0) {
         rpcGetClientaddress (rqstp->rq_xprt, &addr);
         for (i = 0; i < _TP_MAX_USER; i++) {
            /* look for free id */
            if (!tpclnts[i].valid) {
               tpclnts[i].valid = 1;
               tpclnts[i].lastTime = TAInow();
               *ret = i;
               MUTEX_RELEASE (servermux);
               {
		 time_t now = time (NULL);
#ifdef DEBUG
	         printf ("Allocate new TP handle %i by %s at %s\n", 
	                 i, inet_ntoa (addr), ctime (&now));
#endif
		 fflush(stdout);
	       }
               return TRUE;
            }
         }
	 printf ("Failed to allocate TP handle by %s\n", 
	         inet_ntoa (addr));
         *ret = -1;
      }
      else if ((id >= _TP_MAX_USER) || (!tpclnts[id].valid)) {
         *ret = -1;
      }
      else {
         tpclnts[id].lastTime = TAInow();
         *ret = id;
      }
      MUTEX_RELEASE (servermux);
      return TRUE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: rtestpoint_1_freeresult			*/
/*                                                         		*/
/* Procedure Description: frees memory of rpc call			*/
/*                                                         		*/
/* Procedure Arguments: rpc transport info, xdr routine for result,	*/
/*			pointer to result				*/
/*                                                         		*/
/* Procedure Returns: TRUE if successful, FALSE if failed		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int rtestpoint_1_freeresult (SVCXPRT* transp, 
                     xdrproc_t xdr_result, caddr_t result)
   {
      (void) xdr_free (xdr_result, result);
      return TRUE;
   }


#ifndef _NO_TESTPOINTS
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: setPreselect				*/
/*                                                         		*/
/* Procedure Description: set preselected TPs				*/
/*                                                         		*/
/* Procedure Arguments: node						*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   void setPreselect (int node)
   {
      resultRequestTP_r		result; /* result from request */
      TP_r			tp;	/* TP list */
      int			k;      /* index */
   
      /* initialize TP list */
      tp.TP_r_len = 0;
      tp.TP_r_val = malloc (2 * TP_MAX_PRESELECT * sizeof (short));
      /* select TPs */
      for (k = 0; k < TP_MAX_PRESELECT; ++k) {
         if (tpNode.preselect[k] > 0) {
            tp.TP_r_val[tp.TP_r_len++] = tpNode.preselect[k];
         #ifdef DEBUG
         printf("Preselecting %d on node %d\n", tpNode.preselect[k], node);
         #endif
         }
      }
      if (tp.TP_r_len > 0) {
         requesttp_1_svc (0x7fffffff, node, tp, 0, &result, 0);
      }
      free (tp.TP_r_val);
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: tpName2Index				*/
/*                                                         		*/
/* Procedure Description: translates test point names into id numbers	*/
/*                                                         		*/
/* Procedure Arguments: node, test point names, test point list		*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if not (status)		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int tpName2Index (int node, const char* tpNames, TP_r* tp)
   {
      char*		buf;		/* name buffer */
      char*		p;		/* cursor into buffer */
      gdsChnInfo_t	chn;		/* channel info structure */
      int		tp_node;	/* node of test point name */
      char *		saveptr;
   
      /* allocate memory */
      buf = malloc (strlen (tpNames) + 2);
      tp->TP_r_val = malloc (_MAX_TPNAMES * sizeof (testpoint_t));
      if ((tp->TP_r_val == NULL) || (buf == NULL)) {
         free (buf);
         free (tp->TP_r_val);
         return -1;
      }
      tp->TP_r_len = 0;
      strcpy (buf, tpNames);
   
      /* go through name list */
      p = strtok_r (buf, " ,\t\n", &saveptr);
      while ((p != NULL) && (tp->TP_r_len < _MAX_TPNAMES)) {
	 printf("tpName2Index checking %s\n", p);
         if ((gdsChannelInfo (p, &chn) == 0) &&
            (tpIsValid (&chn, &tp_node, tp->TP_r_val + tp->TP_r_len)) &&
            (node == tp_node)) {
            printf ("%s is tp %i (node %i)\n", p, 
                   tp->TP_r_val[tp->TP_r_len], 
                   tp_node);
            tp->TP_r_len++;
         }
	 printf("node=%d; tp_node=%d\n", node, tp_node);
         p = strtok_r (NULL, " \t\n", &saveptr);
      } 
   
      free (buf);
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: cleanupTestpoints				*/
/*                                                         		*/
/* Procedure Description: garbage collector for test points		*/
/*                                                         		*/
/* Procedure Arguments: scheduler task, time, epoch, void*		*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if not			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int cleanupTestpoints (schedulertask_t* info, taisec_t time, 
                     int epoch, void* arg)
   {
      int		i;		/* index into test point list */
      int		j;		/* interface ID */
      int		k;		/* index into usage array */
      int		len;		/* length of test point list */
      tpEntry_t*	tpp;		/* test point entry */
      tainsec_t		t;		/* current time */
      int		remove;		/* true if tp is obsolete */
   
      t = (tainsec_t) time * _ONESEC + (tainsec_t) epoch * _EPOCH;
   
      if (tpNode.valid) 
      {
         /* loop over interfaces */
         for (j = 0; j < TP_MAX_INTERFACE; j++) {
            /* get server mutex */
            MUTEX_GET (servermux);
         
            /* loop over selected test points */
            len = TP_INTERFACE_TO_INDEX_LEN (j);
            for (i = 0; i < len; i++) {
               tpp = &tpNode.indx[j][i];
               if (tpp->id == 0) {
                  continue;
               }
               /* loop over clients */
               for (k = 0; k < tpp->inUse; k++) {
                  remove = 0;
                  /* check time out */
                  if ((tpp->users[k].timeout > 0) &&
                     (tpp->users[k].reqTime + tpp->users[k].timeout < t)) {
                     /* found old testpoint */
                     /* printf ("remove tp %i (node %i) because of timeout\n",
                            tpp->id, tpNode.node);*/
                     remove = 1;
                  }
#if 0
	/* Disable keepalive feature at LASTI (for now) */
                  /* check keep alive */
                  if ((tpp->users[k].clntId >= 0) && 
                     (tpp->users[k].clntId < _TP_MAX_USER) &&
                     ((!tpclnts[tpp->users[k].clntId].valid) ||
                     (tpclnts[tpp->users[k].clntId].lastTime + 
                     _KEEPALIVE_TIMEOUT * _ONESEC < t))) {
                     /* printf ("remove tp %i (node %i) because of keep alive\n",
                            tpp->id, tpNode.node);
                     printf ("client id = %i\n", tpp->users[k].clntId);
                     printf ("client is %svalid\n", 
                            (tpclnts[tpp->users[k].clntId].valid) ? "" : "not ");
                     printf ("last time = %f\n", (double)
                            tpclnts[tpp->users[k].clntId].lastTime / _ONESEC);
                     printf ("cur. time = %f\n", (double) t / _ONESEC);*/
                     remove = 1;
                  }
#endif
               
                  /* remove entry if obsolete */
                  if (remove) {
                     /* decrease in use count */
                     tpp->inUse--;
                     if (tpp->inUse <= 0) {
                        tpp->id = 0;
                     }
                     /* shift array */
                     if (k < tpp->inUse) {
                        memmove (tpp->users + k, tpp->users + k + 1,
                                (tpp->inUse - k) * sizeof (tpUsage_t));
                     }
                  }
               }
            }
         
            /* release server mutex */
            MUTEX_RELEASE (servermux);
         }
      }
   
      /* loop over client interface */
      for (k = 0; k < _TP_MAX_USER; k++) {
         /* if expired keep it around for a while than delete it */
         if ((tpclnts[k].valid) && 
            (tpclnts[k].lastTime + _KEEPALIVE_TIMEOUT * _ONESEC < t)) {
            if (++tpclnts[k].valid > _KEEP_AROUND) {
               tpclnts[k].valid = 0;
            }
         }
      }
   
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: updateTestpoints				*/
/*                                                         		*/
/* Procedure Description: updates test points in refl. memory		*/
/*                                                         		*/
/* Procedure Arguments: scheduler task, time, epoch, void*		*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if not			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int updateTestpoints (schedulertask_t* info, taisec_t time, 
                     int epoch, void* arg)
   {
      int		i;		/* index into test point list */
      int		j;		/* interface ID */
      int		size;		/* size of test point array */
      int		len;		/* length of test point list */
      tpEntry_t*	tpp;		/* test point entry */
      testpoint_t	tp[TP_MAX_INDEX];/* test point index */
      char		tpchn[TP_MAX_INDEX][_CHNNAME_SIZE];
   					/* test point channel names */
      tainsec_t		t;		/* current time */
      int		addr;		/* RM address of tp index */
   
   #if 0
      printf ("update %li/%i\n", time, epoch);
   #endif
      t = (tainsec_t) time * _ONESEC + (tainsec_t) epoch * _EPOCH;
   
      if (tpNode.valid) 
      {
         /* loop over interfaces */
         for (j = 0; j < TP_MAX_INTERFACE; j++) {
            /* get server mutex */
            MUTEX_GET (servermux);
         
            /* make index */
            len = TP_INTERFACE_TO_INDEX_LEN (j);
            memset (tp, 0, len * sizeof (testpoint_t));
            for (i = 0, size = 0; i < len; i++) {
               tpp = &tpNode.indx[j][i];
               /* copy index entry */
               if (tpp->id != 0) {
                  tp[size] = tpp->id;
                  strncpy (tpchn[size], tpp->name, _CHNNAME_SIZE);
                  tpchn[size][_CHNNAME_SIZE-1] = 0;
               }
               else {
                  tp[size] = 0;
                  strcpy (tpchn[size], "");
               }
#if 0
	       // print first four test point numbers and channel names
	       if (size < 4) printf (" %d tp = %s; tpnum = %d\n", size, tpchn[size], tp[size]);
#endif
               size++;
            }
         
            /* release server mutex */
            MUTEX_RELEASE (servermux);
#if 0
            if ((j == 0)) {
               printf ("node = %i, interface = %i  - ", tpNode.node, j);
               printf ("tp = %i %i %i %i %i\n", tp[0],tp[1],tp[2],tp[3],tp[4]);
            }
#endif
         
#if 0
	// TODO: this seems to clobber my test point numbers
            /* disable excitation test points if awg not running */
         #ifndef _NO_AWG_CHECK
            if (!tpwriter[tpNode.node][j].alive) {
            #ifdef DEBUG
               printf ("AWG not alive: clear all TPs\n");
            #endif
               memset (tp, 0, len * sizeof (testpoint_t));
            }
         #endif
#endif
         
#ifndef __linux__
#error
            /* swap on little-endian machines */
	    i = 0;
            *(char*)&i = 1;
            if (i == 1) {
               if (len % 2 == 1) { /* add a point if odd length */
                  tp[len++] = 0;
               }
               for (i = 0; i < len; i += 2) {
                  testpoint_t temp = tp[i];
                  tp[i] = tp[i+1]; tp[i+1] = temp;
                  /* tp[i] = ((tp[i]&0xFF)<<8) | ((tp[i]&0xFF00)>>8); */
               }
            }
#endif
         
            /* write index to reflective memory */
            addr = TP_NODE_INTERFACE_TO_INDEX_OFFSET (tpNode.node, j);
            if ((len <= 0) || (addr <= 0)) {
               continue;
            }
            /* debug */
         #if 0
            printf ("write tps %i %i %i from node %i to 0x%x\n",
                    tp[0], tp[1], tp[2], 
                    TP_NODE_ID_TO_RFM_ID (tpNode.node), addr);
         #endif
         #if RMEM_LAYOUT > 0
            if (rmWrite (TP_NODE_ID_TO_RFM_ID (tpNode.node), (char*) tp, addr, 
               len * sizeof (testpoint_t), 0) != 0) {
               gdsError (GDS_ERR_PROG, "RM1 write failure");
            }
         #if 0
	    printf("rmWrite(0, %x %x %d )\n", tp, addr, len * sizeof (testpoint_t)); 
	 #endif
#if 0
         #if TARGET !=  (TARGET_L1_GDS_AWG1 + 20) && TARGET !=  (TARGET_L1_GDS_AWG1 + 21)
            if (rmWrite (1 + TP_NODE_ID_TO_RFM_ID (tpNode.node), (char*) tp, addr, 
               len * sizeof (testpoint_t), 0) != 0) {
               gdsError (GDS_ERR_PROG, "RM2 write failure");
            }
         #endif
#endif
         #else

            if (rmWrite (TP_NODE_ID_TO_RFM_ID (tpNode.node), (char*) tp, addr, 
               len * sizeof (testpoint_t), 0) != 0) {
               gdsError (GDS_ERR_PROG, "RM write failure");
            }
         #endif
         
           /* write channel information into reflective memory 
            for (i = 0; i < size; i++) {
               strncpy ((tpNode.chninfo[j] + i)->chName, 
                       tpchn[i], MAX_CHNNAME_SIZE - 1);
            }
            tpNode.ipc[j]->channelCount = size;*/
         }
      }
   
      return 0;
   }


#ifndef _NO_AWG_CHECK
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: awgAlive					*/
/*                                                         		*/
/* Procedure Description: tests if awg is alive				*/
/*                                                         		*/
/* Procedure Arguments: scheduler task, time, epoch, void*		*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if not			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int awgAlive (schedulertask_t* info, taisec_t time, 
                     int epoch, void* arg)
   {
      int		j;		/* tp interface index */
      int		cycleCount;	/* current cycle count */
   

      for (j = 0; j < TP_MAX_INTERFACE; j++) {
	 if (tpwriter[j].IPC == NULL) {
	    tpwriter[j].alive = 0;
	    continue;
	 }
	 cycleCount = tpwriter[j].IPC->cycle;
	 tpwriter[j].alive = 
	    (cycleCount != tpwriter[j].oldCycleCount);
	 tpwriter[j].oldCycleCount = cycleCount;
      }
      return 0;
   }
#endif
#endif


#ifndef _NO_TESTPOINTS
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: initializeTestpoints			*/
/*                                                         		*/
/* Procedure Description: initializes server stuff			*/
/*                                                         		*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int initializeTestpoints (void) 
   {
      schedulertask_t	task;	/* update task entry */
   #ifndef _NO_AWG_CHECK
      int		node;	/* node index */
      int		j;	/* tp interface index */
   #endif
   
      /* quit if already initiaized */
      if (initServer == 2) {
         return 0;
      }
   
      /* test if low level init */
      if (initServer == 0) {
         initTestpointServer();
         if (initServer == 0) {
            return -1;
         }
      }
   
      /* fill in pointers of ddcu's which write to tp area */
   #ifndef _NO_AWG_CHECK
      memset (tpwriter, 0, sizeof (tpwriter));
      for (j = 0; j < TP_MAX_INTERFACE; j++) {
	 /* determine unit id of awg dcu */
	 int 	unitID;
	 int 	base;
	 int 	size;
	 int		rfmid;
	 /* determine base address and size of awg dcu */
	 /* We don't know the node number, but it doesn't 
	  * matter for advanced LIGO.  Pick a value no 0 or 1.
	  */
	 unitID = TP_NODE_INTERFACE_TO_UNIT_ID (20, j);
	 base = UNIT_ID_TO_RFM_OFFSET (unitID);
	 size = UNIT_ID_TO_RFM_SIZE (unitID);

	 rfmid = 0;

printf("interface %d: unitID = %d, base = %d, size = %d\n", j, unitID, base, size) ;

	 /* set IPC pointer if valid dcu */
	 if ((base >= 0) && (rmCheck (rfmid, base, size))) {
	    tpwriter[j].IPC = 
	       (rmIpcStr*) (rmBaseAddress (rfmid) + base);
	 }
	 else {
	    tpwriter[j].IPC = NULL;
	 }
	 /* reset alive flag */
	 tpwriter[j].alive = 0;
      }
   #endif
   
      /* make sure heartbeat is installed */
      if (installHeartbeat (NULL) < 0) {
         return -2;
      }
   
      /* init scheduler */
      sd = createScheduler (0, NULL, NULL);
      if (sd == NULL) {
         return -3;
      }
   
      /* setup task info structure of update task */
      SET_TASKINFO_ZERO (&task);
      task.flag = SCHED_REPEAT | SCHED_WAIT;
      task.waittype = SCHED_WAIT_IMMEDIATE;
      task.repeattype = SCHED_REPEAT_INFINITY;
      task.repeatratetype = SCHED_REPEAT_EPOCH;
      task.repeatrate = NUMBER_OF_EPOCHS;
      task.synctype = SCHED_SYNC_EPOCH;
      task.syncval = TESTPOINT_UPDATE_EPOCH;
      task.func = updateTestpoints;
   
      /* schedule update task */
      if (scheduleTask (sd, &task) < 0) {
         closeScheduler (sd, 3 * _EPOCH);
         sd = NULL;
         return -4;
      }
   
      /* setup task info structure of awg alive task */
   #ifndef _NO_AWG_CHECK
      SET_TASKINFO_ZERO (&task);
      task.flag = SCHED_REPEAT | SCHED_WAIT;
      task.waittype = SCHED_WAIT_IMMEDIATE;
      task.repeattype = SCHED_REPEAT_INFINITY;
      task.repeatratetype = SCHED_REPEAT_EPOCH;
      task.repeatrate = 2;
      task.synctype = SCHED_SYNC_EPOCH;
      task.syncval = 1;
      task.func = awgAlive;
   
      /* schedule awg alive task */
      if (scheduleTask (sd, &task) < 0) {
         closeScheduler (sd, 3 * _EPOCH);
         sd = NULL;
         return -4;
      }
   #endif
   
      /* setup task info structure of cleanup task */
      SET_TASKINFO_ZERO (&task);
      task.flag = SCHED_REPEAT | SCHED_WAIT;
      task.waittype = SCHED_WAIT_IMMEDIATE;
      task.repeattype = SCHED_REPEAT_INFINITY;
      task.repeatratetype = SCHED_REPEAT_EPOCH;
      task.repeatrate = NUMBER_OF_EPOCHS;
      task.synctype = SCHED_SYNC_EPOCH;
      task.syncval = 1;
      task.func = cleanupTestpoints;
   
      /* schedule cleanup task  */
      if (scheduleTask (sd, &task) < 0) {
         closeScheduler (sd, 3 * _EPOCH);
         sd = NULL;
         return -4;
      }
   
      /* intialize test point client interface */
      if (testpoint_client() < 0) {
         closeScheduler (sd, 3 * _EPOCH);
         sd = NULL;
         return -5;
      }
   
      initServer = 2;
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: isTestpoint					*/
/*                                                         		*/
/* Procedure Description: returns true if channel is a test point	*/
/*                                                         		*/
/* Procedure Arguments: channel info					*/
/*                                                         		*/
/* Procedure Returns: true if yes, false if no				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int 	_query_node = 0;	/* rm ID of query */
   static int isTestpoint (const gdsChnInfo_t* info) 
   {
      return IS_TP (info) && (info->rmId == _query_node);
   }
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: testpoint_server				*/
/*                                                         		*/
/* Procedure Description: start rpc service task for test points	*/
/*                                                         		*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: does not return if successful, <0 otherwise	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int testpoint_server (void)
   {
   #ifdef _NO_TESTPOINTS
      printf ("test point interface disabled\n");
      gdsWarningMessage ("test point interface disabled");
      return -1;
   #else
   
      int		j;		/* tp interface index */
      int		rpcpmstart;	/* port monitor flag */
      SVCXPRT*		transp;		/* service transport */
      int		proto;		/* protocol */
      char 		remotehost[PARAM_ENTRY_LEN];
   				        /* remote hostname */
      char		section[30];	/* section name */
      char		param[30];	/* paramter name */
      struct in_addr 	addr;		/* host address */
      struct in_addr 	laddr;		/* local address */
      unsigned long	prognum;	/* rpc prog. num. */
      unsigned long	progver;	/* rpc prog. ver. */
      unsigned long	presel;		/* preselected tp */
      int		node;		/* node id */
      int		n;		/* node id */
      int		k;		/* index */
      int		rmNode;		/* RM node ID */
      int		duplicate;	/* duplicate prog num/ver */
      int		reg;		/* # of registered services */
      int		tp; 		/* tp index */
      static confServices conf[TP_MAX_NODE];
   					/* configuration service */
      static int	confnum;	/* # of conf entries */
      static char	confbuf[1024];	/* configuration buffer */
      char*		pConf;		/* conf pointer */
      struct in_addr	host;		/* local host address */
      char 		sysname[PARAM_ENTRY_LEN];
      extern char	system_name[PARAM_ENTRY_LEN];
   

   #ifdef OS_VXWORKS
      rpcTaskInit();
   #endif

      /* initialize test points */
      if (initializeTestpoints() < 0) {
	 printf("unable to initialize test points\n");
         gdsError (GDS_ERR_PROG, "unable to initialize test points");
         return -1;
      }
   
      /* Mark the node as invalid until it's found in the parameter file. */
      tpNode.valid = 0 ;

      for (node = 0; node < TP_MAX_NODE; node++) {
	 /*printf("tpman checking node %d\n", node);*/
	 rmNode = 0;
         if (rmBaseAddress (rmNode) == NULL) {
	    printf("reflective memory pointer not set\n"); 
            continue;
         }
      
         /* make section header */
         sprintf (section, "%s-node%i", site_prefix, node);
      
         /* get remote host from parameter file */
         strcpy (remotehost, "");
         loadStringParam (PRM_FILE, section, PRM_ENTRY1, 
                         remotehost);
	 // Skip unconfigured section
	 if (strcmp (remotehost, "") == 0) continue;
	 printf ("remotehost = %s; section= %s\n", remotehost, section);
         if (rpcGetHostaddress (remotehost, &addr) != 0) {
	    printf("Cannot resolve hostname \"%s\"\n", remotehost);
            continue;
         }
	
	 loadStringParam (PRM_FILE, section, PRM_ENTRY4, sysname);
	 printf ("sysname = %s\n", sysname);
	 if (sysname[0] == 0) continue; /* Mandatory field */
         /* Determine if this is my node */
         if ((rpcGetLocaladdress (&laddr) < 0) ||
            (addr.s_addr != laddr.s_addr) ||
	    strcmp(sysname, system_name)) {
	    printf ("this is not my node %x %x %s\n", addr.s_addr, laddr.s_addr, system_name);
            continue;
         }
	 printf ("this is my node\n");
         inet_ntoa_b (addr, tpNode.hostname);
      
         prognum = RPC_PROGNUM_TESTPOINT;
         progver = RPC_PROGVER_TESTPOINT;

#if 0
	 // Do not load from the config file
         /* get rpc parameters from parameter file */
         loadNumParam (PRM_FILE, section, PRM_ENTRY2, &prognum);
         loadNumParam (PRM_FILE, section, PRM_ENTRY3, &progver);
#endif

	 // Use GDS node to generate a unique RPC number
	 prognum += node;

	 tpNode.prognum = prognum;
	 tpNode.progver = progver;
	 memset (tpNode.indx, 0, TP_MAX_INTERFACE * TP_MAX_INDEX * sizeof (tpEntry_t));
	 /* Mark the node as valid and record the node number. */
	 tpNode.valid = 1 ;
	 tpNode.node = node ;
      
         /* get preselected testpoints */
         for (k = 0; k < TP_MAX_PRESELECT; ++k) {
            sprintf (param, PRM_PRESELECT, k);
            presel = 0;
            loadNumParam (PRM_FILE, section, param, &presel);
            if ((presel > 0) && (presel < TP_HIGHEST_INDEX)) {
               tpNode.preselect[k] = presel;
               printf("Preselecting testpoint %ld on node %d\n", presel, node);
            }
            else {
               tpNode.preselect[k] = 0;
            }
         }
	 /* Since the node has been found, stop looking. */
	 break ;
      }
   
      /* load channel information */
      if (!tpNode.valid) {
	 printf("Failed to find my node in %s\n", PRM_FILE);
	 exit(1);
      } else {
         /* query length of channel info list of the specified node */
         MUTEX_GET (servermux);
         _query_node = node;
         tpNode.tplistlen = gdsChannelListLen (-1, isTestpoint);
	 printf("Channel list length for node %d is %d\n", node, tpNode.tplistlen);
         if (tpNode.tplistlen <= 0) {
            tpNode.tplistlen = 0;
            MUTEX_RELEASE (servermux);
         }
	 else
	 {
	    /* allocate memory for list */
	    tpNode.tplist = malloc (tpNode.tplistlen * sizeof (gdsChnInfo_t));
	    if (tpNode.tplist == NULL) {
	       tpNode.tplistlen = 0;
	       MUTEX_RELEASE (servermux);
	       return -2;
	    }
	    /* load list */
	    tpNode.tplistlen = gdsChannelList (-1, isTestpoint, tpNode.tplist, tpNode.tplistlen);
	    MUTEX_RELEASE (servermux);
	    if (tpNode.tplistlen <= 0) {
	       free (tpNode.tplist);
	       tpNode.tplistlen = 0;
	       tpNode.tplist = NULL;
	    }
	    else
	    {
	       /* build lookup table */
	       memset (tpNode.tplookup, 0, sizeof (tpNode.tplookup));
	       for (j = 0; j < tpNode.tplistlen; j++) {
		  tp = tpNode.tplist[j].chNum;
		  if ((tp > 0) && (tp < TP_HIGHEST_INDEX)) {
		     tpNode.tplookup[tp] = tpNode.tplist + j;
		     /* printf ("node %i tp %i = %s\n", node, tp,
			    tpNode.tplookup[tp]->chName);*/
		  }
	       }
	    }
	 }
      }
   
      /* clear memory */
      /* loop through interfaces */
      for (j = 0; j < TP_MAX_INTERFACE; j++) {

	 char* a = rmBaseAddress (TP_NODE_ID_TO_RFM_ID(tpNode.node)) + 
		   TP_NODE_INTERFACE_TO_DATA_OFFSET(tpNode.node, j);
#ifdef OS_SOLARIS
	 /* Would get a system crash on Solaris with memset() */
	 {
	     int k;
	     int l = DATA_BLOCKS * TP_NODE_INTERFACE_TO_DATA_BLOCKSIZE(tpNode.node, j);
	     for (k = 0; k < l; k++) a[k] = 0;
	 }
#else
	 memset (a, 0, DATA_BLOCKS *
		TP_NODE_INTERFACE_TO_DATA_BLOCKSIZE(tpNode.node, j));
#endif
      }
   
      if (tpNode.valid) {
	 setPreselect (tpNode.node);
      }
   
      /* get local address */
      confnum = 0;
      if (rpcGetLocaladdress (&host) < 0) {
         gdsError (GDS_ERR_PROG, "unable to obtain local address");
         return -3;
      }
   
      /* init rpc services */
      if (rpcInitializeServer (&rpcpmstart, _SVC_FG, _SVC_MODE,
         &transp, &proto) < 0) {
         gdsError (GDS_ERR_PROG, "unable to start rpc service");
         return -4;
      }
   

      /* register with rpc service */
      if (rpcRegisterService (rpcpmstart, transp, proto, 
	 tpNode.prognum, tpNode.progver, rtestpoint_1) != 0) 
      {
	 gdsError (GDS_ERR_PROG, 
		  "unable to register test point service");
	 return -6;
      }
      printf ("Test point manager (%lx / %li): node %i\n", 
	     tpNode.prognum, tpNode.progver, node) ;

      /* Set the global variable value for the testpoint_manager_node */
      testpoint_manager_node = node;
      testpoint_manager_rpc = tpNode.prognum;

      /* add configuration info */
      if (confnum == 0) {
	 pConf = confbuf;
	 conf[0].id = 0;
	 conf[0].answer = stdAnswer;
	 conf[0].user = confbuf;
	 confnum++;
      }
      else {
	 pConf = strend (confbuf);
	 strcpy (pConf, "\n");
	 pConf++;
      }
      sprintf (pConf, "tp %i 0 %s %ld %ld", node,
	      inet_ntoa (host), tpNode.prognum, tpNode.progver);
   
      /* announce service */
      if (conf_server (conf, confnum, 1) < 0) {
         gdsWarningMessage ("unable to start configuration services");
      }
   
      /* start server */
      rpcStartServer (rpcpmstart, &shutdownflag);
   
      /* never reached */
      return -8;
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: initTestpointServer				*/
/*                                                         		*/
/* Procedure Description: initializes test point server			*/
/*                                                         		*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns:void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void initTestpointServer (void) 
   {
   #ifndef _NO_TESTPOINTS
      if (initServer != 0) {
         return;
      }
   
      /* First call, log version ID */
      printf("testpoint_server %s\n", versionId) ;

      /* create server mutex */
      if (MUTEX_CREATE (servermux) != 0) {
         return;
      }
      /* reset test point data memory */
      memset (&tpNode, 0, sizeof (tpNode));
      memset (tpclnts, 0, sizeof (tpclnts));
   
      /* set initServer and return */
      shutdownflag = 1;
      initServer = 1;
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: finiTestpointServer				*/
/*                                                         		*/
/* Procedure Description: cleans up test point server			*/
/*                                                         		*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns:void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void finiTestpointServer (void) 
   {
      int		node;		/* node ID */
   
   #ifndef _NO_TESTPOINTS
      if (initServer == 0) {
         return;
      }
   
      /* destroy server mutex */
      if (MUTEX_DESTROY (servermux) != 0) {
         return;
      }
   
      /* close scheduler */
      if (sd != NULL) {
         closeScheduler (sd, 0); /* timeout must be zero in exit routine */
         sd = NULL;
      }
   
      /* free memory */
      if (tpNode.valid && (tpNode.tplist != NULL)) {
	 free (tpNode.tplist);
      }
   
      /* set initServer and return */
      shutdownflag = 0;
      initServer = 0;
   #endif
   }

