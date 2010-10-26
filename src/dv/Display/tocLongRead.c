/* tocLongRead.c                                                            */
/* Toc file LongPlayBack                                                    */
/* compile with                                                            
gcc -o tocLongRead tocLongRead.c datasrv.o daqc_access.o  -L/home/hding/Try/Lib/Grace -lgrace_np -L/home/hding/Try/Lib/UTC_GPS -ltai -lm -lnsl -lsocket -lpthread

-L/home/hding/Cds/Frame/Lib/Xmgr -lacegr_np_my -L/home/hding/Cds/Frame/Lib/UTC_GPS -ltai -lm -lnsl -lsocket -lpthread                          
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
/*#include "/home/hding/Cds/Frame/Lib/Xmgr/acegr_np.h"*/

#define CHANNUM     16
#define PLAYDATA    0
#define PLAYTREND   1
#define PLAYTREND60 60
#define XAXISDATE   0
#define XAXISTOTAL  1

#define ZEROLOG         1.0e-20
#define XSHIFT          0.02
#define YSHIFT          0.01

char  chName[CHANNUM][80], chUnit[CHANNUM][80];;
int   chNum[CHANNUM];
float slope[CHANNUM], offset[CHANNUM];
int   chstatus[CHANNUM];
char  starttime[24], firsttimestring[24], lasttimestring[24];
short warning; /* for log0 case */
int   copies, startstop;

int   longauto, linestyle;
double winYMin[CHANNUM], winYMax[CHANNUM]; 
int   xyType[CHANNUM];  /* 0-linear; 1-Log; 2-Ln; 3-exp */

short   playMode, dcstep=1;
short    multiple, decimation, windowNum;
short   dmin=0, dmax=0, dmean=0;
short   xaxisFormat, xGrid, yGrid;

char    filename[100];

short   gColor[CHANNUM];
int     monthdays[13] = {31,31,28,31,30,31,30,31,31,30,31,30,31};

char    origDir[1024], displayIP[80];

void    graphout(int width, short skip);
void    graphmulti(int width, short skip);
void    stepread(char* infile, int decm, int minmax, int type);
double  juliantime(char* timest, int extrasec);
double  juliantimegps(int igps);
void    transferfile(char* infile);

int     dataRecv, hr, trend;
long    totaldata = 0, totaldata0, skip = 0, skipstatus; 
time_t  duration, gpstimest;
char    tempfile[100];
int     i, j;

