/* trendMat.c  */
/* Compile with 
   gcc -o trendMat trendMat.c Lib/datasrv.o Lib/daqc_access.o decimate.o -L/home/hding/Cds/Frame/Lib/UTC_GPS -ltai -lsocket -lpthread 
*/
#include "Lib/datasrv.h"
#include "decimate.h"
#include "mexversion.h"

#define CHANNUM      16
#define ONLINE       0
#define OFFLINE      1
#define PLAYTREND    1
#define PLAYTREND60  60

double  *sData1[CHANNUM], *sData2[CHANNUM], *sData3[CHANNUM];
long    acntr = 0;

char   timestring[24], starttime[24];
char   chName[CHANNUM][64], filename[100];
int    chNum, duration, Dur, dataRecv=0,chstatus[CHANNUM] ;
float  slope[CHANNUM], offset[CHANNUM];
short  playMode, /*1 or 60 */ trendMode; /* 0 - 3 */
short  finished = 0, bi=0, online;
double  *xTick;
unsigned long processID = 0;
size_t sizeF, sizeDTrend;
struct DTrend  *trendsec; 

/* Data-receiving thread prototype */
void* read_data();

int main(int argc, char *argv[])
{
int   LISTENER_PORT=7000, DAQD_PORT_NUM=0; 
char  DAQD_HOST[80], temp[24], ch[2];
int   i, j;
short same=1;

       fprintf(stderr, "DataGetTrend: starting connection to Server\n");
       chNum = atoi(argv[1]);
       for ( j=0; j<chNum; j++ ) {
	  strcpy(chName[j], argv[j+2] );
       }
       strcpy(starttime, argv[chNum+2] );
       duration = atoi(argv[chNum+3]);
       strcpy(DAQD_HOST, argv[chNum+4]);
       DAQD_PORT_NUM = atoi(argv[chNum+5]);
       playMode = atoi(argv[chNum+6]);
       trendMode = atoi(argv[chNum+7]);
       strcpy(filename, argv[chNum+8]);
       if ( argc >= chNum+10 ) {
	 bi = atoi(argv[chNum+9]);
       }

       if ( DataConnect(DAQD_HOST, DAQD_PORT_NUM, LISTENER_PORT, read_data) != 0 ) {
	  fprintf ( stderr,"connection to Server %s failed\n", DAQD_HOST ); 
	  exit(1);
       }
       if ( strcmp(starttime, "0") == 0 || strcmp(starttime, "") == 0 ) {
	 playMode = 1;
	 online = ONLINE;
       }
       else
	 online = OFFLINE;
       Dur = (int)(duration/playMode) + 1;
       sizeDTrend = sizeof(struct DTrend);
       xTick = (double*)malloc(sizeof(double) * Dur);
       if ( xTick == NULL ) {
	 fprintf (stderr, "Not enough memory. No output.\n" );
	 return;;
       }
       switch ( trendMode ) {
         case 0: /* all */
	   for ( j=0; j<chNum; j++ ) {
	     sData1[j] = (double*)malloc(sizeof(double)*Dur);
	     sData2[j] = (double*)malloc(sizeof(double)*Dur);
	     sData3[j] = (double*)malloc(sizeof(double)*Dur);
	     if ( sData1[j] == NULL || sData2[j] == NULL || sData3[j] == NULL ) {
	       fprintf (stderr, "Not enough memory. No output.\n" );
	       exit(1);
	     }
	   }
	   break;
         case 1: /* min */
	   for ( j=0; j<chNum; j++ ) {
	     sData1[j] = (double*)malloc(sizeof(double)*Dur);
	     if ( sData1[j] == NULL) {
	       fprintf (stderr, "Not enough memory. No output.\n" );
	       exit(1);
	     }
	   }
	   break;
         case 2: /* mean */
	   for ( j=0; j<chNum; j++ ) {
	     sData2[j] = (double*)malloc(sizeof(double)*Dur);
	     if ( sData2[j] == NULL) {
	       fprintf (stderr, "Not enough memory. No output.\n" );
	       exit(1);
	     }
	   }
	   break;
         case 3: /* max */
	   for ( j=0; j<chNum; j++ ) {
	     sData3[j] = (double*)malloc(sizeof(double)*Dur);
	     if ( sData3[j] == NULL) {
	       fprintf (stderr, "Not enough memory. No output.\n" );
	       exit(1);
	     }
	   }
	   break;
       }       
              
       for ( j=0; j<chNum; j++ ) {
	 DataChanAdd(chName[j], 0);
       }
       if ( online == ONLINE )
	 processID = DataWriteTrendRealtime();
       else
	 processID = DataWriteTrend(starttime, duration, playMode, 0);
       if ( processID == -1 ) {
	 fprintf (stderr, "DataGetTrend: no output file has been generated.\n" );
	 return 0;
       }
       else
	 fprintf (stderr, "passing %d\n", processID );
       while ( !finished );
       if ( online == ONLINE )
	 DataWriteStop(processID);
       while ( finished != 1 );       
       sleep(3);
       DataQuit();
       if ( dataRecv )
	 fprintf (stderr, "DataGetTrend done. The file %s is generated.\n", filename );
       else
	 fprintf (stderr, "DataGetTrend: no output file has been generated.\n" );
       return 0;
}


