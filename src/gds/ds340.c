static char *versionId = "Version $Id$" ;
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: ds340							*/
/*                                                         		*/
/* Module Description: implements functions for controlling a DS340	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

/* Header File List: */
#ifdef OS_VXWORKS
#include <vxWorks.h>
#include <selectLib.h>
#include <ioLib.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include "dtt/gdsutil.h"
#include "dtt/gdstask.h"
#include "dtt/cobox.h"
#include "dtt/ds340.h"

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Constants: _DS340_MICROWAIT	  micro wait				*/
/*            _WAIT_FOR_ANSWER	  wait for answer from DS340 (in units	*/
/*				  of microwaits)			*/
/*            _ID_STRING	  id for DS340				*/
/*            								*/
/*----------------------------------------------------------------------*/
#define _DS340_MICROWAIT	62500		/* 1/16 sec */
#define _WAIT_FOR_ANSWER	32		/* 2 sec */
#define _ID_STRING		"StanfordResearchSystems,DS34"
#define _ADC_RANGE		2047


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Types: _DS340_ConfigBlock	  DS340 configuration block		*/
/*            								*/
/*----------------------------------------------------------------------*/
   struct _DS340_ConfigBlock {
      /** Status flag. */
      int 		status;
      /** Toggle bits. */
      int 		toggles;
      /** Standard event status byte. */
      int 		es_byte;
      /** Serial poll byte. */
      int 		sps_byte;
      /** DDS status bit. */
      int 		dds_byte;
      /** DS340 identification string. */
      char 		name[64];
      /** Function type. */
      DS340_FuncType 	func;
      /** Amplitude. */
      float 		ampl;
      /** Frequency. */
      float 		freq;
      /** Offset. */
      float 		offs;
      /** Sweep start frequency. */
      float 		stfr;
      /** Sweep stop frequency. */
      float 		spfr;
      /** Sweep trigger rate. */
      float 		srat;
      /** Sampling frequency of arbitrary waveform. */
      float 		fsmp;
      /** Output descriptor. */
      int 		odesc;  
      /** Buffer length. */
      int 		buflen;
      /** Buffer. */
      char 		iobuf[DS340_BUFLEN];
      /** mutex to protect data structure */
      mutexID_t	mux;
   };
   typedef struct _DS340_ConfigBlock _DS340_ConfigBlock;


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Globals: DS340		DS340 configuration block		*/
/*          awg_clnt		rpc client handles		 	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static _DS340_ConfigBlock 	DS340[NUM_DS340];
   static int			init = 0;


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Forward declarations: 						*/
/*	initDS340		init of DS340 configuration block	*/
/*	finiDS340		cleanup of DS340 configuration block	*/
/*      ioStrDS340		send buffer to DS340			*/
/*      byte_swap		byte swaping for DS340 waveform		*/
/*      								*/
/*----------------------------------------------------------------------*/
   __init__(initDS340driver);
#ifndef __GNUC__
#pragma init(initDS340driver)
#endif
   __fini__(finiDS340driver);
