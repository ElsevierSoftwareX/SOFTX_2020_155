/* frameMemRead.c                                                         */
/* LongPlayBack                                                           */
/* compile with gcc -o frameMemRead frameMemRead.c datasrv.o daqc_access.o*/
/*     -L/home/hding/Try/Lib/Grace -l grace_np                      */
/*                 -L/home/hding/Try/Lib/UTC_GPS -ltai              */
/*                 -lm -lnsl -lsocket -lpthread                           */

#ifdef __APPLE__
#include <float.h>
#endif
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include "datasrv.h"

#define CHANNUM     16
#define PLAYDATA    0
#define PLAYTREND   1
#define PLAYTREND60 60
#define XAXISUTC    0
#define XAXISGTS    1

/*#define FAST        16384 * 4*/
#define FAST        65536
#define SLOW        16

#define ZEROLOG     1.0e-20
#define XSHIFT      0.04
#define YSHIFT      0.01

struct ftm
{
   int year;
   int month;
   int day;
   int hour;
   int min;
   int sec;
};
static struct ftm fStart;
static struct ftm fStop;

/* struct DTrend defined in Th/datasrv.h 
 * struct DTrend {
 *     double  min ;
 *     double  max ;
 *     double  mean ;
 * } ;
 */

static int              debug = 0;		/* JCB - print debugging messages if true. */

static char             chName[CHANNUM][MAX_LONG_CHANNEL_NAME_LENGTH + 1];
static char             chUnit[CHANNUM][MAX_LONG_CHANNEL_NAME_LENGTH + 1];
static int              chNum[CHANNUM];
static float            slope[CHANNUM];
static float            offset[CHANNUM];
static int              chstatus[CHANNUM];
static char             timestring[128];
static char             starttime[128];
static char             firsttimestring[128];
static char             lasttimestring[128];
static short            warning;		/* for log0 case */
static int              copies;
static int              startstop;
static int              isgps;

static int              longauto;
static int              linestyle;
static double           winYMin[CHANNUM];
static double           winYMax[CHANNUM];
static int              xyType[CHANNUM];	/* 0-linear; 1-Log; 2-Ln; 3-exp */


static unsigned long    processID = 0;
static int              playMode, dcstep = 1;
static int              finished;
static int              multiple;
static int              decimation;
static int              windowNum;
static int              dmin = 0, dmax = 0, dmean = 0;
static int              xaxisFormat, xGrid, yGrid;

static char             filename[100];

static short            gColor[CHANNUM];

static size_t           sizeDouble, sizeDTrend;
static char             origDir[1024], displayIP[80];

static time_t           duration, gpstimest;


void                    *read_block ();

static void             graphout (int width, short skip);
static void             graphmulti (int width, short skip);
static void             stepread (char *infile, int decm, int minmax, int type);
static double           juliantime (char *timest, int extrasec);
static double           juliantimegps (int igps);
static void             timeinstring (char *timest, int extrasec, char *timestout);
#if 0
/* This function appears to be unused. JCB */
static int difftimestrings (char *timest1, char *timest2, int hr);
#endif


static int complexDataType[16] =
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };


static char *
complexSuffix (int t)
{
   static char *suf[5] = { " (real)", " (imaginary)", " (magnitude)", " (phase)", "" };
   if (t >= 0 && t < 16)
   {
      t = complexDataType[t];
      switch (t)
      {
         case 1:
         case 2:
         case 3:
         case 4:
           return suf[t - 1];
      }
   }
   return suf[4];
}

static int
ReadPlaylistFile (char *playlist_name)
{
   FILE *fd;
   int j;
   int read_ok = 1;
   char rdata[32];

   if ((playlist_name != (char *) NULL)
       && ((fd = fopen (playlist_name, "r")) != (FILE *) NULL))
   {
      read_ok &= (fscanf (fd, "%s", rdata) == 1);
      longauto = atoi (rdata);
      read_ok &= (fscanf (fd, "%s", rdata) == 1);
      linestyle = atoi (rdata);

      for (j = 0; j < windowNum; j++)
      {
         read_ok &= (fscanf (fd, "%s", chName[j]) == 1);
         read_ok &= (fscanf (fd, "%s", chUnit[j]) == 1);
         read_ok &= (fscanf (fd, "%s", rdata) == 1);
         chNum[j] = atoi (rdata);
         read_ok &= (fscanf (fd, "%s", rdata) == 1);
         xyType[j] = atoi (rdata);
         if (longauto == 0)
         {			/* not auto, record y settings */
            read_ok &= (fscanf (fd, "%s", rdata) == 1);
            winYMin[j] = atof (rdata);
            read_ok &= (fscanf (fd, "%s", rdata) == 1);
            winYMax[j] = atof (rdata);
         }
      }
      for (j = 0; j < windowNum; j++)
      {
         read_ok &= (fscanf (fd, "%s", rdata) == 1);
         gColor[j] = (short) atoi (rdata);
      }
      read_ok &= (fscanf (fd, "%s", rdata) == 1);
      multiple = atoi (rdata);
      read_ok &= (fscanf (fd, "%s", rdata) == 1);
      xaxisFormat = atoi (rdata);
      read_ok &= (fscanf (fd, "%s", rdata) == 1);
      decimation = atoi (rdata);
      read_ok &= (fscanf (fd, "%s", rdata) == 1);
      xGrid = atoi (rdata);
      read_ok &= (fscanf (fd, "%s", rdata) == 1);
      yGrid = atoi (rdata);
      read_ok &= (fscanf (fd, "%s", rdata) == 1);
      dmean = atoi (rdata);
      if (debug != 0)
         fprintf (stderr, "ReadPlaylistFile() - dmean = %d\n", dmean);	/* JCB */
      read_ok &= (fscanf (fd, "%s", rdata) == 1);
      dmax = atoi (rdata);
      if (debug != 0)
         fprintf (stderr, "ReadPlaylistFile() - dmax = %d\n", dmax);	/* JCB */
      read_ok &= (fscanf (fd, "%s", rdata) == 1);
      dmin = atoi (rdata);
      if (debug != 0)
         fprintf (stderr, "ReadPlaylistFile() - dmin = %d\n", dmin);	/* JCB */
      read_ok &= (fscanf (fd, "%s", rdata) == 1);
      isgps = atoi (rdata);
      if (isgps == 0)
      {
         read_ok &= (fscanf (fd, "%s", rdata) == 1);
         fStart.year = atoi (rdata);
         read_ok &= (fscanf (fd, "%s", rdata) == 1);
         fStart.month = atoi (rdata);
         read_ok &= (fscanf (fd, "%s", rdata) == 1);
         fStart.day = atoi (rdata);
         read_ok &= (fscanf (fd, "%s", rdata) == 1);
         fStart.hour = atoi (rdata);
         read_ok &= (fscanf (fd, "%s", rdata) == 1);
         fStart.min = atoi (rdata);
         read_ok &= (fscanf (fd, "%s", rdata) == 1);
         fStart.sec = atoi (rdata);
         read_ok &= (fscanf (fd, "%s", rdata) == 1);
         fStop.day = atoi (rdata);
         read_ok &= (fscanf (fd, "%s", rdata) == 1);
         fStop.hour = atoi (rdata);
         read_ok &= (fscanf (fd, "%s", rdata) == 1);
         fStop.min = atoi (rdata);
         read_ok &= (fscanf (fd, "%s", rdata) == 1);
         fStop.sec = atoi (rdata);
      }
      else
      {
         read_ok &= (fscanf (fd, "%s", rdata) == 1);
         gpstimest = (time_t) atoi (rdata);
         read_ok &= (fscanf (fd, "%s", rdata) == 1);
         duration = (time_t) atoi (rdata);
      }
      read_ok &= (fscanf (fd, "%s", rdata) == 1);
      startstop = atoi (rdata);
      if (debug != 0)
         fprintf (stderr, "calling close(), line %d\n", __LINE__);	/* JCB */
      (void) fclose (fd);
   }
   return read_ok;
}

