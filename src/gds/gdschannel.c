static char *versionId = "Version $Id$" ;
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: Channel information API					*/
/*                                                         		*/
/* Procedure Description: API for accessing channel information		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Defines: Describes which channel databaseto should be used		*/
/*          _CHANNEL_DB		possible channel databases:		*/
/*				_CHN_DB_DEFAULT - 0			*/
/*				_CHN_DB_PARAM - 1: parameter file	*/
/*				_CHN_DB_DAQ - 2:   standard DAQ DB	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#define	_CHN_DB_DEFAULT		0
#define _CHN_DB_PARAM		1
#define _CHN_DB_DAQ		2

/* make sure _CHANNEL_DB is set */
#if !defined(_CHANNEL_DB)
#define _CHANNEL_DB		_CHN_DB_PARAM
#endif

/* set default behaviour here */
#if (_CHANNEL_DB==_CHN_DB_DEFAULT)
#undef _CHANNEL_DB
#define _CHANNEL_DB		_CHN_DB_DAQ
#endif

#ifndef __EXTENSIONS__
#define __EXTENSIONS__
#endif 
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Includes								*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#ifdef OS_VXWORKS
#include <vxWorks.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include "dtt/hardware.h"
#include "dtt/gdsutil.h"
#include "dtt/gdstask.h"
#include "dtt/gdssock.h"
#include "dtt/gdschannel.h"
#if (_CHANNEL_DB==_CHN_DB_DAQ)
/* for VxWorks */
#ifdef OS_VXWORKS
#include <vxWorks.h>
#include <sockLib.h>
#include <inetLib.h>
#include <stdioLib.h>
#include <hostLib.h>
#include <ioLib.h>
/* for others */
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#endif
#endif
#if defined (_CONFIG_DYNAMIC)
#include "dtt/confinfo.h"
#ifndef _NO_TESTPOINTS
#include "dtt/rchannel.h"
#endif
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Constants: PRM_FILE		  parameter file name			*/
/*            PRM_SECTION	  section heading is channel name!	*/
/*            PRM_NAME		  entry for channel name		*/
/*            PRM_IFOID		  entry for ifo ID			*/
/*            PRM_RMID		  entry for refl. mem. ID	 	*/
/*            PRM_DCUID		  entry for DCU ID			*/
/*            PRM_CHNNUM	  entry for channel number		*/
/*            PRM_DATATYPE	  entry for data type			*/
/*            PRM_DATARATE	  entry for data rate			*/
/*            PRM_RMOFFSET	  entry for refl. mem. offset		*/
/*            PRM_RMBLOCKSIZE	  entry for refl. mem. block size	*/
/*            SEC_SERVER	  section name for DAQ NDS server	*/
/*            PRM_SERVERNAME	  entry for server name			*/
/*            PRM_SERVERPORT	  entry for server port			*/
/*            TP_PATH		  testpoint info file path name		*/
/*            TP_FILE		  testpoint info file name		*/
/*            TP_MAXIFO		  max. num. of ifos w/ testpoints	*/
/*            DAQD_SERVER	  default server name for channel info	*/
/*            DAQD_PORT		  default server port for channel info	*/
/*            _TIMEOUT		  timeout for socket connect		*/
/*            _TICK		  number of ticks to wait		*/
/*            _NETID		  net protocol used for rpc		*/
/*            								*/
/*----------------------------------------------------------------------*/
#define _CHNLIST_SIZE		200
#if !defined (_CONFIG_DYNAMIC)
#define PRM_FILE		gdsPathFile2("/param", "chn", SITE_PREFIX, ".par")
#define PRM_FILE2		gdsPathFile("/param", "nds.par")
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
#define SEC_SERVER		gdsSectionSite ("nds")
#define PRM_SERVERNAME		"hostname"
#define PRM_SERVERPORT		"port"
#define TP_FILE			gdsPathFile2("/param", "tpchn_", SITE_PREFIX, "%i.par")
#ifdef GDS_UNIX_TARGET
#define TP_MAXIFO		16
#else
#define TP_MAXIFO		4
#endif
#define DAQD_SERVER		"fb0"
#endif
#define DAQD_PORT		8088
#define _TIMEOUT		500000	/* 500ms timeout */
#define _TICKS			10
#define _NETID			"tcp"


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Types: 								*/
/*                                                         		*/
/*----------------------------------------------------------------------*/


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Globals: chn_init - initialization status				*/
/*	    chnmux - mutex which protects the channel info list		*/
/*          chninfo - pointer to channel info list			*/
/*          chninfonum - number of channels				*/
/*          chninfosize - size of memory buffer fro channels		*/
/*          daqServer - name of NDS 					*/
/*          daqPort - port numnber of NDS				*/
/*          dbServer - channel data base server address			*/
/*          dbPrognum - channel data base program number		*/
/*          dbProgver - channel data base program version		*/
/*          								*/
/*----------------------------------------------------------------------*/
   static int			chn_init = 0;
   static mutexID_t		chnmux;
   static gdsChnInfo_t*		chninfo = NULL;
   static int			chninfonum = 0;
   static int			chninfosize = 0;
#if (_CHANNEL_DB==_CHN_DB_DAQ)
   static int			daqSetUser = 0;
   static char			daqServer[256];
   static int			daqPort;
#endif
#if defined (_CONFIG_DYNAMIC)
   static char			dbServer[256] = "";
   static unsigned long 	dbPrognum = 0;
   static unsigned long		dbProgver = 0;
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Prototypes:							*/
/* 	initChnInfo - initializes channel information			*/
/* 	finiChnInfo - terminate routine					*/
/* 	readChnInfo - reads channel information				*/
/*      resizeChnInfo - resized the channel info list			*/
/* 	CVHex - hex to int						*/
/* 	RecvRec - receive record from NDS				*/
/* 	SendRequest - send request to NDS				*/
/* 									*/
/*----------------------------------------------------------------------*/
   __init__ (initChnInfo);
#ifndef __GNUC__
#pragma init(initChnInfo)
#endif
   __fini__(finiChnInfo);
#ifndef __GNUC__
#pragma fini(finiChnInfo)
#endif
   static int readChnInfo (void);
   static int resizeChnInfo (int newlen);