#ifndef __GNUC__
#pragma fini(finiDS340driver)
#endif
   static int initDS340 (int ID);
   static int ioStrDS340 (int ID);
   static void byte_swap (char* p);


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: byte_swap					*/
/*                                                         		*/
/* Procedure Description: byte swaping for DS340 waveform		*/
/*                                                         		*/
/* Procedure Arguments: swap argument					*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void byte_swap (char* p)
   {
      char		c;
      static short	d = 1;
   
      if (*((char*) &d) == 0) {
         c = *p;
         *p = *(p+1);
         *(p+1) = c;
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: initDS340					*/
/*                                                         		*/
/* Procedure Description: zeros entry in DS340_ ConfigBlock		*/
/*                                                         		*/
/* Procedure Arguments: index						*/
/*                                                         		*/
/* Procedure Returns: GDS_ERROR						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int initDS340 (int ID) 
   {
      int		ret;
      if (ID < 0) {
         for (ID = 0, ret = 0; ID < NUM_DS340; ID++) {
            if (initDS340 (ID) < 0) {
               ret = GDS_ERR_PRM;
            }
         }
         return ret;
      }
      else {
         if (ID < 0 || ID > NUM_DS340) {
            return GDS_ERR_PRM;
         }
         memset ((void*) (DS340 + ID), 0, sizeof (DS340_ConfigBlock));
         return GDS_ERR_NONE;
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: resetDS340					*/
/*                                                         		*/
/* Procedure Description: closes odesc and zeros DS340ConfigBlock	*/
/*                                                         		*/
/* Procedure Arguments: id						*/
/*                                                         		*/
/* Procedure Returns: GDS_ERROR						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int resetDS340 (int ID) 
   {
      int		ret;
      if (ID < 0) {
         for (ID = 0, ret = 0; ID < NUM_DS340; ID++) {
            if (resetDS340 (ID) < 0) {
               ret = GDS_ERR_PRM;
            }
         }
         return ret;
      }
      else {
         if (ID < 0 || ID > NUM_DS340) {
            return GDS_ERR_PRM;
         }
         MUTEX_GET (DS340[ID].mux);
         if (DS340[ID].odesc) {
            close (DS340[ID].odesc);
            DS340[ID].odesc = 0;
         }
         ret = initDS340 (ID);
         DS340[ID].buflen = 0;
         MUTEX_RELEASE (DS340[ID].mux);
         return ret;
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: connectSerialDS340				*/
/*                                                         		*/
/* Procedure Description: initialize communication with physical	*/
/*                         device via specified serial device 		*/
/*                                                         		*/
/* Procedure Arguments: ID and serial device name			*/
/*                                                         		*/
/* Procedure Returns: GDS_ERROR						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int connectSerialDS340 (int ID, const char* devname) 
   {
      int 		fd;
   
      if (ID < 0 || ID > NUM_DS340) {
         return GDS_ERR_PRM;
      }
   
      resetDS340 (ID);
      if (!(fd = open (devname, O_RDWR, 664))) {
         return GDS_ERR_FILE;
      }
      MUTEX_GET (DS340[ID].mux);
      DS340[ID].odesc = fd;
      MUTEX_RELEASE (DS340[ID].mux);
   
      if (pingDS340 (ID) || downloadDS340Block (ID)) {
         resetDS340 (ID);
         return GDS_ERR_UNDEF;
      } 
      else {
         MUTEX_GET (DS340[ID].mux);
         DS340[ID].status |= DS340_ALIVE;
         MUTEX_RELEASE (DS340[ID].mux);
         return GDS_ERR_NONE;
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: connectCoboxDS340				*/
/*                                                         		*/
/* Procedure Description: initialize communication with physical	*/
/*                         device via specified CoBox			*/
/*                                                         		*/
/* Procedure Arguments: ID and ethernet address and port		*/
/*                                                         		*/
/* Procedure Returns: GDS_ERROR						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int connectCoboxDS340 (int ID, const char *netaddr, int serialport) 
   {
      int 		fd;
      char		buf[1000];
   
      if (ID < 0 || ID > NUM_DS340) {
         return GDS_ERR_PRM;
      }
   
      resetDS340 (ID);
      if ((fd = openCobox (netaddr, serialport)) <= 0) {
         sprintf (buf, "connectCoboxDS340() cannot open %s %d\n",
                 netaddr, serialport); 
         gdsDebug (buf);
         return GDS_ERR_FILE;
      }
   
      MUTEX_GET (DS340[ID].mux);
      DS340[ID].odesc = fd;
      DS340[ID].status |= DS340_COBOX;
      MUTEX_RELEASE (DS340[ID].mux);
      if (pingDS340 (ID) || downloadDS340Block (ID)) {
         resetDS340 (ID);
         gdsDebug ("unable to reach DS340");
         return GDS_ERR_UNDEF;
      } 
      else {
         MUTEX_GET (DS340[ID].mux);
         DS340[ID].status |= DS340_ALIVE;
         MUTEX_RELEASE (DS340[ID].mux);
         return GDS_ERR_NONE;
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: ioStrDS340					*/
/*                                                         		*/
/* Procedure Description: write/read to/from device 			*/
/* 									*/
/* Procedure Arguments: function generator id				*/
/*                                                         		*/
/* Procedure Returns: number of bytes read (buflen)			*/
/*                                                         		*/
/* Note: parses input string to guess at expected output delay 		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int ioStrDS340 (int ID) 
   {
      fd_set 	fds;
      struct timeval 	twait;
      struct timespec 	wait;
      int 		i;
      int		n;
      int 		ct = 0;
      int 		nlct = 0;
      int 		qct = 0;
      int 		rct = 0;
      _DS340_ConfigBlock* cb;
      char*		cp;
      char*		cpe;
   
      if (ID < 0 || ID > NUM_DS340) 
         return GDS_ERR_PRM;
      cb = DS340 + ID;
      cp = cb->iobuf;
      cpe = cp + DS340_BUFLEN;
   
      if (!cb->odesc) {
         gdsDebug ("ioStrDS340(): no output descriptor");
         return GDS_ERR_PROG;
      }
   
   #ifdef OS_VXWORKS
      twait.tv_sec = 0;
      twait.tv_usec = _DS340_MICROWAIT;
      wait.tv_sec = 0;
      wait.tv_nsec = 1000 * _DS340_MICROWAIT;
   #else
      twait.tv_sec = 0;
      twait.tv_usec = _DS340_MICROWAIT;
      wait.tv_sec = 0;
      wait.tv_nsec = 1000 * _DS340_MICROWAIT;
   #endif
   
      /* flush input buffer */
      FD_ZERO (&fds);
      FD_SET (cb->odesc, &fds);
      if (select (FD_SETSIZE, &fds, NULL, NULL, &twait) > 0 &&
         FD_ISSET (cb->odesc,  &fds)) {
         /* printf ("select() = %d,  n = %d\n", i, n); */
         ct = read (cb->odesc, cp, 1024);
         /* if (ct) {
            printf ("Cleared %d bytes from input buffer:\n%s\n", ct, cp);
         } */
      }
   
      /* send only one line of commands -
         ie. everything up to the first newline or null  */
      while (*cp && *cp != '\n') {
         if (*cp == '?') ++qct;		/* count queries */
         if (++cp == cpe) {
            gdsDebug ("ioStrDS340(): output string too long");
            return GDS_ERR_PROG;
         }
      }
      *cp = '\n';
      cb->buflen = ++cp - cb->iobuf; 
      if (!cb->buflen) {
         gdsDebug ("ioStrDS340(): zero length output");
         return GDS_ERR_PROG;
      }
   
      cp = cb->iobuf;
      /* printf ("ioStrDS340(): writing %d bytes: %s", cb->buflen, cp);*/
      write (cb->odesc, cp, cb->buflen);
      memset (cp, 0, DS340_BUFLEN);
   
      /* poll for return string */
      /* printf ("Sent %d queries\n", qct); */
      for (n = 0; n < _WAIT_FOR_ANSWER * qct; ++n) {
         if (nlct) {
            break;
         }
         FD_ZERO (&fds);
         FD_SET (cb->odesc, &fds);
         if (select (FD_SETSIZE, &fds, NULL, NULL, &twait) > 0 &&
            FD_ISSET (cb->odesc,  &fds)) {
         
            ct = read (cb->odesc, cp, 1024);
         #if 0
            /* begin diagnostics */
            if (ct) {
               printf ("ioStrDS340(%d)[%d]: %d bytes: %s",
                      n, ID, ct, cp);
               if (cp[ct-1] != '\n')
                  printf (" --\n\t");
               else
                  printf ("\t");
               for (i = 0; i < ct; ++i)
                  printf ("%02x ", (int) cp[i]);
               printf ("\n");
            }
            /* end diagnostics */
         #endif
         
            /* parse return string */
            if (ct && !rct) rct = 1;
            for (i = 0; i < ct; ++i) {
               if (cp[i] == ';') ++rct;
               if (cp[i] == '\n') {
                  /* if (rct != qct) {
                     printf ("Warning - %d queries, %d replies\n", 
                            qct, rct );
                  }
                  printf ("reply string: \n\t%s", cb->iobuf); */
                  ++nlct;
               }
            }
            cp += ct;
            nanosleep (&wait, NULL);
         }
      }
      return GDS_ERR_NONE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: isDS340Alive				*/