int
main (int argc, char *argv[])
{
   int serverPort, userPort;
   char serverIP[80];
   int j;
   int sum;
   int monthdays[13] = { 31, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };


   sizeDouble = sizeof (double);
   sizeDTrend = sizeof (struct DTrend);

   strcpy (origDir, argv[4]);
   strcpy (displayIP, argv[5]);
   strcpy (filename, argv[6]);	/* name in /tmp/<pid>DC directory, longfile0 */
   windowNum = atoi (argv[8]);
   for (j = 0; j < windowNum; j++)
   {
      slope[j] = 1.0;
      offset[j] = 0.0;
   }

   finished = 0;

   /* Read the playset file which defines what to display and how. */
   if (ReadPlaylistFile (argv[3]) != 1)
   {
      fprintf (stderr, "Cannot open playlist, exiting.\n");
      return -1;
   }

   if (multiple < 0)
   {				/* multiple copies */
      copies = atoi (argv[7]);;
      multiple = 0;
      windowNum = 1;
      strcpy (chName[0], chName[copies]);
      xyType[0] = xyType[copies];
      chNum[0] = chNum[copies];
   }
   else
      copies = -1;

   if (decimation == 0)
   {
      if (debug != 0) 
         fprintf(stderr, "frameMemRead - playMode == PLAYDATA\n") ; /* JCB */
      playMode = PLAYDATA;	/* play full data */
   }
   else if (decimation == 1)
   {
      if (debug != 0) 
         fprintf(stderr, "frameMemRead - playMode == PLAYTREND\n") ; /* JCB */
      playMode = PLAYTREND;	/* play second-trend */
   }
   else
   {
      if (debug != 0) 
         fprintf(stderr, "frameMemRead - playMode == PLAYTREND60\n") ; /* JCB */
      playMode = PLAYTREND60;	/* play minute-trend */
   }
   switch (decimation)
   {
      case 10:
         dcstep = 10;
         break;
      case 100:
         dcstep = 60;
         break;
      default:
         dcstep = 1;
         break;
   }
   if (debug != 0)
      fprintf(stderr, "frameMemRead - dcstep = %d\n", dcstep) ; /* JCB */

   sum = dmin + dmax + dmean;
   if (sum == 0)
      dmean = 1;
   else if ((multiple != 0) && (sum > 1))
   {
      dmean = 1;
      dmin = 0;
      dmax = 0;
   }

   /* Convert date to gps time if needed. */
   if (isgps == 0)
   {				/* UTC */
      (void) snprintf (starttime, sizeof (starttime),
		       "%02d-%02d-%02d-%02d-%02d-%02d", fStart.year,
		       fStart.month, fStart.day, fStart.hour, fStart.min,
		       fStart.sec);
      if ((fStart.year + 2000) % 4 == 0)	/* leap year */
         monthdays[2] = 29;
      else
         monthdays[2] = 28;
      if (strcmp (starttime, "0-0-0-0-0-0") != 0 && startstop == 1)
      {
         fStart.sec -= fStop.sec;
         while (fStart.sec < 0)
         {
            fStart.min--;
            fStart.sec += 60;
         }
         fStart.min -= fStop.min;
         while (fStart.min < 0)
         {
            fStart.hour--;
            fStart.min += 60;
         }
         fStart.hour -= fStop.hour;
         while (fStart.hour < 0)
         {
            fStart.day--;
            fStart.hour += 24;
         }
         fStart.day -= fStop.day;
         while (fStart.day <= 0)
         {
            fStart.month--;
            if (fStart.month < 0)
            {
               fStart.year--;
               fStart.month += 12;
            }
            fStart.day += monthdays[fStart.month];
         }
         while (fStart.month <= 0)
         {
            fStart.year--;
            fStart.month += 12;
         }
         if (fStart.year < 0)
            fStart.year += 100;
      }
      (void) snprintf (starttime, sizeof (starttime),
		       "%02d-%02d-%02d-%02d-%02d-%02d", fStart.year,
		       fStart.month, fStart.day, fStart.hour, fStart.min,
		       fStart.sec);
      duration = (time_t) (fStop.sec + fStop.min * 60 + (fStop.hour + fStop.day * 24) * 3600);
      if (debug != 0)
         fprintf(stderr, "frameMemRead - starttime = %s, duration = %ld\n", starttime, duration) ; /* JCB */
      /* GTS starting time needed for graphing */
      gpstimest = (time_t) DataUTCtoGPS (starttime);
      fprintf(stderr, "frameMemRead - gpstimest = %ld\n", gpstimest) ; /* JCB */
   }
   else
   {				/* GPS */
      if (startstop == 1)
      {
         gpstimest -= duration;
      }
      (void) snprintf (starttime, sizeof (starttime), "%ld", (long) gpstimest);
   }

   strcpy (serverIP, argv[1]);
   serverPort = atoi (argv[2]);
   userPort = 7000;

   /* Connect to Data Server */
   fprintf (stderr, "\nConnecting to NDS Server %s (TCP port %d)\n", serverIP, serverPort);
   /* DataConnect() defined in Th/datasrv.c */
   if (DataConnect (serverIP, serverPort, userPort, read_block) != 0)
   {
      if (copies < 0)
         fprintf (stderr, "LongPlayBack: can't connect to the data server %s:%d. Make sure the server is running.\n", serverIP, serverPort);
      else
         fprintf (stderr, "LongPlayBack Ch.%d: can't connect to the data server %s:%d. Make sure the server is running.\n", chNum[0], serverIP, serverPort);
      exit (EXIT_FAILURE);
   }
   if (debug != 0)
      fprintf (stderr, "main() - Done with DataConnect()\n");	/* JCB */
   (void) fflush (stderr);	/* JCB */


   for (j = 0; j < windowNum; j++)
   {
      int cdt = 0;
      if (strlen (chName[j]) > 5)
      {
         int l = (int) strlen (chName[j]) - 4;
         if (strcmp (chName[j] + l, ".img") == 0)
         {
            cdt = 2;
            chName[j][l] = (char) 0;
         }
         if (strcmp (chName[j] + l, ".mag") == 0)
         {
            cdt = 3;
            chName[j][l] = (char) 0;
         }
         if (strcmp (chName[j] + l, ".phs") == 0)
         {
            cdt = 4;
            chName[j][l] = (char) 0;
         }
         if (strcmp (chName[j] + l - 1, ".real") == 0)
         {
            chName[j][l - 1] = (char) 0;
            cdt = 1;
         }
      }
      complexDataType[j] = cdt;
      if (debug != 0)
         fprintf (stderr, "main() - calling DataChanAdd()\n");	/* JCB */
      (void) fflush (stderr);	/* JCB */
      {
         int chans = 0;
         chans = DataChanAdd (chName[j], 0);
         if (debug != 0)
            fprintf (stderr, "main() - done with DataChanAdd(), %d chans\n", chans);	/* JCB */
         (void) fflush (stderr);	/* JCB */
      }
   }

   switch (playMode)
   {
      case PLAYDATA:
         if (strcmp (starttime, "0-0-0-0-0-0") == 0)
         {
            fprintf (stderr, "LongPlayBack Error: no starting time\n");
            exit (0);
         }
         else
         {
            if (debug != 0)
               fprintf (stderr, "main() - calling DataWrite()\n");	/* JCB */
            (void) fflush (stderr);	/* JCB */
            processID = DataWrite (starttime, (int) duration, isgps);
            fprintf (stderr, "T0=%s; Length=%d (s)\n", starttime, (int) duration);
         }
         break;
      case PLAYTREND:
         if (debug != 0)
            fprintf (stderr, "main() - 1. calling DataWriteTrend()\n");	/* JCB */
         (void) fflush (stderr);	/* JCB */
         processID = DataWriteTrend (starttime, (int) duration, 1, isgps);
         fprintf (stderr, "T0=%s; Length=%d (s)\n", starttime, (int) duration);
         break;
      case PLAYTREND60:
         if (debug != 0)
            fprintf (stderr, "main() - 2. calling DataWriteTrend()\n");	/* JCB */
         (void) fflush (stderr);	/* JCB */
         processID = DataWriteTrend (starttime, (int) duration, 60, isgps);
         fprintf (stderr, "T0=%s; Length=%d (s)\n", starttime, (int) duration);
         break;
      default:
         break;
   }

   if (((long) processID) < 0)
   {
      finished = 1;
      fprintf (stderr, "No data output.\n");
   }
   while (finished == 0)
      (void) sleep (2);

   return 0;
}

