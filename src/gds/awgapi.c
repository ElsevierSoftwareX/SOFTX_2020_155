static char *versionId = "Version $Id$" ;
/* -*- mode: c; c-basic-offset: 3; -*- */
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: awgapi							*/
/*                                                         		*/
/* Module Description: implements functions for controlling the AWG	*/
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

/* Header File List: */
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#ifdef OS_VXWORKS
#include <vxWorks.h>
#include <semLib.h>
#include <taskLib.h>
#include <inetLib.h>
#include <hostLib.h>
#include <sysLib.h>
#include <timers.h>

#else
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "dtt/gdsutil.h"
#include "dtt/awgapi.h"
#include "dtt/rawgapi.h"
#include "dtt/awgfunc.h"
#include "dtt/gdschannel.h"
#include "dtt/ds340.h"
#ifndef _NO_TESTPOINTS
#include "dtt/testpointinfo.h"
#endif
#include "dtt/targets.h"
#include "dtt/rmorg.h"
#if defined (_CONFIG_DYNAMIC)
#include "dtt/confinfo.h" 
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Defines: Describes the local AWG capabilities			*/
/*          _AWG_LOCAL		local AWG is present			*/
/*          _AWG_LOCAL_SLOT	slot # of local AWG			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#if (TARGET == TARGET_H1_GDS_AWG1) ||  (TARGET == TARGET_H1_GDS_AWG1+10) 
#define _AWG_LOCAL
#define _AWG_LOCAL_SLOT		AWG_ID(0,0)

#elif (TARGET == TARGET_H2_GDS_AWG1) || (TARGET == TARGET_H2_GDS_AWG1+10)
#define _AWG_LOCAL
#define _AWG_LOCAL_SLOT		AWG_ID(1,0)

#elif (TARGET == TARGET_L1_GDS_AWG1) || (TARGET == TARGET_L1_GDS_AWG1+10)
#define _AWG_LOCAL
#define _AWG_LOCAL_SLOT		AWG_ID(0,0)

#elif (TARGET == TARGET_M_GDS_UNIX || TARGET == TARGET_L_GDS_UNIX || TARGET == TARGET_H_GDS_UNIX)
#define _AWG_LOCAL
#define _AWG_LOCAL_SLOT		AWG_ID(0,0)
#endif

#ifdef _AWG_LOCAL
#include "awg.h"
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Constants: _NETID		  net protocol used for rpc		*/
/* 	      _MAX_IFO		  maximum number of ifo's		*/
/*            _MAX_AWG_PER_IFO	  maximum number of awg's / ifo		*/
/*            _SHOWBUF_SIZE	  maximum size of show reply		*/
/*            MAX_SLOT_NAME	  maximum length of channel name	*/
/*            MAX_SLOT_LIST	  maximum number of cached channel names*/
/*            PRM_PATH		  parameter file path			*/
/*            PRM_FILE		  parameter file name			*/
/*            PRM_SECTION	  section heading for awg		*/
/*            PRM_SECTION2	  section heading for cobox		*/
/*            PRM_ENTRY1	  parameter file host entry		*/
/*            PRM_ENTRY2	  parameter file rpc prog.num. entry	*/
/*            PRM_ENTRY3	  parameter file rpc vers.num. entry	*/
/*            PRM_ENTRY4	  parameter file cobox port # entry	*/
/*            _HELP_TEXT	  help text for cmd line interface	*/
/*            								*/
/*----------------------------------------------------------------------*/
#define _NETID			"tcp"
#define _MAX_IFO		128
#define _MAX_AWG_PER_IFO	5
#define MAX_SLOT_NAME 		256
#define MAX_SLOT_LIST 		16
#define _SHOWBUF_SIZE		(128 * 1024)
#if !defined (_AWG_LIB) && !defined (_CONFIG_DYNAMIC)
#define PRM_FILE		gdsPathFile ("/param", "awg.par")
#define PRM_SECTION		"awg"
#define PRM_SECTION2		"dsg"
#define PRM_ENTRY1		"hostname"
#define PRM_ENTRY2		"prognum"
#define PRM_ENTRY3		"progver"
#define PRM_ENTRY4		"port"
#endif
#define _HELP_TEXT \
	"Arbitrary waveform generator commands:\n" \
	"  help: shows this help text\n" \
	"  channels : display all excitation channels\n" \
	"  show 'node'.'awg': show awg usage\n" \
	"  new 'channel': reserve an awg slot\n" \
	"  free 'slot' : frees the awg slot\n" \
	"  set/add 'slot' 'waveform': sets/adds a waveform\n" \
	"  gain 'slot' 'value' 'tRamp': sets the overall gain of a slot\n" \
	"  stop 'slot': stops wavforms of an awg slot\n" \
	"  ramp 'tRamp': sets the general phase in/out time\n" \
	"  clear 'node'.'awg' : reset an awg\n" \
	"  stat 'node'.'awg' : get statistics data\n" \
	"Parameters:\n" \
	"   node: interferometer number, starting at zero\n" \
	"   awg: awg number, stating at zero\n" \
	"   channel: full channel name, e.g. H1:LSC-TEST_IN\n" \
	"   slot: slot number returned by new or original channel name\n" \
	"   tRamp: ramp time; default is 1\n" \
	"   waveform 1: 'func' freq ampl ofs phase ratio\n" \
	"   waveform 2: impulse freq ampl duration delay\n" \
	"   waveform 3: const ampl\n" \
	"   waveform 4: 'noise' freq1 freq2 ampl ofs\n" \
	"   waveform 5: sweep freq1 freq2 ampl1 ampl2 time 'sweeptype' 'updn'\n" \
	"   waveform 6: arb freq scale 'trigger' rate point1 point2 point3 ...\n" \
	"   waveform 7: stream scaling\n" \
	"   func: sine, square, ramp or triangle\n" \
	"   noise: normal or uniform\n" \
	"   sweeptype: linear or log\n" \
	"   updn: + (up), - (down) or blank for bidirectional\n" \
	"   trigger: c (continous) r (random) w (wait) t (trig) \n"


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Types:   awgHost_t		host addresses type            		*/
/*          slotentry		channel name cache for slots		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#if defined (_AWG_LIB) || defined (_CONFIG_DYNAMIC)
   struct awgHost_t {
      int		valid;
      char		hostname[100];
      unsigned long	prognum;
      unsigned long	progver;
   };
   typedef struct awgHost_t awgHost_t;
#endif

   struct slotentry {
      char name[MAX_SLOT_NAME];
      int slot;
   };
   typedef struct slotentry slotentry;


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Globals: awg_init		whether clients were already init.	*/
/*          awg_clnt		rpc client handles		 	*/
/*          ds340addr		network address of DS340		*/
/*          ds340port		network port of DS340  			*/
/*          awgHost		host addresses            		*/
/*          slotlist		channel name cache for slot numbers	*/
/*          slotinit		slot initialized	       		*/
/*          ramptime		general phase in/out time in sec	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int		awg_init = 0;
   static CLIENT*	awg_clnt[_MAX_IFO][_MAX_AWG_PER_IFO];
   static char		ds340addr[NUM_DS340][256] = {{0}};
   static int		ds340port[NUM_DS340] = {0};
#if defined (_AWG_LIB) || defined (_CONFIG_DYNAMIC)
   static awgHost_t	awgHost[_MAX_IFO][_MAX_AWG_PER_IFO] = {{{0}}};
#endif
   static slotentry 	slotlist[MAX_SLOT_LIST];
   static int 		slotinit = 1;
   static double 	ramptime = 0.0;

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Forward declarations: 						*/
/*	initAWGclient		init of rpc client			*/
/*      								*/
/*----------------------------------------------------------------------*/
   static int initAWGclient (void);
#if defined (_CONFIG_DYNAMIC) && !defined (_AWG_LIB)
   static int ds340SetHostAddress (int ds340, const char* hostname, 
                     int port);
   static int awgSetHostAddress (int ifo, int awg, const char* hostname, 
                     unsigned long prognum, unsigned long progver);
#endif
   static void initSlot (void);
   static int updateSlot (int slot, const char* name);
   static int freeSlot (int slot);
   static const char* readSlot (const char* p, int* slot);


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: awgCheckInterface				*/
/*                                                         		*/
/* Procedure Description: check if AWG interface exists			*/
/*                                                         		*/
/* Procedure Arguments: node, awg #					*/
/*                                                         		*/
/* Procedure Returns: 1 if exists, 0 otherwise				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int awgCheckInterface (int node, int j)
   {
      if ((node < 0) || (node >= _MAX_IFO) ||
         ((j < 0) || (j >= _MAX_AWG_PER_IFO))) {
         return 0;
      }
   #ifdef _AWG_LOCAL
      if (AWG_ID (node, j) == _AWG_LOCAL_SLOT) {
         return 1;
      }
   #endif
      return (awg_clnt[node][j] != NULL);
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: awgSetChannel				*/
/*                                                         		*/
/* Procedure Description: reserves an awg slot to a channel		*/
/*                                                         		*/
/* Procedure Arguments: channel name					*/
/*                                                         		*/
/* Procedure Returns: slot number if successful, <0 if failed		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int awgSetChannel (const char* name)
   {
      static int	lastawgnum = 0; /* stores the last awg num */
      int		status;	/* status flag */
      int 		ret;	/* return value */
      int		node;	/* node ID */
      int		j;	/* index */
      gdsChnInfo_t	chn;	/* channel info */
      int		chntype;/* channel type */
      int		arg1 = 0;/* first argument for new channel */
      int		arg2 = 0;/* second argument for new channel */
      int		id;	/* channel id */
   #ifndef _NO_TESTPOINTS
      testpoint_t	tp;	/* test point id */
   #endif
   
      /* test whether awg interface is initialized */
      if (awg_init == 0) {
         status = awg_client();
         if (status < 0) {
            return status - 10;
         }
      }
   
      /* get channel information */
      if (gdsChannelInfo (name, &chn) < 0) {
         return -1;
      }
   #ifdef _NO_TESTPOINTS
      if (chn.rmOffset < 0) {
         chntype = AWG_DS340_CHANNEL;
         node = chn.rmId;
         id = chn.chNum;
         arg1 = arg2 = 0;
      }
      else {
         chntype = AWG_MEM_CHANNEL;
         node = chn.rmId;
         id = chn.rmOffset;
         arg1 = chn.rmBlockSize;
         arg2 = chn.dataRate / NUMBER_OF_EPOCHS;
      }
   #else
      chntype = 0;
      if (tpIsValid (&chn, &node, &tp)) {
         id = tp;
      }
      else {
         /* not a valid excitation channel */
         return -1;
      }
   
      /* determine channel type */
      if (chntype == 0) {
         arg1 = arg2 = 0;
         switch (TP_ID_TO_INTERFACE (tp))
         {
            /* LSC test point channel */
            case TP_LSC_EX_INTERFACE:
               {
                  chntype = AWG_LSC_TESTPOINT;
                  break;
               }
            /* ASC test point channel */
            case TP_ASC_EX_INTERFACE:
               {
                  chntype = AWG_ASC_TESTPOINT;
                  break;
               }
            /* DAC channel */
            case TP_DAC_INTERFACE:
               {
                  chntype = AWG_DAC_CHANNEL;
                  arg1 = AWG_DAC_DELAY;
                  break;
               }
            /* DS340 channel */
            case TP_DS340_INTERFACE:
               {
                  chntype = AWG_DS340_CHANNEL;
                  break;
               }
            /* not an excitation test point/channel */
            default : 
               {
                  return -2;
               }
         }
      }
   #endif
   
      /* treat DS340 separately */
      if (chntype == AWG_DS340_CHANNEL) {
         char		buf[512];
         id -= TP_ID_DS340_OFS;
         /* printf ("test %i - %s - %i\n", id,
                ds340addr[id], ds340port[id]);*/
         if ((id >= 0) && (id < NUM_DS340) &&
            (connectCoboxDS340 (id, ds340addr[id], ds340port[id]) >= 0)) {
            sprintf (buf, "found DSG @ cobox %s/port%i\n", 
                    ds340addr[id], ds340port[id]);
            gdsDebug (buf);
            return AWG_ID (_MAX_IFO, 0) + id;
         }
         else {
            sprintf (buf, "no DSG @ cobox %s/port%i\n", 
                    ds340addr[id], ds340port[id]);
            gdsDebug (buf);
            return -7;
         }
      }
   
      /* we have a valid awg output type! Now check node id */
      if ((node < 0) || (node >= _MAX_IFO)) {
         return -3;
      }
   
      /* cycle through awg number */
      for (j = lastawgnum + 1; j <= lastawgnum + _MAX_AWG_PER_IFO; j++) {
         if (awgCheckInterface (node, j % _MAX_AWG_PER_IFO)) {
            break;
         }
      }
      j %= _MAX_AWG_PER_IFO;
      lastawgnum = j;
   
      /* test validity of awg */
      if (!awgCheckInterface (node,j)) {
         return -4;
      }
   
      /* test for local awg */
   #ifdef _AWG_LOCAL
      if (AWG_ID (node, j) == _AWG_LOCAL_SLOT) {
         ret = getIndexAWG (chntype, id, arg1, arg2);
      }
      else {
      #endif
      
      /* ask awg for free slot */
         if (awgnewchannel_1 (chntype, id, arg1, arg2, &ret, 
            awg_clnt[node][j]) != RPC_SUCCESS) {
            return -5;
         }
      #ifdef _AWG_LOCAL
      }
   #endif
   
      /* was a free slot available */
      if (ret < 0) {
         return -6;
      }
   
      /* return slot number */
      return AWG_ID (node, j) + ret;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: awgRemoveChannel				*/
