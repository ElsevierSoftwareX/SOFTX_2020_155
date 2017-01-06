/*----------------------------------------------------------------------*/
/*                                                                      */
/* Module Name: framer4.c                                               */
/* Compile with                                                         */
/*  gcc -o framer4 framer4.c datasrv.o daqc_access.o                    */
/*     -L/home/hding/Try/Lib/Grace -l grace_np                    */
/*     -L/home/hding/Try/Lib/UTC_GPS -ltai                        */
/*     -lm -lnsl -lsocket -lpthread                                     */
/*                                                                      */
/*----------------------------------------------------------------------*/

#include "framer4.h"


int main(int argc, char *argv[])
{
int   i=0;
char  origDir[1024], logDir[80], displayIP[80];
char  wfile[80];
int   zoomflag;

/* Open pipe file to Dadsp */
	key = atoi(argv[1]);
	if ( (msqid = msgget(key, IPC_CREAT | 0666)) < 0 ) {
	  perror( "framer4 msgget:msqid" );
	  exit(-1);
	}
	sprintf ( msgout, "display: attached message queue %d\n", msqid );
	printmessage(1);
        strcpy(serverIP, argv[2]);
	serverPort = atoi(argv[3]);
        strcpy(origDir, argv[4]);
        strcpy(displayIP, argv[5]);
        strcpy(logDir, argv[6]);
        strcpy(version_n, argv[7]);
        strcpy(version_d, argv[8]);
	zoomflag = atoi(argv[9]);
	nolimit = atoi(argv[10]);
	shellID = atoi(argv[11]);
	if ( nolimit ) {
	  sprintf ( msgout, "Starting Dataviewer display with no time limit.\n" );
	  printmessage(1);
	}
	sprintf ( wfile, "%slog_warning", logDir );
	sprintf ( msgout, "Starting framer4 with warning file %s, displayIP %s, server %s:%d\n", wfile, displayIP, serverIP, serverPort );

	firsttime = 1;
	initdata = 0;

/* set up warning file  */
	fwarning = fopen(wfile, "a");
	printmessage(1);
	if (fwarning == NULL) {
	  sprintf ( msgout, "Error for opening warning file: %s. Exit.\n", wfile );
	  printmessage(0);
	  return -1;
	}
	sprintf ( msgout, "LOGFILE:\n" );
	fprintf ( fwarning, "%s", msgout );
	/*fprintf ( fwarning, "WARNING: %s", msgout );*/
	/*printmessage(-1);*/

/* Launch xmgrace with named-pipe support 
   this function has been rewritten for our own need*/
	/*if (GraceOpen(1000000, displayIP, origDir) == -1) {*/
	if (GraceOpen(640, displayIP, origDir, zoomflag) == -1) {
	  sprintf ( msgout, "Can't run xmgrace. \n" );
	  printmessage(-2);
	  exit (EXIT_FAILURE);
	}
	sprintf ( msgout, "Done launching xmgrace \n" );
	printmessage(1);

/* Connect to Data Server */
	while ( DataConnect(serverIP, serverPort, userPort, read_data) != 0 ) {
	   i++;
           if ( i>25 ) {
	      sprintf ( msgout, "can't connect to the data server after 25 tries\n" );
	      printmessage(-2);
	      exit(-2);
	   }
	}
	sprintf ( msgout, "Done connecting to the data server\n" );
	printmessage(1);

/* Paint Xmgrace */	
	graphini();
/* Initialize variables */
	newReadMode = STOPMODE;
	startnew = 1;
	for ( i=0; i<ADC_BUFF_SIZE; i++ ) fastTime[i] = -16.0 + i*0.0078125;
	for ( i=0; i<16; i++ ) {
	   chNameChange[i] = 1;
	   winYMin[i] = -4000.0;
	   winYMax[i] = 4000.0;
	   winScaler[i] = 4000.0;
	   graphRate[i] = 128;
	   winXScaler[i] = 8.0;
	   slope[i] = 1.0;
	   offset[i] = 0.0;
	   uniton[i] = 1;
	}
	graphOption = GRAPHTIME;
	graphMethod = GMODESTAND;
	graphMethod0 = GMODESTAND;
	chSelect = 0;
	dPoints = ADC_BUFF_SIZE;
	sPoints = ADC_BUFF_SIZE;


/* Go into Endless Loop */
	while ( 1 ) {
	   checkOpInputs();
	}
	return 0;
}



void changeReadMode()
{
int     i;
char    starttime[24];
int     duration;

        for ( i=0; i<16; i++ ) chNameChange[i] = 1;

	trailer = 0;
	if ( paused == 0)
	   trendCount = 0;

	switch ( newReadMode ) {
	  case STOPMODE: /* Stop Display */
	      if ( !stopped ) {
		 stopped = 1;
		 DataWriteStop(processID);
	      }
	      for ( i=0; i<16; i++ ) chNameChange[i] = 1;
	      if (fppause != NULL)
		fclose(fppause);
	      paused = 0;
	      sprintf ( msgout, "New display mode: STOPMODE\n" );
	      printmessage(reset);
	      break;
	  case REALTIME:
              timeCount = 0; /* shut down after 12 hours */
	      if ( paused == 1 ) {
		 paused = -1; /* stop recording but hold on realtime */
		 sprintf ( msgout, "REALTIME/RESUME\n" );
		 printmessage(0);
		 if ( errorpause ) {
		    sprintf ( msgout, "Couldn't record paused data\n" );
		    printmessage(-2);
		    errorpause = 0;
		 }
		 else {
		    if (fppause != NULL)
		      fclose(fppause);
		    resumePause();
		 }
		 paused = 0;
		 return;
	      }
	      firsttime = 1;
	      if ( startnew ) { /* use only once */
		 changeConfigure();
		 startnew = 0;
	      }
	      	      sprintf ( msgout, "New display mode: REALTIME\n" );
	      printmessage(reset);
	      if ( !stopped ) {
		 DataWriteStop(processID);
		 //sleep(1);
	      }
	      else
		 stopped = 0;
	      if ( graphOption == GRAPHTREND )
		 processID = DataWriteTrendRealtime();
	      else if ( graphOption == GRAPHTIME && refreshRate > 1 
                      && (graphMethod == GMODESTAND ||graphMethod == GMODEMULTI ))
		 if ( nslow ) {
		    stopped = 1;
		    sprintf ( msgout,"STOP! can't play slow channel for refresh rate > 1.\n" );
		    printmessage(-1);
		 }
		 else
		    processID = DataWriteRealtimeFast();
	      else
		 processID = DataWriteRealtime();
	      break;
	  default:
	      break;
	}
	
	if ( processID == -1 ) {/* error happened */
	   stopped = 1;
	   DataWriteStop(processID);
	   printmessage(-3);
	   quitdisplay(2);
	}
	return;
}


void  changeConfigure()
{
int j;

        if ( !stopped ) {
	   DataWriteStop(processID);
	   //sleep(1);
	}
        firsttime = 1;
	trendCount = 0;
	trailer = 0;
	if ( paused ) {
	   paused = 0;
	   stopped = 1;
	   sprintf ( msgout,  "Trend Pause Canceled\n");
	   printmessage(0);
	}

	DataChanDelete("all");
	if ( graphOption0 == GRAPHTREND || filterOn ) {
	   for ( j=0; j<windowNum; j++ ) {
	      DataChanAdd(chName[chMarked[j]], 0);
	   }
	}
	else {
	   if ( trigOn )
	      DataChanAdd(chName[chanTrig0-1], 0);
	   nslow = 0;
	   for ( j=0; j<windowNum; j++ ) {
	      if ( chType[chMarked[j]] == 3 ) nslow = 1;
	      if ( !trigOn || chMarked[j] != chanTrig0-1 ) {
		 if ( chType[chMarked[j]] == 1 ) { /* if fast channel */
		   DataChanAdd(chName[chMarked[j]], resolution);
		 }
		 else { /* if slower channel */
		    DataChanAdd(chName[chMarked[j]], 0);
		 }
	      }
	   }
	}

	if ( stopped == 0  ) {
	   /*sleep(2);*/
	   /* change the following settings here instead of in checkOpInputs(),
              so they won't happen in the middle of the graphing  */
	   graphOption = graphOption0;
	   graphMethod = graphMethod0;
	   chSelect = chSelect0;
	   chanTrig = chanTrig0;
	   if ( newReadMode == REALTIME ) {
	      if ( graphOption == GRAPHTREND ) 
		 processID = DataWriteTrendRealtime();
	      else if ( graphOption == GRAPHTIME && refreshRate > 1 
			&& (graphMethod == GMODESTAND ||graphMethod == GMODEMULTI )) {
		 if ( nslow ) {
		    stopped = 1;
		    sprintf ( msgout,"STOP! can't play slow channel for refresh rate > 1.\n" );
		    printmessage(-1);
		 }
		 else
		    processID = DataWriteRealtimeFast();
	      }
	      else {
		 processID = DataWriteRealtime();
	      }
	   }
	}
	else {
	   graphOption = graphOption0;
	   graphMethod = graphMethod0;
	   chSelect = chSelect0;
	   chanTrig = chanTrig0;
	}
	if ( processID == -1 ) {/* error happened */
	   stopped = 1;
	   DataWriteStop(processID);
	   printmessage(-3);
	}
	return;
}