void *
read_block ()
{
   int dataRecv, irate = 0, bsec, hr = 0, firsttime = 0;
   double *chData = (double *) NULL ;
   double fvalue;
   int sRate[CHANNUM];
   long totaldata = 0, totaldata0 = 0, skip = 0, skipstatus;
   time_t ngps, ngps0 = 0;
   char tempfile[FILENAME_MAX];
   FILE *fd[16], *fdM[16], *fdm[16];
   int i, j, bc, bytercv;

   dataRecv = 0;
   /* chData[]:
    * This is a really big array, 1/2 MByte in size.  It was an automatic
    * variable, but there are limits to how big an automatic can be which
    * varies from one OS to another.  Mac OS X in particular, won't allow
    * automatics of that size.  Allocated storage can be much bigger, so
    * use malloc to get the memory.
    */
   chData = (double *) malloc (sizeof (double) * FAST);
   if (debug != 0)
      fprintf (stderr, "read_block() calling DataReadStart()\n");	/* JCB */
   (void) fflush (stderr);	/* JCB */
   DataReadStart ();

   /* Create files in which to store the data in /tmp for display. */
   for (j = 0; j < windowNum; j++)
   {
      if (playMode == PLAYDATA)
      {
         /* The file name to be created is passed as a parameter to the
          * frameMemRead program, argument 6.
          */
         (void) snprintf (tempfile, (size_t) FILENAME_MAX, "%s%d", filename, j);
         fprintf (stderr, "read_block() - playMode == PLAYDATA, creating %s\n", tempfile);	/* JCB */
         fd[j] = fopen (tempfile, "w");
         if (fd[j] == NULL)
         {
            fprintf (stderr, "Unexpected error when opening files. Exit.\n");
            DataQuit ();
            if (chData != NULL)
               free(chData) ;
            return NULL;
         }
      }
      else
      {
         if (dmax != 0)
         {
            (void) snprintf (tempfile, (size_t) FILENAME_MAX, "%s%dmax", filename, j);
            if (debug != 0)
               fprintf (stderr, "read_block() - dmax: creating %s\n", tempfile);	/* JCB */
            fdM[j] = fopen (tempfile, "w");
            if (fdM[j] == NULL)
            {
               fprintf (stderr, "Unexpected error when opening files. Exit.\n");
               DataQuit ();
               if (chData != NULL)
                  free(chData) ;
               return (void *) NULL;
            }
         }
         if (dmin != 0)
         {
            (void) snprintf (tempfile, (size_t) FILENAME_MAX, "%s%dmin", filename, j);
            if (debug != 0)
               fprintf (stderr, "read_block() - dmin: creating %s\n", tempfile);	/* JCB */
            fdm[j] = fopen (tempfile, "w");
            if (fdm[j] == NULL)
            {
               fprintf (stderr, "Unexpected error when opening files. Exit.\n");
               DataQuit ();
               if (chData != NULL)
                  free(chData) ;
               return (void *) NULL;
            }
         }
         if (dmean != 0)
         {
            (void) snprintf (tempfile, (size_t) FILENAME_MAX, "%s%dmean", filename, j);
            if (debug != 0)
               fprintf (stderr, "read_block() - dmean: creating %s\n", tempfile);	/* JCB */
            fd[j] = fopen (tempfile, "w");
            if (fd[j] == NULL)
            {
               fprintf (stderr, "Unexpected error when opening files. Exit.\n");
               DataQuit ();
               if (chData != NULL)
                  free(chData) ;
               return (void *) NULL;
            }
         }
      }
   }

   /* Enter a loop to recieve the data. */
   while (1)
   {
      if (debug != 0)
         fprintf (stderr, "read_block() - calling DataRead()\n");	/* JCB */
      if ((bytercv = DataRead ()) == -2)
      {
         if (debug != 0)
            fprintf (stderr, " read_block() - bytercv = %d, line %d\n", bytercv, __LINE__);	/* JCB */
         for (j = 0; j < windowNum; j++)
         {
            if (strcmp (chUnit[j], "no_conv.") == 0)
            {
               slope[j] = 1.0;
               offset[j] = 0.0;
            }
            else
               (void) DataGetChSlope (chName[j], &slope[j], &offset[j], &chstatus[j]);
         }
      }
      else if (bytercv < 0)
      {
         if (debug != 0)
            fprintf (stderr, " read_block() - bytercv = %d, line %d\n", bytercv, __LINE__);	/* JCB */
         if (copies < 0)
            fprintf (stderr, "LONG: DataRead = %d\n", bytercv);
         else
            fprintf (stderr, "LONG Ch.%d: DataRead = %d\n", chNum[0], bytercv);
         DataReadStop ();
         break;
      }
      else if (bytercv == 0)
      {
         break;
      }
      else
	 /* bytercv is a positive number */
      {
         if (debug != 0)
            fprintf (stderr, " read_block() - bytercv = %d, line %d\n", bytercv, __LINE__);	/* JCB */
         if (firsttime == 0)
         {
            for (j = 0; j < windowNum; j++)
            {
               if (strcmp (chUnit[j], "no_conv.") == 0)
               {
                  if (debug != 0)
                     fprintf (stderr, "  read_block() - assigning slope = 1.0, offset = 0.0\n");	/* JCB */
                  slope[j] = 1.0;
                  offset[j] = 0.0;
               }
               else
               {
                  (void) DataGetChSlope (chName[j], &slope[j], &offset[j], &chstatus[j]);
                  fprintf (stderr, "  read_block() - slope = %f, offset = %f\n", slope[j], offset[j]);
               }
            }
         }
         DataTimestamp (timestring);
         ngps = DataTimeGps ();
         if (firsttime == 0)
         {
            firsttime = 1;
            ngps0 = ngps;
            strcpy (firsttimestring, timestring);
         }
         if (ngps - ngps0 - totaldata > 0)
         {
            skip += ngps - ngps0 - totaldata;
            skipstatus = 1;
         }
         else
            skipstatus = 0;
         fprintf (stderr, "\r%s", timestring);

         if (playMode == PLAYDATA)
         {
            if (debug != 0)
               fprintf (stderr, "  read_block() - playMode == PLAYDATA\n");	/* JCB */
            bsec = DataTrendLength ();
            if (debug != 0)
               fprintf (stderr, "  read_block() - block length = %d seconds\n", bsec);	/* JCB */
            hr = 1;
            for (j = 0; j < windowNum; j++)
            {
               for (bc = 0; bc < bsec; bc++)
               {
                  /* Find data in the desired data channels */
                  sRate[j] = DataGetCh (chName[j], chData, bc, complexDataType[j]);
                  if (debug != 0)
                     fprintf (stderr, "  read_block() - sRate[%d] = %d\n", j, sRate[j]);	/* JCB */
                  switch (xyType[j])
                  {
                     case 1:	/* Log */
                        if (debug != 0)
                           fprintf (stderr, "  read_block() - xyType[%d] is Log\n", j);	/* JCB */
                        for (i = 0; i < sRate[j]; i++)
                        {
                           fvalue = chData[i];
#if 0
                           if (fvalue == 0.0)
#else
                           if (fabs (fvalue) < DBL_EPSILON)
#endif
                           {
                              fvalue = ZEROLOG;
                              warning = 1;
                           }
                           else
                              fvalue = fabs (fvalue);
                           if (debug != 0)
                              fprintf (stderr, "  read_block() - writing fvalue = %e to file\n", fvalue);	/* JCB */
                           fprintf (fd[j], "%2f\t%le\n", (double) (ngps0 + totaldata + bc) + (double) i / sRate[j], offset[j] + slope[j] * fvalue);
                        }
                        break;
                     case 2:	/* Ln */
                        if (debug != 0)
                           fprintf (stderr, "  read_block() - xyType[%d] is Ln\n", j);	/* JCB */
                        for (i = 0; i < sRate[j]; i++)
                        {
                           fvalue = chData[i];
#if 0
                           if (fvalue == 0.0)
#else
                           if (fabs (fvalue) < DBL_EPSILON)
#endif
                           {
                              fvalue = ZEROLOG;
                              warning = 1;
                           }
                           else
                              fvalue = log (fabs (fvalue));
                           if (debug != 0)
                              fprintf (stderr, "  read_block() - writing fvalue = %e to file\n", fvalue);	/* JCB */
                           fprintf (fd[j], "%2f\t%le\n", (double) (ngps0 + totaldata + bc) + (double) i / sRate[j], offset[j] + slope[j] * fvalue);
                        }
                        break;
                     default:
                        if (debug != 0)
                           fprintf (stderr, "  read_block() - xyType[%d] is default\n", j);	/* JCB */
                        for (i = 0; i < sRate[j]; i++)
                        {
                           if (debug != 0)
                              fprintf (stderr, "  read_block() - writing fvalue = %e to file, line %d\n", chData[i], __LINE__);	/* JCB */
                           fprintf (fd[j], "%2f\t%le\n", (double) (ngps0 + totaldata + bc) + (double) i / sRate[j], offset[j] + slope[j] * chData[i]);
                        }
                        break;
                  }
               }
            }
            totaldata = totaldata + bsec;
         }
         else
         {
            double *xTick = (double *) NULL ;
            struct DTrend *trendsec = (struct DTrend *) NULL;

            if (debug != 0)
               fprintf (stderr, "  read_block() - playMode != PLAYDATA\n");	/* JCB */
            irate = DataTrendLength ();
            totaldata0 = (long) (ngps - ngps0 + (long) irate);
            if (playMode == PLAYTREND60)
            {
               if (debug != 0)
                  fprintf (stderr, "  read_block() - playMode == PLAYTREND60\n");	/* JCB */
               irate = irate / 60;
            }
            if (debug != 0)
               fprintf (stderr, "  read_block() - irate = %d\n", irate);	/* JCB */
            /* Allocate storage for xTick and initialize it to 0. */
            xTick = (double *) malloc (sizeDouble * irate);
            if (xTick == (double *) NULL)
            {
               fprintf (stderr, "Failed to allocate memory, file %s line %d\n", __FILE__, __LINE__);
               exit (EXIT_FAILURE);
            }
            trendsec = (struct DTrend *) malloc (sizeDTrend * irate);
            /* Allocate storage for trendsec, initialize it to 0. */
            if (trendsec == (struct DTrend *) NULL)
            {
               fprintf (stderr, "Failed to allocate memory, file %s line %d\n", __FILE__, __LINE__);
               exit (EXIT_FAILURE);
            }
            /* Initialize the memory just allocated. */
            for (i = 0; i < irate; ++i)
            {
               xTick[i] = 0.0;
               trendsec[i].min = trendsec[i].max = trendsec[i].mean = 0.0;
            }

            if (playMode == PLAYTREND60)
               hr = 60;
            else
               hr = 1;
            if (xaxisFormat == XAXISGTS)
            {
               if (debug != 0)
                  fprintf (stderr, "  read_block() - xaxisFormat == XAXISGTS\n");	/* JCB */
               for (i = 0; i < irate; i++)
                  xTick[i] = (double) (ngps + i * hr);
            }
            else
            {			/* XAXISUTC */
               if (debug != 0)
                  fprintf (stderr, "  read_block() - xaxisFormat == XAXISUTC\n");	/* JCB */
               for (i = 0; i < irate; i++)
                  xTick[i] = juliantime (timestring, i * hr);
            }
            for (j = 0; j < windowNum; j++)
            {
               {
                  int trend_secs;
                  trend_secs = DataTrendGetCh (chName[j], trendsec);
                  if (debug != 0)
                     fprintf (stderr, "DataTrendGetCh() returned %d\n", trend_secs);	/* JCB */
               }
               if (dcstep == 1)
               {
                  switch (xyType[j])
                  {
                     case 1:	/* Log */
                        if (debug != 0)
                        fprintf (stderr, "  read_block() - xyType[%d] == Log, line %d\n", j, __LINE__);	/* JCB */
                        if (dmax != 0)
                        {
                           for (i = 0; i < irate; i++)
                           {
                              fvalue = trendsec[i].max;
#if 0
                              if (fvalue == 0.0)
#else
                              if (fabs (fvalue) < DBL_EPSILON)
#endif
                              {
                                 fvalue = ZEROLOG;
                                 warning = 1;
                              }
                              else
                                 fvalue = fabs (fvalue);
                              if (debug != 0)
                                 fprintf (stderr, "  read_block() - dmax != 0, write %e to file\n", fvalue);	/* JCB */
                              fprintf (fdM[j], "%2f\t%le\n", xTick[i],
                              offset[j] + slope[j] * fvalue);
                           }
                        }
                        if (dmean != 0)
                        {
                           for (i = 0; i < irate; i++)
                           {
                              fvalue = trendsec[i].mean;
#if 0
                              if (fvalue == 0.0)
#else
                              if (fabs (fvalue) < DBL_EPSILON)
#endif
                              {
                                 fvalue = ZEROLOG;
                                 warning = 1;
                              }
                              else
                                 fvalue = fabs (fvalue);
                              if (debug != 0)
                                 fprintf (stderr, "  read_block() - dmean != 0, write %e to file\n", fvalue);	/* JCB */
                              fprintf (fd[j], "%2f\t%le\n", xTick[i],
                              offset[j] + slope[j] * fvalue);
                           }
                        }
                        if (dmin != 0)
                        {
                           for (i = 0; i < irate; i++)
                           {
                              fvalue = trendsec[i].min;
#if 0
                              if (fvalue == 0.0)
#else
                              if (fabs (fvalue) < DBL_EPSILON)
#endif
                              {
                                 fvalue = ZEROLOG;
                                 warning = 1;
                              }
                              else
                                 fvalue = fabs (fvalue);
                              if (debug != 0)
                                 fprintf (stderr, "  read_block() - dmin != 0, write %e to file\n", fvalue);	/* JCB */
                              fprintf (fdm[j], "%2f\t%le\n", xTick[i], offset[j] + slope[j] * fvalue);
                           }
                        }
                        break;
                     case 2:	/* Ln */
                        if (debug != 0)
                           fprintf (stderr, "  read_block() - xyType[%d] == Ln, line %d\n", j, __LINE__);	/* JCB */
                        if (dmax != 0)
                        {
                           for (i = 0; i < irate; i++)
                           {
                              fvalue = trendsec[i].max;
#if 0
                              if (fvalue == 0.0)
#else
                              if (fabs (fvalue) < DBL_EPSILON)
#endif
                              {
                                 fvalue = ZEROLOG;
                                 warning = 1;
                              }
                              else
                                 fvalue = log (fabs (fvalue));
                              if (debug != 0)
                                 fprintf (stderr, "  read_block() - dmax != 0, write %e to file\n", fvalue);	/* JCB */
                              fprintf (fdM[j], "%2f\t%le\n", xTick[i], offset[j] + slope[j] * fvalue);
                           }
                        }
                        if (dmean != 0)
                        {
                           for (i = 0; i < irate; i++)
                           {
                              fvalue = trendsec[i].mean;
#if 0
                              if (fvalue == 0.0)
#else
                              if (fabs (fvalue) < DBL_EPSILON)
#endif
                              {
                                 fvalue = ZEROLOG;
                                 warning = 1;
                              }
                              else
                                 fvalue = log (fabs (fvalue));
                              if (debug != 0)
                                 fprintf (stderr, "  read_block() - dmean != 0, write %e to file\n", fvalue);	/* JCB */
                              fprintf (fd[j], "%2f\t%le\n", xTick[i], offset[j] + slope[j] * fvalue);
                           }
                        }
                        if (dmin != 0)
                        {
                           for (i = 0; i < irate; i++)
                           {
                              fvalue = trendsec[i].min;
#if 0
                              if (fvalue == 0.0)
#else
                              if (fabs (fvalue) < DBL_EPSILON)
#endif
                              {
                                 fvalue = ZEROLOG;
                                 warning = 1;
                              }
                              else
                                 fvalue = log (fabs (fvalue));
                              if (debug != 0)
                                 fprintf (stderr, "  read_block() - dmin != 0, write %e to file\n", fvalue);	/* JCB */
                              fprintf (fdm[j], "%2f\t%le\n", xTick[i], offset[j] + slope[j] * fvalue);
                           }
                        }
                        break;
                     default:
                        if (debug != 0)
                           fprintf (stderr, "  read_block() - xyType[%d] is default, line %d\n", j, __LINE__);	/* JCB */
                        if (dmax != 0)
                        {
                           for (i = 0; i < irate; i++)
                           {
                              if (debug != 0)
                                 fprintf (stderr, "  read_block() - dmax != 0, write %e to file, line %d\n", trendsec[i].max, __LINE__);	/* JCB */
                              fprintf (fdM[j], "%2f\t%le\n", xTick[i], offset[j] + slope[j] * trendsec[i].max);
                           }
                        }
                        if (dmean != 0)
                        {
                           for (i = 0; i < irate; i++)
                           {
                              if (debug != 0)
                                 fprintf (stderr, "  read_block() - dmean != 0, write %e to file, line %d\n", trendsec[i].mean, __LINE__);	/* JCB */
                              fprintf (fd[j], "%2f\t%le\n", xTick[i], offset[j] + slope[j] * trendsec[i].mean);
                           }
                        }
                        if (dmin != 0)
                        {
                           for (i = 0; i < irate; i++)
                           {
                              if (debug != 0)
                                 fprintf (stderr, "  read_block() - dmin != 0, write %e to file, line %d\n", trendsec[i].min, __LINE__);	/* JCB */
                              fprintf (fdm[j], "%2f\t%le\n", xTick[i], offset[j] + slope[j] * trendsec[i].min);
                           }
                        }
                        break;
                  }
               }
               else
               {		/* dcstep > 1 */
                  if (debug != 0)
                     fprintf (stderr, "  read_block() - dcstep > 1\n");	/* JCB */
                  if (dmax != 0)
                  {
                     if (debug != 0)
                        fprintf (stderr, "  read_block() - dmean != 0, write %e to file, line %d\n", trendsec[0].max, __LINE__);	/* JCB */
                     fprintf (fdM[j], "%ld %2f %le\n", skipstatus, xTick[0], offset[j] + slope[j] * trendsec[0].max);
                     for (i = 1; i < irate; i++)
                     {
                        if (debug != 0)
                           fprintf (stderr, "  read_block() - dmax != 0, write %e to file, line %d\n", trendsec[i].max, __LINE__);	/* JCB */
                        fprintf (fdM[j], "%d %2f %le\n", 0, xTick[i], offset[j] + slope[j] * trendsec[i].max);
                     }
                  }
                  if (dmean != 0)
                  {
                     if (debug != 0)
                        fprintf (stderr, "  read_block() - dmean != 0, write %e to file, line %d\n", trendsec[0].mean, __LINE__);	/* JCB */
                     fprintf (fd[j], "%ld %2f %le\n", skipstatus, xTick[0], offset[j] + slope[j] * trendsec[0].mean);
                     for (i = 1; i < irate; i++)
                     {
                        if (debug != 0)
                           fprintf (stderr, "  read_block() - dmean != 0, write %e to file, line %d\n", trendsec[i].mean, __LINE__);	/* JCB */
                        fprintf (fd[j], "%d %2f %le\n", 0, xTick[i], offset[j] + slope[j] * trendsec[i].mean);
                     }
                  }
                  if (dmin != 0)
                  {
                     if (debug != 0)
                        fprintf (stderr, "  read_block() - dmin != 0, write %e to file, line %d\n", trendsec[0].min, __LINE__);	/* JCB */
                     fprintf (fdm[j], "%ld %2f %le\n", skipstatus, xTick[0], offset[j] + slope[j] * trendsec[0].min);
                     for (i = 1; i < irate; i++)
                     {
                        if (debug != 0)
                           fprintf (stderr, "  read_block() - dmin != 0, write %e to file, line %d\n", trendsec[i].min, __LINE__);	/* JCB */
                        fprintf (fdm[j], "%d %2f %le\n", 0, xTick[i], offset[j] + slope[j] * trendsec[i].min);
                     }
                  }
               }
            }
            totaldata = totaldata0;
            if (xTick != NULL)
               free (xTick);
            if (trendsec != NULL)
               free (trendsec);
         }
         dataRecv = 1;
      }
   }				/* end of while loop  */

   fprintf (stderr, "\r");	/* Clear rolling timecode display */
   warning = 0;
   if (dataRecv != 0)
   {
      if (debug != 0)
         fprintf (stderr, "read_block() - dataRecv = %d, line %d\n", dataRecv, __LINE__);	/* JCB */
      /* Launch xmgr with named-pipe support */
      if (GraceOpen (1000000, displayIP, origDir, 0) == -1)
      {
         fprintf (stderr, "Can't run Grace. \n");
         exit (EXIT_FAILURE);
      }
      //fprintf ( stderr, "Done launching Grace \n" );
      switch (playMode)
      {
         case PLAYDATA:
            if (debug != 0)
               fprintf (stderr, "read_block() - playMode == PLAYDATA, line %d\n", __LINE__);	/* JCB */
            GracePrintf ("with g0");
            for (j = 0; j < windowNum; j++)
            {
               fprintf (fd[j], "&\n");
               if (debug != 0)
                  fprintf (stderr, "calling close(), line %d\n", __LINE__);	/* JCB */
               (void) fclose (fd[j]);
               (void) snprintf (tempfile, (size_t) FILENAME_MAX, "%s%d", filename, j);
               GracePrintf ("read xy \"%s\"", tempfile);
            }
            break;
         case PLAYTREND:
            if (debug != 0)
               fprintf (stderr, "read_block() - playMode == PLAYTREND, line %d\n", __LINE__);	/* JCB */
            /*@fallthrough@*/
         case PLAYTREND60:
            if (debug != 0)
               fprintf (stderr, "read_block() - playMode == PLAYTREND60, line %d\n", __LINE__);	/* JCB */
            for (j = 0; j < windowNum; j++)
            {
               if (multiple)
                  GracePrintf ("with g0");
               else
                  GracePrintf ("with g%d", j);
               if (dmax != 0)
               {
                  if (dcstep == 1)
                     fprintf (fdM[j], "&\n");
                  if (debug != 0)
                     fprintf (stderr, "calling close(), line %d\n", __LINE__);	/* JCB */
                  (void) fclose (fdM[j]);
                  (void) snprintf (tempfile, (size_t) FILENAME_MAX, "%s%dmax", filename, j);
                  if (dcstep == 1)
                     GracePrintf ("read xy \"%s\"", tempfile);
                  else
                     stepread (tempfile, dcstep, 0, xyType[j]);
               }
               if (dmean != 0)
               {
                  if (dcstep == 1)
                     fprintf (fd[j], "&\n");
                  if (debug != 0)
                  fprintf (stderr, "calling close(), line %d\n", __LINE__);	/* JCB */
                  (void) fclose (fd[j]);
                  (void) snprintf (tempfile, (size_t) FILENAME_MAX, "%s%dmean", filename, j);
                  if (dcstep == 1)
                     GracePrintf ("read xy \"%s\"", tempfile);
                  else
                     stepread (tempfile, dcstep, 1, xyType[j]);
               }
               if (dmin != 0)
               {
                  if (dcstep == 1)
                     fprintf (fdm[j], "&\n");
                  if (debug != 0)
                     fprintf (stderr, "calling close(), line %d\n", __LINE__);	/* JCB */
                  (void) fclose (fdm[j]);
                  (void) snprintf (tempfile, (size_t) FILENAME_MAX, "%s%dmin", filename, j);
                  if (dcstep == 1)
                     GracePrintf ("read xy \"%s\"", tempfile);
                  else
                     stepread (tempfile, dcstep, 2, xyType[j]);
               }
            }
            break;
         default:
            if (debug != 0)
               fprintf (stderr, "read_block() - playMode == default, line %d\n", __LINE__);	/* JCB */
            break;
      }

      if (playMode == PLAYDATA)
      {
         if (debug != 0)
            fprintf (stderr, "read_block() - playMode == PLAYDATA, line %d\n", __LINE__);	/* JCB */
         irate = 1;
      }
      else
      {
         if (debug != 0)
            fprintf (stderr, "read_block() - playMode != PLAYDATA, line %d\n", __LINE__);	/* JCB */
         if (skip > 0)
         {
            fprintf (stderr, "%ld seconds worth of data was unavailable on this server\n", skip);
         }
      }
      if (playMode == PLAYTREND60)
      {
         if (debug != 0)
            fprintf (stderr, "read_block() - playMode == PLAYTREND60, line %d\n", __LINE__);	/* JCB */
         if (copies < 0)
            fprintf (stderr, "%.1f minutes of trend displayed\n", (float) totaldata / hr);
         timeinstring (timestring, (irate - 1) * 60, lasttimestring);
      }
      else
      {
         if (debug != 0)
            fprintf (stderr, "read_block() - playMode != PLAYTREND60, line %d\n", __LINE__);	/* JCB */
         fprintf (stderr, "%ld seconds of data displayed\n\n", totaldata);
         timeinstring (timestring, irate - 1, lasttimestring);
      }
      if (debug != 0)
         fprintf (stderr, "read_block() - multiple = %d, skip = %ld at line %d\n", multiple, skip, __LINE__);	/* JCB */
      if (multiple == 0)
      {
         if (skip > 0)
            graphout ((int) (totaldata / hr), 1);
         else
            graphout ((int) (totaldata / hr), 0);
      }
      else
      {
         if (skip > 0)
            graphmulti ((int) (totaldata / hr), 1);
         else
            graphmulti ((int) (totaldata / hr), 0);
      }
   }
   else
   {
      if (debug != 0)
         fprintf (stderr, "read_block() - dataRcv == 0, line %d\n", __LINE__);	/* JCB */
      fprintf (stderr, "No data found\n\n");
      DataQuit ();
      finished = 1;
      (void) fflush (stdout);
      if (chData != NULL)
         (void) free(chData) ;
      return (void *) NULL;
   }
   if (warning != 0)
   {
      if (copies < 0)
         fprintf (stderr, "Warning: 0 as input of log, put 1.0e-20 instead\n");
      else
         fprintf (stderr, "Warning Ch.%d: 0 as input of log, put 1.0e-20 instead\n", chNum[0]);
   }

   /* Finishing */
   if (debug != 0)
      fprintf (stderr, "read_block - calling GraceFlush(), line %d\n", __LINE__);	/* JCB */
   GraceFlush ();
   //sleep(4);
   /*DataWriteStop(processID); */
   GraceClosePipe ();
   DataQuit ();
   finished = 1;
   (void) free (chData);
   (void) fflush (stdout);
   return (void *) NULL;
}



