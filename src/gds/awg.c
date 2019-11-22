static char *versionId = "Version $Id$" ;
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: Arbitrary waveform generator				*/
/*                                                         		*/
/* Procedure Description: software generated waveforms			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

#define DEBUG
//#define DEBUG_INIT

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Defines: Describes the output capabilities of the AWG		*/
/*          _AWG_DAC		awg outputs waveforms to a digital-to-	*/
/*				analog converter 			*/
/*          _AWG_DS340		awg outputs waveforms to a stand-alone	*/
/*				DS340 Stanford signal generator		*/
/*          _AWG_RM		awg outputs waveforms to memory buffer	*/
/*				in a reflective memory module		*/
/*          _AWG_STATISTICS	gather realtime statistics		*/
/*          _AWG_FILTER_TIME	time const of average filter, 0 = none	*/
/*          _AWG_FILTER_COEF	filter coefficient			*/
/*          _AWG_FILTER		filter macro				*/
/*          _AWG_MEAN		mean macro				*/
/*          _AWG_STDDEV		standard deviation macro		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#if !defined(_AWG_DAC) && !defined(_AWG_DS340) && !defined(_AWG_RM)
//#define _AWG_DAC
//#define _AWG_DS340
#define _AWG_RM
#endif

#ifndef _AWG_STATISTICS
#define _AWG_STATISTICS
#endif

#if !defined (_AWG_FILTER_TIME)
#define _AWG_FILTER_TIME	10	/* in sec */
#endif
#define _AWG_FILTER_COEF 	(1.0/((_AWG_FILTER_TIME) * \
				       NUMBER_OF_EPOCHS))

#if (_AWG_FILTER_TIME < 1)
#define _AWG_FILTER(u,e)	(u) += (e)
#define _AWG_MEAN(u,n)		(u) /= (n)
#define _AWG_STDDEV(d,m,n)	(d) = sqrt(((d) - pow((m),2)/(n)) / \
					   ((n) - 1))
#else
#define _AWG_FILTER(u,e)	(u) = (1 - _AWG_FILTER_COEF) * (u) + \
				      _AWG_FILTER_COEF * (e)
#define _AWG_MEAN(u,n)		
#define _AWG_STDDEV(d,m,n)	(d) = sqrt((d) - pow((m),2))
#endif

#if TARGET == 321 || TARGET == 320
#undef _AWG_DAC
#endif

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Includes								*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#ifdef OS_VXWORKS
#include <vxWorks.h>
#include <cacheLib.h>
#include <signal.h>
#endif
#include <stdio.h>
#include <limits.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <semaphore.h>
#include "dtt/gdsutil.h"
#include "gdsconst.h"
#include "dtt/awg.h"
#include "dtt/awgfunc.h"
#include "dtt/gdstask.h"
#ifndef _NO_TESTPOINTS
#include "dtt/testpoint.h"
#include "dtt/testpointinfo.h"
#endif
#include "dtt/map.h"
#include "dtt/gdsdac.h"
#include "dtt/timingcard.h"
#include "dtt/targets.h"
#include "dtt/hardware.h"
#include "dtt/rmorg.h"
#include "dtt/rmapi.h"


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Defines: numerical constants to share between Unix/vxWorks		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#define POWER(x,y)	(exp (log (x) * (y))
/* :COMPILER: replace EXTREMLY inefficient fmod function in VxWorks */
#ifdef OS_VXWORKS
#define fmod(x,y)	(((x) >= 0) ? ((x) - (y) * floor ((x) / (y))) : \
				      ((x) - (y) * ceil ((x) / (y))))
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Constants: _SHOWBUF_SIZE       size of show buffer			*/
/*            _AWG_PRIORITY       priority of the awg task		*/
/*	      _AWGCOPY_PRIORITY   priority of the awg copy task		*/
/*	      _AWGDAC_PRIORITY    priority of the dac copy task		*/
/*	      _AWG_TASKNAME	  name of the awg task			*/
/*	      _AWGCOPY_TASKNAME	  name of the awg copy task		*/
/*	      _AWGDAC_TASKNAME	  name of the dac copy task		*/
/*            _INVALID_RM_CHANNEL invalid flag for RM channels		*/
/*            _MAX_BUF		  maximum number of buffer pages	*/
/*            _MAX_PAGE		  maximum page length			*/
/*            _DAC_PAGE		  page length for DAC			*/
/*            _LSCTP_PAGE	  page length for LSC test points	*/
/*            _ASCTP_PAGE	  page length for ASC test points	*/
/*            _MAX_DAC_CHN	  maximum dac channels			*/
/*            _AHEAD		  number of epochs the awg is ahead	*/
/*            _MAX_BEHIND	  max # of epochs the awg can be behind	*/
/*            _DAC_ISR_AHEAD	  how many epochs the DAC isr is ahead	*/
/*            _DAC_CONVERSION	  DAC conversion gain			*/
/*            _DAC_ID		  DAC board ID				*/
/*            _LSCX_BASE	  offset of LSC exitation data block	*/
/*            _ASCX_BASE	  offset of ASC exitation data block	*/
/*            _RM_ID		  reflective memory ID			*/
/*            								*/
/*----------------------------------------------------------------------*/
#define _SHOWBUF_SIZE		(128 * 1024)
#ifdef OS_VXWORKS
#define _AWG_PRIORITY		30
#define _AWGCOPY_PRIORITY	20
#define _AWGDAC_PRIORITY	10
#else
#define _AWG_PRIORITY		4
#define _AWGCOPY_PRIORITY	2
#define _AWGDAC_PRIORITY	2
#endif
#define _AWG_TASKNAME		"tAWG"
#define _AWGCOPY_TASKNAME	"tAWGcopy"
#define _AWGDAC_TASKNAME	"tDACcopy"
#define __ONESEC		((double) _ONESEC)
#define _INVALID_RM_CHANNEL	0xBAD
#if RMEM_LAYOUT > 0
#define _MAX_BUF		DAQ_NUM_DATA_BLOCKS
#else
#define _MAX_BUF		8	
#endif
/* This will need to go higher for systems faster than 32 kHz */
/* or if DCU block size is increased */
#define _MAX_PAGE		(DAQ_DCU_SIZE/256)
#define _DAC_PAGE		1024	
#define _LSCTP_PAGE		TP_LSC_CHN_LEN	
#define _ASCTP_PAGE		TP_ASC_CHN_LEN	
#define _MAX_DAC_CHN		32	
#define _MAX_DS340_CHN		10	
#define _AHEAD			3
#define _MAX_BEHIND		1
#define _DAC_ISR_AHEAD		1
#define _DAC_ID			0
#define _DAC_CONVERSION		(32767.0 / 5.0)
#define PRM_FILE		gdsPathFile ("/param", "awg.par")

#ifdef GDS_UNIX_TARGET
#define _LSCX_UNIT_ID           GDS_4k_LSC_EX_ID
#define _ASCX_UNIT_ID           GDS_4k_ASC_EX_ID
extern int testpoint_manager_node;
#define _TP_NODE                testpoint_manager_node
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
   UNIT_ID_TO_DATA_OFFSET (_LSCX_UNIT_ID);
   static const int 		_ASCX_BASE = 
   UNIT_ID_TO_DATA_OFFSET (_ASCX_UNIT_ID);
   static const int 		_LSCX_SIZE = 
   UNIT_ID_TO_DATA_BLOCKSIZE (_LSCX_UNIT_ID);
   static const int 		_ASCX_SIZE = 
   UNIT_ID_TO_DATA_BLOCKSIZE (_ASCX_UNIT_ID);


/*----------------------------------------------------------------------

Organization of generating waveforms:
1. Waveforms are calulated ahead of time as specified by _AHEAD
2. Waveforms are written into a buffer which address can be determined
   by 'buf index' = 'epoch of waveform' % _MAX_BUF
3. after the buffers for one epoch are filled the ready flag is set
   and the readycount semaphore is increased.
4. The copy task waits for the ready flag and when given copies 
   data to the reflective memory or teh DAC.
5. If the readycount becomes 2 or the next page is outdated it
   gets discared automatically.

------------------------------------------------------------------------*/


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Types: streampagebuf_t	stream buffer page			*/
/*	  streambuf_t		stream buffer				*/
/*	  streambufpage_t	buffers for output waveform page	*/
/*	  filter_coeff_t	filter coefficients			*/
/*        awgbuf_t		buffer for a set of waveforms		*/
/*        awgmbuf_t		multiple buffers of type awgbuf_t	*/
/*        dacbuf_t		buffer for DAC data			*/
/*        awgtp_t		structure to store testpoint data	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   struct streambufpage_t {
      /* ready flag: 0 - not ready, 1 - ready, 2 - in process */
      int		ready;
      /* Time stamp of last fill */
      taisec_t		time;
      int		epoch;    
      /* page length */
      int		pagelen;
      /* Data buffer */
      float		buf[_MAX_PAGE];
   };
   typedef struct streambufpage_t streambufpage_t;

   struct streambuf_t {
      /* stream buffer pages */
      streambufpage_t	buf[NUMBER_OF_EPOCHS * MAX_STREAM_BUFFER];
   };
   typedef struct streambuf_t streambuf_t;

   struct filter_coeff_t {
      /* filter valid */
      int		valid;
      /* filter gain */
      double		gain;
      /* number of sos */
      int		nsos;
      /* filter coefficients */
      double		sos[MAX_AWG_SOS][4];
      /* history */
      double		hist[MAX_AWG_SOS][2];
   };
   typedef struct filter_coeff_t filter_coeff_t;

   struct awgpagebuf_t {
      /* ouput type */
      AWG_OutputType	otype;
      /* output pointer (testpoints) */
      float*		optr;
      /* channel number (DAC) */
      int		onum;
      /* page length */
      int		pagelen;
      /* status of page buffer */
      int		status;
      /* buffer for page */
      float		page [_MAX_PAGE];
   };
   typedef struct awgpagebuf_t awgpagebuf_t;

   struct awgbuf_t {
      /* ready flag: 0 - not ready, 1 - ready, 2 - in process */
      int		ready;
      /* time and epoch described with this buffer */
      taisec_t		time;
      int		epoch;
      /* buffer for waveforms */
      awgpagebuf_t	buf[MAX_NUM_AWG];
   };
   typedef struct awgbuf_t awgbuf_t;

   struct awgmbuf_t {
      /* counts how many buffers are ready */
      sem_t		ready;
      /* buffers for awgs */
      awgbuf_t		bufs[_MAX_BUF];
   };
   typedef struct awgmbuf_t awgmbuf_t;

   struct dacbuf_t {
      /* ready flag */
      int		ready;
      /* time and epoch described with this buffer */
      taisec_t		time;
      int		epoch;
      /* buffer for dac waveforms */
      short		buf[_DAC_PAGE * _MAX_DAC_CHN];
      /* buffer length */
      int		buflen;
   };
   typedef struct dacbuf_t dacbuf_t;

#ifndef _NO_TESTPOINTS
   struct awgtp_t {
      /* time when tp becomes active */
      taisec_t		time;
      /* epoch when tp becomes active, always 0 */
      int 		epoch;
      /* list of lsc excitation test points */
      testpoint_t 	lsctp[TP_LSC_EX_NUM];
      /* list of asc excitation test points */
      testpoint_t 	asctp[TP_ASC_EX_NUM];
   };
   typedef struct awgtp_t awgtp_t;
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Globals: AWG[] - configuration and status of module			*/
/*          awginit - non-zero if initialized				*/
/*          DACnumchn - number of channels used by the DAC		*/
/*          awgmux - protect globals (other than AWG)			*/
/*          awgTID - task ID of awg task				*/
/*          awgcopyTID - task ID of awg copy task			*/
/*          awgdacTID - task ID of dac copy task			*/
/*          awgbuf - buffer for testpoint pages				*/
/*          awgstream - buffer for waveform stream			*/
/*          dacmux - protects dacbuf					*/
/*          dacbuf - buffer for stroring DAC data			*/
/*          dacstat - status of DAC: down, sync, ... up			*/
/*          awgtp - stores tp info, most recent info at tp[0]		*/
/*             	    last one at tp[1] (needs the mutex for access)	*/
/*          statmux - mutex for realtime statistics			*/
/*          stat - realtime statistics					*/
/*          								*/
/*----------------------------------------------------------------------*/
   static AWG_ConfigBlock 	AWG [MAX_NUM_AWG];
   static int 			awginit	= 0;
#ifdef _AWG_DAC
   static int 			DACnumchn = ICS115_0_CHN_NUM;
   static sem_t			semDACisr;
#define STATUSDAC_DOWN 		0
#define STATUSDAC_SYNC		1
#define STATUSDAC_INIT 		2
#define STATUSDAC_RESTART	3
#define STATUSDAC_READY		4
#define STATUSDAC_UP 		5
#define DAC_TIMEOUT		3 /* in sec */
   static int			dacstat;
#endif
   static mutexID_t		awgmux;
   static taskID_t		awgTID;
   static taskID_t		awgcopyTID;
   static taskID_t		awgdacTID;
   static awgmbuf_t		awgbuf;
   static streambuf_t	 	awgstream[MAX_NUM_AWG];
   static filter_coeff_t	awgfilter[MAX_NUM_AWG];
#ifdef _AWG_DAC
   static mutexID_t		dacmux;
   static dacbuf_t*		dacbuf = 0;
#endif
#ifndef _NO_TESTPOINTS
   static awgtp_t 		awgtp[2];
#endif
#ifdef _AWG_STATISTICS
   static mutexID_t		statmux;
   static awgStat_t		awgstat;
#endif
   int				awgdebug = 0;


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Macros: _GET_OUTPUT_BUFFER gets the output buffer			*/
/*         _GET_STREAM_BUFFER gets the stream buffer     		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#define _GET_OUTPUT_BUFFER(epoch,id) \
		(awgbuf.bufs[epoch % _MAX_BUF].buf[id].page)
#define _GET_STREAM_BUFFER(time,epoch,id) \
		(&awgstream[id].buf[(time % MAX_STREAM_BUFFER) * \
			NUMBER_OF_EPOCHS + epoch])

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Prototypes:							*/
/* 	openConfigFile - opens a configuration file			*/
/* 	configAWGfp - configures an AWG channel from file		*/
/* 	calcAWGComp - calculates an AWG component			*/
/* 	calcAWGFilter - calculates an AWG filter			*/
/* 	flushAWG - sets the output buffer parameters			*/
/* 	readTPinfo - reads the test point info				*/
/* 	getTPslot - gets the TP slot # and the tp address		*/
/* 	retireAWGComp - ritires old awg componets			*/
/* 	finiAWG - cleanup						*/
/* 									*/
/*----------------------------------------------------------------------*/
   static FILE* openConfigFile (const char* filename);
   static int configAWGfp (int ID, FILE* fp);
   static int calcAWGComp (int ID, int cid, taisec_t time, int epoch,
                     tainsec_t delay);
   static int calcAWGGain (int ID, taisec_t time, int epoch,
                     tainsec_t delay);
   static int calcAWGFilter (int ID, taisec_t time, int epoch,
                     tainsec_t delay);
   static int flushAWG (int ID, taisec_t time, int epoch);
#ifndef _NO_TESTPOINTS
   static int readTPinfo (taisec_t tai, int epoch);
#endif
   static int getTPslot (AWG_OutputType type, int tpID, 
                     taisec_t time, int epoch, float** tpptr);
   static int invalidateTPslots (int epoch);
   static void retireAWGComp (tainsec_t time);
#ifdef _AWG_DAC
   static void tdacISR (void);
   static void copyDACdata (awgbuf_t* buf);
   static void copyRMdata (awgbuf_t* buf);
#endif
   AWG_ConfigBlock* getAWGconfigBlock (int ID);
   __fini__(finiAWG);