/*----------------------------------------------------------------------*/
/*                                                                      */
/* External Procedure Name: read_data                                   */
/*                                                                      */
/* Procedure Description: Reads data from Data Server, reformats it,    */
/*              rewrites it to files for DaDsp, and issues commands     */
/*              to DaDsp to read & display the data.                    */
/*              input data and perform a filter algorithm prior to      */
/*              sending data to DaDsp                                   */
/*                                                                      */
/* Procedure Arguments:                                                 */
/*                                                                      */
/* Procedure Returns: Nothing                                           */
/*                                                                      */
/*----------------------------------------------------------------------*/
void* read_data()
{
/*double chData[NCF*2];  should be global because of the limit of multithread */

int   i, j, k, l;
int   hshift, hskip, acntr, byterecv, hsize, fPoints, tPoints, step;
int   freqMax1, irate, drop, trendfrom, trendto;
unsigned long block = 0, block1; 
double result, fnorm, c1, c2, c;
double tempxscaler;
int   tempdelay;
char  acetempname[80];
FILE  *fptemp;
int   timelimit;

double realtrigger;

	DataReadStart();

        while ( 1 ) {
	   if ( quitting ) {
	      quitdisplay(0);
	      break;
	   }
	   if ( stopped ) { 
              /* add this since 'DataRead()) <= 0' may have few second delay  */
              DataReadStop();
	      break;
	   }
	   if ( printing >= 0 )
	      printgraph();
	   if ( (byterecv=DataRead()) == -2 ) {
	      /* reset slope and offset */
	         sprintf ( msgout, "Slopes, offsets, or status are changed\n" );
	     printmessage(0);
	     firsttime = 1;
	     for ( j=0; j<windowNum; j++ ) {
	       DataGetChSlope(chName[chMarked[j]], &slope[chMarked[j]], &offset[chMarked[j]], &chstatus[chMarked[j]]);
	       dfprintf ( stderr,"ch%d status=%d\n",j, chstatus[j] );
	       dfprintf ( stderr, "%s %f %f\n", chName[chMarked[j]], slope[chMarked[j]], offset[chMarked[j]] );
	     }
	     
	   }
	   else if ( byterecv < 0 ) {
#if 0
	      sprintf ( msgout, "DataRead = %d. Stop signal received from Server.\n", byterecv );
	      printmessage(0);
#endif
              DataReadStop();
	      /*stopped = 1;*/
	      break;
	   }
	   else if ( byterecv == 0 ) {
	      trailer = 1;
	      stopped = 1;
#if 0
	      sprintf ( msgout, "trailer received\n" );
	      printmessage(0);
#endif
	      break;
	   }
	   else { /* read data */
           DataTimestamp(timestring);
           if ( !nolimit )
	     timeCount++;
	   block1 = block;
	   block = DataSequenceNum();
	   if ( block == 0 ) {
	      block1 = -1;
	      firsttime = 1;
	   }
	   if ( graphOption == GRAPHTIME && refreshRate > 1 ) {
	      bindex = (block+1) % blocksize;
	   }
	   else
	      bindex = 0;
	   if ( block > block1 + 1 ) {
	      drop = block - block1 - 1;
	      sprintf ( msgout, "   !!data dropped %d blocks\n", drop);
	      printmessage(0);
              for ( j=0; j<=16; j++ ) chNameChange[j] = 1;
	   }
	   else
	      drop = 0;
	   
	   if ( paused == 1 ) { /* Pause Trend: record data in a file */
	      sprintf ( msgout, "pause data: %s\n", timestring );
	      printmessage(0);
	      if ( errorpause == 0 ) {
		 for ( j=0; j<windowNum; j++ ) {
		   if ( DataChanType(chName[chMarked[j]]) <= 0 ) { /*if no data */
		     fprintf(fppause, "%d %f\n", trendCount+1,0.0  );
		   }
		   else {
		     DataTrendGetCh(chName[chMarked[j]], trenddata );
                     switch ( xyType[chMarked[j]] ) {
                       case 1: 
			   if ( trenddata[0].mean == 0)
			      trenddata[0].mean = ZEROLOG;
			   else
			      trenddata[0].mean = fabs(trenddata[0].mean);
                            break;
                       case 2: 
			   if ( trenddata[0].mean == 0)
			      trenddata[0].mean = ZEROLOG;
			   else
			      trenddata[0].mean = log(fabs(trenddata[0].mean));
                           break;
                      default: 
                           break;
                     }
		     fprintf(fppause, "%d %f\n", trendCount+1,trenddata[0].mean  );
		   } 
		 }
		 pausefilecn++;
	      }
	      trendCount += (1 + drop);
	   }
	   else if ( paused == -1 ){ /* skip data while restoring paused trend */
	      sprintf ( msgout, "skipping data: %s\n", timestring );
	      printmessage(0);
	   }
	   /* The following are the general cases */
	   else {
	   if ( bindex == 0 ) {
	    sprintf ( msgout, "data read: %s\n", timestring );
	    printmessage(reset);
	      /*fprintf ( stderr, "data read: %s  block=%d\n", timestring, block );*/
	      tempcycle++;
	      tempcycle = tempcycle % 16;
	      if ( firsttime ) {
		 GracePrintf( "clear string" );
	      }
	      if ( screenChange ) {
		 GracePrintf( "with g0" );
		 for ( j=0; j<16; j++ ) 
		    GracePrintf( "kill s%d", j );
		 if ( graphOption == GRAPHTIME  ) {
		   for ( j=1; j<=16; j++ ) {
		     GracePrintf( "with g%d", j );
		     GracePrintf( "kill s0" );
		     GracePrintf( "kill s1" );
		     GracePrintf( "kill s2" );
		   }
		   screenChange++;
		 }

	      }
	   }


	   warning = 0;

           switch ( graphOption ) {
	   case GRAPHTIME:
	     switch ( graphMethod ) {
	     case GMODESTAND: 
	     case GMODEMULTI: 
	       if ( graphMethod == GMODEMULTI ) {
		 GracePrintf( "with g0" );
		 for ( j=0; j<16; j++ ) 
		   GracePrintf( "kill s%d", j );
		 GracePrintf( "with g1" );
		 GracePrintf( "kill s0" );
	       }
	       else { /* GMODESTAND */
		 GracePrintf( "with g0" );
		 for ( j=0; j<16; j++ ) 
		    GracePrintf( "kill s%d", j );
		 for ( j=1; j<=16; j++ ) {
		   GracePrintf( "with g%d", j );
		   GracePrintf( "kill s0" );
		 }
	       }
	       fflush(stdout);

	       GracePrintf( "with g0" );
	       for ( j=0; j<windowNum; j++ ) {
		 /* Find data in the desired data channels */
		 irate = DataGetCh(chName[chMarked[j]], chData_0, 0, complexDataType[chMarked[j]]);
		 if ( irate < 0 ) {
		   sprintf ( msgout, "Ch.%d: no data output\n", j+1 );
		   printmessage(-1);
		   fflush (stdout);
		   return NULL;
		 }
		 step = irate/resolution;
		 if (filterOn && step >= FILTER_WIDTH) { /* filtering */
		   if (firsttime) {
		     for ( i=0; i<FILTER_WIDTH; i++ ) {
		       fhist[j][i] = chData_0[0];
		     }
		   }
		   movAvgFilter(chData_0, chData_1, fhist[j], irate, step);
		   chData = (double *) chData_1;
		   irate = resolution;
		 }
		 else if (filterOn && step == 2) { 
		   if (initdata == 0) { /* initial pt chData */
		     chData = (double *) chData_1;
		     initdata = 1;
		   }
		   for ( i=0; i<irate/2; i++ ) {
		     chData[i] = chData_0[2*i];
                   }
		 }
		 else { /* no filter */
		   chData = (double *) chData_0;
		 }

		 /* Get the data and write it to files which can be read by DaDsp  */
		 acntr = 0;
		 if ( chNameChange[chMarked[j]] ) {
		   for ( i=0; i<ADC_BUFF_SIZE; i++ ) 
		     tempData[j][i] =   chData[0]; /* show constant if status changed */
		 }
		 if ( bindex == 0 ) {
		   sprintf(acetempname, "/tmp/%dFRAME/framertemp%d_%d",shellID,j,tempcycle );
		   fptemp = fopen(acetempname, "w");
		 }
		 if ( chType[chMarked[j]] == 1 || chType[chMarked[j]] == 2 ) {  /* if fast I&II channel */
		   /* note: fast I channels display graphRate[j]=min{2*chSize[j],resolution} data 
		      fast II channels have full data always */
		   dataShiftS = graphRate[chMarked[j]]/refreshRate;
		   /* corr of delay and scale is taken cared in dc3
                      delay only applies to X<=1 */
		   if ( graphMethod == GMODEMULTI ) 
                      tempxscaler = winXScaler[chMarked[chSelect]];
		   else 
                      tempxscaler = winXScaler[chMarked[j]];
		   if ( graphRate[chMarked[j]] < 2048 ) { /* graphRate[j] < 2048 */
		      if (tempxscaler >= 1)
			fPoints = ADC_BUFF_SIZE - graphRate[chMarked[j]]*tempxscaler; 
		      else /* take the whole 1 sec */
			fPoints = ADC_BUFF_SIZE - graphRate[chMarked[j]]; 
		      if ( fPoints < 0 ) fPoints = 0;
		      tPoints = ADC_BUFF_SIZE;
		      for ( i=dataShiftS; i<ADC_BUFF_SIZE; i++ ) 
			 tempData[j][i-dataShiftS] = tempData[j][i];
		      acntr = ADC_BUFF_SIZE - dataShiftS;
                   }
                   else { /* show 1 sec only */
		      fPoints = 0;
		      tPoints = graphRate[chMarked[j]];
		      for ( i=dataShiftS; i<graphRate[chMarked[j]]; i++ ) 
			 tempData[j][i-dataShiftS] = tempData[j][i];
		      acntr = graphRate[chMarked[j]] - dataShiftS;
                   }
		   if ( trigOn && chMarked[j] == chanTrig-1 && chType[chMarked[j]] == 1) {
		     l = 0;
		     hskip = (2*chSize[chMarked[j]])/graphRate[chMarked[j]];
		     for ( i=0; i<2*chSize[chMarked[j]]/refreshRate; i+=hskip )  {
		       tempData[j][acntr] = chData[i];
		       acntr++;
		     }
		   }
		   else {
		     for ( i=0; i<dataShiftS; i++ )  {
		       tempData[j][acntr] = chData[i];
		       acntr++;
		     }
		   }
		   if ( bindex == 0 ) {
		     maxSec = 2048/resolution;
		     if ( graphMethod != GMODEMULTI ) { 
		       if ( maxSec <= 1 || winXScaler[chMarked[j]] < 1 ) {
			 c = -1.0;
		       }
		       else { /* c = -min{maxSec, winXScaler[j]} */
			 if (maxSec < winXScaler[chMarked[j]])
			   c = -maxSec;
			 else
			   c = -winXScaler[chMarked[j]];
		       }
		     }
		     if ( graphMethod == GMODEMULTI ) { /* show no xyType */
		       if ( maxSec <= 1 || winXScaler[chMarked[chSelect]] < 1 ) 
			 c = -1.0;
		       else 
			 c = -winXScaler[chMarked[chSelect]];
#if 0
		       if ( uniton[chMarked[j]] ) {
			 for (i=fPoints; i<tPoints; i++ ) {
			   fprintf(fptemp, "%2f\t%le\n", c, offset[chMarked[j]]+slope[chMarked[j]]*tempData[j][i] );
			   c = c + 1.0/graphRate[chMarked[j]];
			 }
		       }
		       else 
#endif
		       {
			 for (i=fPoints; i<tPoints; i++ ) {
			   fprintf(fptemp, "%2f\t%le\n", c, tempData[j][i] );
			   c = c + 1.0/graphRate[chMarked[j]];
			 }
		       }
		     }
		     else if ( uniton[chMarked[j]] ) {
		       switch ( xyType[chMarked[j]] ) {
		       case 1: 
			 for (i=fPoints; i<tPoints; i++ ) {
			   result = tempData[j][i];
			   if ( result == 0 ) {
			     result = ZEROLOG;
			     warning = 1;
			   }
			   else
			     result = fabs(result);
#if 0
			   fprintf(fptemp, "%2f\t%le\n", c, offset[chMarked[j]]+slope[chMarked[j]]*result );
#else
			   fprintf(fptemp, "%2f\t%le\n", c, result );
#endif
			   c = c + 1.0/graphRate[chMarked[j]];
			 }
			 break;
		       case 2: 
			 for (i=fPoints; i<tPoints; i++ ) {
			   result = tempData[j][i];
			   if ( result == 0 ) {
			     result = ZEROLOG;
			     if ( chMarked[j] == chSelect ) warning = 1;
			   }
			   else
			     result = log(fabs(result));
#if 0
			   fprintf(fptemp, "%2f\t%le\n", c, offset[chMarked[j]]+slope[chMarked[j]]*result );
#else
			   fprintf(fptemp, "%2f\t%le\n", c, result );
#endif
			   c = c + 1.0/graphRate[chMarked[j]];
			 }
			 break;
		       default:
			 for (i=fPoints; i<tPoints; i++ ) {
#if 0
			   fprintf(fptemp, "%2f\t%le\n", c, offset[chMarked[j]]+slope[chMarked[j]]*tempData[j][i] );
#else
			   fprintf(fptemp, "%2f\t%le\n", c, tempData[j][i] );
#endif
			   c = c + 1.0/graphRate[chMarked[j]];
			 }
			 break;
		       }
		     }
		     else { /* unit off */
		       switch ( xyType[chMarked[j]] ) {
		       case 1: 
			 for (i=fPoints; i<tPoints; i++ ) {
			   result = tempData[j][i];
			   if ( result == 0 ) {
			     result = ZEROLOG;
			     warning = 1;
			   }
			   else
			     result = fabs(result);
			   fprintf(fptemp, "%2f\t%le\n", c, result );
			   c = c + 1.0/graphRate[chMarked[j]];
			 }
			 break;
		       case 2: 
			 for (i=fPoints; i<tPoints; i++ ) {
			   result = tempData[j][i];
			   if ( result == 0 ) {
			     result = ZEROLOG;
			     if ( j == chSelect ) warning = 1;
			   }
			   else
			     result = log(fabs(result));
			   fprintf(fptemp, "%2f\t%le\n", c, result );
			   c = c + 1.0/graphRate[chMarked[j]];
			 }
			 break;
		       default:
			 for (i=fPoints; i<tPoints; i++ ) {
			   fprintf(fptemp, "%2f\t%le\n", c, tempData[j][i] );
			   c = c + 1.0/graphRate[chMarked[j]];
			 }
			 break;
		       }
		     }
		   }
		 }
		 else {  /* if slow channel. refreshRate=1 */
		   for ( i=0; i<16-irate; i++ ) 
		     tempData[j][i] = tempData[j][i+irate];
		   for ( i=16-irate; i<16; i++ ) 
		     tempData[j][i] = chData[i-16+irate];
		   if ( graphMethod == GMODEMULTI ) {
		     if ( uniton[chMarked[j]] ) {
		       for (i=0; i<16; i++ ) {
#if 0
			 fprintf(fptemp, "%2f\t%le\n", i-15.5, offset[chMarked[j]]+slope[chMarked[j]]*tempData[j][i] );
#else
			 fprintf(fptemp, "%2f\t%le\n", i-15.5, result );
#endif
		       }
		     }
		     else {
		       for (i=0; i<16; i++ ) {
			 fprintf(fptemp, "%2f\t%le\n", i-15.5, tempData[j][i] );
		       }
		     }
		   }
		   else if ( uniton[chMarked[j]] ) {
		     switch ( xyType[chMarked[j]] ) {
		     case 1: 
		       for (i=0; i<16; i++ ) {
			 result = tempData[j][i];
			 if ( result == 0 ) {
			   result = ZEROLOG;
			   warning = 1;
			 }
			 else
			   result = fabs(result);
#if 0
			 fprintf(fptemp, "%2f\t%le\n", i-15.5, offset[chMarked[j]]+slope[chMarked[j]]*result );
#else
			 fprintf(fptemp, "%2f\t%le\n", i-15.5, result );
#endif
		       }
		       break;
		     case 2: 
		       for (i=0; i<16; i++ ) {
			 result = tempData[j][i];
			 if ( result == 0 ) {
			   result = ZEROLOG;
			   warning = 1;
			 }
			 else
			   result = log(fabs(result));
#if 0
			 fprintf(fptemp, "%2f\t%le\n", i-15.5, offset[chMarked[j]]+slope[chMarked[j]]*result );
#else
			 fprintf(fptemp, "%2f\t%le\n", i-15.5, result );
#endif
		       }
		       break;
		     default: 
		       for (i=0; i<16; i++ ) 
#if 0
			 fprintf(fptemp, "%2f\t%le\n", i-15.5, offset[chMarked[j]]+slope[chMarked[j]]*tempData[j][i] );
#else
			 fprintf(fptemp, "%2f\t%le\n", i-15.5, tempData[j][i] );
#endif
		       break;
		     }
		   }
		   else { /* unit off */
		     switch ( xyType[chMarked[j]] ) {
		     case 1: 
		       for (i=0; i<16; i++ ) {
			 result = tempData[j][i];
			 if ( result == 0 ) {
			   result = ZEROLOG;
			   warning = 1;
			 }
			 else
			   result = fabs(result);
			 fprintf(fptemp, "%2f\t%le\n", i-15.5, result );
		       }
		       break;
		     case 2: 
		       for (i=0; i<16; i++ ) {
			 result = tempData[j][i];
			 if ( result == 0 ) {
			   result = ZEROLOG;
			   warning = 1;
			 }
			 else
			   result = log(fabs(result));
			 fprintf(fptemp, "%2f\t%le\n", i-15.5, result );
		       }
		       break;
		     default: 
		       for (i=0; i<16; i++ ) 
			 fprintf(fptemp, "%2f\t%le\n", i-15.5, tempData[j][i] );
		       break;
		     }
		   }
		 }
		 chNameChange[chMarked[j]] = 0;
		 if ( bindex == 0 ) {
		   fclose(fptemp);
		   GracePrintf( "read xy \"%s\"", acetempname ); 
		 }
	       } /* end of for windowNum */
	       for ( i=0; i<16; i++ ) chNameChange[i] = 0;
	       if ( bindex == 0 ) {
		 if ( graphMethod == GMODEMULTI ) 
		   graphmulti();
		 else
		   graphout();
	       }
	       break;
	     default: 
	       break;
	     }
	     break;
	   case GRAPHTREND: 
	     switch ( graphMethod ) {
	     case GMODESTAND: 
	     case GMODEMULTI: 
	       if ( trendCount == 0 && stopped == 0 ) {
		 strcpy(trendtimestring, timestring);
		 if ( graphMethod == GMODEMULTI ) {
		   GracePrintf( "with g0" );
		   for ( j=0; j<16; j++ ) 
		     GracePrintf( "kill s%d", j );
		   GracePrintf( "with g1" );
		   GracePrintf( "kill s0" );
		 }
		 else { /* GMODESTAND */
		   for ( j=0; j<=16; j++ ) {
		     GracePrintf( "with g%d", j );
		     GracePrintf( "kill s0" );
		     GracePrintf( "kill s1" );
		     GracePrintf( "kill s2" );
		   }
		 }
	       }
	       fflush(stdout);
	       if ( trendCount >=  43200 - 1 ) { /* 12 hours */
		 sprintf ( msgout,"Trend has been played more than 24 hours. Reset at %s.\n", timestring );
		 printmessage(0);
		 GracePrintf( "saveall \"%s.save\"", timestring );
		 trendCount = 0;
		 firsttime = 1;
		 break;
	       }
	       for ( j=0; j<windowNum; j++ ) {
		 if ( graphMethod == GMODESTAND ) {
		   GracePrintf( "with g%d", j );
		   if ( DataChanType(chName[chMarked[j]]) <= 0 ) { /*if no data */
		     GracePrintf( "g%d.s0 point %2f, %2f", j, (float)(trendCount+1), 0.0 );
		   }
		   else {
		     irate = DataTrendGetCh(chName[chMarked[j]], trenddata );
		     switch ( graphOption ) {
		     case GRAPHTREND: 
		       switch ( xyType[chMarked[j]] ) {
		       case 1: 
			 for ( i=0; i<irate; i++ ) {
			   result = trenddata[i].mean;
			   if ( result == 0 ) {
			     result = ZEROLOG;
			     warning = 1;
			   }
			   else
			     result = fabs(result);
			   GracePrintf( "g%d.s0 point %2f, %le", j, (float)(trendCount+i), result );
			 }
			 break;
		       case 2: 
			 for ( i=0; i<irate; i++ ) {
			   result = trenddata[i].mean;
			   if ( result == 0 ) {
			     result = ZEROLOG;
			     warning = 1;
			   }
			   else
			     result = log(fabs(result));
			   GracePrintf( "g%d.s0 point %2f, %le", j, (float)(trendCount+i), result );
			 }
			 break;
		       default: 
			 for ( i=0; i<irate; i++ ) {
			   GracePrintf( "g%d.s0 point %2f, %le", j, (float)(trendCount+i), trenddata[i].mean );
			 }
			 break;
		       }
		       break;
		     }
		   }
		 }
		 else { /* GMODEMULTI*/
		   GracePrintf( "with g0" );
		   if ( DataChanType(chName[chMarked[j]]) <= 0 ) { /*if no data */
		     GracePrintf( "g0.s%d point %2f, %2f", j, (float)(trendCount+1), 0.0 );
		   }
		   else {
		     irate = DataTrendGetCh(chName[chMarked[j]], trenddata );
		     for ( i=0; i<irate; i++ ) { 
		       result = trenddata[i].mean;
		       GracePrintf( "g0.s%d point %2f, %le", j, (float)(trendCount+i), result );
		     }
		   }
		 }
	       }
	       for ( i=0; i<16; i++ ) chNameChange[i] = 0;	
	       trendCount += (irate + drop);
	       GraceFlush();
	       if ( graphMethod == GMODEMULTI )
		 if ( trendCount <= 11 ) 
		   graphtrmulti(0, 10);
		 else if ( trendCount <= 31 ) 
		   graphtrmulti(0, 30);
		 else if ( trendCount <= 61 ) 
		   graphtrmulti(0, 60);
		 else {
		   trendto = floor((double)trendCount/60);
		   trendto = (trendto + 1)*60;
		   graphtrmulti(0, trendto);
		 }
	       else {
		 if ( trendCount <= 11 ) 
		   graphtrend(0, 10);
		 else if ( trendCount <= 31 ) 
		   graphtrend(0, 30);
		 else if ( trendCount <= 61 ) 
		   graphtrend(0, 60);
		 else {
		   trendto = floor((double)trendCount/60);
		   trendto = (trendto + 1)*60;
		   graphtrend(0, trendto);
		 }
	       }
	       break;
	     default: 
	       break;
	     }
	     break;
	   default: 
	     break;
           } /* end of switch(graphOption) */
	   

	   /* check if it has been running more than 12 hours */
	   if ( graphOption == GRAPHTIME && refreshRate > 1 )
	     timelimit = 43200*16;
	   else
	     timelimit = 43200;
	   if ( timeCount > timelimit ) {
	     if ( !stopped ) {
	       stopped = 1;
	       DataWriteStop(processID);
	       sprintf ( msgout, "Display stopped since it has been running more than 12 hours. Press Play to start again.\n" );
	       printmessage(2);
	     }
	     for ( i=0; i<16; i++ ) chNameChange[i] = 1;
	   }
	   /* check trigger when not in trend... option */
	   trigApply = 0;
	   if ( trigOn && graphOption == GRAPHTIME ) {
	     irate = DataGetCh(chName[chMarked[chanTrig-1]], chData, 0, 1);
	     if ( irate < 0 ) {
	       sprintf ( msgout, "no data output for trigger channel\n" );
	       printmessage(-1);
	       fflush (stdout);
	       return NULL;
	     }
	     realtrigger = 0.0;
	     hsize = chSize[chMarked[chanTrig-1]];
	     if ( chType[chMarked[chanTrig-1]] == 1 )
	       k = 2*hsize/refreshRate;
	     else
	       k = 2*hsize;
	     
	     if ( trigHow == TRIGABOVE ) {
	       for ( i=0; i<k; i++ ) {
		 if ( chData[i] > trigLevel) {
		   sprintf ( msgout, "triggered above Level=%f, by Ch.%d data[%d]=%f\n", trigLevel,chanTrig,i,chData[i] );
		   printmessage(0);
		   trigApply = 1;
		   break;
		 }
	       }
	     }
	     else { /* TRIGBELOW  */
	       for ( i=0; i<k; i++ ) {
		 if ( chData[i] < trigLevel) {
		   sprintf ( msgout, "triggered below Level=%f, by Ch.%d data[%d]=%f\n", trigLevel,chanTrig,i,chData[i] );
		   printmessage(2);
		   trigApply = 1;
		   break;
		 }
	       }
	     }
	     if ( trigApply ) {
	       if ( !stopped ) {
		 stopped = 1;
		 DataWriteStop(processID);
	       }
	       for ( i=0; i<16; i++ ) chNameChange[i] = 1;
	     }
	   } /* end trigger  */
	   }
        }
        }  /* end of while loop  */
	
	fflush (stdout);
	return NULL;

}

