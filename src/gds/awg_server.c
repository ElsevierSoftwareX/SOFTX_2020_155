static char *versionId = "Version $Id$" ;
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: awg_server						*/
/*                                                         		*/
/* Module Description: implements server functions for handling the 	*/
/* test point interface							*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

#ifndef DEBUG
#define DEBUG
#define RPC_SVC_FG
#endif

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Defines: Describes the output capabilities of the AWG		*/
/*          _AWG_DAC		awg outputs waveforms to a digital-to-	*/
/*				analog converter 			*/
/*          _AWG_DS340		awg outputs waveforms to a stand-alone	*/
/*				DS340 Stanford signal generator		*/
/*          _AWG_RM		awg outputs waveforms to memory buffer	*/
/*				in a reflective memory module		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#if !defined(_AWG_DAC) && !defined(_AWG_DS340) && !defined(_AWG_RM)
#define _AWG_DAC
#define _AWG_DS340
#define _AWG_RM
#endif

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Includes								*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#ifndef __EXTENSIONS__
#define __EXTENSIONS__
#endif
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#endif

#include "dtt/gdsutil.h"
#ifdef _AWG_RM
#include "dtt/rmorg.h"
#include "dtt/gdssched.h"
#include "dtt/rmapi.h"
#endif	
#include "dtt/awg.h"
#include "dtt/rawgapi.h"
#include "dtt/confserver.h"
#include "dtt/awg_server.h"
#include "dtt/hardware.h"


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Constants: _NETID		  net protocol used for rpc		*/
/*            _SHUTDOWN_DELAY	  wait period before shut down		*/
/*            _SHOWBUF_SIZE	  maximum size of show reply		*/
/*            SIG_PF		  signal function prototype 		*/
/*            _IDLE		  server idle state			*/
/*            _BUSY		  server busy state			*/
/*            PRM_SECTION	  parameter section name		*/
/*            PRM_ENTRY1	  entry name for host 			*/
/*            PRM_ENTRY2	  entry name for rpc prog num		*/
/*            PRM_ENTRY3	  entry name for rpc ver num		*/
/* 	      DATA_EPOCH_DELAY	  delay in epochs when data is ready	*/
/*            _LSCX_UNIT_ID	  unit id of LSC excitation		*/
/*            _ASCX_UNIT_ID	  unit id of ASC excitation		*/
/*            _TP_NODE		  node id                		*/
/*           _LSCX_BASE		  base address of LSC excitation DCU	*/
/*           _ASCX_BASE		  base address of ASC excitation DCU	*/
/*           _LSCX_SIZE		  size of LSC excitation DCU		*/
/*           _ASCX_SIZE		  size of ASC excitation DCU		*/
/*	     DCU_WAIT_FOR_REFRESH_RATE taskDelay parameter		*/
/*	     DCU_WAIT_FOR_REFRESH_TIMEOUT timeout for dcu command: 2s	*/
/*           DDCU_CMD_TIMEOUT	  timeout for command acknowlegment: 2s	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#define _NETID			"tcp"
#define _SHUTDOWN_DELAY		120		/* 120 s */
#define _SHOWBUF_SIZE		(128 * 1024)
#ifndef SIG_PF
#define	SIG_PF 			void (*) (int)
#endif
#define	_IDLE 			0
#define	_BUSY	 		1
#define PRM_FILE		gdsPathFile ("/param", "awg.par")
#define PRM_SECTION		gdsSectionSiteIfo ("awg%i")
#define PRM_ENTRY2		"prognum"
#define PRM_ENTRY3		"progver"