#pragma fini(finiAWG)


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Task Name: tAWG							*/
/*                                                         		*/
/* Procedure Description: task to process AWG block on heartbeat	*/
/* 									*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   void tAWG (void) {
      taisec_t		tai;		/* time of current heartbeat */
      int		epoch;		/* epoch of current heartbeat */
      tainsec_t		tnew;		/* time of current heartbeat */
      tainsec_t		told;           /* time of last heartbeat */
      taisec_t		awgtime;	/* time of waveform to calc. */
      int		awgepoch;	/* epoch of waveform to calc. */
      awgbuf_t*		buf;		/* pointer to waveform buffer */
   #ifdef _AWG_STATISTICS
      tainsec_t		tstart;		/* start time */
      tainsec_t		tstop;		/* stop time */
      double 		dt;		/* time difference */
   #endif
   
   #ifdef VXWORKS
      signal (SIGFPE, SIG_IGN);
   #endif
      /* initialize time */
      told = TAInow ();
   
      /* infinite loop synchronized to the system heartbeat */
      for (;;) {
         if (syncWithHeartbeatEx (&tai, &epoch) != 0) {
            gdsError (GDS_ERR_MISSING, "Lost heartbeat interrupt");
            return;
         }
      
         /* realtime statistics */
      #ifdef _AWG_STATISTICS
         tstart = TAInow();
      #endif
         if (awgdebug) {
            printf ("tAWG at %i epoch %i (t = %i.%09i)\n", (int)tai, epoch,
                   (int)(tstart / _ONESEC), (int)(tstart % _ONESEC));
         }
         /* check for lost epochs */
         tnew = ((tainsec_t) tai) * _ONESEC + 
                ((tainsec_t) epoch) * _EPOCH;
         if (tnew - told > _EPOCH + _EPOCH / 2) {
            gdsError (GDS_ERR_MISSING, "Lost a heartbeat");
         }
         told = tnew;
      
         /* calulate time of waveform calculation */
         awgepoch = epoch + _AHEAD;
         if (awgepoch < NUMBER_OF_EPOCHS) {
            awgtime = tai;
         }
         else {
            awgtime = tai + 1;   
            awgepoch -= NUMBER_OF_EPOCHS;
         }
         if (awgdebug) {
            printf ("AWG heartbeat at time = %lu epoch %i\n", 
                   awgtime, awgepoch);
         }
         /* invalidate all testpoint slots before writing waveforms */
         invalidateTPslots (awgepoch);
      
         /* process waveforms */
         processAllAWG (awgtime, awgepoch);
      
      	 /* complete buffer info */
         MUTEX_GET (awgmux);
         buf = &awgbuf.bufs[awgepoch % _MAX_BUF];
         buf->time = awgtime;
         buf->epoch = awgepoch;
         buf->ready = 1;
         MUTEX_RELEASE (awgmux);
      	 /* start copy process */
         if (sem_post (&awgbuf.ready) != 0) {
            gdsError (GDS_ERR_CORRUPT, 
                     "unable to start AWG copy process");
            sem_init (&awgbuf.ready, 0, 1);
         }
      
      	 /* read testpoint index if necessary */
      #ifndef _NO_TESTPOINTS
         if (epoch == TESTPOINT_VALID1) {
            if (readTPinfo (tai + 1, 0) < 0) {
               gdsError (GDS_ERR_CORRUPT, 
                        "unable to read testpoint information");
            }
         }
      #endif
      
      	 /* retire old AWG components */
         retireAWGComp (tnew + (_AHEAD - 1) * _EPOCH);
      
      	 /* gather realtime statistics */
      #ifdef _AWG_STATISTICS
         tstop = TAInow();
         dt = (double) (tstop - tstart);
         if (fabs (dt) < 10.0 * __ONESEC) { /* no outliers */
            MUTEX_GET (statmux);
            awgstat.pwNum += 1.0;
            _AWG_FILTER (awgstat.pwMean, dt);
            _AWG_FILTER (awgstat.pwStddev, dt*dt);
            if (dt > awgstat.pwMax) {
               awgstat.pwMax = dt;
            }
            MUTEX_RELEASE (statmux);
         }
      #endif
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: retireAWGComp				*/
/*                                                         		*/
/* Procedure Description: retires expired AWG components		*/
/* 									*/
/* Procedure Arguments: time						*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   void retireAWGComp (tainsec_t time)
   {
      int 		i;	/* AWG index */
      int		j;	/* component index */
      int 		k;	/* first component to retired */
      int 		l;	/* last+1 component to retire */
      AWG_ConfigBlock*	awg;	/* pointer to awg */
      AWG_Component*	comp;	/* pointer to awg comnponent */
   
      for (i = 0, awg = AWG; i < MAX_NUM_AWG; i++, awg++) {
         /* get mutex */
         MUTEX_GET (awg->mux);
      
      	 /* test whether configured */
         if (((awg->status & AWG_CONFIG) == 0) ||
            (awg->otype == awgNone) ||
            (awg->ncomp <= 0)) {
            MUTEX_RELEASE (awg->mux);
            continue;
         }
      
         /* go through list and determine first stretch of
            expired entries */
         for (j = 0, comp = awg->comp, k = 0, l = -1; 
             j < awg->ncomp; j++, comp++) {
            /* make sure waveform is valid, then test time */
            if ((comp->wtype == awgNone) ||
               ((comp->duration >= 0) && (comp->restart <= 0) &&
               (comp->start + comp->duration <= time))) {
               /* found an expired entry */
               l = j + 1;
            }
            else {
               /* found an non expired entry */
               if (l == -1) {
                  /* haven't found an expired one yet, so set k */
                  k = j + 1;
               }
               else {
                  /* have already found expired ones, so quit */
                  break;
               }
            }
         }
      
      	 /* retire entries if found */
         if (l > 0) {
            /* only move components if there are non expired entries 
               at the end */
            if (l < awg->ncomp) {
               memmove (awg->comp + k, awg->comp + l,
                       (awg->ncomp - l) * sizeof (AWG_Component));
               memmove (awg->rb + k, awg->rb + l,
                       (awg->ncomp - l) * sizeof (randBlock));
            }
            awg->ncomp -= (l - k);
         }
      
         /* release mutex */
         MUTEX_RELEASE (awg->mux);
      }
   
      return;
   }


#if defined(_AWG_DAC)
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Task Name: tdacISR							*/
/*                                                         		*/
/* Procedure Description: ISR for DAC; copies data to DAC swing buffer	*/
/* 									*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/    
   void tdacISR (void) 
   {
      dacbuf_t*		dbuf;		/* pointer to dac buffer */
      int		i;		/* index into dac buffers */
      tainsec_t		tstart;		/* start time */
      int		isrepoch;	/* epoc	h needed by DAC */
   #ifdef _AWG_STATISTICS
      tainsec_t		tstop;		/* stop time */
      double 		dt;		/* time difference */
   #endif
   
      for (;;) {
         /* wait for semaphore */
         sem_wait (&semDACisr);
      
         /* realtime statistics */
         tstart = TAInow();
         isrepoch = ((tstart + _DAC_ISR_AHEAD * _EPOCH + _EPOCH / 10) % 
                    _ONESEC) / _EPOCH;
         if (awgdebug && (isrepoch >= 0)) {
            printf ("dac isr at %i [%i]\n", (int) (tstart % _ONESEC), isrepoch);
         }
      
         MUTEX_GET (dacmux);
         if ((dacstat != STATUSDAC_READY) && 
            (dacstat != STATUSDAC_UP)) {
            MUTEX_RELEASE (dacmux);
            continue;
         }
      	 /* dac is running normally */
         if (dacstat == STATUSDAC_READY) {
            dacstat = STATUSDAC_UP;
            printf ("DAC up\n");
         }
         MUTEX_RELEASE (dacmux);
      
         /* look for ready page */
         dbuf = &dacbuf[isrepoch % _MAX_BUF];
         MUTEX_GET (dacmux);
         if (dbuf->ready != 1) {
            dacstat = STATUSDAC_DOWN;
            for (i = 0; i < _MAX_BUF; i++) {
               dacbuf[i].ready = 0;
            }
            MUTEX_RELEASE (dacmux);
         #ifdef _AWG_STATISTICS
            MUTEX_GET (statmux);
            awgstat.dcNumCrit += 1.0;
            MUTEX_RELEASE (statmux);
         #endif
            printf ("DAC buffer not ready in time\n");
            gdsError (GDS_ERR_TIME, "DAC buffer not ready in time");
            continue;
         }
         dbuf->ready = 2;
         MUTEX_RELEASE (dacmux);
      
         /* copy data and test for error */
         if (dacCopyData (_DAC_ID, dbuf->buf, 
            _DAC_PAGE * DACnumchn) != 0) {
            /* set DAC to down state */
            MUTEX_GET (dacmux);
            dacstat = STATUSDAC_DOWN;
            for (i = 0; i < _MAX_BUF; i++) {
               dacbuf[i].ready = 0;
            }
            MUTEX_RELEASE (dacmux);
            printf ("error while copying data to DAC\n");
            gdsError (GDS_ERR_TIME, "error while copying data to DAC");
         #ifdef _AWG_STATISTICS
            MUTEX_GET (statmux);
            awgstat.dcNumCrit += 1.0;
            MUTEX_RELEASE (statmux);
         #endif
         }
         MUTEX_GET (dacmux);
         dbuf->ready = 0;
         MUTEX_RELEASE (dacmux);
      
         /* gather realtime statistics */
      #ifdef _AWG_STATISTICS
         tstop = TAInow();
         dt = (double) (tstop - tstart);
         if (fabs (dt) < 10.0 * __ONESEC) { /* no outliers */
            MUTEX_GET (statmux);
            awgstat.dcNum += 1.0;
            _AWG_FILTER (awgstat.dcMean, dt);
            _AWG_FILTER (awgstat.dcStddev, dt*dt);
            if (dt > awgstat.dcMax) {
               awgstat.dcMax = dt;
            }
            MUTEX_RELEASE (statmux);
         }
      #endif
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure: resyncDAC / restartDAC				*/
/*                                                         		*/
/* Procedure Description: resync/restart the DAC			*/
/* 									*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/  
   void resyncDAC (void)
   {
      /* wait a little while */
      struct timespec delay = {DAC_TIMEOUT, 0};
      nanosleep (&delay, NULL);
   
      /* now initialize dac */
      if (dacReinit (_DAC_ID) != 0) {
         gdsError (GDS_ERR_TIME, "unable to initialize AWG DAC");
         MUTEX_GET (dacmux);
         dacstat = STATUSDAC_DOWN; 
         MUTEX_RELEASE (dacmux);
         printf ("DAC down (restart)\n");
      }
      else {
         MUTEX_GET (dacmux);
         dacstat = STATUSDAC_INIT; 
         MUTEX_RELEASE (dacmux);
         printf ("DAC init\n");
      }
   }


   void restartDAC (void)
   {
      /* start DAC */
      if (dacRestart (_DAC_ID, dacbuf[0].buf, dacbuf[1].buf, 
         _DAC_PAGE * DACnumchn) != 0) {
         gdsError (GDS_ERR_TIME, "unable to start AWG DAC");
         MUTEX_GET (dacmux);
         dacstat = STATUSDAC_DOWN;
         MUTEX_RELEASE (dacmux);
         printf ("DAC down (restart)\n");
      }
      else {
         gdsConsoleMessage ("restart DAC");
         MUTEX_GET (dacmux);
         dacstat = STATUSDAC_READY;
         MUTEX_RELEASE (dacmux);
         printf ("DAC ready\n");
         printf ("dac ready at %i\n", (int) (TAInow() % _ONESEC));
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure: copyDACdata					*/
/*                                                         		*/
/* Procedure Description: copies data from main buffer to DAC buffer	*/
/* 									*/
/* Procedure Arguments: buf - main buffer				*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/  
   void copyDACdata (awgbuf_t* buf)
   {
      int		stat;		/* dac status */
      dacbuf_t*		dbuf;		/* pointer to dac buffer */
      int		i;		/* index into awg buffers */
      /*int		j;		 index into dac fata buf */
      /*int		k;		 index into awg page */
      /*float		val;		 float value to be copied */
      awgpagebuf_t*	p;		/* pointer to awg page */
      int		attr;		/* task attribute */
      taskID_t	 	TID;		/* task ID */
   
      if (awgdebug) {
         printf ("copyDACdata for time = %lu epoch %i\n", 
                buf->time, buf->epoch);
      }
      MUTEX_GET (dacmux);
      stat = dacstat; 
      MUTEX_RELEASE (dacmux);
      /* nothing to do if DAC down and not in epoch 6 or later */
      if ((stat == STATUSDAC_DOWN) && (buf->epoch < 6)) {
         return;
      }
      /* if DAC down, reinitialize DAC  */
      if (stat == STATUSDAC_DOWN) {
         printf ("DAC sync\n");
         MUTEX_GET (dacmux);
         dacstat = STATUSDAC_SYNC;
         MUTEX_RELEASE (dacmux);
      #ifdef OS_VXWORKS
         attr = 0;
      #else
         attr = PTHREAD_CREATE_DETACHED | PTHREAD_SCOPE_PROCESS;
      #endif
         if (taskCreate (attr, _AWGDAC_PRIORITY, &TID, 
            "tDACsync", (taskfunc_t) resyncDAC, NULL) != 0) {
            gdsError (GDS_ERR_PROG, "Unable to create DAC task");
            printf ("DAC down\n");
            MUTEX_GET (dacmux);
            dacstat = STATUSDAC_DOWN;
            MUTEX_RELEASE (dacmux);
         }
         return;
      }
      /* if DAC sync, return */
      if (stat == STATUSDAC_SYNC) {
         return;
      }
      /* if DAC is initialized and not in the first 2 epochs, return */
      if ((stat == STATUSDAC_INIT) && (buf->epoch > 1)) {
         return;
      }
   
      /* copy data into DAC buf */
      dbuf = &dacbuf[buf->epoch % _MAX_BUF];
      dbuf->time = buf->time;
      dbuf->epoch = buf->epoch;
      /* check if buffer are emptied */
      if (((stat == STATUSDAC_UP) || (stat == STATUSDAC_READY)) && 
         (dbuf->ready != 0)) {
         /* can't keep up: start fresh & disable old buffers */
         MUTEX_GET (dacmux);
         dacstat = STATUSDAC_DOWN; 
         for (i = 0; i < _MAX_BUF; i++) {
            dacbuf[i].ready = 0;
         }
         MUTEX_RELEASE (dacmux);
         printf ("DAC set to down: can't keep up at epoch %i\n", 
                dbuf->epoch);
         gdsDebug ("DAC set to down: can't keep up");
         return;
      }
      /* first set buffer to zero */
      memset ((void*) dbuf->buf, 0, 
             (_DAC_PAGE * DACnumchn) * sizeof (short));
      /* now loop over awg channels */
      for (i = 0, p = buf->buf; i < MAX_NUM_AWG; i++, p++) {
         /* test whether valid DAC channel */
         if ((p->otype != awgDAC) || (p->pagelen != _DAC_PAGE) ||
            (p->onum < 0) || (p->onum >= DACnumchn)) {
            continue;
         }
      	 /* now do the copy */
         dacConvertData (dbuf->buf, p->page, p->onum, _DAC_PAGE);
         /*for (k = 0, j = p->onum; k < _DAC_PAGE; k++, j += DACnumchn) {
            val = floor (_DAC_CONVERSION * p->page[k]);
            make sure clipping works ok */
            /*if (fabs (val) >= (float) SHRT_MAX) {
               dbuf->buf[j] = (val > 0) ? SHRT_MAX : -SHRT_MAX;
            }
            else {
               dbuf->buf[j] = (short) (val);
            }
         }*/
      }
      dbuf->ready = 1;
   
      /* if DAC status init, restart DAC */
      if ((stat == STATUSDAC_INIT) && (buf->epoch == 1)) {
         /* start fresh: disable old buffers */
         MUTEX_GET (dacmux);
         for (i = 0; i < _MAX_BUF; i++) {
            dacbuf[i].ready = 0;
         }
         dacstat = STATUSDAC_RESTART;
         MUTEX_RELEASE (dacmux);
         printf ("DAC restart\n");
         /* restart dac with first 2 buffers */
      #ifdef OS_VXWORKS
         attr = 0;
      #else
         attr = PTHREAD_CREATE_DETACHED | PTHREAD_SCOPE_PROCESS;
      #endif
         if (taskCreate (attr, _AWGDAC_PRIORITY, &TID, 
            "tDACstart", (taskfunc_t) restartDAC, NULL) != 0) {
            gdsError (GDS_ERR_PROG, "Unable to create DAC restart task");
            printf ("DAC down\n");
            MUTEX_GET (dacmux);
            dacstat = STATUSDAC_DOWN;
            MUTEX_RELEASE (dacmux);
         }
      }
   
      /* check for timing errors in the middle of the second */
      if ((buf->epoch == 8) && (stat == STATUSDAC_UP) &&
         dacError (_DAC_ID)) {
         MUTEX_GET (dacmux);
         dacstat = STATUSDAC_DOWN; 
         for (i = 0; i < _MAX_BUF; i++) {
            dacbuf[i].ready = 0;
         }
         MUTEX_RELEASE (dacmux);
         printf ("DAC set to down: timing error\n");
         gdsDebug ("DAC set to down: timing error");
         return;
      }
   }
#endif

   int awgInterleave = 4;
   int awgVmeReadDelay = 4;

#if defined(_AWG_RM)
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure: copyRMdata					*/
/*                                                         		*/
/* Procedure Description: copies data to reflective memory	 	*/
/* 									*/
/* Procedure Arguments: buf - main buffer				*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/  
   void copyRMdata (awgbuf_t* buf)          
   {
      int		i;		/* index into awg buffers */
      awgpagebuf_t*	p;		/* pointer to awg page */
      for (i = 0, p = buf->buf; i < MAX_NUM_AWG; i++, p++) {
         /* test whether valid test point channel */
         switch (p->otype) 
         {
            case awgLSCtp:
            case awgDAC: /* copy DAC channel into LSC EXC if necessary */
               {
                  if ((p->pagelen != _LSCTP_PAGE) ||
                     (p->optr == NULL)) {
                     continue;
                  }
                  break;
               }
            case awgASCtp:
               {
                  if ((p->pagelen != _ASCTP_PAGE) ||
                     (p->optr == NULL)) {
                     continue;
                  }
                  break;
               }
            case awgMem:
               {
                  if ((p->pagelen <= 0) ||
                     (p->optr == NULL)) {
                     continue;
                  }
                  break;
               }
            default:
               {
                  continue;
               }
         }
      
         /* now start transfer */
         /* memcpy ((void*) p->optr, (void*) p->page, 
                p->pagelen * sizeof (float)); */
         /* don't forget to write status */
         p->status = 0;
         {
            static int cnt = 0;
            if (buf->epoch == 0) {
               cnt = 0;
            #if 0
               printf ("addr = 0x%x size = %d\n", (int) p->optr, 
                      (int) (p->pagelen + 1) * sizeof (float));
               printf ("values = %g %g %g %g\n",
                      p->page[p->pagelen-1], p->page[p->pagelen-2],
                      p->page[p->pagelen-3], p->page[p->pagelen-4]);
            #endif
            }
         /*
            if (1 || buf->epoch == 8) {
               float x[4];
               static int ofs[100] = {0, 0, 0, 0, 0, 0};
               if (buf->epoch == 0) {
                  ofs[i] = (int) p->optr + 
                           (p->pagelen - 3) * sizeof (float);
               }
               if (ofs[i]) {
                  if (i == 2) {
                     printf ("ofs[2] = %i\n", ofs[i]);
                  }
                  rmRead (_RM_ID, (char*)x, ofs[i], 
                         4 * sizeof (float), 0);
                  printf ("values prev. = %g %g %g %g\n",
                         x[3], x[2], x[1], x[0]);
               }
            }
         */
         }
      #if RMEM_LAYOUT == 1
      /* Interleave the writes to slow network blasting a bit */
      {
      int j, k;
      int sz =  (p->pagelen + 1) * sizeof (float);
      int nc = sz/awgInterleave;
      void *srcptr = (void*) &p->status;
      int destidx = (int) p->optr;
      for(j = 0; j < nc; j++) {
             rmWrite (0, srcptr, destidx, awgInterleave, 0);
      #if TARGET != (TARGET_L1_GDS_AWG1 + 20) && TARGET != (TARGET_L1_GDS_AWG1 + 21)
              if (AWG[i].rmem) rmWrite (1, srcptr, destidx, awgInterleave, 0);
      #endif
         srcptr += awgInterleave;
         destidx += awgInterleave;
      
         /* VME accesses to slow things down */
         for (k = 0; k < awgVmeReadDelay; k++) statusTimingCard();
      /*		*((int *) (ICS115_0_BASE_ADDRESS+0x50008)) = 0;*/
      }
           rmWrite (0, srcptr, destidx, sz - awgInterleave*nc, 0);
           if (AWG[i].rmem) rmWrite (1, srcptr, destidx, sz - awgInterleave*nc, 0);
      }
      #if 0
         rmWrite (0, (void*) &p->status, (int) p->optr, 
                  (p->pagelen + 1) * sizeof (float), 0);
         rmWrite (1, (void*) &p->status, (int) p->optr, 
                  (p->pagelen + 1) * sizeof (float), 0);
      #endif
      #else
         {int i; 
            for (i = 0; i < 5; ++i) 
               rmWrite (_RM_ID, (void*) &p->status, (int) p->optr, 
                       (p->pagelen + 1) * sizeof (float), 0);
	  }
      #endif
         /*{
            if (1 || buf->epoch == 0) {
               float x[4];
               static int ofs[100] = {0, 0, 0, 0, 0, 0};
               if (buf->epoch == 0) {
                  ofs[i] = (int) p->optr + 
                           (p->pagelen - 3) * sizeof (float);
               }
               if (ofs[i]) {
                  rmRead (_RM_ID, (char*)x, ofs[i], 
                         4 * sizeof (float), 0);
                  printf ("values prev. = %g %g %g %g\n",
                         x[3], x[2], x[1], x[0]);
               }
            }
         }*/
      }
   }
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Task Name: tAWGcopy							*/
/*                                                         		*/
/* Procedure Description: task to copy waveforms to RM/DAC		*/
/* 									*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/      
   void tAWGcopy (void) 
   {
      int		i;		/* buffer index */
      awgbuf_t*		buf;		/* buffer pointer */
      awgbuf_t*		tmp;		/* buffer pointer */
      int		count;		/* number of waiting waveforms */
      tainsec_t		time;		/* time of waveform */
      tainsec_t		tstart;		/* start time */
      tainsec_t		tstop;		/* stop time */
      awgpagebuf_t*	p;		/* pointer to awg page */
   #ifdef _AWG_STATISTICS
      double 		dt;		/* time difference */
   #endif
   
      printf("tAWGcopy started\n");

      /* start a copy whenever the ready semaphore is available */
      while (sem_wait (&awgbuf.ready) == 0) {
      
         /* get mutex */
         tstart = TAInow();
         if (awgdebug) {
            printf ("tAWGcopy at %i.%09i\n", (int)(tstart / _ONESEC), 
                   (int)(tstart % _ONESEC));
         }
         MUTEX_GET (awgmux);
      
         /* find oldest buffer which is ready */
         buf = NULL;
         for (i = 0, tmp = awgbuf.bufs; i < _MAX_BUF; i++, tmp++) {
            if (tmp->ready == 1) {
               if ((buf == NULL) || (tmp->time < buf->time) ||
                  ((tmp->time == buf->time) && 
                  (tmp->epoch < buf->epoch))) {
                  buf = tmp;
               }
            }
         }
         /* test whether something was ready */
         if (buf == NULL) {
            MUTEX_RELEASE (awgmux);
            continue;
         }
      
         /* calculate waveform time */
         time = ((tainsec_t) buf->time) * _ONESEC + 
                ((tainsec_t) buf->epoch) * _EPOCH;
      
      	/* check whether we can keep up */
         if (((sem_getvalue (&awgbuf.ready, &count) == 0) &&
            (count > _MAX_BEHIND)) || (time < tstart)) {
            buf->ready = 0;
            MUTEX_RELEASE (awgmux);
            gdsError (GDS_ERR_TIME, 
                     "AWG can't keep up: waveforms discarded");
         #ifdef _AWG_STATISTICS
            MUTEX_GET (statmux);
            awgstat.rmNumCrit++;
            MUTEX_RELEASE (statmux);
         #endif
            continue;
         }
         else {
            /* set ready flag for processing */
            buf->ready = 2;
            MUTEX_RELEASE (awgmux);
         }
      
         /* first copy DAC channels */
      #ifdef _AWG_DAC
         copyDACdata (buf);
      #endif
      
         /* now copy testpoint channels */
      #ifdef _AWG_RM
         copyRMdata (buf);
      #endif
      
         /* flag that we are done */
         MUTEX_GET (awgmux);
         buf->ready = 0;
         /* reset page buffers */
         for (i = 0, p = buf->buf; i < MAX_NUM_AWG; i++, p++) {
            p->otype = (AWG_OutputType) awgNone;
            p->optr = NULL;
            p->onum = -1;
            p->pagelen = 0;
         }
         MUTEX_RELEASE (awgmux);
      
         /* check whether we fell too much behind */
         tstop = TAInow();
         if (time < tstop) {
            gdsError (GDS_ERR_TIME, 
                     "AWG couldn't keep up: waveforms may be incomplete");
         #ifdef _AWG_STATISTICS
            MUTEX_GET (statmux);
            awgstat.rmNumCrit++;
            MUTEX_RELEASE (statmux);
         #endif
         }
      
         /* gather realtime statistics */
      #ifdef _AWG_STATISTICS
         dt = (double) (tstop - tstart);
         if (fabs (dt) < 10.0 * __ONESEC) { /* no outliers */
            MUTEX_GET (statmux);
            _AWG_FILTER (awgstat.rmMean, dt);
            _AWG_FILTER (awgstat.rmStddev, dt*dt);
            if (dt > awgstat.rmMax) {
               awgstat.rmMax = dt;
            }
            dt = (double) (time - tstop);
            if ((awgstat.rmNum == 0) || (dt < awgstat.rmCrit)) {
               awgstat.rmCrit = dt;
            }
            awgstat.rmNum += 1.0;
            MUTEX_RELEASE (statmux);
         }
      #endif
      }
   }