/*                                                         		*/
/* Procedure Description: check if alive				*/
/*                                                         		*/
/* Procedure Arguments: ID						*/
/*                                                         		*/
/* Procedure Returns: true if alive					*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int isDS340Alive (int ID) 
   {
      _DS340_ConfigBlock* cb;
      int		ret;
   
      if (ID < 0 || ID > NUM_DS340) {
         return 0;
      }
      cb = DS340 + ID;
   
      MUTEX_GET (DS340[ID].mux);
      ret = ((cb->status & DS340_ALIVE) != 0);
      MUTEX_RELEASE (DS340[ID].mux);
      return ret;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: pingDS340					*/
/*                                                         		*/
/* Procedure Description: check communcation to physical device		*/
/*                                                         		*/
/* Procedure Arguments: ID						*/
/*                                                         		*/
/* Procedure Returns: GDS_ERROR						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int pingDS340 (int ID) 
   {
      _DS340_ConfigBlock* cb;
   
      if (ID < 0 || ID > NUM_DS340) {
         return GDS_ERR_PRM;
      }
      cb = DS340 + ID;
   
      MUTEX_GET (DS340[ID].mux);
      if (!cb->odesc) {
         MUTEX_RELEASE (DS340[ID].mux);
         gdsDebug ("pingDS340() - no output descriptor");
         return GDS_ERR_UNDEF;
      }
   
      if (cb->status & DS340_LOCK) {
         MUTEX_RELEASE (DS340[ID].mux);
         gdsDebug ("pingDS340() - locked");
         return GDS_ERR_UNDEF;
      }
   
      sprintf (cb->iobuf, "*IDN?"); 	
      if (ioStrDS340 (ID)) {
         MUTEX_RELEASE (DS340[ID].mux);
         gdsDebug ("pingDS340(): io error\n");
         return GDS_ERR_PROG;
      }
      if (strstr (cb->iobuf, _ID_STRING) == NULL) {
         MUTEX_RELEASE (DS340[ID].mux);
         gdsDebug ("pingDS340(): unreachable\n");
         return GDS_ERR_PROG;
      }
      /* printf ("pingDS340 - %d bytes: %s\n", cb->buflen, cb->iobuf); */
      MUTEX_RELEASE (DS340[ID].mux);
      return GDS_ERR_NONE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: setDS340					*/