checkOpInputs()
{
int i, j, msgint;
int iyr, imo, ida, ihr, imn, isc;
short chfound, chinsert;
static short resetReadMode;

	/* Check all the operator display options */
	/* sprintf ( mb.mtext, "\0" ); */
	mb.mtext[0] = '\0';
	mb.mtext[1] = '\0';
	/*while ( (msgint = msgrcv(msqid, &mb, MSQSIZE, 0, 0)) > 0 ) { */
	while (1 ) {
	   if ( (msgint = msgrcv(msqid, &mb, MSQSIZE, 0, 0)) == -1 ){
	      perror("ERROR msgrcv");
	      sprintf ( msgout, "Message Queue failed. Current display abandoned. Please start another New Connection.\n" );
	      printmessage(reset);
	      quitdisplay(0);
	   }
	   sprintf ( msgout, "messg received %d ************************* \n", mb.mtype );
	   printmessage(reset);
	   switch ( mb.mtype ) {
	     case 1: /* New display mode */
	         newReadMode = atoi(mb.mtext);
		 if (newReadMode == PAUSEMODE ) {
		    if ( (stopped == 0) && (graphOption == GRAPHTREND) ) {
		       sprintf(pausefile, "/tmp/%dFRAME/framerpause",shellID );
		       fppause = fopen(pausefile, "w");
		       if ( fppause == NULL ) {
			  sprintf ( msgout,"Can't open pause file\n"  );
			  printmessage(-2);
			  errorpause = 1;
		       }
		       else
			  errorpause = 0;
		       paused = 1;
		       pausefilecn = 0;
		    }
		    else {
		       newReadMode = STOPMODE;
		       paused = 0;
		       changeReadMode();
		    }
		 }
		 else
		    changeReadMode();
		 break;
	     case 2: /* display method */
	         graphMethod0 = atoi(mb.mtext);
                 switch ( graphMethod0 ) {
                   case GMODESTAND: 
		       sprintf ( msgout, "New Display Method: Standard\n" );
		       printmessage(reset);
                       break;
                   case GMODEMULTI: 
		       sprintf ( msgout, "New Display Method: Multiple\n" );
		       printmessage(reset);
                       break;
		 }		 
		 screenChange = 1;
		 if ( !reset )
		   changeConfigure();
		 for ( i=0; i<16; i++ ) chNameChange[i] = 1;	
		 break;
	     case 3: /* signal name selected */
		{
		 int cdt = 0;
		 if (strlen(mb.mtext) > 5) {
			int l = strlen(mb.mtext) - 4;
			if (!strcmp(mb.mtext + l, ".img")) {
			   cdt = 2;
			   mb.mtext[l] = 0;
			}
			if (!strcmp(mb.mtext  + l, ".mag")) {
			   cdt = 3;
			   mb.mtext[l] = 0;
			}
			if (!strcmp(mb.mtext + l, ".phs")) {
			   cdt = 4;
			   mb.mtext[l] = 0;
			}
			if (!strcmp(mb.mtext + l -1, ".real")) {
			   mb.mtext[l-1] = 0;
			   cdt = 1;
			}
   		 }
		 j = DataChanRate(mb.mtext);
		 if ( j < 0 ) {
		   sprintf ( msgout, "signal %s for Ch.%d doesn't not exist. Unchanged.\n", mb.mtext, chChange );
		   printmessage(-1);
		   break;
                 }
	         complexDataType[chChange-1] = cdt;
	         strcpy(chName[chChange-1],mb.mtext);
		 sprintf ( msgout, "New signal %s for Ch.%d\n", chName[chChange-1], chChange );
		 printmessage(reset);
		 chNameChange[chChange-1] = 1;
		 if ( j == 1 ) {
		    chType[chChange-1] = 3;
		    chSize[chChange-1] = j;
		 }
		 else {
		    chSize[chChange-1] = j/2;
		    if ( chSize[chChange-1] >= 128 ) /* if rate>=256 */
		       chType[chChange-1] = 1;
		    else 
		       chType[chChange-1] = 2;
		 }
		 if ( resolution <= j ) 
		    graphRate[chChange-1] = resolution;
		 else 
		    graphRate[chChange-1] = j;
		 if ( !reset ) {
		    screenChange = 1;
		    changeConfigure();
		 }
		 break;
		}
	     case 4: /* display option */
	         graphOption0 = atoi(mb.mtext);
                 switch ( graphOption0 ) {
                   case GRAPHTIME: 
		       sprintf ( msgout, "New Display Option: Time Sequence\n" );
		       printmessage(reset);
                       break;
                   case GRAPHTREND: 
		       sprintf ( msgout, "New Display Option: Trend\n" );
		       printmessage(reset);
                       break;
		 }		 
		 firsttime = 1;
		 chNameChange[chSelect0] = 1;
		 if ( !reset ) {
		    screenChange = 1;
		    changeConfigure();
		    for ( i=0; i<16; i++ ) chNameChange[i] = 1;	
		 }
		 break;
	     case 5: /* Y axis min */
	         sprintf ( msgout, "New Y axis min %s\n", mb.mtext );
		 printmessage(reset);
		 firsttime = 1;
		 if ( globOn ) {
		   for ( i=0; i<16; i++ ) {
		     winYMin[i] = atof(mb.mtext);
		     winScaler[i] = (winYMax[i]-winYMin[i])/2.0;
		   }
		 }
		 else {
		   winYMin[chSelect0] = atof(mb.mtext);
		   winScaler[chSelect0] = (winYMax[chSelect0]-winYMin[chSelect0])/2.0;
		 }
		 if ( chSelect != chSelect0 && !reset ) 
		   changeConfigure();
		 firsttime = 1;
		 if ( graphOption0 == GRAPHTIME ) {
		    screenChange = 1;
		 }
		 break;
	     case 6: /* Y axis max */
	         sprintf ( msgout, "New Y axis max %s\n", mb.mtext );
		 printmessage(reset);
		 if ( globOn ) {
		   for ( i=0; i<16; i++ ) {
		     winYMax[i] = atof(mb.mtext);
		     winScaler[i] = (winYMax[i]-winYMin[i])/2.0;
		   }
		 }
		 else {
		   winYMax[chSelect0] = atof(mb.mtext);
		   winScaler[chSelect0] = (winYMax[chSelect0]-winYMin[chSelect0])/2.0;
		 }
		 if ( chSelect != chSelect0 && !reset ) 
		   changeConfigure();
		 firsttime = 1;
		 if ( graphOption0 == GRAPHTIME ) {
		    screenChange = 1;
		 }
		 break;
	     case 7: /* Y Scale Type*/
		 if ( globOn ) 
		   for ( i=0; i<16; i++ ) xyType[i] = atoi(mb.mtext);	
		 else
		   xyType[chSelect0] = atoi(mb.mtext);
                 switch ( xyType[chSelect0] ) {
                   case 0: 
		     sprintf ( msgout, "New Y Scale Type for Ch.%d: Linear\n", chSelect0+1 );
		     printmessage(reset);
		   break;
                   case 1: 
		     sprintf ( msgout, "New Y Scale Type for Ch.%d: Log\n", chSelect0+1 );
		     printmessage(reset);
		   break;
                   case 2: 
		     sprintf ( msgout, "New Y Scale Type for Ch.%d: Ln\n", chSelect0+1 );
		     printmessage(reset);
		   break;
                   case 3: 
		     sprintf ( msgout, "New Y Scale Type for Ch.%d: Exp\n", chSelect0+1 );
		     printmessage(reset);
		   break;
                 }		 
		 firsttime = 1;
		 break;
	     case 8: /* x axis setting */
	         j = atoi(mb.mtext);
		 if ( j>0 ) {
		    dPoints = ADC_BUFF_SIZE - 128 * j;
		    if ( globOn ) {
		      for ( i=0; i<16; i++ ) 
			winXScaler[i] = 1.0*j;
		    }
		    else
		      winXScaler[chSelect0] = 1.0*j;
		    sprintf ( msgout, "New X axis time sequence %d sec\n", j );
		    printmessage(reset);
		 }
		 else {
		    if ( globOn ) {
		      for ( i=0; i<16; i++ ) 
			winXScaler[i] = -1.0/j;
		    }
		    else
		      winXScaler[chSelect0] = -1.0/j;
		    sprintf ( msgout, "New X axis time sequence 1/%d sec\n", (-j) );
		    printmessage(reset);
		 }
		 if ( chSelect != chSelect0 && !reset ) 
		   changeConfigure();
		 firsttime = 1;
		 if ( graphOption0 == GRAPHTIME ) 
		    screenChange = 1;
		 break;
	     case 9: /* channel highlight */
	         j = atoi(mb.mtext);
		 chSelect0 = j;
		 /* test if already selected */
		 if (!reset) {
		   chfound = -1; 
		   for ( i=0; i<windowNum; i++ ) {
		     if (chMarked[i] < chSelect0 ) ;
		     else if (chMarked[i] == chSelect0 ) {
		       chSelect_seq = i;
		       chfound = 1;
		     }
		     else if (chMarked[i] > chSelect0 && chfound<0) {
		       chfound = 0;
		       chinsert = i;
		     }		     
		   }
		   if (chfound < 0) {
		     chMarked[windowNum] = chSelect0;
		     chSelect_seq = windowNum;
		     windowNum++;
		   }
		   else if (chfound == 0) {
		     for ( i=windowNum; i>chinsert; i-- ) {
		       chMarked[i] = chMarked[i-1];
		     }
		     chMarked[chinsert] = chSelect0;
		     chSelect_seq = chinsert;
		     windowNum++;
		   }
		 }

		 if (reset) {
		    chNameChange[chSelect0] = 1;
		    sprintf ( msgout, "Resetting for CH.%d ...\n", chSelect0+1 );
		    printmessage(reset);
		 }
		 else if ( graphOption0 == GRAPHTREND ) {
		    firsttime = 1;
		    trailer = 0;
		    sprintf ( msgout, "New channel select: %d\n", chSelect0+1 );
		    printmessage(reset);
		    chSelect = chSelect0;
		    screenChange = 1;
		    changeConfigure();
		 }
		 else {
		    sprintf ( msgout, "New channel select %d\n", chSelect0+1 );
		    printmessage(reset);
		    chNameChange[chSelect0] = 1;
		    screenChange = 1;
		    changeConfigure();
		 }
		 /*
		 else
		    sprintf ( msgout, "New Chan Error: channel number must be <= %d\n", windowNum );	
		 */       
		 break;
	     case 10: /* Changing signal */
	         chChange = atoi(mb.mtext);
		 sprintf ( msgout, "New changing channel %d\n", chChange );
		 printmessage(reset);
		 break;
	     case 11: /* Restoring  */
	         reset = atoi(mb.mtext);
		 if ( reset ) {
		    globOn = 0;
		    resetReadMode = newReadMode;
		    sprintf ( msgout, "New: restoring\n" );
		    printmessage(1);
		    newReadMode = STOPMODE;
		    changeReadMode();
		 }
		 else { /* finished */
		    for ( i=0; i<16; i++ ) chNameChange[i] = 1;	
		    screenChange = 1;
		    newReadMode = resetReadMode;
		    if ( newReadMode == REALTIME)
		       stopped = 0;
		    changeConfigure();
		    sprintf ( msgout, "New: finished restoring\n" );
		    printmessage(0);
		 }
		 break;
	     case 12: /* X delay */
	         sprintf ( msgout, "New delay for Ch.%d: %s\n", chSelect0+1, mb.mtext );
		 winDelay[chSelect0] =  atoi(mb.mtext);
		 if ( chSelect != chSelect0 && !reset ) 
		   changeConfigure();
		 firsttime = 1;
		 if ( graphOption0 == GRAPHTIME ) 
		    screenChange = 1;
		 break;
	     case 13: /* Chan Unit */
	         strcpy(chUnit[chChange-1], mb.mtext);
		 sprintf ( msgout, "Unit %s for Ch.%d\n", chUnit[chChange-1], chChange );
		 printmessage(reset);
		 break;
	     case 14: /* Chan Unit */
	         j = atoi(mb.mtext);
		 if ( globOn ) 
		   for ( i=0; i<16; i++ ) uniton[i] = j;	
		 else
		   uniton[chSelect0] = j;
		 firsttime = 1;
	         if (j)
		   sprintf ( msgout, "engineering unit conversion on\n" );
		 else
		   sprintf ( msgout, "engineering unit conversion off\n" );
		 printmessage(reset);
	 	break;
	     case 15: /* global setting */
	         globOn = atoi(mb.mtext);
	         if (globOn)
		   sprintf ( msgout, "Global on\n" );
		 else
		   sprintf ( msgout, "Global off\n" );
		 printmessage(reset);
		 break;
	     case 16: /* Auto */
	         j = atoi(mb.mtext);
		 if ( globOn ) 
		   for ( i=0; i<16; i++ ) autoon[i] = j;	
		 else
		   autoon[chSelect0] = j;
		 firsttime = 1;
	         if (j)
		   sprintf ( msgout, "auto on\n" );
		 else
		   sprintf ( msgout, "auto off\n" );
		 printmessage(reset);
	 	break;
	     case 17: /* filter flag */
	         filterOn = atoi(mb.mtext);
		 if ( !reset ) 
		   changeConfigure();
		 break;
	     case 22: /* reset Xmgrace */
		 /* Re-onnect to Data Server */
		 while ( DataReConnect(serverIP, serverPort )) {
		   i++;
		   if ( i>25 ) {
		     sprintf ( msgout, "can't re-connect to the data server after 25 tries\n" );
		     printmessage(-2);
		     break;
		   }
		 }
		 sprintf ( msgout, "Done re-connecting to the data server\n" );
		 printmessage(1);
	         graphini();
		 sprintf ( msgout, "Reset Xmgrace\n" );
		 printmessage(1);
		 break;
	     case 23: /* Quit */
	         j = atoi(mb.mtext);
		 if ( j == 1 )
		    quitdisplay(1);
	         else if ( stopped || trailer )
		    quitdisplay(0);
		 else
		    quitting = 1;
		 break;
	     case 24: /* Window number select */
                 sscanf ( mb.mtext, " %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d ", &windowNum, &chMarked[0], &chMarked[1], &chMarked[2], &chMarked[3], &chMarked[4], &chMarked[5], &chMarked[6], &chMarked[7], &chMarked[8], &chMarked[9], &chMarked[10], &chMarked[11], &chMarked[12], &chMarked[13], &chMarked[14], &chMarked[15] );
		 sprintf ( msgout, "Total Channel Number: %d\n", windowNum );
		 printmessage(reset);
		 screenChange = 1;
		 if ( !reset ) 
		   changeConfigure();
		 break;
	     case 25: /* Resolution Select */
                 resolution = atoi(mb.mtext) * 128;
		 if ( !reset ) 
		   changeConfigure();
		 for ( i=0; i<16; i++ ) {
                    if ( resolution <= 2*chSize[i] ) 
                       graphRate[i] = resolution;
                    else 
                       graphRate[i] = 2*chSize[i];
                 }
		 sprintf ( msgout, "New resolution %d Hz\n", resolution );
		 printmessage(reset);
		 firsttime = 1;
		 break;
	     case 26: /* Refresh Rate Select */
	         j = atoi(mb.mtext);
		 if ( j == 1 )
		    refreshRate = 1;
		 else
		    refreshRate = 16;
		 blocksize = 16/j;
		 if ( !reset ) 
		    changeConfigure();
		 sprintf ( msgout, "New Refresh Rate %d/Sec\n", j );
		 printmessage(reset);
		 firsttime = 1;
		 break;
	     case 27: /* Print Hardcopy: 0 to file, 1 to printer */
	         printing = atoi(mb.mtext);
		 if ( stopped || trailer ) /* else do in readdata */
		    printgraph(); 
		 break;
	     case 28: /* Color Select */
	         j = atoi(mb.mtext);
		 if (j != 16) {
		   gColor[chSelect0] = j;
		   sprintf ( msgout, "New Color for Ch.%d\n", (chSelect0+1) );
		   printmessage(reset);
		 }
		 else {
		   sprintf ( msgout, "New Color: default\n" );
		   printmessage(reset);
		   for ( i=0; i<16; i++ ) {
                      gColor[i] = gColorDefault[i];
                   }
		 }
		 break;
	     case 36: /* Trigger On Select */
	         trigOn = atoi(mb.mtext);
		 if ( trigOn ) 
		    sprintf ( msgout, "Trigger On\n" );
		 else
		    sprintf ( msgout, "Trigger Off\n" );
		 printmessage(reset);
		 if ( !reset ) 
		    changeConfigure();
		 break;
	     case 37: /* Trigger Chan Select */
	         chanTrig0 = atoi(mb.mtext);
		 sprintf ( msgout, "NEW Trigger Chan %d\n", chanTrig0 );
		 printmessage(reset);
		 if ( !reset ) 
		    changeConfigure();
		 break;
	     case 39: /* Trigger Method Select */
	         trigHow = atoi(mb.mtext);
                 switch ( trigHow ) {
                   case TRIGABOVE: 
		     sprintf ( msgout, "NEW Trigger Method: Above\n" );
		     printmessage(reset);
		     break;
                   case TRIGBELOW: 
		     sprintf ( msgout, "NEW Trigger Method: Below\n" );
		     printmessage(reset);
		     break;
                 }		 
		 break;
	     case 40: /* Trigger Level Select */
	         trigLevel = atof(mb.mtext);
		 sprintf ( msgout, "NEW Trigger Level %f ADC\n", trigLevel );
		 printmessage(reset);
		 break;
	     case 48: /* Signal On Select */
	         sigOn = atoi(mb.mtext);
		 if ( sigOn )
		    sprintf ( msgout, "Show large window\n" );
		 else
		    sprintf ( msgout, "Not show large window\n" );
		 printmessage(reset);
		 firsttime = 1;
		 break;
	     default:
	         sprintf ( msgout, "message type unknown\n" );
		 printmessage(reset);
		 break;
	   }
	}
}