#ifdef _AWG_RM
#define DATA_EPOCH_DELAY	1
#ifdef GDS_UNIX_TARGET
#define _LSCX_UNIT_ID		GDS_4k_LSC_EX_ID
#define _ASCX_UNIT_ID		GDS_4k_ASC_EX_ID
extern int testpoint_manager_node;
extern int testpoint_manager_rpc;
#define _TP_NODE		testpoint_manager_node
#else
#if IFO != GDS_IFO2
#define _LSCX_UNIT_ID		GDS_4k_LSC_EX_ID
#define _ASCX_UNIT_ID		GDS_4k_ASC_EX_ID
#define _TP_NODE		TP_NODE_0
#else
#define _LSCX_UNIT_ID		GDS_2k_LSC_EX_ID
#define _ASCX_UNIT_ID		GDS_2k_ASC_EX_ID
#define _TP_NODE		TP_NODE_1
#endif
#endif
#define _RM_ID			TP_NODE_ID_TO_RFM_ID(_TP_NODE)

   static const int 		_LSCX_BASE = 
   UNIT_ID_TO_RFM_OFFSET (_LSCX_UNIT_ID);
   static const int 		_ASCX_BASE = 
   UNIT_ID_TO_RFM_OFFSET (_ASCX_UNIT_ID);
   static const int 		_LSCX_SIZE = 
   UNIT_ID_TO_RFM_SIZE (_LSCX_UNIT_ID);
   static const int 		_ASCX_SIZE = 
   UNIT_ID_TO_RFM_SIZE (_ASCX_UNIT_ID);
#endif

/* DCU timeout for DDA refresh parameters */
#define 			DCU_WAIT_FOR_REFRESH_RATE	30
#define 			DCU_WAIT_FOR_REFRESH_TIMEOUT	4
#define 			DDCU_CMD_TIMEOUT		120

#ifdef RPC_SVC_FG
#define _SVC_FG		1
#else
#define _SVC_FG		0
#endif
#if !defined (OS_VXWORKS) && !defined (PORTMAP)
#define _SVC_MODE	RPC_SVC_MT_AUTO
#else
#define _SVC_MODE	0
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Globals: shutdownflag	shutdown flag				*/
/*          initServer		if 0 the server is not yet initialized	*/
/*          lscIPC		ipc area of lsc excitation engine	*/
/*          ascIPC		ipc area of asc excitation engine	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int			shutdownflag = 1;
   static int			initServer = 0;
#ifdef _AWG_RM
   static scheduler_t* 		sd = NULL;
   typedef struct rmIpcStr rmIpcStr;
   static rmIpcStr*		lscIPC = NULL;
   static rmIpcStr*		ascIPC = NULL;
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Forward declarations: 						*/
/*	initAWGServer		initializes AWG server			*/
/*	finiAWGServer		cleans up AWG server			*/
/*	initializeDCUs		initializes DCU stuff			*/
/*      rawgapi_1		rpc dispatch function			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   __init__(initAWGServer);
#pragma init(initAWGServer);
   __fini__(finiAWGServer);
#pragma fini(finiAWGServer);
#ifdef _AWG_RM
   static int initializeDCUs (void);
#endif
   extern void rawgprog_1 (struct svc_req* rqstp, 
                     register SVCXPRT* transp);


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Remote Procedure Name: awgnewchannel_1_svc				*/
/*                                                         		*/
/* Procedure Description: reserves a slot for a new channel in the awg	*/
/*                                                         		*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: slot # if successful, <0 if not			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   bool_t awgnewchannel_1_svc (int chntype, int id, int arg1, int arg2,
                     int* result, struct svc_req* rqstp)
   {
      gdsDebug ("reserve an awg channel");
   
      *result = getIndexAWG (chntype, id, arg1, arg2);
      return TRUE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Remote Procedure Name: awgremovechannel_1_svc			*/
/*                                                         		*/
/* Procedure Description: removes a channel(slot) from the awg		*/
/*                                                         		*/
/* Procedure Arguments: slot number					*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if not			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   bool_t awgremovechannel_1_svc (int slot, int* result, 
                     struct svc_req* rqstp)
   {
      gdsDebug ("remove awg channel");
   
      *result = releaseIndexAWG (slot);
      return TRUE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Remote Procedure Name: awgaddwaveform_1_svc				*/
