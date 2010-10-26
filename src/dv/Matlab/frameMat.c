/* test.c  */
/* Compile with 
   gcc -o frameMat frameMat.c Lib/datasrv.o Lib/daqc_access.o decimate.o -L/home/hding/Cds/Frame/Lib/UTC_GPS -ltai -lsocket -lpthread 
*/
#include "Lib/datasrv.h"
#include "decimate.h"
#include "mexversion.h"

#define CHANNUM      16
#define ONLINE       0
#define OFFLINE      1

double   *xTick, xhskip;
double  *sData[CHANNUM];
long    acntr = 0;

char   timestring[24], starttime[24];
char   chName[CHANNUM+1][64], filename[100];
int    rate[CHANNUM], chstatus[CHANNUM];
float  slope[CHANNUM], offset[CHANNUM];
int    chNum, resolution, duration, dataRecv=0, filFlag;
short  finished = 0, online, bi=0;
unsigned long processID = 0;

/* Data-receiving thread prototype */
void* read_data();

int main(int argc, char *argv[])
{
int   LISTENER_PORT=7000, DAQD_PORT_NUM=0; 
char  DAQD_HOST[80], temp[24], ch[2];
int   i, j; 
short same=1; 

       fprintf(stderr, "DataGet: starting connection to Server\n");
       chNum = atoi(argv[1]);
       for ( j=0; j<chNum; j++ ) {
	  strcpy(chName[j], argv[j+2] );
       }
       strcpy(starttime, argv[chNum+2] );
       duration = atoi(argv[chNum+3]);
       resolution = atoi(argv[chNum+4]);
       filFlag = atoi(argv[chNum+5]);
       strcpy(DAQD_HOST, argv[chNum+6]);
       DAQD_PORT_NUM = atoi(argv[chNum+7]);
       strcpy(filename, argv[chNum+8]);
       if ( argc >= chNum+10 ) {
	 bi = atoi(argv[chNum+9]);
       }
       /*filFlag = 0;*/
       fprintf(stderr, "      %s - %d\n", DAQD_HOST, DAQD_PORT_NUM);

       if ( DataConnect(DAQD_HOST, DAQD_PORT_NUM, LISTENER_PORT, read_data) != 0 ) {
	  fprintf ( stderr,"connection to Server %s failed\n", DAQD_HOST ); 
	  exit(1);
       }
       for ( j=0; j<chNum; j++ ) {
	 rate[j] = DataChanRate(chName[j]);
	 if ( rate[j] < 0 ){
	    fprintf (stderr, "Error: channel %s is not found. No output.\n", chName[j] );
	    exit(1);
	 }
       }
       if ( strcmp(starttime, "0") == 0 || strcmp(starttime, "") == 0 )
	 online = ONLINE;
       else
	 online = OFFLINE;
       /* check if sampling rates are equal */
       for ( j=0; j<chNum; j++ ) {
	 if ( rate[j] != rate[0] ) {
	    same = 0;
	    filFlag = 0;
	    fprintf (stderr, "Warning: the sampling rates of the channels are not the same - no filter will be applied.\n" );
	    fprintf(stderr, "Continue? [y/n] ");
	    scanf("%s",ch);
	    if ( ch[0] != 'y' ) {
	      fprintf (stderr, "Terminated.\n"  );
	      return 0;
	    }
	    break;
	 }
       }
       if (resolution == 0) {
	 resolution = rate[0];
	 filFlag = 0;
       }
       /* decide the smallest sampling rate */
       for ( j=0; j<chNum; j++ ) {
	 if ( rate[j] < resolution )
	    resolution = rate[j];
       }
       fprintf (stderr, "Sampling rate: %d; Filter flag: %d\n", resolution, filFlag );
       xTick = (double*)malloc(sizeof(double)*(duration+1)*resolution);
       if ( xTick == NULL ) {
	 fprintf (stderr, "Not enough memory. No output.\n" );
	 exit(1);
       }
       for ( j=0; j<chNum; j++ ) {
	 sData[j] = (double*)malloc(sizeof(double)*(duration+1)*resolution);
	 if ( sData[j] == NULL ) {
	   fprintf (stderr, "Not enough memory. No output.\n" );
	   exit(1);
	 }
       }
              
       for ( j=0; j<chNum; j++ ) {
	 if ( filFlag )
	   DataChanAdd(chName[j], 0);
	 else /* no filter */
	   DataChanAdd(chName[j], resolution);
       }
       xhskip = 1.0/resolution;
       if ( online == ONLINE )
	 processID = DataWriteRealtime();
       else
	 processID = DataWrite(starttime, duration, 0);
       if ( processID == -1 ) {
	 fprintf (stderr, "DataGetTrend: no output file has been generated.\n" );
	 return 0;
       }
       while ( !finished );
       if ( online == ONLINE )
	 DataWriteStop(processID); 
       while ( finished != 1 );       
       sleep(3);
       DataQuit();
       if ( dataRecv )
	 fprintf (stderr, "DataGet done. The file %s is generated.\n", filename );
       else
	 fprintf (stderr, "DataGet: no output file has been generated.\n" );
       return 0;
}