static void
graphout (int width, short skip)
{
   float x0[16], y0[16], w, h;
   double xmin, xmax;
   int i;

   if (playMode == PLAYDATA)
   {
      if (debug != 0)
         fprintf (stderr, "graphout() - playMode == PLAYDATA, line %d\n", __LINE__);	/* JCB */
      xmin = (double) gpstimest;
      xmax = xmin + (double) duration;
   }
   else if (xaxisFormat == XAXISGTS)
   {				/* GTS */
      if (debug != 0)
         fprintf (stderr, "graphout() - xaxisFormat == XAXISGTS, line %d\n", __LINE__);	/* JCB */
      xmin = (double) gpstimest;
      xmax = (double) (gpstimest + duration - 1);
   }
   else
   {				/* UTC */
      if (debug != 0)
         fprintf (stderr, "graphout() - UTC, line %d\n", __LINE__);	/* JCB */
      if (isgps == 0)
      {
         xmin = juliantime (starttime, 0);
         xmax = juliantime (starttime, (int) duration);
      }
      else
      {
         xmin = juliantimegps ((int) gpstimest);
         xmax = juliantimegps ((int) (gpstimest + duration));
      }
   }
#if 0
   if (xmax == xmin)
#else
   if (fabs (xmax - xmin) < DBL_EPSILON)
#endif
      xmax = xmin + 1;

   GracePrintf ("background color 7");
   switch (windowNum)
   {
      case 1:
         x0[0] = 0.06;
         y0[0] = 0.06;
         w = 1.18;
         h = 0.81;
         break;
      case 2:
         x0[0] = 0.06;
         y0[0] = 0.06;
         x0[1] = 0.06;
         y0[1] = 0.50;
         w = 1.18;
         h = 0.35;
         break;
      case 3:
      case 4:
         x0[0] = 0.06;
         y0[0] = 0.06;
         x0[1] = 0.06;
         y0[1] = 0.50;
         x0[2] = 0.68;
         y0[2] = 0.06;
         x0[3] = 0.68;
         y0[3] = 0.50;
         w = 0.56;
         h = 0.35;
         break;
      case 5:
      case 6:
         x0[0] = 0.06;
         y0[0] = 0.06;
         x0[1] = 0.06;
         y0[1] = 0.35;
         x0[2] = 0.06;
         y0[2] = 0.64;
         x0[3] = 0.68;
         y0[3] = 0.06;
         x0[4] = 0.68;
         y0[4] = 0.35;
         x0[5] = 0.68;
         y0[5] = 0.64;
         w = 0.56;
         h = 0.23;
         break;
      case 7:
      case 8:
      case 9:
         x0[0] = 0.06;
         y0[0] = 0.06;
         x0[1] = 0.06;
         y0[1] = 0.35;
         x0[2] = 0.06;
         y0[2] = 0.64;
         x0[3] = 0.47;
         y0[3] = 0.06;
         x0[4] = 0.47;
         y0[4] = 0.35;
         x0[5] = 0.47;
         y0[5] = 0.64;
         x0[6] = 0.88;
         y0[6] = 0.06;
         x0[7] = 0.88;
         y0[7] = 0.35;
         x0[8] = 0.88;
         y0[8] = 0.64;
         w = 0.36;
         h = 0.23;
         break;
      case 10:
      case 11:
      case 12:
         x0[0] = 0.06;
         y0[0] = 0.06;
         x0[1] = 0.06;
         y0[1] = 0.35;
         x0[2] = 0.06;
         y0[2] = 0.64;
         x0[3] = 0.37;
         y0[3] = 0.06;
         x0[4] = 0.37;
         y0[4] = 0.35;
         x0[5] = 0.37;
         y0[5] = 0.64;
         x0[6] = 0.68;
         y0[6] = 0.06;
         x0[7] = 0.68;
         y0[7] = 0.35;
         x0[8] = 0.68;
         y0[8] = 0.64;
         x0[9] = 0.98;
         y0[9] = 0.06;
         x0[10] = 0.98;
         y0[10] = 0.35;
         x0[11] = 0.98;
         y0[11] = 0.64;
         w = 0.263;
         h = 0.23;
         break;
      default:			/* case 13 - 16  */
         x0[0] = 0.06;
         y0[0] = 0.06;
         x0[1] = 0.06;
         y0[1] = 0.277;
         x0[2] = 0.06;
         y0[2] = 0.494;
         x0[3] = 0.06;
         y0[3] = 0.711;
         x0[4] = 0.37;
         y0[4] = 0.06;
         x0[5] = 0.37;
         y0[5] = 0.277;
         x0[6] = 0.37;
         y0[6] = 0.494;
         x0[7] = 0.37;
         y0[7] = 0.711;
         x0[8] = 0.68;
         y0[8] = 0.06;
         x0[9] = 0.68;
         y0[9] = 0.277;
         x0[10] = 0.68;
         y0[10] = 0.494;
         x0[11] = 0.68;
         y0[11] = 0.711;
         x0[12] = 0.98;
         y0[12] = 0.06;
         x0[13] = 0.98;
         y0[13] = 0.277;
         x0[14] = 0.98;
         y0[14] = 0.494;
         x0[15] = 0.98;
         y0[15] = 0.711;
         w = 0.263;
         h = 0.157;
         break;
   }
   for (i = 0; i < windowNum; i++)
   {
      GracePrintf ("with g%d", i);
      GracePrintf ("view %f, %f,%f,%f", x0[i] + XSHIFT, y0[i] + YSHIFT, x0[i] + w, y0[i] + h);
      GracePrintf ("frame color 1");
      GracePrintf ("frame background color 0");
      GracePrintf ("frame fill on");
      switch (xyType[i])
      {
         case 2:			/* Ln */
            GracePrintf ("yaxis ticklabel prepend \"\\-Exp\"");
            break;
         default:			/* Linear */
            GracePrintf ("yaxis ticklabel prepend \"\\-\"");
            break;
      }
      GracePrintf ("yaxis label \"%s\" ", chUnit[i]);
      GracePrintf ("yaxis label layout para");
      GracePrintf ("yaxis label place auto");
      GracePrintf ("yaxis label font 4");
      if (windowNum > 6)
         GracePrintf ("yaxis label char size 0.6");
      else
         GracePrintf ("yaxis label char size 0.8");
      if (windowNum > 9)
         GracePrintf ("subtitle size 0.5");
      else
         GracePrintf ("subtitle size 0.7");
      GracePrintf ("subtitle font 1");
      GracePrintf ("subtitle color 1");
      /*if ( chstatus[i] == 0 )
         GracePrintf( "subtitle color 1" );
         else
         GracePrintf( "subtitle color 2" ); */
      GracePrintf ("world xmin %f", xmin);
      GracePrintf ("world xmax %f", xmax);
      if (playMode == PLAYDATA)
      {
         GracePrintf ("xaxis ticklabel format decimal");
         GracePrintf ("xaxis ticklabel prec 0");
      }
      else if (xaxisFormat == XAXISGTS)
      {
         GracePrintf ("xaxis ticklabel format decimal");
         if (width <= 2)
            GracePrintf ("xaxis ticklabel prec 1");
         else
            GracePrintf ("xaxis ticklabel prec 0");
      }
      else
         GracePrintf ("xaxis ticklabel format yymmddhms");
      if (windowNum <= 6)
      {
         GracePrintf ("xaxis tick major %le", (xmax - xmin) / 4.0);
         GracePrintf ("xaxis tick minor %le", (xmax - xmin) / 8.0);
      }
      else
      {
         GracePrintf ("xaxis tick major %le", (xmax - xmin) / 2.0);
         GracePrintf ("xaxis tick minor %le", (xmax - xmin) / 4.0);
      }
      if (longauto == 0)
      {
         GracePrintf ("world ymin %le", winYMin[i]);
         GracePrintf ("world ymax %le", winYMax[i]);
         GracePrintf ("yaxis tick major %le", (winYMax[i] - winYMin[i]) / 5.0);
         GracePrintf ("yaxis tick minor %le", (winYMax[i] - winYMin[i]) / 10.0);
      }
      if (windowNum > 2)
         GracePrintf ("xaxis ticklabel char size 0.40");
      else
         GracePrintf ("xaxis ticklabel char size 0.53");
      GracePrintf ("xaxis tick color 1");
      if (xGrid != 0)
      {
         GracePrintf ("xaxis tick major color 7");
         GracePrintf ("xaxis tick major linewidth 1");
         GracePrintf ("xaxis tick major linestyle 1");
         GracePrintf ("xaxis tick minor color 7");
         GracePrintf ("xaxis tick minor linewidth 1");
         GracePrintf ("xaxis tick minor linestyle 3");
         GracePrintf ("xaxis tick major grid on");
         GracePrintf ("xaxis tick minor grid on");
      }
      if (windowNum > 5)
         GracePrintf ("yaxis ticklabel char size 0.6");
      else if (windowNum > 2)
         GracePrintf ("yaxis ticklabel char size 0.7");
      else
         GracePrintf ("yaxis ticklabel char size 0.8");
      GracePrintf ("yaxis tick color 1");
      if (yGrid != 0)
      {
         GracePrintf ("yaxis tick major color 7");
         GracePrintf ("yaxis tick major linewidth 1");
         GracePrintf ("yaxis tick major linestyle 1");
         GracePrintf ("yaxis tick minor color 7");
         GracePrintf ("yaxis tick minor linewidth 1");
         GracePrintf ("yaxis tick minor linestyle 3");
         GracePrintf ("yaxis tick major grid on");
         GracePrintf ("yaxis tick minor grid on");
      }
      if (xyType[i] == 3)
         GracePrintf ("yaxis ticklabel format exponential");
      else if (xyType[i] == 1)
      {
         GracePrintf ("g%d type logy", i);
         GracePrintf ("yaxis ticklabel prec 5");
         GracePrintf ("yaxis ticklabel format exponential");
      }
      else
         GracePrintf ("yaxis ticklabel format general");
   }

   if (playMode == PLAYDATA)
   {
      GracePrintf ("with g0");
      for (i = 0; i < windowNum; i++)
      {
         GracePrintf ("s%d color %d", i, gColor[i]);
      }
      for (i = 1; i < windowNum; i++)
      {
         GracePrintf ("move g0.s%d to g%d.s0", i, i);
      }
      for (i = 0; i < windowNum; i++)
      {
         GracePrintf ("g%d fixedpoint off", i);
         GracePrintf ("g%d fixedpoint type 0", i);
         GracePrintf ("g%d fixedpoint xy 0.000000, 0.000000", i);
         GracePrintf ("g%d fixedpoint format decimal decimal", i);
         GracePrintf ("g%d fixedpoint prec 0, 6", i);
      }
   }
   else
   {
      for (i = 0; i < windowNum; i++)
      {
         GracePrintf ("with g%d", i);
         GracePrintf ("s0 color 2");
         GracePrintf ("s1 color 6");
         GracePrintf ("s2 color 15");

         if (xaxisFormat == XAXISUTC)
         {
            GracePrintf ("g%d fixedpoint off", i);
            GracePrintf ("g%d fixedpoint type 0", i);
            GracePrintf ("g%d fixedpoint xy 0.000000, 0.000000", i);
            GracePrintf ("g%d fixedpoint format mmddyyhms decimal", i);
            GracePrintf ("g%d fixedpoint prec 6, 6", i);
         }
         else if (xaxisFormat == XAXISGTS)
         {
            GracePrintf ("g%d fixedpoint off", i);
            GracePrintf ("g%d fixedpoint type 0", i);
            GracePrintf ("g%d fixedpoint xy 0.000000, 0.000000", i);
            GracePrintf ("g%d fixedpoint format decimal decimal", i);
            GracePrintf ("g%d fixedpoint prec 0, 6", i);
         }
         if (linestyle != 1)
         {
            if ((linestyle == 2) || (skip != 0))
            {
               GracePrintf ("s0 linestyle 0");
               GracePrintf ("s1 linestyle 0");
               GracePrintf ("s2 linestyle 0");
               GracePrintf ("s0 symbol 2");
               GracePrintf ("s1 symbol 2");
               GracePrintf ("s2 symbol 2");
               GracePrintf ("s0 symbol size 0.2");
               GracePrintf ("s1 symbol size 0.2");
               GracePrintf ("s2 symbol size 0.2");
               GracePrintf ("s0 symbol fill 1");
               GracePrintf ("s1 symbol fill 1");
               GracePrintf ("s2 symbol fill 1");
            }
         }
      }
   }

   for (i = 0; i < windowNum; i++)
   {
      GracePrintf ("with g%d", i);
      GracePrintf ("g%d on", i);
      if (playMode == PLAYDATA)
         GracePrintf ("subtitle \"Ch %d: %s%s\"", chNum[i], chName[i], complexSuffix (i));
      else
         GracePrintf ("subtitle \"Ch %d: %s\"", chNum[i], chName[i]);
      if (longauto != 0)
         GracePrintf ("autoscale yaxes");
      GracePrintf ("clear stack");
   }
	/** title string **/
   GracePrintf ("with string 0");
   GracePrintf ("string loctype view");
   GracePrintf ("string 0.2, 0.94");
   GracePrintf ("string font 4");
   GracePrintf ("string char size 0.9");
   GracePrintf ("string color 1");
   if (playMode == PLAYDATA)
      GracePrintf ("string def \"\\1 Data Display start at %s (%d seconds)\"", firsttimestring, width);
   else if (xaxisFormat == XAXISUTC)
      GracePrintf ("string def \"\\1 Trend from %s to %s \"", firsttimestring, lasttimestring);
   else
   {
      if (playMode == PLAYTREND)
         GracePrintf ("string def \"\\1 Trend %d seconds from %s to %s\"", width, firsttimestring, lasttimestring);
      else if (playMode == PLAYTREND60)
         GracePrintf ("string def \"\\1 Trend %d minutes from %s to %s\"", width, firsttimestring, lasttimestring);
   }
   GracePrintf ("string on");
   if (playMode != PLAYDATA)
   {
      if (dmax != 0)
      {
         GracePrintf ("legend string 0 \"M A X\"");
         if (dmean != 0)
         {
            GracePrintf ("legend string 1 \"M E A N\"");
            GracePrintf ("legend string 2 \"M I N\"");
         }
         else
            GracePrintf ("legend string 1 \"M I N\"");
      }
      else if (dmean != 0)
      {
         GracePrintf ("legend string 0 \"M E A N\"");
         GracePrintf ("legend string 1 \"M I N\"");
      }
      else
         GracePrintf ("legend string 0 \"M I N\"");
      GracePrintf ("legend loctype view");
      GracePrintf ("legend 0.022, 0.97");
      GracePrintf ("legend color 4");
      GracePrintf ("legend char size 0.5");
      GracePrintf ("legend on");
   }
   GracePrintf ("clear stack");
   /*if ( windowNum > 1 )
      GracePrintf( "with g1" ); */
   GracePrintf ("with g0");
   GracePrintf ("redraw");
   (void) fflush (stdout);
}