/*                                                         		*/
/* Procedure Description: add a waveform to a awg slot			*/
/*                                                         		*/
/* Procedure Arguments: slot #, awg component				*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if not			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   bool_t awgaddwaveform_1_svc (int slot, awgcomponent_list_r* comps, 
                     int* result, struct svc_req* rqstp)
   {
      AWG_Component* 	comp;		/* awg components */
      int		numComp = 0; 	/* number of components */
      int		i;		/* component index */
      int		j;		/* index */
   
      gdsDebug ("add awg channel waveform");
   
      /* allocate memory */
      comp = NULL;
      if (comps != NULL) {
         numComp = comps->awgcomponent_list_r_len;
         comp = calloc (numComp, sizeof (AWG_Component));
      }
      if (comp == NULL) {
         *result = -1;
         return TRUE;
      }
   
      /* copy result */
      for (i = 0; i < numComp; i++) {
         comp[i].wtype = 
            comps->awgcomponent_list_r_val[i].wtype;
         for (j = 0; j < 4; j++) {
            comp[i].par[j] = 
               comps->awgcomponent_list_r_val[i].par[j];
         }
         comp[i].start = 
            comps->awgcomponent_list_r_val[i].start;
         comp[i].duration = 
            comps->awgcomponent_list_r_val[i].duration;
         comp[i].restart = 
            comps->awgcomponent_list_r_val[i].restart;
         for (j = 0; j < 2; j++) {
            comp[i].ramptime[j] = 
               comps->awgcomponent_list_r_val[i].ramptime[j];
         }
         comp[i].ramptype = 
            comps->awgcomponent_list_r_val[i].ramptype;
         for (j = 0; j < 4; j++) {
            comp[i].ramppar[j] = 
               comps->awgcomponent_list_r_val[i].ramppar[j];
         }
      }
   
      /* add waveform */
      *result = addWaveformAWG (slot, comp, numComp);
      free (comp);
      return TRUE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Remote Procedure Name: awgsetwaveform_1_svc				*/
/*                                                         		*/
/* Procedure Description: set a waveform into a awg slot		*/
/*                                                         		*/
/* Procedure Arguments: slot #, data array and length			*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if not			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   bool_t awgsetwaveform_1_svc (int slot, awgwaveform_r wave, 
                     int* result, struct svc_req* rqstp)
   {
      gdsDebug ("set awg channel waveform");
   
      /* set wavefomrs */
      *result = setWaveformAWG (slot, wave.awgwaveform_r_val, 
                               wave.awgwaveform_r_len);
      return TRUE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Remote Procedure Name: awgsendwaveform_1_svc				*/
/*                                                         		*/
/* Procedure Description: send a waveform stream			*/
/*                                                         		*/
/* Procedure Arguments: slot #, time, epoch, data array and length	*/
/*                                                         		*/
/* Procedure Returns: >=0 if successful, <0 if not			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   bool_t awgsendwaveform_1_svc (int slot, unsigned int time, int epoch, 
                     awgwaveform_r wave, int* result, 
                     struct svc_req* rqstp)
   {
      gdsDebug ("send awg channel stream");
   
      /* send stream */
      *result = sendWaveformAWG (slot, time, epoch,
                                wave.awgwaveform_r_val, 
                                wave.awgwaveform_r_len);
      return TRUE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Remote Procedure Name: awgstopwaveform_1_svc				*/
/*                                                         		*/
/* Procedure Description: stops a waveform in a awg slot		*/
/*                                                         		*/
/* Procedure Arguments: slot #, type of termination, termination arg	*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if not			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   bool_t awgstopwaveform_1_svc (int slot, int terminate, int64_t arg, 
				 int* result, struct svc_req* rqstp)
   {
      gdsDebug ("set awg channel waveform");
   
      /* set wavefomrs */
      *result = stopWaveformAWG (slot, terminate, arg);
      return TRUE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Remote Procedure Name: awgclearwaveforms_1_svc			*/
/*                                                         		*/
/* Procedure Description: clears all waveforms from a awg slot		*/
/*                                                         		*/
/* Procedure Arguments: slot #						*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if not			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   bool_t awgclearwaveforms_1_svc (int slot, int* result, 
                     struct svc_req* rqstp)
   {
      gdsDebug ("clear awg channl waveforms");
   
      /* clear waveforms from awg slot */
      *result = resetAWG (slot);
   
      return TRUE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Remote Procedure Name: awgquerywaveforms_1_svc			*/