/*                                                         		*/
/* Procedure Description: sets a configuration block			*/
/*                                                         		*/
/* Procedure Arguments: ID, conf. block					*/
/*                                                         		*/
/* Procedure Returns: GDS_ERROR						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int setDS340 (int ID, const DS340_ConfigBlock* conf)
   {
      _DS340_ConfigBlock* cb;
   
      if (ID < 0 || ID > NUM_DS340 || conf == NULL) {
         return GDS_ERR_PRM;
      }
      cb = DS340 + ID;
      MUTEX_GET (DS340[ID].mux);
      memcpy (cb, conf, sizeof (DS340_ConfigBlock));
      MUTEX_RELEASE (DS340[ID].mux);
      return GDS_ERR_NONE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: getDS340					*/
/*                                                         		*/
/* Procedure Description: gets a configuration block			*/
/*                                                         		*/
/* Procedure Arguments: ID, conf. block					*/
/*                                                         		*/
/* Procedure Returns: GDS_ERROR						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int getDS340 (int ID, DS340_ConfigBlock* conf) 
   {
      _DS340_ConfigBlock* cb;
   
      if (ID < 0 || ID > NUM_DS340 || conf == NULL) {
         return GDS_ERR_PRM;
      }
      cb = DS340 + ID;
      MUTEX_GET (DS340[ID].mux);
      memcpy (conf, cb, sizeof (DS340_ConfigBlock));
      MUTEX_RELEASE (DS340[ID].mux);
      return GDS_ERR_NONE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: downloadDS340Wave				*/
/*                                                         		*/
/* Procedure Description: Update local wave params from device		*/
/*                                                         		*/
/* Procedure Arguments: ID						*/
/*                                                         		*/
/* Procedure Returns: GDS_ERROR						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int downloadDS340Wave (int ID) 
   {
      _DS340_ConfigBlock* cb;
      char*		cp;
      char*		tok;
      char*		lasts;
      char		c;
   
      if (ID < 0 || ID > NUM_DS340) {
         return GDS_ERR_PRM;
      }
      cb = DS340 + ID;
      MUTEX_GET (DS340[ID].mux);
      if (!cb->odesc) {
         MUTEX_RELEASE (DS340[ID].mux);
         gdsDebug ("downloadDS340Wave(): no output");
         return GDS_ERR_MISSING;
      }
      cp = cb->iobuf;
      sprintf (cp, "FUNC?; FREQ?; OFFS?; FSMP?; AMPL?\n");
      if (ioStrDS340 (ID)) {
         MUTEX_RELEASE (DS340[ID].mux);
         return GDS_ERR_PROG;
      }
      if ((tok = strtok_r (cp, ";", &lasts)) &&
         sscanf (tok, "%d", (int*) &cb->func) &&
         (tok = strtok_r (0, ";", &lasts)) &&	
         sscanf (tok, "%f", &cb->freq) &&
         (tok = strtok_r (0, ";", &lasts)) &&
         sscanf (tok, "%f", &cb->offs) &&
         (tok = strtok_r (0, ";", &lasts)) &&
         sscanf (tok, "%f", &cb->fsmp) &&
         (tok = strtok_r (0, ";", &lasts)) &&
         (sscanf (tok, "%f%*c%c", &cb->ampl, &c) == 2)) {
      	 /* always use peak amplitude */
         if (toupper (c) == 'R') {
            cb->toggles |= DS340_ATYP;
            cb->ampl *= sqrt (2);
         }
         else {
            cb->ampl /= 2;
         }
      } 
      else {
         MUTEX_RELEASE (DS340[ID].mux);
         return GDS_ERR_PRM;
      }
      MUTEX_RELEASE (DS340[ID].mux);
      return GDS_ERR_NONE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: downloadDS340Sweep				*/