#if (_CHANNEL_DB==_CHN_DB_DAQ) && !defined(OS_VXWORKS)
   static int CVHex(const char *text, int N);
   static int RecvRec();
   static int SendRequest(int mSocket, const char* text, char *reply, 
                     int length, int *Size);
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: gdsChannelInfo				*/
/*                                                         		*/
/* Procedure Description: gets the channel information			*/
/* 									*/
/* Procedure Arguments: channel name, ptr to channel info structure	*/
/*                                                         		*/
/* Procedure Returns: 0 if successful; <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int gdsChannelInfo (const char* name, gdsChnInfo_t* info)
   {
      gdsChnInfo_t* 	pinfo;		/* ptr to found info */
   
      if (channel_client () < 0) {
         return -1;
      }
   
      /* binary search */
      pinfo = bsearch ((void*) name, (void*) chninfo, 
                      chninfonum, sizeof (gdsChnInfo_t), 
                      (int (*) (const void*, const void*)) gds_strcasecmp);
   
      /* not found */
      if (pinfo == NULL) {
	 printf("channel not found\n");
         return -1;
      }
      /* copy info if found */
      if (info != NULL) {
         *info = *pinfo;
      }
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: gdsChannelListLen				*/
/*                                                         		*/
/* Procedure Description: gets the channel list length			*/
/* 									*/
/* Procedure Arguments: ifo ID, query argument				*/
/*                                                         		*/
/* Procedure Returns: length if successful; <0 otherwise		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int gdsChannelListLen (int ifo, gdsChannelQuery query)
   {
      int		i;		/* index */
      int		n;		/* count */
   
      if (channel_client () < 0) {
         return -1;
      }
   
      /* count channels */
      MUTEX_GET (chnmux);
      for (i = 0, n = 0; i < chninfonum; i++) {
         if ((query != NULL) && !query (chninfo + i)) {
            continue;
         }
	 /*printf("gdsChannelListLen; channel %d has ifo %d\n", i, chninfo[i].ifoId);*/
         if ((ifo < 0) || (chninfo[i].ifoId == ifo)) {
            n++;
         }
      }
      MUTEX_RELEASE (chnmux);
   
      return n;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: gdsChannelList				*/
/*                                                         		*/
/* Procedure Description: gets the channel list				*/
/* 									*/
/* Procedure Arguments: ifo ID, query argument, ptr to info list,	*/
/*                      size of info list				*/
/*                                                         		*/
/* Procedure Returns: length if successful; <0 otherwise		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int gdsChannelList (int ifo, gdsChannelQuery query, gdsChnInfo_t* info, 
                     int maxChn)
   {
      int		i;		/* index */
      int		n;		/* count */
   
      if (channel_client () < 0) {
         return -1;
      }
   
      /* go through list */
      MUTEX_GET (chnmux);
      for (i = 0, n = 0; i < chninfonum; i++) {
         if ((query != NULL) && !query (chninfo + i)) {
            continue;
         }
         if ((n < maxChn) &&
            ((ifo < 0) || (chninfo[i].ifoId == ifo))) {
            info[n] = chninfo[i];
            n++;
         }
      }
      MUTEX_RELEASE (chnmux);
   
      return n;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: gdsChannelNames				*/
/*                                                         		*/
/* Procedure Description: gets the channel names			*/
/* 									*/
/* Procedure Arguments: ifo ID, query argument				*/
/*                                                         		*/
/* Procedure Returns: channel names					*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   char* gdsChannelNames (int ifo, gdsChannelQuery query, int info)
   {
      int		i;		/* index */
      int		n;		/* count */
      int		size;		/* size of return argument */
      char*		ret;		/* return argument */
      char*		p;		/* temp pointer */
   
      if (channel_client () < 0) {
         return 0;
      }
   
      /* determine result length first */
      MUTEX_GET (chnmux);
      size = 0;
      for (i = 0; i < chninfonum; i++) {
         if ((query != NULL) && !query (chninfo + i)) {
            continue;
         }
         if ((ifo < 0) || (chninfo[i].ifoId == ifo)) {
            size += strlen (chninfo[i].chName) + 1;
            if ((info & 1) && (chninfo[i].dataRate > 0)) {
               char buf[256];
               sprintf (buf, " %i", chninfo[i].dataRate);
               size += strlen (buf);
            }
         }
      }
      /* allocate return list */
      ret = malloc (size + 10);
      if (ret == NULL) {
         MUTEX_RELEASE (chnmux);
         return NULL;
      }
      *ret = 0;
      p = ret;
      /* copy names */
      for (i = 0, n = 0; i < chninfonum; i++) {
         if ((query != NULL) && !query (chninfo + i)) {
            continue;
         }
         if ((ifo < 0) || (chninfo[i].ifoId == ifo)) {
            if (n > 0) {
               strcpy (p, " ");
               p++;
            }
            strcpy (p, chninfo[i].chName);
            p += strlen (p);
            if ((info & 1) && (chninfo[i].dataRate > 0)) {
               char buf[256];
               sprintf (buf, " %i", chninfo[i].dataRate);
               strcpy (p, buf);
               p += strlen (p);
            }
            n++;
         }
      }
      MUTEX_RELEASE (chnmux);
   
      return ret;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: gdsChannelSetHostAddress			*/