/*                                                         		*/
/* Procedure Description: queries waveforms from a awg slot		*/
/*                                                         		*/
/* Procedure Arguments: slot #						*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if not			*/
/*                    waveform components	          		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   bool_t awgquerywaveforms_1_svc (int slot, int maxComp, 
                     awgquerywaveforms_r* result, struct svc_req* rqstp)
   {
      int		numComp;	/* number of components */
      AWG_Component* 	comp;		/* awg components */
      awgcomponent_r*	comps;		/* remote awg components */	
      int		k;		/* component index */
      int		l;		/* index */
   
      gdsDebug ("query awg channel");
   
      /* allocate memory */
      result->wforms.awgcomponent_list_r_val = NULL;
      comp = calloc (maxComp, sizeof (AWG_Component));
   
      /* query awg */
      numComp = queryWaveformAWG (slot, comp, maxComp);
      if (numComp < 0) {
         result->status = numComp;
         return TRUE;
      }
   
      /* allocate memory for result */
      result->wforms.awgcomponent_list_r_len = numComp;
      result->wforms.awgcomponent_list_r_val = 
         calloc (numComp, sizeof (awgcomponent_r));
      comps = result->wforms.awgcomponent_list_r_val;
      if (comps == NULL) {
         result->wforms.awgcomponent_list_r_len = 0;
         result->status = -10;
         return TRUE;
      }
   
      /* copy components */
      for (k = 0; k < numComp; k++) {
         comps[k].wtype = comp[k].wtype;
         for (l = 0; l < 4; l++) {
            comps[k].par[l] = comp[k].par[l];
         }
         comps[k].start = comp[k].start;
         comps[k].duration = comp[k].duration;
         comps[k].restart = comp[k].restart;
         for (l = 0; l < 2; l++) {
            comps[k].ramptime[l] = comp[k].ramptime[l];
         }
         comps[k].ramptype = comp[k].ramptype;
         for (l = 0; l < 4; l++) {
            comps[k].ramppar[l] = comp[k].ramppar[l];
         }
      }
   
      /* free memory */
      free (comp);
   
      /* return */
      result->status = 0;
      return TRUE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Remote Procedure Name: awgsetgain_1_svc				*/
/*                                                         		*/
/* Procedure Description: send a waveform stream			*/
/*                                                         		*/
/* Procedure Arguments: slot #, gain, ramp time				*/
/*                                                         		*/
/* Procedure Returns: >=0 if successful, <0 if not			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   bool_t awgsetgain_1_svc (int slot, double gain, tainsec_t time, 
                     int* result, struct svc_req* rqstp)
   {
      gdsDebug ("set awg gain");
   
      /* set gain */
      *result = setGainAWG (slot, gain, time);
      return TRUE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Remote Procedure Name: awgsetfilter_1_svc				*/
/*                                                         		*/
/* Procedure Description: set filter function				*/
/*                                                         		*/
/* Procedure Arguments: slot #, filter coeff. array and length		*/
/*                                                         		*/
/* Procedure Returns: >=0 if successful, <0 if not			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   bool_t awgsetfilter_1_svc (int slot, awgfilter_r filter, int* result, 
                     struct svc_req* rqstp)
   {
      gdsDebug ("set awg filter");
   
      /* set filter */
      *result = setFilterAWG (slot, filter.awgfilter_r_val, 
                             filter.awgfilter_r_len);
      return TRUE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Remote Procedure Name: awgreset_1_svc				*/
/*                                                         		*/
/* Procedure Description: resets the whole awg bank			*/
/*                                                         		*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if not			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   bool_t awgreset_1_svc (int* result, struct svc_req* rqstp)
   {
      gdsDebug ("reset all awg channels");
   
      /* reset AWG */
      *result = releaseIndexAWG (-1);
   
      return TRUE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Remote Procedure Name: awgshow_1_svc					*/