#ifndef _NO_TESTPOINTS
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: readTPinfo					*/
/*                                                         		*/
/* Procedure Description: reads the TP info				*/
/* 									*/
/* Procedure Arguments: tai, epoch					*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int readTPinfo (taisec_t tai, int epoch) 
   {
      int		ret1;		/* return value */
      int		ret2;		/* return value */
   
      /* get the mutex */
      MUTEX_GET (awgmux);
   
      /* copy testpoint info from reflective memory */
      awgtp[1] = awgtp [0];
      ret1 = tpGetIndexDirect (_TP_NODE, TP_LSC_EX_INTERFACE,
                              awgtp[0].lsctp, TP_LSC_EX_NUM, tai, epoch);
      /* printf ("got lsc tp = %i %i %i\n", 
             awgtp[0].lsctp[0], awgtp[0].lsctp[1], awgtp[0].lsctp[2]);*/
      ret2 = tpGetIndexDirect (_TP_NODE, TP_ASC_EX_INTERFACE, 
                              awgtp[0].asctp, TP_ASC_EX_NUM, tai, epoch);
      /* printf ("got asc tp = %i %i %i\n", 
             awgtp[0].asctp[0], awgtp[0].asctp[1], awgtp[0].asctp[2]);*/
      awgtp[0].time = tai;
      awgtp[0].epoch = 0;
   
      /* release the mutex */
      MUTEX_RELEASE (awgmux);
   
      return ((ret1 < ret2) ? ret1 : ret2);
   }
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: getTPslot					*/
/*                                                         		*/
/* Procedure Description: gets a TP slot # and a TP address		*/
/* 									*/
/* Procedure Arguments: type - lsc or asc testpoint			*/
/*                      tpID - testpoint ID				*/
/*                      time - time of waveform				*/
/*                      epoch - epoch of waveform	      		*/
/*                      tpptr - pointer to TP data			*/
/*                                                         		*/
/* Procedure Returns: slot # if successful, <0 otherwise		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int getTPslot (AWG_OutputType type, int tpID, 
                     taisec_t time, int epoch, float** tpptr)
   {
   #ifdef _NO_TESTPOINTS
      return -1;
   
   #else
      awgtp_t*		tp;		/* releavnt testpoint info */
      testpoint_t*	tplist;		/* testpoint list */
      int		tplistlen;	/* length of testpoint list */
      long		tpBase;		/* offset to TP data block */
      long		tpSize;		/* size of TP data block */
      long		tpChnLen;	/* length of tp channel */
      int		i;		/* index */
      int		slot;		/* test point slot */
   
      /* lsc or asc test point ? */
      if (!((type == awgLSCtp) || (type == awgASCtp)) || (tpID <= 0)) {
         return -1;
      }
   
      /* get the mutex */
      MUTEX_GET (awgmux);
   
      /* test whether new or old tp info is needed */
      if ((time >= awgtp[0].time) || 
         ((time == awgtp[0].time) && (epoch >= awgtp[0].epoch))) {
         tp = &awgtp[0];
      }
      else {
         tp = &awgtp[1];
      }
      /* LSC or DAC */
      if (type == awgLSCtp) {
         tplist = tp->lsctp;
         tplistlen = TP_LSC_EX_NUM;
         tpBase = _LSCX_BASE;
         tpSize = _LSCX_SIZE;
         tpChnLen = TP_LSC_CHN_LEN;
      }
      /* ASC */
      else {
         tplist = tp->asctp;
         tplistlen = TP_ASC_EX_NUM;
         tpBase = _ASCX_BASE;
         tpSize = _ASCX_SIZE;
         tpChnLen = TP_ASC_CHN_LEN;
      }
      /* now serach for tp ID in list */
      slot = -1;
      for (i = 0; i < tplistlen; i++) {
         if (tplist[i] == tpID) {
            /* found it */
            slot = i;
            break;
         }
      }
      /* calculate address in reflective memory */
      if ((slot >= 0) && (tpptr != NULL)) {
      #if 0 && RMEM_LAYOUT == 1
         tpBase += slot * TP_DATUM_LEN * tpChnLen;
      #else
         /* Don't forget status word at beginning of channel data! */
         tpBase += slot * TP_DATUM_LEN * (tpChnLen + 1);
      #endif
         *tpptr = (float*) CHN_ADDR (tpBase, tpSize, epoch);
      }
      else if (tpptr != NULL) {
         *tpptr = NULL;
      }
   
      /* release the mutex */
      MUTEX_RELEASE (awgmux);
   
      return slot;
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: invalidateTPslots				*/
/*                                                         		*/
/* Procedure Description: sets the invalid flag of TP slots		*/
/* 									*/
/* Procedure Arguments: epoch - epoch to set invalid	      		*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int invalidateTPslots (int epoch)
   {
   #ifdef _NO_TESTPOINTS
      return -1;
   
   #else
      int		tplistlen;	/* length of testpoint list */
      long		tpBase;		/* offset to TP data block */
      long		tpSize;		/* size of TP data block */
      long		tpChnLen;	/* length of tp channel */
      int		i;		/* index */
      int		slot;		/* test point slot */
      int		tpptr;		/* pointer to test point slot */
      int 		status;		/* channel status flag */
   
      status = _INVALID_RM_CHANNEL;
      for (i = 0; i < 2; i++) {
         /* LSC or DAC */
         if (i == 0) {
            tplistlen = TP_LSC_EX_NUM;
            tpBase = _LSCX_BASE;
            tpSize = _LSCX_SIZE;
            tpChnLen = TP_LSC_CHN_LEN;
         }
         /* ASC */
         else {
            tplistlen = TP_ASC_EX_NUM;
            tpBase = _ASCX_BASE;
            tpSize = _ASCX_SIZE;
            tpChnLen = TP_ASC_CHN_LEN;
         }
         /* go through tp slot list */
         tpptr = CHN_ADDR (tpBase, tpSize, epoch);
         for (slot = 0; slot < tplistlen; slot++) {
         #if RMEM_LAYOUT > 0
            rmWrite (0, (char*) &status, tpptr, TP_DATUM_LEN, 0);
         #if TARGET != (TARGET_L1_GDS_AWG1 + 20) && TARGET != (TARGET_L1_GDS_AWG1 + 21)
            rmWrite (1, (char*) &status, tpptr, TP_DATUM_LEN, 0);
         #endif
         /*printf("inv tpslot 0/1 at 0x%x len=%d values=0x%x\n",
         tpptr, TP_DATUM_LEN, status);*/
         /*	    for (k = 0; k < awgVmeReadDelay; k++)
         *((int *) 0x50201000) = 0;*/
         #else	
            rmWrite (_RM_ID, (char*) &status, tpptr, TP_DATUM_LEN, 0);
         #endif
            /* Don't forget status word at beginning of channel data! */
            tpptr += TP_DATUM_LEN * (tpChnLen + 1);
         }
      }
   
      return 0;
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: getAWGconfigBlock				*/
/*                                                         		*/
/* Procedure Description: returns pointer to AWG coniguration block	*/
/* 									*/
/* Procedure Arguments: ID						*/
/*                                                         		*/
/* Procedure Returns: configBlock if successful, NULL otherwise		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   AWG_ConfigBlock* getAWGconfigBlock (int ID)
   {
      if (ID < 0 || ID >= MAX_NUM_AWG) {
         gdsDebug ("getAWGconfigBlock ID out of range"); 
         return NULL;
      }
      else {
         return &AWG [ID];
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: initAWG					*/
/*                                                         		*/
/* Procedure Description: initializes globals and starts tasks		*/
/* 									*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int initAWG (void) 
   {
      int	i;	/* index */
      int	attr;	/* task creation attributes */
   
      /* test if already initialized */
      if (awginit != 0) {
         return 0;
      }
      /* First call, log version ID */
      printf("awg_server %s\n", versionId) ;

   #ifndef _NO_TESTPOINTS
      if (testpoint_client() < 0) {
         gdsError (GDS_ERR_MISSING, 
                  "Unable to install test point client interface");
         return -1;
      }
   #endif
   
   #ifndef DEBUG_INIT
      /* initialize heartbeat */
      if (installHeartbeat (NULL) != 0) {
         gdsError (GDS_ERR_MISSING, "Unable to install heartbeat");
         return -1;
      }
   
   #ifdef _AWG_RM   
      /* check RM base address */
   #if OS_VXWORKS && (RMEM_LAYOUT != 1)
      if (VMIVME5588_0_BASE_ADDRESS == 0) {
         gdsError (GDS_ERR_MISSING, "Unable to find reflective memory");
         return -2;
      }
   #endif
   
      printf ("LSC base data address = 0x%08x  blocksize = %ik\n",
             _LSCX_BASE, _LSCX_SIZE / 1024);
      printf ("ASC base data address = 0x%08x  blocksize = %ik\n",
             _ASCX_BASE, _ASCX_SIZE / 1024);
   #endif
   
   #ifdef _AWG_DAC
      /* allocate DAC buffer */
   #ifdef _AWG_DAC
   #ifdef OS_VXWORKS
      dacbuf = cacheDmaMalloc (_MAX_BUF * sizeof (dacbuf_t));
   #else
      dacbuf = malloc (_MAX_BUF * sizeof (dacbuf_t));
   #endif
      if (dacbuf == 0) {
         gdsError (GDS_ERR_MEM, "Unable to allocate DAC buffer");
      }
   #endif
   
      /* check DAC base address */
      if (ICS115_0_BASE_ADDRESS == 0) {
         gdsError (GDS_ERR_MISSING, "Unable to find ICS115");
         return -2;
      }
      /* read in DAC channel number */
      if ((DACnumchn <= 0) || (DACnumchn > _MAX_DAC_CHN)) {
         gdsError (GDS_ERR_PRM, 
                  "invalid number of DAC channels");
         return -2;
      }
   #endif
   #endif
   
      /* set awg memory to zero */
      memset ((void*) AWG, 0, MAX_NUM_AWG * sizeof (AWG_ConfigBlock));
      for (i = 0; i < MAX_NUM_AWG; ++i) {
         AWG[i].gain.value = 1.0;
      }
   
      /* create awg mutex's */
      if (MUTEX_CREATE (awgmux) != 0) {
         gdsError (GDS_ERR_MEM, "Unable to create mutex");
         return -3;
      }
      for (i = 0; i < MAX_NUM_AWG; i++) {
         if (MUTEX_CREATE (AWG[i].mux) != 0) {
            gdsError (GDS_ERR_MEM, "Unable to create mutex");
            return -3;
         }
      }
   
      /* create statistics mutex */
   #ifdef _AWG_STATISTICS
      if (MUTEX_CREATE (statmux) != 0) {
         gdsError (GDS_ERR_MEM, "Unable to create mutex");
         return -3;
      }
      (void) getStatisticsAWG (NULL);
   #endif
   
      /* initialize output buffers */
      memset (&awgbuf, 0, sizeof (awgbuf));
      if (sem_init (&awgbuf.ready, 0, 0) != 0) {
         gdsError (GDS_ERR_MISSING, "Unable to initialize AWG semaphore");
         return -4;
      }
      /* initialize stream buffers */
      memset (&awgstream, 0, sizeof (awgstream));
      /* initialize filters */
      memset (&awgfilter, 0, sizeof (awgfilter));
   
      /* initialize dac buffers */
   #ifdef _AWG_DAC
      memset (dacbuf, 0, _MAX_BUF * sizeof (dacbuf_t));
   #endif
   
      /* initialize testpoint info */
   #ifndef _NO_TESTPOINTS
      memset (awgtp, 0, 2 * sizeof (awgtp_t));
   #endif
   
      awgdacTID = awgTID = awgcopyTID = 0;
   #ifndef DEBUG_INIT
   #ifdef _AWG_DAC
      /* initialize DAC buffer mutex */
      if (MUTEX_CREATE (dacmux) != 0) {
         gdsError (GDS_ERR_MEM, "Unable to initialize DAC mutex");
         return -3;
      }
   
      /* initialize DAC ISR semaphore */
      if (sem_init (&semDACisr, 0, 0) != 0) {
         gdsError (GDS_ERR_MEM, "Unable to initialize DAC semaphore");
         return -4;
      }
   
      /* initialize DAC */
      dacstat = STATUSDAC_DOWN;
      if (dacInit (_DAC_ID, &semDACisr) != 0) {
         gdsError (GDS_ERR_MISSING, "Unable to initialize DAC");
         return -5;
      }
   
      /* start DAC ISR task */
   #ifdef OS_VXWORKS
      attr = VX_FP_TASK;
   #else
      attr = PTHREAD_CREATE_DETACHED | PTHREAD_SCOPE_SYSTEM;
   #endif
      if (taskCreate (attr, _AWGDAC_PRIORITY, &awgdacTID, 
         _AWGDAC_TASKNAME, (taskfunc_t) tdacISR, NULL) != 0) {
         gdsError (GDS_ERR_PROG, "Unable to create DAC task");
         return -6;
      }
   #endif
   
      /* start awg task */
   #ifdef OS_VXWORKS
      attr = VX_FP_TASK;
   #else
      attr = PTHREAD_CREATE_DETACHED | PTHREAD_SCOPE_SYSTEM;
   #endif
      if (taskCreate (attr, _AWG_PRIORITY, &awgTID, 
         _AWG_TASKNAME, (taskfunc_t) tAWG, NULL) != 0) {
         gdsError (GDS_ERR_PROG, "Unable to create AWG task");
         return -6;
      }
   
      /* start copy task */
      awgcopyTID = 0;
   #ifdef OS_VXWORKS
      attr = VX_FP_TASK;
   #else
      attr = PTHREAD_CREATE_DETACHED | PTHREAD_SCOPE_SYSTEM;
   #endif
      if (taskCreate (attr, _AWGCOPY_PRIORITY, &awgcopyTID, 
         _AWGCOPY_TASKNAME, (taskfunc_t) tAWGcopy, NULL) != 0) {
         gdsError (GDS_ERR_PROG, "Unable to create AWG copy task");
         return -6;
      }
   #endif
   
      /* set initialization flag */
      awginit = 1;
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: finiAWG					*/
/*                                                         		*/
/* Procedure Description: cleans up globals				*/
/* 									*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   void finiAWG (void)
   {
      /* test if initialized */
      if (awginit == 0) {
         return;
      }
   
      /* reset AWG */
      resetAllAWG ();
   
      /* :TODO: would need more cleanup! */
   
      /* clear dac memory */
   #ifdef _AWG_DAC
      free (dacbuf);
      dacbuf = 0;
   #endif
      /* set initialization flag */
      awginit = 0;
      return;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: checkAWG					*/