char *
complexSuffix(int t)
{
	static char *suf[5] = {" (real)", " (img)", " (mag)", " (phs)", "" };
	if (t >= 0 && t < 16) {
	   t = complexDataType[t];
	   switch (t) {
		case 1:
		case 2:
		case 3:
		case 4: return suf[t-1];
	   }
	}
	return suf[4];
}

void graphout()   
{
double x1,y1;
double x0[16], y0[16], w, h;
double xtickmajor,xtickminor,ytickmajor,ytickminor;
double xmaximum,xminimum,ymaximum,yminimum;
float  logmin, logmaj, rat[16], fj; 
int i;

        if ( initGraph ) {
	  initGraph = 0;
	  GracePrintf( "focus off" );
	}
	for ( i=0; i<16; i++ ) {
	  x0[i] = 0.0;
	  y0[i] = 0.0;
	}
	if ( firsttime || screenChange ) {
	   GracePrintf( "background color 7" );
	   if ( sigOn || graphOption != GRAPHTIME ) {
	     switch ( windowNum ) {
	     case 1: 
	         x0[0] = 0.06; 
		 y0[0] = 0.06;
		 w = 1.18;
		 h = 0.38;
		 break;
	     case 2: 
	         x0[0] = 0.06; y0[0] = 0.08; 
		 x0[1] = 0.68; y0[1] = 0.08; 
		 w = 0.56;
		 h = 0.345;
		 break;
	     case 3: 
	         x0[0] = 0.06; y0[0] = 0.09; 
		 x0[1] = 0.68; y0[1] = 0.09; 
		 x0[2] = 0.68; y0[2] = 0.55;
		 w = 0.56;
		 h = 0.36;
		 break;
	     case 4: 
	     case 5: 
	         x0[0] = 0.06; y0[0] = 0.06; 
		 x0[1] = 0.06; y0[1] = 0.35; 
		 x0[2] = 0.68; y0[2] = 0.06;
		 x0[3] = 0.68; y0[3] = 0.35; 
		 x0[4] = 0.68; y0[4] = 0.64; 
		 w = 0.56;
		 h = 0.23;
		 break;
	     case 6: 
	     case 7: 
	         x0[0] = 0.06; y0[0] = 0.06; 
		 x0[1] = 0.06; y0[1] = 0.34; 
		 x0[2] = 0.47; y0[2] = 0.06;
		 x0[3] = 0.47; y0[3] = 0.34; 
		 x0[4] = 0.88; y0[4] = 0.06; 
		 x0[5] = 0.88; y0[5] = 0.34;
		 x0[6] = 0.88; y0[6] = 0.62; 
		 w = 0.36;
		 h = 0.22;
		 break;
	     case 8: 
	         x0[0] = 0.06; y0[0] = 0.06; 
		 x0[1] = 0.06; y0[1] = 0.35; 
		 x0[2] = 0.37; y0[2] = 0.06;
		 x0[3] = 0.37; y0[3] = 0.35; 
		 x0[4] = 0.68; y0[4] = 0.06; 
		 x0[5] = 0.68; y0[5] = 0.35;
		 x0[6] = 0.98; y0[6] = 0.06; 
		 x0[7] = 0.98; y0[7] = 0.35;
		 w = 0.263;
		 h = 0.22;
		 break;
	     case 9: 
	     case 10: 
	         x0[0] = 0.06; y0[0] = 0.06; 
		 x0[1] = 0.06; y0[1] = 0.35; 
		 x0[2] = 0.37; y0[2] = 0.06;
		 x0[3] = 0.37; y0[3] = 0.35; 
		 x0[4] = 0.68; y0[4] = 0.06; 
		 x0[5] = 0.68; y0[5] = 0.35;
		 x0[6] = 0.68; y0[6] = 0.64; 
		 x0[7] = 0.98; y0[7] = 0.06; 
		 x0[8] = 0.98; y0[8] = 0.35;
		 x0[9] = 0.98; y0[9] = 0.64; 
		 w = 0.263;
		 h = 0.22;
		 break;
	     case 11: 
	     case 12: 
	         x0[0] = 0.06; y0[0] = 0.06; 
		 x0[1] = 0.06; y0[1] = 0.277; 
		 x0[2] = 0.37; y0[2] = 0.06; 
		 x0[3] = 0.37; y0[3] = 0.277; 
		 x0[4] = 0.68; y0[4] = 0.06; 
		 x0[5] = 0.68; y0[5] = 0.277; 
		 x0[6] = 0.68; y0[6] = 0.494;
		 x0[7] = 0.68; y0[7] = 0.711; 
		 x0[8] = 0.98; y0[8] = 0.06;
		 x0[9] = 0.98; y0[9] = 0.277; 
		 x0[10] = 0.98; y0[10] = 0.494; 
		 x0[11] = 0.98; y0[11] = 0.711;
		 w = 0.263;
		 h = 0.157;
		 break;
	     default: /* case 13-16  */
	         x0[0] = 0.06; y0[0] = 0.06; 
		 x0[1] = 0.06; y0[1] = 0.248; 
		 x0[2] = 0.06; y0[2] = 0.436;
		 x0[3] = 0.37; y0[3] = 0.06; 
		 x0[4] = 0.37; y0[4] = 0.248; 
		 x0[5] = 0.37; y0[5] = 0.436;
		 x0[6] = 0.68; y0[6] = 0.06; 
		 x0[7] = 0.68; y0[7] = 0.248; 
		 x0[8] = 0.68; y0[8] = 0.436;
		 x0[9] = 0.68; y0[9] = 0.624; 
		 x0[10] = 0.68; y0[10] = 0.812; 
		 x0[11] = 0.98; y0[11] = 0.06;
		 x0[12] = 0.98; y0[12] = 0.248; 
		 x0[13] = 0.98; y0[13] = 0.436; 
		 x0[14] = 0.98; y0[14] = 0.624;
		 x0[15] = 0.98; y0[15] = 0.812;
		 w = 0.263;
		 h = 0.128;
		 break;
	     }
	   }
	   else { /* not to show large window */
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
	   }
	   for ( i= 0; i<windowNum; i++ ) {
	      GracePrintf( "with g%d", i );
	      if (sigOn && windowNum > 3)
		GracePrintf( "subtitle size %f",0.7 );
	      else if (windowNum > 4)
		GracePrintf( "subtitle size %f",0.7 );
	      else
		GracePrintf( "subtitle size %f",0.9 );
	      GracePrintf( "subtitle font 1" );
	      if ( chstatus[chMarked[i]] == 0 )
		GracePrintf( "subtitle color 1" );
	      else
		GracePrintf( "subtitle color 2" );
	      if ( winXScaler[chMarked[i]] < 1) {
		xminimum = -1.0 + winDelay[chMarked[i]]/16.0;
		xmaximum = xminimum + winXScaler[chMarked[i]];
	      }
	      else if ( graphRate[chMarked[i]] < 2048 ) {
		if (maxSec < winXScaler[chMarked[i]])
		  xminimum = -maxSec;
		else
		  xminimum = -winXScaler[chMarked[i]];
		xmaximum = 0.0;
	      }
	      else {
		xminimum = -1.0;
		xmaximum = 0.0;
	      }
	      GracePrintf( "world xmin %f", xminimum );
	      GracePrintf( "world xmax %f", xmaximum );
	      GracePrintf( "xaxis tick color 7" );
	      if (windowNum <= 2)
		GracePrintf( "xaxis ticklabel char size 0.6" );
	      else if (windowNum <= 6)
		GracePrintf( "xaxis ticklabel char size 0.5" );
	      else
		GracePrintf( "xaxis ticklabel char size 0.43" );
	      fj = winXScaler[chMarked[i]];
	      if ( fj >= 2 ) {
		GracePrintf( "xaxis tick major 1" );
		GracePrintf( "xaxis tick minor 1" );
		GracePrintf( "xaxis ticklabel prec 1" );
		GracePrintf( "xaxis ticklabel format decimal" );
	      }
	      else if (fj == 1 ) {
		GracePrintf( "xaxis tick major 0.5" );
		GracePrintf( "xaxis tick minor 0.25" );
		GracePrintf( "xaxis ticklabel prec 3" );
		GracePrintf( "xaxis ticklabel format decimal" );
	      }
	      else { /* fj < 1 */
		GracePrintf( "xaxis tick major %le", (fj*0.5) );
		GracePrintf( "xaxis tick minor %le", (fj*0.25) );
		GracePrintf( "xaxis ticklabel prec 4" );
		GracePrintf( "xaxis ticklabel format decimal" );
	      }
	      if ( winScaler[chMarked[i]] > 0 ) {
		yminimum = winYMin[chMarked[i]];
		ymaximum = winYMax[chMarked[i]];
		GracePrintf( "world ymin %le", yminimum );
		GracePrintf( "world ymax %le", ymaximum );
		GracePrintf( "yaxis tick major %le", (winScaler[chMarked[i]]/ytick_maj) );
		GracePrintf( "yaxis tick minor %le", (winScaler[chMarked[i]]/ytick_min) );
	      }
	      GracePrintf( "yaxis tick color 7" );
	      if (windowNum <= 4)
		GracePrintf( "yaxis ticklabel char size 0.6" );
	      else
		GracePrintf( "yaxis ticklabel char size 0.43" );
	      GracePrintf( "yaxis ticklabel format decimal" );
	      GracePrintf( "g%d type xy", i );
	      if ( winScaler[chMarked[i]] < 0.0001) {
		 GracePrintf( "yaxis ticklabel format exponential" );
		 GracePrintf( "yaxis ticklabel prec 3" );
	      }
	      else if ( winScaler[chMarked[i]] < 0.01)
		 GracePrintf( "yaxis ticklabel prec 6" );
	      else if ( winScaler[chMarked[i]] < 20.0)
		 GracePrintf( "yaxis ticklabel prec 2" );
	      else
		 GracePrintf( "yaxis ticklabel prec 0" );
	      if (uniton[chMarked[i]]
		  && strcasecmp(chUnit[chMarked[i]], "none") && strcasecmp(chUnit[chMarked[i]], "undef")) 
		GracePrintf( "yaxis label \"%s\" ", chUnit[chMarked[i]] );
 	      else
		GracePrintf( "yaxis label \"\" ");
	      GracePrintf( "yaxis label layout para" );
	      GracePrintf( "yaxis label place auto" );
	      GracePrintf( "yaxis label font 4" );
	      if (windowNum <= 4)
		  GracePrintf( "yaxis label char size 0.9" );
	      else
		  GracePrintf( "yaxis label char size 0.7" );
	      GracePrintf( "view %f, %f,%f,%f", x0[i]+XSHIFT,y0[i],x0[i]+w,y0[i]+h );
	      GracePrintf( "frame color 1" );
	      GracePrintf( "frame background color 1" );
	      GracePrintf( "frame fill on" );
              switch ( xyType[chMarked[i]] ) {
                case 1: /* Log */
		    if ( winYMax[chMarked[i]] <= ZEROLOG )
		       winScaler[chMarked[i]] = -1.0;
		    else if ( winYMin[chMarked[i]] < ZEROLOG ) {
		       GracePrintf( "world ymin %le", ZEROLOG );
		       rat[i] = winYMax[chMarked[i]]/ZEROLOG;
		    }
		    else {
		       rat[i] = winYMax[chMarked[i]]/winYMin[chMarked[i]];
		    }
		    if ( rat[i] > 100 ) {
		       logmaj = 1.0;
		       logmin = 2.0;
		    }
		    else {
		       logmaj = 1.0;
		       logmin = 1.0;
		       /*logmaj = 0.2;
			 logmin = 0.0;*/
		    }
		    GracePrintf( "g%d type logy", i );
		    GracePrintf( "yaxis tick major %f", logmaj );
		    GracePrintf( "yaxis tick minor %f", logmin );
		    GracePrintf( "yaxis ticklabel format exponential" );
		    /*GracePrintf( "yaxis ticklabel prepend \"\\-1.0E\"" );*/
                    break;
                case 2: /* Ln */
                    break;
                case 3: /* exp */
		    GracePrintf( "yaxis ticklabel format exponential" );
		    GracePrintf( "yaxis ticklabel prec 5" );
                    break;
                default: /* Linear */ 
		    if ( winScaler[chMarked[i]] <0.001 || winScaler[chMarked[i]] > 10000000.0 )
		       GracePrintf( "yaxis ticklabel format exponential" );
                    break;
              }	      
	   }
	   if (sigOn || graphOption != GRAPHTIME ) {
	     GracePrintf( "with g16" ); 
	     w = 0.58;
	     h = 0.28;
	     x1 = 0.06;
	     y1 = 0.64;
	     switch ( windowNum ) {
             case 1: 
             case 2: 
	         w = 1.18;
	         h = 0.38;
		 x1 = 0.06;
		 y1 = 0.55;
                 break;
             case 3: 
	         w = 0.56;
                 y1 = 0.55;
	         h = 0.36;
                 break;
             case 4: 
	         w = 1.18;
                 break;
             case 5: 
	         x1 = 0.05;
	         h = 0.265;
                 break;
             case 6: 
	         w = 1.18;
		 h = 0.27;
                 break;
             case 7: 
	         x1 = 0.05;
	         w = 0.78;
		 h = 0.27;
                 break;
             case 8: 
	         w = 1.18;
	         h = 0.28;
                 break;
             case 9: 
             case 10: 
	         h = 0.28;
                 break;
             case 11: 
             case 12: 
	         h = 0.38;
		 y1 = 0.51;
                 break;
                 break;
             default: /* 13-16 */
                 break;
	     }
	     GracePrintf( "legend off" );
	     GracePrintf( "with string 0" );
	     GracePrintf( "string off" );
	     GracePrintf( "string loctype view" );
	     GracePrintf( "string 0.03, 0.94" );
	     GracePrintf( "string font 4" );
	     GracePrintf( "string char size 0.9" );
	     GracePrintf( "string color 1" );
	     
	     /* set up x-axis for amplitude */
	     GracePrintf( "subtitle size %f",0.9 );
	     GracePrintf( "subtitle font 2" );
	     if ( chstatus[chSelect] == 0 )
	       GracePrintf( "subtitle color 1" );
	     else
	       GracePrintf( "subtitle color 2" );
	     if ( winXScaler[chSelect] < 1) {
	       xminimum = -1.0 + winDelay[chSelect]/16.0;
	       xmaximum = xminimum + winXScaler[chSelect];
	     }
	     else if ( graphRate[chSelect] < 2048 ) {
	       xminimum = -winXScaler[chSelect];
	       xmaximum = 0.0;
	     }
	     else {
	       xminimum = -1.0;
	       xmaximum = 0.0;
	     }
	     yminimum = winYMin[chSelect];
	     ymaximum = winYMax[chSelect];
	     xtickmajor = 1.0;
	     xtickminor = 1.0;
	     ytickmajor = winScaler[chSelect]/ytick_maj;
	     ytickminor = winScaler[chSelect]/ytick_min;
	   GracePrintf( "world xmin %f", xminimum );
	   GracePrintf( "world xmax %f", xmaximum );
	   GracePrintf( "xaxis tick major %f", xtickmajor );
	   GracePrintf( "xaxis tick minor %f", xtickminor );
	   GracePrintf( "xaxis tick color 7" );
	   GracePrintf( "xaxis ticklabel char size 0.6" );
	   GracePrintf( "xaxis ticklabel format decimal" );
	   GracePrintf( "xaxis ticklabel prec 3" );
	   if ( winScaler[chSelect] > 0 ) {
	     GracePrintf( "world ymin %le", yminimum );
	     GracePrintf( "world ymax %le", ymaximum );
	     GracePrintf( "yaxis tick major %le", ytickmajor );
	     GracePrintf( "yaxis tick minor %le", ytickminor );	    
	   }
	   GracePrintf( "yaxis tick color 7" );
	   GracePrintf( "yaxis ticklabel char size 0.6" );
	   GracePrintf( "yaxis ticklabel format decimal" );
	   GracePrintf( "g16 type xy" );
	   if ( winScaler[chSelect] < 0.0001) {
	     GracePrintf( "yaxis ticklabel format exponential" );
	     GracePrintf( "yaxis ticklabel prec 3" );
	   }
	   else if ( winScaler[chSelect] < 0.01)
	      GracePrintf( "yaxis ticklabel prec 6" );
	   else if ( winScaler[chSelect] < 20.0)
	      GracePrintf( "yaxis ticklabel prec 2" );
	   else
	      GracePrintf( "yaxis ticklabel prec 0" );
	   if (uniton[chSelect]
	       && strcasecmp(chUnit[chMarked[chSelect]], "none") && strcasecmp(chUnit[chMarked[i]], "undef"))
	     GracePrintf( "yaxis label \"%s\" ", chUnit[chSelect] );
	   else
	     GracePrintf( "yaxis label \"\" ");
	   GracePrintf( "yaxis label layout para" );
	   GracePrintf( "yaxis label place auto" );
	   GracePrintf( "yaxis label font 4" );
	   GracePrintf( "yaxis label char size 0.9" );
	   GracePrintf( "view %f, %f,%f,%f",x1+XSHIFT,y1,x1+w,y1+h );
	   GracePrintf( "frame color 1" );
	   GracePrintf( "frame background color 1" );
	   GracePrintf( "frame fill on" );
	   switch ( xyType[chSelect] ) {
	   case 1: /* Log */
	     if ( winYMax[chSelect] <= ZEROLOG )
	       winScaler[chSelect] = -1.0;
	     else if ( winYMin[chSelect] < ZEROLOG ) {
	       GracePrintf( "world ymin %le", ZEROLOG );
	     }
	     if ( rat[chSelect] > 100 ) {
	        logmaj = 1.0;
		logmin = 2.0;
	     }
	     else {
	        logmaj = 1.0;
		logmin = 1.0;
	     }
	     GracePrintf( "g16 type logy" );
	     GracePrintf( "yaxis tick major %f", logmaj );
	     GracePrintf( "yaxis tick minor %f", logmin );
	     GracePrintf( "yaxis ticklabel format exponential" );
	     break;
	   case 2: /* Ln */
	     break;
	   case 3: /* exp */
	     GracePrintf( "yaxis ticklabel format exponential" );
	     GracePrintf( "yaxis ticklabel prec 5" );
	     break;
	   default: /* Linear */ 
	     if ( winScaler[chSelect] < 0.001 || winScaler[chSelect] > 10000000.0 )
	       GracePrintf( "yaxis ticklabel format exponential" );
	     break;
	   }	      
	   }
	}

	/** g0 **/
	GracePrintf( "with g0" );
	for ( i=0; i<windowNum; i++ ) {
	   GracePrintf( "s%d color %d", i, gColor[chMarked[i]] );      
	}
	for ( i=1; i<windowNum; i++ ) {
	   GracePrintf( "with g%d", i );
	   GracePrintf( "move g0.s%d to g%d.s0", i, i );         
	}
	GracePrintf( "with g0" );
	/* Don't know why it's needed. otherwise it goes to auto */
	if ( winXScaler[chMarked[0]] < 1) {
	  xminimum = -1.0 + winDelay[chMarked[0]]/16.0;
	  xmaximum = xminimum + winXScaler[chMarked[0]];
	}
	else if ( graphRate[chMarked[0]] < 2048 ) {
	  if (maxSec < winXScaler[chMarked[0]])
	    xminimum = -maxSec;
	  else
	    xminimum = -winXScaler[chMarked[0]];
	  xmaximum = 0.0;
	}
	else {
	  xminimum = -1.0;
	  xmaximum = 0.0;
	}
	if (windowNum <= 2)
	  GracePrintf( "xaxis ticklabel char size 0.6" );
	else if (windowNum <= 6)
	  GracePrintf( "xaxis ticklabel char size 0.5" );
	else
	  GracePrintf( "xaxis ticklabel char size 0.43" );
	fj = winXScaler[chMarked[0]];
	if ( fj >= 2 ) {
	  GracePrintf( "xaxis tick major 1" );
	  GracePrintf( "xaxis tick minor 1" );
	  GracePrintf( "xaxis ticklabel prec 1" );
	  GracePrintf( "xaxis ticklabel format decimal" );
	}
	else if (fj == 1 ) {
	  GracePrintf( "xaxis tick major 0.5" );
	  GracePrintf( "xaxis tick minor 0.25" );
	  GracePrintf( "xaxis ticklabel prec 3" );
	  GracePrintf( "xaxis ticklabel format decimal" );
	}
	else { /* fj < 1 */
	  GracePrintf( "xaxis tick major %le", (fj*0.5) );
	  GracePrintf( "xaxis tick minor %le", (fj*0.25) );
	  GracePrintf( "xaxis ticklabel prec 4" );
	  GracePrintf( "xaxis ticklabel format decimal" );
	}
	yminimum = winYMin[0];
	ymaximum = winYMax[0];
	xtickmajor = 1.0;
	xtickminor = 1.0;
	ytickmajor = winScaler[0]/ytick_maj;
	ytickminor = winScaler[0]/ytick_min;
	GracePrintf( "world xmin %f", xminimum );
	GracePrintf( "world xmax %f", xmaximum );
	if ( winScaler[0] > 0 ) {
	  GracePrintf( "world ymin %le", yminimum );
	  GracePrintf( "world ymax %le", ymaximum );
	  GracePrintf( "yaxis tick major %le", ytickmajor );
	  GracePrintf( "yaxis tick minor %le", ytickminor );	    
	}

	if ( autoon[chMarked[0]] ) 
	   GracePrintf( "autoscale yaxes" );
	GracePrintf( "subtitle \"\\-Ch %d:  %s%s\"", (chMarked[0]+1), chName[chMarked[0]], complexSuffix(chMarked[0]) );
	GracePrintf( "string off" );
	GracePrintf( "clear stack" );
	/** g1--g15 **/
	for ( i=1; i<windowNum; i++ ) {
	   GracePrintf( "with g%d", i );
	   if ( autoon[chMarked[i]] ) {
	     GracePrintf( "autoscale yaxes" );
	   }
	   GracePrintf( "subtitle \"\\-Ch %d:  %s%s\"", (chMarked[i]+1), chName[chMarked[i]], complexSuffix(chMarked[i]) );
	   GracePrintf( "clear stack" );
	}
	/** g16 **/
	if ( sigOn || graphOption != GRAPHTIME ) {
	  GracePrintf( "with g16" );
	  GracePrintf( "copy g%d.s0 to g16.s0", chSelect_seq );
	  /*if ( chType[chSelect] == 1 ) 
	    GracePrintf( "s0 color %d", gColor[chSelect] );*/      
	  if ( autoon[chSelect] ) GracePrintf( "autoscale yaxes" );
	  GracePrintf( "subtitle \"\\1 Ch %d:  %s%s       %s\"",(chSelect+1),chName[chSelect], complexSuffix(chSelect), timestring );
	  GracePrintf( "g16 on" );
	  GracePrintf( "string off" );
	}
	else {
	   GracePrintf( "g16 off" );
	   if ( windowNum == 1 )
	     GracePrintf( "string def \"\\1 DAQS Data Display %d Channel at %s\"", windowNum, timestring );
	   else
	     GracePrintf( "string def \"\\1 DAQS Data Display %d Channels at %s\"", windowNum, timestring );
	   GracePrintf( "string loctype view" );
	   GracePrintf( "string 0.32, 0.94" );
	   GracePrintf( "string font 4" );
	   GracePrintf( "string char size 0.9" );
	   GracePrintf( "string color 1" );
	   GracePrintf( "string on" );
	}
	GracePrintf( "clear stack" );

	for ( i=0; i<windowNum; i++ ) {
	   GracePrintf( "g%d on", i );
	}
	for ( i=windowNum; i<16; i++ ) { 
	  GracePrintf( "g%d off", i );         
	}

	GracePrintf( "redraw" );
	/*GracePrintf( "with g0" );*/
	firsttime = 0;
        if ( warning ) {
	   sprintf ( msgout,"  0 as input of log\n"  );
	   printmessage(-1);
	}
	warning = 0;
        if ( screenChange > 3 ) 
	   screenChange = 0;
	fflush(stdout);
	GraceFlush();
}