/*                                                         		*/
/* Procedure Description: returns awg configuration			*/
/*                                                         		*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if not			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   bool_t awgshow_1_svc (awgshow_r* result, struct svc_req* rqstp)
   {
      char*		p;
   
      gdsDebug ("show all awg channels");
   
      /* reset AWG */
      result->res = NULL;
      p = malloc (_SHOWBUF_SIZE);
      if (p == NULL) {
         return FALSE;
      }
      if (showAllAWGs (p, _SHOWBUF_SIZE) == NULL) {
         free (p);
         result->status = -1;
         result->res = malloc (1);
         if (result->res == NULL) {
            return FALSE;
         }
         result->res[0] = 0;
         return TRUE;
      }
      if (strlen (p) < _SHOWBUF_SIZE - 1) {
         p = realloc (p, strlen (p) + 1);
         if (p == NULL) {
            return FALSE;
         }
      }
      /* result */
      result->status = 0;
      result->res = p;
   
      return TRUE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Remote Procedure Name: awgshow_1_svc					*/
/*                                                         		*/
/* Procedure Description: returns awg configuration			*/
/*                                                         		*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if not			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   bool_t awgshowslot_1_svc (int slot, awgshow_r* result, 
                     struct svc_req* rqstp)
   {
      char*		p;
   
      gdsDebug ("show awg channel");
   
      /* reset AWG */
      result->res = NULL;
      p = malloc (_SHOWBUF_SIZE);
      if (p == NULL) {
         return FALSE;
      }
      if (showAWG (slot, p, _SHOWBUF_SIZE) < 0) {
         free (p);
         result->status = -1;
         result->res = malloc (1);
         if (result->res == NULL) {
            return FALSE;
         }
         result->res[0] = 0;
         return TRUE;
      }
      if (strlen (p) < _SHOWBUF_SIZE - 1) {
         p = realloc (p, strlen (p) + 1);
         if (p == NULL) {
            return FALSE;
         }
      }
      /* result */
      result->status = 0;
      result->res = p;
   
      return TRUE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Remote Procedure Name: awgstatistics_1_svc				*/
/*                                                         		*/
/* Procedure Description: resets the whole awg bank			*/
/*                                                         		*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if not			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/           
   bool_t awgstatistics_1_svc (int reset, awgstat_r* result, 
                     struct svc_req* rqstp)
   {
      gdsDebug ("get statistics of awg");
   
      /* reset if non zero */
      if (reset != 0) {
         result->status = getStatisticsAWG (NULL);
      }
      else {
         result->status = getStatisticsAWG ((awgStat_t*) result);
      }
   
      return TRUE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: rawgprog_1_freeresult			*/