/*                                                         		*/
/* Procedure Description: removes a channel/slot from an awg 		*/
/*                                                         		*/
/* Procedure Arguments: slot						*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 when failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int awgRemoveChannel (int slot)
   {
      int		status;	/* status flag */
      int 		ret;	/* return value */
      int		i;	/* index */
      int		j;	/* index */
      int		sl; 	/* slot within awg */
   
      /* test whether awg interface is initialized */
      if (awg_init == 0) {
         status = awg_client();
         if (status < 0) {
            return status - 10;
         }
      }
   
      /* use slot number to determine awg */
      i = slot / _AWG_IFO_OFS - 1;
      j = (slot % _AWG_IFO_OFS) / _AWG_NUM_OFS;
      sl = (slot % _AWG_IFO_OFS) % _AWG_NUM_OFS;
   
      /* treat DS340 separately */
      if ((i == _MAX_IFO) && (sl >= 0) && (sl < NUM_DS340)) {
         return resetDS340 (sl);
      }
   
      /* test parameters */
      if (!awgCheckInterface (i,j) || (sl < 0)) {
         return -1;
      }
   
      /* test for local awg */
   #ifdef _AWG_LOCAL
      if (AWG_ID (i, j) == _AWG_LOCAL_SLOT) {
         return releaseIndexAWG (sl);
      }
   #endif
   
      /* clear channel on remote AWG */
      if (awgremovechannel_1 (sl, &ret, awg_clnt[i][j]) != 
         RPC_SUCCESS) {
         return -2;
      }
   
      /* return error code */
      if (ret >= 0) {
         return 0;
      }
      else {
         return ret - 2;
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: isExcitationChannel				*/
/*                                                         		*/
/* Procedure Description: returns true if a valid excitation channel	*/
/*                                                         		*/
/* Procedure Arguments: channel info					*/
/*                                                         		*/
/* Procedure Returns: true if excitation channel			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int isExcitationChannel (const gdsChnInfo_t* info)
   {
      int		node;
      testpoint_t	tp;

      if (!tpIsValid (info, &node, &tp)) {
         return 0;
      }

      if ((node < 0) || (node >= _MAX_IFO)) {
         return 0;
      }
      switch (tpType (info)) {
         case tpInvalid:
         case tpLSC:
         case tpASC:
            return 0;
         default:
            return 1;
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: awgGetChannelNames				*/
/*                                                         		*/
/* Procedure Description: get all valid channel names	 		*/
/*                                                         		*/
/* Procedure Arguments: names, length					*/
/*                                                         		*/
/* Procedure Returns: length if successful, <0 when failed		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int awgGetChannelNames (char* names, int len, int info)
   {
      int		status;	/* status flag */
      int		size;	/* size of channel list */
      char*		chn;	/* channel list */
   
      /* test whether awg interface is initialized */
      if (awg_init == 0) {
         status = awg_client();
         if (status < 0) {
            return status - 10;
         }
      }

      /* allocate channel list */
      chn = gdsChannelNames (-1, isExcitationChannel, info);
      size = strlen (chn);
      if (names == NULL) {
         free (chn);
         return size;
      }
      else {
         if (size > len - 1) size = len - 1;
         strncpy (names, chn, size);
         names[size] = 0;
         free (chn);
         return size;
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: awgAddWaveform				*/
/*                                                         		*/
/* Procedure Description: adds waveforms to an awg slot			*/
/*                                                         		*/
/* Procedure Arguments: slot, list of components			*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 when failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int awgAddWaveform (int slot, AWG_Component* comp, int numComp)
   {
      int		status;	/* status flag */
      int 		ret;	/* return value */
      int		i;	/* index */
      int		j;	/* index */
      int		sl; 	/* slot within awg */
      int		k;	/* index */
      int		l;	/* index */
      awgcomponent_list_r wforms;	/* awg components */
   
      /* test whether awg interface is initialized */
      if (awg_init == 0) {
         status = awg_client();
         if (status < 0) {
            return status - 10;
         }
      }
   
      /* anything to do? */
      if (numComp <= 0) {
         return 0;
      }
   
      /* use slot number to determine awg */
      i = slot / _AWG_IFO_OFS - 1;
      j = (slot % _AWG_IFO_OFS) / _AWG_NUM_OFS;
      sl = (slot % _AWG_IFO_OFS) % _AWG_NUM_OFS;
   
      /* treat DS340 separately */
      if ((i == _MAX_IFO) && (sl >= 0) && (sl < NUM_DS340) &&
         isDS340Alive (sl)) {
         DS340_ConfigBlock	conf;	/* configuration block */
      
         /* get configuration */;
         getDS340 (sl, &conf);
         conf.toggles &= DS340_INVT | DS340_SYNC | DS340_TERM;
      
         /* set waveform */
         if (comp->wtype == awgSine) {
            conf.func = ds340_sin;
         } 
         else if (comp->wtype == awgSquare) {
            conf.func = ds340_square;
         }
         else if (comp->wtype == awgRamp) {
            conf.func = ds340_ramp;
         }
         else if (comp->wtype == awgTriangle) {
            conf.func = ds340_triangle;
         } 
         else if (comp->wtype == awgNoiseN) {
            conf.func = ds340_noise;
         }
         else if (comp->wtype == awgArb) {
            conf.func = ds340_arb;
            conf.toggles |= DS340_TSRC;
         }
         else {
            return -3;
         }
      
         if ((comp->ramptime[0] < 1E-9) && (comp->ramptime[1] < 1E-9)) {
             /* periodic, noise or abritrary waveform */
            if (comp->wtype == awgNoiseN) {
               conf.ampl = comp->par[0];
               conf.offs = comp->par[3];
            }
            else if (comp->wtype == awgArb) {
               conf.fsmp = comp->par[1];
               conf.ampl = comp->par[0];
               conf.offs = comp->par[3];
            }
            else {
               conf.freq = comp->par[1];
               conf.ampl = comp->par[0];
               conf.offs = comp->par[3];
            }
         }
         /* no test for sweep */
         else if ((comp->ramptime[0] == 0) && (comp->ramptime[1] != 0) &&
                 (comp->restart > 0) && (comp->duration > 0) &&
                 (conf.func != ds340_noise)) {
            /* sweep waveform */
            conf.toggles |= DS340_SWEN | DS340_STRS;
            if (SWEEP_OUT_TYPE (comp->ramptype) == AWG_PHASING_LOG) {
               conf.toggles |= DS340_STYP;
            } 
            else if (SWEEP_OUT_TYPE (comp->ramptype) != 
                    AWG_PHASING_LINEAR) {
               return -3;
            }
            if ((numComp > 1) && 
               (comp[1].start == comp[0].start + comp[0].duration) &&
               (comp[0].par[1] == comp[1].ramppar[1]) &&
               (comp[1].par[1] == comp[0].ramppar[1])) {
               conf.toggles |= DS340_SDIR;
            }
            conf.func = ds340_sin;
            conf.stfr = comp[0].par[1];
            conf.spfr = comp[0].ramppar[1];
            conf.ampl = comp[0].par[0];
            conf.srat = 1.0/(double)comp[0].restart;
            if ((conf.toggles & DS340_STYP) != 0) {
               conf.srat *= 2;
            }
         }
         else {
            return -3;
         }
      	 /* set waveform */
         setDS340 (sl, &conf);
         if (uploadDS340Block (sl) < 0) {
            return -2;
         } 
         else {
            return 0;
         }
      }
   
      /* test parameters */
      if (!awgCheckInterface (i, j) || (sl < 0)) {
         return -1;
      }
   
      /* test for local awg */
   #ifdef _AWG_LOCAL
      if (AWG_ID (i, j) == _AWG_LOCAL_SLOT) {
         return addWaveformAWG (sl, comp, numComp);
      }
   #endif
   
      /* allocate memory */
      wforms.awgcomponent_list_r_len = numComp;
      wforms.awgcomponent_list_r_val = 
         calloc (numComp, sizeof (awgcomponent_r));
      if (wforms.awgcomponent_list_r_val == NULL) {
         return -2;
      }
   
      /* copy components */
      for (k = 0; k < numComp; k++) {
         wforms.awgcomponent_list_r_val[k].wtype = comp[k].wtype;
         for (l = 0; l < 4; l++) {
            wforms.awgcomponent_list_r_val[k].par[l] = comp[k].par[l];
         }
         wforms.awgcomponent_list_r_val[k].start = comp[k].start;
         wforms.awgcomponent_list_r_val[k].duration = comp[k].duration;
         wforms.awgcomponent_list_r_val[k].restart = comp[k].restart;
         for (l = 0; l < 2; l++) {
            wforms.awgcomponent_list_r_val[k].ramptime[l] = 
               comp[k].ramptime[l];
         }
         wforms.awgcomponent_list_r_val[k].ramptype = comp[k].ramptype;
         for (l = 0; l < 4; l++) {
            wforms.awgcomponent_list_r_val[k].ramppar[l] = 
               comp[k].ramppar[l];
         }
      }
   
      /* add waveforms */
      if (awgaddwaveform_1 (sl, &wforms, &ret, awg_clnt[i][j]) != 
         RPC_SUCCESS) {
         free (wforms.awgcomponent_list_r_val);
         return -2;
      }
      free (wforms.awgcomponent_list_r_val);
   
      /* return error code */
      if (ret >= 0) {
         return 0;
      }
      else {
         return ret - 2;
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: awgSetWaveform				*/
/*                                                         		*/
/* Procedure Description: set arbitrary waveform of an awg slot		*/
/*                                                         		*/
/* Procedure Arguments: slot, data array, length			*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 when failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int awgSetWaveform (int slot, float y[], int len)
   {
      int		status;	/* status flag */
      int 		ret;	/* return value */
      int		i;	/* index */
      int		j;	/* index */
      int		sl; 	/* slot within awg */
      awgwaveform_r	wform;	/* awg components */
   
      /* test whether awg interface is initialized */
      if (awg_init == 0) {
         status = awg_client();
         if (status < 0) {
            return status - 10;
         }
      }
   
      /* anything to do? */
      if (len < 0) {
         return 0;
      }
   
      /* use slot number to determine awg */
      i = slot / _AWG_IFO_OFS - 1;
      j = (slot % _AWG_IFO_OFS) / _AWG_NUM_OFS;
      sl = (slot % _AWG_IFO_OFS) % _AWG_NUM_OFS;
   
      /* treat DS340 separately */
      if ((i == _MAX_IFO) && (sl >= 0) && (sl < NUM_DS340) &&
         isDS340Alive (sl)) {
         return sendWaveDS340 (sl, y, len);
      }
   
      /* test parameters */
      if (!awgCheckInterface (i, j) || (sl < 0)) {
         return -1;
      }
   
      /* test for local awg */
   #ifdef _AWG_LOCAL
      if (AWG_ID (i, j) == _AWG_LOCAL_SLOT) {
         return setWaveformAWG (sl, y, len);
      }
   #endif
   
      /* set waveforms */
      wform.awgwaveform_r_len = len;
      wform.awgwaveform_r_val = y;
      if (awgsetwaveform_1 (sl, wform, &ret, awg_clnt[i][j]) != 
         RPC_SUCCESS) {
         return -2;
      }
   
      /* return error code */
      return ret;
   }



/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: awgSendWaveform				*/
/*                                                         		*/
/* Procedure Description: send stream to an awg slot			*/
/*                                                         		*/
/* Procedure Arguments: slot, time, epoch, data array, length		*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 when failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int awgSendWaveform (int slot, taisec_t time, int epoch,
                     float y[], int len)
   {
      int		status;	/* status flag */
      int 		ret;	/* return value */
      int		i;	/* index */
      int		j;	/* index */
      int		sl; 	/* slot within awg */
      awgwaveform_r	wform;	/* awg components */
   
      /* test whether awg interface is initialized */
      if (awg_init == 0) {
         status = awg_client();
         if (status < 0) {
            return -5;
         }
      }
   
      /* anything to do? */
      if (len <= 0) {
         return -2;
      }
   
      /* use slot number to determine awg */
      i = slot / _AWG_IFO_OFS - 1;
      j = (slot % _AWG_IFO_OFS) / _AWG_NUM_OFS;
      sl = (slot % _AWG_IFO_OFS) % _AWG_NUM_OFS;
   
      /* test parameters: DS340not supported */
      if (!awgCheckInterface (i, j) || (sl < 0)) {
         return -1;
      }
   
      /* test for local awg */
   #ifdef _AWG_LOCAL
      if (AWG_ID (i, j) == _AWG_LOCAL_SLOT) {
         return sendWaveformAWG (sl, time, epoch, y, len);
      }
   #endif
   
      /* set waveforms */
      wform.awgwaveform_r_len = len;
      wform.awgwaveform_r_val = y;
      if (awgsendwaveform_1 (sl, time, epoch, wform, 
         &ret, awg_clnt[i][j]) != RPC_SUCCESS) {
         return -5;
      }
   
      /* return error code */
      return ret;
   }



/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: awgStopWaveform				*/
/*                                                         		*/
/* Procedure Description: clears waveforms from an awg slot		*/
/*                                                         		*/
/* Procedure Arguments: slot						*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 when failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int awgStopWaveform (int slot, int terminate, tainsec_t time)
   {
      int		status;	/* status flag */
      int 		ret;	/* return value */
      int		i;	/* index */
      int		j;	/* index */
      int		sl; 	/* slot within awg */
   
      /* test whether awg interface is initialized */
      if (awg_init == 0) {
         status = awg_client();
         if (status < 0) {
            return status - 10;
         }
      }
   
      /* use slot number to determine awg */
      i = slot / _AWG_IFO_OFS - 1;
      j = (slot % _AWG_IFO_OFS) / _AWG_NUM_OFS;
      sl = (slot % _AWG_IFO_OFS) % _AWG_NUM_OFS;
   
      /* treat DS340 separately */
      if ((i == _MAX_IFO) && (sl >= 0) && (sl < NUM_DS340) &&
         isDS340Alive (sl)) {
         return awgClearWaveforms (slot);
      }
   
      /* test parameters */
      if (!awgCheckInterface (i, j) || (sl < 0)) {
         return -1;
      }
   
      /* test for local AWG */
   #ifdef _AWG_LOCAL
      if (AWG_ID (i, j) == _AWG_LOCAL_SLOT) {
         return stopWaveformAWG (sl, terminate, time);
      }
   #endif
   
      /* clear waveforms on remote AWG */
      if (awgstopwaveform_1 (sl, terminate, time, &ret, 
         awg_clnt[i][j]) != RPC_SUCCESS) {
         return -2;
      }
   
      /* return error code */
      if (ret >= 0) {
         return 0;
      }
      else {
         return ret - 2;
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: awgClearWaveforms				*/
/*                                                         		*/
/* Procedure Description: clears waveforms from an awg slot		*/
/*                                                         		*/
/* Procedure Arguments: slot						*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 when failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int awgClearWaveforms (int slot)
   {
      int		status;	/* status flag */
      int 		ret;	/* return value */
      int		i;	/* index */
      int		j;	/* index */
      int		sl; 	/* slot within awg */
   
      /* test whether awg interface is initialized */
      if (awg_init == 0) {
         status = awg_client();
         if (status < 0) {
            return status - 10;
         }
      }
   
      /* use slot number to determine awg */
      i = slot / _AWG_IFO_OFS - 1;
      j = (slot % _AWG_IFO_OFS) / _AWG_NUM_OFS;
      sl = (slot % _AWG_IFO_OFS) % _AWG_NUM_OFS;
   
      /* treat DS340 separately */
      if ((i == _MAX_IFO) && (sl >= 0) && (sl < NUM_DS340) &&
         isDS340Alive (sl)) {
         DS340_ConfigBlock	conf;
         getDS340 (sl, &conf);
         conf.ampl = 0;
         conf.offs = 0;
         setDS340 (sl, &conf);
         if (uploadDS340Wave (sl) < 0) {
            return -2;
         } 
         else {
            return 0;
         }
      }
   
      /* test parameters */
      if (!awgCheckInterface (i, j) || (sl < 0)) {
         return -1;
      }
   
      /* test for local AWG */
   #ifdef _AWG_LOCAL
      if (AWG_ID (i, j) == _AWG_LOCAL_SLOT) {
         return resetAWG (sl);
      }
   #endif
   
      /* clear waveforms on remote AWG */
      if (awgclearwaveforms_1 (sl, &ret, awg_clnt[i][j]) != 
         RPC_SUCCESS) {
         return -2;
      }
   
      /* return error code */
      if (ret >= 0) {
         return 0;
      }
      else {
         return ret - 2;
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: awgQueryWaveforms				*/
/*                                                         		*/
/* Procedure Description: queries an arbitrary waveform generator	*/
/*                                                         		*/
/* Procedure Arguments: slot id, pointer to result array, max length	*/
/*                                                         		*/
/* Procedure Returns: length of wave form list if successful, 		*/
/*		      <0 when failed					*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int awgQueryWaveforms (int slot, AWG_Component* comp, int maxComp)
   {
      int		status;	/* status flag */
      awgquerywaveforms_r 	ret;	/* return value */
      int		i;	/* index */
      int		j;	/* index */
      int		sl; 	/* slot within awg */
   
      /* test whether awg interface is initialized */
      if (awg_init == 0) {
         status = awg_client();
         if (status < 0) {
            return status - 10;
         }
      }
   
      /* use slot number to determine awg */
      i = slot / _AWG_IFO_OFS - 1;
      j = (slot % _AWG_IFO_OFS) / _AWG_NUM_OFS;
      sl = (slot % _AWG_IFO_OFS) % _AWG_NUM_OFS;
   
      /* treat DS340 separately */
      if ((i == _MAX_IFO) && (sl >= 0) && (sl < NUM_DS340) &&
         isDS340Alive (sl)) {
         DS340_ConfigBlock	conf;	/* DS340 configuration */
         AWG_WaveType 		wtype;	/* wave type */
      
         if (downloadDS340Block (sl) < 0) {
            return -2;
         }
         getDS340 (sl, &conf);
      	 /* determine waveform */
         if (conf.func == ds340_sin) {
            wtype = awgSine;
         }
         else if (conf.func == ds340_square) {
            wtype = awgSquare;
         }
         else if (conf.func == ds340_ramp) {
            wtype = awgSquare;
         }
         else if (conf.func == ds340_triangle) {
            wtype = awgSquare;
         }
         else if (conf.func == ds340_noise) {
            wtype = awgNoiseN;
         }
         else {
            return -3;
         }
      
      	 /* test for sweep */
         if ((conf.toggles & DS340_SWEN) != 0) {
            int		cnum;		/* number fo components */
            int		flag;		/* AWG component flag */
         
            /* test sweep directions */
            if ((conf.toggles & DS340_SDIR) == 0) {
               if (maxComp < 1) {
                  return 1;
               }
               flag = 0;
            }
            else {
               if (maxComp < 2) {
                  return 2;
               }
               flag = AWG_SWEEP_CYCLE;
            }
            /* test sweep type */
            if ((conf.toggles & DS340_STYP) != 0) {
               flag |= AWG_SWEEP_LOG;
            }
            if (conf.srat < 1E-6) {
               return -3;
            }
            /* calculate sweep parameters */
            if (awgSweepComponents (TAInow(), _ONESEC/conf.srat, conf.stfr,
               conf.spfr, conf.ampl, conf.ampl, flag, comp, &cnum) < 0) {
               return -3;
            }
            else {
               return cnum;
            }
         }
         /* periodic or noise */
         else {
            if (maxComp < 1) {
               return 1;
            }
            memset (comp, 0, sizeof (AWG_Component));
            comp->start = TAInow();
            comp->duration = -1;
            comp->restart = -1;
         
            /* noise first */
            if (wtype == awgNoiseU) {
               comp->wtype = wtype;
               comp->par[0] = conf.ampl;
               comp->par[1] = 0;
               comp->par[2] = 10E6;
               comp->par[3] = conf.offs;
               return 1;
            }
            /* periodic */
            if (awgPeriodicComponent (wtype, conf.freq, conf.ampl, 0, 
               conf.offs, comp) < 0) {
               return -3;
            }
            else {
               return 1;
            }
         }
      } 
      /* only non DS340 channels */
   
      /* test parameters */
      if (!awgCheckInterface (i, j) || (sl < 0)) {
         return -1;
      }
   
      /* test for local awg */
   #ifdef _AWG_LOCAL
      if (AWG_ID (i, j) == _AWG_LOCAL_SLOT) {
         return queryWaveformAWG (sl, comp, maxComp);
      }
   #endif
   
      /* query awg */
      memset (&ret, 0, sizeof (awgquerywaveforms_r));
      if ((awgquerywaveforms_1 (sl, maxComp, &ret, awg_clnt[i][j]) != 
         RPC_SUCCESS) || (ret.status < 0)) {
         return -2;
      }
   
      /* copy result */
      for (i = 0; (i < maxComp) && 
          (i < ret.wforms.awgcomponent_list_r_len); i++) {
         comp[i].wtype = 
            ret.wforms.awgcomponent_list_r_val[i].wtype;
         for (j = 0; j < 4; j++) {
            comp[i].par[j] = 
               ret.wforms.awgcomponent_list_r_val[i].par[j];
         }
         comp[i].start = 
            ret.wforms.awgcomponent_list_r_val[i].start;
         comp[i].duration = 
            ret.wforms.awgcomponent_list_r_val[i].duration;
         comp[i].restart = 
            ret.wforms.awgcomponent_list_r_val[i].restart;
         for (j = 0; j < 2; j++) {
            comp[i].ramptime[j] = 
               ret.wforms.awgcomponent_list_r_val[i].ramptime[j];
         }
         comp[i].ramptype = 
            ret.wforms.awgcomponent_list_r_val[i].ramptype;
         for (j = 0; j < 4; j++) {
            comp[i].ramppar[j] = 
               ret.wforms.awgcomponent_list_r_val[i].ramppar[j];
         }
      }
      /* free memory of return array */
      xdr_free ((xdrproc_t)xdr_awgquerywaveforms_r, (char*) &ret);
   
      /* return length of returned waveform list */
      return ret.wforms.awgcomponent_list_r_len;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: awgSetGain					*/
/*                                                         		*/
/* Procedure Description: set the gain of an AWG slot			*/
/*                                                         		*/
/* Procedure Arguments: slot, gain, ramp time				*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 when failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int awgSetGain (int slot, double gain, tainsec_t time)
   {
      int		status;	/* status flag */
      int 		ret;	/* return value */
      int		i;	/* index */
      int		j;	/* index */
      int		sl; 	/* slot within awg */
   
      /* test whether awg interface is initialized */
      if (awg_init == 0) {
         status = awg_client();
         if (status < 0) {
            return -5;
         }
      }
   
      /* use slot number to determine awg */
      i = slot / _AWG_IFO_OFS - 1;
      j = (slot % _AWG_IFO_OFS) / _AWG_NUM_OFS;
      sl = (slot % _AWG_IFO_OFS) % _AWG_NUM_OFS;
   
      /* test parameters: DS340not supported */
      if (!awgCheckInterface (i, j) || (sl < 0)) {
         return -1;
      }
   
      /* test for local awg */
   #ifdef _AWG_LOCAL
      if (AWG_ID (i, j) == _AWG_LOCAL_SLOT) {
         return setGainAWG (sl, gain, time);
      }
   #endif
   
      /* set waveforms */
      if (awgsetgain_1 (sl, gain, time, &ret, 
         awg_clnt[i][j]) != RPC_SUCCESS) {
         return -5;
      }
   
      /* return error code */
      return ret;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: awgSetFilter				*/
/*                                                         		*/
/* Procedure Description: set a filter					*/
/*                                                         		*/
/* Procedure Arguments: slot, coeff, len				*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 when failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int awgSetFilter (int slot, double y[], int len)
   {
      int		status;	/* status flag */
      int 		ret;	/* return value */
      int		i;	/* index */
      int		j;	/* index */
      int		sl; 	/* slot within awg */
      awgfilter_r	coeff;	/* awg components */
   
      /* test whether awg interface is initialized */
      if (awg_init == 0) {
         status = awg_client();
         if (status < 0) {
            return -5;
         }
      }
   
      /* anything to do? */
      if ((len < 0) || !y) {
         return -2;
      }
   
      /* use slot number to determine awg */
      i = slot / _AWG_IFO_OFS - 1;
      j = (slot % _AWG_IFO_OFS) / _AWG_NUM_OFS;
      sl = (slot % _AWG_IFO_OFS) % _AWG_NUM_OFS;
   
      /* test parameters: DS340not supported */
      if (!awgCheckInterface (i, j) || (sl < 0)) {
         return -1;
      }
   
      /* test for local awg */
   #ifdef _AWG_LOCAL
      if (AWG_ID (i, j) == _AWG_LOCAL_SLOT) {
         return setFilterAWG (sl, y, len);
      }
   #endif
   
      /* set waveforms */
      coeff.awgfilter_r_len = len;
      coeff.awgfilter_r_val = y;
      if (awgsetfilter_1 (sl, coeff, &ret, 
         awg_clnt[i][j]) != RPC_SUCCESS) {
         return -5;
      }
   
      /* return error code */
      return ret;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: awgReset					*/
/*                                                         		*/
/* Procedure Description: resets arbitrary waveform generators		*/
/*                                                         		*/
/* Procedure Arguments: awg id						*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 when failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int awgReset (int id)
   {
      int		status;	/* status flag */
      int 		ret;	/* return value */
      int		i;	/* index */
      int		j;	/* index */
   
      /* test whether awg interface is initialized */
      if (awg_init == 0) {
         status = awg_client();
         if (status < 0) {
            return status;
         }
      }
   
      ret = 0;
      if (id == -1) {
         /* reset all awg's */
         for (i = 0; i < _MAX_IFO; i++) {
            for (j = 0; j < _MAX_AWG_PER_IFO; j++) {
               if (awgCheckInterface (i, j)) {
                  /* test for local AWG */
               #ifdef _AWG_LOCAL
                  if (AWG_ID (i, j) == _AWG_LOCAL_SLOT) {
                     ret = releaseIndexAWG (-1);
                  }
                  else {
                  #endif
                  /* reset on remote AWG */
                     if ((awgreset_1 (&status, awg_clnt[i][j]) !=
                        RPC_SUCCESS) || (status != 0)) {
                        ret = -1;
                     }
                  #ifdef _AWG_LOCAL
                  }
               	#endif
               }
            }
         }
      	 /* reset all DS340's */
         resetDS340 (-1);
      }
      else if (id < 0) {
         /* reset all awg's of an ifo */
         i = (-id) / _AWG_IFO_OFS - 1;
      	 /* treat DS340 separately */
         if (i == _MAX_IFO) {
            resetDS340 (-1);
         }
         else if ((i >= 0) && (i < _MAX_IFO)) {
            for (j = 0; j < _MAX_AWG_PER_IFO; j++) {
               if (awgCheckInterface (i, j)) {
                  /* test for local AWG */
               #ifdef _AWG_LOCAL
                  if (AWG_ID (i, j) == _AWG_LOCAL_SLOT) {
                     ret = releaseIndexAWG (-1);
                  }
                  else {
                  #endif
                  /* reset on remote AWG */
                     if ((awgreset_1 (&status, awg_clnt[i][j]) !=
                        RPC_SUCCESS) || (status != 0)) {
                        ret = -1;
                     }
                  #ifdef _AWG_LOCAL
                  }
               	#endif
               }
            }
         }
      }
      else {
         /* reset one specific ifo */
         i = id / _AWG_IFO_OFS - 1;
         j = (id % _AWG_IFO_OFS) / _AWG_NUM_OFS;
      	 /* treat DS340 separately */
         if (i == _MAX_IFO) {
            resetDS340 (j);
         }
         /* do awg's */
         else if (awgCheckInterface (i, j)) {
            /* test for local AWG */
         #ifdef _AWG_LOCAL
            if (AWG_ID (i, j) == _AWG_LOCAL_SLOT) {
               ret = releaseIndexAWG (-1);
            }
            else {
            #endif
            /* reset on remote AWG */
               if ((awgreset_1 (&status, awg_clnt[i][j]) !=
                  RPC_SUCCESS) || (status != 0)) {
                  ret = -1;
               }
            #ifdef _AWG_LOCAL
            }
         #endif
         }
      }
   
      return ret;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: awgStatistics				*/
/*                                                         		*/
/* Procedure Description: obtains statistics data from an AWG		*/
/*                                                         		*/
/* Procedure Arguments: awg id, pointer to stat. data			*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 when failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int awgStatistics (int slot, awgStat_t* stat)
   {
      int		status;	/* status flag */
      awgstat_r 	ret;	/* return value */
      int		i;	/* index */
      int		j;	/* index */
      int		sl; 	/* slot within awg */
   
      /* test whether awg interface is initialized */
      if (awg_init == 0) {
         status = awg_client();
         if (status < 0) {
            return status - 10;
         }
      }
   
      /* use slot number to determine awg */
      i = slot / _AWG_IFO_OFS - 1;
      j = (slot % _AWG_IFO_OFS) / _AWG_NUM_OFS;
      sl = (slot % _AWG_IFO_OFS) % _AWG_NUM_OFS;
      if (!awgCheckInterface (i, j) || (sl < 0)) {
         return -1;
      }
   
      /* test for local awg */
   #ifdef _AWG_LOCAL
      if (AWG_ID (i, j) == _AWG_LOCAL_SLOT) {
         return getStatisticsAWG (stat);
      }
   #endif
   
      /* query awg */
      memset (&ret, 0, sizeof (awgstat_r));
      if ((awgstatistics_1 (stat == NULL, &ret, 
         awg_clnt[i][j]) != RPC_SUCCESS) || (ret.status < 0)) {
         return -2;
      }
   
      /* copy result; do it the dumb way */
      if (stat != NULL) {
         memcpy (stat, &ret, sizeof (awgStat_t));
      }
      /* free memory of return array */
      xdr_free ((xdrproc_t)xdr_awgstat_r, (char*) &ret);
   
      /* return */
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: awgShow					*/
/*                                                         		*/
/* Procedure Description: returns config. string of AWG			*/
/*                                                         		*/
/* Procedure Arguments: awg id						*/
/*                                                         		*/
/* Procedure Returns: string if successful, NULL when failed		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   char* awgShow (int id)
   {
      int		status;	/* status flag */
      awgshow_r 	ret;	/* return value */
      int		i;	/* index */
      int		j;	/* index */
      int		sl; 	/* slot within awg */
      char*		p;	/* show buffer */
   
      /* test whether awg interface is initialized */
      if (awg_init == 0) {
         status = awg_client();
         if (status < 0) {
            return NULL;
         }
      }
   
      /* use slot number to determine awg */
      i = abs(id) / _AWG_IFO_OFS - 1;
      j = (abs(id) % _AWG_IFO_OFS) / _AWG_NUM_OFS;
      sl = (abs(id) % _AWG_IFO_OFS) % _AWG_NUM_OFS;
   
      /* treat DS340 separately */
      if (i == _MAX_IFO) {
         /* alloc memory */
         p = malloc (_SHOWBUF_SIZE);
         if (p == NULL) {
            return NULL;
         }
         strcpy (p, "Only connected DSG channels are shown\n");
         j = strlen (p);
      	 /* query ds340 */
         for (i = 0; i < NUM_DS340; i++) {
            if ((isDS340Alive (i)) && 
               (strlen (p) + 100 < _SHOWBUF_SIZE)) {
               sprintf (strend (p), 
                       "\n=== Digital signal generator %i @ %s/%i===\n", 
                       i, ds340addr[i], ds340port[i]);
               downloadDS340Block (i);
               showDS340Block (i, strend (p), _SHOWBUF_SIZE-strlen(p)-1);
            }
         }
         if (strlen (p) == j) {
            strcpy (p, "No DSG channels connected\n");
         }
         /* return */
         if (strlen (p) < _SHOWBUF_SIZE - 1) {
            p = realloc (p, strlen (p) + 1);
         }
         return p;
      }
   
      /* test parameters */
      if (!awgCheckInterface (i, j) || (sl < 0)) {
         return NULL;
      }
   
      /* test for local awg */
   #ifdef _AWG_LOCAL
      if (AWG_ID (i, j) == _AWG_LOCAL_SLOT) {
         p = malloc (_SHOWBUF_SIZE);
         if (p == NULL) {
            return NULL;
         }
         if (id < 0) {
            if (showAWG (sl, p, _SHOWBUF_SIZE) < 0) {
               free (p);
               return NULL;
            }
         }
         else {
            if (showAllAWGs (p, _SHOWBUF_SIZE) == NULL) {
               free (p);
               return NULL;
            }
         }
         if (strlen (p) < _SHOWBUF_SIZE - 1) {
            p = realloc (p, strlen (p) + 1);
         }
         return p;
      }
   #endif
   
      /* query awg */
      memset (&ret, 0, sizeof (awgshow_r));
      if (id < 0) {
         if ((awgshowslot_1 (sl, &ret, awg_clnt[i][j]) != RPC_SUCCESS) || 
            (ret.status < 0)) {
            xdr_free ((xdrproc_t)xdr_awgshow_r, (char*) &ret);
            return NULL;
         }
      }
      else {
         if ((awgshow_1 (&ret, awg_clnt[i][j]) != RPC_SUCCESS) || 
            (ret.status < 0)) {
            xdr_free ((xdrproc_t)xdr_awgshow_r, (char*) &ret);
            return NULL;
         }
      }
   
      /* return */
      return ret.res;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: awg_client					*/
/*                                                         		*/
/* Procedure Description: installs awg client interface			*/
/*                                                         		*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: # of AWGs if successful, <0 if failed		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/           
   int awg_client (void)
   {
      int		i;		/* index */
      int		j;		/* index */
      int		awgnum;		/* number of AWGs */
   #if defined (_CONFIG_DYNAMIC)
      const char* const* cinfo;		/* configuration info */
      confinfo_t	crec;		/* conf. info record */
   #endif
   
      /* test whether awg interface is initialized */
      if (awg_init != 0) {
         awgnum = 0;
         for (i = 0; i < _MAX_IFO; i++) {
            for (j = 0; j < _MAX_AWG_PER_IFO; j++) {
               if (awg_clnt[i][j] != NULL) {
                  awgnum++;
               }
            }
         }
         for (i = 0; i < NUM_DS340; i++) {
            if ((strlen (ds340addr[i]) > 0) &&
               (ds340port[i] > 0)) {
               awgnum++;
            }
         }
         return awgnum;
      }
      else {
         /* dynamic configuration */
      #if defined (_CONFIG_DYNAMIC)
         for (cinfo = getConfInfo (0, 0); *cinfo != NULL; cinfo++) {
            if ((parseConfInfo (*cinfo, &crec) == 0) &&
               (gds_strcasecmp (crec.interface, 
                                CONFIG_SERVICE_AWG) == 0)) {
               if ((crec.ifo >= 0) && (crec.ifo < _MAX_IFO) &&
                  (crec.num >= 0) && (crec.num < _MAX_AWG_PER_IFO) &&
                  (crec.port_prognum > 0) && (crec.progver > 0)) {
                  awgSetHostAddress (crec.ifo, crec.num, crec.host, 
                                    crec.port_prognum, crec.progver);
               }
               else if ((crec.ifo == -1) &&
                       (crec.num >= 0) && (crec.num < NUM_DS340) &&
                       (crec.port_prognum > 0) && (crec.progver == -1)) {
                  ds340SetHostAddress (crec.num, crec.host, 
                                      crec.port_prognum);
               }
            }
         }
      #endif
      
         /* init clients */
         return initAWGclient();
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: awg_cleanup					*/
/*                                                         		*/
/* Procedure Description: terminates awg client interface		*/
/*                                                         		*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/ 
   void awg_cleanup (void)
   {
      int		i;		/* index */
      int		j;		/* index */
   
      if (awg_init == 0) {
         return;
      }
   
      for (i = 0; i < _MAX_IFO; i++) {
         for (j = 0; j < _MAX_AWG_PER_IFO; j++) {
            if (awg_clnt[i][j] != NULL) {
               clnt_destroy (awg_clnt[i][j]);
               awg_clnt[i][j] = NULL;
            }
         }
      
      }
   
      awg_init = 0;
   }


#if defined (_AWG_LIB) || defined (_CONFIG_DYNAMIC)
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: awgSetHostAddress				*/
/*                                                         		*/
/* Procedure Description: sets the host adddr of the excitation engine	*/
/*                                                         		*/
/* Procedure Arguments: ifo id, awg id, hostname, prog #, ver #		*/
/*                                                         		*/
/* Procedure Returns: 0 if successful					*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int awgSetHostAddress (int ifo, int awg, const char* hostname, 
                     unsigned long prognum, unsigned long progver)
   {   
      if ((awg_init > 0) || (ifo < 0) || (ifo >= _MAX_IFO) ||
         (awg < 0) || (awg >= _MAX_AWG_PER_IFO)) {
         return -1;
      }
      /* set node parameters */
      awgHost[ifo][awg].valid = 1;
      strncpy (awgHost[ifo][awg].hostname, hostname, 
              sizeof (awgHost[ifo][awg].hostname));
      awgHost[ifo][awg].hostname[sizeof (awgHost[ifo][awg].hostname)-1] = 0;
      awgHost[ifo][awg].prognum = (prognum > 0) ? 
                           prognum : RPC_PROGNUM_AWG;
      awgHost[ifo][awg].progver = (progver > 0) ? 
                           progver : RPC_PROGVER_AWG;
      return 0;
   }


   int ds340SetHostAddress (int ds340, const char* hostname, int port)
   {
      if ((awg_init > 0) || (ds340 < 0) || (ds340 >= NUM_DS340)) {
         return -1;
      }
      strncpy (ds340addr[ds340], hostname, 100);
      ds340addr[ds340][99] = 0;
      ds340port[ds340] = port;
      return 0;
   }
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: initAWGclient				*/
/*                                                         		*/
/* Procedure Description: init. the AWG interface			*/
/*                                                         		*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: # of AWGs if successful, <0 when failed		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int initAWGclient (void) 
   {
      int		i;		/* index */
      int		j;		/* index */
      char		prms[80];	/* section heading */
      struct in_addr 	addr;		/* host address */
      char 		remotehost[PARAM_ENTRY_LEN];/* remote hostname */
      char		hostname[30];	/* ip name */
      unsigned long	prognum;	/* rpc prog. num. */
      unsigned long	progver;	/* rpc prog. ver. */
      struct timeval	timeout;	/* timeout for establishing conn. */
      int		awgnum;		/* number of AWGs */
   #if !defined (_AWG_LIB) && !defined (_CONFIG_DYNAMIC)
      FILE*		fd;		/* file descriptor */
      int		port;		/* cobox port number */
   #endif
   
      /* initialize rpc for VxWorks */
   #ifdef OS_VXWORKS
      rpcTaskInit ();
   #endif
   
      /* initialze awg number */
      awgnum = 0;
      /* create client handles */
      for (i = 0; i < _MAX_IFO; i++) {
      #if !defined (_AWG_LIB) && !defined (_CONFIG_DYNAMIC)
      	 /* check whether file exists */ 
         fd = fopen (PRM_FILE, "r");
         if (fd == NULL) {
            /* if file doesn't exist, set all handles to NULL */
            for (j = 0; j < _MAX_AWG_PER_IFO; j++) {
               awg_clnt[i][j] = NULL;
            }
            continue;
         }
         fclose (fd);
      #endif
      	 /* if file exists, read in parameters */
         for (j = 0; j < _MAX_AWG_PER_IFO; j++) {
            awg_clnt[i][j] = NULL;
         #if !defined (_AWG_LIB) && !defined (_CONFIG_DYNAMIC)
            /* construct section heading */
            sprintf (prms, "%s%i-%s%i", SITE_PREFIX, i + 1, PRM_SECTION, j);
            /* get remote host from parameter file */
            strcpy (remotehost, "");
            loadStringParam (PRM_FILE, prms, PRM_ENTRY1, remotehost);
            if (strcmp (remotehost, "") == 0) {
               continue;
            }
            /* get rpc parameters from parameter file */
            prognum = RPC_PROGNUM_AWG;
            if (loadNumParam (PRM_FILE, prms, PRM_ENTRY2, &prognum) < 0) {
               continue;
            }
            progver = RPC_PROGVER_AWG;
            if (loadNumParam (PRM_FILE, prms, PRM_ENTRY3, &progver) < 0) {
               continue;
            }
         #else
            if (!awgHost[i][j].valid) {
               continue;
            }
            strncpy (remotehost, awgHost[i][j].hostname, 
                    sizeof (remotehost));
            remotehost[sizeof(remotehost)-1] = 0;
            prognum = awgHost[i][j].prognum;
            progver = awgHost[i][j].progver;
         #endif
         
            /* check validity of host name */
            if (rpcGetHostaddress (remotehost, &addr) != 0) {
               continue;
            }
         
            /* create an rpc client handle */
         #ifdef OS_VXWORKS
            inet_ntoa_b (addr, hostname);
         #else
            strncpy (hostname, inet_ntoa (addr), sizeof (hostname));
            hostname[sizeof(hostname)-1] = 0;
         #endif
            timeout.tv_sec = RPC_PROBE_WAIT;
            timeout.tv_usec = 0;
         #ifndef OS_VXWORKS
            rpcProbe (hostname, prognum, progver, _NETID, &timeout, 
                     &awg_clnt[i][j]);
         #endif
            if (awg_clnt[i][j] != NULL) {
               awgnum++;
               sprintf (prms, "rpc client for awg %i.%i created", 
                       i, j);
               gdsDebug (prms);
            }
            else {
               sprintf (prms, "rpc client for awg %i.%i failed", 
                       i, j);
               gdsError (GDS_ERR_PROG, prms);
            }
         }
      }
   
      /* initialize cobox */
   #if !defined (_AWG_LIB) && !defined (_CONFIG_DYNAMIC)
      memset (ds340addr, 0, sizeof (ds340addr));
      memset (ds340port, 0, sizeof (ds340port));
      /* check whether file exists */ 
      fd = fopen (PRM_FILE, "r");
      if (fd != NULL) {
         fclose (fd);
      
         for (i = 0; i < NUM_DS340; i++) {
            /* if file exists, read in parameters */
            /* construct section heading */
            sprintf (prms, "%s-%s%i", SITE_PREFIX, PRM_SECTION2, i);
            /* get remote host from parameter file */
            strcpy (remotehost, "");
            loadStringParam (PRM_FILE, prms, PRM_ENTRY1, remotehost);
            if (strcmp (remotehost, "") == 0) {
               continue;
            }
            /* get serial port number from parameter file */
            port = 0;
            if (loadIntParam (PRM_FILE, prms, PRM_ENTRY4, &port) < 0) {
               continue;
            }
            if (port <= 0) {
               continue;
            }
            strncpy (ds340addr[i], remotehost, 100);
            ds340addr[i][99] = 0;
            ds340port[i] = port;
            /* printf ("%i:host = %s; port = %i\n", i, ds340addr[i],
                   ds340port[i]);*/
            awgnum++;
         }
      }
   #else
      for (i = 0; i < NUM_DS340; i++) {
         if (strlen (ds340addr[i]) > 0) {
            /* printf ("%i:host = %s; port = %i\n", i, ds340addr[i],
                   ds340port[i]);*/
            awgnum++;
         }
      }
   #endif
   
      /* initialization successful */
      awg_init = 1;
      return awgnum;
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
         char* p = (char*) malloc (strlen (m) + 1);
         if (p != 0) {
            strcpy (p, m);
         }
         return p;
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: awgWaveformCmd				*/
/*                                                         		*/
/* Procedure Description: returns awg components from string command	*/
/*                                                         		*/
/* Procedure Arguments: command string, awg component array (return, 	*/
/*    max 2 elements), number of elements returned, error message,	*/
/*    pointer to point array (return for arb awg), length of poin array	*/
/*    boolean for DS340							*/
/*                                                         		*/
/* Procedure Returns: 0 if successful; <0 on error			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/           
   int awgWaveformCmd (const char* cmd, AWG_Component comp[], int* cnum,
                     char** errmsg, float** points, int* num, int isDS340)
   {
      static tainsec_t oldTimeStamp = 0;
      tainsec_t timeStamp;
      const char*	p;	/* cursor into command string */
   
      if (errmsg != NULL) {
         *errmsg = NULL;
      }
      if (points != NULL) {
         *points = NULL;
      }
   
      /* check parameters */
      if ((cmd == 0) || (comp == 0) || (cnum == 0) || 
         (num == 0) || (points == 0)) {
         if (errmsg != 0)  {
            *errmsg = cmdreply ("error: invalid arguments");
         }
         return -1;
      }
      *num = 0;
      *cnum = 1;
   
      /* skip blanks */
      p = cmd;
      while ((*p == ' ') || (*p == '\t')) {
         p++;
      }
      /* time stamp */
      timeStamp = TAInow();
      if (timeStamp - oldTimeStamp < _EPOCH / 10) {
         timeStamp = oldTimeStamp;
      }
      else {
         oldTimeStamp = timeStamp;
      }
      timeStamp += 4 * _EPOCH;
   
      /* determine waveform */
      if (strlen (p) == 0) {
         /* no waveform means clear */
         return 0;
      }
      /* sine */
      else if (gds_strncasecmp (p, "sine", 4) == 0) {
         double 		f, a, o, q;
         if ((sscanf (p + 4, "%lf%lf%lf%lf", &f, &a, &o, &q) != 4) ||
            (awgPeriodicComponentEx (awgSine, timeStamp, 
            f, a, q, o, comp) < 0)) {
            if (errmsg != NULL) {
               *errmsg = cmdreply ("error: invalid arguments");
            }
            return -2;
         }
      }
      
      /* square */
      else if (gds_strncasecmp (p, "square", 6) == 0) {
         double 		f, a, o, q, r;
         int			num;
         num = sscanf (p + 6, "%lf%lf%lf%lf%lf", &f, &a, &o, &q, &r);
         if (num == 4) {
            if (awgPeriodicComponentEx (awgSquare, timeStamp,
               f, a, q, o, comp) < 0) {
               if (errmsg != NULL) {
                  *errmsg = cmdreply ("error: invalid arguments");
               }
               return -3;
            }
         }
         else if (num == 5) {
            if (awgSquareWaveComponentEx (timeStamp, 
               f, a, q, o, r, comp) < 0) {
               if (errmsg != NULL) {
                  *errmsg = cmdreply ("error: invalid arguments");
               }
               return -3;
            }
            *cnum = 2;
         }
         else {
            if (errmsg != NULL) {
               *errmsg = cmdreply ("error: invalid arguments");
            }
            return -3;
         }
      }
      
      /* ramp */
      else if (gds_strncasecmp (p, "ramp", 4) == 0) {
         double 		f, a, o, q;
         if ((sscanf (p + 4, "%lf%lf%lf%lf", &f, &a, &o, &q) != 4) ||
            (awgPeriodicComponentEx (awgRamp, timeStamp,
            f, a, q, o, comp) < 0)) {
            if (errmsg != NULL) {
               *errmsg = cmdreply ("error: invalid arguments");
            }
            return -4;
         }
      }
      
      /* triangle */
      else if (gds_strncasecmp (p, "triangle", 8) == 0) {
         double 		f, a, o, q;
         if ((sscanf (p + 8, "%lf%lf%lf%lf", &f, &a, &o, &q) != 4) ||
            (awgPeriodicComponentEx (awgTriangle, timeStamp,
            f, a, q, o, comp) < 0)) {
            if (errmsg != NULL) {
               *errmsg = cmdreply ("error: invalid arguments");
            }
            return -5;
         }
      }
      
      /* impulse */
      else if (gds_strncasecmp (p, "impulse", 7) == 0) {
         double 		f, a, du, de;
         if ((sscanf (p + 7, "%lf%lf%lf%lf", &f, &a, &du, &de) != 4) ||
            (awgPeriodicComponentEx (awgImpulse, timeStamp,
            f, a, du, de, comp) < 0)) {
            if (errmsg != NULL) {
               *errmsg = cmdreply ("error: invalid arguments");
            }
            return -6;
         }
      }
      
      /* constant offset */
      else if (gds_strncasecmp (p, "const", 5) == 0) {
         double 		a;
         if ((sscanf (p + 5, "%lf", &a) != 1) ||
            (awgConstantComponentEx (timeStamp, a, comp) < 0)) {
            if (errmsg != NULL) {
               *errmsg = cmdreply ("error: invalid arguments");
            }
            return -7;
         }
      }
      
      /* normally distributed noise */
      else if (gds_strncasecmp (p, "normal", 6) == 0) {
         double 		f1, f2, a, o;
         if ((sscanf (p + 6, "%lf%lf%lf%lf", &f1, &f2, &a, &o) != 4) ||
            (awgNoiseComponentEx (awgNoiseN, timeStamp,
            f1, f2, a, o, comp) < 0)) {
            if (errmsg != NULL) {
               *errmsg = cmdreply ("error: invalid arguments");
            }
            return -8;
         }
      }
      
      /* uniformly distributed noise */
      else if (gds_strncasecmp (p, "uniform", 7) == 0) {
         double 		f1, f2, a, o;
         if ((sscanf (p + 7, "%lf%lf%lf%lf", &f1, &f2, &a, &o) != 4) ||
            (awgNoiseComponentEx (awgNoiseU, timeStamp,
            f1, f2, a, o, comp) < 0)) {
            if (errmsg != NULL) {
               *errmsg = cmdreply ("error: invalid arguments");
            }
            return -9;
         }
      }
      
      /* frequency/amplitude sweep */
      else if (gds_strncasecmp (p, "sweep", 5) == 0) {
         double		f1, f2, a1, a2, dt, ft;
         char		stype[256];
         char		dir = 0;
         int		sFlag = 0;
      
         /* scan sweep parameters */
         if (sscanf (p + 5, "%lf%lf%lf%lf%lf %s %c", &f1, &f2, &a1, 
            &a2, &dt, stype, &dir) < 6) {
            if (errmsg != NULL) {
               *errmsg = cmdreply ("error: invalid arguments");
            }
            return -10;
         }
         if (gds_strncasecmp (stype, "log", 3) == 0) {
            sFlag |= AWG_SWEEP_LOG;
         }
         f1 = fabs (f1); f2 = fabs (f2);
         a1 = fabs (a1); a2 = fabs (a2);
         dt = fabs (dt);
         ft = (f1 < f2) ? f1 : f2;
         if (ft < 1E-6) {
            if (errmsg != NULL) {
               *errmsg = cmdreply ("error: invalid arguments");
            }
            return -11;
         }
         if (dir == '-') {
            ft = f1; f1 = f2; f2 = ft;
            ft = a1; a1 = a2; a2 = ft;
         }
         else if (dir != '+') {
            sFlag |= AWG_SWEEP_CYCLE;
            dt *= 2;
         }
         /* calculate sweep parameters */
         if (awgSweepComponents (timeStamp, 
            dt * (double) _ONESEC, f1, f2, 
            a1, a2, sFlag, comp, cnum) < 0) {
            if (errmsg != NULL) {
               *errmsg = cmdreply ("error: invalid arguments");
            }
            return -12;
         }
      }
      
      /* arbitrary waveform */
      else if (gds_strncasecmp (p, "arb", 3) == 0) {
         /* scan arb waveform parameter */
         double		f;	/* sampling frequency */
         double		scale;	/* scaling factor */
         char		trig;	/* trigger type */
         double		trigval;/* trigger value */
         double		rate;	/* trigger interval */
         float		point;	/* data point */
         char*		tok;	/* token */
         char*		lasts;	/* temp */
         int		len = 0;/* array length */
         float*		y;	/* data array */
         float		max;	/* max of data array */
         char*		cmdcopy;/* copy of command string */
      
         cmdcopy = cmdreply (p + 3);
         if (cmdcopy == 0) {
            if (errmsg != NULL) {
               *errmsg = cmdreply ("error: not enough memory");
            }
            return -13;
         }
      
         /* read sampling frequency */
         if (((tok = strtok_r (cmdcopy, " ", &lasts)) == NULL) ||
            (sscanf (tok, "%lf", &f) != 1) || (f <= 0)) {
            if (errmsg != NULL) {
               *errmsg = cmdreply ("error: invalid arguments");
            }
            free (cmdcopy);
            return -14;
         }
         /* read scaling factor */
         if (((tok = strtok_r (NULL, " ", &lasts)) == NULL) ||
            (sscanf (tok, "%lf", &scale) != 1)) {
            if (errmsg != NULL) {
               *errmsg = cmdreply ("error: invalid arguments");
            }
            free (cmdcopy);
            return -15;
         }
         /* read trigger type */
         if (((tok = strtok_r (NULL, " ", &lasts)) == NULL) ||
            (sscanf (tok, "%c", &trig) != 1) ||
            ((tolower (trig) != 'c') && (tolower (trig) != 'w') && 
            (tolower (trig) != 't') && (tolower (trig) != 'r'))) {
            if (errmsg != NULL) {
               *errmsg = cmdreply ("error: invalid arguments");
            }
            free (cmdcopy);
            return -16;
         }
         if (tolower (trig) == 't') {
            trigval = 3;
         }
         else if (tolower (trig) == 'w') {
            trigval = 2;
         }
         else if (tolower (trig) == 'r') {
            trigval = 1;
         }
         else {
            trigval = 0;
         }
         /* read interval value */
         if (((tok = strtok_r (NULL, " ", &lasts)) == NULL) ||
            (sscanf (tok, "%lf", &rate) != 1) || (rate < 0)) {
            if (errmsg != NULL) {
               *errmsg = cmdreply ("error: invalid arguments");
            }
            free (cmdcopy);
            return -17;
         }
      
         y = malloc (10000 * sizeof (float));
         if (y == NULL) {
            if (errmsg != NULL) {
               *errmsg = cmdreply ("error: not enough memory");
            }
            free (cmdcopy);
            return -18;
         }
         max = 0;
         while (((tok = strtok_r (NULL, " ", &lasts)) != NULL) &&
               (sscanf (tok, "%f", &point) == 1)) {
            y [len++] = point;
            if (fabs (point) > max) {
               max = fabs (point);
            }
            if (len % 10000 == 0) {
               y = realloc (y, (len + 10000) * sizeof (float));
               if (y == NULL) {
                  if (errmsg != NULL) {
                     *errmsg = cmdreply ("error: not enough memory");
                  }
                  free (cmdcopy);
                  return -19;
               } 
            }
         }
         /* only DS340 needs amplitude separately */
         if (!isDS340) {
            max = 1;
         }
         if (awgPeriodicComponentEx (awgArb, timeStamp,
            f, scale, rate, trigval, 
            comp) < 0) {
            free (y);
            if (errmsg != NULL) {
               *errmsg = cmdreply ("error: unable to download waveform");
            }
            free (cmdcopy);
            return -20;
         }
         *points = y;
         *num = len;
         free (cmdcopy);
      }
      
      /* waveform stream */
      else if (gds_strncasecmp (p, "stream", 6) == 0) {
         double 		a;
         if ((sscanf (p + 6, "%lf", &a) != 1) ||
            (awgStreamComponentEx (timeStamp, a, comp) < 0)) {
            if (errmsg != NULL) {
               *errmsg = cmdreply ("error: invalid arguments");
            }
            return -7;
         }
      }
      
      /* unrecognized waveform */
      else {
         if (errmsg != NULL) {
            *errmsg = cmdreply ("error: unrecognized waveform");
         }
         return -21;
      }
   
      /* check waveform components */
      if (!awgIsValidComponent (comp) ||
         ((*cnum == 2) && !awgIsValidComponent (comp + 1))) {
         if (errmsg != NULL) {
            *errmsg = cmdreply ("error: invalid arguments");
         }
         if (*points != 0) {
            free (*points);
         }
         return -22;
      }
   
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: awgSetPhaseIn				*/
/*                                                         		*/
/* Procedure Description: returns awg components from string command	*/
/*                                                         		*/
/* Procedure Arguments: awg component array, number of elements		*/
/*                                                         		*/
/* Procedure Returns: 0 if successful; <0 on error			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/           
   int awgSetPhaseIn (AWG_Component comp[], int cnum)
   {
      int			i;		/* index */
      if (ramptime <= 0) {
         return 0;
      }
      for (i = 0; i < cnum; ++i) {
         /* makes sure we have a useful waveform */
         if ((comp[i].wtype != awgSine) &&
            (comp[i].wtype != awgSquare) &&
            (comp[i].wtype != awgRamp) &&
            (comp[i].wtype != awgTriangle) &&
            (comp[i].wtype != awgConst) &&
            (comp[i].wtype != awgImpulse) &&
            (comp[i].wtype != awgNoiseN) &&
            (comp[i].wtype != awgNoiseU)) {
            continue;
         }
      	 /* don't if waveform is restarted */
         if (comp[i].restart > 0) {
            continue;
         }
      	 /* don't if ramp is already set */
         if (comp[i].ramptype) {
            continue;
         }
      	 /* OK */
         comp[i].ramppar[0] = 0; /* amplitude */
         comp[i].ramppar[1] = 0; /* frequency */
         comp[i].ramppar[2] = 0; /* phase */
         comp[i].ramppar[3] = 0; /* offset */
         comp[i].ramptype = RAMP_TYPE (AWG_PHASING_LINEAR, 
                                      AWG_PHASING_STEP, AWG_PHASING_STEP);
         comp[i].ramptime[0] = (tainsec_t)(1E9*ramptime);
         comp[i].ramptime[1] = 0;
      }
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: ___Slot					*/
/*                                                         		*/
/* Procedure Description: slot cache					*/
/*                                                         		*/
/* Procedure Arguments: -						*/
/*                                                         		*/
/* Procedure Returns: - 						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/           
   static void initSlot (void)
   {
      memset (slotlist, 0, sizeof (slotlist));
      slotinit = 0;
   }

   static int updateSlot (int slot, const char* name)
   {
      int i;
   
      if (slotinit) initSlot();
      /* already exists? */
      for (i = 0; i < MAX_SLOT_LIST; ++i) {
         if (slotlist[i].slot == slot) {
            strncpy (slotlist[i].name, name, MAX_SLOT_NAME);
            slotlist[i].name[MAX_SLOT_NAME-1] = 0;
            return 0;
         }
      }
      /* put it into new slot */
      for (i = 0; i < MAX_SLOT_LIST; ++i) {
         if (slotlist[i].slot) {
            continue;
         }
         slotlist[i].slot = slot;
         strncpy (slotlist[i].name, name, MAX_SLOT_NAME);
         slotlist[i].name[MAX_SLOT_NAME-1] = 0;
         return 0;
      }
      return -1;
   }

   static int freeSlot (int slot)
   {
      int i;
   
      if (slotinit) initSlot();
      for (i = 0; i < MAX_SLOT_LIST; ++i) {
         if (slotlist[i].slot == slot) {
            slotlist[i].slot = 0;
            return 0;
         }
      }
      return -1;
   }

   static const char* readSlot (const char* p, int* slot)
   {
      int i;
      int num;
   
      while (isspace (*p)) ++p;
      /* number */
      if (isdigit (*p)) {
         if (sscanf (p, "%i%n", slot, &num) != 1) {
            *slot = -1;
            return NULL;
         }
         return p + num;
      }
      /* name */
      else {
         for (num = 0; p[num] && !isspace (p[num]); ++num) ;
         for (i = 0; i < MAX_SLOT_LIST; ++i) {
            if (gds_strncasecmp (p, slotlist[i].name, num) == 0) {
               *slot = slotlist[i].slot;
               return p + num;
            }
         }
         *slot = -1;
         return NULL;
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: awgFree					*/
/*                                                         		*/
/* Procedure Description: Free allocated memory 			*/
/*                                                         		*/
/* Procedure Arguments: pointer to be freed    				*/
/*                                                         		*/
/* Procedure Returns: void      					*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   void
   awgFree(void* p) {
      free(p);
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: awgCommand					*/
/*                                                         		*/
/* Procedure Description: command line interface			*/
/*                                                         		*/
/* Procedure Arguments: command string					*/
/*                                                         		*/
/* Procedure Returns: reply string					*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

   char* awgCommand (const char* cmd)
   {
      char*		p;
      int		i;	/* node number */
      int		j;	/* awg number */
      int		sl;	/* slot number */
      char		buf[100]; /* buffer */
      float		id; /* node.awg number */

      /* help */
      if (gds_strncasecmp (cmd, "help", 4) == 0) {
         return cmdreply (_HELP_TEXT);
      }
      /* show */
      else if (gds_strncasecmp (cmd, "show", 4) == 0) {
         p = (char*)(cmd + 4);
         while (isspace (*p)) ++p;
         if (isdigit (*p) || (*p == '+') || (*p == '-') || (*p == '.')) {
            if (sscanf (cmd + 4, "%f", &id) != 1) {
               return cmdreply ("error: arguments for show are 'node'.'awg'/'slot'");
            }
            else {
               if (id < _AWG_IFO_OFS) {
                  i = (int) id;
                  j = (int) (10 * (id - i));
                  sl = -1;
               }
               else {
                  i = (int)id / _AWG_IFO_OFS - 1;
                  j = ((int)id % _AWG_IFO_OFS) / _AWG_NUM_OFS;
                  sl = ((int)id % _AWG_IFO_OFS) % _AWG_NUM_OFS;
               }	
               if (i == _MAX_IFO) {
                  return awgShow (AWG_ID (i,j));
               }
               if (!awgCheckInterface (i, j)) {
                  sprintf (buf, "error: node %i/awg %i not available", i, j);
                  return cmdreply (buf);
               }
               return awgShow (sl < 0 ? AWG_ID (i,j) : -(int)id);
            }
         }
         else {
            p = (char*) readSlot (cmd + 4, &sl);
            if (p == NULL) {
               sprintf (buf, "error: illegal slot name");
               return cmdreply (buf);
            }
            return awgShow (-sl);
         }
      }
      /* channels */
      else if (gds_strncasecmp (cmd, "channel", 6) == 0) {
         i = awgGetChannelNames (NULL, 0, 1);
         if (i < 0) {
            return cmdreply ("error: channel information not available");
         }
         else if (i == 0) {
            return cmdreply ("no channels available");
         }
         p = malloc (i + 10);
         if (p == NULL) {
            return NULL;
         }
         if (awgGetChannelNames (p, i + 9, 1) < 0) {
            return cmdreply ("error: channel information not available");
         }
         else {
            return p;
         }
      }
      /* new */
      else if (gds_strncasecmp (cmd, "new", 3) == 0) {
         /* remove blanks */
         p = (char*) (cmd + 3);
         while (*p == ' ') {
            p++;
         }
         if (strchr (p, ' ') != NULL) {
            *strchr (p, ' ') = 0;
         }
         sl = awgSetChannel (p);
         if (sl < 0) {
            return cmdreply ("error: no slot available or invalid channel name");
         }
         updateSlot (sl, p);
         sprintf (buf, "slot %i", sl);
         return cmdreply (buf);
      }
      /* free */
      else if (gds_strncasecmp (cmd, "free", 4) == 0) {
         if (readSlot (cmd + 4, &sl) == NULL) {
         /*if (sscanf (cmd + 4, "%i", &sl) != 1) {*/
            return cmdreply ("error: invalid slot number/name");
         }
         freeSlot (sl);
         if (awgRemoveChannel (sl) < 0) {
            return cmdreply ("error: slot not available or invalid");
         }
         sprintf (buf, "slot %i freed", sl);
         return cmdreply (buf);
      }
      /* set & add */
      else if ((gds_strncasecmp (cmd, "set", 3) == 0) ||
              (gds_strncasecmp (cmd, "add", 3) == 0)) {
         AWG_Component		comp[20];/* awg components */
         int			cnum;	/* number of comp entries */
         int			cnew;	/* temp. component # */
         char*			errmsg;	/* error message */
         float*			y;	/* awg point array */
         int			len;	/* number of awg points */
         char*			cmdcopy;/* copy of command string */
         char*			lasts;	/* temp for strtok_r */
      
         memset (comp, 0, 20 * sizeof (AWG_Component));
         for (cnum = 0; cnum < 20; cnum++) {
            comp[cnum].start = TAInow();
            comp[cnum].duration = -1;
            comp[cnum].restart = -1;
         }
         /* scan slot number */
         p = (char*) readSlot (cmd + 3, &sl);
         if (p == NULL) {
            return cmdreply ("error: invalid slot number/name");
         }
      
         /* loop over waveforms */
         cnum = 0;
         cmdcopy = cmdreply (p);
         if (cmdcopy == NULL) {
            return cmdreply ("error: unable to set waveform");
         }
         p = strtok_r (cmdcopy, ",;", &lasts);
         while ((p != NULL) && (cnum < 19)) {
           /* skip blanks */
            while (*p == ' ') {
               p++;
            }
            /* determine waveform component */
            y = NULL;
            if (awgWaveformCmd (p, comp + cnum, &cnew, &errmsg, &y, 
               &len, (sl / _AWG_IFO_OFS - 1) == _MAX_IFO) < 0) {
               free (cmdcopy);
               return errmsg;
            }
            cnum += cnew;
            if (y != NULL) {
               if ((len <= 0) || (awgSetWaveform (sl, y, len) < 0)) {
                  free (y);
                  free (cmdcopy);
                  return cmdreply ("error: unable to download waveform");
               }
               free (y);
            }
            p = strtok_r (NULL, ",;", &lasts);
         }
         free (cmdcopy);
      
         /* set phase in */
         if (awgSetPhaseIn (comp, cnum) < 0) {
            return cmdreply ("error: illegal phase in");
         }
      
         /* check if anything set */
         if (gds_strncasecmp (cmd, "set", 3) == 0) {
            /* stop waveforms first */
            if (ramptime > 0) {
               if (awgStopWaveform (sl, 2, (tainsec_t)(1E9*ramptime)) < 0) {
                  return cmdreply ("error: unable to stop waveform");
               }
            }
            else {
               if (awgClearWaveforms (sl)) {
                  return cmdreply ("error: unable to clear waveform");
               }
            }
            if (cnum <= 0) {
               return cmdreply ("waveform cleared");
            }
         }
      	 /* now add new waveform */
         if (cnum > 0) {
            if (awgAddWaveform (sl, comp, cnum) < 0) {
               return cmdreply ("error: slot not available or invalid");
            }
         }
         sprintf (buf, "slot %i enabled", sl);
         return cmdreply (buf);
      }
      /* gain */
      else if (gds_strncasecmp (cmd, "gain", 4) == 0) {
         int		n;
         double		gain;
         double		tramp;
      
         p = (char*)readSlot (cmd + 4, &sl);
         if (p == NULL) {
            return cmdreply ("error: invalid slot number/name");
         }
         if (sscanf (p, "%lf%n", &gain, &n) != 1) {
            return cmdreply ("error: arguments for gain are 'value' 'time'");
         }
         p += n;
         tramp = 1.0;
         if (sscanf (p, "%lf", &tramp) != 1) tramp = 1.0;
         if (awgSetGain (sl, gain, (tainsec_t)(1E9 * tramp)) < 0) {
            return cmdreply ("error: unable to set gain");
         }
         sprintf (buf, "gain in slot %i is %f (ramp = %f sec)", 
                 sl, gain, tramp);
         return cmdreply (buf);
      }
      /* stop */
      else if (gds_strncasecmp (cmd, "stop", 4) == 0) {
         p = (char*)readSlot (cmd + 4, &sl);
         if (p == NULL) {
            return cmdreply ("error: invalid slot number/name");
         }
         if (ramptime > 0) {
            if (awgStopWaveform (sl, 2, (tainsec_t)(1E9*ramptime)) < 0) {
               return cmdreply ("error: unable to stop waveform");
            }
         }
         else {
            if (awgStopWaveform (sl, 0, 0)) {
               return cmdreply ("error: unable to stop waveform");
            }
         }
         sprintf (buf, "slot %i stopped", sl);
         return cmdreply (buf);
      }
      /* ramp */
      else if (gds_strncasecmp (cmd, "ramp", 4) == 0) {
         if (sscanf (cmd + 4, "%lf", &ramptime) != 1) ramptime = 0.0;
         sprintf (buf, "phase in/out time %f sec", ramptime);
         return cmdreply (buf);
      }
      /* clear */
      else if (gds_strncasecmp (cmd, "clear", 5) == 0) {
         if (sscanf (cmd + 5, "%f", &id) != 1) {
            return cmdreply ("error: arguments for clear are 'node'.'awg'");
         }
         else {
            i = (int) id;
            j = (int) (10 * (id - i));
            if (id < 0) {
               if (awgReset (-1) < 0) {
                  return cmdreply ("error: reset failed");
               }
               return cmdreply ("reset succeeded");
            }
            else if ((i == _MAX_IFO) && !isDS340Alive (j)) {
               sprintf (buf, "DS340(%i) not available", j);
               return cmdreply (buf);
            }
            else if (!awgCheckInterface (i, j)) {
               sprintf (buf, "error: node %i/awg %i not available", i, j);
               return cmdreply (buf);
            }
            if (awgReset (AWG_ID (i,j)) < 0) {
               return cmdreply ("error: reset failed");
            }
            return cmdreply ("reset succeeded");
         }
      }
      /* filter */
      else if (gds_strncasecmp (cmd, "filter", 6) == 0) {
         int			n;	/* number of conv chars */
         int			max;	/* max coeff */
         double*		ba;	/* ba array */
         int			len;	/* length of ba array */
         char*			ret;	/* return string */
      
         /* scan slot number */
         p = (char*) readSlot (cmd + 6, &sl);
         if (p == NULL) {
         /*if (sscanf (cmd + 6, "%i%n", &sl, &n) != 1) {*/
            return cmdreply ("error: invalid slot number/name");
         }
         /*p = (char*)cmd + 6 + n;*/
      	 /* get coeffcients */
         max = strlen (p) / 2 + 10;
         ba = calloc (max, sizeof (double));
         for (len = 0; len < max; ++len) {
            while (isspace ((int)*p)) ++p;
            if (!*p) {
               break;
            }
            if (!isdigit ((int)*p) && (*p != '-') && (*p != '+') && (*p != '.')) {
               len = -1;
               break;
            }
            sscanf (p, "%lf%n", ba +len, &n);
            p += n;
         }
         if (awgSetFilter (sl, ba, len) < 0) {
            ret = cmdreply ("error: filter failed");
         }
         else if (len == 0) {
            ret = cmdreply ("filter reset");
         }
         else {
            ret = cmdreply ("filter set");
         }
         free (ba);
         return ret;
      }
      /* stat */
      else if (gds_strncasecmp (cmd, "stat", 4) == 0) {
         if (sscanf (cmd + 4, "%f", &id) != 1) {
            return cmdreply ("error: arguments for stat are 'node'.'awg'");
         }
         else {
            awgStat_t		stat;
            char		zero;
            i = (int) id;
            j = (int) (10 * (id - i));
            if (!awgCheckInterface (i, j)) {
               sprintf (buf, "error: node %i/awg %i not available", i, j);
               return cmdreply (buf);
            }
            if (sscanf (cmd + 4, "%*f %c", &zero) == 1) {
               awgStatistics (AWG_ID (i,j), NULL);
               return cmdreply ("statistics reset");
            }
            else if (awgStatistics (AWG_ID (i,j), &stat) < 0) {
               return cmdreply ("error: no statistics available");
            } 
            else {
               p = malloc (16 * 100);
               sprintf 
                  (p, 
                  "Staistics of node %i/awg %i (time in ms/heartbeat)\n"
                  "%10.0f : number of waveform processing cycles\n"
                  "%10.3f : average time to process waveforms\n"
                  "%10.3f : standard deviation of processing waveforms\n"
                  "%10.3f : maximum time needed to process waveforms\n"
                  "%10.0f : number of reflective memory writes\n"
                  "%10.3f : average time to write to reflective memory\n"
                  "%10.3f : standard deviation of writing to reflective memory\n"
                  "%10.3f : maximum time needed to write to reflective memory\n"
                  "%10.3f : spare time after writing to reflective memory\n"
                  "%10.0f : number of late writes to reflective memory\n"
                  "%10.0f : number of writes to the DAC\n"
                  "%10.3f : average time to write to the DAC\n"
                  "%10.3f : standard deviation of writing to the DAC\n"
                  "%10.3f : maximum time needed to write to the DAC\n"
                  "%10.0f : number of missed writes to the DAC\n",
                  i, j, stat.pwNum, stat.pwMean/1E6, stat.pwStddev/1E6,
                  stat.pwMax/1E6, stat.rmNum, stat.rmMean/1E6, 
                  stat.rmStddev/1E6, stat.rmMax/1E6, stat.rmCrit/1E6, 
                  stat.rmNumCrit, stat.dcNum, stat.dcMean/1E6, 
                  stat.dcStddev/1E6, stat.dcMax/1E6, stat.dcNumCrit);
               return p;
            }
         }
      }
      else {
         return cmdreply ("error: unrecognized command\n"
                         "use help for further information");
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: awgcmdline					*/
/*                                                         		*/
/* Procedure Description: command line interface			*/
/*                                                         		*/
/* Procedure Arguments: command string					*/
/*                                                         		*/
/* Procedure Returns: reply string					*/
/*                                                         		*/
/*----------------------------------------------------------------------*/           
   int awgcmdline (const char* cmd)
   {
      char* 		p;
      int		ret;
   
      p = awgCommand (cmd);
      if (p) {
         printf ("%s\n", p);
      }
      else {
         printf ("failed\n");
      }
      ret = (strncmp (p, "error:", 6) == 0) ? -1 : 0;
      free (p);
      return ret;
   }
