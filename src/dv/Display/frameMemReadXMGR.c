/* frameMemRead.c                                                            */
/* LongPlayBack                                                           */
/* compile with gcc -o frameMemRead frameMemRead.c datasrv.o daqc_access.o*/
/*     -L/home/hding/Cds/Frame/Lib/Xmgr -lacegr_np_my                     */
/*                 -L/home/hding/Cds/Frame/Lib/UTC_GPS -ltai              */
/*                 -lm -lnsl -lsocket -lpthread                           */

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
#define XAXISUTC   0
#define XAXISGTS  1

#define FAST        16384
#define SLOW        16

#define ZEROLOG         1.0e-20
#define XSHIFT          0.02
#define YSHIFT          0.01

static int debug = 0; /* JCB */

struct ftm {
        int year;
        int month;
        int day;
        int hour;
        int min;
        int sec;
};
struct ftm fStart, fStop;
struct DTrend  *trendsec; 
double *xTick;

char  chName[CHANNUM][80], chUnit[CHANNUM][80];;
int   chNum[CHANNUM];
float slope[CHANNUM], offset[CHANNUM];
int   chstatus[CHANNUM];
char  timestring[24];
char  starttime[24], firsttimestring[24], lasttimestring[24];
short warning; /* for log0 case */
int   copies, startstop, isgps;

int   longauto, linestyle;
double winYMin[CHANNUM], winYMax[CHANNUM]; 
int   xyType[CHANNUM];  /* 0-linear; 1-Log; 2-Ln; 3-exp */


unsigned long processID=0;
short   playMode, dcstep=1;
short   finished, multiple, decimation, windowNum;
short   dmin=0, dmax=0, dmean=0;
short   xaxisFormat, xGrid, yGrid;

char    filename[100];

short   gColor[CHANNUM];
int     monthdays[13] = {31,31,28,31,30,31,30,31,31,30,31,30,31};

size_t  sizeDouble, sizeDTrend;
char    origDir[1024], displayIP[80];

time_t  duration, gpstimest;

void*   read_block();
void    graphout(int width, short skip);
void    graphmulti(int width, short skip);
void    stepread(char* infile, int decm, int minmax, int type);
double  juliantime(char* timest, int extrasec);
double  juliantimegps(int igps);
void    timeinstring(char* timest, int extrasec, char* timestout );
int     difftimestrings(char* timest1, char* timest2, int hr);

int     ACEgrPrintf (const char* fmt, ...);