/* Thread receives data from DAQD */
void* read_data()
{
double  chData[16384];
float   inData[16384], outData[16384], *(temp[CHANNUM]);
int     i, j, byterv, FIL; 
long    totaldata = 0; /* in seconds */
FILE    *fd;
int     bc, bsec;

       DataReadStart();
       /* initialize the filter data using an empty array */
       if ( filFlag ) {
	 FIL = rate[0]/resolution;
	 for ( j=0; j<chNum; j++ ) {
	   decimate (filFlag, inData, outData, 0, FIL, NULL, &(temp[j]));
	 }
       }
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
	  if ( totaldata == 0 )
	    strcpy(starttime, timestring);
	  /* Process received data here */
          fprintf (stderr, "[%d] data read: %s\n", processID, timestring );
	  bsec = DataTrendLength();
	  for ( j=0; j<chNum; j++ ) {
	   for ( bc=0; bc<bsec; bc++ ) {
	     /* Find data in the desired data channels */
	     DataGetCh(chName[j], chData, bc );
	     if ( filFlag == 0 ) {
	       for ( i=0; i<resolution; i++ ) {
		 sData[j][(totaldata+bc)*resolution + i] = offset[j]+slope[j]*chData[i];
	       }
	     }
	     else { /* apply filter */
	       for ( i=0; i<rate[0]; i++ ) 
		 inData[i] = offset[j]+slope[j]*(float)chData[i];
		 decimate (filFlag, inData, outData, rate[0], FIL, temp[j], &(temp[j]));
		 for ( i=0; i<resolution; i++ ) {
		   sData[j][(totaldata+bc)*resolution + i] = (double)outData[i];
		 }
	     }
	   }
	  }
	  for ( i=0; i<resolution*bsec; i++ ) {
	     xTick[totaldata*resolution + i] = (double)(totaldata + i*xhskip);
	  }
	  totaldata = totaldata + bsec;
	  if ( totaldata >= duration )
	    finished = -1;
	  dataRecv = 1;
	  }
       } /* end of while loop  */

       /* cleanup */
       if ( filFlag ) {
	 for ( j=0; j<chNum; j++ ) 
	   decimate (filFlag, inData, outData, 0, FIL, temp[j], NULL);
       }


       if ( dataRecv ) {
	  /* write to file */
	 fd = fopen( filename, "w+b" );
	 if ( fd == NULL ) {
	   fprintf ( stderr, "Unexpected error when opening files. Exit.\n" );
	   return;
	 }
	 fprintf ( fd, "%%dataVersion%3.1f-%d;\n", VERSION, bi );
	 fprintf ( fd, "dataType=0;\n" );
	 fprintf ( fd, "dataChnum=%d;\n", chNum );
	 fprintf ( fd, "dataDuration=%d;\n", totaldata );
	 fprintf ( fd, "dataResolution=%d;\n", resolution );
	 fprintf ( fd, "dataFilter=%d;\n", filFlag );
	 fprintf ( fd, "dataChname={\n" );
	 for ( j=0; j<chNum-1; j++ ) {
	   fprintf ( fd, "'%s';\n", chName[j] );
	 }
	 fprintf ( fd, "'%s'\n};\n", chName[chNum-1] );
	 fprintf ( fd, "dataStart='%s';\n", starttime );
	 if ( !bi ) { /* text */
	   fprintf ( fd, "dataArray=[..." );
	   
	   for ( j=0; j<resolution*totaldata; j++ ) {
	     fprintf ( fd, "\n" );
	     fprintf ( fd, "%2f ", xTick[j] );
	     for ( i=0; i<chNum; i++ ) {
	       fprintf ( fd, "%le ", sData[i][j] );
	     }
	   }
	   fprintf ( fd, " ];\n" );
	 }
	 else { /* binary */
	   fwrite(xTick,sizeof(double),resolution*totaldata,fd);
	   for ( i=0; i<chNum; i++ ) 
	     fwrite(sData[i],sizeof(double),resolution*totaldata,fd);
	 }
	 fclose( fd );
	 fprintf (stderr, "DataGet: %d second of data write to file %s\n", totaldata, filename );
       }
       else
	  fprintf (stderr, "DataGet: no data received\n" );
       for ( j=0; j<chNum; j++ ) 
	 free(sData[j]);
       free(xTick);
       
       finished = 1;
       fflush (stdout);
       return NULL;
}