int main(int argc, char *argv[])
{
int     j, sum; 
char    rdata[32];

FILE   *fd;

        printf ( "Entering tocLongRead\n" );
        strcpy(origDir, argv[2]);
        strcpy(displayIP, argv[3]);
        strcpy(filename, argv[4]);
	windowNum = atoi(argv[6]);
	trend = atoi(argv[7]); /* 0-full frame input, 1, 60-trend frame */
	for ( j=0; j<windowNum; j++ ) {
	   slope[j] = 1.0;
	   offset[j] = 0.0;
	}

	fd = fopen( argv[1], "r" );
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
	fscanf ( fd, "%s", rdata ); /* isgps */
	fscanf ( fd,"%s", rdata );
	gpstimest = atoi(rdata);
	fscanf ( fd, "%s", rdata);
	duration = atoi(rdata);
	fscanf ( fd, "%s", rdata);
	startstop = atoi(rdata);
	fscanf ( fd, "%s", rdata);
	totaldata0 = atoi(rdata);
	fscanf ( fd, "%s", rdata);
	skip = atoi(rdata);
	fclose(fd);
	totaldata = duration;

	if ( startstop == 1) {
	  gpstimest = gpstimest - duration;
	}
	DataGPStoUTC(gpstimest, firsttimestring);
	DataGPStoUTC(gpstimest+duration, lasttimestring);

	if ( multiple < 0 ) { /* multiple copies */
	   copies = atoi(argv[5]);;
	   multiple = 0;
	   windowNum = 1;
	   strcpy(chName[0], chName[copies]);
	   xyType[0] = xyType[copies];
	   chNum[0] = chNum[copies];
	}
	else
	  copies = -1;

	if ( decimation == 0 )
          switch ( trend ) {
            case 0: 
                playMode = PLAYDATA;
                break;
            case 1: 
                playMode = PLAYTREND;
                break;
            case 60: 
                playMode = PLAYTREND60;
                break;
          }	  
	else if ( decimation == 1 ) {
	  playMode = PLAYTREND;  /* play second-trend */
	  if (trend == 60)
	    playMode = PLAYTREND60;  /* play minute-trend */
	}
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

	if ( copies < 0 )
	  fprintf ( stderr, "LongPlayBack starting\n" );
	else
	  fprintf ( stderr, "LongPlayBack Ch.%d starting\n", chNum[0] );

       /* prepare graphing  */ 
       if ( playMode == PLAYTREND60 ) 
	 hr = 60;
       else
	 hr = 1;

	dataRecv = 1;
	if ( dataRecv ) {
           /* Launch xmgrace with named-pipe support 
	      this function has been rewritten for our own need*/
	   if (GraceOpen(640, displayIP, origDir, 1) == -1) {
	     fprintf ( stderr, "Can't run xmgrace. \n" );
	     exit (EXIT_FAILURE);
	   }
	   fprintf ( stderr, "Done launching xmgrace \n" );
           switch ( playMode ) {
             case PLAYDATA:
	         GracePrintf( "with g0" );
		 for (j=0; j<windowNum; j++ ) {
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
		     sprintf( tempfile, "%s%dM", filename, j );
		     if ( dcstep == 1 ) {
		       if ( xaxisFormat == XAXISDATE ) 
			 transferfile(tempfile); /* rewrite to julian time format */
		       else
			 GracePrintf( "read xy \"%s\"", tempfile );
		     }
		     else
		        stepread(tempfile, dcstep, 0, xyType[j]);
		   }
		   if ( dmean ) {
		     sprintf( tempfile, "%s%d", filename, j );
		     if ( dcstep == 1 ) {
		       if ( xaxisFormat == XAXISDATE ) 
			 transferfile(tempfile);
		       else
			 GracePrintf( "read xy \"%s\"", tempfile );
		     }
		     else
		        stepread(tempfile, dcstep, 1, xyType[j]);
		   }
		   if ( dmin ) {
		     sprintf( tempfile, "%s%dm", filename, j );
		     if ( dcstep == 1 ) {
		       if ( xaxisFormat == XAXISDATE ) 
			 transferfile(tempfile);
		       else
			 GracePrintf( "read xy \"%s\"", tempfile );
		     }
		     else
		        stepread(tempfile, dcstep, 2, xyType[j]);
		   }
		 }
                 break;
             default: 
                 break;
           }
	   
           if ( playMode == PLAYDATA ) 
	      fprintf ( stderr,"totla data skipped: %d second(s)\n", skip );
	   else {
	     if ( copies < 0 )
	       fprintf ( stderr,"totla data skipped: %d second(s)\n", skip );
	     else
	       fprintf ( stderr,"Ch.%d totla data skipped: %d second(s)\n", chNum[0], skip );
	   }
           if ( playMode == PLAYTREND60 ) {
	     if ( copies < 0 )
	       fprintf ( stderr, "LongPlayBack: total %9.1f minutes (%d seconds) of data displayed\n", (float)totaldata0/hr, totaldata0 );
	     else
	       fprintf ( stderr, "LongPlayBack Ch.%d: total %9.1f minutes (%d seconds) of data displayed\n", chNum[0], (float)totaldata0/hr, totaldata0 );
	   }
	   else {
	     if ( copies < 0 )
	       fprintf ( stderr, "LongPlayBack: total %d seconds of data displayed\n", totaldata0 );
	     else
	       fprintf ( stderr, "LongPlayBack Ch.%d: total %d seconds of data displayed\n", chNum[0], totaldata0 );
	   }

	   if ( !multiple ) {
	     if ( skip > 0 )
	       graphout(totaldata, 1);
	     else
	       graphout(totaldata, 0);
	   }
	   else {
	     if ( skip > 0 )
	       graphmulti(totaldata, 1);
	     else
	       graphmulti(totaldata, 0);
	   }
	}
	else {
	   if ( copies < 0 )
	     fprintf ( stderr, "LongPlayBack: no data received\n" );
	   else
	     fprintf ( stderr, "LongPlayBack Ch.%d: no data received\n", chNum[0] );
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
	GraceFlush();
	//sleep(4);
	GraceClosePipe();  /* close pipe */
	DataQuit();
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
        if ( playMode == PLAYDATA || xaxisFormat == XAXISTOTAL ) {
	  xmin = gpstimest*1.0;
	  xmax = xmin + width;
	}
	else {
	   xmin = juliantime(firsttimestring, 0);
	   xmax = juliantime(lasttimestring, 0);
	}
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
	   if ( playMode == PLAYDATA || xaxisFormat == XAXISTOTAL ) {
	     GracePrintf( "xaxis ticklabel format decimal" );
	     GracePrintf( "xaxis ticklabel prec 4" );
	   }
	   else {
	      GracePrintf( "xaxis ticklabel format yymmddhms" );
	   }
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
	   GracePrintf( "yaxis ticklabel char size 0.80" );
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
	      GracePrintf( "g%d fixedpoint prec 12, 2", i );
           }
	   for ( i=1; i<windowNum; i++ ) {
	      GracePrintf( "move g0.s%d to g%d.s0", i, i );
           }
        }
	else {
           for ( i=0; i<windowNum; i++ ) {
	      GracePrintf( "with g%d", i );
	      GracePrintf( "s0 color 2" );
	      GracePrintf( "s1 color 6" );
	      GracePrintf( "s2 color 15" );
	      if (  xaxisFormat == XAXISDATE ) {
		GracePrintf( "g%d fixedpoint off", i );
		GracePrintf( "g%d fixedpoint type 0", i );
		GracePrintf( "g%d fixedpoint xy 0.000000, 0.000000", i );
		GracePrintf( "g%d fixedpoint format mmddyyhms decimal", i );
		GracePrintf( "g%d fixedpoint prec 6, 6", i );
	      }
	   }
	}
	for ( i=0; i<windowNum; i++ ) {
	  if (linestyle != 1) {
	    if ( (linestyle == 2) || skip ) {
	      GracePrintf( "with g%d", i );
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
	else if ( xaxisFormat == XAXISDATE )
	   GracePrintf( "string def \"\\1 Trend Data from %s to %s \"", firsttimestring,lasttimestring); 
	else {
           if ( playMode == PLAYTREND )
	      GracePrintf( "string def \"\\1 Trend Data  %d seconds from %s to %s\"", width, firsttimestring,lasttimestring ); 
           else if ( playMode == PLAYTREND60 )
	      GracePrintf( "string def \"\\1 Trend Data  %d minutes from %s to %s\"", (int)(width/60), firsttimestring,lasttimestring ); 
	}
	GracePrintf( "string on" );
	GracePrintf( "with g0" );
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
	  GracePrintf( "legend 0.04, 0.98" );
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
        else if ( xaxisFormat == XAXISTOTAL ) {
	   xmin = 0.0;
           xmax = (float)width;
        }
	else {
	   xmin = juliantime(firsttimestring, 0);
	   xmax = juliantime(lasttimestring, 0);
	}
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
	else if ( xaxisFormat == XAXISTOTAL ) {
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
	   if ( xaxisFormat == XAXISDATE ) {
	      GracePrintf( "subtitle \"%s from %s to %s\"", tempstr, firsttimestring,lasttimestring ); 
           }
           else { /* XAXISTOTAL */
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

/* convert time string to julian time */
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




/* read data file into xmgr with a decimation 
   minmax = 0 (max), 1 (mean), 2 (min)
*/
void stepread(char* infile, int decm, int minmax, int type)
{
char   outfile[100], line[80], tempst[80];
FILE   *fpw, *fpr;
double x, y, yy[61], jul; 
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
	if (xaxisFormat == XAXISDATE) {
	  jul = juliantimegps(x);
	  fprintf ( fpw, "%2f\t%le\n", jul, y );
	}
	else 
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


/* transfer xy file to julianformat and output to xmgr */
void transferfile(char* infile)
{ 
char   outfile[100], line[80];
FILE   *fpw, *fpr;
double xx, yy, jul;

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
    while (fgets ( line, 80, fpr ) != NULL) {
      sscanf ( line, "%lf %le\n", &xx, &yy);
      jul = juliantimegps(xx);
      fprintf ( fpw, "%2f\t%le\n", jul, yy );
    }
    fprintf ( fpw, "&\n" );
    fclose(fpw);
    fclose(fpr);
    GracePrintf( "read xy \"%s\"", outfile );     
    remove(infile);
    return;
}
		      
