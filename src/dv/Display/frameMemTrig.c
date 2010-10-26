/* frameMemRead.c                                                         */
/* LongPlayBack                                                           */
/* compile with gcc -o frameMemTrig frameMemTrig.c datasrv.o daqc_access.o*/
/*     -L/home/hding/Cds/Frame/Lib/Grace -l grace_np                      */
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
#include "../Th/datasrv.h"

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

struct ftm {
        int year;
        int month;
        int day;
        int hour;
        int min;
        int sec;
};
struct ftm fStart, fStop;
struct DTrend  *trendsec, *trendsec1, *trendsec2; 
double *xTick;

char  chName[CHANNUM][80], chUnit[CHANNUM][80];
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

/* trigger */
int trigchno, trigHow1=0, trigHow2=0, trigOp=0;
double trigLev1=0.0, trigLev2=0.0;  
char  chNameTrig1[80], chNameTrig2[80], chUnitTrig1[80], chUnitTrig2[80];
float slope1=1.0, slope2=1.0, offset1=0.0, offset2=0.0;
int   chstatus1, chstatus2;
int   chRateTrig1, chRateTrig2, minRate;
int   data_yes=0, data_no=0;
int   triggered[3600];

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
int     trigger_cond1(double data1[]);
int     trigger_cond2(double data1[], double data2[]);
int     trigger_trendcond1(int r);
int     trigger_trendcond2(int r);