static void
graphmulti (int width, short skip)
{
   double xmin, xmax;
   int i;
   char tempstr[100];

   if (playMode == PLAYDATA)
   {
      if (debug != 0)
         fprintf (stderr, "graphmulti() - playMode == PLAYDATA\n");	/* JCB */
      xmin = 0.0;
      xmax = (float) width;
   }
   else if (xaxisFormat == XAXISGTS)
   {
      if (debug != 0)
         fprintf (stderr, "graphmulti() - xaxisFormat == XAXISGTS\n");	/* JCB */
      xmin = 0.0;
      xmax = (float) width - 1;
   }
   else
   {
      if (debug != 0)
         fprintf (stderr, "graphmulti() - not playMode or xaxisFormat, default line %d\n", __LINE__);	/* JCB */
      xmin = juliantime (firsttimestring, 0);
      xmax = juliantime (lasttimestring, 0);
   }
#if 0
   if (xmax == xmin)
#else
   if (fabs (xmax - xmin) < DBL_EPSILON)
#endif
      xmax = xmin + 1;
   //GracePrintf( "doublebuffer true" );
   GracePrintf ("background color 7");

   GracePrintf ("with g0");	/* main graph */
   GracePrintf ("view %f,%f,%f,%f", 0.10, 0.10, 0.74, 0.85);
   GracePrintf ("frame color 1");
   GracePrintf ("frame background color 0");
   GracePrintf ("frame fill on");
   GracePrintf ("xaxis tick color 1");
   GracePrintf ("yaxis tick color 1");
   if (xGrid != 0)
   {
      if (debug != 0)
         fprintf (stderr, "graphmulti() - xGrid != 0, line %d\n", __LINE__);	/* JCB */
      GracePrintf ("xaxis tick major color 7");
      GracePrintf ("xaxis tick major linewidth 1");
      GracePrintf ("xaxis tick major linestyle 1");
      GracePrintf ("xaxis tick minor color 7");
      GracePrintf ("xaxis tick minor linewidth 1");
      GracePrintf ("xaxis tick minor linestyle 3");
      GracePrintf ("xaxis tick major grid on");
      GracePrintf ("xaxis tick minor grid on");
   }
   else if (debug != 0)
      fprintf (stderr, "graphmulti() - xGrid == 0, line %d\n", __LINE__);	/* JCB */
   if (yGrid != 0)
   {
      if (debug != 0)
         fprintf (stderr, "graphmulti() - yGrid != 0, line %d\n", __LINE__);	/* JCB */
      GracePrintf ("yaxis tick major color 7");
      GracePrintf ("yaxis tick major linewidth 1");
      GracePrintf ("yaxis tick major linestyle 1");
      GracePrintf ("yaxis tick minor color 7");
      GracePrintf ("yaxis tick minor linewidth 1");
      GracePrintf ("yaxis tick minor linestyle 3");
      GracePrintf ("yaxis tick major grid on");
      GracePrintf ("yaxis tick minor grid on");
   }
   else if (debug != 0)
      fprintf (stderr, "graphmulti() - yGrid == 0, line %d\n", __LINE__);	/* JCB */
   GracePrintf ("subtitle size 1");
   GracePrintf ("subtitle font 1");
   GracePrintf ("subtitle color 1");
   GracePrintf ("world xmin %f", xmin);
   GracePrintf ("world xmax %f", xmax);
   if (windowNum <= 6)
   {
      if (debug != 0)
         fprintf (stderr, "graphmulti() - windowNum <= 6, line %d\n", __LINE__);	/* JCB */
      GracePrintf ("xaxis tick major %le", (xmax - xmin) / 4.0);
      GracePrintf ("xaxis tick minor %le", (xmax - xmin) / 8.0);
   }
   else
   {
      if (debug != 0)
         fprintf (stderr, "graphmulti() - windowNum > 6, line %d\n", __LINE__);	/* JCB */
      GracePrintf ("xaxis tick major %le", (xmax - xmin) / 2.0);
      GracePrintf ("xaxis tick minor %le", (xmax - xmin) / 4.0);
   }
   GracePrintf ("xaxis ticklabel char size 0.53");
   if (playMode == PLAYDATA)
      GracePrintf ("xaxis ticklabel prec 2");
   else if (xaxisFormat == XAXISGTS)
   {
      GracePrintf ("xaxis ticklabel format decimal");
      if (width <= 2)
         GracePrintf ("xaxis ticklabel prec 1");
      else
         GracePrintf ("xaxis ticklabel prec 0");
   }
   else
      GracePrintf ("xaxis ticklabel format yymmddhms");
   GracePrintf ("yaxis ticklabel char size 0.43");
   GracePrintf ("yaxis ticklabel format general");

   GracePrintf ("with g0");
   /*GracePrintf( "g0 on" ); */
   for (i = 0; i < windowNum; i++)
   {
      GracePrintf ("s%d color %d", i, gColor[i]);
      GracePrintf ("legend string %d \"Ch %d:%s%s\"", i, chNum[i], chName[i], complexSuffix (i));
      if (xyType[i] == 3)
         GracePrintf ("yaxis ticklabel format exponential");
      /* if one channel is in Exp then show exp scale to all */
      else if (xyType[i] == 1)
      {
         GracePrintf ("g%d type logy", i);
         GracePrintf ("yaxis ticklabel format exponential");
      }
   }

   if (linestyle != 1)
   {
      if ((linestyle == 2) || (skip != 0))
      {
         for (i = 0; i < 9; i++)
         {
            GracePrintf ("s%d linestyle 0", i);
            GracePrintf ("s%d symbol 2", i);
            GracePrintf ("s%d symbol size 0.2", i);
            GracePrintf ("s%d symbol fill 1", i);
         }
      }
   }
   GracePrintf ("s10 linestyle 3");
   GracePrintf ("s11 linestyle 3");
   GracePrintf ("s12 linestyle 3");
   GracePrintf ("s13 linestyle 3");
   GracePrintf ("s14 linestyle 3");
   GracePrintf ("s15 linestyle 3");
   if (playMode == PLAYDATA)
      GracePrintf ("subtitle \"Display Multiple Data start at %s (%d seconds)\"", firsttimestring, width);
   else
   {
      if (dmin != 0)
         strcpy (tempstr, "Display Multiple MIN Trend");
      else if (dmax != 0)
         strcpy (tempstr, "Display Multiple MAX Trend");
      else
         strcpy (tempstr, "Display Multiple MEAN Trend");
      if (xaxisFormat == XAXISUTC)
      {
         GracePrintf ("subtitle \"%s from %s to %s\"", tempstr, firsttimestring, lasttimestring);
      }
      else
      {				/* XAXISGTS */
         if (playMode == PLAYTREND)
            GracePrintf ("subtitle \"%s %d seconds from %s to %s\"", tempstr, width, firsttimestring, lasttimestring);
         else if (playMode == PLAYTREND60)
            GracePrintf ("subtitle \"%s %d minutes from %s to %s\"", tempstr, width, firsttimestring, lasttimestring);
      }
   }
   GracePrintf ("legend loctype view");
   GracePrintf ("legend 0.75, 0.82");
   GracePrintf ("legend char size 0.7");
   GracePrintf ("legend color 1");
   GracePrintf ("legend on");
   GracePrintf ("autoscale yaxes");
   /*GracePrintf( "clear stack" ); */

   (void) fflush (stdout);
}