/*                                                         		*/
/* Procedure Description: checks if tasks are still alive tasks		*/
/* 									*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: true if running, false otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int checkAWG (void) 
   {
      int ok = 1;
   #ifdef OS_VXWORKS
      if ((awgTID != 0) && taskIsSuspended (awgTID)) {
         ok = 0;
      }
      if ((awgcopyTID != 0) && taskIsSuspended (awgcopyTID)) {
         ok = 0;
      }
      if ((awgdacTID != 0) && taskIsSuspended (awgdacTID)) {
         ok = 0;
      }
   #endif
      return ok;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: restartAWG					*/
/*                                                         		*/
/* Procedure Description: restarts suspended AWG tasks			*/
/* 									*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int restartAWG (void) 
   {
   #ifdef OS_VXWORKS
      if ((awgTID != 0) && taskIsSuspended (awgTID)) {
         taskRestart (awgTID);
      }
      if ((awgcopyTID != 0) && taskIsSuspended (awgcopyTID)) {
         taskRestart (awgcopyTID);
      }
      if ((awgdacTID != 0) && taskIsSuspended (awgdacTID)) {
         taskRestart (awgdacTID);
      }
   #endif
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: awgLock         				*/
/*                                                         		*/
/* Procedure Description: make all current waveform (un)breakable	*/
/* 									*/
/* Procedure Arguments: 1 - lock; 0 - unlock;      			*/
/*                                                         		*/
/* Procedure Returns: nothing                              		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   void awgLock (unsigned int lock) {
      int i;
      for (i = 0; i < MAX_NUM_AWG; i++) {
         MUTEX_GET (AWG[i].mux);
         AWG[i].unbreakable = lock && (AWG[i].status & AWG_IN_USE);
         MUTEX_RELEASE (AWG[i].mux);
      }
   }

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: getIndexAWG					*/
/*                                                         		*/
/* Procedure Description: gets an unsued awg slot			*/
/* 									*/
/* Procedure Arguments: channel type, id, arg1 arg2			*/
/*                                                         		*/
/* Procedure Returns: awg index if successful, <0 otherwise		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int getIndexAWG (AWG_OutputType chntype, int id,
                   int arg1, int arg2)
   {
      int		i;		/* awg index */
      int		ifree;		/* awg index to free slot */
      void*		optr;		/* output pointer */
      int		chnnum;		/* channel number */
      int		pagesize;	/* page size */
      tainsec_t		delay;		/* delay in nsec */
   
      /* test valid channel type and valid id range */
      optr = NULL;
      switch (chntype) 
      {
         case awgLSCtp:
            {
               chnnum = id;
               if (TP_ID_TO_INTERFACE(id) != TP_LSC_EX_INTERFACE) {
                  return -1;
               }
               pagesize = _LSCTP_PAGE; 
               delay = arg1;
               break;
            }
         case awgASCtp:
            {
               chnnum = id;
               if (TP_ID_TO_INTERFACE(id) != TP_ASC_EX_INTERFACE) {
                  return -1;
               }
               pagesize = _ASCTP_PAGE; 
               delay = arg1;
               break;
            }
         case awgDAC:
            {
               chnnum = id - TP_ID_DAC_OFS;
               if (TP_ID_TO_INTERFACE(id) != TP_DAC_INTERFACE) {
                  return -1;
               }
               pagesize = _DAC_PAGE;
               delay = arg1;
               break;
            }
         case awgDS340:
            {
               chnnum = id - TP_ID_DS340_OFS;
               if (TP_ID_TO_INTERFACE(id) != TP_DS340_INTERFACE) {
                  return -1;
               }
               pagesize = 0; 
               delay = arg1;
               break;
            }
         case awgMem:
            {

	printf("unsupported\n");
	abort();
#if 0
               /* id = channel address */
               optr = (void*) id;
               /* argument 1 = block size; stored as ID */
               chnnum = arg1;
               /* argument 2 = page size */
               pagesize = arg2;
               delay = 0;
               break;
#endif
            }
         case awgFile:
         case awgNone:
         default:
            {
               return -2;
            }
      }
   
      /* search if already in list */
      ifree = -3;
      for (i = 0; i < MAX_NUM_AWG; i++) {
         MUTEX_GET (AWG[i].mux);
         if ((AWG[i].status & AWG_IN_USE) == 0) {
            /* free slot */
            ifree = i;
         }
         else {
            /* occupied slot: check if the same */
            if ((AWG[i].otype == chntype) &&
               (AWG[i].ID == chnnum) &&
               (AWG[i].pagesize == pagesize)) {
               /* duplicate */
               MUTEX_RELEASE (AWG[i].mux);
               return i;
            }
         }
         MUTEX_RELEASE (AWG[i].mux);
      }
   
      /* not found: check if free slot avaialable */
      if (ifree >= 0) {
         MUTEX_GET (AWG[ifree].mux);
         AWG[ifree].otype = chntype;
         AWG[ifree].delay = delay;
         AWG[ifree].ID = chnnum;
      #if RMEM_LAYOUT > 0
         AWG[ifree].rmem = TP_ID_TO_RMEM(id);
      #endif
         AWG[ifree].optr = optr;
         AWG[ifree].oname[0] = '\0';
         AWG[ifree].pagesize = pagesize;
         AWG[ifree].ncomp = 0;
         memset (&AWG[ifree].gain, 0, sizeof (AWG_Gain));
         AWG[ifree].gain.value = 1.0;
         AWG[ifree].status = AWG_CONFIG | AWG_IN_USE | AWG_ENABLE;
         AWG[ifree].unbreakable = 0;
         MUTEX_RELEASE (AWG[ifree].mux);
      }
      /* return */
      return ifree;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: releaseIndexAWG				*/
/*                                                         		*/
/* Procedure Description: markes an awg slot as unused			*/
/* 									*/
/* Procedure Arguments: awg index					*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int releaseIndexAWG (int ID)
   {
      int		i;	/* awg index */
   
      if (ID == -1) {
         for (i = 0; i < MAX_NUM_AWG; i++) {
            releaseIndexAWG (i);
         }
         return 0;
      }
   
      if ((ID < 0) || (ID >= MAX_NUM_AWG)) {
         return -1;
      }
   
      if (AWG[ID].unbreakable) 
         return 0;
   
      /* mark as free */
      MUTEX_GET (AWG[ID].mux);
      AWG[ID].status = 0;
      AWG[ID].otype = (AWG_OutputType) awgNone;
      AWG[ID].delay = 0;
      AWG[ID].ID = 0;
      strcpy (AWG[ID].oname, "");
      AWG[ID].optr = NULL;
      AWG[ID].pagesize = 0;
      AWG[ID].ncomp = 0;
      free (AWG[ID].wave);
      AWG[ID].wave = NULL;
      AWG[ID].wavelen = 0;
      memset (&AWG[ID].gain, 0, sizeof (AWG_Gain));
      AWG[ID].gain.value = 1.0;
      AWG[ID].unbreakable = 0;
      awgfilter[ID].valid = 0;
      MUTEX_RELEASE (AWG[ID].mux);
   
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: flushAWG					*/
/*                                                         		*/
/* Procedure Description: sets the output buffer parameters		*/
/*                                                         		*/
/* Procedure Arguments: AWG index, time, epoch				*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int flushAWG (int ID, taisec_t time, int epoch) 
   {
      int 		n;
      float*		vp;
      awgpagebuf_t*	pbuf;
   
      /* check index */
      if ((ID < 0) || (ID >= MAX_NUM_AWG)) {
         return -1;
      }
   
      /* set the ouput waveform buffer page */
      pbuf = &awgbuf.bufs[epoch % _MAX_BUF].buf[ID];
   
      /* flush waveform to output */
      switch (AWG[ID].otype) 
      {
         case awgFile:
            {
               /* write waveform to file */
               vp = pbuf->page;
               if (AWG[ID].optr != NULL) {
                  for (n = 0; n < AWG[ID].pagesize; ++n) {
                     fprintf ((FILE*) AWG[ID].optr, "%9g\n", vp[n]);
                  }
               }
               break;
            }
         case awgLSCtp:
         case awgASCtp:
            {
               /* complete buffer information */
               if (getTPslot (AWG[ID].otype, AWG[ID].ID, 
                  time, epoch, &pbuf->optr) >= 0) {
                  pbuf->otype = AWG[ID].otype;
                  pbuf->onum = AWG[ID].ID;
                  pbuf->pagelen = AWG[ID].pagesize;
               }
               else {
                  pbuf->otype = (AWG_OutputType) awgNone;
               }
               break;
            }
         case awgDAC:
            {
               /* complete buffer information */
               pbuf->otype = awgDAC;
               pbuf->onum = AWG[ID].ID;
               pbuf->pagelen = AWG[ID].pagesize;
               pbuf->optr = NULL;
               /* copy into LSC testpoint as well? */
               if (getTPslot (awgLSCtp, TP_ID_DAC_OFS + AWG[ID].ID, 
                  time, epoch, &pbuf->optr) < 0) {
                  pbuf->optr = NULL;
               }
               break;
            }
         case awgDS340:
            {
               /* complete buffer information */
               if (AWG[ID].ncomp > 0) {
                  pbuf->otype = awgDS340;
                  pbuf->onum = AWG[ID].ID;
                  memcpy ((void*) pbuf->page, (void*) &AWG[ID].comp[0],
                         sizeof (AWG_Component));
                  pbuf->pagelen = 0;
                  pbuf->optr = NULL;
               }
               else {
                  pbuf->otype = (AWG_OutputType) awgNone;
               }
               break;
            }
         case awgMem:
            {
	printf("unsupported\n");
	abort();
#if 0
               /* complete buffer information */
               pbuf->otype = awgMem;
               pbuf->onum = 0;
               pbuf->pagelen = AWG[ID].pagesize;
               pbuf->optr = (float*) CHN_ADDR ((int) AWG[ID].optr, 
                                              AWG[ID].ID, epoch);
#endif
               break;
            }
         case awgNone:
         default:
            {
               pbuf->otype = (AWG_OutputType) awgNone;
               break;
            }
      }
   
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: resetAWG					*/
/*                                                         		*/
/* Procedure Description: flushes local buffer and deconfigures AWG	*/
/*                                                         		*/
/* Procedure Arguments: AWG index					*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int resetAWG (int ID) 
   {
      if ((ID < 0) || (ID >= MAX_NUM_AWG)) {
         gdsDebug ("resetAWG() ID out of range");
         return -1; 
      }
   
      /* get the mux */
      MUTEX_GET (AWG[ID].mux);
   
      if (AWG[ID].unbreakable) {
         MUTEX_RELEASE (AWG[ID].mux);
         return 0;
      }
   
      /* make sure the ouput waveform is set to zero */
      switch (AWG[ID].otype) 
      {
         case awgFile :
            {
               /* close the file */
               if (AWG[ID].optr != NULL) {
                  fclose(AWG[ID].optr);
                  AWG[ID].optr = NULL;
               }
               break;
            }
         case awgDS340:
            {
               /* turn DS340 off */
               break;
            }
         default:
            {
               /* nothing to do */
               break;
            }
      }
   
      /* remove waveform components and disable output */
      AWG[ID].ncomp = 0;
      free (AWG[ID].wave);
      AWG[ID].wave = NULL;
      AWG[ID].wavelen = 0;
      /* awgfilter[ID].valid = 0; */
      /* AWG[ID].status &= ~((long) AWG_ENABLE); */
   
      /* release the mux */
      MUTEX_RELEASE (AWG[ID].mux);
   
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: openConfigFile				*/
/*                                                         		*/
/* Procedure Description: Opens filename or PRM_FILE			*/
/* 									*/
/* Called by: configAWG and configAllAWG                       		*/
/*                                                         		*/
/* Procedure Arguments: base filename					*/
/*                                                         		*/
/* Procedure Returns: FILE*						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   FILE* openConfigFile (const char *filename) 
   {
      FILE*		fp;
      char 		cbuf[128];
   
      if ((filename == NULL) || !(fp = fopen (filename, "r"))) {
         fp = fopen (PRM_FILE, "r");
         if (fp) {
            sprintf (cbuf, "openConfigFile() %s\n", PRM_FILE);
            gdsDebug (cbuf);
         }
      } 
      return fp;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: configAWG					*/
/*                                                         		*/
/* Procedure Description: configures AWG[ID] from filename		*/ 
/*                                                         		*/
/* Procedure Arguments: AWG index and parameter filename		*/
/*                                                         		*/
/* Procedure Returns: GDS_ERROR						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int configAWG (int ID, const char *filename) 
   {
      FILE*		fp;
      int 		err;
   
      if ((ID < 0) || (ID >= MAX_NUM_AWG)) {
         gdsDebug ("configAWG() ID out of range");
         return GDS_ERR_PRM; 
      }
   
      if (filename != NULL) {
         fp = openConfigFile (filename);
      }
      else {
         fp = openConfigFile (AWG_CONFIG_FILE);
      }
   
      if (fp == NULL) {
         gdsDebug("configAWG() No file"); 
         return GDS_ERR_MISSING;
      }
   
      err = configAWGfp (ID, fp);
      fclose (fp);
      return err;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: configAWGfp					*/