/*                                                         		*/
/* Procedure Description: set nds address				*/
/* 									*/
/* Procedure Arguments: server name, port #				*/
/*                                                         		*/
/* Procedure Returns: 0 if successful; <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int gdsChannelSetHostAddress (const char* hostname, int port)
   {
   #if (_CHANNEL_DB==_CHN_DB_DAQ)
      if (hostname == NULL) {
         return -1;
      }
      daqSetUser = 1;
      strncpy (daqServer, hostname, sizeof (daqServer) - 1);
      daqServer[sizeof (daqServer) - 1] = 0;
      daqPort = (port <= 0) ? DAQD_PORT : port;
   #endif
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: gdsChannelSetDBAddress			*/
/*                                                         		*/
/* Procedure Description: set channel db address			*/
/* 									*/
/* Procedure Arguments: server name, prog. num, prog. version		*/
/*                                                         		*/
/* Procedure Returns: 0 if successful; <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int gdsChannelSetDBAddress (const char* hostname, 
                     unsigned long prognum, unsigned long progver)
   {
   #if defined (_CONFIG_DYNAMIC)
      if (hostname == NULL) {
         return -1;
      }
      strncpy (dbServer, hostname, sizeof (dbServer) - 1);
      dbServer[sizeof (dbServer) - 1] = 0;
      dbPrognum = (prognum == 0) ? RPC_PROGNUM_GDSCHN : prognum;
      dbProgver = (progver == 0) ? RPC_PROGVER_GDSCHN : progver;
   #endif
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: channel_client				*/
/*                                                         		*/
/* Procedure Description: initialzed channel info			*/
/* 									*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: 0 if successful; <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int channel_client (void)
   {
   #if defined (_CONFIG_DYNAMIC)
      const char* const* cinfo;		/* configuration info */
      confinfo_t	crec;		/* conf. info record */
   #endif
   
      /* already initialized */
      if (chn_init >= 2) {
         return 0;
      }

      /* intialize interface first */
      if (chn_init == 0) {
         initChnInfo ();
         if (chn_init == 0) {
            gdsError (GDS_ERR_MEM, "failed to initialze channel API");
            return -1;
         }
      }
   
      /* get nds parameters */
   #if defined (_CONFIG_DYNAMIC)
   #if (_CHANNEL_DB==_CHN_DB_DAQ)
      if (!daqSetUser) {
   #endif
         for (cinfo = getConfInfo (0, 0); cinfo && *cinfo; cinfo++) {
            if ((parseConfInfo (*cinfo, &crec) == 0) &&
               (gds_strcasecmp (crec.interface, 
               CONFIG_SERVICE_NDS) == 0) &&
               (crec.ifo == -1) && (crec.progver == -1)) {
               gdsChannelSetHostAddress (crec.host, crec.port_prognum);
            }
         #ifndef _NO_TESTPOINTS
            if ((parseConfInfo (*cinfo, &crec) == 0) &&
               (gds_strcasecmp (crec.interface, 
               CONFIG_SERVICE_CHN) == 0) &&
               (crec.ifo == -1) && (crec.port_prognum > 0) &&
               (crec.progver > 0)) {
               gdsChannelSetDBAddress (crec.host, crec.port_prognum,
                                    crec.progver);
            }
         #endif
         }
   #if (_CHANNEL_DB==_CHN_DB_DAQ)
      }
   #endif
   #endif
   
      /* read channel information */
      if (readChnInfo() < 0) {
         gdsError (GDS_ERR_MEM, 
                  "unable to read channel database");
         return -2;
      }
      /* return */
      chn_init = 2;
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: initChnInfo					*/
/*                                                         		*/
/* Procedure Description: initialzed channel info			*/
/* 									*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: 0 if successful; <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void initChnInfo (void)
   {
      if (chn_init > 0) {
         return;
      }
      /* First time, log version ID. */
      printf("channel_client %s\n", versionId) ;

      if (chninfo == NULL) {
         if (MUTEX_CREATE (chnmux) != 0) {
            gdsError (GDS_ERR_MEM, 
                     "unable to inialize channel database");
            return;
         }
         chninfo = malloc (_CHNLIST_SIZE * sizeof (gdsChnInfo_t));
         chninfonum = 0;
         chninfosize = _CHNLIST_SIZE;
         if (chninfo == NULL) {
            gdsError (GDS_ERR_MEM, 
                     "unable to inialize channel database");
            return;
         }
      }
      chn_init = 1;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: channel_cleanup				*/