void graphtrend(int from, int to)   
{
double x1,y1;
double x0[16], y0[16], w, h;
double ymaximum,yminimum;
float  logmin, logmaj, rat[16]; 
int i;

        if ( initGraph ) {
	  initGraph = 0;
	  GracePrintf( "focus off" );
	}
	for ( i=0; i<16; i++ ) {
	  x0[i] = 0.0;
	  y0[i] = 0.0;
	}
	if ( firsttime || screenChange ) {
	   GracePrintf( "background color 7" );
	   if ( sigOn ) {
	     switch ( windowNum ) {
	     case 1: 
	         x0[0] = 0.06; 
		 y0[0] = 0.06;
		 w = 1.18;
		 h = 0.38;
		 break;
	     case 2: 
	         x0[0] = 0.06; y0[0] = 0.08; 
		 x0[1] = 0.68; y0[1] = 0.08; 
		 w = 0.56;
		 h = 0.345;
		 break;
	     case 3: 
	         x0[0] = 0.06; y0[0] = 0.09; 
		 x0[1] = 0.68; y0[1] = 0.09; 
		 x0[2] = 0.68; y0[2] = 0.55;
		 w = 0.56;
		 h = 0.36;
		 break;
	     case 4: 
	     case 5: 
	         x0[0] = 0.06; y0[0] = 0.06; 
		 x0[1] = 0.06; y0[1] = 0.35; 
		 x0[2] = 0.68; y0[2] = 0.06;
		 x0[3] = 0.68; y0[3] = 0.35; 
		 x0[4] = 0.68; y0[4] = 0.64; 
		 w = 0.56;
		 h = 0.23;
		 break;
	     case 6: 
	     case 7: 
	         x0[0] = 0.06; y0[0] = 0.06; 
		 x0[1] = 0.06; y0[1] = 0.34; 
		 x0[2] = 0.47; y0[2] = 0.06;
		 x0[3] = 0.47; y0[3] = 0.34; 
		 x0[4] = 0.88; y0[4] = 0.06; 
		 x0[5] = 0.88; y0[5] = 0.34;
		 x0[6] = 0.88; y0[6] = 0.62; 
		 w = 0.36;
		 h = 0.22;
		 break;
	     case 8: 
	         x0[0] = 0.06; y0[0] = 0.06; 
		 x0[1] = 0.06; y0[1] = 0.35; 
		 x0[2] = 0.37; y0[2] = 0.06;
		 x0[3] = 0.37; y0[3] = 0.35; 
		 x0[4] = 0.68; y0[4] = 0.06; 
		 x0[5] = 0.68; y0[5] = 0.35;
		 x0[6] = 0.98; y0[6] = 0.06; 
		 x0[7] = 0.98; y0[7] = 0.35;
		 w = 0.263;
		 h = 0.22;
		 break;
	     case 9: 
	     case 10: 
	         x0[0] = 0.06; y0[0] = 0.06; 
		 x0[1] = 0.06; y0[1] = 0.35; 
		 x0[2] = 0.37; y0[2] = 0.06;
		 x0[3] = 0.37; y0[3] = 0.35; 
		 x0[4] = 0.68; y0[4] = 0.06; 
		 x0[5] = 0.68; y0[5] = 0.35;
		 x0[6] = 0.68; y0[6] = 0.64; 
		 x0[7] = 0.98; y0[7] = 0.06; 
		 x0[8] = 0.98; y0[8] = 0.35;
		 x0[9] = 0.98; y0[9] = 0.64; 
		 w = 0.263;
		 h = 0.22;
		 break;
	     case 11: 
	     case 12: 
	         x0[0] = 0.06; y0[0] = 0.06; 
		 x0[1] = 0.06; y0[1] = 0.277; 
		 x0[2] = 0.37; y0[2] = 0.06; 
		 x0[3] = 0.37; y0[3] = 0.277; 
		 x0[4] = 0.68; y0[4] = 0.06; 
		 x0[5] = 0.68; y0[5] = 0.277; 
		 x0[6] = 0.68; y0[6] = 0.494;
		 x0[7] = 0.68; y0[7] = 0.711; 
		 x0[8] = 0.98; y0[8] = 0.06;
		 x0[9] = 0.98; y0[9] = 0.277; 
		 x0[10] = 0.98; y0[10] = 0.494; 
		 x0[11] = 0.98; y0[11] = 0.711;
		 w = 0.263;
		 h = 0.157;
		 break;
	     default: /* case 13-16  */
	         x0[0] = 0.06; y0[0] = 0.06; 
		 x0[1] = 0.06; y0[1] = 0.248; 
		 x0[2] = 0.06; y0[2] = 0.436;
		 x0[3] = 0.37; y0[3] = 0.06; 
		 x0[4] = 0.37; y0[4] = 0.248; 
		 x0[5] = 0.37; y0[5] = 0.436;
		 x0[6] = 0.68; y0[6] = 0.06; 
		 x0[7] = 0.68; y0[7] = 0.248; 
		 x0[8] = 0.68; y0[8] = 0.436;
		 x0[9] = 0.68; y0[9] = 0.624; 
		 x0[10] = 0.68; y0[10] = 0.812; 
		 x0[11] = 0.98; y0[11] = 0.06;
		 x0[12] = 0.98; y0[12] = 0.248; 
		 x0[13] = 0.98; y0[13] = 0.436; 
		 x0[14] = 0.98; y0[14] = 0.624;
		 x0[15] = 0.98; y0[15] = 0.812;
		 w = 0.263;
		 h = 0.128;
		 break;
	     }
	   }
	   else { /* not to show large window */
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
	   }
	   for ( i= 0; i<windowNum; i++ ) {
	      GracePrintf( "with g%d", i );
	      if (sigOn && windowNum > 3)
		GracePrintf( "subtitle size %f",0.7 );
	      else if (windowNum > 4)
		GracePrintf( "subtitle size %f",0.7 );
	      else
		GracePrintf( "subtitle size %f",0.9 );
	      GracePrintf( "subtitle font 2" );
	      GracePrintf( "subtitle color 1" );
	      GracePrintf( "frame color 1" );
	      GracePrintf( "xaxis tick color 7" );
	      GracePrintf( "xaxis ticklabel prec 0" );
	      GracePrintf( "xaxis ticklabel char size 0.43" );
	      GracePrintf( "xaxis ticklabel format decimal" );
	      if ( winScaler[chMarked[i]] > 0 ) {
		yminimum = winYMin[chMarked[i]];
		ymaximum = winYMax[chMarked[i]];
		GracePrintf( "world ymin %le", yminimum );
		GracePrintf( "world ymax %le", ymaximum );
		GracePrintf( "yaxis tick major %le", (winScaler[chMarked[i]]/ytick_maj) );
		GracePrintf( "yaxis tick minor %le", (winScaler[chMarked[i]]/ytick_min) );
	      }
	      GracePrintf( "yaxis tick color 7" );
	      GracePrintf( "yaxis ticklabel char size 0.43" );
	      GracePrintf( "yaxis ticklabel format decimal" );
	      GracePrintf( "g%d type xy", i );
	      if ( winScaler[chMarked[i]] < 0.0001) {
		 GracePrintf( "yaxis ticklabel format exponential" );
		 GracePrintf( "yaxis ticklabel prec 3" );
	      }
	      else if ( winScaler[chMarked[i]] < 0.01)
		 GracePrintf( "yaxis ticklabel prec 6" );
	      else if ( winScaler[chMarked[i]] < 20.0)
		 GracePrintf( "yaxis ticklabel prec 2" );
	      else
		 GracePrintf( "yaxis ticklabel prec 0" );
	      GracePrintf( "view %f, %f,%f,%f", x0[i]+XSHIFT,y0[i],x0[i]+w,y0[i]+h );
	      GracePrintf( "frame color 1" );
	      GracePrintf( "frame background color 1" );
	      GracePrintf( "frame fill on" );
              switch ( xyType[chMarked[i]] ) {
                case 1: /* Log */
		    if ( winYMax[chMarked[i]] <= ZEROLOG )
		       winScaler[chMarked[i]] = -1.0;
		    else if ( winYMin[chMarked[i]] < ZEROLOG ) {
		       GracePrintf( "world ymin %le", ZEROLOG );
		       rat[i] = winYMax[chMarked[i]]/ZEROLOG;
		    }
		    else {
		       rat[i] = winYMax[chMarked[i]]/winYMin[chMarked[i]];
		    }
		    if ( rat[i] > 100 ) {
		       logmaj = 1.0;
		       logmin = 2.0;
		    }
		    else {
		       logmaj = 1.0;
		       logmin = 1.0;
		    }
		    GracePrintf( "g%d type logy", i );
		    GracePrintf( "yaxis tick major %f", logmaj );
		    GracePrintf( "yaxis tick minor %f", logmin );
		    GracePrintf( "yaxis ticklabel format exponential" );
                    break;
                case 2: /* Ln */
                    break;
                case 3: /* exp */
		    GracePrintf( "yaxis ticklabel format exponential" );
		    GracePrintf( "yaxis ticklabel prec 5" );
                    break;
                default: /* Linear */ 
		    if ( winScaler[chMarked[i]] <0.001 || winScaler[chMarked[i]] > 10000000.0 )
		       GracePrintf( "yaxis ticklabel format exponential" );
                    break;
              }	      
	   }
	   if (sigOn) {
	     GracePrintf( "with g16" ); 
	     w = 0.58;
	     h = 0.28;
	     x1 = 0.06;
	     y1 = 0.64;
	     switch ( windowNum ) {
             case 1: 
             case 2: 
	         w = 1.18;
	         h = 0.38;
		 x1 = 0.06;
		 y1 = 0.55;
                 break;
             case 3: 
	         w = 0.56;
                 y1 = 0.55;
	         h = 0.36;
                 break;
             case 4: 
	         w = 1.18;
                 break;
             case 5: 
	         x1 = 0.05;
	         h = 0.265;
                 break;
             case 6: 
	         w = 1.18;
		 h = 0.27;
                 break;
             case 7: 
	         x1 = 0.05;
	         w = 0.78;
		 h = 0.27;
                 break;
             case 8: 
	         w = 1.18;
	         h = 0.28;
                 break;
             case 9: 
             case 10: 
	         h = 0.28;
                 break;
             case 11: 
             case 12: 
	         h = 0.38;
		 y1 = 0.51;
                 break;
                 break;
             default: /* 13-16 */
                 break;
	     }
	     /* set up x-axis for amplitude */
	     GracePrintf( "subtitle size %f",0.9 );
	     GracePrintf( "subtitle font 2" );
	     GracePrintf( "subtitle color 1" );
	     GracePrintf( "frame color 1" );
	     
	     GracePrintf( "xaxis tick color 7" );
	     GracePrintf( "xaxis ticklabel prec 0" );
	     GracePrintf( "xaxis ticklabel char size 0.43" );
	     GracePrintf( "xaxis ticklabel format decimal" );
	     if ( winScaler[chSelect] > 0 ) {
	       yminimum = winYMin[chSelect];
	       ymaximum = winYMax[chSelect];
	       GracePrintf( "world ymin %le", yminimum );
	       GracePrintf( "world ymax %le", ymaximum );
	       GracePrintf( "yaxis tick major %le", winScaler[chSelect]/ytick_maj );
	       GracePrintf( "yaxis tick minor %le", winScaler[chSelect]/ytick_min );
	     }
	     GracePrintf( "yaxis tick color 7" );
	     if ( winScaler[chSelect] < 0.01)
	       GracePrintf( "yaxis ticklabel prec 6" );
	     else if ( winScaler[chSelect] < 20.0)
	       GracePrintf( "yaxis ticklabel prec 2" );
	     else
	       GracePrintf( "yaxis ticklabel prec 0" );
	     GracePrintf( "yaxis ticklabel char size 0.43" );
	     GracePrintf( "yaxis ticklabel format decimal" );
	     GracePrintf( "g16 type xy" );
	     GracePrintf( "view %f, %f,%f,%f",x1+XSHIFT,y1,x1+w,y1+h );
	     GracePrintf( "frame color 1" );
	     GracePrintf( "frame background color 1" );
	     GracePrintf( "frame fill on" );
	     if ( winScaler[chSelect] < 0.0001) {
	       GracePrintf( "yaxis ticklabel format exponential" );
	       GracePrintf( "yaxis ticklabel prec 3" );
	     }
	     else if ( winScaler[chSelect] < 0.01)
	       GracePrintf( "yaxis ticklabel prec 6" );
	     else if ( winScaler[chSelect] < 20.0)
	       GracePrintf( "yaxis ticklabel prec 2" );
	     else
	       GracePrintf( "yaxis ticklabel prec 0" );
	     GracePrintf( "view %f, %f,%f,%f",x1+XSHIFT,y1,x1+w,y1+h );
	     GracePrintf( "frame color 1" );
	     GracePrintf( "frame background color 1" );
	     GracePrintf( "frame fill on" );
	     switch ( xyType[chSelect] ) {
	     case 1: /* Log */
	       if ( winYMax[chSelect] <= ZEROLOG )
		 winScaler[chSelect] = -1.0;
	       else if ( winYMin[chSelect] < ZEROLOG ) {
		 GracePrintf( "world ymin %le", ZEROLOG );
	       }
	       if ( rat[chSelect] > 100 ) {
		 logmaj = 1.0;
		 logmin = 2.0;
	       }
	       else {
		 logmaj = 1.0;
		 logmin = 1.0;
	       }
	       GracePrintf( "g16 type logy" );
	       GracePrintf( "yaxis tick major %f", logmaj );
	       GracePrintf( "yaxis tick minor %f", logmin );
	       GracePrintf( "yaxis ticklabel format exponential" );
	       break;
	     case 2: /* Ln */
	       break;
	     case 3: /* exp */
	       GracePrintf( "yaxis ticklabel format exponential" );
	       GracePrintf( "yaxis ticklabel prec 5" );
	       break;
	     default: /* Linear */ 
	       break;
	     }	      
	   }
	}
	for ( i= 0; i<windowNum; i++ ) {
	   GracePrintf( "with g%d", i );
	   GracePrintf( "world xmin %f", (float)from );
	   GracePrintf( "world xmax %f", (float)to );
	   GracePrintf( "xaxis tick major %f", (to-from)/5.0 );
	   GracePrintf( "xaxis tick minor %f", (to-from)/20.0 );
	}
	GracePrintf( "with g0" );
	GracePrintf( "string off" );
	for ( i=0; i<windowNum; i++ ) {
	   GracePrintf( "with g%d", i );
	   GracePrintf( "g%d on", i );
	   GracePrintf( "s0 color %d", gColor[chMarked[i]] );
	   if ( autoon[chMarked[i]] ) GracePrintf( "autoscale yaxes" );
	   GracePrintf( "subtitle \"\\-Ch %d:  %s%s\"", (chMarked[i]+1), chName[chMarked[i]], complexSuffix(chMarked[i]) );
	   /*GracePrintf( "clear stack" );*/
	}
	GracePrintf( "copy g%d.s0 to g16.s0", chSelect_seq );
	if ( sigOn ) {
	GracePrintf( "with g16" );
	GracePrintf( "world xmin %f", (float)from );
	GracePrintf( "world xmax %f", (float)to );
	GracePrintf( "xaxis tick major %f", (to-from)/5.0 );
	GracePrintf( "xaxis tick minor %f", (to-from)/20.0 );
	if ( autoon[chSelect] ) GracePrintf( "autoscale yaxes" );
	GracePrintf( "subtitle \"\\-Ch %d:  %s%s  from %s to %s\"",(chSelect+1),chName[chSelect], complexSuffix(chSelect), trendtimestring, timestring );
	GracePrintf( "g16 on" );
	GracePrintf( "string off" );
	}
	else {
	  GracePrintf( "g16 off" );
	  GracePrintf( "string def \"\\1 Trend Display %d Channels from  %s  to  %s\"",  windowNum, trendtimestring, timestring );
	  GracePrintf( "string loctype view" );
	  GracePrintf( "string 0.27, 0.94" );
	  GracePrintf( "string font 4" );
	  GracePrintf( "string char size 0.9" );
	  GracePrintf( "string color 1" );
	  GracePrintf( "string on" );
	}
	
	GracePrintf( "clear stack" );
	for ( i=0; i<windowNum; i++ ) {
	   GracePrintf( "g%d on", i );
	}
	for ( i=windowNum; i<16; i++ ) { 
	  GracePrintf( "g%d off", i );         
	}
	GracePrintf( "redraw" );
	firsttime = 0;
        if ( warning ) {
	   sprintf ( msgout,"  0 as input of log\n"  );
	   printmessage(-1);
	}
	warning = 0;
	screenChange = 0;
	fflush(stdout);
	GraceFlush();

}


