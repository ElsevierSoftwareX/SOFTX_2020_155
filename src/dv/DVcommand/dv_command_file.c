/* dv_command.c                                                           */
/* Standalone PlayBack                                                    */
/* compile with:
gcc -o dv_command_file dv_command_file.c datasrv.o daqc_access.o -L/home/hding/Try/Lib/UTC_GPS -ltai -lm -lnsl -lsocket -lpthread

./dv_agr 198.129.208.138 0 playset1 745013504 60 agr.agr

./xmgrace agr.agr -hardcopy -printfile agr.ps

*/

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
float slope[CHANNUM], offset[CHANNUM];
int   chstatus[CHANNUM];
char  timestring[24];
char  starttime[24], firsttimestring[24], lasttimestring[24];
short warning; /* for log0 case */
int   startstop=0, isgps=1;

int   longauto, linestyle;
double winYMin[CHANNUM], winYMax[CHANNUM]; 
int   xyType[CHANNUM] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};  
      /* 0-linear; 1-Log; 2-Ln; 3-exp */


unsigned long processID=0;
short   playMode, dcstep=1;
short   finished, decimation, windowNum;
short   dmin=0, dmax=0, dmean=0;
short   xaxisFormat, xGrid=1, yGrid=1;

char    filename[100];

short   gColor[CHANNUM] = {2,3,4,5,6,8,9,10,11,12,13,14,15,2,3,4};
int     monthdays[13] = {31,31,28,31,30,31,30,31,31,30,31,30,31};

size_t  sizeDouble, sizeDTrend;
char    origDir[1024];

time_t  duration, gpstimest;

char    agrfile[200], psfile[200];
FILE   *fagr;

void*   read_block();
void    graphout(int width, short skip);
void    stepread(char* infile, int decm, int minmax, int type);
double  juliantime(char* timest, int extrasec);
double  juliantimegps(int igps);
void    timeinstring(char* timest, int extrasec, char* timestout );
int     difftimestrings(char* timest1, char* timest2, int hr);
int     read_xy(char* inputfile, FILE *fwrite);