/* convert UTC time string to julian time */
static double
juliantime (char *timest, int extrasec)
{
   int yy, mm, dd, hh, mn, ss;
   double julian, hour;


   (void) sscanf (timest, "%02d-%02d-%02d-%02d-%02d-%02d", &yy, &mm, &dd, &hh, &mn, &ss);
   if (yy == 98 || yy == 99)
      yy += 1900;
   else
      yy += 2000;
   ss += extrasec;
   hour = hh + mn / 60.0 + ss / 3600.0;
   julian = (double) ((1461 * (yy + 4800 + (mm - 14) / 12)) / 4 +
		      (367 * (mm - 2 - 12 * ((mm - 14) / 12))) / 12 -
		      (3 * ((yy + 4900 + (mm - 14) / 12) / 100)) / 4 + dd -
		      32075);
   julian = julian + hour / 24.0 - 0.5;

   return julian;
}


/* convert gps time to julian time */
static double
juliantimegps (int igps)
{
   int yy, mm, dd, hh, mn, ss;
   double julian, hour;
   char timest[24];

   DataGPStoUTC (igps, timest);
   (void) sscanf (timest, "%02d-%02d-%02d-%02d-%02d-%02d", &yy, &mm, &dd, &hh, &mn, &ss);
   if (yy == 98 || yy == 99)
      yy += 1900;
   else
      yy += 2000;
   hour = hh + mn / 60.0 + ss / 3600.0;
   julian = (double) ((1461 * (yy + 4800 + (mm - 14) / 12)) / 4 +
		      (367 * (mm - 2 - 12 * ((mm - 14) / 12))) / 12 -
		      (3 * ((yy + 4900 + (mm - 14) / 12) / 100)) / 4 + dd -
		      32075);
   julian = julian + hour / 24.0 - 0.5;

   return julian;
}