void graphmulti()   
{
double ymaximum,yminimum,xminimum,xmaximum;
int i;

/* use global setting */

        if ( initGraph ) {
	  initGraph = 0;
	  GracePrintf( "focus off" );
	}
	if ( firsttime || screenChange ) {
	   GracePrintf( "background color 7" );
	   GracePrintf( "with g0" ); /* main graph */
	   GracePrintf( "view %f,%f,%f,%f", 0.08,0.10,0.9,0.85 );
	   GracePrintf( "frame color 1" );
	   GracePrintf( "frame background color 1" );
	   GracePrintf( "frame fill on" );
	   GracePrintf( "subtitle size %f", 0.9 );
	   GracePrintf( "subtitle font 2" );
	   GracePrintf( "subtitle color 1" );
	   if ( winXScaler[chSelect] < 1) {
	     xminimum = -1.0 + winDelay[chSelect]/16.0;
	     xmaximum = xminimum + winXScaler[chSelect];
	   }
	   else if ( graphRate[chSelect] < 2048 ) {
	     xminimum = -winXScaler[chSelect];
	     xmaximum = 0.0;
	   }
	   else {
	     xminimum = -1.0;
	     xmaximum = 0.0;
	   }
	   GracePrintf( "world xmin %f", xminimum );
	   GracePrintf( "world xmax %f", xmaximum );
	   GracePrintf( "xaxis tick major 1" );
	   GracePrintf( "xaxis tick minor 0.5" );
	   GracePrintf( "xaxis tick color 7" );
	   GracePrintf( "xaxis ticklabel prec 2" );
	   GracePrintf( "xaxis ticklabel char size 0.6" );
	   GracePrintf( "xaxis ticklabel format decimal" );
	   yminimum = winYMin[chMarked[0]];
	   ymaximum = winYMax[chMarked[0]];
	   for ( i=1; i<windowNum; i++ ) {
              if ( yminimum > winYMin[chMarked[i]] )
		yminimum = winYMin[chMarked[i]];
	      if ( ymaximum < winYMax[chMarked[i]] )
		ymaximum = winYMax[chMarked[i]];
           }
	   GracePrintf( "world ymin %le", yminimum );
	   GracePrintf( "world ymax %le", ymaximum );
	   GracePrintf( "yaxis tick major %le", (ymaximum-yminimum)/(2*ytick_maj) );
	   GracePrintf( "yaxis tick minor %le", (ymaximum-yminimum)/(2*ytick_min) );
	   GracePrintf( "yaxis tick color 7" );
	   GracePrintf( "yaxis ticklabel prec 4" );
	   GracePrintf( "yaxis ticklabel char size 0.6" );
	   GracePrintf( "yaxis ticklabel format decimal" );
#if 0
	   GracePrintf( "string def \"\\1        Channel List    \"" );
	   GracePrintf( "string loctype view" );
	   GracePrintf( "string 0.95, 0.78" );
	   GracePrintf( "string font 4" );
	   GracePrintf( "string char size 0.9" );
	   GracePrintf( "string color 1" );
	   GracePrintf( "string on" );
#endif

	   GracePrintf( "legend loctype view" );
	   GracePrintf( "legend 0.91, 0.72" );
	   GracePrintf( "legend char size 0.7" );
	   GracePrintf( "legend color 1" );
	   GracePrintf( "legend on" );

	}
	GracePrintf( "with g0" ); /* main graph */
	GracePrintf( "s%d linewidth 2", chSelect );
	GracePrintf( "s10 linestyle 3" );
	GracePrintf( "s11 linestyle 3" );
	GracePrintf( "s12 linestyle 3" );
	GracePrintf( "s13 linestyle 3" );
	GracePrintf( "s14 linestyle 3" );
	GracePrintf( "s15 linestyle 3" );
	GracePrintf( "subtitle \"\\1T0=%s\"", timestring );
	for ( i=0; i<windowNum; i++ ) {
	   GracePrintf( "s%d color %d", i, gColor[chMarked[i]] ); 
	   if ( uniton[i] )
	     GracePrintf( "legend string %d \"%d: %s (%s) \"", i,(i+1),chName[chMarked[i]],chUnit[chMarked[i]] );
	   else
	     GracePrintf( "legend string %d \"%d: %s \"", i,(i+1),chName[chMarked[i]] );
	}
    { int scaled = 0;
	for ( i=0; i<windowNum; i++ ) {
	   if ( autoon[chMarked[i]] ) { 
		GracePrintf( "autoscale yaxes" );
		scaled = 1;
	   }
	   //printf("autoscale for %d is %d\n", i, autoon[chMarked[i]]);
	}
	GracePrintf( "clear stack" );
    }

	for ( i=1; i<=16; i++ ) { 
	  GracePrintf( "g%d off", i );         
	}
	GracePrintf( "with g0" );
	GracePrintf( "g0 on" );
	GracePrintf( "redraw" );
	firsttime = 0;
#if 0
        if ( screenChange > 5 ) 
	   screenChange = 0;
#endif
	fflush(stdout);
	GraceFlush();
}