int main(int argc, char *argv[])
{
int     serverPort, userPort;
char    serverIP[80];

int     j, sum; 
char    rdata[32], tmpstr[200];

FILE   *fd;

        if (argc != 7 && argc != 8) {
	  fprintf ( stderr, "Error: Command-line Dataviewer needs six (or seven) arguments: Server IP, Server Port, Input File, Starting time (GPS), Duration (in seconds), output ps file name, optional log scale bitmask. Only %d received. Exit.\n", argc-1 );
	  exit(0);
	} 
        if (argc == 8) {
	  int i;
	  unsigned int mask = atoi(argv[7]);
	  for (i = 0; i < 16; i++)
	    xyType[i] = (mask & (1 << i)) >> i;
 	}
	fprintf ( stderr, "Command-line Dataviewer Playback Starting...\n" );
	sizeDouble = sizeof(double);
	sizeDTrend = sizeof(struct DTrend);

	if ( getenv("DVPATH") == NULL ) {
	  fprintf ( stderr, "Error: $DVPATH is not set. Exit.\n" );
	  exit(0);
	}
	sprintf ( origDir,"%s/", getenv("DVPATH") );
	printf ( "$DVPATH=%s\n", origDir );

        finished = 0;
	fd = fopen( argv[3], "r" );
	if ( fd == NULL ) {
	  fprintf ( stderr, "Error: Can't open reading file %s. Exit.\n", argv[3] );
	  return -1;
	}


	fscanf ( fd,"%s", rdata );
	windowNum = atoi(rdata);
	fscanf ( fd,"%s", rdata );
	longauto = atoi(rdata);
	for ( j=0; j<windowNum; j++ ) {
	   fscanf ( fd,"%s", chName[j] );
	   fscanf ( fd,"%s", chUnit[j] );
	   if ( longauto == 0 ) { /* not auto, record y settings */
	     fscanf ( fd,"%s", rdata );
	     winYMin[j] = atof(rdata);
	     fscanf ( fd,"%s", rdata );
	     winYMax[j] = atof(rdata);
	   }
	   printf("channel `%s' min=%f max=%f\n", chName[j], winYMin[j], winYMax[j]);
	}
	fscanf( fd, "%s", rdata );
	xaxisFormat = atoi(rdata);
	fscanf( fd, "%s", rdata );
	decimation = atoi(rdata);
	fscanf( fd, "%s", rdata );
	dmean = atoi(rdata);
	fscanf( fd, "%s", rdata );
	dmax = atoi(rdata);
	fscanf( fd, "%s", rdata );
	dmin = atoi(rdata);
	fclose(fd);

	sprintf(filename, "/tmp/%ddvcommand", getpid()); 
	for ( j=0; j<windowNum; j++ ) {
	   slope[j] = 1.0;
	   offset[j] = 0.0;
	}

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

	gpstimest = atoi(argv[4]);
	duration = atoi(argv[5]);
	strcpy(psfile, argv[6]);
	sprintf(agrfile, "/tmp/%dagr.agr", getpid()); 

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
	fprintf ( stderr,"Connecting to Server %s-%d\n", serverIP, serverPort );
	if ( DataConnect(serverIP, serverPort, userPort, read_block) != 0 ) {
	  fprintf ( stderr, "PlayBack: can't connect to the data server %s-%d. Make sure the server is running.\n", serverIP, serverPort );
	  exit(1);
	}


	for ( j=0; j<windowNum; j++ ) {
	   DataChanAdd(chName[j], 0);
        }

        switch ( playMode ) {
          case PLAYDATA: 
	      if ( strcmp(starttime, "0-0-0-0-0-0") == 0 ) {
		 fprintf ( stderr, "PlayBack Error: no starting time\n"  );
		 exit(0);
	      }
	      else {
		 processID = DataWrite(starttime, duration, isgps);
		 fprintf ( stderr, "PlayBack time %s, duration %d\n", starttime, duration );
	      }
              break;
          case PLAYTREND: 
	      processID = DataWriteTrend(starttime, duration, 1, isgps);
	      fprintf ( stderr, "Request second-trend: time %s, duration %d\n", starttime, duration );
              break;
          case PLAYTREND60: 
	      processID = DataWriteTrend(starttime, duration, 60, isgps);
	      fprintf ( stderr, "Request minute-trend: time %s, duration %d\n", starttime, duration );
              break;
          default: 
              break;
        }
	
	if ( processID == -1 ) {
	   finished = 1;
	   fprintf ( stderr,"No data output.\n" );
           return -1;
	}
	while ( !finished ) ;
	sleep(2);

        /* generate .ps file  */
	sprintf ( tmpstr, "%sxmgrace", origDir );
	if ( fork() == 0 ) {
	  if (longauto)
	    execlp(tmpstr,"xmgrace",agrfile,"-hardcopy","-printfile",psfile,(char*)NULL);
	  else
	    execlp(tmpstr,"xmgrace", "-autoscale", "none", agrfile,"-hardcopy","-printfile",psfile,(char*)NULL);
	  perror("Error in writing .ps file");
	  exit(0);
	}
	else {
	  wait(0);
	}
	fprintf ( stderr,"postscript file %s generated.\n", psfile );
	remove (agrfile);
	
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
	      fprintf ( stderr, "LONG: DataRead = %d\n", bytercv );
              DataReadStop();
	      break;
	   }
	   else if ( bytercv == 0 ) {
	      fprintf ( stderr, "LONG: trailer received\n" );
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
	      fprintf ( stderr, "           WARNING: data skipped: %d sec.\n", ngps - ngps0 - totaldata );
	      skipstatus = 1;
	   }
	   else
	      skipstatus = 0;
	   fprintf ( stderr, "[#%d] data read: %s\n", processID, timestring );
           if ( playMode == PLAYDATA ) {
	      bsec = DataTrendLength();
	      printf ( "            Block length =%d seconds\n", bsec );
	      hr = 1;
	      for ( j=0; j<windowNum; j++ ) {
	      for ( bc=0; bc<bsec; bc++ ) {
		 /* Find data in the desired data channels */
		 sRate[j] = DataGetCh(chName[j], chData, bc, 0);
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
           /* open and write to agr file */
	   fagr = fopen( agrfile, "w" );
	   if ( fagr == NULL ) {
	     fprintf ( stderr, "Can't open writing filr %s. Exit.\n", agrfile );
	     exit(1);
	   }
           switch ( playMode ) {
             case PLAYDATA:
	         fprintf( fagr, "@with g0\n" );
		 for (j=0; j<windowNum; j++ ) {
		    fprintf ( fd[j], "&\n" );
		    fclose( fd[j] );
		    sprintf( tempfile, "%s%d", filename, j );
		    read_xy(tempfile, fagr);
		 }
                 break;
             case PLAYTREND: 
             case PLAYTREND60: 
		 for ( j=0; j<windowNum; j++ ) {
		   fprintf( fagr, "@with g%d\n", j );
		   if ( dmax ) {
		     if ( dcstep == 1 )
		       fprintf ( fdM[j], "&\n" );
		     fclose( fdM[j] );
		     sprintf( tempfile, "%s%dM", filename, j );
		     if ( dcstep == 1 )
		       read_xy(tempfile, fagr);
		     else
		       stepread(tempfile, dcstep, 0, xyType[j]);
		   }
		   if ( dmean ) {
		     if ( dcstep == 1 )
		       fprintf ( fd[j], "&\n" );
		     fclose( fd[j] );
		     sprintf( tempfile, "%s%d", filename, j );
		     if ( dcstep == 1 )
		       read_xy(tempfile, fagr);
		     else
		       stepread(tempfile, dcstep, 1, xyType[j]);
		   }
		   if ( dmin ) {
		     if ( dcstep == 1 )
		       fprintf ( fdm[j], "&\n" );
		     fclose( fdm[j] );
		     sprintf( tempfile, "%s%dm", filename, j );
		     if ( dcstep == 1 )
		       read_xy(tempfile, fagr);
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
	   else 
	      fprintf ( stderr,"totla data skipped: %d second(s)\n", skip );
           if ( playMode == PLAYTREND60 ) {
	      fprintf ( stderr, "PlayBack: total %9.1f minutes (%d seconds) of data saved\n", (float)totaldata/hr, totaldata );
	      timeinstring( timestring, (irate-1)*60, lasttimestring );
	   }
	   else {
	     fprintf ( stderr, "PlayBack: total %d seconds of data saved\n", totaldata );
	     timeinstring( timestring, irate-1, lasttimestring );
	   }
	   if ( skip > 0 )
	     graphout(totaldata/hr, 1);
	   else
	     graphout(totaldata/hr, 0);
	}
	else {
	   fprintf ( stderr, "PlayBack: no data received\n" );
	   finished = 1;
	   fflush(stdout);
	   return NULL;
	}
        if ( warning ) {
	   fprintf ( stderr,"Warning: 0 as input of log, put 1.0e-20 instead\n"  );
	}

	/* Finishing */
	fflush(stdout);
	sleep(4);
	/*DataWriteStop(processID);*/
	fclose(fagr);
	DataQuit();
	finished = 1;
	/* remove temp data files */
	for (j=0; j<windowNum; j++ ) {
	  if ( playMode == PLAYDATA ) {
	    sprintf( tempfile, "%s%d", filename, j );
	    remove (tempfile);
	  }
	  else {
	    if ( dmax ) {
	      sprintf( tempfile, "%s%dM", filename, j );
	      remove (tempfile);
	    }
	    if ( dmin ) {
	      sprintf( tempfile, "%s%dm", filename, j );
	      remove (tempfile);
	    }
	    if ( dmean ) {
	      sprintf( tempfile, "%s%d", filename, j );
	      remove (tempfile);
	    }
	  }
	}
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
	fprintf( fagr, "@background color 7\n" );
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
	   fprintf( fagr, "@with g%d\n", i );
	   fprintf( fagr, "@view %f, %f,%f,%f\n", x0[i]+XSHIFT,y0[i]+YSHIFT,x0[i]+w,y0[i]+h );
	   fprintf( fagr, "@frame color 1\n" );
	   fprintf( fagr, "@frame background color 0\n" );
	   fprintf( fagr, "@frame fill on\n" );
	   switch ( xyType[i] ) {
	   case 2: /* Ln */
	     fprintf( fagr, "@yaxis ticklabel prepend \"\\-Exp\"\n" );
	     break;
	   default: /* Linear */ 
	     fprintf( fagr, "@yaxis ticklabel prepend \"\\-\"\n" );
	     break;
	   }	      
	   switch ( xyType[i] ) {
#if 0
	   case 1: /* Log */
	      fprintf( fagr, "@yaxis label \"log\" \n" );
	      break;
#endif
	   default:
	      fprintf( fagr, "@yaxis label \"%s\" \n", chUnit[i] );
	      break;
 	   }
	   fprintf( fagr, "@yaxis label layout para\n" );
	   fprintf( fagr, "@yaxis label place auto\n" );
	   fprintf( fagr, "@yaxis label font 4\n" );
	   if (windowNum > 6)
	     fprintf( fagr, "@yaxis label char size 0.6\n" );
	   else
	     fprintf( fagr, "@yaxis label char size 0.8\n" );
	   if (windowNum > 9)
	     fprintf( fagr, "@subtitle size 0.5\n" );
	   else
	     fprintf( fagr, "@subtitle size 0.7\n" );
	   fprintf( fagr, "@subtitle font 1\n" );
	   fprintf( fagr, "@subtitle color 1\n" );
	   fprintf( fagr, "@world xmin %f\n", xmin );
	   fprintf( fagr, "@world xmax %f\n", xmax );
	   if ( playMode == PLAYDATA ) {
	     fprintf( fagr, "@xaxis ticklabel format decimal\n" );
	     fprintf( fagr, "@xaxis ticklabel prec 0\n" );
	   }
	   else if ( xaxisFormat == XAXISGTS ) {
	     fprintf( fagr, "@xaxis ticklabel format decimal\n" );
	     if ( width <= 2 )
	       fprintf( fagr, "@xaxis ticklabel prec 1\n" );
	     else
	       fprintf( fagr, "@xaxis ticklabel prec 0\n" );
	   }
	   else
	      fprintf( fagr, "@xaxis ticklabel format yymmddhms\n" );
	   if ( windowNum <= 6 ) {
	      fprintf( fagr, "@xaxis tick major %le\n", (xmax-xmin)/4.0 );
	      fprintf( fagr, "@xaxis tick minor %le\n", (xmax-xmin)/8.0 );
	   }
	   else {
	      fprintf( fagr, "@xaxis tick major %le\n", (xmax-xmin)/2.0 );
	      fprintf( fagr, "@xaxis tick minor %le\n", (xmax-xmin)/4.0 );
	   }
	   if ( longauto == 0 ) {
	     fprintf( fagr, "@world ymin %le\n", winYMin[i] );
	     fprintf( fagr, "@world ymax %le\n", winYMax[i] );
	     fprintf( fagr, "@yaxis tick major %le\n", (winYMax[i]-winYMin[i])/5.0 );
	     fprintf( fagr, "@yaxis tick minor %le\n", (winYMax[i]-winYMin[i])/10.0 );
	   }
	   if (windowNum > 2)
	     fprintf( fagr, "@xaxis ticklabel char size 0.40\n" );
	   else
	     fprintf( fagr, "@xaxis ticklabel char size 0.53\n" );
	   fprintf( fagr, "@xaxis tick color 1\n" );
	   if ( xGrid ) {
	      fprintf( fagr, "@xaxis tick major color 7\n" );
	      fprintf( fagr, "@xaxis tick major linewidth 1\n" );
	      fprintf( fagr, "@xaxis tick major linestyle 1\n" );
	      fprintf( fagr, "@xaxis tick minor color 7\n" );
	      fprintf( fagr, "@xaxis tick minor linewidth 1\n" );
	      fprintf( fagr, "@xaxis tick minor linestyle 3\n" );
	      fprintf( fagr, "@xaxis tick major grid on\n" );
	      fprintf( fagr, "@xaxis tick minor grid on\n" );
	   }
	   if (windowNum > 5)
	     fprintf( fagr, "@yaxis ticklabel char size 0.6\n" );
	   else if (windowNum > 2)
	     fprintf( fagr, "@yaxis ticklabel char size 0.7\n" );
	   else
	     fprintf( fagr, "@yaxis ticklabel char size 0.8\n" );
	   fprintf( fagr, "@yaxis tick color 1\n" );
	   if ( yGrid ) {
	      fprintf( fagr, "@yaxis tick major color 7\n" );
	      fprintf( fagr, "@yaxis tick major linewidth 1\n" );
	      fprintf( fagr, "@yaxis tick major linestyle 1\n" );
	      fprintf( fagr, "@yaxis tick minor color 7\n" );
	      fprintf( fagr, "@yaxis tick minor linewidth 1\n" );
	      fprintf( fagr, "@yaxis tick minor linestyle 3\n" );
	      fprintf( fagr, "@yaxis tick major grid on\n" );
	      fprintf( fagr, "@yaxis tick minor grid on\n" );
	   }
	   if ( xyType[i] == 3 )
	      fprintf( fagr, "@yaxis ticklabel format exponential\n" );
	   else if ( xyType[i] == 1 ) {
	      fprintf( fagr, "@g%d type logy\n", i );
	      fprintf( fagr, "@yaxis ticklabel prec 5\n" );
	      fprintf( fagr, "@yaxis ticklabel format general\n" );
	   }
	   else
	      fprintf( fagr, "@yaxis ticklabel format general\n" );
	}

        if ( playMode == PLAYDATA ) {
	   fprintf( fagr, "@with g0\n" );
	   for ( i=0; i<windowNum; i++ ) {
	      fprintf( fagr, "@s%d color %d\n", i, gColor[i] );
           }
	   for ( i=1; i<windowNum; i++ ) {
	      fprintf( fagr, "@move g0.s%d to g%d.s0\n", i, i );
           }
	   for ( i=0; i<windowNum; i++ ) {
	     fprintf( fagr, "@g%d fixedpoint off\n", i );
	     fprintf( fagr, "@g%d fixedpoint type 0\n", i );
	     fprintf( fagr, "@g%d fixedpoint xy 0.000000, 0.000000\n", i );
	     fprintf( fagr, "@g%d fixedpoint format decimal decimal\n", i );
	     fprintf( fagr, "@g%d fixedpoint prec 0, 6\n", i );
           }
        }
	else {
           for ( i=0; i<windowNum; i++ ) {
	      fprintf( fagr, "@with g%d\n", i );
	      fprintf( fagr, "@s0 color 2\n" );
	      fprintf( fagr, "@s1 color 6\n" );
	      fprintf( fagr, "@s2 color 15\n" );

	      if (  xaxisFormat == XAXISUTC ) {
		fprintf( fagr, "@g%d fixedpoint off\n", i );
		fprintf( fagr, "@g%d fixedpoint type 0\n", i );
		fprintf( fagr, "@g%d fixedpoint xy 0.000000, 0.000000\n", i );
		fprintf( fagr, "@g%d fixedpoint format mmddyyhms decimal\n", i );
		fprintf( fagr, "@g%d fixedpoint prec 6, 6\n", i );
	      }
	      else if (  xaxisFormat == XAXISGTS ) {
		fprintf( fagr, "@g%d fixedpoint off\n", i );
		fprintf( fagr, "@g%d fixedpoint type 0\n", i );
		fprintf( fagr, "@g%d fixedpoint xy 0.000000, 0.000000\n", i );
		fprintf( fagr, "@g%d fixedpoint format decimal decimal\n", i );
		fprintf( fagr, "@g%d fixedpoint prec 0, 6\n", i );
	      }
	      if (linestyle != 1) {
		if ( (linestyle == 2) || skip ) {
		  fprintf( fagr, "@s0 linestyle 0\n" );
		  fprintf( fagr, "@s1 linestyle 0\n" );
		  fprintf( fagr, "@s2 linestyle 0\n" );
		  fprintf( fagr, "@s0 symbol 2\n" );
		  fprintf( fagr, "@s1 symbol 2\n" );
		  fprintf( fagr, "@s2 symbol 2\n" );
		  fprintf( fagr, "@s0 symbol size 0.2\n" );
		  fprintf( fagr, "@s1 symbol size 0.2\n" );
		  fprintf( fagr, "@s2 symbol size 0.2\n" );
		  fprintf( fagr, "@s0 symbol fill 1\n" );
		  fprintf( fagr, "@s1 symbol fill 1\n" );
		  fprintf( fagr, "@s2 symbol fill 1\n" );
		}
	      }
	   }
	}

        for ( i=0; i<windowNum; i++ ) {
	   fprintf( fagr, "@with g%d\n", i );
	   fprintf( fagr, "@g%d on\n", i );
	   if ( playMode == PLAYDATA ) 
	     fprintf( fagr, "@subtitle \"Ch %d: %s\"\n", i+1, chName[i] );
	   else
	     fprintf( fagr, "@subtitle \"Trend Ch %d: %s\"\n", i+1, chName[i] );
	   if (longauto) {
	     fprintf( fagr, "@autoscale yaxes\n" );
	     printf("Autoscaling channel %d\n", i);
	   }
	   fprintf( fagr, "@clear stack\n" );
        }
	/** title string **/
	fprintf( fagr, "@with string 0\n" );
	fprintf( fagr, "@string loctype view\n" );
	fprintf( fagr, "@string 0.2, 0.94\n" );
	fprintf( fagr, "@string font 4\n" );
	fprintf( fagr, "@string char size 0.9\n" );
	fprintf( fagr, "@string color 1\n" );
        if ( playMode == PLAYDATA ) 
	   fprintf( fagr, "@string def \"\\1 Data Display start at %s (%d seconds)\"\n", firsttimestring, width); 
	else if ( xaxisFormat == XAXISUTC )
	   fprintf( fagr, "@string def \"\\1 Actual Trend Data available from %s to %s \"\n", firsttimestring,lasttimestring); 
	else {
           if ( playMode == PLAYTREND )
	      fprintf( fagr, "@string def \"\\1 Actual Trend Data available  %d seconds from %s to %s\"\n", width, firsttimestring,lasttimestring ); 
           else if ( playMode == PLAYTREND60 )
	      fprintf( fagr, "@string def \"\\1 Actual Trend Data available  %d minutes from %s to %s\"\n", width, firsttimestring,lasttimestring ); 
	}
	fprintf( fagr, "@string on\n" );
        if ( playMode != PLAYDATA ) {
	  if ( dmax ) {
	    fprintf( fagr, "@legend string 0 \"M A X\"\n" );
	    if ( dmean ) {
	      fprintf( fagr, "@legend string 1 \"M E A N\"\n" );
	      fprintf( fagr, "@legend string 2 \"M I N\"\n" );
	    }
	    else
	      fprintf( fagr, "@legend string 1 \"M I N\"\n" );
	  }
	  else if ( dmean ) {
	    fprintf( fagr, "@legend string 0 \"M E A N\"\n" );
	    fprintf( fagr, "@legend string 1 \"M I N\"\n" );
	  }
	  else 
	    fprintf( fagr, "@legend string 0 \"M I N\"\n" );
	  fprintf( fagr, "@legend loctype view\n" );
	  fprintf( fagr, "@legend 0.022, 0.97\n" );
	  fprintf( fagr, "@legend color 4\n" );
	  fprintf( fagr, "@legend char size 0.5\n" );
	  fprintf( fagr, "@legend on\n" );
	}
	fprintf( fagr, "@clear stack\n" );
	fprintf( fagr, "@with g0\n" );
	fprintf( fagr, "@redraw\n" );
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
    read_xy(outfile, fagr);
    remove(infile);
    return;
}
 

/*  Replacement of
    fprintf( fagr, "@read xy \"%s\"\n", inputfile );
*/
int read_xy(char* inputfile, FILE *fwrite)
{
FILE *fw;
char tline[200];

    fw = fopen( inputfile, "r" );
    if ( fw == NULL ) {
      printf ( "Error: can't open data file: %s. Exit.\n", inputfile);
      exit(-3);
    }
    while ( fgets(tline, 200, fw) != NULL ) {
      fprintf( fwrite, "%s", tline ); 
    }
    return 0;
}