/*                                                         		*/
/* Procedure Description: Update local sweep params from device		*/
/*                                                         		*/
/* Procedure Arguments: ID						*/
/*                                                         		*/
/* Procedure Returns: GDS_ERROR						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int downloadDS340Sweep (int ID) 
   {
      _DS340_ConfigBlock* cb;
      int 		i;
      char*		cp;
      char*		tok;
      char*		lasts;
   
      if (ID < 0 || ID > NUM_DS340) 
         return GDS_ERR_PRM;
      cb = DS340 + ID;
   
      MUTEX_GET (DS340[ID].mux);
      if (!cb->odesc) {
         MUTEX_RELEASE (DS340[ID].mux);
         gdsDebug ("downloadDS340Sweep(): no output");
         return GDS_ERR_MISSING;
      }
      cp = cb->iobuf;
   
      sprintf (cp, "SWEN?; STFR?; SPFR?; SRAT?; SDIR?; STYP?; STRS?\n"); 
      if (ioStrDS340 (ID)) {
         MUTEX_RELEASE (DS340[ID].mux);
         return GDS_ERR_PROG;
      }
      if ((tok = strtok_r (cp, ";", &lasts)) &&
         sscanf (tok, "%d", &i) && i)
         cb->toggles |= DS340_SWEN;
      if ((tok = strtok_r (0, ";", &lasts)) &&
         sscanf (tok, "%f", &cb->stfr) &&
         (tok = strtok_r (0, ";", &lasts)) &&	
         sscanf (tok, "%f", &cb->spfr) &&
         (tok = strtok_r (0, ";", &lasts)) &&
         sscanf (tok, "%f", &cb->srat));
      else {
         gdsDebug ("downloadDS340Sweep() parse error");
      }
      if ((tok = strtok_r (cp, ";", &lasts)) &&
         sscanf (tok, "%d", &i) && i)
         cb->toggles |= DS340_SDIR;
      if ((tok = strtok_r (cp, ";", &lasts)) &&
         sscanf (tok, "%d", &i) && i)
         cb->toggles |= DS340_STYP;
      if ((tok = strtok_r (cp, ";", &lasts)) &&
         sscanf (tok, "%d", &i) && i)
         cb->toggles |= DS340_STRS;
      MUTEX_RELEASE (DS340[ID].mux);
      return GDS_ERR_NONE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: downloadDS340Status				*/
/*                                                         		*/
/* Procedure Description: Update local status bytes and switches	*/
/*                                                         		*/
/* Procedure Arguments: ID						*/
/*                                                         		*/
/* Procedure Returns: GDS_ERROR						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int downloadDS340Status (int ID) {
      _DS340_ConfigBlock*	cb;
      int 		i;
      char*		cp;
      char*		lasts;
      char*		tok;
   
      if (ID < 0 || ID > NUM_DS340) 
         return GDS_ERR_PRM;
      cb = DS340 + ID;
   
      MUTEX_GET (DS340[ID].mux);
      if (!cb->odesc) {
         MUTEX_RELEASE (DS340[ID].mux);
         gdsDebug ("downloadDS340Status(): no output");
         return GDS_ERR_MISSING;
      }
      cp = cb->iobuf;
   
      sprintf (cp, "*IDN?");
      ioStrDS340 (ID);
      strcpy (cb->name, cp);
   
      sprintf (cp, "INVT?; TERM?; SYNC?; FSEN?; TSRC? \n");
      if (ioStrDS340 (ID)) {
         MUTEX_RELEASE (DS340[ID].mux);
         return GDS_ERR_PROG;
      }
      if ((tok = strtok_r (cp, ";", &lasts)) &&
         sscanf (tok, "%d", &i) && i)
         cb->toggles |= DS340_INVT;
      if ((tok = strtok_r (0, ";", &lasts)) &&
         sscanf (tok, "%d", &i) && i)
         cb->toggles |= DS340_TERM;
      if ((tok = strtok_r (0, ";", &lasts)) &&
         sscanf (tok, "%d", &i) && i)
         cb->toggles |= DS340_SYNC;
      if ((tok = strtok_r (0, ";", &lasts)) &&
         sscanf (tok, "%d", &i) && i)
         cb->toggles |= DS340_FSEN;
      if ((tok = strtok_r (0, ";", &lasts)) &&
         sscanf (tok, "%d", &i) && i)
         cb->toggles |= DS340_TSRC;
   
      sprintf (cp, "*ESR?; *STB?; STAT?"); 
      if (ioStrDS340 (ID)) {
         MUTEX_RELEASE (DS340[ID].mux);
         return GDS_ERR_PROG;
      }
      if ((tok = strtok_r (cp, ";", &lasts)) &&
         sscanf (tok, "%d", &cb->es_byte) &&
         (tok = strtok_r (0, ";", &lasts)) &&	
         sscanf (tok, "%d", &cb->sps_byte) &&
         (tok = strtok_r (0, ";", &lasts)) &&	
         sscanf (tok, "%d", &cb->dds_byte));
      else {
         MUTEX_RELEASE (DS340[ID].mux);
         return GDS_ERR_PRM;
      }
      MUTEX_RELEASE (DS340[ID].mux);
      return GDS_ERR_NONE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: downloadDS340Block				*/