/*                                                         		*/
/* Procedure Description: configures AWG[ID] from FILE*			*/
/*                                                         		*/
/* Called by: configAWG and configAllAWG                       		*/
/*                                                         		*/
/* Procedure Arguments: AWG index and parameter file ptr		*/
/*                                                         		*/
/* Procedure Returns: GDS_ERROR						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int configAWGfp (int ID, FILE* fp) 
   {
      AWG_ConfigBlock*		awg;
      int			j;
      int 			nentry;
      int			cursor;
      char 			cbuf[PARAM_ENTRY_LEN];
      char*			vp;
      char*			sec;
      int 			n;
   
      if (ID < 0 || ID >= MAX_NUM_AWG) {
         gdsDebug ("configAWGfp() ID out of range"); 
         return GDS_ERR_PRM;
      }
   
      /* make sure awg is reseted */
      resetAWG (ID);
   
      /*--- Look for appropriate section in parameter file ---*/
      sprintf (cbuf, "AWG%d", ID);
      if (!(sec = getParamFileSection (fp, cbuf, &nentry, 0))) {
         resetAWG (ID);
         return GDS_ERR_MISSING;
      }
   
      /* get the mux */
      MUTEX_GET (AWG[ID].mux);
      awg = AWG + ID;
   
      /*--- Mandatory parameters ---*/
   
      /*** pagesize ***/
      vp = getParamSectionEntry ("pagesize", sec, nentry, &cursor);
      if (!vp) {
         gdsDebug ("configAWGfp() no pagesize");
         MUTEX_RELEASE (AWG[ID].mux);
         resetAWG (ID);
         return GDS_ERR_FORMAT;
      }
      awg->pagesize = atoi(vp);
   
      /*** output ***/
      vp = getParamSectionEntry ("output", sec, nentry, &cursor);
      if (vp && sscanf (vp, "%s", cbuf)) { 
         /*
         value field should be of the form <type> <name>, eg.
         "file wave2.out"
         "testpoint 2"
         */
      
         if (gds_strncasecmp (cbuf, "none", 4) == 0) {
            MUTEX_RELEASE (AWG[ID].mux);
            resetAWG (ID);
            return GDS_ERR_NONE;
         }
         /* FILE */ 
         else if (gds_strncasecmp (cbuf, "file", 4) == 0 &&
                 sscanf (vp, "%*s %s", awg->oname) &&
                 (awg->optr = fopen (awg->oname, "w"))) { 
            awg->otype = awgFile;
            awg->delay = 0;
            awg->ID = 0;
         }
         /* lsc test point */	 
         else if ((gds_strncasecmp (cbuf, "lsctestpoint", 12) == 0 ||
                 gds_strncasecmp (cbuf, "lsctp", 5) == 0) &&
                 sscanf (vp, "%*s %d", &n) &&
                 n >= 0) {
            awg->otype = awgLSCtp;
            awg->delay = 0;
            awg->ID = n;
            AWG[ID].optr = NULL;
            sprintf (awg->oname, "Testpoint %d", n);
         }
         
         /* Bad output spec */ 
         else {
            sprintf (cbuf, "configAWGfp() bad format: %s ", vp);
            gdsDebug (cbuf);
            MUTEX_RELEASE (AWG[ID].mux);
            resetAWG (ID);
            return GDS_ERR_FORMAT;
         }
      }
      MUTEX_RELEASE (AWG[ID].mux);
   
      /*--- optional paramters ---*/
   
      /*--- load components ---*/
      /*--- start with comp0 and continue until first missing comp */
      for (j = 0; j < MAX_NUM_AWG_COMPONENTS; j++) {
         sprintf (cbuf, "comp%d", j);
         if ((vp = getParamSectionEntry (cbuf, sec, nentry, &cursor))) {
            if (addCompAWG (ID, vp) < 0) {
               break;
            }
         }
      }
   
      if (checkConfigAWG (ID)) {
         gdsDebug ("configAWG() bad config");
         resetAWG (ID);
         return GDS_ERR_FORMAT;
      } 
   
      return GDS_ERR_NONE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: showAWG					*/
/*                                                         		*/
/* Procedure Description: prints AWG[ID] configuration.			*/
/* 									*/
/* Procedure Arguments: AWG index and output string			*/
/*                                                         		*/
/* Procedure Returns: GDS_ERROR						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int showAWG (int ID, char* s, int max) {
      AWG_ConfigBlock*	awg;
      int 		i;
      char		buf[1024]; 	/* temp buffer */
      char* 		p; 		/* cursor */
      int		size;		/* written so far */
   
      if (ID < 0 || ID >= MAX_NUM_AWG) {
         gdsDebug ("showAWG() ID out of range"); 
         return GDS_ERR_PRM;
      }
      if (s == NULL) {
         gdsDebug ("showAWG() no output file"); 
         return GDS_ERR_FILE;
      }
   
      awg = AWG + ID;
      max--; /* subtract 0 character */
      size = 0; p = s;
      /* slot number */
      sprintf (buf, "\n=== AWG%d ===\n", ID);
      p = strencpy (p, buf, max - size); 
      size = (size + strlen (buf) > max) ? max : size + strlen (buf);
      /* status */
      sprintf (buf, "status: " );
      p = strencpy (p, buf, max - size); 
      size = (size + strlen (buf) > max) ? max : size + strlen (buf);
      if (awg->status & AWG_CONFIG) {
         sprintf (buf, "CONFIG     ");
         p = strencpy (p, buf, max - size); 
         size = (size + strlen (buf) > max) ? max : size + strlen (buf);
      }
      else {
         sprintf (buf, "NO-CONFIG  ");
         p = strencpy (p, buf, max - size); 
         size = (size + strlen (buf) > max) ? max : size + strlen (buf);
      }
      if (awg->status & AWG_ENABLE) {
         sprintf (buf, "ENABLE ");
         p = strencpy (p, buf, max - size); 
         size = (size + strlen (buf) > max) ? max : size + strlen (buf);
      }
      else {
         sprintf (buf, "DISABLE");
         p = strencpy (p, buf, max - size); 
         size = (size + strlen (buf) > max) ? max : size + strlen (buf);
      }
      sprintf (buf, "             0x%lx\n", awg->status);
      p = strencpy (p, buf, max - size); 
      size = (size + strlen (buf) > max) ? max : size + strlen (buf);
      /* global parameters */
      sprintf (buf, "gain:  %s%-10g ramptime: %-9g delay: %-9.6f\n",
              (awg->gain.state ? "*" : " "), 
              (awg->gain.state == 0 ? awg->gain.value : 
              (awg->gain.state == 1 ? awg->gain.old : awg->gain.current)), 
              (double)awg->gain.ramptime / __ONESEC, 
              (double)awg->delay / __ONESEC);
      p = strencpy (p, buf, max - size); 
      size = (size + strlen (buf) > max) ? max : size + strlen (buf);
      /* output */
      sprintf (buf, "output: ");
      p = strencpy (p, buf, max - size); 
      size = (size + strlen (buf) > max) ? max : size + strlen (buf);
      switch (AWG[ID].otype) 
      {
         case awgFile:
            {
               sprintf (buf, "FILE %-26s ", awg->oname); 
               p = strencpy (p, buf, max - size); 
               size = (size + strlen (buf) > max) ? max : size + strlen (buf);
               break;
            }
         case awgLSCtp:
            {
               sprintf (buf, "LSC TP     %-20i", awg->ID); 
               p = strencpy (p, buf, max - size); 
               size = (size + strlen (buf) > max) ? max : size + strlen (buf);
               break;
            }
         case awgASCtp:
            {
               sprintf (buf, "ASC TP     %-20i", awg->ID); 
               p = strencpy (p, buf, max - size); 
               size = (size + strlen (buf) > max) ? max : size + strlen (buf);
               break;
            }
         case awgDAC:
            {
               sprintf (buf, "DAC        %-20i", awg->ID); 
               p = strencpy (p, buf, max - size); 
               size = (size + strlen (buf) > max) ? max : size + strlen (buf);
               break;
            }
         case awgDS340:
            {
               sprintf (buf, "DS340      %-20i", awg->ID); 
               p = strencpy (p, buf, max - size); 
               size = (size + strlen (buf) > max) ? max : size + strlen (buf);
               break;
            }
         case awgMem:
            {
               sprintf (buf, "MEMORY     0x%-18lx", (long) awg->optr); 
               p = strencpy (p, buf, max - size); 
               size = (size + strlen (buf) > max) ? max : size + strlen (buf); 
               break;
            }
         case awgNone :
         default :
            {
               sprintf (buf, "undefined  %-20s", ""); 
               p = strencpy (p, buf, max - size); 
               size = (size + strlen (buf) > max) ? max : size + strlen (buf);
               break;
            }
      }
      sprintf (buf, "pagesize: %-9d\n", awg->pagesize);
      p = strencpy (p, buf, max - size); 
      size = (size + strlen (buf) > max) ? max : size + strlen (buf);
      /* components */
      sprintf (buf, "component list:\n");
      p = strencpy (p, buf, max - size); 
      size = (size + strlen (buf) > max) ? max : size + strlen (buf);
      for (i = 0; i < awg->ncomp; i++) {
         if (!awg->comp[i].wtype) 
            continue;
         sprintf (buf, "  comp[%02d]", i);
         p = strencpy (p, buf, max - size); 
         size = (size + strlen (buf) > max) ? max : size + strlen (buf);
         switch (awg->comp[i].wtype) {
            case awgNone : 
               {
                  sprintf (buf, "  wtype: %-8s", "None");
                  p = strencpy (p, buf, max - size); 
                  size = (size + strlen (buf) > max) ? max : size + strlen (buf);
                  break;
               }
            case awgSine : 
               {
                  sprintf (buf, "  wtype: %-8s", "Sine");  
                  p = strencpy (p, buf, max - size); 
                  size = (size + strlen (buf) > max) ? max : size + strlen (buf);
                  break;
               }
            case awgSquare :
               {
                  sprintf (buf, "  wtype: %-8s", "Square");  
                  p = strencpy (p, buf, max - size); 
                  size = (size + strlen (buf) > max) ? max : size + strlen (buf);
                  break;
               }
            case awgRamp :
               {
                  sprintf (buf, "  wtype: %-8s", "Ramp");  
                  p = strencpy (p, buf, max - size); 
                  size = (size + strlen (buf) > max) ? max : size + strlen (buf);
                  break;
               }
            case awgTriangle :
               {
                  sprintf (buf, "  wtype: %-8s", "Triangle");  
                  p = strencpy (p, buf, max - size); 
                  size = (size + strlen (buf) > max) ? max : size + strlen (buf);
                  break;
               }
            case awgImpulse :
               {
                  sprintf (buf, "  wtype: %-8s", "Impulse");  
                  p = strencpy (p, buf, max - size); 
                  size = (size + strlen (buf) > max) ? max : size + strlen (buf);
                  break;
               }
            case awgConst :
               {
                  sprintf (buf, "  wtype: %-8s", "Const");  
                  p = strencpy (p, buf, max - size); 
                  size = (size + strlen (buf) > max) ? max : size + strlen (buf);
                  break;
               }
            case awgNoiseN :
               {
                  sprintf (buf, "  wtype: %-8s", "Noise(N)");  
                  p = strencpy (p, buf, max - size); 
                  size = (size + strlen (buf) > max) ? max : size + strlen (buf);
                  break;
               }
            case awgNoiseU :
               {
                  sprintf (buf, "  wtype: %-8s", "Noise(U)");  
                  p = strencpy (p, buf, max - size); 
                  size = (size + strlen (buf) > max) ? max : size + strlen (buf);
                  break;
               }
            case awgArb :
               {
                  sprintf (buf, "  wtype: %-8s", "Arb");  
                  p = strencpy (p, buf, max - size); 
                  size = (size + strlen (buf) > max) ? max : size + strlen (buf);
                  break;
               }
            case awgStream :
               {
                  sprintf (buf, "  wtype: %-8s", "Stream");  
                  p = strencpy (p, buf, max - size); 
                  size = (size + strlen (buf) > max) ? max : size + strlen (buf);
                  break;
               }
            default :
               {
                  sprintf (buf, "  wtype: %-8s", "Error");  
                  p = strencpy (p, buf, max - size); 
                  size = (size + strlen (buf) > max) ? max : size + strlen (buf);
                  break;
               }
         }
         sprintf (buf, "   par[]:  %-9g %-9g %-9g %-9g\n",
                 awg->comp[i].par[0], awg->comp[i].par[1],
                 awg->comp[i].par[2], awg->comp[i].par[3]);
         p = strencpy (p, buf, max - size); 
         size = (size + strlen (buf) > max) ? max : size + strlen (buf);
         sprintf (buf, "                              " 
                 "time:   %-19g %-9g %-9g\n",
                 fmod ((double) awg->comp[i].start / __ONESEC, 1E3),
                 ((awg->comp[i].duration == -1) ? -1.0 :
                 (double) awg->comp[i].duration / __ONESEC),
                 (double) awg->comp[i].restart / __ONESEC);
         p = strencpy (p, buf, max - size); 
         size = (size + strlen (buf) > max) ? max : size + strlen (buf);
         sprintf (buf, "                              " 
                 "rampt:  %-9g %-9g\n",
                 (double) awg->comp[i].ramptime[0] / __ONESEC,
                 (double) awg->comp[i].ramptime[1] / __ONESEC);
         p = strencpy (p, buf, max - size); 
         size = (size + strlen (buf) > max) ? max : size + strlen (buf);
         sprintf (buf, "                              " 
                 "ramp:   %-9d %-9d %-9d\n", 
                 PHASE_IN_TYPE(awg->comp[i].ramptype),
                 PHASE_OUT_TYPE(awg->comp[i].ramptype),
                 SWEEP_OUT_TYPE(awg->comp[i].ramptype));
         p = strencpy (p, buf, max - size); 
         size = (size + strlen (buf) > max) ? max : size + strlen (buf);
         sprintf (buf, "                              " 
                 "ramp[]: %-9g %-9g %-9g %-9g\n",
                 awg->comp[i].ramppar[0], awg->comp[i].ramppar[1],
                 awg->comp[i].ramppar[2], awg->comp[i].ramppar[3]);
         p = strencpy (p, buf, max - size); 
         size = (size + strlen (buf) > max) ? max : size + strlen (buf);
      }
      /* filter */
      if (awgfilter[ID].valid) {
         sprintf (buf, "filter: gain     %-12g\n", awgfilter[ID].gain);
         p = strencpy (p, buf, max - size); 
         size = (size + strlen (buf) > max) ? max : size + strlen (buf);
         for (i = 0; i < awgfilter[ID].nsos; ++i) {
            sprintf (buf, "        sos[%02d]  %-12g %-12g %-12g %-12g\n", i,
                    awgfilter[ID].sos[i][0], awgfilter[ID].sos[i][1],
                    awgfilter[ID].sos[i][2], awgfilter[ID].sos[i][3]);
            p = strencpy (p, buf, max - size); 
            size = (size + strlen (buf) > max) ? max : size + strlen (buf);
         }
      }
   
      return GDS_ERR_NONE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: processAWG					*/
/*                                                         		*/
/* Procedure Description: writes a page of waveform to local buffer	*/
/* 									*/
/* Procedure Arguments: AWG index					*/
/*                                                         		*/
/* Procedure Returns: GDS_ERROR						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int processAWG (int ID, taisec_t time, int epoch) {
      AWG_ConfigBlock*	awg;		/* pointer to awg */
      int 		cid;		/* index into components */
      int		invCount;	/* counts invalid components */
      tainsec_t		delay;		/* channel delay (fine adjust) */
   
      /* verify valid ID */
      if (ID < 0 || ID >= MAX_NUM_AWG) {
         gdsDebug ("processAWG() ID out of range"); 
         return -1;
      }
   
      /* get mutex */
      awg = AWG + ID;
      MUTEX_GET (awg->mux);
   
      /* check whether anything to do */
      if (((awg->status & AWG_CONFIG) == 0) || 
         ((awg->status & AWG_ENABLE) == 0)) {
         MUTEX_RELEASE (awg->mux);
         return 0;
      }
   
      /* get time delay of channel */
      delay = awg->delay;
   
      /* zero buffer */
      memset ((void*) _GET_OUTPUT_BUFFER (epoch, ID), 0, 
             _MAX_PAGE * sizeof (float));
   
      /* cycle through awg components */
      invCount = 0;
      for (cid = 0; cid < awg->ncomp; cid++) {
         /* process waveform */
         invCount += calcAWGComp (ID, cid, time, epoch, delay);
      	 /* exit after a couple of invalid/not yet ready waveforms */
         if (invCount < -MAX_INVALID_COMPONENTS) {
            break;
         }
      }
      /* compute filter */
      if (awgfilter[ID].valid) {
         calcAWGFilter (ID, time, epoch, delay);
      }
      /* compute gain */
      calcAWGGain (ID, time, epoch, delay);
   
      /* set output buffer parameters and return */
      flushAWG (ID, time, epoch);
      awg->tproc = (tainsec_t) time * _ONESEC + 
                   (tainsec_t) epoch * _EPOCH;
      MUTEX_RELEASE (awg->mux);
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: disableAWG					*/
/*                                                         		*/
/* Procedure Description: unsets AWG_ENABLE bit in status register	*/
/* 									*/
/* Procedure Arguments: AWG index					*/
/*                                                         		*/
/* Procedure Returns: GDS_ERROR						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int disableAWG (int ID) 
   {
      if (ID < 0 || ID >= MAX_NUM_AWG) {
         gdsDebug ("disableAWG() ID out of range"); 
         return GDS_ERR_PRM;
      }
   
      /* get the mutex and change the status */
      MUTEX_GET (AWG[ID].mux);
      AWG[ID].status &= ~((long) AWG_ENABLE);
      MUTEX_RELEASE (AWG[ID].mux);
   
      return GDS_ERR_NONE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: enableAWG					*/
/*                                                         		*/
/* Procedure Description: sets AWG_ENABLE bit in status register	*/
/* 									*/
/* Procedure Arguments: AWG index					*/
/*                                                         		*/
/* Procedure Returns: GDS_ERROR						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int enableAWG (int ID) 
   {
      if (ID < 0 || ID >= MAX_NUM_AWG) {
         gdsDebug ("enableAWG() ID out of range"); 
         return GDS_ERR_PRM;
      }
   
      /* get the mutex */
      MUTEX_GET (AWG[ID].mux);
   
      if (!(AWG[ID].status & AWG_CONFIG)) {
         MUTEX_RELEASE (AWG[ID].mux);
         return GDS_ERR_FORMAT;
      }
   
      /* change the status */
      AWG[ID].status |= AWG_ENABLE;
      MUTEX_RELEASE (AWG[ID].mux);
   
      return GDS_ERR_NONE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: addWaveformAWG				*/