/*                                                         		*/
/* Procedure Description: terminates channel info			*/
/* 									*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   void channel_cleanup (void)
   {
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: finiChnInfo					*/
/*                                                         		*/
/* Procedure Description: terminates channel info			*/
/* 									*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void finiChnInfo (void)
   {
      if (chninfo != NULL) {
         free (chninfo);
         chninfo = NULL;
         chninfonum = 0;
         chninfosize = 0;
         MUTEX_DESTROY (chnmux);
      }
      chn_init = 0;
      return;
   }


#if !defined (_CONFIG_DYNAMIC) && \
    ((_CHANNEL_DB == _CHN_DB_PARAM) || !defined (_NO_TESTPOINTS))
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: readChnFile					*/
/*                                                         		*/
/* Procedure Description: reads channel info from file			*/
/* 			  mutex MUST be owned by caller			*/
/*                                                         		*/
/* Procedure Arguments: filename					*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 otherwise.			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int readChnFile (const char* filename)
   {
      FILE*		fp;		/* channel info file */
      char		section[PARAM_ENTRY_LEN]; /* section name */
      char		u[PARAM_ENTRY_LEN]; /* unit name */
      char*		sec;		/* pointer to section */
      int		nentry;		/* number of section entries */
      int		cursor;		/* section cursor */
      char*		p;		/* temp ptr */
      int		chninfoprev;	/* # of already loaded channels */
      gdsChnInfo_t*	chnptr;     	/* pointer to chn info */
   
      printf("read %s\n", filename);

      /* open parameter file */
      if ((fp = fopen (filename, "r")) == NULL) {
         return -1;
      }
   
      /* loop through sections in parameter file */
      chninfoprev = chninfonum;
      while (nextParamFileSection (fp, section) != NULL) {
         /* check if enough memory */
         if (chninfonum >= chninfosize - 2) {
            if (resizeChnInfo (0) != 0) {
               free (chninfo);
               chninfo = NULL;
               chninfonum = 0;
               chninfosize = 0;
               fclose (fp);
               return -10;
            }
         }
         /* now add elements */
         sec = getParamFileSection (fp, NULL, &nentry, 0);
         if (sec == NULL) {
            free (chninfo);
            chninfo = NULL;
            chninfonum = 0;
            chninfosize = 0;
            fclose (fp);
            return -11;
         }
         memset (chninfo + chninfonum, 0, sizeof (gdsChnInfo_t));
         strncpy (chninfo[chninfonum].chName, section, MAX_CHNNAME_SIZE - 1);
         chninfo[chninfonum].chName[MAX_CHNNAME_SIZE - 1] = 0;
         p = chninfo[chninfonum].chName;
         while (*p != '\0') {
            *p = toupper (*p);
            p++;
         }
         cursor = -1;
         chninfo[chninfonum].ifoId = 0;
         loadParamSectionEntry (PRM_IFOID, sec, nentry, &cursor, 
                              5, &chninfo[chninfonum].ifoId);
         chninfo[chninfonum].rmId = 0;
         loadParamSectionEntry (PRM_RMID, sec, nentry, &cursor, 
                              5, &chninfo[chninfonum].rmId);

	 printf("%s rmid %d\n", chninfo[chninfonum].chName, chninfo[chninfonum].rmId);
         chninfo[chninfonum].dcuId = -1;
         loadParamSectionEntry (PRM_DCUID, sec, nentry, &cursor, 
                              5, &chninfo[chninfonum].dcuId);
         chninfo[chninfonum].chNum = 0;
         loadParamSectionEntry (PRM_CHNNUM, sec, nentry, &cursor, 
                              5, &chninfo[chninfonum].chNum);
         chninfo[chninfonum].tpNum = chninfo[chninfonum].chNum;
         chninfo[chninfonum].dataType = 1;
         loadParamSectionEntry (PRM_DATATYPE, sec, nentry, &cursor, 
                              5, &chninfo[chninfonum].dataType);
         chninfo[chninfonum].dataRate = 14;
         loadParamSectionEntry (PRM_DATARATE, sec, nentry, &cursor, 
                              1, &chninfo[chninfonum].dataRate);
         chninfo[chninfonum].gain = 1;
         loadParamSectionEntry (PRM_GAIN, sec, nentry, &cursor, 
                              6, &chninfo[chninfonum].gain);
         chninfo[chninfonum].slope = 1;
         loadParamSectionEntry (PRM_SLOPE, sec, nentry, &cursor, 
                              6, &chninfo[chninfonum].slope);
         chninfo[chninfonum].offset = 0;
         loadParamSectionEntry (PRM_OFFSET, sec, nentry, &cursor, 
                              6, &chninfo[chninfonum].offset);
         strcpy (u, "");
         loadParamSectionEntry (PRM_OFFSET, sec, nentry, &cursor, 3, u);
         strncpy (chninfo[chninfonum].unit, u, 32);
         chninfo[chninfonum].unit[31] = 0;
         chninfo[chninfonum].unit[31] = 0;
         p = chninfo[chninfonum].unit;
         while (*p != '\0') {
            *p = toupper (*p);
            p++;
         }
         chninfo[chninfonum].rmOffset = 0;
         loadParamSectionEntry (PRM_RMOFFSET, sec, nentry, &cursor, 
                              4, &chninfo[chninfonum].rmOffset);
         chninfo[chninfonum].rmBlockSize = 0;
         loadParamSectionEntry (PRM_RMBLOCKSIZE, sec, nentry, &cursor, 
                              4, &chninfo[chninfonum].rmBlockSize);
         /* test if this is a new channel */
	 /*printf ("new channel `%s' ifoId %d\n", chninfo[chninfonum].chName,
		 chninfo[chninfonum].ifoId); */
         chnptr = bsearch (chninfo + chninfonum, chninfo, 
                          chninfoprev, sizeof (gdsChnInfo_t), 
                          (int (*) (const void*, const void*)) gds_strcasecmp);
         if (chnptr == NULL) {
            chninfonum++;
         }
         else { /* override old one */
            *chnptr = chninfo[chninfonum];
         }
         free (sec);
      }
      
      /* close the file */
      fclose (fp);
      return 0;
   }
#endif