int main(int argc, char *argv[])
{
int     serverPort, userPort;
char    serverIP[80];

int     j, sum; 
char    rdata[32];

FILE   *fd;

	if (debug) fprintf(stderr, "frameMemReadXMGR - main()\n") ;

	sizeDouble = sizeof(double);
	sizeDTrend = sizeof(struct DTrend);

        strcpy(origDir, argv[4]);
        strcpy(displayIP, argv[5]);
        strcpy(filename, argv[6]);
	windowNum = atoi(argv[8]);
	for ( j=0; j<windowNum; j++ ) {
	   slope[j] = 1.0;
	   offset[j] = 0.0;
	}

        finished = 0;
	fd = fopen( argv[3], "r" );
	fscanf ( fd,"%s", rdata );
	longauto = atoi(rdata);
	fscanf ( fd,"%s", rdata );
	linestyle = atoi(rdata);
	for ( j=0; j<windowNum; j++ ) {
	   fscanf ( fd,"%s", chName[j] );
	   fscanf ( fd,"%s", chUnit[j] );
	   fscanf ( fd,"%s", rdata );
	   chNum[j] = atoi(rdata);
	   fscanf ( fd,"%s", rdata );
	   xyType[j] = atoi(rdata);
	   if ( longauto == 0 ) { /* not auto, record y settings */
	     fscanf ( fd,"%s", rdata );
	     winYMin[j] = atof(rdata);
	     fscanf ( fd,"%s", rdata );
	     winYMax[j] = atof(rdata);
	   }
	}
	for ( j=0; j<windowNum; j++ ) {
	  fscanf( fd, "%s", rdata );
	  gColor[j] = atoi(rdata);
	}
	fscanf( fd, "%s", rdata );
	multiple = atoi(rdata);
	fscanf( fd, "%s", rdata );
	xaxisFormat = atoi(rdata);
	fscanf( fd, "%s", rdata );
	decimation = atoi(rdata);
	fscanf( fd, "%s", rdata );
	xGrid = atoi(rdata);
	fscanf( fd, "%s", rdata );
	yGrid = atoi(rdata);
	fscanf( fd, "%s", rdata );
	dmean = atoi(rdata);
	fscanf( fd, "%s", rdata );
	dmax = atoi(rdata);
	fscanf( fd, "%s", rdata );
	dmin = atoi(rdata);
	fscanf ( fd, "%s", rdata );
	isgps = atoi(rdata);
	if ( isgps == 0 ) {
	  fscanf ( fd, "%s", rdata );
	  fStart.year = atoi(rdata);
	  fscanf ( fd, "%s", rdata );
	  fStart.month = atoi(rdata);
	  fscanf ( fd, "%s", rdata );
	  fStart.day = atoi(rdata);
	  fscanf ( fd, "%s", rdata );
	  fStart.hour = atoi(rdata);
	  fscanf ( fd, "%s", rdata );
	  fStart.min = atoi(rdata);
	  fscanf ( fd, "%s", rdata );
	  fStart.sec = atoi(rdata);
	  fscanf ( fd, "%s", rdata );
	  fStop.day = atoi(rdata);
	  fscanf ( fd, "%s", rdata );
	  fStop.hour = atoi(rdata);
	  fscanf ( fd, "%s", rdata );
	  fStop.min = atoi(rdata);
	  fscanf ( fd, "%s", rdata);
	  fStop.sec = atoi(rdata);
        }
        else {
	  fscanf ( fd,"%s", rdata );
	  gpstimest = atoi(rdata);
	  fscanf ( fd, "%s", rdata);
	  duration = atoi(rdata);
        }
	fscanf ( fd, "%s", rdata);
	startstop = atoi(rdata);
	fclose(fd);

	if ( multiple < 0 ) { /* multiple copies */
	   copies = atoi(argv[7]);;
	   multiple = 0;
	   windowNum = 1;
	   strcpy(chName[0], chName[copies]);
	   xyType[0] = xyType[copies];
	   chNum[0] = chNum[copies];
	}
	else
	  copies = -1;

	if ( decimation == 0 )
	   playMode = PLAYDATA;  /* play full data */
	else if ( decimation == 1 )
	   playMode = PLAYTREND;  /* play second-trend */
	else
	   playMode = PLAYTREND60;  /* play minute-trend */
        switch ( decimation ) {
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

	sum = dmin + dmax + dmean;
	if ( sum == 0 )
	  dmean = 1;
	else if ( multiple && sum > 1 ) {
	  dmean = 1; dmin = 0; dmax = 0;
	}

	if ( isgps == 0 ) { /* UTC */
	  sprintf ( starttime, "%d-%d-%d-%d-%d-%d", fStart.year,fStart.month,fStart.day,fStart.hour,fStart.min,fStart.sec );
	  if ((fStart.year+2000) % 4 == 0) /* leap year */
	    monthdays[2] = 29;
	  else
	    monthdays[2] = 28;
	  if ( strcmp(starttime, "0-0-0-0-0-0") != 0 && startstop == 1) {
	    fStart.sec -= fStop.sec;
	    while ( fStart.sec < 0 ) {
	      fStart.min --;
	      fStart.sec += 60;
	    }
	    fStart.min -= fStop.min;
	    while ( fStart.min < 0 ) {
	      fStart.hour --;
	      fStart.min += 60;
	    }
	    fStart.hour -= fStop.hour;
	    while ( fStart.hour < 0 ) {
	      fStart.day --;
	      fStart.hour += 24;
	    }
	    fStart.day -= fStop.day;
	    while ( fStart.day <= 0 ) {
	      fStart.month --;
	      if ( fStart.month < 0 ) {
		fStart.year --;
		fStart.month += 12;
	      }
	      fStart.day += monthdays[fStart.month];
	    }
	    while ( fStart.month <= 0 ) {
	      fStart.year --;
	      fStart.month += 12;
	    }
	    if ( fStart.year < 0 )
	      fStart.year += 100;
	  }
	  if ( fStart.year < 10 )
	    sprintf ( starttime, "0%d-%d-%d-%d-%d-%d", fStart.year,fStart.month,fStart.day,fStart.hour,fStart.min,fStart.sec );
	  else
	    sprintf ( starttime, "%d-%d-%d-%d-%d-%d", fStart.year,fStart.month,fStart.day,fStart.hour,fStart.min,fStart.sec );
	  duration = fStop.sec + fStop.min*60 + (fStop.hour+fStop.day*24)*3600;
	  /* GTS starting time needed for graphing */
	  gpstimest = DataUTCtoGPS(starttime);
	}
	else { /* GPS */
	  if ( startstop == 1) {
	     gpstimest -= duration;
	  }
	  sprintf ( starttime, "%d", gpstimest);
	}
	
	strcpy(serverIP, argv[1]);
	serverPort = atoi(argv[2]);
	userPort = 7000; 

	/* Connect to Data Server */
	if ( copies < 0 )
	  fprintf ( stderr, "LongPlayBack starting\n" );
	else
	  fprintf ( stderr, "LongPlayBack Ch.%d starting\n", chNum[0] );
	fprintf ( stderr,"Connecting to Server %s-%d\n", serverIP, serverPort );
	if ( DataConnect(serverIP, serverPort, userPort, read_block) != 0 ) {
	     if ( copies < 0 )
	       fprintf ( stderr, "LongPlayBack: cann't connect to the data server %s-%d. Make sure the server is running.\n", serverIP, serverPort );
	     else
	       fprintf ( stderr, "LongPlayBack Ch.%d: cann't connect to the data server %s-%d. Make sure the server is running.\n", chNum[0], serverIP, serverPort );
	     exit(1);
	}


	for ( j=0; j<windowNum; j++ ) {
	   DataChanAdd(chName[j], 0);
        }

        switch ( playMode ) {
          case PLAYDATA: 
	      if (debug) fprintf(stderr, "main() - case PLAYDATA\n") ;
	      if ( strcmp(starttime, "0-0-0-0-0-0") == 0 ) {
		 fprintf ( stderr, "LongPlayBack Error: no starting time\n"  );
		 exit(0);
	      }
	      else {
		 processID = DataWrite(starttime, duration, isgps);
		 if ( copies < 0 )
		   fprintf ( stderr, "LongPlayBack time %s, duration %d\n", starttime, duration );
		 else
		   fprintf ( stderr, "LongPlayBack  Ch.%d time %s, duration %d\n",  chNum[0], starttime, duration );
	      }
              break;
          case PLAYTREND: 
	      if (debug) fprintf(stderr, "main() - case PLAYTREND\n") ;
	      processID = DataWriteTrend(starttime, duration, 1, isgps);
	      if ( copies < 0 )
		fprintf ( stderr, "Request second-trend: time %s, duration %d\n", starttime, duration );
	      else
		fprintf ( stderr, "Request second-trend Ch.%d: time %s, duration %d\n", chNum[0], starttime, duration );
              break;
          case PLAYTREND60: 
	      if (debug) fprintf(stderr, "main() - case PLAYTREND60\n") ;
	      processID = DataWriteTrend(starttime, duration, 60, isgps);
	      if ( copies < 0 )
		fprintf ( stderr, "Request minute-trend: time %s, duration %d\n", starttime, duration );
	      else
		fprintf ( stderr, "Request minute-trend Ch.%d: time %s, duration %d\n", chNum[0], starttime, duration );
              break;
          default: 
              break;
        }
	
	if ( ((long)processID) == -1 ) {
	   finished = 1;
	   fprintf ( stderr,"No data output.\n" );
	}
	while ( !finished ) sleep(1);

	return 0;
}


void* read_block()
{
int     dataRecv, irate = 0, bsec, hr, firsttime = 0;
double  chData[FAST], fvalue;
int     sRate[CHANNUM];
long    totaldata = 0, totaldata0 = 0, skip = 0, skipstatus; 
time_t  ngps, ngps0;
char    tempfile[100];
FILE    *fd[16], *fdM[16], *fdm[16];
int     i, j, l, bc, bytercv;

	dataRecv = 0;
	DataReadStart();
	for (j=0; j<windowNum; j++ ) {
	  if ( playMode == PLAYDATA ) {
	    sprintf( tempfile, "%s%d", filename, j );
	    fd[j] = fopen( tempfile, "w" );
	    if ( fd[j] == NULL ) {
	      fprintf ( stderr, "Unexpected error when opening files. Exit.\n" );
	      return;
	    }
	  }
	  else {
	    if ( dmax ) {
	      sprintf( tempfile, "%s%dM", filename, j );
	      fdM[j] = fopen( tempfile, "w" );
	      if ( fdM[j] == NULL ) {
		fprintf ( stderr, "Unexpected error when opening files. Exit.\n" );
		return;
	      }
	    }
	    if ( dmin ) {
	      sprintf( tempfile, "%s%dm", filename, j );
	      fdm[j] = fopen( tempfile, "w" );
	      if ( fdm[j] == NULL ) {
		fprintf ( stderr, "Unexpected error when opening files. Exit.\n" );
		return;
	      }
	    }
	    if ( dmean ) {
	      sprintf( tempfile, "%s%d", filename, j );
	      fd[j] = fopen( tempfile, "w" );
	      if ( fd[j] == NULL ) {
		fprintf ( stderr, "Unexpected error when opening files. Exit.\n" );
		return;
	      }
	    }
	  }
	}

	while ( 1 ) {
	   if ( (bytercv = DataRead()) == -2 ) {
	      fprintf ( stderr, "Reset slopes and offsets:\n" );
	      for ( j=0; j<windowNum; j++ ) {
		if ( strcmp(chUnit[j], "no_conv.") == 0 ) {
		  slope[j] = 1.0;
		  offset[j] = 0.0;
		}
		else
		  DataGetChSlope(chName[j], &slope[j], &offset[j], &chstatus[j]);
		 fprintf ( stderr, "%s %f %f %d\n", chName[j], slope[j], offset[j], chstatus[j] );
              }
	   }
	   else if ( bytercv < 0 ) {
	     if ( copies < 0 )
	       fprintf ( stderr, "LONG: DataRead = %d\n", bytercv );
	     else
	       fprintf ( stderr, "LONG Ch.%d: DataRead = %d\n", chNum[0], bytercv );
              DataReadStop();
	      break;
	   }
	   else if ( bytercv == 0 ) {
	     if ( copies < 0 )
	       fprintf ( stderr, "LONG: trailer received\n" );
	     else
	       fprintf ( stderr, "LONG Ch.%d: trailer received\n", chNum[0]);
	      break;
	   }
	   else {
	     /*if ( firsttime == 0 ) {
	      for ( j=0; j<windowNum; j++ ) {
fprintf ( stderr,"!!j=%d:\n", j  );
		if ( strcmp(chUnit[j], "no_conv.") == 0 ) {
fprintf ( stderr,"!!strcmp = 0\n" );
		  slope[j] = 1.0;
		  offset[j] = 0.0;
		}
		else
		  DataGetChSlope(chName[j], &slope[j], &offset[j], &chstatus[j]);
		 fprintf ( stderr, "%s %f %f %d\n", chName[j], slope[j], offset[j], chstatus[j] );
              }
	      } */
           DataTimestamp(timestring);
	   ngps = DataTimeGps();
	   if ( firsttime == 0 ) {
              firsttime = 1;
	      ngps0 = ngps;
	      strcpy(firsttimestring, timestring);
           }
	   if ( ngps - ngps0 - totaldata > 0 ) {
	      skip += ngps - ngps0 - totaldata;
	      if ( copies < 0 )
		fprintf ( stderr, "           WARNING: data skipped: %d sec.\n", ngps - ngps0 - totaldata );
	      else
		fprintf ( stderr, "           WARNING Ch.%d: data skipped: %d sec.\n", chNum[0], ngps - ngps0 - totaldata );
	      skipstatus = 1;
	   }
	   else
	      skipstatus = 0;
	   if ( copies < 0 )
	     fprintf ( stderr, "[#%d] data read: %s\n", processID, timestring );
	   else
	     fprintf ( stderr, "[#%d] data read Ch.%d: %s\n", processID, chNum[0], timestring );
           if ( playMode == PLAYDATA ) {
	      bsec = DataTrendLength();
	      printf ( "            Block length =%d seconds\n", bsec );
	      hr = 1;
	      for ( j=0; j<windowNum; j++ ) {
	      for ( bc=0; bc<bsec; bc++ ) {
		 /* Find data in the desired data channels */
		 sRate[j] = DataGetCh(chName[j], chData, bc, 1);
                 switch ( xyType[j] ) {
                   case 1: /* Log */
		     for ( i=0; i<sRate[j]; i++ ) {
		       fvalue = chData[i];
		       if ( fvalue == 0.0 ) {
			 fvalue = ZEROLOG;
			 warning = 1;
		       }
		       else
			 fvalue = fabs(fvalue);
		       fprintf ( fd[j], "%2f\t%le\n", (double)(ngps0+totaldata+bc)+(double)i/sRate[j], offset[j]+slope[j]*fvalue );
		     }
                     break;
                   case 2: /* Ln */
		     for ( i=0; i<sRate[j]; i++ ) {
		       fvalue = chData[i];
		       if ( fvalue == 0.0 ) {
			 fvalue = ZEROLOG;
			 warning = 1;
		       }
		       else
			 fvalue = log(fabs(fvalue));
		       fprintf ( fd[j], "%2f\t%le\n", (double)(ngps0+totaldata+bc)+(double)i/sRate[j], offset[j]+slope[j]*fvalue );
		     }
		     break;
                   default: 
		     for ( i=0; i<sRate[j]; i++ ) {
		       fprintf ( fd[j], "%2f\t%le\n", (double)(ngps0+totaldata+bc)+(double)i/sRate[j], offset[j]+slope[j]*chData[i] );
		     }
		     break;
                 }	
	      }	 
              }
	      totaldata = totaldata + bsec;
	   }
	   else {
	      irate = DataTrendLength();
              totaldata0 = ngps - ngps0 + irate;
	      if ( playMode == PLAYTREND60 ) 
		 irate = irate/60;
	      xTick = (double*)malloc(sizeDouble * irate);
	      trendsec = (struct DTrend*)malloc(sizeDTrend * irate);
	      if ( playMode == PLAYTREND60 ) 
		 hr = 60;
	      else
		 hr = 1;
	      if ( xaxisFormat == XAXISGTS ) {
		 for ( i=0; i<irate; i++ ) 
		    xTick[i] = ngps + i*hr;
	      }
	      else { /* XAXISUTC */
		 for ( i=0; i<irate; i++ ) 
		    xTick[i] = juliantime(timestring, i*hr);
	      }
	      for ( j=0; j<windowNum; j++ ) {
		 DataTrendGetCh(chName[j], trendsec );
		 if ( dcstep == 1 ) {
                 switch ( xyType[j] ) {
                   case 1: /* Log */
		     if ( dmax ) {
		       for ( i=0; i<irate; i++ ) {
			 fvalue = trendsec[i].max;
			 if ( fvalue == 0.0 ) {
			   fvalue = ZEROLOG;
			   warning = 1;
			 }
			 else
			   fvalue = fabs(fvalue);
			 fprintf ( fdM[j], "%2f\t%le\n", xTick[i], offset[j]+slope[j]*fvalue );
		       }
		     }
		     if ( dmean ) {
		       for ( i=0; i<irate; i++ ) {
			 fvalue = trendsec[i].mean;
			 if ( fvalue == 0.0 ) {
			   fvalue = ZEROLOG;
			   warning = 1;
			 }
			 else
			   fvalue = fabs(fvalue);
			 fprintf ( fd[j], "%2f\t%le\n", xTick[i], offset[j]+slope[j]*fvalue );
		       }
		     }
		     if ( dmin ) {
		       for ( i=0; i<irate; i++ ) {
			 fvalue = trendsec[i].min;
			 if ( fvalue == 0.0 ) {
			   fvalue = ZEROLOG;
			   warning = 1;
			 }
			 else
			   fvalue = fabs(fvalue);
			 fprintf ( fdm[j], "%2f\t%le\n", xTick[i], offset[j]+slope[j]*fvalue );
		       }
		     }
		     break;
                   case 2: /* Ln */
		     if ( dmax ) {
		       for ( i=0; i<irate; i++ ) {
			 fvalue = trendsec[i].max;
			 if ( fvalue == 0.0 ) {
			   fvalue = ZEROLOG;
			   warning = 1;
			 }
			 else
			   fvalue = log(fabs(fvalue));
			 fprintf ( fdM[j], "%2f\t%le\n", xTick[i], offset[j]+slope[j]*fvalue );
		       }
		     }
		     if ( dmean ) {
		       for ( i=0; i<irate; i++ ) {
			 fvalue = trendsec[i].mean;
			 if ( fvalue == 0.0 ) {
			   fvalue = ZEROLOG;
			   warning = 1;
			 }
			 else
			   fvalue = log(fabs(fvalue));
			 fprintf ( fd[j], "%2f\t%le\n", xTick[i], offset[j]+slope[j]*fvalue );
		       }
		     }
		     if ( dmin ) {
		       for ( i=0; i<irate; i++ ) {
			 fvalue = trendsec[i].min;
			 if ( fvalue == 0.0 ) {
			   fvalue = ZEROLOG;
			   warning = 1;
			 }
			 else
			   fvalue = log(fabs(fvalue));
			 fprintf ( fdm[j], "%2f\t%le\n", xTick[i], offset[j]+slope[j]*fvalue );
		       }
		     }
		     break;
                   default: 
		     if ( dmax ) {
		       for ( i=0; i<irate; i++ ) {
			 fprintf ( fdM[j], "%2f\t%le\n", xTick[i], offset[j]+slope[j]*trendsec[i].max );
		       }
		     }
		     if ( dmean ) {
		       for ( i=0; i<irate; i++ ) {
			 fprintf ( fd[j], "%2f\t%le\n", xTick[i], offset[j]+slope[j]*trendsec[i].mean );
		       }
		     }
		     if ( dmin ) {
		       for ( i=0; i<irate; i++ ) {
			 fprintf ( fdm[j], "%2f\t%le\n", xTick[i], offset[j]+slope[j]*trendsec[i].min );
		       }
		     }
		     break;
                 }	
		 }
		 else { /* dcstep > 1 */
		   if ( dmax ) {
		     fprintf ( fdM[j], "%d %2f %le\n", skipstatus, xTick[0], offset[j]+slope[j]*trendsec[0].max );
		     for ( i=1; i<irate; i++ ) {
		       fprintf ( fdM[j], "%d %2f %le\n", 0, xTick[i], offset[j]+slope[j]*trendsec[i].max );
		     }
		   }
		   if ( dmean ) {
		     fprintf ( fd[j], "%d %2f %le\n", skipstatus, xTick[0], offset[j]+slope[j]*trendsec[0].mean );
		     for ( i=1; i<irate; i++ ) {
		       fprintf ( fd[j], "%d %2f %le\n", 0, xTick[i], offset[j]+slope[j]*trendsec[i].mean );
		     }
		   }
		   if ( dmin ) {
		     fprintf ( fdm[j], "%d %2f %le\n", skipstatus, xTick[0], offset[j]+slope[j]*trendsec[0].min );
		     for ( i=1; i<irate; i++ ) {
		       fprintf ( fdm[j], "%d %2f %le\n", 0, xTick[i], offset[j]+slope[j]*trendsec[i].min );
		     }
		   }
		 }
	      }
	      if ( irate > 1 )
		 fprintf ( stderr, "           data length %d\n", irate );
	      /*totaldata += irate;*/
	      totaldata = totaldata0;
	      free(xTick);
	      free(trendsec);
	   }
	   dataRecv = 1;
	}
        }  /* end of while loop  */

	warning = 0;
	if ( dataRecv ) {
           /* Launch xmgr with named-pipe support */
	   if (ACEgrOpen(1000000, displayIP, origDir) == -1) {
	     fprintf ( stderr, "Can't run xmgr. \n" );
	     exit (EXIT_FAILURE);
	   }
	   fprintf ( stderr, "Done launching xmgr \n" );
           switch ( playMode ) {
             case PLAYDATA:
	         ACEgrPrintf( "with g0" );
		 for (j=0; j<windowNum; j++ ) {
		    fprintf ( fd[j], "&\n" );
		    fclose( fd[j] );
		    sprintf( tempfile, "%s%d", filename, j );
		    ACEgrPrintf( "read xy \"%s\"", tempfile ); 
		 }
                 break;
             case PLAYTREND: 
             case PLAYTREND60: 
		 for ( j=0; j<windowNum; j++ ) {
		   if ( multiple )
		     ACEgrPrintf( "with g0" );
		   else
		     ACEgrPrintf( "with g%d", j );
		   if ( dmax ) {
		     if ( dcstep == 1 )
		       fprintf ( fdM[j], "&\n" );
		     fclose( fdM[j] );
		     sprintf( tempfile, "%s%dM", filename, j );
		     if ( dcstep == 1 )
		        ACEgrPrintf( "read xy \"%s\"", tempfile );
		     else
		        stepread(tempfile, dcstep, 0, xyType[j]);
		   }
		   if ( dmean ) {
		     if ( dcstep == 1 )
		       fprintf ( fd[j], "&\n" );
		     fclose( fd[j] );
		     sprintf( tempfile, "%s%d", filename, j );
		     if ( dcstep == 1 )
		        ACEgrPrintf( "read xy \"%s\"", tempfile );
		     else
		        stepread(tempfile, dcstep, 1, xyType[j]);
		   }
		   if ( dmin ) {
		     if ( dcstep == 1 )
		       fprintf ( fdm[j], "&\n" );
		     fclose( fdm[j] );
		     sprintf( tempfile, "%s%dm", filename, j );
		     if ( dcstep == 1 )
		        ACEgrPrintf( "read xy \"%s\"", tempfile );
		     else
		        stepread(tempfile, dcstep, 2, xyType[j]);
		   }
		 }
                 break;
             default: 
                 break;
           }
	   
           if ( playMode == PLAYDATA ) 
	      irate = 1;
	   else {
	     if ( copies < 0 )
	       fprintf ( stderr,"totla data skipped: %d second(s)\n", skip );
	     else
	       fprintf ( stderr,"Ch.%d totla data skipped: %d second(s)\n", chNum[0], skip );
	   }
           if ( playMode == PLAYTREND60 ) {
	     if ( copies < 0 )
	       fprintf ( stderr, "LongPlayBack: total %9.1f minutes (%d seconds) of data displayed\n", (float)totaldata/hr, totaldata );
	     else
	       fprintf ( stderr, "LongPlayBack Ch.%d: total %9.1f minutes (%d seconds) of data displayed\n", chNum[0], (float)totaldata/hr, totaldata );
	      timeinstring( timestring, (irate-1)*60, lasttimestring );
	   }
	   else {
	     if ( copies < 0 )
	       fprintf ( stderr, "LongPlayBack: total %d seconds of data displayed\n", totaldata );
	     else
	       fprintf ( stderr, "LongPlayBack Ch.%d: total %d seconds of data displayed\n", chNum[0], totaldata );
	     timeinstring( timestring, irate-1, lasttimestring );
	   }
	   if ( !multiple ) {
	     if ( skip > 0 )
	       graphout(totaldata/hr, 1);
	     else
	       graphout(totaldata/hr, 0);
	   }
	   else {
	     if ( skip > 0 )
	       graphmulti(totaldata/hr, 1);
	     else
	       graphmulti(totaldata/hr, 0);
	   }
	}
	else {
	   if ( copies < 0 )
	     fprintf ( stderr, "LongPlayBack: no data received\n" );
	   else
	     fprintf ( stderr, "LongPlayBack Ch.%d: no data received\n", chNum[0] );
	   finished = 1;
	   fflush(stdout);
	   return NULL;
	}
        if ( warning ) {
	   if ( copies < 0 )
	     fprintf ( stderr,"Warning: 0 as input of log, put 1.0e-20 instead\n"  );
	   else
	     fprintf ( stderr,"Warning Ch.%d: 0 as input of log, put 1.0e-20 instead\n", chNum[0] );
	}

	/* Finishing */
	ACEgrFlush ();        
	sleep(4);
	/*DataWriteStop(processID);*/
	ACEgrPrintf( "close pipe" );
	DataQuit();
	finished = 1;
	fflush(stdout);
	return NULL;
}



void graphout(int width, short skip)
{
float  x0[16], y0[16], w, h, x1, y1; 
double xmin, xmax;
int    i;

        if ( longauto ) 
	  fprintf ( stderr,"Auto setting...\n" );
	else
	  fprintf ( stderr,"Non auto setting...\n" );
        if ( playMode == PLAYDATA ) {
           xmin = (double)gpstimest;
	   xmax = xmin + (double)duration;
        }
        else if ( xaxisFormat == XAXISGTS ) { /* GTS */
	   xmin = (double)gpstimest;
           xmax = (double)(gpstimest + duration - 1);
        }
	else { /* UTC */
	   if ( isgps == 0 ) {
	     xmin = juliantime(starttime, 0);
	     xmax = juliantime(starttime, duration);
	   }
	   else {
	     xmin = juliantimegps(gpstimest);
	     xmax = juliantimegps(gpstimest+duration);
	   }
	}
	if (xmax == xmin)
	  xmax = xmin + 1;
        ACEgrPrintf( "doublebuffer true" );
	ACEgrPrintf( "background color 7" );
	switch ( windowNum ) {
	     case 1: 
	         x0[0] = 0.06; y0[0] = 0.028;
		 w = 0.90;
		 h = 0.88;
		 break;
	     case 2: 
	         x0[0] = 0.06; y0[0] = 0.026;
		 x0[1] = 0.06; y0[1] = 0.51; 
		 w = 0.90;
		 h = 0.42;
		 break;
	     case 3: 
	     case 4: 
	         x0[0] = 0.04; y0[0] = 0.10; 
		 x0[1] = 0.04; y0[1] = 0.57;
		 x0[2] = 0.54; y0[2] = 0.10; 
		 x0[3] = 0.54; y0[3] = 0.57; 
		 w = 0.45;
		 h = 0.35;
		 break;
	     case 5: 
	     case 6: 
	         x0[0] = 0.10; y0[0] = 0.02; 
		 x0[1] = 0.10; y0[1] = 0.32; 
		 x0[2] = 0.10; y0[2] = 0.62; 
		 x0[3] = 0.56; y0[3] = 0.02;
		 x0[4] = 0.56; y0[4] = 0.32; 
		 x0[5] = 0.56; y0[5] = 0.62; 
		 w = 0.37;
		 h = 0.26;
		 break;
	     case 7: 
	     case 8: 
	     case 9: 
	         x0[0] = 0.04; y0[0] = 0.02; 
		 x0[1] = 0.04; y0[1] = 0.32; 
		 x0[2] = 0.04; y0[2] = 0.62; 
		 x0[3] = 0.3666; y0[3] = 0.02;
		 x0[4] = 0.3666; y0[4] = 0.32; 
		 x0[5] = 0.3666; y0[5] = 0.62; 
		 x0[6] = 0.6933; y0[6] = 0.02; 
		 x0[7] = 0.6933; y0[7] = 0.32;
		 x0[8] = 0.6933; y0[8] = 0.62; 
		 w = 0.295;
		 h = 0.26;
		 break;
	     case 10: 
	     case 11: 
	     case 12: 
	         x0[0] = 0.04; y0[0] = 0.02; 
		 x0[1] = 0.04; y0[1] = 0.32; 
		 x0[2] = 0.04; y0[2] = 0.62; 
		 x0[3] = 0.2875; y0[3] = 0.02;
		 x0[4] = 0.2875; y0[4] = 0.32; 
		 x0[5] = 0.2875; y0[5] = 0.62; 
		 x0[6] = 0.535; y0[6] = 0.02; 
		 x0[7] = 0.535; y0[7] = 0.32;
		 x0[8] = 0.535; y0[8] = 0.62; 
		 x0[9] = 0.7825; y0[9] = 0.02; 
		 x0[10] = 0.7825; y0[10] = 0.32;
		 x0[11] = 0.7825; y0[11] = 0.62; 
		 w = 0.2075;
		 h = 0.26;
		 break;
	     default: /* case 13 - 16  */
	         x0[0] = 0.04; y0[0] = 0.019; 
		 x0[1] = 0.04; y0[1] = 0.259; 
	         x0[2] = 0.04; y0[2] = 0.499; 
	         x0[3] = 0.04; y0[3] = 0.739; 
		 x0[4] = 0.2875; y0[4] = 0.019; 
		 x0[5] = 0.2875; y0[5] = 0.259; 
		 x0[6] = 0.2875; y0[6] = 0.499; 
		 x0[7] = 0.2875; y0[7] = 0.739; 
		 x0[8] = 0.535; y0[8] = 0.019; 
		 x0[9] = 0.535; y0[9] = 0.259; 
		 x0[10] = 0.535; y0[10] = 0.499;
		 x0[11] = 0.535; y0[11] = 0.739; 
		 x0[12] = 0.7825; y0[12] = 0.019;
		 x0[13] = 0.7825; y0[13] = 0.259; 
		 x0[14] = 0.7825; y0[14] = 0.499; 
		 x0[15] = 0.7825; y0[15] = 0.739;
		 w = 0.2075;
		 h = 0.20;
		 break;
	}
	for ( i= 0; i<windowNum; i++ ) {
	   ACEgrPrintf( "with g%d", i );
	   ACEgrPrintf( "view %f, %f,%f,%f", x0[i]+XSHIFT,y0[i]+YSHIFT,x0[i]+w,y0[i]+h );
	   ACEgrPrintf( "frame color 1" );
	   ACEgrPrintf( "frame background color 0" );
	   ACEgrPrintf( "frame fill on" );
	   switch ( xyType[i] ) {
	   case 2: /* Ln */
	     ACEgrPrintf( "yaxis ticklabel prepend \"\\-Exp\"" );
	     break;
	   default: /* Linear */ 
	     ACEgrPrintf( "yaxis ticklabel prepend \"\\-\"" );
	     break;
	   }	      
	   ACEgrPrintf( "yaxis label \"%s\" ", chUnit[i] );
	   ACEgrPrintf( "yaxis label layout para" );
	   ACEgrPrintf( "yaxis label place auto" );
	   ACEgrPrintf( "yaxis label font 4" );
	   ACEgrPrintf( "yaxis label char size 0.9" );
	   ACEgrPrintf( "subtitle size 0.7" );
	   ACEgrPrintf( "subtitle font 1" );
	   ACEgrPrintf( "subtitle color 1" );
	   /*if ( chstatus[i] == 0 )
	     ACEgrPrintf( "subtitle color 1" );
	   else
	   ACEgrPrintf( "subtitle color 2" );*/
	   ACEgrPrintf( "world xmin %f", xmin );
	   ACEgrPrintf( "world xmax %f", xmax );
	   if ( playMode == PLAYDATA ) {
	     ACEgrPrintf( "xaxis ticklabel format decimal" );
	     ACEgrPrintf( "xaxis ticklabel prec 0" );
	   }
	   else if ( xaxisFormat == XAXISGTS ) {
	     ACEgrPrintf( "xaxis ticklabel format decimal" );
	     if ( width <= 2 )
	       ACEgrPrintf( "xaxis ticklabel prec 1" );
	     else
	       ACEgrPrintf( "xaxis ticklabel prec 0" );
	   }
	   else
	      ACEgrPrintf( "xaxis ticklabel format yymmddhms" );
	   if ( windowNum <= 6 ) {
	      ACEgrPrintf( "xaxis tick major %le", (xmax-xmin)/4.0 );
	      ACEgrPrintf( "xaxis tick minor %le", (xmax-xmin)/8.0 );
	   }
	   else {
	      ACEgrPrintf( "xaxis tick major %le", (xmax-xmin)/2.0 );
	      ACEgrPrintf( "xaxis tick minor %le", (xmax-xmin)/4.0 );
	   }
	   if ( longauto == 0 ) {
	     ACEgrPrintf( "world ymin %le", winYMin[i] );
	     ACEgrPrintf( "world ymax %le", winYMax[i] );
	     ACEgrPrintf( "yaxis tick major %le", (winYMax[i]-winYMin[i])/5.0 );
	     ACEgrPrintf( "yaxis tick minor %le", (winYMax[i]-winYMin[i])/10.0 );
	   }
	   ACEgrPrintf( "xaxis ticklabel char size 0.53" );
	   ACEgrPrintf( "xaxis tick color 1" );
	   if ( xGrid ) {
	      ACEgrPrintf( "xaxis tick major color 7" );
	      ACEgrPrintf( "xaxis tick major linewidth 1" );
	      ACEgrPrintf( "xaxis tick major linestyle 1" );
	      ACEgrPrintf( "xaxis tick minor color 7" );
	      ACEgrPrintf( "xaxis tick minor linewidth 1" );
	      ACEgrPrintf( "xaxis tick minor linestyle 3" );
	      ACEgrPrintf( "xaxis tick major grid on" );
	      ACEgrPrintf( "xaxis tick minor grid on" );
	   }
	   ACEgrPrintf( "yaxis ticklabel char size 0.80" );
	   ACEgrPrintf( "yaxis tick color 1" );
	   if ( yGrid ) {
	      ACEgrPrintf( "yaxis tick major color 7" );
	      ACEgrPrintf( "yaxis tick major linewidth 1" );
	      ACEgrPrintf( "yaxis tick major linestyle 1" );
	      ACEgrPrintf( "yaxis tick minor color 7" );
	      ACEgrPrintf( "yaxis tick minor linewidth 1" );
	      ACEgrPrintf( "yaxis tick minor linestyle 3" );
	      ACEgrPrintf( "yaxis tick major grid on" );
	      ACEgrPrintf( "yaxis tick minor grid on" );
	   }
	   if ( xyType[i] == 3 )
	      ACEgrPrintf( "yaxis ticklabel format exponential" );
	   else if ( xyType[i] == 1 ) {
	      ACEgrPrintf( "g%d type logy", i );
	      ACEgrPrintf( "yaxis ticklabel prec 5" );
	      ACEgrPrintf( "yaxis ticklabel format exponential" );
	   }
	   else
	      ACEgrPrintf( "yaxis ticklabel format general" );
	}

        if ( playMode == PLAYDATA ) {
	   ACEgrPrintf( "with g0" );
	   for ( i=0; i<windowNum; i++ ) {
	      ACEgrPrintf( "s%d color %d", i, gColor[i] );
           }
	   for ( i=1; i<windowNum; i++ ) {
	      ACEgrPrintf( "move g0.s%d to g%d.s0", i, i );
           }
	   for ( i=0; i<windowNum; i++ ) {
	     ACEgrPrintf( "g%d fixedpoint off", i );
	     ACEgrPrintf( "g%d fixedpoint type 0", i );
	     ACEgrPrintf( "g%d fixedpoint xy 0.000000, 0.000000", i );
	     ACEgrPrintf( "g%d fixedpoint format decimal decimal", i );
	     ACEgrPrintf( "g%d fixedpoint prec 0, 6", i );
           }
        }
	else {
           for ( i=0; i<windowNum; i++ ) {
	      ACEgrPrintf( "with g%d", i );
	      ACEgrPrintf( "s0 color 2" );
	      ACEgrPrintf( "s1 color 6" );
	      ACEgrPrintf( "s2 color 15" );

	      if (  xaxisFormat == XAXISUTC ) {
		ACEgrPrintf( "g%d fixedpoint off", i );
		ACEgrPrintf( "g%d fixedpoint type 0", i );
		ACEgrPrintf( "g%d fixedpoint xy 0.000000, 0.000000", i );
		ACEgrPrintf( "g%d fixedpoint format mmddyyhms decimal", i );
		ACEgrPrintf( "g%d fixedpoint prec 6, 6", i );
	      }
	      else if (  xaxisFormat == XAXISGTS ) {
		ACEgrPrintf( "g%d fixedpoint off", i );
		ACEgrPrintf( "g%d fixedpoint type 0", i );
		ACEgrPrintf( "g%d fixedpoint xy 0.000000, 0.000000", i );
		ACEgrPrintf( "g%d fixedpoint format decimal decimal", i );
		ACEgrPrintf( "g%d fixedpoint prec 0, 6", i );
	      }
	      if (linestyle != 1) {
		if ( (linestyle == 2) || skip ) {
		  ACEgrPrintf( "s0 linestyle 0" );
		  ACEgrPrintf( "s1 linestyle 0" );
		  ACEgrPrintf( "s2 linestyle 0" );
		  ACEgrPrintf( "s0 symbol 2" );
		  ACEgrPrintf( "s1 symbol 2" );
		  ACEgrPrintf( "s2 symbol 2" );
		  ACEgrPrintf( "s0 symbol size 0.2" );
		  ACEgrPrintf( "s1 symbol size 0.2" );
		  ACEgrPrintf( "s2 symbol size 0.2" );
		  ACEgrPrintf( "s0 symbol fill 1" );
		  ACEgrPrintf( "s1 symbol fill 1" );
		  ACEgrPrintf( "s2 symbol fill 1" );
		}
	      }
	   }
	}

        for ( i=0; i<windowNum; i++ ) {
	   ACEgrPrintf( "with g%d", i );
	   ACEgrPrintf( "g%d on", i );
	   if ( playMode == PLAYDATA ) 
	     ACEgrPrintf( "subtitle \"Ch %d: %s\"", chNum[i], chName[i] );
	   else
	     ACEgrPrintf( "subtitle \"Trend Ch %d: %s\"", chNum[i], chName[i] );
	   if (longauto)
	     ACEgrPrintf( "autoscale yaxes" );
	   ACEgrPrintf( "clear stack" );
        }
	/** title string **/
        if ( playMode == PLAYDATA ) 
	   ACEgrPrintf( "string def \"\\1 Data Display start at %s (%d seconds)\"", firsttimestring, width); 
	else if ( xaxisFormat == XAXISUTC )
	   ACEgrPrintf( "string def \"\\1 Trend from %s to %s \"", firsttimestring,lasttimestring); 
	else {
           if ( playMode == PLAYTREND )
	      ACEgrPrintf( "string def \"\\1 Trend %d seconds from %s to %s\"", width, firsttimestring,lasttimestring ); 
           else if ( playMode == PLAYTREND60 )
	      ACEgrPrintf( "string def \"\\1 Trend %d minutes from %s to %s\"", width, firsttimestring,lasttimestring ); 
	}
	ACEgrPrintf( "string loctype view" );
	ACEgrPrintf( "string 0.14, 0.98" );
	ACEgrPrintf( "string color 1" );
	ACEgrPrintf( "string on" );
	ACEgrPrintf( "with g0" );
        if ( playMode != PLAYDATA ) {
	  if ( dmax ) {
	    ACEgrPrintf( "legend string 0 \"M A X\"" );
	    if ( dmean ) {
	      ACEgrPrintf( "legend string 1 \"M E A N\"" );
	      ACEgrPrintf( "legend string 2 \"M I N\"" );
	    }
	    else
	      ACEgrPrintf( "legend string 1 \"M I N\"" );
	  }
	  else if ( dmean ) {
	    ACEgrPrintf( "legend string 0 \"M E A N\"" );
	    ACEgrPrintf( "legend string 1 \"M I N\"" );
	  }
	  else 
	    ACEgrPrintf( "legend string 0 \"M I N\"" );
	  ACEgrPrintf( "legend loctype view" );
	  ACEgrPrintf( "legend 0.04, 0.98" );
	  ACEgrPrintf( "legend color 4" );
	  ACEgrPrintf( "legend char size 0.5" );
	  ACEgrPrintf( "legend on" );
	}
	ACEgrPrintf( "clear stack" );
	/*if ( windowNum > 1 )
	  ACEgrPrintf( "with g1" );*/
	ACEgrPrintf( "with g0" );
	ACEgrPrintf( "redraw" );
	fflush(stdout);
}


void graphmulti(int width, short skip)
{
double  xmin, xmax;
int     i, k;
char    tempstr[100];

        if ( playMode == PLAYDATA ) {
           xmin = 0.0;
	   xmax = (float)width;
        }
        else if ( xaxisFormat == XAXISGTS ) {
	   xmin = 0.0;
           xmax = (float)width - 1;
        }
	else {
	   xmin = juliantime(firsttimestring, 0);
	   xmax = juliantime(lasttimestring, 0);
	}
	if (xmax == xmin)
	  xmax = xmin + 1;
        ACEgrPrintf( "doublebuffer true" );
	ACEgrPrintf( "background color 7" );

	ACEgrPrintf( "with g0" ); /* main graph */
	ACEgrPrintf( "view %f,%f,%f,%f", 0.08,0.10,0.74,0.85 );
	ACEgrPrintf( "frame color 1" );
	ACEgrPrintf( "frame background color 0" );
	ACEgrPrintf( "frame fill on" );
	ACEgrPrintf( "xaxis tick color 1" );
	ACEgrPrintf( "yaxis tick color 1" );
	if ( xGrid ) {
	   ACEgrPrintf( "xaxis tick major color 7" );
	   ACEgrPrintf( "xaxis tick major linewidth 1" );
	   ACEgrPrintf( "xaxis tick major linestyle 1" );
	   ACEgrPrintf( "xaxis tick minor color 7" );
	   ACEgrPrintf( "xaxis tick minor linewidth 1" );
	   ACEgrPrintf( "xaxis tick minor linestyle 3" );
	   ACEgrPrintf( "xaxis tick major grid on" );
	   ACEgrPrintf( "xaxis tick minor grid on" );
	}
	if ( yGrid ) {
	   ACEgrPrintf( "yaxis tick major color 7" );
	   ACEgrPrintf( "yaxis tick major linewidth 1" );
	   ACEgrPrintf( "yaxis tick major linestyle 1" );
	   ACEgrPrintf( "yaxis tick minor color 7" );
	   ACEgrPrintf( "yaxis tick minor linewidth 1" );
	   ACEgrPrintf( "yaxis tick minor linestyle 3" );
	   ACEgrPrintf( "yaxis tick major grid on" );
	   ACEgrPrintf( "yaxis tick minor grid on" );
	}
	ACEgrPrintf( "subtitle size 1" );
	ACEgrPrintf( "subtitle font 1" );
	ACEgrPrintf( "subtitle color 1" );
	ACEgrPrintf( "world xmin %f", xmin );
	ACEgrPrintf( "world xmax %f", xmax );
	if ( windowNum <= 6 ) {
	   ACEgrPrintf( "xaxis tick major %le", (xmax-xmin)/4.0 );
	   ACEgrPrintf( "xaxis tick minor %le", (xmax-xmin)/8.0 );
	}
	else {
	   ACEgrPrintf( "xaxis tick major %le", (xmax-xmin)/2.0 );
	   ACEgrPrintf( "xaxis tick minor %le", (xmax-xmin)/4.0 );
	}
	ACEgrPrintf( "xaxis ticklabel char size 0.53" );
	if ( playMode == PLAYDATA ) 
	   ACEgrPrintf( "xaxis ticklabel prec 2" );
	else if ( xaxisFormat == XAXISGTS ) {
	  ACEgrPrintf( "xaxis ticklabel format decimal" );
	  if ( width <= 2 )
	    ACEgrPrintf( "xaxis ticklabel prec 1" );
	  else
	    ACEgrPrintf( "xaxis ticklabel prec 0" );
	}
	else
	  ACEgrPrintf( "xaxis ticklabel format yymmddhms" );
	ACEgrPrintf( "yaxis ticklabel char size 0.43" );
	ACEgrPrintf( "yaxis ticklabel format general" );

	ACEgrPrintf( "with g0" );
	/*ACEgrPrintf( "g0 on" );*/
	for ( i=0; i<windowNum; i++ ) {
	   ACEgrPrintf( "s%d color %d", i, gColor[i] );
	   ACEgrPrintf( "legend string %d \"Ch %d:%s\"", i, chNum[i], chName[i] );
	   if ( xyType[i] == 3 )
	      ACEgrPrintf( "yaxis ticklabel format exponential" );
	   /* if one channel is in Exp then show exp scale to all */
	   else if ( xyType[i] == 1 ) {
	      ACEgrPrintf( "g%d type logy", i );
	      ACEgrPrintf( "yaxis ticklabel format exponential" );
	   }
	}

	if (linestyle != 1) {
	  if ( (linestyle == 2) || skip ) {
	    for ( i=0; i<9; i++ ) {
	      ACEgrPrintf( "s%d linestyle 0", i );
	      ACEgrPrintf( "s%d symbol 2", i );
	      ACEgrPrintf( "s%d symbol size 0.2", i );
	      ACEgrPrintf( "s%d symbol fill 1", i );
	    }
	  }
	}
	ACEgrPrintf( "s10 linestyle 3" );
	ACEgrPrintf( "s11 linestyle 3" );
	ACEgrPrintf( "s12 linestyle 3" );
	ACEgrPrintf( "s13 linestyle 3" );
	ACEgrPrintf( "s14 linestyle 3" );
	ACEgrPrintf( "s15 linestyle 3" );
        if ( playMode == PLAYDATA ) 
	   ACEgrPrintf( "subtitle \"Display Multiple Data start at %s (%d seconds)\"", firsttimestring, width );
	else { 
	   if ( dmin )
	      strcpy (tempstr, "Display Multiple MIN Trend");
	   else if ( dmax )
	      strcpy (tempstr, "Display Multiple MAX Trend");
	   else 
	      strcpy (tempstr, "Display Multiple MEAN Trend");
	   if ( xaxisFormat == XAXISUTC ) {
	      ACEgrPrintf( "subtitle \"%s from %s to %s\"", tempstr, firsttimestring,lasttimestring ); 
           }
           else { /* XAXISGTS */
              if ( playMode == PLAYTREND )
		 ACEgrPrintf( "subtitle \"%s %d seconds from %s to %s\"", tempstr, width, firsttimestring,lasttimestring ); 
	      else if ( playMode == PLAYTREND60 )
		 ACEgrPrintf( "subtitle \"%s %d minutes from %s to %s\"", tempstr, width, firsttimestring,lasttimestring );
           }
	}
	ACEgrPrintf( "legend loctype view" );
	ACEgrPrintf( "legend 0.75, 0.82" );
	ACEgrPrintf( "legend char size 0.7" );
	ACEgrPrintf( "legend color 1" );
	ACEgrPrintf( "legend on" );
	ACEgrPrintf( "autoscale yaxes" );
	/*ACEgrPrintf( "clear stack" );*/

	fflush(stdout);

}


/* convert UTC time string to julian time */
double juliantime(char* timest, int extrasec)
{
int yy, mm, dd, hh,mn, ss;
double julian, hour;


  sscanf( timest, "%d-%d-%d-%d-%d-%d", &yy,&mm,&dd,&hh,&mn,&ss );
  if ( yy == 98 || yy == 99 )
    yy += 1900;
  else
    yy += 2000;
  ss += extrasec;
  hour = hh + mn/60.0 + ss/3600.0;
  julian =  ( 1461*(yy + 4800 + (mm - 14)/12))/4 +
               (367*(mm - 2 - 12*((mm - 14)/12)))/12 -
               (3*((yy + 4900 + (mm - 14)/12)/100))/4 + dd - 32075;
  julian = julian + hour/24.0 - 0.5;

  return julian; 
}


/* convert gps time to julian time */
double juliantimegps(int igps)
{
int yy, mm, dd, hh,mn, ss;
double julian, hour;
char   timest[24]; 

  DataGPStoUTC(igps, timest); 
  sscanf( timest, "%d-%d-%d-%d-%d-%d", &yy,&mm,&dd,&hh,&mn,&ss );
  if ( yy == 98 || yy == 99 )
    yy += 1900;
  else
    yy += 2000;
  hour = hh + mn/60.0 + ss/3600.0;
  julian =  ( 1461*(yy + 4800 + (mm - 14)/12))/4 +
               (367*(mm - 2 - 12*((mm - 14)/12)))/12 -
               (3*((yy + 4900 + (mm - 14)/12)/100))/4 + dd - 32075;
  julian = julian + hour/24.0 - 0.5;

  return julian; 
}


/* give time string after adding some extra seconds */
void timeinstring(char* timest, int extrasec, char* timestout )
{
int yy, mm, dd, hh,mn, ss;
int     monthdays[13] = {31,31,28,31,30,31,30,31,31,30,31,30, 31};

  sscanf( timest, "%d-%d-%d-%d-%d-%d", &yy,&mm,&dd,&hh,&mn,&ss );
  if ((yy+2000) % 4 == 0) /* leap year */
    monthdays[2] = 29;
  else
    monthdays[2] = 28;
  ss += extrasec;

  while ( ss >= 60 ) {
    mn++;
    ss -= 60;
  }
  while ( mn >= 60 ) {
    hh++;
    mn -= 60;
  }
  while ( hh >= 24 ) {
    dd++;
    hh -= 24;
  }
  while ( dd > monthdays[mm] ) {
    dd -= monthdays[mm];
    mm++;
    if ( mm > 12 ) {
      mm -= 12;
      yy++;
    }
  }
  if ( yy < 10 && yy >= 0 )
    sprintf ( timestout, "0%d-%d-%d-%d-%d-%d", yy,mm,dd,hh,mn,ss  );
  else
    sprintf ( timestout, "%d-%d-%d-%d-%d-%d", yy,mm,dd,hh,mn,ss  );
  return;
}


/* difference of two time strings. assume timest1>timest2 
   hr=1 in seconds;  hr=60 in minutes                         
*/
int difftimestrings(char* timest1, char* timest2, int hr)
{
int  yy1, mm1, dd1, hh1,mn1, ss1, yy2, mm2, dd2, hh2,mn2, ss2;
int  monthdays1[13] = {31,31,28,31,30,31,30,31,31,30,31,30, 31};
int  monthdays2[13] = {31,31,28,31,30,31,30,31,31,30,31,30, 31};
int totalsec, tmp, tot1, tot2, ii;

  if (strcmp(timest1, timest2) == 0) 
    return 0;
  sscanf( timest1, "%d-%d-%d-%d-%d-%d", &yy1,&mm1,&dd1,&hh1,&mn1,&ss1 );
  sscanf( timest2, "%d-%d-%d-%d-%d-%d", &yy2,&mm2,&dd2,&hh2,&mn2,&ss2 );
  /* leap year */
  if ((yy1+2000) % 4 == 0) 
    monthdays1[2] = 29;
  else
    monthdays1[2] = 28;
  if ((yy2+2000) % 4 == 0) 
    monthdays2[2] = 29;
  else
    monthdays2[2] = 28;
  tmp = ss1 - ss2;
  if (tmp < 0) {
    tmp = tmp + 60;
    mn1--;
  }
  totalsec = tmp;
  tmp = mn1 - mn2;
  if (tmp < 0) {
    tmp = tmp + 60;
    hh1--;
  }
  totalsec = totalsec + tmp*60;
  tmp = hh1 - hh2;
  if (tmp < 0) {
    tmp = tmp + 24;
    dd1--;
  }
  totalsec = totalsec + tmp*3600;

  /* total days beyond 2000 */
  tot1 = yy1*365;
  /* add one day for each leap year */
  if (yy1 == 1) 
    tot1 += 1;
  else if (yy1 > 1) 
    tot1 += (yy1 - 1)/4 + 1;
  for ( ii=0; ii<mm1-1; ii++) {
    tot1 += monthdays1[ii];
  }
  tot1 += dd1;
  tot2 = yy2*365;
  if (yy2 == 1) 
    tot2 += 1;
  else if (yy2 > 1) 
    tot2 += (yy2 - 1)/4 + 1;
  for ( ii=0; ii<mm2-1; ii++) {
    tot2 += monthdays2[ii];
  }
  tot2 += dd2;

  totalsec = (tot1-tot2)*24*3600 + totalsec;
  if ( hr== 60) totalsec = totalsec/60;
  return totalsec;
}


/* read data file into xmgr with a decimation 
   minmax = 0 (max), 1 (mean), 2 (min)
*/
void stepread(char* infile, int decm, int minmax, int type)
{
char   outfile[100], line[80], tempst[80];
FILE   *fpw, *fpr;
double x, y, yy[61]; 
int    i, count, status;

    sprintf ( outfile,"%sd", infile );
    fpr = fopen(infile, "r");
    fpw = fopen(outfile, "w");
    if (fpw == NULL) {
      printf ( "Error in opening writing file %s. Exit.\n", outfile );
      exit(1);
    }
    if (fpr == NULL) {
      printf ( "Error in opening reading file %s. Exit.\n", infile );
      exit(1);
    }

    count = 1;
    while (fgets ( line, 80, fpr ) != NULL) {
      sscanf ( line, "%d %lf %le\n", &status, &x, &yy[count]);
      if ( count % decm == 0 || status == 1 ) {
	if ( minmax == 1 ) { /* mean */
	  y = 0;
	  for ( i=1; i<=count; i++ ) {
	    y = y + yy[i];
	  }
	  y = y/count;
	  count = 1;
	}
	else if ( minmax == 0 ) { /* max */
	  y = yy[1];
	  for ( i=1; i<=count; i++ ) {
	    if ( y < yy[i] )
	      y = yy[i];
	  }
	  count = 1;
	}
	else if ( minmax == 2 ) { /* min */
	  y = yy[1];
	  for ( i=1; i<=count; i++ ) {
	    if ( y > yy[i] )
	      y = yy[i];
	  }
	  count = 1;
	}
	switch ( type ) {
          case 1: /* Log */
	    if ( y == 0.0 ) {
	      y = ZEROLOG;
	      warning = 1;
	    }
	    else
	      y = fabs(y);
	    break;
          case 2: /* Ln */
	    if ( y == 0.0 ) {
	      y = ZEROLOG;
	      warning = 1;
	    }
	    else
	      y = log(fabs(y));
	    break;
          default: 
	    break;
	}	
	fprintf ( fpw, "%2f\t%le\n", x, y );
      }
      else {
	count++;
      }
    }

    fprintf ( fpw, "&\n" );
    fclose(fpw);
    fclose(fpr);
    ACEgrPrintf( "read xy \"%s\"", outfile );
    remove(infile);
    return;
}
 