/*                                                         		*/
/* Procedure Description: adds waveforms to an AWG			*/
/* 									*/
/* Procedure Arguments: awg index, component list, number of comp.	*/
/*                                                         		*/
/* Procedure Returns: number of waveforms added if successful, 		*/
/*		      <0 otherwise					*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int addWaveformAWG (int ID, AWG_Component* comp, int numComp)
   {
      AWG_ConfigBlock*	awg;		/* pointer to awg */
      int 		cid;		/* index into components */
      int		ncopy;		/* number of copied comp. */
      AWG_Component*	cp;		/* pointer to component */
      tainsec_t		t;		/* current time */
      double		dt;		/* time spacing */
      double 		f1;		/* lower ferquency */
      double 		f2;		/* upper ferquency */
   
      /* verify valid ID */
      if ((ID < 0) || (ID >= MAX_NUM_AWG)) {
         gdsDebug ("addWaveformAWG() ID out of range"); 
         return -1;
      }
   
      /* get mutex */
      awg = AWG + ID;
      MUTEX_GET (awg->mux);
   
      if (awg -> unbreakable) {
         MUTEX_RELEASE (awg->mux);
         return 0;
      }
   
      /* test validity */
      if (((awg->status & AWG_CONFIG) == 0) ||
         ((awg->status & AWG_IN_USE) == 0)) {
         MUTEX_RELEASE (awg->mux);
         return -10;
      }
      t = awg->tproc + _EPOCH;
      MUTEX_RELEASE (awg->mux);
   

      /* test whether valid waveforms were provided */
      for (cid = 0, cp = comp; cid < numComp; ++cid, ++cp) {

 	//cp->start = TAInow();

	 printf("component %d; start=%ld localtime now=%ld\n", cid, cp->start, TAInow());

         /* start now if start <= 0 */
         if (cp->start <= 0) {
            cp->start = t + _EPOCH;
            if ((cp->wtype == awgSine) || (cp->wtype == awgRamp) ||
               (cp->wtype == awgSquare) || (cp->wtype == awgTriangle)) {
               cp->par[2] -= fmod (TWO_PI * cp->par[1] * (double) 
                                  (cp->start % _ONEDAY) / __ONESEC, 
                                  TWO_PI);
            }
         }
         /* test validity of component */
         if (!awgIsValidComponent (cp)) {
           cp->wtype = (AWG_WaveType) awgNone;
	   printf("invalid\n");
         }
         /* fix start value if necessary */
         if (cp->wtype == awgNone) {
            /* this makes sure that invalid components are put at the 
               end of the component list */
            cp->start = 0x7FFFFFFFFFFFFFFFLL;
         }
         if ((cp->wtype != awgNone) && (cp->restart > 0) &&
            (cp->duration > 0) && (cp->start + cp->restart < t)) {
            cp->start += cp->restart * ((t - cp->start) / cp->restart);
         }
      }
   
      /* sort list */
      awgSortComponents (comp, numComp);
   
      /* add to awg list; only insert valid components */
      ncopy = 0; cid = 0;
      while ((ncopy < numComp) && 
            (awg->ncomp < MAX_NUM_AWG_COMPONENTS) &&
            (comp[ncopy].wtype != awgNone)) {
         MUTEX_GET (awg->mux);
      	 /* look for insertion place */
         cid = awgBinarySearch (awg->comp, awg->ncomp, comp + ncopy);
      	 /* make space for new component */
         if (cid < awg->ncomp) {
            memmove (awg->comp + cid + 1, awg->comp + cid, 
                    (awg->ncomp - cid) * sizeof (AWG_Component));
            memmove (awg->rb + cid + 1, awg->rb + cid, 
                    (awg->ncomp - cid) * sizeof (randBlock));
         }
      	 /* add new component */
         awg->comp[cid] = comp[ncopy];
         memset (awg->rb + cid, 0, sizeof (randBlock));
         dt = 1.0 / (double)(awg->pagesize * NUMBER_OF_EPOCHS);
         f1 = comp[ncopy].par[1] * dt;
         f2 = comp[ncopy].par[2] * dt;
         rand_filter (awg->rb + cid, dt, f1, f2);
      	 /* update length of list and number of insertions */
         awg->ncomp++;
         ncopy++;
         MUTEX_RELEASE (awg->mux);
      }
   
      /* return */
      return ncopy;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: setWaveformAWG				*/
/*                                                         		*/
/* Procedure Description: sets a waveform to an AWG			*/
/* 									*/
/* Procedure Arguments: awg index, data array and length		*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int setWaveformAWG (int ID, float y[], int len)
   {
      AWG_ConfigBlock*	awg;		/* pointer to awg */
   
      /* verify valid ID */
      if ((ID < 0) || (ID >= MAX_NUM_AWG) || (y == NULL)) {
         gdsDebug ("setWaveformAWG() ID out of range"); 
         return -1;
      }
   
      /* get mutex */
      awg = AWG + ID;
      MUTEX_GET (awg->mux);
   
      if (awg -> unbreakable) {
         MUTEX_RELEASE (awg->mux);
         return 0;
      }
   
      /* test validity */
      if (((awg->status & AWG_CONFIG) == 0) ||
         ((awg->status & AWG_IN_USE) == 0)) {
         MUTEX_RELEASE (awg->mux);
         return -10;
      }
   
      /* store waveform */
      free (awg->wave);
      if (len <= 0) {
         awg->wave = NULL;
         awg->wavelen = 0;
      }
      else {
         awg->wave = calloc (len, sizeof (float));
         if (awg->wave == NULL) {
            gdsDebug ("setWaveformAWG() out of memory");
            MUTEX_RELEASE (awg->mux);
            return -11;
         }
         memcpy (awg->wave, y, len * sizeof (float));
         awg->wavelen = len;
      }
   
      /* return */
      MUTEX_RELEASE (awg->mux);
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: sendWaveformAWG				*/
/*                                                         		*/
/* Procedure Description: sets a waveform to an AWG			*/
/* 									*/
/* Procedure Arguments: awg index, time, epoch, data array and length	*/
/*                                                         		*/
/* Procedure Returns: >=0 if successful, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int sendWaveformAWG (int ID, taisec_t time, int epoch,
                     float y[], int len)
   {
      AWG_ConfigBlock*	awg;		/* pointer to awg */
      int64_t   	diff;		/* time difference in epochs */
      int	 	pagesize;	/* page size */
      int	 	n;		/* # of epochs sent */
      int	 	i;		/* buffer index */
      streambufpage_t*	s;		/* stream buffer pointer */
      int		dup;		/* stream buffer duplicate? */
      taisec_t		buftime;	/* buffer time */
      int		bufepoch;	/* buffer epoch */
   
      /* verify valid ID */
      if ((ID < 0) || (ID >= MAX_NUM_AWG)) {
         gdsDebug ("sendWaveformAWG() ID out of range"); 
         return -1;
      }
      /* get mutex */
      awg = AWG + ID;
      MUTEX_GET (awg->mux);
      if (awg -> unbreakable) {
         MUTEX_RELEASE (awg->mux);
         return 0;
      }
   
      /* test validity */
      if (((awg->status & AWG_CONFIG) == 0) ||
         ((awg->status & AWG_IN_USE) == 0)) {
         MUTEX_RELEASE (awg->mux);
         gdsDebug ("sendWaveformAWG() slot not configured"); 
         return -1;
      }
      pagesize = awg->pagesize;
      MUTEX_RELEASE (awg->mux);
   
      /* verify data length */
      n = len / pagesize;
      if ((len % pagesize != 0) || (n <= 0) || (n > NUMBER_OF_EPOCHS)) {
         gdsDebug ("sendWaveformAWG() invalid page length"); 
         return -2;
      }
      /* verify time */
      diff = ((int64_t)time * NUMBER_OF_EPOCHS + epoch) - 
	(int64_t)(TAInow() / _EPOCH);
      if (diff < NUMBER_OF_EPOCHS) {
         gdsDebug ("sendWaveformAWG() too old"); 
         return -3;
      }
      if (diff + n >=  (MAX_STREAM_BUFFER - 1) * NUMBER_OF_EPOCHS) {
         gdsDebug ("sendWaveformAWG() too new"); 
         return -4;
      }
      /* copy data */
      dup = 0;
      for (i = 0; i < n; ++i) {
         /* buffer time */
         buftime = time;
         bufepoch = epoch + i;
         while (bufepoch >= NUMBER_OF_EPOCHS) {
            bufepoch -= NUMBER_OF_EPOCHS;
            ++buftime;
         }
         s = _GET_STREAM_BUFFER (buftime, bufepoch, ID);
         memcpy (s->buf, y + i * pagesize, pagesize * sizeof (float));
         if (s->ready && (s->time == buftime) && (s->epoch == bufepoch)) {
            dup = 1;
         }
         s->time = buftime;
         s->epoch = bufepoch;
         s->pagelen = pagesize;
         s->ready = 1;
      }
      /* check if continues */
      buftime = time;
      bufepoch = epoch - 1;
      while (bufepoch < 0) {
         bufepoch += NUMBER_OF_EPOCHS;
         --buftime;
      }
      s = _GET_STREAM_BUFFER (buftime, bufepoch, ID);
      if (!s->ready || (s->time != buftime) || (s->epoch != bufepoch)) {
         return 3;
      }
      /* check duplicate */
      if (dup) {
         return 2;
      }
      /* check if we can add another one */
      if (diff + 2 * n >=  (MAX_STREAM_BUFFER - 1) * NUMBER_OF_EPOCHS) {
         return 1;
      }
      /* normal return */
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: stopWaveformAWG				*/
/*                                                         		*/
/* Procedure Description: stops a waveform in an AWG			*/
/* 									*/
/* Procedure Arguments: awg index, terminate type, terminate argument	*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int stopWaveformAWG (int ID, int terminate, tainsec_t arg)
   {
      tainsec_t		now;		/* get current time */
      tainsec_t		stop;		/* stop time */
      int		cid;		/* index into components */
      AWG_Component*	cp;		/* pointer to component */
   
      if ((ID < 0) || (ID >= MAX_NUM_AWG)) {
         gdsDebug ("stopWaveformAWG() ID out of range");
         return -1; 
      }
   
      MUTEX_GET (AWG[ID].mux);
      if (AWG[ID].unbreakable) {
         MUTEX_RELEASE (AWG[ID].mux);
         return 0;
      }
      MUTEX_RELEASE (AWG[ID].mux);
   
      /* handle reset */
      if ((terminate != 1) && (terminate != 2)) {
         return resetAWG (ID);
      }
   
      /* get the mux */
      MUTEX_GET (AWG[ID].mux);
      now = AWG[ID].tproc + _EPOCH;
      stop = now + ((arg > 0) ? arg : 0);
   
      /* first delete all waveform components currently not active */
      cid = 0;
      while ((cid < AWG[ID].ncomp) && (AWG[ID].comp[cid].start <= now)) {
         cid++;
      }
   
      /* set duration to infinte, CAUTION: only works for swept sine*/
      if (terminate == 1) {
         cp = AWG[ID].comp + cid - 1;
         if ((cid > 0) && (cp->restart == -1)) {
            if ((cp->duration != -1) && 
               (cp->start + cp->duration - cp->ramptime[1] < now) &&
               (AWG[ID].ncomp > cid)) {
               cid++;
               cp++;
            }
            cp->duration = -1;
         }
         AWG[ID].ncomp = cid;
      }
      
      /* patch ramp time */
      else {
         for (cid = 0; cid < AWG[ID].ncomp; cid++) {
            cp = AWG[ID].comp + cid;
            if (cp->duration != -1) {
               /* no phase out */
               cp->restart = -1;
               cp->duration = (cp->duration < stop - cp->start) ? 
                              cp->duration : stop - cp->start;
            }
            else {
               cp->ramptype = 
                  RAMP_TYPE (PHASE_IN_TYPE (cp->ramptype),
                            AWG_PHASING_LINEAR, AWG_PHASING_STEP);
               cp->ramptime[1] = (arg > 0) ? arg : 0;
               cp->ramppar[0] = 0;	/* amplitude & offset */
               cp->ramppar[3] = 0;
               cp->duration = stop - cp->start;
               cp->restart = -1;
            }
         }
      }
   
      /* release the mux */
      MUTEX_RELEASE (AWG[ID].mux);
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: queryWaveformAWG				*/
/*                                                         		*/
/* Procedure Description: queries waveforms from an AWG			*/
/* 									*/
/* Procedure Arguments: awg index, component list, max. number of comp.	*/
/*                                                         		*/
/* Procedure Returns: number of returned comp., <0 if failed		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int queryWaveformAWG (int ID, AWG_Component* comp, int maxComp)
   {
      AWG_ConfigBlock*	awg;		/* pointer to awg */
      int 		cid;		/* index into components */
   
      /* verify valid ID */
      if ((ID < 0) || (ID >= MAX_NUM_AWG)) {
         gdsDebug ("queryWaveformAWG() ID out of range"); 
         return -1;
      }
   
      /* get mutex */
      awg = AWG + ID;
      MUTEX_GET (awg->mux);
   
      /* test validity */
      if (((awg->status & AWG_CONFIG) == 0) ||
         ((awg->status & AWG_IN_USE) == 0)) {
         MUTEX_RELEASE (awg->mux);
         return -10;
      }
   
      /* copy components */
      for (cid = 0; (cid < maxComp) && (cid < awg->ncomp) &&
          (cid < MAX_NUM_AWG_COMPONENTS); cid++) {
         comp[cid] = awg->comp[cid];
      }
   
      MUTEX_RELEASE (awg->mux);
      return cid;
   }



/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: setGainAWG					*/
/*                                                         		*/
/* Procedure Description: set overall gain of an AWG			*/
/* 									*/
/* Procedure Arguments: awg index, gain, ramp time.			*/
/*                                                         		*/
/* Procedure Returns: 0 if success, <0 if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int setGainAWG (int ID, double g, tainsec_t time)
   {
      AWG_Gain*		gain;		/* gain */
   
      /* verify valid ID */
      if ((ID < 0) || (ID >= MAX_NUM_AWG)) {
         gdsDebug ("setFiltermAWG() ID out of range"); 
         return -1;
      }
      /* get mutex */
      gain = &AWG[ID].gain;	
      MUTEX_GET (AWG[ID].mux);
      if (AWG[ID].unbreakable) {
         MUTEX_RELEASE (AWG[ID].mux);
         return 0;
      }
   
      /* set old gain */
      switch (gain->state) {
         case 0:
         default:
            gain->old = gain->value;
            break;
         case 1: /* nothing */
            break;
         case 2:
            gain->old = gain->current;
            break;
      }
      gain->value = g;
      if (time >= 0) gain->ramptime = time;
      gain->state = 1;
   
      MUTEX_RELEASE (AWG[ID].mux);
   
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: setFilterAWG				*/
/*                                                         		*/
/* Procedure Description: set filter of an AWG				*/
/* 									*/
/* Procedure Arguments: awg index, coeff. list, max. number of coeff.	*/
/*                                                         		*/
/* Procedure Returns: 0 if success, <0 if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int setFilterAWG (int ID, double y[], int len)
   {
      int		i;		/* index */
   
      if ((len < 0) || ((len > 0) && !y)) {
         return -2;
      }
      /* verify valid ID */
      if ((ID < 0) || (ID >= MAX_NUM_AWG)) {
         gdsDebug ("setFiltermAWG() ID out of range"); 
         return -1;
      }
      if (AWG[ID].unbreakable) {
         return 0;
      }
      /* disable filter */
      if (len == 0) {
         awgfilter[ID].valid = 0;
         return 0;
      }
      /* gain */
      MUTEX_GET (AWG[ID].mux);
   
      awgfilter[ID].gain = y[0];
      /* coeff */
      for (i = 0; i < ((len - 1) / 4) && (i < MAX_AWG_SOS); ++i) {
         awgfilter[ID].sos[i][0] = y[1+4*i+0];
         awgfilter[ID].sos[i][1] = y[1+4*i+1];
         awgfilter[ID].sos[i][2] = y[1+4*i+2];
         awgfilter[ID].sos[i][3] = y[1+4*i+3];
         awgfilter[ID].hist[i][0] = 0;
         awgfilter[ID].hist[i][1] = 0;
      }
      awgfilter[ID].nsos = i;
      awgfilter[ID].valid = 1;
      MUTEX_RELEASE (AWG[ID].mux);
   
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: resetAllAWG					*/
/*                                                         		*/
/* Procedure Description: calls resetAWG for all AWG in bank		*/
/* 									*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   void resetAllAWG (void) 
   {
      int		i;
   
      for (i = 0; i < MAX_NUM_AWG; i++) {
         resetAWG (i);
      }
   
      return;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: configAllAWG				*/
/*                                                         		*/
/* Procedure Description: calls configAWGfp for all AWG in bank		*/
/* 									*/
/* Procedure Arguments: filename					*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   void configAllAWG (char* filename) 
   {
      FILE*		fp;
      int 		i;
   
      if (filename != NULL) {
         fp = openConfigFile (filename);
      }
      else {
         fp = openConfigFile (AWG_CONFIG_FILE);
      }
      if (fp == NULL) {
         gdsDebug("configAllAWG() No config file"); 
         return;
      }
   
      for (i = 0; i < MAX_NUM_AWG; i++) {
         configAWGfp (i, fp);
      }
   
      fclose (fp);
      return;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: showAllAWG					*/
/*                                                         		*/
/* Procedure Description: calls showAWG for all AWG in bank		*/
/* 									*/
/* Procedure Arguments: file handle					*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   void showAllAWG (FILE* fp) 
   {
      int  		i;
      char*		buf;
   
      if (fp == NULL) {
         fp = stdout;
      }
   
      buf = malloc (_SHOWBUF_SIZE);
      for (i = 0; i < MAX_NUM_AWG; ++i) {
         showAWG (i, buf, _SHOWBUF_SIZE);
         fputs (buf, fp);
      }
      free (buf);
      return;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: showAllAWGs					*/
/*                                                         		*/
/* Procedure Description: calls showAWG for all AWG in bank		*/
/* 									*/
/* Procedure Arguments: string, max length				*/
/*                                                         		*/
/* Procedure Returns: string						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   char* showAllAWGs (char* s, int max) 
   {
      int  		i;
      int		size;	/* buffer size */
   
      if ((s == NULL) || (max <= 0)) {
         gdsDebug("showAllAWG() No output string"); 
         return NULL;
      }
      *s = 0;
      for (i = 0; i < MAX_NUM_AWG; ++i) {
         size = strlen (s);
         showAWG (i, s + size, max - size);
      }
      return s;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: processAllAWG				*/
/*                                                         		*/
/* Procedure Description: calls processAWG for all AWG in bank		*/
/* 									*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   void processAllAWG (taisec_t time, int epoch) 
   {
      int 		i;
   
      for (i = 0; i < MAX_NUM_AWG; i++) {
         processAWG (i, time, epoch);
      }
      return;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: disableAllAWG			        */