/*                                                         		*/
/* Procedure Description: frees memory form rpc call			*/
/*                                                         		*/
/* Procedure Arguments: rpc transport info, xdr routine for result,	*/
/*			pointer to result				*/
/*                                                         		*/
/* Procedure Returns: TRUE if successful, FALSE if failed		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int rawgprog_1_freeresult (SVCXPRT* transp, 
                     xdrproc_t xdr_result, caddr_t result)
   {
      (void) xdr_free (xdr_result, result);
      return TRUE;
   }


#if defined(_AWG_RM)
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: updateCycleCount				*/
/*                                                         		*/
/* Procedure Description: updates test points DCU IPC area		*/
/*                                                         		*/
/* Procedure Arguments: scheduler task, time, epoch, void*		*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if not			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int updateCycleCount (schedulertask_t* info, taisec_t time, 
                     int epoch, void* arg)
   {
      taisec_t		dtime; 	/* time of data */
      int		depoch;	/* epcoh of data */
   
      /* calculate cycle count */
      depoch = epoch - DATA_EPOCH_DELAY;
      if (depoch < 0) {
         depoch += NUMBER_OF_EPOCHS;
         dtime = time - 1;
      }
      else {
         dtime = time;
      }
      /* update IPC area */
      if (lscIPC != NULL) {
         lscIPC->bp[depoch % DATA_BLOCKS].timeSec = dtime;
         lscIPC->bp[depoch % DATA_BLOCKS].timeNSec = depoch * _EPOCH;
         lscIPC->bp[depoch % DATA_BLOCKS].cycle = depoch;
         lscIPC->cycle = depoch;
      }
      if (ascIPC != NULL) {
         ascIPC->bp[depoch % DATA_BLOCKS].timeSec = dtime;
         ascIPC->bp[depoch % DATA_BLOCKS].timeNSec = depoch * _EPOCH;
         ascIPC->bp[depoch % DATA_BLOCKS].cycle = depoch;
         ascIPC->cycle = depoch;
      }
      /* update run flag */
      if ((epoch % NUMBER_OF_EPOCHS == 0) ||
         (epoch % NUMBER_OF_EPOCHS == 4) ||
         (epoch % NUMBER_OF_EPOCHS == 8) ||
         (epoch % NUMBER_OF_EPOCHS == 12)) {
      	 /* if running update RUN status */
         if (checkAWG ()) {
            if (lscIPC != NULL) {
               lscIPC->status = DAQ_STATE_RUN;
            }
            if (ascIPC != NULL) {
               ascIPC->status = DAQ_STATE_RUN;
            }
         }
         /* if suspended try to restart */
         else if (epoch % NUMBER_OF_EPOCHS == 4) {
            restartAWG();
         }
      }
   
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: initializeDCUs				*/
/*                                                         		*/
/* Procedure Description: initializes DCU areas				*/
/*                                                         		*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int initializeDCUs (void) 
   {
      int		k;	/* data block index */
      char*		addr;	/* rm address */
      schedulertask_t	task;	/* update task entry */
   
      /* fill in pointers */
      if ((_LSCX_BASE >= 0) &&
         (rmCheck (_RM_ID, _LSCX_BASE, _LSCX_SIZE))) {
         lscIPC = (rmIpcStr*) (rmBaseAddress (_RM_ID) + _LSCX_BASE);
      }
      if ((_ASCX_BASE >= 0) &&
         (rmCheck (_RM_ID, _ASCX_BASE, _ASCX_SIZE))) {
         ascIPC = (rmIpcStr*) (rmBaseAddress (_RM_ID) + _ASCX_BASE);
      }
   
      /* clear data area */
      if (lscIPC != NULL) {
         addr = rmBaseAddress (_RM_ID) + 
                UNIT_ID_TO_DATA_OFFSET(_LSCX_UNIT_ID);
         memset (addr, 0, DATA_BLOCKS *
                UNIT_ID_TO_DATA_BLOCKSIZE(_LSCX_UNIT_ID));
      }
      if (ascIPC != NULL) {
         addr = rmBaseAddress (_RM_ID) + 
                UNIT_ID_TO_DATA_OFFSET(_ASCX_UNIT_ID);
         memset (addr, 0, DATA_BLOCKS *
                UNIT_ID_TO_DATA_BLOCKSIZE(_ASCX_UNIT_ID));
      }
   
      /* initial IPC area */
      if (lscIPC != NULL) {
         memset (lscIPC, 0, sizeof (rmIpcStr));
         lscIPC->status = DAQ_STATE_RUN_CONFIG;
         lscIPC->dcuId = _LSCX_UNIT_ID;
      #if RMEM_LAYOUT == 0
         lscIPC->dcuType = DAQS_UTYPE_GDS_EXC;
         lscIPC->dcuNodeId = UNIT_ID_TO_RFM_NODE_ID(_LSCX_UNIT_ID);
         lscIPC->errMsg = DAQ_OK;
      #endif
         lscIPC->command = DAQS_CMD_NO_CMD;
         lscIPC->reqAck = DAQS_CMD_NO_CMD;
         lscIPC->request = DAQS_CMD_NO_CMD;
         lscIPC->cmdAck = DAQS_CMD_NO_CMD;
         lscIPC->status = DAQ_STATE_RUN;
         for (k = 0; k < DATA_BLOCKS; k++) {
            lscIPC->bp[k].run = 0;
            lscIPC->bp[k].timeSec = 0;
            lscIPC->bp[k].timeNSec = 0;
            lscIPC->bp[k].cycle = 0xFFFFFFFF;
         }
         lscIPC->cycle = 0xFFFFFFFF;
      #if RMEM_LAYOUT == 0
         strcpy (lscIPC->confName, DAQ_CNAME_NO_CONFIG);
      #endif
      #if 0
         /* ask for refresh of configuration */
         lscIPC->channelCount = -1;
         /*   clear request ack and set request for command. */
         lscIPC->request = DAQS_CMD_RFM_REFRESH;
         k = 0;   
         do {
            taskDelay (1);
            k++;
         } while ((lscIPC->reqAck != DAQS_CMD_RFM_REFRESH) &&
                 (k < DDCU_CMD_TIMEOUT));
         if (k == DDCU_CMD_TIMEOUT) {
            gdsError (GDS_ERR_TIME, 
                     "Unable to load LSC DCU EX configuration");
         }
         /* wait till configuration has updated */
         else {
            k = 0;
            while ((lscIPC->channelCount < 0) && 
                  (k < DCU_WAIT_FOR_REFRESH_TIMEOUT)) {
               taskDelay(DCU_WAIT_FOR_REFRESH_RATE);
               k++;
            }
            if (k == DCU_WAIT_FOR_REFRESH_TIMEOUT) {
               gdsError (GDS_ERR_TIME, 
                        "Unable to load LSC DCU EX configuration");
            }
         }
      #endif
      }
   
      if (ascIPC != NULL) {
         memset (ascIPC, 0, sizeof (rmIpcStr));
         ascIPC->status = DAQ_STATE_RUN_CONFIG;
         ascIPC->dcuId = _LSCX_UNIT_ID;
      #if RMEM_LAYOUT == 0
         ascIPC->dcuType = DAQS_UTYPE_GDS_EXC;
         ascIPC->dcuNodeId = UNIT_ID_TO_RFM_NODE_ID(_ASCX_UNIT_ID);
         ascIPC->errMsg = DAQ_OK;
      #endif
         ascIPC->command = DAQS_CMD_NO_CMD;
         ascIPC->reqAck = DAQS_CMD_NO_CMD;
         ascIPC->request = DAQS_CMD_NO_CMD;
         ascIPC->cmdAck = DAQS_CMD_NO_CMD;
         ascIPC->status = DAQ_STATE_RUN;
         for (k = 0; k < DATA_BLOCKS; k++) {
            ascIPC->bp[k].run = 0;
            ascIPC->bp[k].timeSec = 0;
            ascIPC->bp[k].timeNSec = 0;
            ascIPC->bp[k].cycle = 0xFFFFFFFF;
         }
         ascIPC->cycle = 0xFFFFFFFF;
      #if RMEM_LAYOUT == 0
         strcpy (ascIPC->confName, DAQ_CNAME_NO_CONFIG);
      #endif
      #if 0
         /* ask for refresh of configuration */
         ascIPC->channelCount = -1;
         /*   clear request ack and set request for command. */
         ascIPC->request = DAQS_CMD_RFM_REFRESH;
         k = 0;   
         do {
            taskDelay (1);
            k++;
         } while ((ascIPC->reqAck != DAQS_CMD_RFM_REFRESH) &&
                 (k < DDCU_CMD_TIMEOUT));
         if (k == DDCU_CMD_TIMEOUT) {
            gdsError (GDS_ERR_TIME, 
                     "Unable to load LSC DCU EX configuration");
         }
         /* wait till configuration has updated */
         else {
            k = 0;
            while ((ascIPC->channelCount < 0) && 
                  (k < DCU_WAIT_FOR_REFRESH_TIMEOUT)) {
               taskDelay(DCU_WAIT_FOR_REFRESH_RATE);
               k++;
            }
            if (k == DCU_WAIT_FOR_REFRESH_TIMEOUT) {
               gdsError (GDS_ERR_TIME, 
                        "Unable to load LSC DCU EX configuration");
            }
         }
      #endif
      }
   
      /* setup task info structure for update cycle count */
      SET_TASKINFO_ZERO (&task);
      task.flag = SCHED_REPEAT;
      task.repeattype = SCHED_REPEAT_INFINITY;
      task.repeatratetype = SCHED_REPEAT_EPOCH;
      task.repeatrate = 1;
      task.repeatsynctype = SCHED_SYNC_NEXT;
      task.func = updateCycleCount;
   
      /* schedule cycle count task */
      if (scheduleTask (sd, &task) < 0) {
         closeScheduler (sd, 3 * _EPOCH);
         sd = NULL;
         return -5;
      }
   
      /* return */
      return 0;
   }
