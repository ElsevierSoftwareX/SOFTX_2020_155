/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: channel_server						*/
/*                                                         		*/
/* Module Description: implements server functions for providing  	*/
/* channel information for the diagnostics system			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

/* #define RPC_SVC_FG */

/* Header File List: */
#ifndef __EXTENSIONS__
#define __EXTENSIONS__
#endif 
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
#include <stdlib.h>
#include <string.h>
#include "dtt/gdsutil.h"
#include "dtt/rpcinc.h"
#include "dtt/rchannel.h"
#include "dtt/channel_server.hh"


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Constants: _KEEPALIVE_TIMEOUT  timeout for keep alive signal		*/
/* 	      _CHNNAME_SIZE	  size of channel name			*/
/*            PRM_NAME		  entry for channel name		*/
/*            PRM_IFOID		  entry for ifo ID			*/
/*            PRM_RMID		  entry for refl. mem. ID	 	*/
/*            PRM_DCUID		  entry for DCU ID			*/
/*            PRM_CHNNUM	  entry for channel number		*/
/*            PRM_DATATYPE	  entry for data type			*/
/*            PRM_DATARATE	  entry for data rate			*/
/*            PRM_RMOFFSET	  entry for refl. mem. offset		*/
/*            PRM_RMBLOCKSIZE	  entry for refl. mem. block size	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#define _KEEPALIVE_TIMEOUT	60
#define _CHNNAME_SIZE		32
#define PRM_IFOID		"ifoid"
#define PRM_RMID		"rmid"
#define PRM_DCUID		"dcuid"
#define PRM_CHNNUM		"chnnum"
#define PRM_DATATYPE		"datatype"
#define PRM_DATARATE		"datarate"
#define PRM_GAIN		"gain"
#define PRM_SLOPE		"slope"
#define PRM_OFFSET		"offset"
#define PRM_UNIT		"unit"
#define PRM_RMOFFSET		"rmoffset"
#define PRM_RMBLOCKSIZE		"rmblocksize"

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
/* Globals: servermux		protects globals			*/
/*          list		channel list				*/
/*          shutdownflag	shutdown flag				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static channelinfo_r*	list = 0;
   static int			nlist;
   static int			maxlist;
   static int			shutdownflag = 0;


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Forward declarations: 						*/
/*      rchannel_1		rpc dispatch function			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
extern "C" void rchannel_1 (struct svc_req* rqstp, 
                     register SVCXPRT* transp);


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
extern "C" 
   bool_t chnquery_1_svc (resultChannelQuery_r* result, 
                     struct svc_req* rqstp)
   {
      rpcSetServerBusy (1);
   
      /* copy list into result */
      result->chnlist.channellist_r_len = nlist;
      result->chnlist.channellist_r_val = (channelinfo_r*) 
         malloc (nlist * sizeof (channelinfo_r));
      if (result->chnlist.channellist_r_val == 0) {
         result->status = -1;
      }
      else {
         for (int i = 0; i < nlist; ++i) {
            result->chnlist.channellist_r_val[i] = list[i];
         }
         result->status = 0;
      }
      return TRUE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: rchannel_1_freeresult			*/
/*                                                         		*/
/* Procedure Description: frees memory of rpc call			*/
/*                                                         		*/
/* Procedure Arguments: rpc transport info, xdr routine for result,	*/
/*			pointer to result				*/
/*                                                         		*/
/* Procedure Returns: TRUE if successful, FALSE if failed		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
extern "C"
   int rchannel_1_freeresult (SVCXPRT* transp, 
                     xdrproc_t xdr_result, caddr_t result)
   {
      (void) xdr_free (xdr_result, result);
      return TRUE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: readChannelFile				*/