/*                                                         		*/
/* Procedure Description: calls disableAWG for all AWG in bank		*/
/* 									*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   void disableAllAWG (void) 
   {
      int i;
   
      for (i = 0; i < MAX_NUM_AWG; ++i) {
         disableAWG (i);
      }
      return;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: enableAllAWG				*/
/*                                                         		*/
/* Procedure Description: calls enableAWG for all AWG in bank		*/
/* 									*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   void enableAllAWG (void) 
   {
      int 		i;
      for (i = 0; i < MAX_NUM_AWG; ++i) {
         enableAWG (i);
      }
      return;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: addCompAWG					*/
/*                                                         		*/
/* Procedure Description: adds string specified component to AWG[ID]	*/
/* 									*/
/* Procedure Arguments: AWG index number and formatted string		*/
/*                                                         		*/
/* Procedure Returns: GDS_ERROR						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int addCompAWG (int ID, char *instr) 
   {
      int 		n;		/* read values */
      int 		wtype;		/* waveform type */
      int 		i;		/* temp */
      int		j;		/* temp */
      int		k;		/* temp */
      char*		lp = NULL;
      char*		sp;
      AWG_Component 	comp;		/* awg component */
      double		x;		/* temp */
      double		y;		/* temp */
      long		seed;		/* random generator seed */
   
      if (gds_strncasecmp (instr,"sin", 3) == 0)
         wtype = awgSine;
      else if (gds_strncasecmp (instr,"square", 3) == 0)
         wtype = awgSquare;
      else if (gds_strncasecmp (instr,"ramp", 3) == 0)
         wtype = awgRamp;
      else if (gds_strncasecmp (instr,"triangle", 3) == 0)
         wtype = awgTriangle;
      else if (gds_strncasecmp (instr,"impulse", 3) == 0)
         wtype = awgImpulse;
      else if (gds_strncasecmp (instr,"const", 3) == 0)
         wtype = awgConst;
      else if ((gds_strncasecmp (instr,"unoise", 4) == 0) ||
              (gds_strncasecmp (instr,"noiseu", 6) == 0))
         wtype = awgNoiseU;
      else if ((gds_strncasecmp (instr,"nnoise", 4) == 0) ||
              (gds_strncasecmp (instr,"noisen", 6) == 0) ||
              (gds_strncasecmp (instr,"noi", 3) == 0))	/* default noise */
         wtype = awgNoiseN;
      else {
         return GDS_ERR_MISSING;
      }
   
      /* first check for correctly formatted component string */
      memset ((void*) &comp, 0, sizeof (AWG_Component));
      switch (wtype) {
      
         /* periodic components */
         case awgSine :
         case awgSquare :
         case awgRamp :
         case awgTriangle :
         case awgImpulse :
         case awgConst :
            {
               /* read in parameters */
               n = sscanf (instr, "%*s %lf %lf %lf %lf",
                          comp.par, comp.par+1, comp.par+2, comp.par+3);
            
               /* require all parameters */
               if (n < 4) {
                  gdsDebug ("addCompAWG() Insufficient parameters (periodic)"); 
                  return GDS_ERR_FORMAT;
               }
            
               /* read in times */
               comp.start = _ONESEC * (TAInow() / _ONESEC + 2);
               n = sscanf (instr, "%*s %*f %*f %*f %*f %lf %lf",
                          &x, &y);
               comp.duration = (n >= 1) ? x * _ONESEC : -1;
               comp.restart = (n >= 2) ? y * _ONESEC : 0;
               n = sscanf (instr, "%*s %*f %*f %*f %*f %*f %*f "
                          "%lf %lf", &x, &y);
               comp.ramptime[0] = (n >= 1) ? x * _ONESEC : -1;
               comp.ramptime[1] = (n >= 2) ? y * _ONESEC : 0;
            
               /* read in phasing parameters */
               i = j = k = 0;
               n = sscanf (instr, "%*s %*f %*f %*f %*f %*f %*f "
                          "%*f %*f %d %d %d %lf %lf %lf %lf",
                          &i, &j, &k, comp.ramppar, comp.ramppar+1, 
                          comp.ramppar+2,comp.ramppar+3);
               comp.ramptype = RAMP_TYPE (i, j, k);
               comp.wtype = wtype;
               break;      
            }
         /* aperiodic components */
         case awgNoiseN :
         case awgNoiseU :
            {
               n = sscanf (instr, "%*s %lf %lf %lf %lf %ld",
                          comp.par, comp.par+1, comp.par+2, comp.par+3,
                          &seed);
            
               if (n < 1) {
                  gdsDebug ("addCompAWG() Insufficient parameters (noise)"); 
                  return GDS_ERR_FORMAT;
               }
               else if (n >= 5) {
                  n = sscanf (instr, "%*s %*f %*f %*f %*f %*d "
                             "%lf %lf", &x, &y);
                  comp.duration = (n >= 1) ? x * _ONESEC : -1;
                  comp.restart = (n >= 2) ? y * _ONESEC : 0;
               }
               /* start times */
               comp.start = _ONESEC * (TAInow() / _ONESEC + 2);
            
               /* look for optional switches */
               comp.wtype = wtype;
               strtok_r (instr, " ", &lp); 	/* skip */
               while ((sp = strtok_r (NULL, " ", &lp)) != 0) {
                  if (gds_strncasecmp (sp ,"uni", 3) == 0) {
                     comp.wtype = awgNoiseU;
                  }
               }
               break;
            }
         default :
            {
               gdsDebug ("addCompAWG() Undefined"); 
               return GDS_ERR_FORMAT;
            }
      }
   
      /* look for next empty slot, get the mutex first */
      MUTEX_GET (AWG[ID].mux);
   
      n = 0;
      while ((AWG[ID].comp[n].wtype != 0) && 
            (n < MAX_NUM_AWG_COMPONENTS)) {
         n++;
      }
   
      if (n == MAX_NUM_AWG_COMPONENTS) {
         MUTEX_RELEASE (AWG[ID].mux);
         gdsDebug ("addCompAWG() Component overflow"); 
         return GDS_ERR_FORMAT;
      }
   
      /* add component */
      AWG[ID].comp[n] = comp;
      AWG[ID].rb[n].seed = seed;
      MUTEX_RELEASE (AWG[ID].mux);
   
      return GDS_ERR_NONE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: checkConfigAWG				*/
/*                                                         		*/
/* Procedure Description: checks AWG config and sets AWG_CONFIG bit in	*/
/*				status register				*/
/* 									*/
/* Procedure Arguments: AWG index number				*/
/*                                                         		*/
/* Procedure Returns: GDS_ERROR						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int checkConfigAWG (int ID) 
   {
      int 		n = 0;
      int 		cmax = MAX_NUM_AWG_COMPONENTS;
      int 		bad = 0;
      char 		msg[64];
      AWG_Component*	cp;
   
      if (ID < 0 || ID >= MAX_NUM_AWG) {
         gdsDebug ("checkConfigAWG() ID out of range"); 
         return GDS_ERR_PRM;
      }
   
      /* get the mutex */
      MUTEX_GET (AWG[ID].mux);
   
      AWG[ID].ncomp = 0;
      AWG[ID].status &= ~((long) AWG_CONFIG);
   
      /* check for ordering and consistency of wtypes */
      if (!AWG[ID].comp[0].wtype) {
         MUTEX_RELEASE (AWG[ID].mux);
         gdsDebug("checkConfig() bad type");
         return GDS_ERR_NONE;		/* empty block */
      }
   
      /* ncomp will include all components, valid or not, up to
         the first awgNone. */
      AWG[ID].ncomp = cmax;
      for (n = 0, cp = AWG[ID].comp; n < cmax; n++, cp++)
      
         switch (cp->wtype) {
            case awgNone :
               {
                  AWG[ID].ncomp = cmax = n;
                  break;
               }
            case awgSine :
            case awgSquare :
            case awgRamp :
            case awgTriangle :
            case awgImpulse :
            case awgConst :
               {
                  if (cp->par[1] <= 0) {
                     sprintf (msg, "bad AWG[%d].comp[%d].par[1] = %9g\n",
                             ID, n, cp->par[1]);
                     gdsDebug (msg);
                     bad = 1;
                  }
                  else if (cp->ramppar[1] < 0) {
                     sprintf (msg, "bad AWG[%d].comp[%d].ramppar[1]"
                             " = %9g\n", ID, n, cp->ramppar[1]);
                     gdsDebug (msg);
                     bad = 1;
                  }
                  else if (cp->ramptime[0] < 0) {
                     sprintf (msg, "bad AWG[%d].comp[%d].ramptime[0]"
                             " = %18.12g\n", ID, n, 
                             (double) cp->ramptime[0] / __ONESEC);
                     gdsDebug (msg);
                     bad = 1;
                  }
                  else if (cp->ramptime[1] < 0) {
                     sprintf (msg, "bad AWG[%d].comp[%d].ramptime[1]"
                             " = %18.12g\n", ID, n, 
                             (double) cp->ramptime[1] / __ONESEC);
                     gdsDebug (msg);
                     bad = 1;
                  }
                  else if (PHASE_IN_TYPE(cp->ramptype) > 2) {
                     sprintf (msg, "bad AWG[%d].comp[%d].ramptype(PI)"
                             " = %d\n", ID, n, 
                             PHASE_IN_TYPE(cp->ramptype));
                     gdsDebug (msg);
                     bad = 1;
                  }
                  else if (PHASE_OUT_TYPE(cp->ramptype) > 3) {
                     sprintf (msg, "bad AWG[%d].comp[%d].ramptype(PO)"
                             " = %d\n", ID, n, 
                             PHASE_OUT_TYPE(cp->ramptype));
                     gdsDebug (msg);
                     bad = 1;
                  }
                  else if (SWEEP_OUT_TYPE(cp->ramptype) > 3) {
                     sprintf (msg, "bad AWG[%d].comp[%d].ramptype(SO)"
                             " = %d\n", ID, n, 
                             SWEEP_OUT_TYPE(cp->ramptype));
                     gdsDebug (msg);
                     bad = 1;
                  }
                  else if (cp->restart < 0) {
                     sprintf (msg, "bad AWG[%d].comp[%d].restart"
                             " = %18.12g\n", ID, n,
                             (double) cp->restart / __ONESEC);
                     gdsDebug (msg);
                     bad = 1;
                  }
                  break;
               }
            case awgNoiseN :
               {
               /* check for valid param values here */
                  break;
               }
            case awgNoiseU :
               {
               /* check for valid param values here */
                  break;
               }
            case awgArb :
               {
               /* check for valid param values here */
                  break;
               }
            case awgStream :
               {
               /* check for valid param values here */
                  break;
               }
            default :
               {
                  gdsDebug("checkConfig() unrecognized type");
                  bad = 1;
                  break;
               }
         }
   
      AWG[ID].ncomp = n;
   
      if ((n != 0) && (bad == 0)) {
         AWG[ID].status |= AWG_CONFIG;
      } 
      else {
         AWG[ID].status &= ~AWG_ENABLE;
      }
   
      /* release mutex */
      MUTEX_RELEASE (AWG[ID].mux);
   
      return GDS_ERR_NONE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: getStatisticsAWG				*/