#endif

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: awg_server					*/
/*                                                         		*/
/* Procedure Description: start rpc service task			*/
/*                                                         		*/
/* Procedure Arguments: id # of AWG					*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int awg_server (void)
   {
      int		rpcpmstart;	/* port monitor flag */
      SVCXPRT*		transp;		/* service transport */
      int		proto;		/* protocol */
      unsigned long	prognum;	/* rpc prog. num. */
      unsigned long	progver;	/* rpc prog. ver. */
      static confServices conf;		/* configuration service */
      static char	confbuf[256];	/* configuration buffer */
      struct in_addr	host;		/* local host address */

      /* test if low level init */
      gdsDebug ("start awg server client");
      if (initServer == 0) {
         initAWGServer();
         if (initServer == 0) {
            return -1;
         }
      }
   
      /* initialize AWG */
      if (initAWG() < 0) {
         gdsError (GDS_ERR_MISSING, "Unable to initialize AWG");
         return -2;
      }
   
      /* make sure heartbeat is installed */
      if (installHeartbeat (NULL) < 0) {
         return -3;
      }
   
      /* init scheduler */
      sd = createScheduler (0, NULL, NULL);
      if (sd == NULL) {
         return -4;
      }
   
      /* get rpc parameters from parameter file */
      prognum = RPC_PROGNUM_AWG;
      progver = RPC_PROGVER_AWG;
      
      if ((prognum == 0) || (progver == 0)) {
         return -5;
      }

      prognum += testpoint_manager_node;
   
   #if defined(_AWG_RM)
      /* initialize DCUs */
      if (initializeDCUs () < 0) {
         gdsError (GDS_ERR_PROG, "unable to initialize DCUs");
         return -6;
      }
   #endif
   
      /* init rpc services */
      if (rpcInitializeServer (&rpcpmstart, _SVC_FG, _SVC_MODE,
         &transp, &proto) < 0) {
         gdsError (GDS_ERR_PROG, "unable to start rpc service");
         return -7;
      }
   
      /* register rpc service */
      if (rpcRegisterService (rpcpmstart, transp, proto, 
         prognum, progver, rawgprog_1) != 0) {
         return -8;
      }
   
       /* get local address */
      if (rpcGetLocaladdress (&host) < 0) {
         gdsError (GDS_ERR_PROG, "unable to obtain local address");
         return -3;
      }
   
      /* add configuration info */