int main(int argc, char *argv[])
{
int     serverPort, userPort;
char    serverIP[80];

int     j, sum; 
char    rdata[32];

FILE   *fd;

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
	/* trigger */
	fscanf ( fd, "%s", rdata);
	trigchno = atoi(rdata);
	fscanf ( fd, "%s", chNameTrig1);
	fscanf ( fd, "%s", rdata);
	chRateTrig1 = atoi(rdata);
	fscanf ( fd,"%s", chUnitTrig1 );
	fscanf ( fd, "%s", rdata);
	trigHow1 = atoi(rdata);
	fscanf ( fd, "%s", rdata);
	trigLev1 = atof(rdata);
	if (trigchno > 1) {
	  fscanf ( fd, "%s", chNameTrig2);
	  fscanf ( fd, "%s", rdata);
	  chRateTrig2 = atoi(rdata);
	  fscanf ( fd,"%s", chUnitTrig2 );
	  fscanf ( fd, "%s", rdata);
	  trigHow2 = atoi(rdata);
	  fscanf ( fd, "%s", rdata);
	  trigLev2 = atof(rdata);
	  fscanf ( fd, "%s", rdata);
	  trigOp = atoi(rdata);
	}
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

	if ( playMode != PLAYDATA ) { /* "equal to" only works for full data */
	  if (trigHow1==2 || (trigchno == 2 && trigHow2==2)) {
	    fprintf ( stderr, "WARNING: trigger condition 'Equal to' only applies to Full Data playback.\n No data output.\n" );
	    exit(0);
	  }
	}

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
	minRate = chRateTrig1;
	if (trigchno > 1) {
	  if (chRateTrig2 < minRate)
	    minRate = chRateTrig2;
	  DataChanAdd(chNameTrig2, minRate);
	}
	DataChanAdd(chNameTrig1, minRate);

        switch ( playMode ) {
          case PLAYDATA: 
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
	      processID = DataWriteTrend(starttime, duration, 1, isgps);
	      if ( copies < 0 )
		fprintf ( stderr, "Request second-trend: time %s, duration %d\n", starttime, duration );
	      else
		fprintf ( stderr, "Request second-trend Ch.%d: time %s, duration %d\n", chNum[0], starttime, duration );
              break;
          case PLAYTREND60: 
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
double  chData[FAST], chData1[FAST], chData2[FAST], fvalue;
int     sRate[CHANNUM], r1,r2;
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
	      if ( strcmp(chUnitTrig1, "no_conv.") == 0 ) {
		slope1 = 1.0;
		offset1 = 0.0;
	      }
	      else
		DataGetChSlope(chNameTrig1, &slope1, &offset1, &chstatus1);
	      if (trigchno > 1) {
		if ( strcmp(chUnitTrig2, "no_conv.") == 0 ) {
		  slope2 = 1.0;
		  offset2 = 0.0;
		}
		else
		  DataGetChSlope(chNameTrig2, &slope2, &offset2, &chstatus2);
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
	   if ( firsttime == 0 ) {
	      for ( j=0; j<windowNum; j++ ) {
		if ( strcmp(chUnit[j], "no_conv.") == 0 ) {
		  slope[j] = 1.0;
		  offset[j] = 0.0;
		}
		else
		  DataGetChSlope(chName[j], &slope[j], &offset[j], &chstatus[j]);
		fprintf ( stderr, "%s %f %f %d\n", chName[j], slope[j], offset[j], chstatus[j] );
              }
	      if ( strcmp(chUnitTrig1, "no_conv.") == 0 ) {
		slope1 = 1.0;
		offset1 = 0.0;
	      }
	      else
		DataGetChSlope(chNameTrig1, &slope1, &offset1, &chstatus1);
	      if (trigchno > 1) {
		if ( strcmp(chUnitTrig2, "no_conv.") == 0 ) {
		  slope2 = 1.0;
		  offset2 = 0.0;
		}
		else
		  DataGetChSlope(chNameTrig2, &slope2, &offset2, &chstatus2);
	      }
	   }
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
	      /* test trigger conditions by blocks */
	      if (trigchno == 1) {
		for ( bc=0; bc<bsec; bc++ ) {
		  r1 = DataGetCh(chNameTrig1, chData1, bc, 1);
		  if ( trigger_cond1(chData1) ) 
		    triggered[bc] = 1;
		  else
		    triggered[bc] = 0;
		}
	      }
	      else {
		for ( bc=0; bc<bsec; bc++ ) {
		  r1 = DataGetCh(chNameTrig1, chData1, bc, 1);
		  r2 = DataGetCh(chNameTrig2, chData2, bc, 1);
		  if ( trigger_cond2(chData1, chData2) )
		    triggered[bc] = 1;
		  else
		    triggered[bc] = 0;
		}
	      }
	      for ( j=0; j<windowNum; j++ ) {
	      for ( bc=0; bc<bsec; bc++ ) {
		if (triggered[bc] == 1) { 
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
              }
	      totaldata = totaldata + bsec;
	   }
	   else { /* trend */
	      irate = DataTrendLength();
              totaldata0 = ngps - ngps0 + irate;
	      if ( playMode == PLAYTREND60 ) 
		 irate = irate/60;
	      xTick = (double*)malloc(sizeDouble * irate);
	      trendsec = (struct DTrend*)malloc(sizeDTrend * irate);
	      trendsec1 = (struct DTrend*)malloc(sizeDTrend * irate);
	      if (trigchno == 2) 
		trendsec2 = (struct DTrend*)malloc(sizeDTrend * irate);
	      /* test trigger conditions by blocks */
	      if (trigchno == 1) {
		DataTrendGetCh(chNameTrig1, trendsec1 );
		trigger_trendcond1(irate);
	      }
	      else {
		DataTrendGetCh(chNameTrig1, trendsec1 );
		DataTrendGetCh(chNameTrig2, trendsec2 );
		trigger_trendcond2(irate);
	      }
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
			 if (triggered[i]) {
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
		     }
		     if ( dmean ) {
		       for ( i=0; i<irate; i++ ) {
			 if (triggered[i]) {
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
		     }
		     if ( dmin ) {
		       for ( i=0; i<irate; i++ ) {
			 if (triggered[i]) {
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
		     }
		     break;
                   case 2: /* Ln */
		     if ( dmax ) {
		       for ( i=0; i<irate; i++ ) {
			 if (triggered[i]) {
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
		     }
		     if ( dmean ) {
		       for ( i=0; i<irate; i++ ) {
			 if (triggered[i]) {
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
		     }
		     if ( dmin ) {
		       for ( i=0; i<irate; i++ ) {
			 if (triggered[i]) {
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
		     }
		     break;
                   default: 
		     if ( dmax ) {
		       for ( i=0; i<irate; i++ ) {
			 if (triggered[i]) 
			   fprintf ( fdM[j], "%2f\t%le\n", xTick[i], offset[j]+slope[j]*trendsec[i].max );
		       }
		     }
		     if ( dmean ) {
		       for ( i=0; i<irate; i++ ) {
			 if (triggered[i]) 
			   fprintf ( fd[j], "%2f\t%le\n", xTick[i], offset[j]+slope[j]*trendsec[i].mean );
		       }
		     }
		     if ( dmin ) {
		       for ( i=0; i<irate; i++ ) {
			 if (triggered[i]) 
			   fprintf ( fdm[j], "%2f\t%le\n", xTick[i], offset[j]+slope[j]*trendsec[i].min );
		       }
		     }
		     break;
                 }	
		 }
		 else { /* dcstep > 1 */
		   if ( dmax ) {
		     if (triggered[0]) 
		       fprintf ( fdM[j], "%d %2f %le\n", skipstatus, xTick[0], offset[j]+slope[j]*trendsec[0].max );
		     for ( i=1; i<irate; i++ ) {
		       if (triggered[i]) 
			 fprintf ( fdM[j], "%d %2f %le\n", 0, xTick[i], offset[j]+slope[j]*trendsec[i].max );
		     }
		   }
		   if ( dmean ) {
		     if (triggered[0]) 
		       fprintf ( fd[j], "%d %2f %le\n", skipstatus, xTick[0], offset[j]+slope[j]*trendsec[0].mean );
		     for ( i=1; i<irate; i++ ) {
		       if (triggered[i]) 
			 fprintf ( fd[j], "%d %2f %le\n", 0, xTick[i], offset[j]+slope[j]*trendsec[i].mean );
		     }
		   }
		   if ( dmin ) {
		     if (triggered[0]) 
		       fprintf ( fdm[j], "%d %2f %le\n", skipstatus, xTick[0], offset[j]+slope[j]*trendsec[0].min );
		     for ( i=1; i<irate; i++ ) {
		       if (triggered[i]) 
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
	   if (data_yes == 0) { /* no trigger, no data */
	     fprintf ( stderr, "Trigger condition(s) not satisfied. No data output.\n" );
	     finished = 1;
	     fflush(stdout);
	     return NULL;
	   }

           /* Launch xmgr with named-pipe support */
	   if (GraceOpen(1000000, displayIP, origDir, 0) == -1) {
	     fprintf ( stderr, "Can't run Grace. \n" );
	     exit (EXIT_FAILURE);
	   }
	   fprintf ( stderr, "Done launching Grace \n" );
           switch ( playMode ) {
             case PLAYDATA:
	         GracePrintf( "with g0" );
		 for (j=0; j<windowNum; j++ ) {
		    fprintf ( fd[j], "&\n" );
		    fclose( fd[j] );
		    sprintf( tempfile, "%s%d", filename, j );
		    GracePrintf( "read xy \"%s\"", tempfile ); 
		 }
                 break;
             case PLAYTREND: 
             case PLAYTREND60: 
		 for ( j=0; j<windowNum; j++ ) {
		   if ( multiple )
		     GracePrintf( "with g0" );
		   else
		     GracePrintf( "with g%d", j );
		   if ( dmax ) {
		     if ( dcstep == 1 )
		       fprintf ( fdM[j], "&\n" );
		     fclose( fdM[j] );
		     sprintf( tempfile, "%s%dM", filename, j );
		     if ( dcstep == 1 )
		        GracePrintf( "read xy \"%s\"", tempfile );
		     else
		        stepread(tempfile, dcstep, 0, xyType[j]);
		   }
		   if ( dmean ) {
		     if ( dcstep == 1 )
		       fprintf ( fd[j], "&\n" );
		     fclose( fd[j] );
		     sprintf( tempfile, "%s%d", filename, j );
		     if ( dcstep == 1 )
		        GracePrintf( "read xy \"%s\"", tempfile );
		     else
		        stepread(tempfile, dcstep, 1, xyType[j]);
		   }
		   if ( dmin ) {
		     if ( dcstep == 1 )
		       fprintf ( fdm[j], "&\n" );
		     fclose( fdm[j] );
		     sprintf( tempfile, "%s%dm", filename, j );
		     if ( dcstep == 1 )
		        GracePrintf( "read xy \"%s\"", tempfile );
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
	GraceFlush ();        
	//sleep(4);
	/*DataWriteStop(processID);*/
	GraceClosePipe();
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
	if (linestyle == 0 && data_no ) /* if default, set to points when breaks */
	  linestyle = 2; 
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
	GracePrintf( "background color 7" );
	switch ( windowNum ) {
	     case 1: 
	         x0[0] = 0.06; y0[0] = 0.06;
		 w = 1.18;
		 h = 0.81;
		 break;
	     case 2: 
	         x0[0] = 0.06; y0[0] = 0.06;
		 x0[1] = 0.06; y0[1] = 0.50; 
		 w = 1.18;
		 h = 0.35;
		 break;
	     case 3: 
	     case 4: 
	         x0[0] = 0.06; y0[0] = 0.06; 
		 x0[1] = 0.06; y0[1] = 0.50;
		 x0[2] = 0.68; y0[2] = 0.06; 
		 x0[3] = 0.68; y0[3] = 0.50; 
		 w = 0.56;
		 h = 0.35;
		 break;
	     case 5: 
	     case 6: 
	         x0[0] = 0.06; y0[0] = 0.06; 
		 x0[1] = 0.06; y0[1] = 0.35; 
		 x0[2] = 0.06; y0[2] = 0.64; 
		 x0[3] = 0.68; y0[3] = 0.06;
		 x0[4] = 0.68; y0[4] = 0.35; 
		 x0[5] = 0.68; y0[5] = 0.64; 
		 w = 0.56;
		 h = 0.23;
		 break;
	     case 7: 
	     case 8: 
	     case 9: 
	         x0[0] = 0.06; y0[0] = 0.06; 
		 x0[1] = 0.06; y0[1] = 0.35; 
		 x0[2] = 0.06; y0[2] = 0.64; 
		 x0[3] = 0.47; y0[3] = 0.06;
		 x0[4] = 0.47; y0[4] = 0.35; 
		 x0[5] = 0.47; y0[5] = 0.64; 
		 x0[6] = 0.88; y0[6] = 0.06; 
		 x0[7] = 0.88; y0[7] = 0.35;
		 x0[8] = 0.88; y0[8] = 0.64; 
		 w = 0.36;
		 h = 0.23;
		 break;
	     case 10: 
	     case 11: 
	     case 12: 
	         x0[0] = 0.06; y0[0] = 0.06; 
		 x0[1] = 0.06; y0[1] = 0.35; 
		 x0[2] = 0.06; y0[2] = 0.64; 
		 x0[3] = 0.37; y0[3] = 0.06;
		 x0[4] = 0.37; y0[4] = 0.35; 
		 x0[5] = 0.37; y0[5] = 0.64; 
		 x0[6] = 0.68; y0[6] = 0.06; 
		 x0[7] = 0.68; y0[7] = 0.35;
		 x0[8] = 0.68; y0[8] = 0.64; 
		 x0[9] = 0.98; y0[9] = 0.06; 
		 x0[10] = 0.98; y0[10] = 0.35;
		 x0[11] = 0.98; y0[11] = 0.64; 
		 w = 0.263;
		 h = 0.23;
		 break;
	     default: /* case 13 - 16  */
	         x0[0] = 0.06; y0[0] = 0.06; 
		 x0[1] = 0.06; y0[1] = 0.277; 
	         x0[2] = 0.06; y0[2] = 0.494; 
	         x0[3] = 0.06; y0[3] = 0.711; 
		 x0[4] = 0.37; y0[4] = 0.06; 
		 x0[5] = 0.37; y0[5] = 0.277; 
		 x0[6] = 0.37; y0[6] = 0.494; 
		 x0[7] = 0.37; y0[7] = 0.711; 
		 x0[8] = 0.68; y0[8] = 0.06; 
		 x0[9] = 0.68; y0[9] = 0.277; 
		 x0[10] = 0.68; y0[10] = 0.494;
		 x0[11] = 0.68; y0[11] = 0.711; 
		 x0[12] = 0.98; y0[12] = 0.06;
		 x0[13] = 0.98; y0[13] = 0.277; 
		 x0[14] = 0.98; y0[14] = 0.494; 
		 x0[15] = 0.98; y0[15] = 0.711;
		 w = 0.263;
		 h = 0.157;
		 break;
	}
	for ( i= 0; i<windowNum; i++ ) {
	   GracePrintf( "with g%d", i );
	   GracePrintf( "view %f, %f,%f,%f", x0[i]+XSHIFT,y0[i]+YSHIFT,x0[i]+w,y0[i]+h );
	   GracePrintf( "frame color 1" );
	   GracePrintf( "frame background color 0" );
	   GracePrintf( "frame fill on" );
	   switch ( xyType[i] ) {
	   case 2: /* Ln */
	     GracePrintf( "yaxis ticklabel prepend \"\\-Exp\"" );
	     break;
	   default: /* Linear */ 
	     GracePrintf( "yaxis ticklabel prepend \"\\-\"" );
	     break;
	   }	      
	   GracePrintf( "yaxis label \"%s\" ", chUnit[i] );
	   GracePrintf( "yaxis label layout para" );
	   GracePrintf( "yaxis label place auto" );
	   GracePrintf( "yaxis label font 4" );
	   if (windowNum > 6)
	     GracePrintf( "yaxis label char size 0.6" );
	   else
	     GracePrintf( "yaxis label char size 0.8" );
	   if (windowNum > 9)
	     GracePrintf( "subtitle size 0.5" );
	   else
	     GracePrintf( "subtitle size 0.7" );
	   GracePrintf( "subtitle font 1" );
	   GracePrintf( "subtitle color 1" );
	   /*if ( chstatus[i] == 0 )
	     GracePrintf( "subtitle color 1" );
	   else
	   GracePrintf( "subtitle color 2" );*/
	   GracePrintf( "world xmin %f", xmin );
	   GracePrintf( "world xmax %f", xmax );
	   if ( playMode == PLAYDATA ) {
	     GracePrintf( "xaxis ticklabel format decimal" );
	     GracePrintf( "xaxis ticklabel prec 0" );
	   }
	   else if ( xaxisFormat == XAXISGTS ) {
	     GracePrintf( "xaxis ticklabel format decimal" );
	     if ( width <= 2 )
	       GracePrintf( "xaxis ticklabel prec 1" );
	     else
	       GracePrintf( "xaxis ticklabel prec 0" );
	   }
	   else
	      GracePrintf( "xaxis ticklabel format yymmddhms" );
	   if ( windowNum <= 6 ) {
	      GracePrintf( "xaxis tick major %le", (xmax-xmin)/4.0 );
	      GracePrintf( "xaxis tick minor %le", (xmax-xmin)/8.0 );
	   }
	   else {
	      GracePrintf( "xaxis tick major %le", (xmax-xmin)/2.0 );
	      GracePrintf( "xaxis tick minor %le", (xmax-xmin)/4.0 );
	   }
	   if ( longauto == 0 ) {
	     GracePrintf( "world ymin %le", winYMin[i] );
	     GracePrintf( "world ymax %le", winYMax[i] );
	     GracePrintf( "yaxis tick major %le", (winYMax[i]-winYMin[i])/5.0 );
	     GracePrintf( "yaxis tick minor %le", (winYMax[i]-winYMin[i])/10.0 );
	   }
	   if (windowNum > 2)
	     GracePrintf( "xaxis ticklabel char size 0.40" );
	   else
	     GracePrintf( "xaxis ticklabel char size 0.53" );
	   GracePrintf( "xaxis tick color 1" );
	   if ( xGrid ) {
	      GracePrintf( "xaxis tick major color 7" );
	      GracePrintf( "xaxis tick major linewidth 1" );
	      GracePrintf( "xaxis tick major linestyle 1" );
	      GracePrintf( "xaxis tick minor color 7" );
	      GracePrintf( "xaxis tick minor linewidth 1" );
	      GracePrintf( "xaxis tick minor linestyle 3" );
	      GracePrintf( "xaxis tick major grid on" );
	      GracePrintf( "xaxis tick minor grid on" );
	   }
	   if (windowNum > 5)
	     GracePrintf( "yaxis ticklabel char size 0.6" );
	   else if (windowNum > 2)
	     GracePrintf( "yaxis ticklabel char size 0.7" );
	   else
	     GracePrintf( "yaxis ticklabel char size 0.8" );
	   GracePrintf( "yaxis tick color 1" );
	   if ( yGrid ) {
	      GracePrintf( "yaxis tick major color 7" );
	      GracePrintf( "yaxis tick major linewidth 1" );
	      GracePrintf( "yaxis tick major linestyle 1" );
	      GracePrintf( "yaxis tick minor color 7" );
	      GracePrintf( "yaxis tick minor linewidth 1" );
	      GracePrintf( "yaxis tick minor linestyle 3" );
	      GracePrintf( "yaxis tick major grid on" );
	      GracePrintf( "yaxis tick minor grid on" );
	   }
	   if ( xyType[i] == 3 )
	      GracePrintf( "yaxis ticklabel format exponential" );
	   else if ( xyType[i] == 1 ) {
	      GracePrintf( "g%d type logy", i );
	      GracePrintf( "yaxis ticklabel prec 5" );
	      GracePrintf( "yaxis ticklabel format exponential" );
	   }
	   else
	      GracePrintf( "yaxis ticklabel format general" );
	}

        if ( playMode == PLAYDATA ) {
	   GracePrintf( "with g0" );
	   for ( i=0; i<windowNum; i++ ) {
	      GracePrintf( "s%d color %d", i, gColor[i] );
           }
	   for ( i=1; i<windowNum; i++ ) {
	      GracePrintf( "move g0.s%d to g%d.s0", i, i );
           }
	   for ( i=0; i<windowNum; i++ ) {
	     GracePrintf( "g%d fixedpoint off", i );
	     GracePrintf( "g%d fixedpoint type 0", i );
	     GracePrintf( "g%d fixedpoint xy 0.000000, 0.000000", i );
	     GracePrintf( "g%d fixedpoint format decimal decimal", i );
	     GracePrintf( "g%d fixedpoint prec 0, 6", i );
	     if ( linestyle == 2 ) {
	       GracePrintf( "with g%d", i );
	       GracePrintf( "s0 linestyle 0" );
	       GracePrintf( "s0 symbol 2" );
	       GracePrintf( "s0 symbol size 0.2" );
	       GracePrintf( "s0 symbol fill 1" );
	     }
           }
        }
	else {
           for ( i=0; i<windowNum; i++ ) {
	      GracePrintf( "with g%d", i );
	      GracePrintf( "s0 color 2" );
	      GracePrintf( "s1 color 6" );
	      GracePrintf( "s2 color 15" );

	      if (  xaxisFormat == XAXISUTC ) {
		GracePrintf( "g%d fixedpoint off", i );
		GracePrintf( "g%d fixedpoint type 0", i );
		GracePrintf( "g%d fixedpoint xy 0.000000, 0.000000", i );
		GracePrintf( "g%d fixedpoint format mmddyyhms decimal", i );
		GracePrintf( "g%d fixedpoint prec 6, 6", i );
	      }
	      else if (  xaxisFormat == XAXISGTS ) {
		GracePrintf( "g%d fixedpoint off", i );
		GracePrintf( "g%d fixedpoint type 0", i );
		GracePrintf( "g%d fixedpoint xy 0.000000, 0.000000", i );
		GracePrintf( "g%d fixedpoint format decimal decimal", i );
		GracePrintf( "g%d fixedpoint prec 0, 6", i );
	      }
	      if (linestyle != 1) {
		if ( (linestyle == 2) || skip ) {
		  GracePrintf( "s0 linestyle 0" );
		  GracePrintf( "s1 linestyle 0" );
		  GracePrintf( "s2 linestyle 0" );
		  GracePrintf( "s0 symbol 2" );
		  GracePrintf( "s1 symbol 2" );
		  GracePrintf( "s2 symbol 2" );
		  GracePrintf( "s0 symbol size 0.2" );
		  GracePrintf( "s1 symbol size 0.2" );
		  GracePrintf( "s2 symbol size 0.2" );
		  GracePrintf( "s0 symbol fill 1" );
		  GracePrintf( "s1 symbol fill 1" );
		  GracePrintf( "s2 symbol fill 1" );
		}
	      }
	   }
	}

        for ( i=0; i<windowNum; i++ ) {
	   GracePrintf( "with g%d", i );
	   GracePrintf( "g%d on", i );
	   if ( playMode == PLAYDATA ) 
	     GracePrintf( "subtitle \"Ch %d: %s\"", chNum[i], chName[i] );
	   else
	     GracePrintf( "subtitle \"Trend Ch %d: %s\"", chNum[i], chName[i] );
	   if (longauto)
	     GracePrintf( "autoscale yaxes" );
	   GracePrintf( "clear stack" );
        }
	/** title string **/
	GracePrintf( "with string 0" );
	GracePrintf( "string loctype view" );
	GracePrintf( "string 0.2, 0.94" );
	GracePrintf( "string font 4" );
	GracePrintf( "string char size 0.9" );
	GracePrintf( "string color 1" );
        if ( playMode == PLAYDATA ) 
	   GracePrintf( "string def \"\\1 Data Display start at %s (%d seconds)\"", firsttimestring, width); 
	else if ( xaxisFormat == XAXISUTC )
	   GracePrintf( "string def \"\\1 Trend from %s to %s \"", firsttimestring,lasttimestring); 
	else {
           if ( playMode == PLAYTREND )
	      GracePrintf( "string def \"\\1 Trend %d seconds from %s to %s\"", width, firsttimestring,lasttimestring ); 
           else if ( playMode == PLAYTREND60 )
	      GracePrintf( "string def \"\\1 Trend %d minutes from %s to %s\"", width, firsttimestring,lasttimestring ); 
	}
	GracePrintf( "string on" );
        if ( playMode != PLAYDATA ) {
	  if ( dmax ) {
	    GracePrintf( "legend string 0 \"M A X\"" );
	    if ( dmean ) {
	      GracePrintf( "legend string 1 \"M E A N\"" );
	      GracePrintf( "legend string 2 \"M I N\"" );
	    }
	    else
	      GracePrintf( "legend string 1 \"M I N\"" );
	  }
	  else if ( dmean ) {
	    GracePrintf( "legend string 0 \"M E A N\"" );
	    GracePrintf( "legend string 1 \"M I N\"" );
	  }
	  else 
	    GracePrintf( "legend string 0 \"M I N\"" );
	  GracePrintf( "legend loctype view" );
	  GracePrintf( "legend 0.022, 0.97" );
	  GracePrintf( "legend color 4" );
	  GracePrintf( "legend char size 0.5" );
	  GracePrintf( "legend on" );
	}
	GracePrintf( "clear stack" );
	/*if ( windowNum > 1 )
	  GracePrintf( "with g1" );*/
	GracePrintf( "with g0" );
	GracePrintf( "redraw" );
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
        GracePrintf( "doublebuffer true" );
	GracePrintf( "background color 7" );

	GracePrintf( "with g0" ); /* main graph */
	GracePrintf( "view %f,%f,%f,%f", 0.08,0.10,0.74,0.85 );
	GracePrintf( "frame color 1" );
	GracePrintf( "frame background color 0" );
	GracePrintf( "frame fill on" );
	GracePrintf( "xaxis tick color 1" );
	GracePrintf( "yaxis tick color 1" );
	if ( xGrid ) {
	   GracePrintf( "xaxis tick major color 7" );
	   GracePrintf( "xaxis tick major linewidth 1" );
	   GracePrintf( "xaxis tick major linestyle 1" );
	   GracePrintf( "xaxis tick minor color 7" );
	   GracePrintf( "xaxis tick minor linewidth 1" );
	   GracePrintf( "xaxis tick minor linestyle 3" );
	   GracePrintf( "xaxis tick major grid on" );
	   GracePrintf( "xaxis tick minor grid on" );
	}
	if ( yGrid ) {
	   GracePrintf( "yaxis tick major color 7" );
	   GracePrintf( "yaxis tick major linewidth 1" );
	   GracePrintf( "yaxis tick major linestyle 1" );
	   GracePrintf( "yaxis tick minor color 7" );
	   GracePrintf( "yaxis tick minor linewidth 1" );
	   GracePrintf( "yaxis tick minor linestyle 3" );
	   GracePrintf( "yaxis tick major grid on" );
	   GracePrintf( "yaxis tick minor grid on" );
	}
	GracePrintf( "subtitle size 1" );
	GracePrintf( "subtitle font 1" );
	GracePrintf( "subtitle color 1" );
	GracePrintf( "world xmin %f", xmin );
	GracePrintf( "world xmax %f", xmax );
	if ( windowNum <= 6 ) {
	   GracePrintf( "xaxis tick major %le", (xmax-xmin)/4.0 );
	   GracePrintf( "xaxis tick minor %le", (xmax-xmin)/8.0 );
	}
	else {
	   GracePrintf( "xaxis tick major %le", (xmax-xmin)/2.0 );
	   GracePrintf( "xaxis tick minor %le", (xmax-xmin)/4.0 );
	}
	GracePrintf( "xaxis ticklabel char size 0.53" );
	if ( playMode == PLAYDATA ) 
	   GracePrintf( "xaxis ticklabel prec 2" );
	else if ( xaxisFormat == XAXISGTS ) {
	  GracePrintf( "xaxis ticklabel format decimal" );
	  if ( width <= 2 )
	    GracePrintf( "xaxis ticklabel prec 1" );
	  else
	    GracePrintf( "xaxis ticklabel prec 0" );
	}
	else
	  GracePrintf( "xaxis ticklabel format yymmddhms" );
	GracePrintf( "yaxis ticklabel char size 0.43" );
	GracePrintf( "yaxis ticklabel format general" );

	GracePrintf( "with g0" );
	/*GracePrintf( "g0 on" );*/
	for ( i=0; i<windowNum; i++ ) {
	   GracePrintf( "s%d color %d", i, gColor[i] );
	   GracePrintf( "legend string %d \"Ch %d:%s\"", i, chNum[i], chName[i] );
	   if ( xyType[i] == 3 )
	      GracePrintf( "yaxis ticklabel format exponential" );
	   /* if one channel is in Exp then show exp scale to all */
	   else if ( xyType[i] == 1 ) {
	      GracePrintf( "g%d type logy", i );
	      GracePrintf( "yaxis ticklabel format exponential" );
	   }
	}

	if (linestyle != 1) {
	  if ( (linestyle == 2) || skip ) {
	    for ( i=0; i<9; i++ ) {
	      GracePrintf( "s%d linestyle 0", i );
	      GracePrintf( "s%d symbol 2", i );
	      GracePrintf( "s%d symbol size 0.2", i );
	      GracePrintf( "s%d symbol fill 1", i );
	    }
	  }
	}
	GracePrintf( "s10 linestyle 3" );
	GracePrintf( "s11 linestyle 3" );
	GracePrintf( "s12 linestyle 3" );
	GracePrintf( "s13 linestyle 3" );
	GracePrintf( "s14 linestyle 3" );
	GracePrintf( "s15 linestyle 3" );
        if ( playMode == PLAYDATA ) 
	   GracePrintf( "subtitle \"Display Multiple Data start at %s (%d seconds)\"", firsttimestring, width );
	else { 
	   if ( dmin )
	      strcpy (tempstr, "Display Multiple MIN Trend");
	   else if ( dmax )
	      strcpy (tempstr, "Display Multiple MAX Trend");
	   else 
	      strcpy (tempstr, "Display Multiple MEAN Trend");
	   if ( xaxisFormat == XAXISUTC ) {
	      GracePrintf( "subtitle \"%s from %s to %s\"", tempstr, firsttimestring,lasttimestring ); 
           }
           else { /* XAXISGTS */
              if ( playMode == PLAYTREND )
		 GracePrintf( "subtitle \"%s %d seconds from %s to %s\"", tempstr, width, firsttimestring,lasttimestring ); 
	      else if ( playMode == PLAYTREND60 )
		 GracePrintf( "subtitle \"%s %d minutes from %s to %s\"", tempstr, width, firsttimestring,lasttimestring );
           }
	}
	GracePrintf( "legend loctype view" );
	GracePrintf( "legend 0.75, 0.82" );
	GracePrintf( "legend char size 0.7" );
	GracePrintf( "legend color 1" );
	GracePrintf( "legend on" );
	GracePrintf( "autoscale yaxes" );
	/*GracePrintf( "clear stack" );*/

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
    GracePrintf( "read xy \"%s\"", outfile );
    remove(infile);
    return;
}
 

/* functions for varifying trigger conditions. return 1 if true  */
/* full data - Note: the whole second of data will be displayed if one 
data satisfies the condition.
*/
int trigger_cond1(double data1[])
{
int j;
    if ( trigHow1==0 ) { /* above */
      for ( j=0; j<minRate; j++ ) {
	if ( data1[j]*slope1+offset1 > trigLev1 ) { 
	  data_yes = 1;
	  return 1;
	}
      }
    }
    else if ( trigHow1==1 ) { /* below */
      for ( j=0; j<minRate; j++ ) {
	if ( data1[j]*slope1+offset1 < trigLev1 ) {
	  data_yes = 1;
	  return 1;
	}
      }
    }
    else { /* equal to */
      for ( j=0; j<minRate; j++ ) {
	if ( data1[j]*slope1+offset1 == trigLev1 ) {
	  data_yes = 1;
	  return 1;
	}
      }
    }
    data_no = 1;
    return 0;
}


int trigger_cond2(double data1[], double data2[])
{
int cond1, cond2, j;

    for ( j=0; j<minRate; j++ ) {

      cond1 = 0;
      if ( trigHow1==0 ) { /* above */
	if ( data1[j]*slope1+offset1 > trigLev1 )
	  cond1 = 1;
      }
      else if ( trigHow1==1 ) { /* below */
	if ( data1[j]*slope1+offset1 < trigLev1 )
	  cond1 = 1;
      }
      else { /* equal to */
	if ( data1[j]*slope1+offset1 == trigLev1 )
	  cond1 = 1;
      }
      cond2 = 0;
      if ( trigHow2==0 ) { /* above */
	if ( data2[j]*slope2+offset2 > trigLev2 )
	  cond2 = 1;
      }
      else if ( trigHow2==1 ) { /* below */
	if ( data2[j]*slope2+offset2 < trigLev2 )
	  cond2 = 1;
      }
      else { /* equal to */
	if ( data2[j]*slope2+offset2 == trigLev2 )
	  cond2 = 1;
      }
      if ( trigOp == 0 ) { /* & */
	if ( cond1 && cond2 ) {
	  data_yes = 1;
	  return 1;
	}
      }
      else { /* || */
	if ( cond1 || cond2 ) {
	  data_yes = 1;
	  return 1;
	}
      }
    }    
    data_no = 1;
    return 0;
}

/* trend data */
int trigger_trendcond1(int r)
{
int j;

    if (r > 3600) r=3600; 
    if ( trigHow1==0 ) { /* above */
      for ( j=0; j<r; j++ ) {
	if ( trendsec1[j].max*slope1+offset1 > trigLev1 ) {  
	  triggered[j] =1;
	  data_yes = 1;
	}
	else {
	  triggered[j] =0;
	  data_no = 1;
	}
      }
    }
    else if ( trigHow1==1 ) { /* below */
      for ( j=0; j<r; j++ ) {
	if ( trendsec1[j].min*slope1+offset1 < trigLev1 ) {
	  triggered[j] =1;
	  data_yes = 1;
	}
	else {
	  triggered[j] =0;
	  data_no = 1;
	}
      }
    }
    return 0;
}


int trigger_trendcond2(int r)
{
int cond1, cond2, j;

    if (r > 3600) r=3600; 
    for ( j=0; j<r; j++ ) {
      cond1 = 0;
      if ( trigHow1==0 ) { /* above */
	if ( trendsec1[j].max*slope1+offset1 > trigLev1 )
	  cond1 = 1;
      }
      else if ( trigHow1==1 ) { /* below */
	if ( trendsec1[j].min*slope1+offset1 < trigLev1 )
	  cond1 = 1;
      }
      cond2 = 0;
      if ( trigHow2==0 ) { /* above */
	if ( trendsec2[j].max*slope2+offset2 > trigLev2 )
	  cond2 = 1;
      }
      else if ( trigHow2==1 ) { /* below */
	if ( trendsec2[j].min*slope2+offset2 < trigLev2 )
	  cond2 = 1;
      }
      if ( trigOp == 0 ) { /* & */
	if ( cond1 && cond2 ) {
	  triggered[j] =1;
	  data_yes = 1;
	}
	else {
	  triggered[j] =0;
	  data_no = 1;
	}
      }
      else { /* || */
	if ( cond1 || cond2 ) {
	  triggered[j] =1;
	  data_yes = 1;
	}
	else {
	  triggered[j] =0;
	  data_no = 1;
	}
      }
    }    
    return 0;
}