/*                                                         		*/
/* Procedure Description: Update device from local memory		*/
/*                                                         		*/
/* Procedure Arguments: ID						*/
/*                                                         		*/
/* Procedure Returns: GDS_ERROR						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int downloadDS340Block (int ID) {
      _DS340_ConfigBlock*	cb;
      int 		i;
      int		j;
   
      if (ID < 0 || ID > NUM_DS340) 
         return GDS_ERR_PRM;
      cb = DS340 + ID;
   
      MUTEX_GET (DS340[ID].mux);
      i = cb->odesc;
      j = cb->status;
      MUTEX_RELEASE (DS340[ID].mux);
      initDS340 (ID);
      MUTEX_GET (DS340[ID].mux);
      cb->odesc = i;
      cb->status = j;
      MUTEX_RELEASE (DS340[ID].mux);
      downloadDS340Wave (ID);
      downloadDS340Sweep (ID);
      downloadDS340Status (ID);
      return GDS_ERR_NONE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: sendResetDS340				*/
/*                                                         		*/
/* Procedure Description: send reset message to device 			*/
/*                                                         		*/
/* Procedure Arguments: ID						*/
/*                                                         		*/
/* Procedure Returns: GDS_ERROR						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int sendResetDS340 (int ID) 
   {
      int		ret;
   
      if (ID < 0) {
         for (ID = 0, ret = 0; ID < NUM_DS340; ID++) {
            if (sendResetDS340 (ID) < 0) {
               ret = GDS_ERR_PRM;
            }
         }
      }
      else {
         if (ID < 0 || ID > NUM_DS340) { 
            return GDS_ERR_PRM;
         }
         MUTEX_GET (DS340[ID].mux);
         sprintf (DS340[ID].iobuf, "*RST\n");
         ret = ioStrDS340 (ID);
         MUTEX_RELEASE (DS340[ID].mux);
      }
      return ret;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: sendClearDS340				*/
/*                                                         		*/
/* Procedure Description: send clear message to device 			*/
/*                                                         		*/
/* Procedure Arguments: ID						*/
/*                                                         		*/
/* Procedure Returns: GDS_ERROR						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int sendClearDS340 (int ID) 
   {
      int		ret;
   
      if (ID < 0) {
         for (ID = 0, ret = 0; ID < NUM_DS340; ID++) {
            if (sendResetDS340 (ID) < 0) {
               ret = GDS_ERR_PRM;
            }
         }
      }
      else {
         if (ID < 0 || ID > NUM_DS340) { 
            return GDS_ERR_PRM;
         }
         MUTEX_GET (DS340[ID].mux);
         sprintf (DS340[ID].iobuf, "*CLS\n");
         ret = ioStrDS340 (ID);
         MUTEX_RELEASE (DS340[ID].mux);
      }
      return ret;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: sendTriggerDS340				*/
/*                                                         		*/
/* Procedure Description: send trigger message to device		*/
/*                                                         		*/
/* Procedure Arguments: ID						*/
/*                                                         		*/
/* Procedure Returns: GDS_ERROR						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int sendTriggerDS340 (int ID) 
   {
      int		ret;
   
      if (ID < 0) {
         for (ID = 0, ret = 0; ID < NUM_DS340; ID++) {
            if (sendResetDS340 (ID) < 0) {
               ret = GDS_ERR_PRM;
            }
         }
      }
      else {
         if (ID < 0 || ID > NUM_DS340) { 
            return GDS_ERR_PRM;
         }
         MUTEX_GET (DS340[ID].mux);
         sprintf (DS340[ID].iobuf, "*TRG\n");
         ret = ioStrDS340 (ID);
         MUTEX_RELEASE (DS340[ID].mux);
      }
      return ret;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: uploadDS340Wave				*/
/*                                                         		*/
/* Procedure Description: configure physical device wave settings	*/
/*                                                         		*/
/* Procedure Arguments: ID						*/
/*                                                         		*/
/* Procedure Returns: GDS_ERROR						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int uploadDS340Wave (int ID) 
   {
      _DS340_ConfigBlock* cb;
      char*		cp;
      int		ret;
   
      if (ID < 0 || ID > NUM_DS340) {
         return GDS_ERR_PRM;
      }
      cb = DS340 + ID;
   
      MUTEX_GET (DS340[ID].mux);
      cp = cb->iobuf;
      if (cb->func == ds340_noise) {
         sprintf (cp, "FUNC%d; OFFS%.11g; AMPL%.11gVP\n",
                 cb->func, cb->offs, 2 * cb->ampl);
      }
      else if (cb->func == ds340_arb) {
         sprintf (cp, "FUNC%d; FSMP%.11g; AMPL%.11gVP; TSRC%d\n",
                 cb->func, cb->fsmp, 2 * cb->ampl,
                 cb->toggles & DS340_TSRC ? 5 : 0);
      }
      else {
         sprintf (cp, "FUNC%d; FREQ%.11g; OFFS%.11g; AMPL%.11gVP\n",
                 cb->func, cb->freq, cb->offs, 2 * cb->ampl);
      }
      ret = ioStrDS340 (ID);
      MUTEX_RELEASE (DS340[ID].mux);
      return ret;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: uploadDS340Sweep				*/