/* give time string after adding some extra seconds */
static void
timeinstring (char *timest, int extrasec, char *timestout)
{
   int yy, mm, dd, hh, mn, ss;
   int monthdays[13] = { 31, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

   (void) sscanf (timest, "%02d-%02d-%02d-%02d-%02d-%02d", &yy, &mm, &dd, &hh, &mn, &ss);
   if ((yy + 2000) % 4 == 0)	/* leap year */
      monthdays[2] = 29;
   else
      monthdays[2] = 28;
   ss += extrasec;

   while (ss >= 60)
   {
      mn++;
      ss -= 60;
   }
   while (mn >= 60)
   {
      hh++;
      mn -= 60;
   }
   while (hh >= 24)
   {
      dd++;
      hh -= 24;
   }
   while (dd > monthdays[mm])
   {
      dd -= monthdays[mm];
      mm++;
      if (mm > 12)
      {
         mm -= 12;
         yy++;
      }
   }
   (void) sprintf (timestout, "%02d-%02d-%02d-%02d-%02d-%02d", yy, mm, dd, hh, mn, ss);
   return;
}

#if 0
/* This function appears to be unused.  JCB */
/* difference of two time strings. assume timest1>timest2 
   hr=1 in seconds;  hr=60 in minutes                         
*/
static int
difftimestrings (char *timest1, char *timest2, int hr)
{
   int yy1, mm1, dd1, hh1, mn1, ss1, yy2, mm2, dd2, hh2, mn2, ss2;
   int monthdays1[13] =
      { 31, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
   int monthdays2[13] =
      { 31, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
   int totalsec, tmp, tot1, tot2, ii;

   if (strcmp (timest1, timest2) == 0)
      return 0;
   (void) sscanf (timest1, "%02d-%02d-%02d-%02d-%02d-%02d", &yy1, &mm1, &dd1,
		  &hh1, &mn1, &ss1);
   (void) sscanf (timest2, "%02d-%02d-%02d-%02d-%02d-%02d", &yy2, &mm2, &dd2,
		  &hh2, &mn2, &ss2);
   /* leap year */
   if ((yy1 + 2000) % 4 == 0)
      monthdays1[2] = 29;
   else
      monthdays1[2] = 28;
   if ((yy2 + 2000) % 4 == 0)
      monthdays2[2] = 29;
   else
      monthdays2[2] = 28;
   tmp = ss1 - ss2;
   if (tmp < 0)
   {
      tmp = tmp + 60;
      mn1--;
   }
   totalsec = tmp;
   tmp = mn1 - mn2;
   if (tmp < 0)
   {
      tmp = tmp + 60;
      hh1--;
   }
   totalsec = totalsec + tmp * 60;
   tmp = hh1 - hh2;
   if (tmp < 0)
   {
      tmp = tmp + 24;
      dd1--;
   }
   totalsec = totalsec + tmp * 3600;

   /* total days beyond 2000 */
   tot1 = yy1 * 365;
   /* add one day for each leap year */
   if (yy1 == 1)
      tot1 += 1;
   else if (yy1 > 1)
      tot1 += (yy1 - 1) / 4 + 1;
   for (ii = 0; ii < mm1 - 1; ii++)
   {
      tot1 += monthdays1[ii];
   }
   tot1 += dd1;
   tot2 = yy2 * 365;
   if (yy2 == 1)
      tot2 += 1;
   else if (yy2 > 1)
      tot2 += (yy2 - 1) / 4 + 1;
   for (ii = 0; ii < mm2 - 1; ii++)
   {
      tot2 += monthdays2[ii];
   }
   tot2 += dd2;

   totalsec = (tot1 - tot2) * 24 * 3600 + totalsec;
   if (hr == 60)
      totalsec = totalsec / 60;
   return totalsec;
}
#endif


/* read data file into xmgr with a decimation 
   minmax = 0 (max), 1 (mean), 2 (min)
*/
static void
stepread (char *infile, int decm, int minmax, int type)
{
   /* Note that FILENAME_MAX is defined in stdio.h
    * as the max length for a filename.
    */
   char outfile[FILENAME_MAX], line[80];
   FILE *fpw, *fpr;
   double x = 0.0, y = 0.0, yy[61];
   int i, count, status;

   (void) snprintf (outfile, (size_t) FILENAME_MAX, "%sd", infile);
   fpr = fopen (infile, "r");
   fpw = fopen (outfile, "w");
   if (fpw == NULL)
   {
      printf ("Error in opening writing file %s. Exit.\n", outfile);
      exit (EXIT_FAILURE);
   }
   if (fpr == NULL)
   {
      printf ("Error in opening reading file %s. Exit.\n", infile);
      exit (EXIT_FAILURE);
   }

   count = 1;
   while (fgets (line, 80, fpr) != NULL)
   {
      (void) sscanf (line, "%d %lf %le\n", &status, &x, &yy[count]);
      if (count % decm == 0 || status == 1)
      {
         if (minmax == 1)
         {			/* mean */
            y = 0;
            for (i = 1; i <= count; i++)
            {
               y = y + yy[i];
            }
            y = y / count;
            count = 1;
         }
         else if (minmax == 0)
         {			/* max */
            y = yy[1];
            for (i = 1; i <= count; i++)
            {
               if (y < yy[i])
                  y = yy[i];
            }
            count = 1;
         }
         else if (minmax == 2)
         {			/* min */
            y = yy[1];
            for (i = 1; i <= count; i++)
            {
               if (y > yy[i])
                  y = yy[i];
            }
            count = 1;
         }
         switch (type)
         {
            case 1:		/* Log */
#if 0
              if (y == 0.0)
#else
              if (fabs (y) < DBL_EPSILON)
#endif
              {
                 y = ZEROLOG;
                 warning = 1;
              }
              else
                 y = fabs (y);
              break;
           case 2:		/* Ln */
#if 0
              if (y == 0.0)
#else
              if (fabs (y) < DBL_EPSILON)
#endif
              {
                 y = ZEROLOG;
                 warning = 1;
              }
              else
                 y = log (fabs (y));
              break;
           default:
              break;
         }
         fprintf (fpw, "%2f\t%le\n", x, y);
      }
      else
      {
         count++;
      }
   }

   fprintf (fpw, "&\n");
   if (debug != 0)
      fprintf (stderr, "calling close(), line %d\n", __LINE__);	/* JCB */
   (void) fclose (fpw);
   if (debug != 0)
      fprintf (stderr, "calling close(), line %d\n", __LINE__);	/* JCB */
   (void) fclose (fpr);
   GracePrintf ("read xy \"%s\"", outfile);
   (void) remove (infile);
   return;
}