#if (_CHANNEL_DB == _CHN_DB_DAQ) &&  !defined(OS_VXWORKS)
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: readChnDAQServer				*/
/*                                                         		*/
/* Procedure Description: reads channel info from DAQ server		*/
/* 			  mutex MUST be owned by caller			*/
/*                                                         		*/
/* Procedure Arguments: server name, server port			*/
/*                                                         		*/
/* Procedure Returns: >=0 if successful, <0 otherwise.			*/
/*                    0: TP numbers were not read in			*/
/*                    1: TP numbers were read in			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int readChnDAQServer (char* server, int port) 
   {
      int		sock;		/* socket */
      struct sockaddr_in name;		/* socket name */
      char		buf[1024];	/* buffer */
      int		i;		/* index */
      int 		channelnum;	/* channel number */
      int		chninfoprev;	/* # of already loaded channels */
      int		mVersion;	/* version # of server */
      int		mRevision;	/* revision # of server */
      int	        tpnumInTrend;	/* tpNum stored in trend field */
      int		extendedList;	/* extended listing */
      int		longNames;	/* channel name length == MAX_CHNNAME_SIZE */
      int		nameOffset;	/* either 40 or MAX_CHNNAME_SIZE */
      char*		p;		/* temp pointer */
      struct timeval	timeout = 	
        {_TIMEOUT / 1000000, _TIMEOUT % 1000000}; 
      //wait_time timeout = _TIMEOUT / 1000000.; /* timeout */

      /* create the socket */
      sock = socket (PF_INET, SOCK_STREAM, 0);
      if (sock == -1) {
         return -1;
      }
   
      /* fill destination address */
      name.sin_family = AF_INET;
      name.sin_port = htons (port);  /* convert to network byte order */
      if (nslookup (server, &name.sin_addr) < 0) {
         close (sock);
         return -2;
      }
   
      /* connect to the NDS  */
      if (connectWithTimeout (sock, (struct sockaddr *) &name, 
         sizeof (name), &timeout) < 0) {
         close (sock);
         return -3;
      }
   
      /* Get the server version number */
      if (SendRequest(sock, "version;", buf, 4, 0)) {
         close (sock);
         return -4;
      }
      mVersion = CVHex(buf, 4);
      if (SendRequest(sock, "revision;", buf, 4, 0)) {
         close (sock);
         return -5;
      }
      mRevision = CVHex(buf, 4);
      tpnumInTrend = ((mVersion > 11) || 
                     ((mVersion == 11) && (mRevision >= 1)));
      extendedList =  ((mVersion > 11) || 
                      ((mVersion == 11) && (mRevision >= 3)));
      longNames = (mVersion >= 12) ;
      /*printf ("Version: %i   Revision: %i\n", mVersion, mRevision);*/
   
      /* Request a list of channels. */
      if (extendedList) {
         if (SendRequest(sock, "status channels 2;", buf, 8, 0)) {
            printf ("SendRequest failed\n");
            close (sock);
            return -6;
         }
         channelnum = CVHex(buf, 8);
      }
      else {
         if (SendRequest(sock, "status channels;", buf, 4, 0)) {
            close (sock);
            return -6;
         }
         channelnum = CVHex(buf, 4);
      }
      /*printf ("number of channels = %i\n", channelnum);*/
   
      /* Skip undocumented field. */
      if (!extendedList) RecvRec (sock, buf, 4, 1);
   
      /* printf (" chn name                                      "
             "rate type bps group\n");*/
      /* read channel info from socket stream */
      chninfoprev = chninfonum;
      for (i = 0; i < channelnum; i++) {
         int		j;	/* buffer index */
         int		recsz;	/* receive record size */
         int 		rc;	/* received bytes */
      
         /* check if enough memory */
         if (chninfonum >= chninfosize - 2) {
            if (resizeChnInfo (0) != 0) {
               free (chninfo);
               chninfo = NULL;
               chninfonum = 0;
               chninfosize = 0;
               close (sock);
               return -10;
            }
         }
         memset (chninfo + chninfonum, 0, sizeof (gdsChnInfo_t));
         recsz = 52;
         if ((mVersion == 9) || (mVersion == 10)) recsz = 60;
         if (mVersion == 11) recsz = 124;
         if (extendedList) recsz = 128;
	 /* if longNames, the size needs to be increased for longer
	  * names.  Old name size is 40, new name size is MAX_CHNNAME_SIZE */
	 if (longNames) recsz = 128 + (MAX_CHNNAME_SIZE - 40) ;
         rc = RecvRec (sock, buf, recsz, 1);
         if (rc < recsz) {
            free (chninfo);
            chninfo = NULL;
            chninfonum = 0;
            chninfosize = 0;
            close (sock);
            return -11;
         }
         j = 40;
	 if (longNames) j = MAX_CHNNAME_SIZE ;
         while ((--j >= 0) && (buf[j] == ' ')) {
            buf[j] = '\0';
         }
	 if (longNames)
	    memcpy (chninfo[chninfonum].chName, buf, MAX_CHNNAME_SIZE);
	 else
	    memcpy (chninfo[chninfonum].chName, buf, 40);
	 if (longNames)
	    p = chninfo[chninfonum].chName + (MAX_CHNNAME_SIZE - 1) ;
	 else
	    p = chninfo[chninfonum].chName + 39;
         for (; isspace ((int)*p) && (p > chninfo[chninfonum].chName); p--) {
         }
	 if (longNames)
	    nameOffset = MAX_CHNNAME_SIZE ;
	 else
	    nameOffset = 40 ;
         if (extendedList) {
            chninfo[chninfonum].dataRate  = CVHex (buf + nameOffset, 8);
            chninfo[chninfonum].chNum = CVHex (buf + nameOffset + 8, 8);
            chninfo[chninfonum].tpNum = chninfo[chninfonum].chNum;
            chninfo[chninfonum].chGroup = CVHex (buf + nameOffset + 16, 4);
            chninfo[chninfonum].bps = 0;
            chninfo[chninfonum].dataType = CVHex (buf + nameOffset + 20, 4);
            *((int*)(&chninfo[chninfonum].gain)) = CVHex(buf + nameOffset + 24, 8);
            *((int*)(&chninfo[chninfonum].slope)) = CVHex(buf + nameOffset + 32, 8);
            *((int*)(&chninfo[chninfonum].offset)) = CVHex(buf + nameOffset + 40, 8);
            memcpy(chninfo[chninfonum].unit, buf + nameOffset + 48, 40);
            for (p = chninfo[chninfonum].unit + 39; 
                isspace ((int)*p) && (p > chninfo[chninfonum].unit); p--) {
            }
            *p = 0;
            /*printf ("name = %s\n", chninfo[chninfonum].chName);
            printf ("rate = %i\n", chninfo[chninfonum].dataRate);
            printf ("chNum = %i\n", chninfo[chninfonum].chNum);
            printf ("group = %i\n", chninfo[chninfonum].chGroup);
            printf ("bps = %i\n", chninfo[chninfonum].bps);
            printf ("type = %i\n", chninfo[chninfonum].dataType);
            printf ("gain = %f\n", chninfo[chninfonum].gain);
            printf ("slope = %f\n", chninfo[chninfonum].slope);
            printf ("offset = %f\n", chninfo[chninfonum].offset);
            printf ("unit = %s\n", chninfo[chninfonum].unit);*/
         }
         else {
            chninfo[chninfonum].dataRate  = CVHex (buf + 40, 4);
         /* trend field used for tpnum */
         /*if  (tpnumInTrend) {
            chninfo[chninfonum].tpNum = CVHex (buf + 44, 4);
            if (chninfo[chninfonum].tpNum > 0) {
               chninfo[chninfonum].chNum = chninfo[chninfonum].tpNum;
            }
         }*/
            chninfo[chninfonum].chGroup = CVHex (buf + 48, 4);
            if (recsz > 52) {
               chninfo[chninfonum].bps = CVHex (buf + 52, 4);
               chninfo[chninfonum].dataType = CVHex (buf + 56, 4);
            }
            else {
               chninfo[chninfonum].bps = 0;
               chninfo[chninfonum].dataType = 0;
            }
            if (recsz > 60) {
               *((int*)(&chninfo[chninfonum].gain)) = CVHex(buf+60, 8);
               *((int*)(&chninfo[chninfonum].slope)) = CVHex(buf+68, 8);
               *((int*)(&chninfo[chninfonum].offset)) = CVHex(buf+76, 8);
               memcpy(chninfo[chninfonum].unit, buf+84, 40);
               for (p = chninfo[chninfonum].unit + 39; 
                   isspace ((int)*p) && (p > chninfo[chninfonum].unit); p--) {
               }
               *p = 0;
            }
            else {
               chninfo[chninfonum].gain = 1;
               chninfo[chninfonum].slope = 1;
               chninfo[chninfonum].offset = 0;
               strcpy (chninfo[chninfonum].unit, "");
            }
         }
      	 /* guess ifoId and rmId */
         switch (chninfo[chninfonum].chName[1]) {
            case '1': 
               {
                  chninfo[chninfonum].ifoId = 1;
                  break;
               }
            case '2': 
               {
                  chninfo[chninfonum].ifoId = 2;
                  break;
               }
            case '0': 
            default:
               {
                  chninfo[chninfonum].ifoId = 0;
                  break;
               }
         }
         chninfo[chninfonum].rmId = (chninfo[chninfonum].ifoId - 1) % 2;
         /*if (chninfo[chninfonum].dataRate > 16384) {
            printf ("%4i %40s %1i %5i %4i %3i %5i %5i\n", i, 
                   chninfo[chninfonum].chName, 
                   chninfo[chninfonum].ifoId, 
                   chninfo[chninfonum].dataRate, 
                   chninfo[chninfonum].dataType, 
                   chninfo[chninfonum].bps, 
                   chninfo[chninfonum].chGroup,
                   chninfo[chninfonum].tpNum);
         }*/
         /* test if this is a new channel */
         if (bsearch (chninfo + chninfonum, chninfo, 
            chninfoprev, sizeof (gdsChnInfo_t), 
            (int (*) (const void*, const void*)) gds_strcasecmp) == NULL) {
            chninfonum++;
         }
      }
   
      /* quit */
      strcpy (buf, "quit;");
      write (sock, buf, strlen (buf));
      close (sock);
      return (tpnumInTrend > 0);
   }
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: readChnInfo					*/
/*                                                         		*/
/* Procedure Description: reads channel info into internal array	*/
/* 									*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 otherwsie			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int readChnInfo (void)
   {
   
   #ifndef OS_VXWORKS
      int 		ret = 0;	/* return value */
   #endif
      int 		tpLoaded = 0;	/* true if TP were loaded */

      /* shrink size if necessary */
      MUTEX_GET (chnmux);
      if (chninfosize > _CHNLIST_SIZE) {
         resizeChnInfo (_CHNLIST_SIZE);
      }

      /* sort entries */
      qsort ((void*) chninfo, chninfonum, sizeof (gdsChnInfo_t), 
            (int (*) (const void*, const void*)) gds_strcasecmp);

   
      /* read in test point information */
   #ifndef _NO_TESTPOINTS
      /* read from server */
   #if defined (_CONFIG_DYNAMIC)
      {
         CLIENT*		clnt;		/* rpc client handle */
         struct timeval 	timeout;	/* connect timeout */
         resultChannelQuery_r	res;		/* rpc return */
         int			i;		/* channel list index */
         channelinfo_r*		info;		/* infor pointer */
         char*			p;		/* pointer into name */
         int			chninfoprev;	/* # of already loaded channels */
         gdsChnInfo_t*		chnptr;     	/* pointer to chn info */
      
         timeout.tv_sec = RPC_PROBE_WAIT;
         timeout.tv_usec = 0;
         /* printf ("server %s @ %li/%li\n", dbServer, 
      	    	    dbPrognum, dbProgver); */
         if ((strlen (dbServer) > 0) && rpcProbe (dbServer, dbPrognum, 
            dbProgver, _NETID, &timeout, &clnt)) {
         
            /* get channel database entries */
            memset (&res, 0, sizeof (resultChannelQuery_r));
            if ((chnquery_1 (&res, clnt) == RPC_SUCCESS) &&
               (res.status == 0)) {
               /* copy list */
               chninfoprev = chninfonum;
               info = res.chnlist.channellist_r_val;
               for (i = 0; i < res.chnlist.channellist_r_len; 
                   i++, info++) {
                  /* check if enough memory */
                  if (chninfonum >= chninfosize - 2) {
                     if (resizeChnInfo (0) != 0) {
                        free (chninfo);
                        chninfo = NULL;
                        chninfonum = 0;
                        chninfosize = 0;
                        xdr_free ((xdrproc_t)xdr_resultChannelQuery_r, 
                                 (caddr_t) &res);
                        return -1;
                     }
                  }
                  memset (chninfo + chninfonum, 0, sizeof (gdsChnInfo_t));
                  strncpy (chninfo[chninfonum].chName, info->chName, MAX_CHNNAME_SIZE - 1);
                  chninfo[chninfonum].chName[31] = 0;
                  p = chninfo[chninfonum].chName;
                  while (*p != '\0') {
                     *p = toupper (*p);
                     p++;
                  }
                  chninfo[chninfonum].ifoId = info->ifoId;
		 /* We want to use rmId to set node id for advLIGO */
                  chninfo[chninfonum].rmId = info->rmId;
                  chninfo[chninfonum].dcuId = info->dcuId;
                  chninfo[chninfonum].chNum = info->chNum;
                  chninfo[chninfonum].dataType = info->dataType;
                  chninfo[chninfonum].dataRate = info->dataRate;
                  chninfo[chninfonum].chGroup = info->chGroup;
                  chninfo[chninfonum].bps = info->bps;
                  chninfo[chninfonum].gain = info->gain;
                  chninfo[chninfonum].slope = info->slope;
                  chninfo[chninfonum].offset = info->offset;
                  chninfo[chninfonum].tpNum = info->chNum;
                  strncpy (chninfo[chninfonum].unit, info->unit, 32);
                  chninfo[chninfonum].unit[31] = 0;
                  p = chninfo[chninfonum].unit;
                  while (*p != '\0') {
                     *p = toupper (*p);
                     p++;
                  }
                  chninfo[chninfonum].rmOffset = info->rmOffset;
                  chninfo[chninfonum].rmBlockSize = info->rmBlockSize;
                  /* test if this is a new channel */
                  chnptr = bsearch (chninfo + chninfonum, chninfo, 
                                   chninfoprev, sizeof (gdsChnInfo_t), 
                                   (int (*) (const void*, const void*)) gds_strcasecmp);
                  if (chnptr == NULL) {
                     /*printf ("New channel %s\n", chninfo[chninfonum].chName);*/
                     chninfonum++;
                  }
                  else { /* override old one */
                     /*if ((chninfo[chninfonum].ifoId !=  chnptr->ifoId) ||
   			(chninfo[chninfonum].rmId !=  chnptr->rmId) ||
   			(chninfo[chninfonum].chNum !=  chnptr->chNum) ||
   			(chninfo[chninfonum].dataType !=  chnptr->dataType) ||
   			(chninfo[chninfonum].dataRate !=  chnptr->dataRate) ||
   			(chninfo[chninfonum].tpNum !=  chnptr->tpNum)) {
                        printf ("Channel mismatch %s\n", chninfo[chninfonum].chName);
                        printf ("   ifoId    %7i %7i\n", chninfo[chninfonum].ifoId, chnptr->ifoId);
                        printf ("   rmId     %7i %7i\n", chninfo[chninfonum].rmId, chnptr->rmId);
                        printf ("   chNum    %7i %7i\n", chninfo[chninfonum].chNum, chnptr->chNum);
                        printf ("   dataType %7i %7i\n", chninfo[chninfonum].dataType, chnptr->dataType);
                        printf ("   dataRate %7i %7i\n", chninfo[chninfonum].dataRate, chnptr->dataRate);
                        printf ("   tpNum    %7i %7i\n", chninfo[chninfonum].tpNum, chnptr->tpNum);
   		     }*/
                     *chnptr = chninfo[chninfonum];
                  }
               }
            }
            xdr_free ((xdrproc_t)xdr_resultChannelQuery_r, (caddr_t) &res);
            clnt_destroy (clnt);
         }
      }
   
      /* read from file */
   #else 

      // Read only the correct file
      extern char myParFile[128];
      if (readChnFile (myParFile) <= -10) {
               MUTEX_RELEASE (chnmux);
               return -1;
      }

   #endif
      /* sort entries */
      qsort ((void*) chninfo, chninfonum, sizeof (gdsChnInfo_t), 
            (int (*) (const void*, const void*)) gds_strcasecmp);
   #endif
   
      MUTEX_RELEASE (chnmux);
      return 0;
   }