/*                                                         		*/
/* Procedure Description: reads channel info from file			*/
/* 			  mutex MUST be owned by caller			*/
/*                                                         		*/
/* Procedure Arguments: filename					*/
/*                                                         		*/
/* Procedure Returns: true if successful, false otherwise.		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static bool readChannelFile (const char* filename)
   {
   
      FILE*		fp;		/* channel info file */
      char		section[PARAM_ENTRY_LEN]; /* section name */
      char*		sec;		/* pointer to section */
      int		nentry;		/* number of section entries */
      int		cursor;		/* section cursor */
      char*		p;		/* temp ptr */
      channelinfo_r	info;		/* channel info */
   
      /* open parameter file */
      if ((fp = fopen (filename, "r")) == NULL) {
         return false;
      }
   
      /* loop through sections in parameter file */
      while (nextParamFileSection (fp, section) != NULL) {
         /* now add elements */
         sec = getParamFileSection (fp, NULL, &nentry, 0);
         if (sec == NULL) {
            fclose (fp);
            return false;
         }
         memset (&info, 0, sizeof (channelinfo_r));
         strncpy (info.chName, section, sizeof (info.chName) - 1);
         info.chName[sizeof(info.chName)-1] = 0;
         p = info.chName;
         while (*p != '\0') {
            *p = toupper (*p);
            p++;
         }
         cursor = -1;
         info.ifoId = 0;
         loadParamSectionEntry (PRM_IFOID, sec, nentry, &cursor, 
                              5, &info.ifoId);
         info.rmId = 0;
         loadParamSectionEntry (PRM_RMID, sec, nentry, &cursor, 
                              5, &info.rmId);
         info.dcuId = -1;
         loadParamSectionEntry (PRM_DCUID, sec, nentry, &cursor, 
                              5, &info.dcuId);
         info.chNum = 0;
         loadParamSectionEntry (PRM_CHNNUM, sec, nentry, &cursor, 
                              5, &info.chNum);
         info.dataType = 1;
         loadParamSectionEntry (PRM_DATATYPE, sec, nentry, &cursor, 
                              5, &info.dataType);
         info.dataRate = 14;
#if defined(_ADVANCED_LIGO)
         loadParamSectionEntry (PRM_DATARATE, sec, nentry, &cursor, 
                              1, &info.dataRate);
#else
         loadParamSectionEntry (PRM_DATARATE, sec, nentry, &cursor, 
                              5, &info.dataRate);
#endif
         info.rmOffset = 0;
         loadParamSectionEntry (PRM_RMOFFSET, sec, nentry, &cursor, 
                              4, &info.rmOffset);
         info.rmBlockSize = 0;
         loadParamSectionEntry (PRM_RMBLOCKSIZE, sec, nentry, &cursor, 
                              4, &info.rmBlockSize);
         switch (info.dataType) {
            case 1:
               {
                  info.bps = 2;
                  break;
               }
            case 2:
            case 4:
               {
                  info.bps = 4;
                  break;
               }
            case 3:
            case 5:
               {
                  info.bps = 2;
                  break;
               }
            case 6:
               {
                  info.bps = 8;
                  break;
               }
         }
         info.gain = 1;
         loadParamSectionEntry (PRM_GAIN, sec, nentry, &cursor, 
                              6, &info.gain);
         info.slope = 1;
         loadParamSectionEntry (PRM_SLOPE, sec, nentry, &cursor, 
                              6, &info.slope);
         info.offset = 0;
         loadParamSectionEntry (PRM_OFFSET, sec, nentry, &cursor, 
                              6, &info.offset);
         char u [PARAM_ENTRY_LEN];
         strcpy (u, "");
         loadParamSectionEntry (PRM_OFFSET, sec, nentry, &cursor, 3, u);
         strncpy (info.unit, u, 32);
         info.unit[31] = 0;
         p = info.unit;
         while (*p != '\0') {
            *p = toupper (*p);
            p++;
         }
         free (sec);
         /* add to list */
         if (nlist >= maxlist) {
            maxlist *= 2;
            list = (channelinfo_r*) 
               realloc (list, maxlist * sizeof (channelinfo_r));
         }
         list[nlist++] = info;
      }
   
      /* close the file */
      fclose (fp);
      return true;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: channel_server				*/
/*                                                         		*/
/* Procedure Description: start rpc service task for test points	*/
/*                                                         		*/
/* Procedure Arguments: configuration files				*/
/*                                                         		*/
/* Procedure Returns: does not return if successful, <0 otherwise	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   bool channel_server (const char* const* conf)
   {
      maxlist = 1024;
      list = (channelinfo_r*) 
         calloc (maxlist, sizeof (channelinfo_r));
      nlist = 0;
   
      int		rpcpmstart;	/* port monitor flag */
      SVCXPRT*		transp;		/* service transport */
      int		proto;		/* protocol */
   
      /* read configuration files */
      for (const char* const* iter = conf; *iter; ++iter) {
         readChannelFile (*iter);
      }
   
      /* init rpc service */
      if (rpcInitializeServer (&rpcpmstart, _SVC_FG, _SVC_MODE,
                           &transp, &proto) < 0) {
         gdsError (GDS_ERR_PROG, "unable to start rpc service");
         return false;
      }
   
      /* register with rpc service */
      if (rpcRegisterService (rpcpmstart, transp, proto, 
                           RPC_PROGNUM_GDSCHN, RPC_PROGVER_GDSCHN, 
                           rchannel_1) != 0) {
         gdsError (GDS_ERR_PROG, 
                  "unable to register test point service");
         return false;
      }
      printf ("Channel database server (%x / %i)\n", 
             RPC_PROGNUM_GDSCHN, RPC_PROGVER_GDSCHN);
   
      /* start server */
      rpcStartServer (rpcpmstart, &shutdownflag);
   
      /* never reached */
      return false;
   
   }