void graphtrmulti(int from, int to)   
{
double ymaximum,yminimum;
int i;

        if ( initGraph ) {
	  initGraph = 0;
	  GracePrintf( "focus off" );
	}
	if ( firsttime || screenChange ) {
	   GracePrintf( "background color 7" );
	   GracePrintf( "with g0" ); /* main graph */
	   GracePrintf( "view %f,%f,%f,%f", 0.08,0.10,0.90,0.85 );
	   GracePrintf( "frame color 1" );
	   GracePrintf( "frame background color 1" );
	   GracePrintf( "frame fill on" );
	   GracePrintf( "subtitle size %f", 0.9 );
	   GracePrintf( "subtitle font 2" );
	   GracePrintf( "subtitle color 1" );
	   GracePrintf( "frame color 1" );
	   GracePrintf( "xaxis tick color 7" );
	   GracePrintf( "xaxis ticklabel prec 2" );
	   GracePrintf( "xaxis ticklabel char size 0.6" );
	   GracePrintf( "xaxis ticklabel format decimal" );
	   yminimum = winYMin[chMarked[0]];
	   ymaximum = winYMax[chMarked[0]];
	   for ( i=1; i<windowNum; i++ ) {
              if ( yminimum > winYMin[chMarked[i]] )
		yminimum = winYMin[chMarked[i]];
	      if ( ymaximum < winYMax[chMarked[i]] )
		ymaximum = winYMax[chMarked[i]];
           }
	   GracePrintf( "world ymin %le", yminimum );
	   GracePrintf( "world ymax %le", ymaximum );
	   GracePrintf( "yaxis tick major %le", (ymaximum-yminimum)/(2*ytick_maj) );
	   GracePrintf( "yaxis tick minor %le", (ymaximum-yminimum)/(2*ytick_min) );
	   GracePrintf( "yaxis tick color 7" );
	   GracePrintf( "yaxis ticklabel prec 4" );
	   GracePrintf( "yaxis ticklabel char size 0.6" );
	   GracePrintf( "yaxis ticklabel format decimal" );
#if 0
	   GracePrintf( "string def \"\\1        Channel List     \"" );
	   GracePrintf( "string loctype view" );
	   GracePrintf( "string 0.95, 0.78" );
	   GracePrintf( "string font 4" );
	   GracePrintf( "string char size 0.9" );
	   GracePrintf( "string color 1" );
	   GracePrintf( "string on" );
#endif

	   GracePrintf( "legend loctype view" );
	   GracePrintf( "legend 0.91, 0.72" );
	   GracePrintf( "legend char size 0.7" );
	   GracePrintf( "legend color 1" );
	   GracePrintf( "legend on" );
	   GracePrintf( "s%d linewidth 2", chSelect );
	   GracePrintf( "s10 linestyle 3" );
	   GracePrintf( "s11 linestyle 3" );
	   GracePrintf( "s12 linestyle 3" );
	   GracePrintf( "s13 linestyle 3" );
	   GracePrintf( "s14 linestyle 3" );
	   GracePrintf( "s15 linestyle 3" );
	}
	GracePrintf( "with g0" ); /* main graph */
	GracePrintf( "world xmin %d", from );
	GracePrintf( "world xmax %d", to );
	GracePrintf( "xaxis tick major %f", (to-from)/5.0 );
	GracePrintf( "xaxis tick minor %f", (to-from)/20.0 );
	GracePrintf( "subtitle \"\\1 Trend from %s to %s\"", trendtimestring, timestring );
	for ( i=0; i<windowNum; i++ ) {
	   GracePrintf( "s%d color %d", i, gColor[i] );      
	   GracePrintf( "legend string %d \"Ch %d:%s%s \"", i,(i+1),chName[chMarked[i]], complexSuffix(chMarked[i]) );
	}
	for ( i=0; i<windowNum; i++ ) {
	   if ( autoon[chMarked[i]] ) GracePrintf( "autoscale yaxes" );
	}
	GracePrintf( "clear stack" );

	for ( i=1; i<=16; i++ ) { 
	  GracePrintf( "g%d off", i );         
	}
	GracePrintf( "redraw" );
	firsttime = 0;
        if ( warning ) {
	   sprintf ( msgout,"  0 as input of log\n"  );
	   printmessage(-1);
	}
	warning = 0;
        if ( screenChange ) 
	   screenChange = 0;
	fflush(stdout);
	GraceFlush();
}