#if 0
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: readChnInfo					*/
/*                                                         		*/
/* Procedure Description: reads channel info into internal array	*/
/* 									*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 otherwsie			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int readChnInfo (void)
   {
      /* shrink size if necessary */
      MUTEX_GET (chnmux);
      if (chninfosize > _CHNLIST_SIZE) {
         resizeChnInfo (_CHNLIST_SIZE);
      }
   
      /* read in test point information */
   #ifndef _NO_TESTPOINTS
      /* read from server */
   #if defined (_CONFIG_DYNAMIC)
      {
         CLIENT*		clnt;		/* rpc client handle */
         struct timeval 	timeout;	/* connect timeout */
         resultChannelQuery_r	res;		/* rpc return */
         int			i;		/* channel list index */
         channelinfo_r*		info;		/* infor pointer */
         char*			p;		/* pointer into name */
      
         timeout.tv_sec = RPC_PROBE_WAIT;
         timeout.tv_usec = 0;
         /* printf ("server %s @ %li/%li\n", dbServer, 
      	    	    dbPrognum, dbProgver); */
         if ((strlen (dbServer) > 0) && rpcProbe (dbServer, dbPrognum, 
            dbProgver, _NETID, &timeout, &clnt)) {
         
            /* get channel database entries */
            memset (&res, 0, sizeof (resultChannelQuery_r));
            if ((chnquery_1 (&res, clnt) == RPC_SUCCESS) &&
               (res.status == 0)) {
               /* copy list */
               info = res.chnlist.channellist_r_val;
               for (i = 0; i < res.chnlist.channellist_r_len; 
                   i++, info++) {
               /* check if enough memory */
                  if (chninfonum >= chninfosize - 2) {
                     if (resizeChnInfo (0) != 0) {
                        free (chninfo);
                        chninfo = NULL;
                        chninfonum = 0;
                        chninfosize = 0;
                        xdr_free ((xdrproc_t)xdr_resultChannelQuery_r, 
                                 (caddr_t) &res);
                        return -1;
                     }
                  }
                  memset (chninfo + chninfonum, 0, sizeof (gdsChnInfo_t));
                  strncpy (chninfo[chninfonum].chName, info->chName, MAX_CHNNAME_SIZE-1);
                  chninfo[chninfonum].chName[31] = 0;
                  p = chninfo[chninfonum].chName;
                  while (*p != '\0') {
                     *p = toupper (*p);
                     p++;
                  }
                  chninfo[chninfonum].ifoId = info->ifoId;
                  /*chninfo[chninfonum].rmId = info->rmId;*/
                  chninfo[chninfonum].rmId = (info->ifoId - 1) % 2;
                  chninfo[chninfonum].dcuId = info->dcuId;
                  chninfo[chninfonum].chNum = info->chNum;
                  chninfo[chninfonum].dataType = info->dataType;
                  chninfo[chninfonum].dataRate = info->dataRate;
                  chninfo[chninfonum].chGroup = info->chGroup;
                  chninfo[chninfonum].bps = info->bps;
                  chninfo[chninfonum].gain = info->gain;
                  chninfo[chninfonum].slope = info->slope;
                  chninfo[chninfonum].offset = info->offset;
                  chninfo[chninfonum].tpNum = info->chNum;
                  strncpy (chninfo[chninfonum].unit, info->unit, 32);
                  chninfo[chninfonum].unit[31] = 0;
                  p = chninfo[chninfonum].unit;
                  while (*p != '\0') {
                     *p = toupper (*p);
                     p++;
                  }
                  chninfo[chninfonum].rmOffset = info->rmOffset;
                  chninfo[chninfonum].rmBlockSize = info->rmBlockSize;
                  chninfonum++;
               }
            }
            xdr_free ((xdrproc_t)xdr_resultChannelQuery_r, (caddr_t) &res);
            clnt_destroy (clnt);
         }
      }
   
      /* read from file */
   #else 
      {
         int		ifo;
         char		filename[1024];
         for (ifo = 0; ifo < TP_MAXIFO; ifo++) {
            sprintf (filename, TP_FILE, ifo + 1);
            if (readChnFile (filename) <= -10) {
               MUTEX_RELEASE (chnmux);
               return -1;
            }
         }
      }
   #endif
      /* sort entries */
      qsort ((void*) chninfo, chninfonum, sizeof (gdsChnInfo_t), 
         (int (*) (const void*, const void*)) gds_strcasecmp);
   #endif
   
      /* read in other channel information */
   #if (_CHANNEL_DB == _CHN_DB_PARAM) 
   #if !defined (_CONFIG_DYNAMIC)
      if (readChnFile (PRM_FILE) <= -10) {
         char		msg[256];
         MUTEX_RELEASE (chnmux);
         sprintf (msg, "Unable to load channel information from %s\n", 
                 PRM_FILE);
      #ifdef _AWG_LIB
         gdsWarningMessage (msg);
      #else
         gdsError (GDS_ERR_MISSING, msg);
      #endif
         return 0;
      }
   #endif
   #else
   #ifdef OS_VXWORKS
      MUTEX_RELEASE (chnmux);
      return 0; 
   #endif
      /* get NDS parameters */
   #if !defined (_CONFIG_DYNAMIC)
      if (!daqSetUser) {
         strcpy (daqServer, DAQD_SERVER);
         loadStringParam (PRM_FILE2, SEC_SERVER, PRM_SERVERNAME, daqServer);
         daqPort = DAQD_PORT;
         loadIntParam (PRM_FILE2, SEC_SERVER, PRM_SERVERPORT, &daqPort);
      }
   #endif
      /* read from NDS */
      if (readChnDAQServer (daqServer, daqPort) < 0) {
         char		msg[256];
         MUTEX_RELEASE (chnmux);
         sprintf (msg, "Unable to load channel information from "
            "%s / %i\n", daqServer, daqPort);
      #ifdef _AWG_LIB
         gdsWarningMessage (msg);
      #else
         gdsError (GDS_ERR_MISSING, msg);
      #endif
         return 0;
      }
   #endif
   
      /* sort entries */
      qsort ((void*) chninfo, chninfonum, sizeof (gdsChnInfo_t), 
         (int (*) (const void*, const void*)) gds_strcasecmp);
      MUTEX_RELEASE (chnmux);
      return 0;
   }