/*                                                         		*/
/* Procedure Description: returns the realtime statistivs	 	*/
/* 									*/
/* Procedure Arguments: pointer to stat	(result)			*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int getStatisticsAWG (awgStat_t* stat)
   {
   #ifndef _AWG_STATISTICS
      return -1;
   #else
      if (stat == NULL) {
         MUTEX_GET (statmux);
         memset ((void*) &awgstat, 0, sizeof (awgStat_t));
         MUTEX_RELEASE (statmux);
         return 0;
      }
      else {
         MUTEX_GET (statmux);
         *stat = awgstat;
         MUTEX_RELEASE (statmux);
         /* calculating mean and standard dev. from the sum 
            and sum of squares: processing waveforms */
         if (stat->pwNum > 1.0) {
            _AWG_STDDEV (stat->pwStddev, stat->pwMean, stat->pwNum);
         _AWG_MEAN (stat->pwMean, stat->pwNum);
         }
      	 /* calculating mean and standard dev. from the sum 
            and sum of squares: writing to reflective memory */
         if (stat->rmNum > 1.0) {
            _AWG_STDDEV (stat->rmStddev, stat->rmMean, stat->rmNum);
         _AWG_MEAN (stat->rmMean, stat->rmNum);
            /* stat->rmStddev = 
               sqrt ((stat->rmStddev - pow (stat->rmMean, 2) / 
                    stat->rmNum) / (stat->rmNum - 1));
            stat->rmMean /= stat->rmNum; */
         }
         /* calculating mean and standard dev. from the sum 
            and sum of squares: writing to the DAC */
         if (stat->dcNum > 1.0) {
            _AWG_STDDEV (stat->dcStddev, stat->dcMean, stat->dcNum);
         _AWG_MEAN (stat->dcMean, stat->dcNum);
            /* stat->dcStddev = 
               sqrt ((stat->dcStddev - pow (stat->dcMean, 2) /
                    stat->dcNum) / (stat->dcNum - 1));
            stat->dcMean /= stat->dcNum; */
         }
      
         return 0;
      }
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: calcAWGPeriodic				*/
/*                                                         		*/
/* Procedure Description: calculates AWG periodic component and writes 	*/
/* 			  to or sums with local buffer			*/
/* 									*/
/* Procedure Arguments: awg, component, time, output buffer & its len	*/
/*                                                         		*/
/* Procedure Returns: GDS_ERROR						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int calcAWGPeriodic (AWG_ConfigBlock* awg, AWG_Component* cp,
                     double t, float* vp, int imax)
   {
      AWG_WaveType	wtype;		/* waveform type */
      int		i;		/* sample index */
      double		dt;		/* time delay per sample */
      double		ph;		/* phase */
      double		dph;		/* phase delay per sample */
      double		A;		/* amplitude */
      double		ofs;		/* offset */
      double 		tPI;		/* phase-in time */
      double		tPO = 0;	/* phase-out time */
      double		t1;		/* time when phase-in stops */
      double		t2;		/* time when phase-out starts */
      double		tEND;		/* duration */
      int		PItype;		/* phase-in type */
      int		POtype;		/* phase-out type */
      int		SWtype;		/* sweep type */
      double		c = 0;		/* amplitude ratio */
      double		fSweep = 0;	/* freq. to sweep at phase-out */
      double		phiSweep = 0;	/* phase to sweep at phase-out */
      double		phit2 = 0;	/* phase at t2 */
      double		dOfs = 0;	/* Offset diff. at phase-out */
      double		x;		/* temp */
      double		y;		/* temp */
   
      dt = 1.0 / (double) (NUMBER_OF_EPOCHS * imax);
   
      /* phase of first sample and phase delay per sample */
      /*ph = normPhase (TWO_PI * cp->par[1] * t - cp->par[2]); */
      ph = startPhase (cp->par[1], t, cp->par[2]);
      dph = dt * cp->par[1] * TWO_PI;
   
      /* get rest of parameters */
      wtype = cp->wtype;
      A = cp->par[0];
      ofs = cp->par[3];	
      PItype = PHASE_IN_TYPE (cp->ramptype);
      POtype = PHASE_OUT_TYPE (cp->ramptype);
      SWtype = SWEEP_OUT_TYPE (cp->ramptype);
      tPI = t1 = (double) cp->ramptime[0] / __ONESEC;
   
      if (cp->duration < 0) {
         t2 = tEND = t + 1.0;
      }
      else {
         tPO = (double) cp->ramptime[1] / __ONESEC;
         tEND = (double) cp->duration / __ONESEC;
         t2 = tEND - tPO;
      	 /* check if phase-in stops after phase-out should start
      	    patch it assuming a linear ramp up/down */
         if (t2 < t1) {
            if (tPI + tPO > 0) {
               t1 = (t1 * tPO + t2 * tPI) / (tPI + tPO);
            }
            else {
               t1 = (t1 + t2) / 2.0;
            }
         }
         c = (A != 0) ? cp->ramppar[0] / A : 1.0;
         if (SWtype != AWG_PHASING_LOG) {
            fSweep = (cp->ramppar[1] - cp->par[1]) / 2.0;
            phiSweep = fmod (cp->ramppar[2] - cp->par[2] + TWO_PI * fSweep * tPO + 
                            TWO_PI * cp->par[1] * tEND , TWO_PI);
         }
         else {
            fSweep = (cp->par[1] != 0) ? fabs (cp->ramppar[1] / cp->par[1]) : 1.0;
            fSweep = exp (productLog (EULER_E * fSweep) - 1);
            phiSweep = fmod (cp->ramppar[2] - cp->par[2] + 
                            TWO_PI * cp->par[1] * (fSweep - 1) * tPO + 
                            TWO_PI * cp->par[1] * tEND , TWO_PI);
         }
      	 /* make sure sweep phase is between -pi an +pi */
         if (phiSweep > PI) {
            phiSweep -= TWO_PI;
         }
         else if (phiSweep < -PI) {
            phiSweep += TWO_PI;
         }
         if (SWtype == AWG_PHASING_LOG) {
            phit2 = startPhase(cp->par[1], t2, cp->par[2]);
         }
         dOfs = cp->ramppar[3] - ofs;
      }
   
      /* loop over samples */
      for(i = 0; i < imax; i++, t += dt) {
         /* test whether finished */
         if (t >= tEND) {
            /* after end of waveform */
            if (cp->restart > 0) {
               /* restart waveform */
               cp->start += cp->restart;
               t -= (double) cp->restart  / __ONESEC;
               ph = startPhase (cp->par[1], t, cp->par[2]);
            }
            else {
               /* it's over */
               break;
            }
         }
         /* test whether too early */
         if (t < 0) {
            /* nothiing */
         }
         else if (t < t1) {
            /* phase-in transition */
            *vp += awgPhaseIn (PItype, t, tPI) *
                   awgPeriodicSignal (wtype, A, ofs, ph);
         }
         else if (t <= t2) {
            /* normal periodic signal */
            *vp += awgPeriodicSignal (wtype, A, ofs, ph);
         }
         else if (t < tEND) {
            /* phase-out transition */
            x = ofs + dOfs * awgPhaseIn (POtype, t - t2, tPO);
            if (SWtype != AWG_PHASING_LOG) {
               y = ph;
            }
            else {
               y = phit2;
            }
            y += awgSweepOut (SWtype, t - t2, tPO, 
                             cp->par[1], fSweep, phiSweep);
            *vp += awgPhaseOut (POtype, t - t2, tPO, c) *
                   awgPeriodicSignal (wtype, A, 0.0, y) + x;
         }
      
         /* calculate phase of next sample */
         ph += dph;
         if (ph > TWO_PI) {
            ph -= TWO_PI;
         }
         vp++;
      }
   
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: calcAWGNoise				*/
/*                                                         		*/
/* Procedure Description: calculates AWG noise component and writes 	*/
/* 			  to or sums with local buffer			*/
/* 									*/
/* Procedure Arguments: awg, component, time, output buffer & its len	*/
/*                                                         		*/
/* Procedure Returns: GDS_ERROR						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int calcAWGNoise (AWG_ConfigBlock* awg, AWG_Component* cp,
                     double t, float* vp, int imax, 
                     randBlock* rb)
   {
      AWG_WaveType	wtype;		/* waveform type */
      int		i;		/* buffer index */
      double		dt;		/* time delay per sample */
      double		tEND;		/* duration */
      double 		tPI;		/* phase-in time */
      double		tPO = 0;	/* phase-out time */
      double		t1;		/* time when phase-in stops */
      double		t2;		/* time when phase-out starts */
      int		PItype;		/* phase-in type */
      int		POtype;		/* phase-out type */
      double		A;		/* amplitude */
      double		ofs;		/* offset */
      double		c = 0;		/* amplitude ratio */
      double		dOfs = 0;	/* Offset diff. at phase-out */
      double		x;		/* temp */
   
      /* calculate time parameters */
      dt = 1.0 / (double) (NUMBER_OF_EPOCHS * imax);
      tEND = t2 = ((double) cp->duration < 0) ? t + 1.0 :
                  (double) cp->duration / __ONESEC;
   
      /* get parameters */
      wtype = cp->wtype;
      A = fabs (cp->par[0]);
      ofs = cp->par[3];
   
      /* only phase in & phase out */
      PItype = PHASE_IN_TYPE (cp->ramptype);
      POtype = PHASE_OUT_TYPE (cp->ramptype);
      tPI = t1 = (double) cp->ramptime[0] / __ONESEC;
      if (cp->duration >= 0) {
         tPO = (double) cp->ramptime[1] / __ONESEC;
         t2 = tEND - tPO;
      	 /* check if phase-in stops after phase-out should start
      	    patch it assuming a linear ramp up/down */
         if (t2 < t1) {
            if (tPI + tPO > 0) {
               t1 = (t1 * tPO + t2 * tPI) / (tPI + tPO);
            }
            else {
               t1 = (t1 + t2) / 2.0;
            }
         }
         c = (A != 0) ? cp->ramppar[0] / A : 1.0;
         dOfs = cp->ramppar[3] - ofs;
      }
   
      for(i = 0; i < imax; i++, vp++, t += dt) {
         /* test whether finished */
         if (t >= tEND) {
            /* after end of waveform */
            if (cp->restart > 0) {
               /* restart waveform */
               cp->start += cp->restart;
               t -= (double) cp->restart / __ONESEC;
            }
            else {
               /* it's over */
               break;
            }
         }
         /* test whether too early */
         if (t < 0) {
            /* nothing */
         }
         /* calculate value */
         else if (t < t1) {
            /* phase-in transition */
            *vp += awgPhaseIn (PItype, t, tPI) *
                   awgNoiseSignal (wtype, A, ofs, rb);
         }
         /* normal noise signal */
         else if (t < t2) {
            *vp += awgNoiseSignal (wtype, A, ofs, rb);
         }
         /* phase-out transition */
         else if (t < tEND) {
            x = ofs + dOfs * awgPhaseIn (POtype, t - t2, tPO);
            *vp += awgPhaseOut (POtype, t - t2, tPO, c) *
                   awgNoiseSignal (wtype, A, 0.0, rb) + x;
         }
      }
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: calcAWGImpulse				*/
/*                                                         		*/
/* Procedure Description: calculates AWG impulse component and writes 	*/
/* 			  to or sums with local buffer			*/
/* 									*/
/* Procedure Arguments: awg, component, time, output buffer & its len	*/
/*                                                         		*/
/* Procedure Returns: GDS_ERROR						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int calcAWGImpulse (AWG_ConfigBlock* awg, AWG_Component* cp,
                     double t, float* vp, int imax)
   {
      int		i;		/* buffer index */
      double		dt;		/* time delay per sample */
      double		delay;		/* delay */
      double		tHigh;		/* high duration */
      double		tHighEnd;	/* end of high period */
      double		TT;		/* Period */
      double		tnorm;		/* normalized time */
      double		tEND;		/* duration */
      double 		tPI;		/* phase-in time */
      double		tPO = 0;	/* phase-out time */
      double		t1;		/* time when phase-in stops */
      double		t2;		/* time when phase-out starts */
      int		PItype;		/* phase-in type */
      int		POtype;		/* phase-out type */
      double		A;		/* amplitude */
      double		c = 0;		/* amplitude ratio */
   
      /* get parameters */
      delay = cp->par[3];
      tHigh = cp->par[2];
      A = cp->par[0];
   
      /* calculate time parameters */
      dt = 1.0 / (double) (NUMBER_OF_EPOCHS * imax);
      TT = (cp->par[1] > 0) ? 1.0 / cp->par[1] : 1E99;
      tEND = t2 = ((double) cp->duration < 0) ? t + 1.0 :
                  (double) cp->duration / __ONESEC;
      tHighEnd = (tHigh > dt) ? delay + tHigh : delay + dt;
   
      /* normalize time to be within a period */
      tnorm = (cp->par[1] > 0) ? t - floor (t * cp->par[1]) * TT : t;
   
      /* only phase in & phase out */
      PItype = PHASE_IN_TYPE (cp->ramptype);
      POtype = PHASE_OUT_TYPE (cp->ramptype);
      tPI = t1 = (double) cp->ramptime[0] / __ONESEC;
      if (cp->duration >= 0) {
         tPO = (double) cp->ramptime[1] / __ONESEC;
         t2 = tEND - tPO;
      	 /* check if phase-in stops after phase-out should start
      	    patch it assuming a linear ramp up/down */
         if (t2 < t1) {
            if (tPI + tPO > 0) {
               t1 = (t1 * tPO + t2 * tPI) / (tPI + tPO);
            }
            else {
               t1 = (t1 + t2) / 2.0;
            }
         }
         c = (A != 0) ? cp->ramppar[0] / A : 1.0;
      }
   
      for(i = 0; i < imax; i++, vp++, t += dt) {
         /* test whether finished */
         if (t >= tEND) {
            /* after end of waveform */
            if (cp->restart > 0) {
               /* restart waveform */
               cp->start += cp->restart;
               t -= (double) cp->restart / __ONESEC;
               tnorm = (cp->par[1] > 0) ? 
                       t - floor (t * cp->par[1]) * TT : t;
            }
            else {
               /* it's over */
               break;
            }
         }
         /* test whether too early */
         if (t < 0) {
            /* nothing */
         }
         /* calculate value */
         else if ((tnorm >= delay) && (tnorm < tHighEnd)) {
            /* phase-in transition */
            if (t < t1) {
               *vp += awgPhaseIn (PItype, t, tPI) * A;
            }
            /* normal signal */
            else if (t < t2) {
               *vp += A;
            }
            /* phase-out transition */
            else if (t < tEND) {
               *vp += awgPhaseOut (POtype, t - t2, tPO, c) * A;
            }
         }
      
         /* calculate nomralized time of next sample */
         tnorm += dt;
         if (tnorm > TT) {
            tnorm -= TT;
         }
      }
   
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: calcAWGArb					*/
/*                                                         		*/
/* Procedure Description: calculates AWG arbitrary component and writes	*/
/* 			  to or sums with local buffer			*/
/* 									*/
/* Procedure Arguments: awg, component, time, output buffer & its len	*/
/*                                                         		*/
/* Procedure Returns: GDS_ERROR						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int calcAWGArb (AWG_ConfigBlock* awg, AWG_Component* cp,
                     double t, float* vp, int imax)
   {
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: calcAWGStream				*/
/*                                                         		*/
/* Procedure Description: calculates AWG arbitrary component and writes	*/
/* 			  to or sums with local buffer			*/
/* 									*/
/* Procedure Arguments: awg, component, time, output buffer & its len	*/
/*                                                         		*/
/* Procedure Returns: GDS_ERROR						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int calcAWGStream (AWG_ConfigBlock* awg, AWG_Component* cp,
                     streambufpage_t* s, double t, float* vp, int imax)
   {
      int		i;		/* waveform index */
      double		dt;		/* time delay per sample */
      double		tEND;		/* duration */
      double 		tPI;		/* phase-in time */
      double		tPO = 0;	/* phase-out time */
      double		t1;		/* time when phase-in stops */
      double		t2;		/* time when phase-out starts */
      int		PItype;		/* phase-in type */
      int		POtype;		/* phase-out type */
      double		scale;		/* scaling factor */
      double		c = 0;		/* amplitude ratio */
   
      /* get parameters */
      scale = cp->par[0];
   
      /* calculate time parameters */
      dt = 1.0 / (double) (NUMBER_OF_EPOCHS * imax);
      tEND = t2 = ((double) cp->duration < 0) ? t + 1.0 :
                  (double) cp->duration / __ONESEC;
   
      /* only phase in & phase out */
      PItype = PHASE_IN_TYPE (cp->ramptype);
      POtype = PHASE_OUT_TYPE (cp->ramptype);
      tPI = t1 = (double) cp->ramptime[0] / __ONESEC;
      if (cp->duration >= 0) {
         tPO = (double) cp->ramptime[1] / __ONESEC;
         t2 = tEND - tPO;
      	 /* check if phase-in stops after phase-out should start
      	    patch it assuming a linear ramp up/down */
         if (t2 < t1) {
            if (tPI + tPO > 0) {
               t1 = (t1 * tPO + t2 * tPI) / (tPI + tPO);
            }
            else {
               t1 = (t1 + t2) / 2.0;
            }
         }
         c = (scale != 0) ? cp->ramppar[0] / scale: 1.0;
      }
   
      /* add waveform if buffer is read and has the correct page size */
      if (s->ready && (imax == s->pagelen)) {
         s->ready = 2;
         for (i = 0; i < imax; ++i, ++vp, t += dt) {
            /* test whether finished */
            if (t >= tEND) {
               /* after end of waveform */
               if (cp->restart > 0) {
                  /* restart waveform */
                  cp->start += cp->restart;
                  t -= (double) cp->restart / __ONESEC;
               }
               else {
                  /* it's over */
                  break;
               }
            }
            /* test whether too early */
            if (t < 0) {
               /* nothing */
            }
            /* phase-in transition */
            if (t < t1) {
               *vp += awgPhaseIn (PItype, t, tPI) * scale * s->buf[i];
            }
            /* normal signal */
            else if (t < t2) {
               *vp += scale * s->buf[i];
            }
            /* phase-out transition */
            else if (t < tEND) {
               *vp += awgPhaseOut (POtype, t - t2, tPO, c) * 
                      scale * s->buf[i];
            }
         }
         s->ready = 0;
      }
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: calcAWGComp					*/
/*                                                         		*/
/* Procedure Description: calculates AWG component and writes to or 	*/
/* 			  sums with local buffer			*/
/* 									*/
/* Procedure Arguments: AWG index number, component ID, epoch		*/
/*                                                         		*/
/* Procedure Returns: GDS_ERROR						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int calcAWGComp (int ID, int cid, 
                     taisec_t time, int epoch, tainsec_t delay) 
   {
      AWG_ConfigBlock*	awg;		/* pointer to awg */
      AWG_Component*	cp;		/* pointer to component */
      double		t;		/* time */
      float*		vp;		/* pointer to ouput buffer */
      int		imax;		/* number of samples */
      int		status;		/* status of calculation */
   
      /* get awg mutex */
      awg = AWG + ID;
      /* get waveform component */
      cp = awg->comp + cid;
   
      /* check if there is a waveform */
      if (cp->wtype == awgNone) {
         return -MAX_INVALID_COMPONENTS;
      }
   
      /* time of first sample */
      t = (double) ((tainsec_t) time * _ONESEC + 
                   _EPOCH * (tainsec_t) epoch - cp->start + delay) /
          __ONESEC;
   
      /* check whether waveform is ready */
      if (t + _EPOCH /__ONESEC < 0) {
         return -1;
      }
   
      /* get pointer and size of output buffer */
      vp = _GET_OUTPUT_BUFFER (epoch, ID);
      imax = awg->pagesize;
   
      /* calculate waveform */
      switch (cp->wtype) {
         case awgSine:
         case awgSquare:
         case awgRamp:
         case awgTriangle:
         case awgConst:
            {
               status = calcAWGPeriodic (awg, cp, t, vp, imax);
               break;
            }
         case awgImpulse:
            {
               status = calcAWGImpulse (awg, cp, t, vp, imax);
               break;
            }
         case awgNoiseN:
         case awgNoiseU:
            {
               status = calcAWGNoise (awg, cp, t, vp, imax, 
                                     awg->rb + cid);
               break;
            }
         case awgArb:
            {
               status = calcAWGArb (awg, cp, t, vp, imax);
               break;
            }
         case awgStream:
            {
               /* get pointer to stream buffer */
               streambufpage_t* s = _GET_STREAM_BUFFER (time, epoch, ID);
               /* copy waveform if time of buffer is ok */
               if ((s->time == time) && (s->epoch == epoch)) {
                  status = calcAWGStream (awg, cp, s, t, vp, imax);
               }
               else {
                  status = 0; /* not an invalid waveform! */
               }
               break;
            }
         default:
            {
               /* unsupported component */
               status = -MAX_INVALID_COMPONENTS;
               break;
            }
      }
   
      /* release the mutex and return */
      return status;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: calcAWGGain					*/
/*                                                         		*/
/* Procedure Description: computes the overall gain			*/
/* 									*/
/* Procedure Arguments: AWG index number, epoch				*/
/*                                                         		*/
/* Procedure Returns: GDS_ERROR						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int calcAWGGain (int ID, taisec_t time, int epoch, tainsec_t delay)
   {
      AWG_Gain*		gain;		/* gain */
      float*		vp;		/* pointer to ouput buffer */
      int		i;		/* sample index */
      int		imax;		/* number of samples */
      double		g;		/* gain value */
      double		dg;		/* delta gain value */
      double		dt;		/* dt */
      double		t;		/* t of ramp */
      double		dur;		/* duration of ramp */
      double        r;          /* ramp weight.  fraction of old gain to use in current gain */
   
      gain = &AWG[ID].gain;
   
      /* get pointer and size of output buffer */
      vp = _GET_OUTPUT_BUFFER (epoch, ID);
      imax = AWG[ID].pagesize;
   
      /* start new gain ramp? */
      if (gain->state == 1) {
         if (gain->ramptime <= 0) {
            gain->state = 0;
         }
         else {
            gain->state = 2;
            gain->rampstart = time * _ONESEC + epoch * _EPOCH;
         }
      }
   
      /* simple case first: no ramping */
      if (gain->state != 2) {
         g = gain->value;
         if (g != 1.0) {
            for (i = 0; i < imax; ++i) {
               vp[i] *= g;
            }
         }
         return 0;
      }
   
      /* setup ramp parameters */
      dt = 1.0 / (double) (NUMBER_OF_EPOCHS * imax);
      t = (double)((time * _ONESEC + epoch * _EPOCH) - 
                  gain->rampstart) / __ONESEC;
      dur = (double)(gain->ramptime) / __ONESEC;



      /* compute ramp */
      for (i = 0; i < imax; ++i) {
          double q = t/ dur;  /* fraction of ramp time that has transpired */
          q = q * q;  /* square it, because the ramp formula only uses q^4 and q^2 terms */
         r = (t <= 0 ? 1.0 :
             (t >= dur ? 0.0 : (-q*q + 2 * q))); /* formula taken from awgPhaseIn() in awgfunc.c */
         g = (1-r) * gain->old + r * gain->value;
         vp[i] *= g;
         t += dt;
      }
      gain->current = g;
   
      /* check if ramping is done */
      if (t > dur - 1E-9) {
         gain->state = 0;
      }
   
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: calcAWGFilter				*/
/*                                                         		*/
/* Procedure Description: filters the awg output			*/
/* 									*/
/* Procedure Arguments: AWG index number, epoch				*/
/*                                                         		*/
/* Procedure Returns: GDS_ERROR						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int calcAWGFilter (int ID, taisec_t time, int epoch, tainsec_t delay)
   {
      double		g;		/* gain */
      float*		vp;		/* pointer to ouput buffer */
      int		i;		/* sample index */
      int		j;		/* sos index */
      int		imax;		/* number of samples */
   
      if (!awgfilter[ID].valid) {
         return -1;
      }
   
      /* get pointer and size of output buffer */
      vp = _GET_OUTPUT_BUFFER (epoch, ID);
      imax = AWG[ID].pagesize;
   
      /* gain */
      g = awgfilter[ID].gain;
      for (i = 0; i < imax; ++i) {
         vp[i] *= g;
      }
      /* SOS */
      for (j = 0; j < awgfilter[ID].nsos; ++j) {
         awgSOS (vp, imax, awgfilter[ID].sos[j][0], 
                awgfilter[ID].sos[j][1], awgfilter[ID].sos[j][2],
                awgfilter[ID].sos[j][3], awgfilter[ID].hist[j],
                awgfilter[ID].hist[j] + 1);
      }
   
      return 0;
   }