/* Thread receives data from DAQD */
void* read_data()
{
int     i, j, byterv, irate = 0, firsttime = 0; 
long    totaldata = 0, totaldata0 = 0, skip = 0, newskip; /* in seconds */
time_t  ngps, ngps0;
FILE    *fd;

       DataReadStart();
       while ( 1 ) {
	  if ( (byterv = DataRead()) == -2 ) {
	     fprintf ( stderr, "Reset slopes and offsets.\n" );
	     for ( j=0; j<chNum; j++ ) {
	       DataGetChSlope(chName[j], &slope[j], &offset[j], &chstatus[j]);
	     }
	  }
	  else if ( byterv < 0 ) {
             DataReadStop();
	     break;
	  }
	  else if ( byterv == 0 ) {
	     fprintf (stderr, "trailer received\n" );
	     break;
	     /*printf ( "cann't read file %s\n", timestring );*/
	  }
	  else {
	  DataTimestamp(timestring);
	  ngps = DataTimeGps();
	  if ( firsttime == 0 ) {
	    firsttime = 1;
	    ngps0 = ngps;
	    strcpy(starttime, timestring);
	  }
	  newskip = ngps - ngps0 - totaldata*playMode;
	  if ( newskip > 0 ) {
	    skip += newskip;
	    newskip = newskip/playMode;
	    if ( playMode == PLAYTREND60 ) 
	      fprintf (stderr, "     WARNING: data skipped: %d min. Putting 0's\n", newskip);
	    else
	      fprintf (stderr, "     WARNING: data skipped: %d sec. Putting 0's\n", newskip);
	    for ( i=0; i<newskip; i++ ) {
	      xTick[totaldata + i] = totaldata +i;
	    }
	    for ( j=0; j<chNum; j++ ) {
	      switch ( trendMode ) {
              case 0: 
		for ( i=0; i<newskip; i++ ) {
		  sData1[j][totaldata + i] = 0.0;
		  sData2[j][totaldata + i] = 0.0; 
		  sData3[j][totaldata + i] = 0.0; 
		}
		break;
              case 1: 
		for ( i=0; i<newskip; i++ ) {
		  sData1[j][totaldata + i] = 0.0; 
		}
		break;
              case 2: 
		for ( i=0; i<newskip; i++ ) {
		  sData2[j][totaldata + i] = 0.0; 
		}
		break;
              case 3: 
		for ( i=0; i<newskip; i++ ) {
		  sData3[j][totaldata + i] = 0.0; 
		}
		break;
	      }	    
	    }
	    totaldata += newskip;
	  }
          fprintf (stderr, "[%d] data read: %s\n", processID, timestring );
	  irate = DataTrendLength();
	  totaldata0 = ngps - ngps0 + irate;
	  irate = irate/playMode;
	  trendsec = (struct DTrend*)malloc(sizeDTrend * irate);
	  if ( trendsec == NULL ) {
	    fprintf (stderr, "Not enough memory. No output.\n" );
	    return;;
	  }
	  for ( i=0; i<irate; i++ ) {
	    xTick[totaldata + i] = totaldata +i;
	  }
	  for ( j=0; j<chNum; j++ ) {
	    DataTrendGetCh(chName[j], trendsec );
            switch ( trendMode ) {
              case 0: 
		for ( i=0; i<irate; i++ ) {
		  sData1[j][totaldata + i] = offset[j]+slope[j]*trendsec[i].min;
		  sData2[j][totaldata + i] = offset[j]+slope[j]*trendsec[i].mean;
		  sData3[j][totaldata + i] = offset[j]+slope[j]*trendsec[i].max;
		}
		break;
              case 1: 
		for ( i=0; i<irate; i++ ) {
		  sData1[j][totaldata + i] = offset[j]+slope[j]*trendsec[i].min;
		}
		break;
              case 2: 
		for ( i=0; i<irate; i++ ) {
		  sData2[j][totaldata + i] = offset[j]+slope[j]*trendsec[i].mean;
		}
		break;
              case 3: 
		for ( i=0; i<irate; i++ ) {
		  sData3[j][totaldata + i] = offset[j]+slope[j]*trendsec[i].max;
		}
		break;
	    }	    
	  }
	  if ( irate > 1 )
	    fprintf (stderr, "       data length %d\n", irate );
	  /*totaldata += irate;*/
	  totaldata = totaldata0;
	  if ( totaldata >= duration )
	    finished = -1;
	  totaldata = totaldata/playMode;
	  dataRecv = 1;
	  free(trendsec);
	  }
       }  /* end of while loop */


       if ( dataRecv ) {
	  /* write to file */
	  fd = fopen( filename, "w+b" );
	  if ( fd == NULL ) {
	     fprintf ( stderr, "Unexpected error when opening files. Exit.\n" );
	     return;
	  }
	  fprintf ( fd, "%%dataVersion%3.1f-%d;\n", VERSION, bi );
	  fprintf ( fd, "dataType=%d;\n", playMode );
	  fprintf ( fd, "dataChnum=%d;\n", chNum );
	  fprintf ( fd, "dataDuration=%d;\n", totaldata );
	  fprintf ( fd, "dataTrend=%d;\n", trendMode );
	  fprintf ( fd, "dataSkip=%d;\n", skip );
	  fprintf ( fd, "dataChname={\n" );
	  for ( j=0; j<chNum-1; j++ ) {
	     fprintf ( fd, "'%s';\n", chName[j] );
	  }
	  fprintf ( fd, "'%s'\n};\n", chName[chNum-1] );
	  fprintf ( fd, "dataStart='%s';\n", starttime );
	  
	  if ( !bi ) { /* text */
	    fprintf ( fd, "dataArray=[..." );
	    switch ( trendMode ) {
            case 0: 
	      for ( j=0; j<totaldata; j++ ) {
		fprintf ( fd, "\n" );
		fprintf ( fd, "%f ", xTick[j] );
		for ( i=0; i<chNum; i++ ) {
		  fprintf ( fd, "%le %le %le ", sData1[i][j],sData2[i][j],sData3[i][j] );
		}
	      }
	      break;
            case 1: 
	      for ( j=0; j<totaldata; j++ ) {
		fprintf ( fd, "\n" );
		fprintf ( fd, "%f ", xTick[j] );
		for ( i=0; i<chNum; i++ ) {
		  fprintf ( fd, "%le ", sData1[i][j] );
		}
	      }
	      break;
            case 2: 
	      for ( j=0; j<totaldata; j++ ) {
		fprintf ( fd, "\n" );
		fprintf ( fd, "%f ", xTick[j] );
		for ( i=0; i<chNum; i++ ) {
		  fprintf ( fd, "%le ", sData2[i][j] );
		}
	      }
	      break;
            case 3: 
	      for ( j=0; j<totaldata; j++ ) {
		fprintf ( fd, "\n" );
		fprintf ( fd, "%f ", xTick[j] );
		for ( i=0; i<chNum; i++ ) {
		  fprintf ( fd, "%le ", sData3[i][j] );
		}
	      }
	      break;
	    }	  
	    fprintf ( fd, " ];\n" );
	  }
	  else { /* binary */
	    switch ( trendMode ) {
            case 0: 
	      fwrite(xTick,sizeof(double),totaldata,fd);
	      for ( i=0; i<chNum; i++ ) {
		fwrite(sData1[i],sizeof(double),totaldata,fd);
		fwrite(sData2[i],sizeof(double),totaldata,fd);
		fwrite(sData3[i],sizeof(double),totaldata,fd);
	      }
	      break;
            case 1: 
	      fwrite(xTick,sizeof(double),totaldata,fd);
	      for ( i=0; i<chNum; i++ ) {
		fwrite(sData1[i],sizeof(double),totaldata,fd);
	      }
	      break;
            case 2: 
	      fwrite(xTick,sizeof(double),totaldata,fd);
	      for ( i=0; i<chNum; i++ ) {
		fwrite(sData2[i],sizeof(double),totaldata,fd);
	      }
	      break;
            case 3: 
	      fwrite(xTick,sizeof(double),totaldata,fd);
	      for ( i=0; i<chNum; i++ ) {
		fwrite(sData3[i],sizeof(double),totaldata,fd);
	      }
	      break;
	    }	  
	  }
	  fclose( fd );
	  free(xTick);
	  if (playMode == PLAYTREND60) 
	    fprintf (stderr, "DataGetTrend: %d minute of data write to file %s\n", totaldata, filename );
	  else
	    fprintf (stderr, "DataGetTrend: %d second of data write to file %s\n", totaldata, filename );
       }
       else
	  fprintf (stderr, "DataGetTrend: no data received\n" );
       
       finished = 1;
       fflush (stdout);
       return NULL;
}