#endif

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: resizeChnInfo				*/
/*                                                         		*/
/* Procedure Description: resized the channel info list			*/
/* 			  the calling routine MUST own chnmux!		*/
/* 									*/
/* Procedure Arguments: new len, or 0 for adding _CHNLIST_SIZE 		*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int resizeChnInfo (int newlen)
   {
      int 		nsize;		/* new size */
      gdsChnInfo_t*	newlist;	/* new resized list */
   
      /* determine new size */
      nsize = (newlen != 0) ? newlen : chninfosize + _CHNLIST_SIZE;
      if (nsize == chninfosize) {
         return 0;
      }
   
      /* resize info list */
      newlist = realloc (chninfo, nsize * sizeof (gdsChnInfo_t));
      if (newlist == 0) {
         return -1;
      }
   
      /* cleanup and return */
      chninfo = newlist;
      chninfosize = nsize;
      return 0;
   }


#if (_CHANNEL_DB==_CHN_DB_DAQ) && !defined(OS_VXWORKS)
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: CVHex					*/
/*                                                         		*/
/* Procedure Description: convert ASCII hex into int			*/
/* 									*/
/* Procedure Arguments: text, number of digits				*/
/*                                                         		*/
/* Procedure Returns: converted number					*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int CVHex (const char *text, int N) {
      int v = 0;
      int i;
      for (i=0 ; i<N ; i++) {
         v<<=4;
         if      ((text[i] >= '0') && (text[i] <= '9')) v += text[i] - '0';
         else if ((text[i] >= 'a') && (text[i] <= 'f')) v += text[i] - 'a' + 10;
         else if ((text[i] >= 'A') && (text[i] <= 'F')) v += text[i] - 'A' + 10;
         else 
            return -1;
      }
      return v;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: RecvRec					*/