/*                                                         		*/
/* Procedure Description: configure physical device sweep settings	*/
/*                                                         		*/
/* Procedure Arguments: ID						*/
/*                                                         		*/
/* Procedure Returns: GDS_ERROR						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int uploadDS340Sweep (int ID) 
   {
      _DS340_ConfigBlock* cb;
      char*		cp;
      int		ret;
   
      if (ID < 0 || ID > NUM_DS340) {
         return GDS_ERR_PRM;
      }
      cb = DS340 + ID;
   
      MUTEX_GET (DS340[ID].mux);
      cp = cb->iobuf;
      /* disable first */
      if ((cb->toggles & DS340_SWEN) == 0) {
         sprintf (cp, "SWEN0; ");
      }
      else {
         sprintf (cp, "STFR%.11g; SPFR%.11g; SRAT%.11g; "
                 "STYP%d; SDIR%i; STRS%d; SWEN%d\n",
                 cb->stfr, cb->spfr, cb->srat,
                 cb->toggles & DS340_STYP ? 1 : 0,
                 cb->toggles & DS340_SDIR ? 1 : 0,
                 cb->toggles & DS340_STRS ? 1 : 0,
                 cb->toggles & DS340_SWEN ? 1 : 0);
      }
      /* printf (cp);*/
      ret = ioStrDS340 (ID);
      MUTEX_RELEASE (DS340[ID].mux);
      return ret;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: uploadDS340Status				*/
/*                                                         		*/
/* Procedure Description: configure physical device switches		*/
/*                                                         		*/
/* Procedure Arguments: ID						*/
/*                                                         		*/
/* Procedure Returns: GDS_ERROR						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int uploadDS340Status (int ID) 
   {
      _DS340_ConfigBlock* cb;
      char* 		cp;
      int		ret;
   
      if (ID < 0 || ID > NUM_DS340) {
         return GDS_ERR_PRM;
      }
      cb = DS340 + ID;
   
      MUTEX_GET (DS340[ID].mux);
      cp = cb->iobuf;
      sprintf (cp, "INVT%d; SYNC%d; FSEN%d; TERM%d; TSRC%d\n",
              cb->toggles & DS340_INVT ? 1 : 0,
              cb->toggles & DS340_SYNC ? 1 : 0,
              cb->toggles & DS340_FSEN ? 1 : 0,
              cb->toggles & DS340_TERM ? 1 : 0,
              cb->toggles & DS340_TSRC ? 5 : 0);
      ret = ioStrDS340 (ID);
      MUTEX_RELEASE (DS340[ID].mux);
      return ret;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: uploadDS340Block				*/
/*                                                         		*/
/* Procedure Description: configure physical device with local settings	*/
/*                                                         		*/
/* Procedure Arguments: ID						*/
/*                                                         		*/
/* Procedure Returns: GDS_ERROR						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int uploadDS340Block (int ID) 
   {
      _DS340_ConfigBlock* cb;
      char*		cp;
   
      if (ID < 0 || ID > NUM_DS340) {
         return GDS_ERR_PRM;
      }
      cb = DS340 + ID;
   
      MUTEX_GET (DS340[ID].mux);
      cp = cb->iobuf;
      MUTEX_RELEASE (DS340[ID].mux);
      uploadDS340Wave (ID);
      uploadDS340Sweep (ID);
      uploadDS340Status (ID);
      MUTEX_GET (DS340[ID].mux);
      MUTEX_RELEASE (DS340[ID].mux);
      return GDS_ERR_NONE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: sendWaveDS340				*/