/* switch color: switch/adjust backgroud and frame colors while quitting. */
void  switchcolor()
{
int i, j;

       GracePrintf( "focus on" );
       GracePrintf( "background color 7" );
       switch ( graphMethod ) {
       case GMODESTAND: 
	 for ( i=0; i<=windowNum; i++ ) {
	   if ( i == windowNum )    
	     j = 16;
	   else
	     j = i;
	   GracePrintf( "with g%d", j );
	   /*GracePrintf( "s0 color %d", gqColor[j] );*/
	   GracePrintf( "frame background color 0" );
	   GracePrintf( "xaxis tick color 1" );
	   GracePrintf( "yaxis tick color 1" );
	 }
	 break;
       case GMODEMULTI: 
	 GracePrintf( "with g0" ); 
	 /*for ( i=0; i<windowNum; i++ ) {
	   GracePrintf( "s%d color %d", i, mqColor[i] );
	   }*/
	 GracePrintf( "frame background color 0" );
	 GracePrintf( "xaxis tick color 1" );
	 GracePrintf( "yaxis tick color 1" );
	 GracePrintf( "string color 1" );
	 GracePrintf( "legend color 1" );
	 
	 GracePrintf( "with g1" ); 
	 GracePrintf( "frame background color 0" );
	 GracePrintf( "xaxis tick color 1" );
	 GracePrintf( "yaxis tick color 1" );
	 break;
       }
       
       GracePrintf( "with g16" ); 
       GracePrintf( "with string 0" );
       GracePrintf( "string color 1" );
       GracePrintf( "with g0" );
       return;
}


void  printgraph()
{
int  fproc;
char printfile[24];

       sprintf ( msgout, "Print Hardcopy\n" );
       printmessage(0);
       sprintf ( printfile, "%s.ps", timestring );
       sprintf ( msgout, "printing %s to file\n", printfile );
       printmessage(0);
       GracePrintf( "print to file \"%s\"", printfile );
       GracePrintf( "hardcopy" );
       fflush(stdout);
       printing = -1;
       firsttime = 1;
       return;
}



/* not used. can't switch back to printer device after printing to file.
   don't know why */
/* void  printgraph()
{
int  fproc;
char printfile[24];

       dfprintf ( stderr, "Print Hardcopy\n" );
       switchcolor(printing);
       GracePrintf( "redraw" );
       sprintf ( printfile, "%s.ps", timestring );
       if ( printing == 1 ) {
	  dfprintf ( stderr, "printing %s to printer\n", printfile );
	  GracePrintf( "print hardcopy \"lpr\"" ); this one or next--dont work
	  GracePrintf( "print to hardcopy" );
       }
       else {
	  dfprintf ( stderr, "printing %s to file\n", printfile );
	  GracePrintf( "print to file \"%s\"", printfile );
       }
       GracePrintf( "hardcopy" );
       switchcolor(-1);
       GracePrintf( "redraw" );
       
       fflush(stdout);
       printing = -1;
       firsttime = 1;
       return;
}
*/


/* quit display: how=0 quit xmgrace; how=1 exit */
void  quitdisplay(int how) 
{
       sprintf ( msgout, "Quit data display\n" );
       printmessage(1);
       if ( !stopped ) {
	  DataWriteStop(processID);
	  stopped = 1;
       }
       msgctl(msqid, IPC_RMID, (struct msqid_ds *) 0);
       if ( how == 1 ) {
	  GraceClose();  /* close and exit xmgrace */
	  sprintf ( msgout, "Exit...\n" );
	  printmessage(0);
       }
       else {
#if 0
	  sprintf ( msgout, "Now free the old Grace and open a new one...\n" );
	  printmessage(0);
#endif
	  switchcolor();
	  GracePrintf( "redraw" );
	  GraceFlush ();        
	  GraceClosePipe();  /* close named-pipe - new function in Grace */
       }
       DataQuit();
       fflush(stdout);
       fclose(fwarning);
       fclose(stdout);
       exit(0);
}



void  resumePause()
{
int     i, j, tcount;
double  trms;

       	fppause = fopen( pausefile, "r" );
	if ( fppause == NULL ) {
	   sprintf ( msgout,"Error openning file: resume trend\n" );
	   printmessage(-1);
	   return;
	}
	i = 0;
	sprintf ( msgout,"Resuming paused trend data\n" );
	printmessage(0);
	while ( i < pausefilecn ) {
	   for ( j=0; j<windowNum; j++ ) {
	     fscanf( fppause, "%d%lf", &tcount, &trms );
	     if ( graphMethod == GMODEMULTI )
	       GracePrintf( "g0.s%d point %2f, %le", j, (float)tcount, trms );
	     else
	       GracePrintf( "g%d.s0 point %2f, %le", j, (float)tcount, trms );
	   }
	   i++;
	}
	fclose(fppause);
	sprintf ( msgout,"Resume Done\n" );
	printmessage(1);
	return;
}


/* initial Xmgrace */
void  graphini()
{
int j;
        GracePrintf( "background color 1" );
        GracePrintf( "view ymin 0.001" );
        GracePrintf( "view ymax 0.999" );
        GracePrintf( "view xmin 0.001" );
        GracePrintf( "view xmax 0.999" );
        GracePrintf( "with string 0" );
        GracePrintf( "string loctype view" );
        GracePrintf( "string 0.3, 0.7" );
        GracePrintf( "string font 5" );
        GracePrintf( "string char size 2.5" );
        GracePrintf( "string color 2" );
        GracePrintf( "string def \"DATAVIEWER %s\"", version_n );
        GracePrintf( "string on" );
        GracePrintf( "with string 1" );
        GracePrintf( "string loctype view" );
        GracePrintf( "string 0.35, 0.6" );
        GracePrintf( "string font 5" );
        GracePrintf( "string char size 1.5" );
        GracePrintf( "string color 5" );
        GracePrintf( "string def \"The LIGO Project, %s\"", version_d );
        GracePrintf( "string on" );
	for ( j=0; j<16; j++ ) {
           GracePrintf( "g%d off", j );
        }
        GracePrintf( "redraw" );
	GraceFlush ();        
}

void  printmessage(int stat)
/* stat: 1-don't print (reset) */
{
  if (stat == reset) return;
        if (stat == 0)
	  fprintf ( stderr, "%s", msgout );
        if (stat == 2) {
	  fprintf ( stderr, "%s", msgout );
	  fprintf ( fwarning, "%s", msgout );
	}
	else if ( stat == -1) {
	  fprintf ( stderr, "WARNING: %s", msgout );
	  fprintf ( fwarning, "WARNING: %s", msgout );

	}
	else if ( stat == -2) {
	  fprintf ( stderr, "ERROR: %s", msgout );
	  fprintf ( fwarning, "ERROR: %s", msgout );
	}
	else if ( stat == -3) {
	  char *em = "Error: lost NDS server connection\n";
	  fputs(em, stderr);
	  fputs(em, fwarning);
	  quitdisplay(2);
	}
	fflush(fwarning);
	fflush(stdout);

}


/* moving average filter */
void  movAvgFilter(double *inData, double *filtData, double *hist, int inrate, int step)
{

int ii,jj,kk;
double *filt;
double *dataIn;
double oldhist[FILTER_WIDTH];
double tempTotal; 

  filt = (double *)filtData;
  dataIn = (double *)inData;

  /* load new history */
  for(ii=0; ii<FILTER_WIDTH; ii++) { 
    oldhist[ii] = hist[ii]; 
    hist[ii] = dataIn[inrate - FILTER_WIDTH + ii ];
  }
  /* first data */
  tempTotal =*dataIn;
  for(ii=0; ii<FILTER_WIDTH; ii++) {
    tempTotal += oldhist[ii];
  }
  for(ii=0; ii<FILTER_WIDTH; ii++) {
    dataIn ++;
    tempTotal += *dataIn;
  }
  *filt = tempTotal / FILTER_SIZE; 
  /* second data */
  if (step == FILTER_WIDTH) {
    dataIn += step-FILTER_SIZE+1;
    tempTotal = oldhist[FILTER_WIDTH-1];
  }
  else {
    dataIn += step-FILTER_SIZE;
    tempTotal = *dataIn;
    dataIn ++;
  }
  filt ++;
  for(kk=1; kk<FILTER_SIZE; kk++) {
    tempTotal += *dataIn;
    dataIn ++;
  }
  *filt = tempTotal / FILTER_SIZE; 

  /* others */

  for(ii=0; ii<inrate; ii+=step) {
    dataIn += step-FILTER_SIZE;
    filt ++;
    tempTotal = 0.0;
    for(kk=0; kk<FILTER_SIZE; kk++) {
      tempTotal += *dataIn;
      dataIn ++;
    }
    *filt = tempTotal / FILTER_SIZE; /* average */
  } 
  return;  
}