/*                                                         		*/
/* Procedure Description: receive record from NDS			*/
/* 									*/
/* Procedure Arguments: socket, buffer, buffer length, read all?	*/
/*                                                         		*/
/* Procedure Returns: number of read bytes				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int RecvRec (int mSocket, char *buffer, int length, 
                     int readall) {
      char* point = buffer;
      int   nRead = 0;
      do {
         int nB = recv(mSocket, point, length - nRead, 0);
         if (nB <= 0) 
            return -1;
         point += nB;
         nRead += nB;
      } while (readall && (nRead < length));
      return nRead;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: SendRequest					*/
/*                                                         		*/
/* Procedure Description: send request to NDS				*/
/* 									*/
/* Procedure Arguments: socket, command, reply buffer, buffer length,	*/
/*			size of returned reply buffer			*/
/*                                                         		*/
/* Procedure Returns: 0 if successful					*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int SendRequest (int mSocket, const char* text, char *reply, 
                     int length, int *Size) {
      char status[4];
      int rc;
    /* Send the request */
   #if defined(OS_VXWORKS) || defined(__CYGWIN__)
      rc = send (mSocket, (char*) text, strlen(text), 0 /*MSG_EOR*/);
   #else
      rc = send (mSocket, (char*) text, strlen(text), MSG_EOR);
   #endif
      if (rc <= 0) 
         return rc;
   
    /* Return if no reply expected. */
      if (reply == 0) 
         return 0;
   
    /* Read the reply status. */
      rc = RecvRec (mSocket, status, 4, 0);
      if (rc != 4) 
         return -1;
      rc = CVHex(status, 4);
      if (rc) 
         return rc;
   
    /* Read the reply text. */
      if (length != 0) {
         rc = RecvRec(mSocket, reply, length, 0);
         if (rc < 0)      
            return rc;
         if (rc < length) reply[rc] = 0;
         if (Size)       *Size = rc;
      }
      return 0;
   }
#endif