#ifdef GDS_UNIX_TARGET
      sprintf (confbuf, "awg %i 0 %s %ld %ld",
              testpoint_manager_node,
              inet_ntoa (host), prognum, progver);
#else
      sprintf (confbuf, "awg %i 0 %s %ld %ld",
              ((IFO != GDS_IFO2) ? 0 : 1),
              inet_ntoa (host), prognum, progver);
#endif
      conf.id = 0;
      conf.answer = stdAnswer;
      conf.user = confbuf;
   
      /* announce service */
      if (conf_server (&conf, 1, 1) < 0) {
         gdsWarningMessage ("unable to start configuration services");
      }
   
      printf ("Server for arbitrary waveform generator (%lx / %li)\n", 
             prognum, progver);
   
      /* wait for rpc calls */
      rpcStartServer (rpcpmstart, &shutdownflag);
   
      /* never reached */
      return -9;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: initAWGServer				*/
/*                                                         		*/
/* Procedure Description: initializes AWG server			*/
/*                                                         		*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns:void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void initAWGServer (void) 
   {
      if (initServer != 0) {
         return;
      }
      /* First time, log the version info. */
      printf("awg_server %s\n", versionId) ;
   
      /* set initServer and return */
      shutdownflag = 1;
      initServer = 1;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: finiAWGServer				*/
/*                                                         		*/
/* Procedure Description: cleans up AWG server				*/
/*                                                         		*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns:void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void finiAWGServer (void) 
   {
      if (initServer == 0) {
         return;
      }
   
      /* close scheduler */
      if (sd != NULL) {
         closeScheduler (sd, 3 * _EPOCH);
         sd = NULL;
      }
   
      /* set initServer and return */
      shutdownflag = 0;
      initServer = 0;
   }