/*                                                         		*/
/* Procedure Description: send a waveform to a DS340			*/
/*                                                         		*/
/* Procedure Arguments: 0 if successful, <0 if failed			*/
/*                                                         		*/
/* Procedure Returns: GDS_ERROR						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int sendWaveDS340 (int ID, float data[], int len)
   {
      _DS340_ConfigBlock* cb;
      char*		cp;
      char*		lasts;
      short*		wave;		/* waveform */
      float		max;		/* maximum of data */
      int		i;		/* index */
      unsigned short	check;		/* check sum */
      int		ret;		/* return value */
      char*		tok;		/* token pointer */
   
      if (ID < 0 || ID > NUM_DS340 || len < 0 ||
         !isDS340Alive (ID)) {
         return GDS_ERR_PRM;
      }
      if (len == 0) {
         return 0;
      }
      cb = DS340 + ID;
      cp = cb->iobuf;
      wave = malloc ((len + 1) * sizeof (short));
      if (wave == NULL) {
         return GDS_ERR_MEM;
      }
   
      /* calculate waveform */
      max = 0;
      for (i = 0; i < len; i++) {
         if (fabs (data[i]) > max) {
            max = data[i];
         }
      }
      if (max < 1E-9) {
         max = 1;
      }
      for (i = 0, check = 0; i < len; i++) {
         wave[i] =(short) (_ADC_RANGE * data[i] / max + 0.5);
         if (wave[i] > 2047) {
            wave[i] = 2047;
         }
         if (wave[i] < -2048) {
            wave[i] = -2048;
         }
         check += (short) wave[i];
         byte_swap ((char*) (wave + i));
      }
      wave[len] = check;
      byte_swap ((char*) (wave + len));
   
      /* upload */
      MUTEX_GET (DS340[ID].mux);
      sprintf (cp, "LDWF?%d\n", len);
      ret = ioStrDS340 (ID);
      if (ret == 0) {
         if (((tok = strtok_r (cp, ";", &lasts)) == 0) ||
            !sscanf (tok, "%d", &i) || (i != 1)) {
            ret = -1;
         }
      }
      if ((ret == 0) && (cb->odesc != 0)) {
         write (cb->odesc, (char*) wave, (len + 1) * sizeof (short));
      }
      cb->ampl = max;
   
      /* return */
      MUTEX_RELEASE (DS340[ID].mux);
      free (wave);
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: showDS340Block				*/
/*                                                         		*/
/* Procedure Description: print contents of ConfigBlock			*/
/*                                                         		*/
/* Procedure Arguments: ID, output string, max. length			*/
/*                                                         		*/
/* Procedure Returns: GDS_ERROR						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int showDS340Block (int ID, char* s, int max) 
   {
      _DS340_ConfigBlock* cb;
      char		buf[1024]; 	/* temp buffer */
      char* 		p; 		/* cursor */
      int		size;		/* written so far */
   
      if (ID < 0 || ID > NUM_DS340) {
         return GDS_ERR_PRM;
      }
      cb = DS340 + ID;
      size = 0; p = s;
   
      MUTEX_GET (DS340[ID].mux);
      sprintf (buf, "Device: %s", cb->name);
      p = strencpy (p, buf, max - size); 
      size = (size + strlen (buf) > max) ? max : size + strlen (buf);
      sprintf (buf, "WAVE func: %1d   freq: %.11g Hz   ampl: %.11g V"
              "   offs: %.11g V   fsmp: %.11g Hz\n",
              cb->func, cb->freq, cb->ampl, cb->offs, cb->fsmp);
      p = strencpy (p, buf, max - size); 
      size = (size + strlen (buf) > max) ? max : size + strlen (buf);
      sprintf (buf, "SWEEP start: %.11g Hz  stop: %.11g Hz   rate: %.11g Hz\n",
              cb->stfr, cb->spfr, cb->srat);
      p = strencpy (p, buf, max - size); 
      size = (size + strlen (buf) > max) ? max : size + strlen (buf);
      sprintf (buf, "status: 0x%04X  toggles: 0x%04X\n",
              cb->status, cb->toggles);
      p = strencpy (p, buf, max - size); 
      size = (size + strlen (buf) > max) ? max : size + strlen (buf);
      sprintf (buf, "ES: 0x%02X  SPS: 0x%02X  DDS: 0x%02X\n",
              cb->es_byte, cb->sps_byte, cb->dds_byte);
      p = strencpy (p, buf, max - size); 
      size = (size + strlen (buf) > max) ? max : size + strlen (buf);
   
      MUTEX_RELEASE (DS340[ID].mux);
      return GDS_ERR_NONE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: initDS340driver				*/
/*                                                         		*/
/* Procedure Description: initializes driver				*/
/*                                                         		*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void initDS340driver (void)
   {
      int		ID;	/* id of DS340 */
   
      if (init != 0) {
         return;
      }
      initDS340 (-1);
      for (ID = 0; ID < NUM_DS340; ID++) {
         if (MUTEX_CREATE (DS340[ID].mux) != 0) {
            return;
         };
      }
      init = 1;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: finiDS340driver				*/
/*                                                         		*/
/* Procedure Description: clean up driver				*/
/*                                                         		*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void finiDS340driver (void)
   {
      int		ID;	/* id of DS340 */
   
      resetDS340 (-1);
      for (ID = 0; ID < NUM_DS340; ID++) {
         MUTEX_DESTROY (DS340[ID].mux);
      }
      init = 0;
   }        

