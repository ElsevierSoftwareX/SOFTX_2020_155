/*----------------------------------------------------------------------*/
/*                                                                      */
/* Module Name: framer3.c                                               */
/* Compile with                                                         */
/*  gcc -o framer3 framer3.c datasrv.o daqc_access.o                    */
/*     -L/home/hding/Cds/Frame/Lib/Xmgr -lacegr_np_my                   */
/*     -L/home/hding/Cds/Frame/Lib/UTC_GPS -ltai                        */
/*     -lm -lnsl -lsocket -lpthread                                     */
/*                                                                      */
/*----------------------------------------------------------------------*/

#include "framer3.h"


int main(int argc, char *argv[])
{
int   i=0;
char  origDir[1024], logDir[80], displayIP[80];
char  wfile[80];

/* Open pipe file to Dadsp */
	key = atoi(argv[1]);
	if ( (msqid = msgget(key, IPC_CREAT | 0666)) < 0 ) {
	  perror( "framer3 msgget:msqid" );
	  exit(-1);
	}
	sprintf ( msgout, "display: attached message queue %d\n", msqid );
	printmessage(0);
        strcpy(serverIP, argv[2]);
	serverPort = atoi(argv[3]);
        strcpy(origDir, argv[4]);
        strcpy(displayIP, argv[5]);
        strcpy(logDir, argv[6]);
	shellID = atoi(argv[11]);
	sprintf ( wfile, "%slog_warning", logDir );
	sprintf ( msgout, "Starting framer3 with warning file %s, displayIP %s, server %s:%d\n", wfile, displayIP, serverIP, serverPort );

	firsttime = 1;
	initdata = 0;

/* set up warning file  */
	fwarning = fopen(wfile, "a");
	if (fwarning != NULL)
	  printmessage(2);
	else
	  printmessage(0);
	if (fwarning == NULL) {
	  sprintf ( msgout, "Error for opening warning file: %s. Exit.\n", wfile );
	  printmessage(0);
	  return -1;
	}
	sprintf ( msgout, "LOGFILE:\n" );
	fprintf ( fwarning, "%s", msgout );
	/*fprintf ( fwarning, "WARNING: %s", msgout );*/
	/*printmessage(-1);*/

/* Launch xmgr with named-pipe support 
   this function has been rewritten for our own need*/
	/*if (ACEgrOpen(1000000, displayIP, origDir) == -1) {*/
	if (ACEgrOpen(1000000, displayIP, origDir) == -1) {
	  sprintf ( msgout, "Can't run xmgr. \n" );
	  printmessage(-2);
	  exit (EXIT_FAILURE);
	}
	sprintf ( msgout, "Done launching xmgr \n" );
	printmessage(0);

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
	printmessage(0);

/* Paint Xmgr */	
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

        if ( newReadMode != STOPMODE) {
	   if ( chSelect0 >= windowNum ) {
	      if ( !stopped ) {
		 stopped = 1;
		 DataWriteStop(processID);
	      }
	      sprintf ( msgout, "Readmode: selected channel must be <= %d\n", windowNum );
	      printmessage(-1);
	      return;
	   }
	}
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
	      fclose(fppause);
	      paused = 0;
	      sprintf ( msgout, "New display mode: STOPMODE\n" );
	      printmessage(0);
	      break;
	  case REALTIME:
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
	      printmessage(0);
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
	   printmessage(-3);
	}
	return;
}


void  changeConfigure()
{
int j, chNum;

        if ( !stopped ) {
	   DataWriteStop(processID);
	   //sleep(2);
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

	if ( chSelect0 >= windowNum ) {
	   if ( !stopped ) {
	      DataWriteStop(processID);
	      stopped = 1;
	   }
	   sprintf ( msgout, "Conf Error: selected channel must be <= %d\n", windowNum );
	   printmessage(-1);
	   graphOption = graphOption0;
	   graphMethod = graphMethod0;
	   chSelect = chSelect0;
	   chanTrig = chanTrig0;
	   return;
	} 
	else
	   chNum = windowNum;
	
	DataChanDelete("all");
	if ( graphOption0 == GRAPHTREND || filterOn ) {
	   for ( j=0; j<windowNum; j++ ) {
	      DataChanAdd(chName[j], 0);
	   }
	}
	else {
	   if ( trigOn )
	      DataChanAdd(chName[chanTrig0-1], 0);
	   nslow = 0;
	   for ( j=0; j<chNum; j++ ) {
	      if ( chType[j] == 3 ) nslow = 1;
	      if ( !trigOn || j != chanTrig0-1 ) {
		 if ( chType[j] == 1 ) { /* if fast channel */
		   DataChanAdd(chName[j], resolution);
		 }
		 else { /* if slower channel */
		    DataChanAdd(chName[j], 0);
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
	       DataGetChSlope(chName[j], &slope[j], &offset[j], &chstatus[j]);
	       /*fprintf ( stderr,"ch%d status=%d\n",j, chstatus[j] );*/
	       fprintf ( stderr, "%s %f %f\n", chName[j], slope[j], offset[j] );
	     }
	     
	   }
	   else if ( byterecv < 0 ) {
	      sprintf ( msgout, "DataRead = %d\n", byterecv );
	      printmessage(0);
              DataReadStop();
	      /*stopped = 1;*/
	      break;
	   }
	   else if ( byterecv == 0 ) {
	      trailer = 1;
	      stopped = 1;
	      sprintf ( msgout, "trailer received\n" );
	      printmessage(0);
	      break;
	   }
	   else { /* read data */
           DataTimestamp(timestring);
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
		   if ( DataChanType(chName[j]) <= 0 ) { /*if no data */
		     fprintf(fppause, "%d %f\n", trendCount+1,0.0  );
		   }
		   else {
		     DataTrendGetCh(chName[j], trenddata );
                     switch ( xyType[j] ) {
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
	      printmessage(0);
	      /*fprintf ( stderr, "data read: %s  block=%d\n", timestring, block );*/
	      tempcycle++;
	      tempcycle = tempcycle % 16;
	      if ( firsttime ) {
		 ACEgrPrintf( "clear string" );
	      }
	      if ( screenChange ) {
		 ACEgrPrintf( "with g0" );
		 for ( j=0; j<16; j++ ) 
		    ACEgrPrintf( "kill s%d", j );
		 if ( graphOption == GRAPHTIME  ) {
		   for ( j=1; j<=16; j++ ) {
		     ACEgrPrintf( "with g%d", j );
		     ACEgrPrintf( "kill s0" );
		     ACEgrPrintf( "kill s1" );
		     ACEgrPrintf( "kill s2" );
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
		 ACEgrPrintf( "with g0" );
		 for ( j=0; j<16; j++ ) 
		   ACEgrPrintf( "kill s%d", j );
		 ACEgrPrintf( "with g1" );
		 ACEgrPrintf( "kill s0" );
	       }
	       else { /* GMODESTAND */
		 for ( j=0; j<=16; j++ ) {
		   ACEgrPrintf( "with g%d", j );
		   ACEgrPrintf( "kill s0" );
		 }
	       }
	       fflush(stdout);

	       ACEgrPrintf( "with g0" );
	       for ( j=0; j<windowNum; j++ ) {
		 /* Find data in the desired data channels */
		 irate = DataGetCh(chName[j], chData_0, 0, 1);
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
		 if ( chNameChange[j] ) {
		   for ( i=0; i<ADC_BUFF_SIZE; i++ ) 
		     tempData[j][i] =   chData[0]; /* show constant if status changed */
		 }
		 if ( bindex == 0 ) {
		   sprintf(acetempname, "/tmp/%dFRAME/framertemp%d_%d",shellID,j,tempcycle );
		   fptemp = fopen(acetempname, "w");
		 }
		 if ( chType[j] == 1 || chType[j] == 2 ) {  /* if fast I&II channel */
		   /* note: fast I channels display graphRate[j]=min{2*chSize[j],resolution} data 
		      fast II channels have full data always */
		   dataShiftS = graphRate[j]/refreshRate;
		   /* corr of delay and scale is taken cared in dc3
                      delay only applies to X<=1 */
		   if ( graphMethod == GMODEMULTI ) 
                      tempxscaler = winXScaler[chSelect];
		   else 
                      tempxscaler = winXScaler[j];
		   if ( graphRate[j] < 2048 ) { /* graphRate[j] < 2048 */
		      if (tempxscaler >= 1)
			fPoints = ADC_BUFF_SIZE - graphRate[j]*tempxscaler; 
		      else /* take the whole 1 sec */
			fPoints = ADC_BUFF_SIZE - graphRate[j]; 
		      if ( fPoints < 0 ) fPoints = 0;
		      tPoints = ADC_BUFF_SIZE;
		      for ( i=dataShiftS; i<ADC_BUFF_SIZE; i++ ) 
			 tempData[j][i-dataShiftS] = tempData[j][i];
		      acntr = ADC_BUFF_SIZE - dataShiftS;
                   }
                   else { /* show 1 sec only */
		      fPoints = 0;
		      tPoints = graphRate[j];
		      for ( i=dataShiftS; i<graphRate[j]; i++ ) 
			 tempData[j][i-dataShiftS] = tempData[j][i];
		      acntr = graphRate[j] - dataShiftS;
                   }
		   if ( trigOn && j == chanTrig-1 && chType[j] == 1) {
		     l = 0;
		     hskip = (2*chSize[j])/graphRate[j];
		     for ( i=0; i<2*chSize[j]/refreshRate; i+=hskip )  {
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
		       if ( maxSec <= 1 || winXScaler[j] < 1 ) {
			 c = -1.0;
		       }
		       else { /* c = -min{maxSec, winXScaler[j]} */
			 if (maxSec < winXScaler[j])
			   c = -maxSec;
			 else
			   c = -winXScaler[j];
		       }
		     }
		     if ( graphMethod == GMODEMULTI ) { /* show no xyType */
		       if ( maxSec <= 1 || winXScaler[chSelect] < 1 ) 
			 c = -1.0;
		       else 
			 c = -winXScaler[chSelect];
		       if ( uniton[j] ) {
			 for (i=fPoints; i<tPoints; i++ ) {
			   fprintf(fptemp, "%2f\t%le\n", c, offset[j]+slope[j]*tempData[j][i] );
			   c = c + 1.0/graphRate[j];
			 }
		       }
		       else {
			 for (i=fPoints; i<tPoints; i++ ) {
			   fprintf(fptemp, "%2f\t%le\n", c, tempData[j][i] );
			   c = c + 1.0/graphRate[j];
			 }
		       }
		     }
		     else if ( uniton[j] ) {
		       switch ( xyType[j] ) {
		       case 1: 
			 for (i=fPoints; i<tPoints; i++ ) {
			   result = tempData[j][i];
			   if ( result == 0 ) {
			     result = ZEROLOG;
			     warning = 1;
			   }
			   else
			     result = fabs(result);
			   fprintf(fptemp, "%2f\t%le\n", c, offset[j]+slope[j]*result );
			   c = c + 1.0/graphRate[j];
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
			   fprintf(fptemp, "%2f\t%le\n", c, offset[j]+slope[j]*result );
			   c = c + 1.0/graphRate[j];
			 }
			 break;
		       default:
			 for (i=fPoints; i<tPoints; i++ ) {
			   fprintf(fptemp, "%2f\t%le\n", c, offset[j]+slope[j]*tempData[j][i] );
			   c = c + 1.0/graphRate[j];
			 }
			 break;
		       }
		     }
		     else { /* unit off */
		       switch ( xyType[j] ) {
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
			   c = c + 1.0/graphRate[j];
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
			   c = c + 1.0/graphRate[j];
			 }
			 break;
		       default:
			 for (i=fPoints; i<tPoints; i++ ) {
			   fprintf(fptemp, "%2f\t%le\n", c, tempData[j][i] );
			   c = c + 1.0/graphRate[j];
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
		     if ( uniton[j] ) {
		       for (i=0; i<16; i++ ) {
			 fprintf(fptemp, "%2f\t%le\n", i-15.5, offset[j]+slope[j]*tempData[j][i] );
		       }
		     }
		     else {
		       for (i=0; i<16; i++ ) {
			 fprintf(fptemp, "%2f\t%le\n", i-15.5, tempData[j][i] );
		       }
		     }
		   }
		   else if ( uniton[j] ) {
		     switch ( xyType[j] ) {
		     case 1: 
		       for (i=0; i<16; i++ ) {
			 result = tempData[j][i];
			 if ( result == 0 ) {
			   result = ZEROLOG;
			   warning = 1;
			 }
			 else
			   result = fabs(result);
			 fprintf(fptemp, "%2f\t%le\n", i-15.5, offset[j]+slope[j]*result );
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
			 fprintf(fptemp, "%2f\t%le\n", i-15.5, offset[j]+slope[j]*result );
		       }
		       break;
		     default: 
		       for (i=0; i<16; i++ ) 
			 fprintf(fptemp, "%2f\t%le\n", i-15.5, offset[j]+slope[j]*tempData[j][i] );
		       break;
		     }
		   }
		   else { /* unit off */
		     switch ( xyType[j] ) {
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
		 chNameChange[j] = 0;
		 if ( bindex == 0 ) {
		   fclose(fptemp);
		   ACEgrPrintf( "read xy \"%s\"", acetempname ); 
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
		   ACEgrPrintf( "with g0" );
		   for ( j=0; j<16; j++ ) 
		     ACEgrPrintf( "kill s%d", j );
		   ACEgrPrintf( "with g1" );
		   ACEgrPrintf( "kill s0" );
		 }
		 else { /* GMODESTAND */
		   for ( j=0; j<=16; j++ ) {
		     ACEgrPrintf( "with g%d", j );
		     ACEgrPrintf( "kill s0" );
		     ACEgrPrintf( "kill s1" );
		     ACEgrPrintf( "kill s2" );
		   }
		 }
	       }
	       fflush(stdout);
	       if ( trendCount >=  43200 - 1 ) { /* 12 hours */
		 sprintf ( msgout,"Trend has been played more than 24 hours. Reset at %s.\n", timestring );
		 printmessage(0);
		 ACEgrPrintf( "saveall \"%s.save\"", timestring );
		 trendCount = 0;
		 firsttime = 1;
		 break;
	       }
	       for ( j=0; j<windowNum; j++ ) {
		 if ( graphMethod == GMODESTAND ) {
		   ACEgrPrintf( "with g%d", j );
		   if ( DataChanType(chName[j]) <= 0 ) { /*if no data */
		     ACEgrPrintf( "g%d.s0 point %2f, %2f", j, (float)(trendCount+1), 0.0 );
		   }
		   else {
		     irate = DataTrendGetCh(chName[j], trenddata );
		     switch ( graphOption ) {
		     case GRAPHTREND: 
		       switch ( xyType[j] ) {
		       case 1: 
			 for ( i=0; i<irate; i++ ) {
			   result = trenddata[i].mean;
			   if ( result == 0 ) {
			     result = ZEROLOG;
			     warning = 1;
			   }
			   else
			     result = fabs(result);
			   ACEgrPrintf( "g%d.s0 point %2f, %le", j, (float)(trendCount+i), result );
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
			   ACEgrPrintf( "g%d.s0 point %2f, %le", j, (float)(trendCount+i), result );
			 }
			 break;
		       default: 
			 for ( i=0; i<irate; i++ ) {
			   ACEgrPrintf( "g%d.s0 point %2f, %le", j, (float)(trendCount+i), trenddata[i].mean );
			 }
			 break;
		       }
		       break;
		     }
		   }
		 }
		 else { /* GMODEMULTI*/
		   ACEgrPrintf( "with g0" );
		   if ( DataChanType(chName[j]) <= 0 ) { /*if no data */
		     ACEgrPrintf( "g0.s%d point %2f, %2f", j, (float)(trendCount+1), 0.0 );
		   }
		   else {
		     irate = DataTrendGetCh(chName[j], trenddata );
		     for ( i=0; i<irate; i++ ) { 
		       result = trenddata[i].mean;
		       ACEgrPrintf( "g0.s%d point %2f, %le", j, (float)(trendCount+i), result );
		     }
		   }
		 }
	       }
	       for ( i=0; i<16; i++ ) chNameChange[i] = 0;	
	       trendCount += (irate + drop);
	       ACEgrFlush();
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
	   

	   /* check trigger when not in trend... option */
	   trigApply = 0;
	   if ( trigOn && graphOption == GRAPHTIME ) {
	     irate = DataGetCh(chName[chanTrig-1], chData, 0, 1);
	     if ( irate < 0 ) {
	       sprintf ( msgout, "no data output for trigger channel\n" );
	       printmessage(-1);
	       fflush (stdout);
	       return NULL;
	     }
	     realtrigger = 0.0;
	     hsize = chSize[chanTrig-1];
	     if ( chType[chanTrig-1] == 1 )
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
int i, j;
int iyr, imo, ida, ihr, imn, isc;
static short resetReadMode;

	/* Check all the operator display options */
	sprintf ( mb.mtext, "\0" );
	while ( (i = msgrcv(msqid, &mb, MSQSIZE, 0, 0/*IPC_NOWAIT*/)) > 0 ) {
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
		 j = DataChanRate(mb.mtext);
		 if ( j < 0 ) {
		   sprintf ( msgout, "signal %s for Ch.%d doesn't not exist. Unchanged.\n", mb.mtext, chChange );
		   printmessage(-1);
		   break;
                 }
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
	     case 9: /* channel select */
	         j = atoi(mb.mtext);
		 chSelect0 = j;
		 if (reset) {
		    chNameChange[chSelect0] = 1;
		    sprintf ( msgout, "Resetting for CH.%d ...\n", chSelect0+1 );
		    printmessage(reset);
		 }
		 else if ( graphOption0 == GRAPHTREND ) {
		    firsttime = 1;
		    trailer = 0;
		    if ( chSelect0 >= windowNum ) {
		       if ( !stopped ) {
			  DataWriteStop(processID);
			  stopped = 1;
		       }
		       sprintf ( msgout, "Conf Error: selected channel must be <= %d\n", windowNum );
		       printmessage(reset);
		    }
		    else {
		       sprintf ( msgout, "New channel select: %d\n", chSelect0+1 );
		       printmessage(reset);
		    }
		    chSelect = chSelect0;
		 }
		 else if ( j < windowNum ) {
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
		    resetReadMode = newReadMode;
		    sprintf ( msgout, "New: restoring\n" );
		    printmessage(0);
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
	     case 22: /* reset Xmgr */
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
		 printmessage(0);
	         graphini();
		 sprintf ( msgout, "Reset Xmgr\n" );
		 printmessage(0);
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
	         windowNum = atoi(mb.mtext);
		 sprintf ( msgout, "New Total Channel Number: %d\n", windowNum );
		 printmessage(reset);
		 chSelect0 = 0;
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
	  ACEgrPrintf( "doublebuffer true" );
	  ACEgrPrintf( "focus off" );
	}
	for ( i=0; i<16; i++ ) {
	  x0[i] = 0.0;
	  y0[i] = 0.0;
	}
	if ( firsttime || screenChange ) {
	   ACEgrPrintf( "background color 7" );
	   if ( sigOn || graphOption != GRAPHTIME ) {
	     switch ( windowNum ) {
	     case 1: 
	         x0[0] = 0.06; 
		 y0[0] = 0.026;
		 w = 0.90;
		 h = 0.44;
		 break;
	     case 2: 
	     case 3: 
	         x0[0] = 0.04; y0[0] = 0.10; 
		 x0[1] = 0.54; y0[1] = 0.10; 
		 x0[2] = 0.54; y0[2] = 0.57;
		 w = 0.45;
		 h = 0.35;
		 break;
	     case 4: 
	     case 5: 
	         x0[0] = 0.10; y0[0] = 0.02; 
		 x0[1] = 0.10; y0[1] = 0.32; 
		 x0[2] = 0.56; y0[2] = 0.02;
		 x0[3] = 0.56; y0[3] = 0.32; 
		 x0[4] = 0.56; y0[4] = 0.62; 
		 w = 0.37;
		 h = 0.26;
		 break;
	     case 6: 
	     case 7: 
	         x0[0] = 0.04; y0[0] = 0.02; 
		 x0[1] = 0.04; y0[1] = 0.32; 
		 x0[2] = 0.3666; y0[2] = 0.02;
		 x0[3] = 0.3666; y0[3] = 0.32; 
		 x0[4] = 0.6933; y0[4] = 0.02; 
		 x0[5] = 0.6933; y0[5] = 0.32;
		 x0[6] = 0.6933; y0[6] = 0.62; 
		 w = 0.295;
		 h = 0.26;
		 break;
	     case 8: 
	         x0[0] = 0.04; y0[0] = 0.02; 
		 x0[1] = 0.04; y0[1] = 0.295; 
		 x0[2] = 0.2875; y0[2] = 0.02;
		 x0[3] = 0.2875; y0[3] = 0.295; 
		 x0[4] = 0.535; y0[4] = 0.02; 
		 x0[5] = 0.535; y0[5] = 0.295;
		 x0[6] = 0.7825; y0[6] = 0.02; 
		 x0[7] = 0.7825; y0[7] = 0.295;
		 w = 0.2075;
		 h = 0.23;
		 break;
	     case 9: 
	     case 10: 
	         x0[0] = 0.04; y0[0] = 0.02; 
		 x0[1] = 0.04; y0[1] = 0.295; 
		 x0[2] = 0.2875; y0[2] = 0.02;
		 x0[3] = 0.2875; y0[3] = 0.295; 
		 x0[4] = 0.535; y0[4] = 0.02; 
		 x0[5] = 0.535; y0[5] = 0.295;
		 x0[6] = 0.535; y0[6] = 0.57; 
		 x0[7] = 0.7825; y0[7] = 0.02; 
		 x0[8] = 0.7825; y0[8] = 0.295;
		 x0[9] = 0.7825; y0[9] = 0.57; 
		 w = 0.2075;
		 h = 0.23;
		 break;
	     case 11: 
	     case 12: 
	         x0[0] = 0.04; y0[0] = 0.019; 
		 x0[1] = 0.04; y0[1] = 0.259; 
		 x0[2] = 0.2875; y0[2] = 0.019; 
		 x0[3] = 0.2875; y0[3] = 0.259; 
		 x0[4] = 0.535; y0[4] = 0.019; 
		 x0[5] = 0.535; y0[5] = 0.259; 
		 x0[6] = 0.535; y0[6] = 0.499;
		 x0[7] = 0.535; y0[7] = 0.739; 
		 x0[8] = 0.7825; y0[8] = 0.019;
		 x0[9] = 0.7825; y0[9] = 0.259; 
		 x0[10] = 0.7825; y0[10] = 0.499; 
		 x0[11] = 0.7825; y0[11] = 0.739;
		 w = 0.2075;
		 h = 0.20;
		 break;
	     default: /* case 16  */
	         x0[0] = 0.04; y0[0] = 0.019; 
		 x0[1] = 0.04; y0[1] = 0.218; 
		 x0[2] = 0.04; y0[2] = 0.417;
		 x0[3] = 0.2875; y0[3] = 0.019; 
		 x0[4] = 0.2875; y0[4] = 0.218; 
		 x0[5] = 0.2875; y0[5] = 0.417;
		 x0[6] = 0.535; y0[6] = 0.019; 
		 x0[7] = 0.535; y0[7] = 0.218; 
		 x0[8] = 0.535; y0[8] = 0.417;
		 x0[9] = 0.535; y0[9] = 0.615; 
		 x0[10] = 0.535; y0[10] = 0.813; 
		 x0[11] = 0.7825; y0[11] = 0.019;
		 x0[12] = 0.7825; y0[12] = 0.218; 
		 x0[13] = 0.7825; y0[13] = 0.417; 
		 x0[14] = 0.7825; y0[14] = 0.615;
		 x0[15] = 0.7825; y0[15] = 0.813;
		 w = 0.2075;
		 h = 0.16;
		 break;
	     }
	   }
	   else { /* not to show large window */
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
	   }
	   for ( i= 0; i<windowNum; i++ ) {
	      ACEgrPrintf( "with g%d", i );
	      ACEgrPrintf( "subtitle size %f",0.9 );
	      ACEgrPrintf( "subtitle font 1" );
	      if ( chstatus[i] == 0 )
		ACEgrPrintf( "subtitle color 1" );
	      else
		ACEgrPrintf( "subtitle color 2" );
	      if ( winXScaler[i] < 1) {
		xminimum = -1.0 + winDelay[i]/16.0;
		xmaximum = xminimum + winXScaler[i];
	      }
	      else if ( graphRate[i] < 2048 ) {
		if (maxSec < winXScaler[i])
		  xminimum = -maxSec;
		else
		  xminimum = -winXScaler[i];
		xmaximum = 0.0;
	      }
	      else {
		xminimum = -1.0;
		xmaximum = 0.0;
	      }
	      ACEgrPrintf( "world xmin %f", xminimum );
	      ACEgrPrintf( "world xmax %f", xmaximum );
	      ACEgrPrintf( "xaxis tick color 7" );
	      if (windowNum <= 2)
		ACEgrPrintf( "xaxis ticklabel char size 0.6" );
	      else if (windowNum <= 6)
		ACEgrPrintf( "xaxis ticklabel char size 0.5" );
	      else
		ACEgrPrintf( "xaxis ticklabel char size 0.43" );
	      fj = winXScaler[i];
	      if ( fj >= 2 ) {
		ACEgrPrintf( "xaxis tick major 1" );
		ACEgrPrintf( "xaxis tick minor 1" );
		ACEgrPrintf( "xaxis ticklabel prec 1" );
		ACEgrPrintf( "xaxis ticklabel format decimal" );
	      }
	      else if (fj == 1 ) {
		ACEgrPrintf( "xaxis tick major 0.5" );
		ACEgrPrintf( "xaxis tick minor 0.25" );
		ACEgrPrintf( "xaxis ticklabel prec 3" );
		ACEgrPrintf( "xaxis ticklabel format decimal" );
	      }
	      else { /* fj < 1 */
		ACEgrPrintf( "xaxis tick major %le", (fj*0.5) );
		ACEgrPrintf( "xaxis tick minor %le", (fj*0.25) );
		ACEgrPrintf( "xaxis ticklabel prec 4" );
		ACEgrPrintf( "xaxis ticklabel format decimal" );
	      }
	      if ( winScaler[i] > 0 ) {
		yminimum = winYMin[i];
		ymaximum = winYMax[i];
		ACEgrPrintf( "world ymin %le", yminimum );
		ACEgrPrintf( "world ymax %le", ymaximum );
		ACEgrPrintf( "yaxis tick major %le", (winScaler[i]/ytick_maj) );
		ACEgrPrintf( "yaxis tick minor %le", (winScaler[i]/ytick_min) );
	      }
	      ACEgrPrintf( "yaxis tick color 7" );
	      if (windowNum <= 6)
		ACEgrPrintf( "yaxis ticklabel char size 0.6" );
	      else
		ACEgrPrintf( "yaxis ticklabel char size 0.43" );
	      ACEgrPrintf( "yaxis ticklabel format decimal" );
	      ACEgrPrintf( "g%d type xy", i );
	      if ( winScaler[i] < 0.0001) {
		 ACEgrPrintf( "yaxis ticklabel format exponential" );
		 ACEgrPrintf( "yaxis ticklabel prec 3" );
	      }
	      else if ( winScaler[i] < 0.01)
		 ACEgrPrintf( "yaxis ticklabel prec 6" );
	      else if ( winScaler[i] < 20.0)
		 ACEgrPrintf( "yaxis ticklabel prec 2" );
	      else
		 ACEgrPrintf( "yaxis ticklabel prec 0" );
	      if ( uniton[i] )
		ACEgrPrintf( "yaxis label \"%s\" ", chUnit[i] );
	      else
		ACEgrPrintf( "yaxis label \"no conv.\" " );
	      ACEgrPrintf( "yaxis label layout para" );
	      ACEgrPrintf( "yaxis label place auto" );
	      ACEgrPrintf( "yaxis label font 4" );
	      ACEgrPrintf( "yaxis label char size 0.9" );
	      ACEgrPrintf( "view %f, %f,%f,%f", x0[i]+XSHIFT,y0[i],x0[i]+w,y0[i]+h );
	      ACEgrPrintf( "frame color 1" );
	      ACEgrPrintf( "frame background color 1" );
	      ACEgrPrintf( "frame fill on" );
              switch ( xyType[i] ) {
                case 1: /* Log */
		    if ( winYMax[i] <= ZEROLOG )
		       winScaler[i] = -1.0;
		    else if ( winYMin[i] < ZEROLOG ) {
		       ACEgrPrintf( "world ymin %le", ZEROLOG );
		       rat[i] = winYMax[i]/ZEROLOG;
		    }
		    else {
		       rat[i] = winYMax[i]/winYMin[i];
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
		    ACEgrPrintf( "g%d type logy", i );
		    ACEgrPrintf( "yaxis tick major %f", logmaj );
		    ACEgrPrintf( "yaxis tick minor %f", logmin );
		    ACEgrPrintf( "yaxis ticklabel format exponential" );
		    /*ACEgrPrintf( "yaxis ticklabel prepend \"\\-1.0E\"" );*/
                    break;
                case 2: /* Ln */
                    break;
                case 3: /* exp */
		    ACEgrPrintf( "yaxis ticklabel format exponential" );
		    ACEgrPrintf( "yaxis ticklabel prec 5" );
                    break;
                default: /* Linear */ 
		    if ( winScaler[i] <0.001 || winScaler[i] > 10000000.0 )
		       ACEgrPrintf( "yaxis ticklabel format exponential" );
                    break;
              }	      
	   }
	   if (sigOn || graphOption != GRAPHTIME ) {
	     ACEgrPrintf( "with g16" ); 
	     w = 0.45;
	     h = 0.35;
	     x1 = 0.04;
	     y1 = 0.62;
	     switch ( windowNum ) {
             case 1: 
             case 2: 
	         w = 0.90;
	         h = 0.44;
		 x1 = 0.06;
		 y1 = 0.51;
                 break;
             case 3: 
                 y1 = 0.57;
                 break;
             case 4: 
             case 6: 
	         w = 0.95;
                 break;
             case 7: 
	         w = 0.60;
                 break;
             case 8: 
	         w = 0.95;
	         h = h+0.03;
		 y1 = 0.58;
                 break;
             case 9: 
             case 10: 
	         h = h+0.03;
		 y1 = 0.58;
                 break;
             case 11: 
             case 12: 
	         h = h+0.087;
		 y1 = 0.50;
                 break;
             case 16: 
                 y1 = 0.619;
                 break;
             default: 
                 break;
	     }
	     ACEgrPrintf( "legend off" );
	     ACEgrPrintf( "with string 0" );
	     ACEgrPrintf( "string off" );
	     ACEgrPrintf( "string loctype view" );
	     ACEgrPrintf( "string 0.03, 0.98" );
	     ACEgrPrintf( "string font 4" );
	     ACEgrPrintf( "string char size 0.9" );
	     ACEgrPrintf( "string color 1" );
	     
	     /* set up x-axis for amplitude */
	     ACEgrPrintf( "subtitle size %f",0.9 );
	     ACEgrPrintf( "subtitle font 1" );
	     if ( chstatus[chSelect] == 0 )
	       ACEgrPrintf( "subtitle color 1" );
	     else
	       ACEgrPrintf( "subtitle color 2" );
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
	   ACEgrPrintf( "world xmin %f", xminimum );
	   ACEgrPrintf( "world xmax %f", xmaximum );
	   ACEgrPrintf( "xaxis tick major %f", xtickmajor );
	   ACEgrPrintf( "xaxis tick minor %f", xtickminor );
	   ACEgrPrintf( "xaxis tick color 7" );
	   ACEgrPrintf( "xaxis ticklabel char size 0.6" );
	   ACEgrPrintf( "xaxis ticklabel format decimal" );
	   ACEgrPrintf( "xaxis ticklabel prec 3" );
	   if ( winScaler[chSelect] > 0 ) {
	     ACEgrPrintf( "world ymin %le", yminimum );
	     ACEgrPrintf( "world ymax %le", ymaximum );
	     ACEgrPrintf( "yaxis tick major %le", ytickmajor );
	     ACEgrPrintf( "yaxis tick minor %le", ytickminor );	    
	   }
	   ACEgrPrintf( "yaxis tick color 7" );
	   ACEgrPrintf( "yaxis ticklabel char size 0.6" );
	   ACEgrPrintf( "yaxis ticklabel format decimal" );
	   ACEgrPrintf( "g16 type xy" );
	   if ( winScaler[chSelect] < 0.0001) {
	     ACEgrPrintf( "yaxis ticklabel format exponential" );
	     ACEgrPrintf( "yaxis ticklabel prec 3" );
	   }
	   else if ( winScaler[chSelect] < 0.01)
	      ACEgrPrintf( "yaxis ticklabel prec 6" );
	   else if ( winScaler[chSelect] < 20.0)
	      ACEgrPrintf( "yaxis ticklabel prec 2" );
	   else
	      ACEgrPrintf( "yaxis ticklabel prec 0" );
	   if ( uniton[chSelect] )
	     ACEgrPrintf( "yaxis label \"%s\" ", chUnit[chSelect] );
	   else
	     ACEgrPrintf( "yaxis label \"no conv.\" " );
	   ACEgrPrintf( "yaxis label layout para" );
	   ACEgrPrintf( "yaxis label place auto" );
	   ACEgrPrintf( "yaxis label font 4" );
	   ACEgrPrintf( "yaxis label char size 0.9" );
	   ACEgrPrintf( "view %f, %f,%f,%f",x1+XSHIFT,y1,x1+w,y1+h );
	   ACEgrPrintf( "frame color 1" );
	   ACEgrPrintf( "frame background color 1" );
	   ACEgrPrintf( "frame fill on" );
	   switch ( xyType[chSelect] ) {
	   case 1: /* Log */
	     if ( winYMax[chSelect] <= ZEROLOG )
	       winScaler[chSelect] = -1.0;
	     else if ( winYMin[chSelect] < ZEROLOG ) {
	       ACEgrPrintf( "world ymin %le", ZEROLOG );
	     }
	     if ( rat[chSelect] > 100 ) {
	        logmaj = 1.0;
		logmin = 2.0;
	     }
	     else {
	        logmaj = 1.0;
		logmin = 1.0;
	     }
	     ACEgrPrintf( "g16 type logy" );
	     ACEgrPrintf( "yaxis tick major %f", logmaj );
	     ACEgrPrintf( "yaxis tick minor %f", logmin );
	     ACEgrPrintf( "yaxis ticklabel format exponential" );
	     break;
	   case 2: /* Ln */
	     break;
	   case 3: /* exp */
	     ACEgrPrintf( "yaxis ticklabel format exponential" );
	     ACEgrPrintf( "yaxis ticklabel prec 5" );
	     break;
	   default: /* Linear */ 
	     if ( winScaler[chSelect] < 0.001 || winScaler[chSelect] > 10000000.0 )
	       ACEgrPrintf( "yaxis ticklabel format exponential" );
	     break;
	   }	      
	   }
	}
	for ( i=0; i<windowNum; i++ ) {
	   ACEgrPrintf( "g%d on", i );
	}

	/** g0 **/
	ACEgrPrintf( "with g0" );
	for ( i=0; i<windowNum; i++ ) {
	   ACEgrPrintf( "s%d color %d", i, gColor[i] );      
	}
	for ( i=1; i<windowNum; i++ ) {
	   ACEgrPrintf( "move g0.s%d to g%d.s0", i, i );         
	}
	if ( autoon[0] ) 
	   ACEgrPrintf( "autoscale" );
	ACEgrPrintf( "subtitle \"\\-Ch 1:  %s\"", chName[0] );
	ACEgrPrintf( "string off" );
	ACEgrPrintf( "clear stack" );
	/** g1--g15 **/
	for ( i=1; i<windowNum; i++ ) {
	   ACEgrPrintf( "with g%d", i );
	   if ( autoon[i] ) ACEgrPrintf( "autoscale" );
	   ACEgrPrintf( "subtitle \"\\-Ch %d:  %s\"", (i+1), chName[i] );
	   ACEgrPrintf( "clear stack" );
	}
	/** g16 **/
	if ( sigOn || graphOption != GRAPHTIME ) {
	  ACEgrPrintf( "with g16" );
	  ACEgrPrintf( "copy g%d.s0 to g16.s0", chSelect );
	  /*if ( chType[chSelect] == 1 ) 
	    ACEgrPrintf( "s0 color %d", gColor[chSelect] );*/      
	  if ( autoon[chSelect] ) ACEgrPrintf( "autoscale" );
	  ACEgrPrintf( "subtitle \"\\1 Ch %d:  %s       %s\"",(chSelect+1),chName[chSelect], timestring );
	  ACEgrPrintf( "g16 on" );
	  ACEgrPrintf( "string off" );
	}
	else {
	   ACEgrPrintf( "g16 off" );
	   if ( windowNum == 1 )
	     ACEgrPrintf( "string def \"\\1 DAQS Data Display %d Channel at %s\"", windowNum, timestring );
	   else
	     ACEgrPrintf( "string def \"\\1 DAQS Data Display %d Channels at %s\"", windowNum, timestring );
	   ACEgrPrintf( "string loctype view" );
	   ACEgrPrintf( "string 0.32, 0.98" );
	   ACEgrPrintf( "string font 4" );
	   ACEgrPrintf( "string char size 0.9" );
	   ACEgrPrintf( "string color 1" );
	   ACEgrPrintf( "string on" );
	}
	ACEgrPrintf( "clear stack" );

	for ( i=windowNum; i<16; i++ ) { 
	  ACEgrPrintf( "kill g%d", i );         
	}

	ACEgrPrintf( "with g0" );
	ACEgrPrintf( "redraw" );
	firsttime = 0;
        if ( warning ) {
	   sprintf ( msgout,"  0 as input of log\n"  );
	   printmessage(-1);
	}
	warning = 0;
        if ( screenChange > 5 ) 
	   screenChange = 0;
	fflush(stdout);
	ACEgrFlush();
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
	  ACEgrPrintf( "doublebuffer true" );
	  ACEgrPrintf( "focus off" );
	}
	for ( i=0; i<16; i++ ) {
	  x0[i] = 0.0;
	  y0[i] = 0.0;
	}
	if ( firsttime || screenChange ) {
	   ACEgrPrintf( "background color 7" );
	   if ( sigOn ) {
	   switch ( windowNum ) {
	     case 1: 
	         x0[0] = 0.06; 
		 y0[0] = 0.026;
		 w = 0.90;
		 h = 0.44;
		 break;
	     case 2: 
	     case 3: 
	         x0[0] = 0.04; y0[0] = 0.10; 
		 x0[1] = 0.54; y0[1] = 0.10; 
		 x0[2] = 0.54; y0[2] = 0.57;
		 w = 0.45;
		 h = 0.35;
		 break;
	     case 4: 
	     case 5: 
	         x0[0] = 0.10; y0[0] = 0.02; 
		 x0[1] = 0.10; y0[1] = 0.32; 
		 x0[2] = 0.56; y0[2] = 0.02;
		 x0[3] = 0.56; y0[3] = 0.32; 
		 x0[4] = 0.56; y0[4] = 0.62; 
		 w = 0.37;
		 h = 0.26;
		 break;
	     case 6: 
	     case 7: 
	         x0[0] = 0.04; y0[0] = 0.02; 
		 x0[1] = 0.04; y0[1] = 0.32; 
		 x0[2] = 0.3666; y0[2] = 0.02;
		 x0[3] = 0.3666; y0[3] = 0.32; 
		 x0[4] = 0.6933; y0[4] = 0.02; 
		 x0[5] = 0.6933; y0[5] = 0.32;
		 x0[6] = 0.6933; y0[6] = 0.62; 
		 w = 0.295;
		 h = 0.26;
		 break;
	     case 8: 
	         x0[0] = 0.04; y0[0] = 0.02; 
		 x0[1] = 0.04; y0[1] = 0.295; 
		 x0[2] = 0.2875; y0[2] = 0.02;
		 x0[3] = 0.2875; y0[3] = 0.295; 
		 x0[4] = 0.535; y0[4] = 0.02; 
		 x0[5] = 0.535; y0[5] = 0.295;
		 x0[6] = 0.7825; y0[6] = 0.02; 
		 x0[7] = 0.7825; y0[7] = 0.295;
		 w = 0.2075;
		 h = 0.23;
		 break;
	     case 9: 
	     case 10: 
	         x0[0] = 0.04; y0[0] = 0.02; 
		 x0[1] = 0.04; y0[1] = 0.295; 
		 x0[2] = 0.2875; y0[2] = 0.02;
		 x0[3] = 0.2875; y0[3] = 0.295; 
		 x0[4] = 0.535; y0[4] = 0.02; 
		 x0[5] = 0.535; y0[5] = 0.295;
		 x0[6] = 0.535; y0[6] = 0.57; 
		 x0[7] = 0.7825; y0[7] = 0.02; 
		 x0[8] = 0.7825; y0[8] = 0.295;
		 x0[9] = 0.7825; y0[9] = 0.57; 
		 w = 0.2075;
		 h = 0.23;
		 break;
	     case 11: 
	     case 12: 
	         x0[0] = 0.04; y0[0] = 0.019; 
		 x0[1] = 0.04; y0[1] = 0.259; 
		 x0[2] = 0.2875; y0[2] = 0.019; 
		 x0[3] = 0.2875; y0[3] = 0.259; 
		 x0[4] = 0.535; y0[4] = 0.019; 
		 x0[5] = 0.535; y0[5] = 0.259; 
		 x0[6] = 0.535; y0[6] = 0.499;
		 x0[7] = 0.535; y0[7] = 0.739; 
		 x0[8] = 0.7825; y0[8] = 0.019;
		 x0[9] = 0.7825; y0[9] = 0.259; 
		 x0[10] = 0.7825; y0[10] = 0.499; 
		 x0[11] = 0.7825; y0[11] = 0.739;
		 w = 0.2075;
		 h = 0.20;
		 break;
	     default: /* case 16  */
	         x0[0] = 0.04; y0[0] = 0.019; 
		 x0[1] = 0.04; y0[1] = 0.218; 
		 x0[2] = 0.04; y0[2] = 0.417;
		 x0[3] = 0.2875; y0[3] = 0.019; 
		 x0[4] = 0.2875; y0[4] = 0.218; 
		 x0[5] = 0.2875; y0[5] = 0.417;
		 x0[6] = 0.535; y0[6] = 0.019; 
		 x0[7] = 0.535; y0[7] = 0.218; 
		 x0[8] = 0.535; y0[8] = 0.417;
		 x0[9] = 0.535; y0[9] = 0.615; 
		 x0[10] = 0.535; y0[10] = 0.813; 
		 x0[11] = 0.7825; y0[11] = 0.019;
		 x0[12] = 0.7825; y0[12] = 0.218; 
		 x0[13] = 0.7825; y0[13] = 0.417; 
		 x0[14] = 0.7825; y0[14] = 0.615;
		 x0[15] = 0.7825; y0[15] = 0.813;
		 w = 0.2075;
		 h = 0.16;
		 break;
	   }
	   }
	   else { /* not to show large window */
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
	   }
	   for ( i= 0; i<windowNum; i++ ) {
	      ACEgrPrintf( "with g%d", i );
	      ACEgrPrintf( "subtitle size %f",0.9 );
	      ACEgrPrintf( "subtitle font 1" );
	      ACEgrPrintf( "subtitle color 1" );
	      ACEgrPrintf( "frame color 1" );
	      ACEgrPrintf( "xaxis tick color 7" );
	      ACEgrPrintf( "xaxis ticklabel prec 0" );
	      ACEgrPrintf( "xaxis ticklabel char size 0.43" );
	      ACEgrPrintf( "xaxis ticklabel format decimal" );
	      if ( winScaler[i] > 0 ) {
		yminimum = winYMin[i];
		ymaximum = winYMax[i];
		ACEgrPrintf( "world ymin %le", yminimum );
		ACEgrPrintf( "world ymax %le", ymaximum );
		ACEgrPrintf( "yaxis tick major %le", (winScaler[i]/ytick_maj) );
		ACEgrPrintf( "yaxis tick minor %le", (winScaler[i]/ytick_min) );
	      }
	      ACEgrPrintf( "yaxis tick color 7" );
	      ACEgrPrintf( "yaxis ticklabel char size 0.43" );
	      ACEgrPrintf( "yaxis ticklabel format decimal" );
	      ACEgrPrintf( "g%d type xy", i );
	      if ( winScaler[i] < 0.0001) {
		 ACEgrPrintf( "yaxis ticklabel format exponential" );
		 ACEgrPrintf( "yaxis ticklabel prec 3" );
	      }
	      else if ( winScaler[i] < 0.01)
		 ACEgrPrintf( "yaxis ticklabel prec 6" );
	      else if ( winScaler[i] < 20.0)
		 ACEgrPrintf( "yaxis ticklabel prec 2" );
	      else
		 ACEgrPrintf( "yaxis ticklabel prec 0" );
	      ACEgrPrintf( "view %f, %f,%f,%f", x0[i]+XSHIFT,y0[i],x0[i]+w,y0[i]+h );
	      ACEgrPrintf( "frame color 1" );
	      ACEgrPrintf( "frame background color 1" );
	      ACEgrPrintf( "frame fill on" );
              switch ( xyType[i] ) {
                case 1: /* Log */
		    if ( winYMax[i] <= ZEROLOG )
		       winScaler[i] = -1.0;
		    else if ( winYMin[i] < ZEROLOG ) {
		       ACEgrPrintf( "world ymin %le", ZEROLOG );
		       rat[i] = winYMax[i]/ZEROLOG;
		    }
		    else {
		       rat[i] = winYMax[i]/winYMin[i];
		    }
		    if ( rat[i] > 100 ) {
		       logmaj = 1.0;
		       logmin = 2.0;
		    }
		    else {
		       logmaj = 1.0;
		       logmin = 1.0;
		    }
		    ACEgrPrintf( "g%d type logy", i );
		    ACEgrPrintf( "yaxis tick major %f", logmaj );
		    ACEgrPrintf( "yaxis tick minor %f", logmin );
		    ACEgrPrintf( "yaxis ticklabel format exponential" );
                    break;
                case 2: /* Ln */
                    break;
                case 3: /* exp */
		    ACEgrPrintf( "yaxis ticklabel format exponential" );
		    ACEgrPrintf( "yaxis ticklabel prec 5" );
                    break;
                default: /* Linear */ 
		    if ( winScaler[i] <0.001 || winScaler[i] > 10000000.0 )
		       ACEgrPrintf( "yaxis ticklabel format exponential" );
                    break;
              }	      
	   }
	   if (sigOn) {
	     ACEgrPrintf( "with g16" ); 
	     w = 0.45;
	     h = 0.35;
	     x1 = 0.04;
	     y1 = 0.62;
	     switch ( windowNum ) {
             case 1: 
             case 2: 
	         w = 0.90;
	         h = 0.44;
		 x1 = 0.06;
		 y1 = 0.51;
                 break;
             case 3: 
                 y1 = 0.57;
                 break;
             case 4: 
             case 6: 
	         w = 0.95;
                 break;
             case 7: 
	         w = 0.60;
                 break;
             case 8: 
	         w = 0.95;
	         h = h+0.03;
		 y1 = 0.58;
                 break;
             case 9: 
             case 10: 
	         h = h+0.03;
		 y1 = 0.58;
                 break;
             case 11: 
             case 12: 
	         h = h+0.087;
		 y1 = 0.50;
                 break;
             case 16: 
                 y1 = 0.619;
                 break;
             default: 
                 break;
	     }
	     /* set up x-axis for amplitude */
	     ACEgrPrintf( "subtitle size %f",0.9 );
	     ACEgrPrintf( "subtitle font 1" );
	     ACEgrPrintf( "subtitle color 1" );
	     ACEgrPrintf( "frame color 1" );
	     
	     ACEgrPrintf( "xaxis tick color 7" );
	     ACEgrPrintf( "xaxis ticklabel prec 0" );
	     ACEgrPrintf( "xaxis ticklabel char size 0.43" );
	     ACEgrPrintf( "xaxis ticklabel format decimal" );
	     if ( winScaler[chSelect] > 0 ) {
	       yminimum = winYMin[chSelect];
	       ymaximum = winYMax[chSelect];
	       ACEgrPrintf( "world ymin %le", yminimum );
	       ACEgrPrintf( "world ymax %le", ymaximum );
	       ACEgrPrintf( "yaxis tick major %le", winScaler[chSelect]/ytick_maj );
	       ACEgrPrintf( "yaxis tick minor %le", winScaler[chSelect]/ytick_min );
	     }
	     ACEgrPrintf( "yaxis tick color 7" );
	     if ( winScaler[chSelect] < 0.01)
	       ACEgrPrintf( "yaxis ticklabel prec 6" );
	     else if ( winScaler[chSelect] < 20.0)
	       ACEgrPrintf( "yaxis ticklabel prec 2" );
	     else
	       ACEgrPrintf( "yaxis ticklabel prec 0" );
	     ACEgrPrintf( "yaxis ticklabel char size 0.43" );
	     ACEgrPrintf( "yaxis ticklabel format decimal" );
	     ACEgrPrintf( "g16 type xy" );
	     ACEgrPrintf( "view %f, %f,%f,%f",x1+XSHIFT,y1,x1+w,y1+h );
	     ACEgrPrintf( "frame color 1" );
	     ACEgrPrintf( "frame background color 1" );
	     ACEgrPrintf( "frame fill on" );
	     if ( winScaler[chSelect] < 0.0001) {
	       ACEgrPrintf( "yaxis ticklabel format exponential" );
	       ACEgrPrintf( "yaxis ticklabel prec 3" );
	     }
	     else if ( winScaler[chSelect] < 0.01)
	       ACEgrPrintf( "yaxis ticklabel prec 6" );
	     else if ( winScaler[chSelect] < 20.0)
	       ACEgrPrintf( "yaxis ticklabel prec 2" );
	     else
	       ACEgrPrintf( "yaxis ticklabel prec 0" );
	     ACEgrPrintf( "view %f, %f,%f,%f",x1+XSHIFT,y1,x1+w,y1+h );
	     ACEgrPrintf( "frame color 1" );
	     ACEgrPrintf( "frame background color 1" );
	     ACEgrPrintf( "frame fill on" );
	     switch ( xyType[chSelect] ) {
	     case 1: /* Log */
	       if ( winYMax[chSelect] <= ZEROLOG )
		 winScaler[chSelect] = -1.0;
	       else if ( winYMin[chSelect] < ZEROLOG ) {
		 ACEgrPrintf( "world ymin %le", ZEROLOG );
	       }
	       if ( rat[chSelect] > 100 ) {
		 logmaj = 1.0;
		 logmin = 2.0;
	       }
	       else {
		 logmaj = 1.0;
		 logmin = 1.0;
	       }
	       ACEgrPrintf( "g16 type logy" );
	       ACEgrPrintf( "yaxis tick major %f", logmaj );
	       ACEgrPrintf( "yaxis tick minor %f", logmin );
	       ACEgrPrintf( "yaxis ticklabel format exponential" );
	       break;
	     case 2: /* Ln */
	       break;
	     case 3: /* exp */
	       ACEgrPrintf( "yaxis ticklabel format exponential" );
	       ACEgrPrintf( "yaxis ticklabel prec 5" );
	       break;
	     default: /* Linear */ 
	       break;
	     }	      
	   }
	}
	for ( i= 0; i<windowNum; i++ ) {
	   ACEgrPrintf( "with g%d", i );
	   ACEgrPrintf( "world xmin %f", (float)from );
	   ACEgrPrintf( "world xmax %f", (float)to );
	   ACEgrPrintf( "xaxis tick major %f", (to-from)/5.0 );
	   ACEgrPrintf( "xaxis tick minor %f", (to-from)/20.0 );
	}
	ACEgrPrintf( "with g0" );
	ACEgrPrintf( "string off" );
	for ( i=0; i<windowNum; i++ ) {
	   ACEgrPrintf( "with g%d", i );
	   ACEgrPrintf( "g%d on", i );
	   ACEgrPrintf( "s0 color %d", gColor[i] );
	   if ( autoon[i] ) ACEgrPrintf( "autoscale" );
	   ACEgrPrintf( "subtitle \"\\-Ch %d:  %s\"", (i+1), chName[i] );
	   /*ACEgrPrintf( "clear stack" );*/
	}
	ACEgrPrintf( "copy g%d.s0 to g16.s0", chSelect );
	if ( sigOn ) {
	ACEgrPrintf( "with g16" );
	ACEgrPrintf( "world xmin %f", (float)from );
	ACEgrPrintf( "world xmax %f", (float)to );
	ACEgrPrintf( "xaxis tick major %f", (to-from)/5.0 );
	ACEgrPrintf( "xaxis tick minor %f", (to-from)/20.0 );
	if ( autoon[chSelect] ) ACEgrPrintf( "autoscale" );
	ACEgrPrintf( "subtitle \"\\-Trend Ch %d:  %s  from %s to %s\"",(chSelect+1),chName[chSelect], trendtimestring, timestring );
	ACEgrPrintf( "g16 on" );
	ACEgrPrintf( "string off" );
	}
	else {
	  ACEgrPrintf( "g16 off" );
	  ACEgrPrintf( "string def \"\\1 Trend Display %d Channels from  %s  to  %s\"",  windowNum, trendtimestring, timestring );
	  ACEgrPrintf( "string loctype view" );
	  ACEgrPrintf( "string 0.27, 0.98" );
	  ACEgrPrintf( "string font 4" );
	  ACEgrPrintf( "string char size 0.9" );
	  ACEgrPrintf( "string color 1" );
	  ACEgrPrintf( "string on" );
	}
	
	ACEgrPrintf( "clear stack" );
	for ( i=windowNum; i<16; i++ ) { 
	  ACEgrPrintf( "kill g%d", i );         
	}
	ACEgrPrintf( "with g0" );
	ACEgrPrintf( "redraw" );
	firsttime = 0;
        if ( warning ) {
	   sprintf ( msgout,"  0 as input of log\n"  );
	   printmessage(-1);
	}
	warning = 0;
	screenChange = 0;
	fflush(stdout);
	ACEgrFlush();

}


void graphmulti()   
{
double ymaximum,yminimum,xminimum,xmaximum;
int i;

/* use global setting */

        if ( initGraph ) {
	  initGraph = 0;
	  ACEgrPrintf( "doublebuffer true" );
	  ACEgrPrintf( "focus off" );
	}
	if ( firsttime || screenChange ) {
	   ACEgrPrintf( "background color 7" );
	   ACEgrPrintf( "with g0" ); /* main graph */
	   ACEgrPrintf( "view %f,%f,%f,%f", 0.08,0.10,0.74,0.85 );
	   ACEgrPrintf( "frame color 1" );
	   ACEgrPrintf( "frame background color 1" );
	   ACEgrPrintf( "frame fill on" );
	   ACEgrPrintf( "subtitle size %f", 0.9 );
	   ACEgrPrintf( "subtitle font 1" );
	   ACEgrPrintf( "subtitle color 1" );
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
	   ACEgrPrintf( "world xmin %f", xminimum );
	   ACEgrPrintf( "world xmax %f", xmaximum );
	   ACEgrPrintf( "xaxis tick major 1" );
	   ACEgrPrintf( "xaxis tick minor 0.5" );
	   ACEgrPrintf( "xaxis tick color 7" );
	   ACEgrPrintf( "xaxis ticklabel prec 2" );
	   ACEgrPrintf( "xaxis ticklabel char size 0.6" );
	   ACEgrPrintf( "xaxis ticklabel format decimal" );
	   yminimum = winYMin[0];
	   ymaximum = winYMax[0];
	   for ( i=1; i<windowNum; i++ ) {
              if ( yminimum > winYMin[i] )
		yminimum = winYMin[i];
	      if ( ymaximum < winYMax[i] )
		ymaximum = winYMax[i];
           }
	   ACEgrPrintf( "world ymin %le", yminimum );
	   ACEgrPrintf( "world ymax %le", ymaximum );
	   ACEgrPrintf( "yaxis tick major %le", (ymaximum-yminimum)/(2*ytick_maj) );
	   ACEgrPrintf( "yaxis tick minor %le", (ymaximum-yminimum)/(2*ytick_min) );
	   ACEgrPrintf( "yaxis tick color 7" );
	   ACEgrPrintf( "yaxis ticklabel prec 4" );
	   ACEgrPrintf( "yaxis ticklabel char size 0.6" );
	   ACEgrPrintf( "yaxis ticklabel format decimal" );
	   ACEgrPrintf( "string def \"\\1        Channel List    \"" );
	   ACEgrPrintf( "string loctype view" );
	   ACEgrPrintf( "string 0.75, 0.78" );
	   ACEgrPrintf( "string font 4" );
	   ACEgrPrintf( "string char size 0.9" );
	   ACEgrPrintf( "string color 1" );
	   ACEgrPrintf( "string on" );

	   ACEgrPrintf( "legend loctype view" );
	   ACEgrPrintf( "legend 0.78, 0.72" );
	   ACEgrPrintf( "legend char size 0.7" );
	   ACEgrPrintf( "legend color 1" );
	   ACEgrPrintf( "legend on" );

	}
	ACEgrPrintf( "with g0" ); /* main graph */
	ACEgrPrintf( "s%d linewidth 2", chSelect );
	ACEgrPrintf( "s10 linestyle 3" );
	ACEgrPrintf( "s11 linestyle 3" );
	ACEgrPrintf( "s12 linestyle 3" );
	ACEgrPrintf( "s13 linestyle 3" );
	ACEgrPrintf( "s14 linestyle 3" );
	ACEgrPrintf( "s15 linestyle 3" );
	ACEgrPrintf( "subtitle \"\\1 Display tY Multiple  %s\"", timestring );
	for ( i=0; i<windowNum; i++ ) {
	   ACEgrPrintf( "s%d color %d", i, gColor[i] ); 
	   if ( uniton[i] )
	     ACEgrPrintf( "legend string %d \"%d: %s %s \"", i,(i+1),chName[i],chUnit[i] );
	   else
	     ACEgrPrintf( "legend string %d \"%d: %s no conv. \"", i,(i+1),chName[i] );
	}
	for ( i=0; i<windowNum; i++ ) {
	   if ( autoon[i] ) ACEgrPrintf( "autoscale" );
	}
	ACEgrPrintf( "clear stack" );

	for ( i=1; i<=16; i++ ) { 
	  ACEgrPrintf( "kill g%d", i );         
	}
	ACEgrPrintf( "with g0" );
	ACEgrPrintf( "g0 on" );
	ACEgrPrintf( "redraw" );
	firsttime = 0;
        if ( screenChange > 5 ) 
	   screenChange = 0;
	fflush(stdout);
	ACEgrFlush();
}


void graphtrmulti(int from, int to)   
{
double ymaximum,yminimum;
int i;

        if ( initGraph ) {
	  initGraph = 0;
	  ACEgrPrintf( "doublebuffer true" );
	  ACEgrPrintf( "focus off" );
	}
	if ( firsttime || screenChange ) {
	   ACEgrPrintf( "background color 7" );
	   ACEgrPrintf( "with g0" ); /* main graph */
	   ACEgrPrintf( "view %f,%f,%f,%f", 0.08,0.10,0.74,0.85 );
	   ACEgrPrintf( "frame color 1" );
	   ACEgrPrintf( "frame background color 1" );
	   ACEgrPrintf( "frame fill on" );
	   ACEgrPrintf( "subtitle size %f", 0.9 );
	   ACEgrPrintf( "subtitle font 1" );
	   ACEgrPrintf( "subtitle color 1" );
	   ACEgrPrintf( "frame color 1" );
	   ACEgrPrintf( "xaxis tick color 7" );
	   ACEgrPrintf( "xaxis ticklabel prec 2" );
	   ACEgrPrintf( "xaxis ticklabel char size 0.6" );
	   ACEgrPrintf( "xaxis ticklabel format decimal" );
	   yminimum = winYMin[0];
	   ymaximum = winYMax[0];
	   for ( i=1; i<windowNum; i++ ) {
              if ( yminimum > winYMin[i] )
		yminimum = winYMin[i];
	      if ( ymaximum < winYMax[i] )
		ymaximum = winYMax[i];
           }
	   ACEgrPrintf( "world ymin %le", yminimum );
	   ACEgrPrintf( "world ymax %le", ymaximum );
	   ACEgrPrintf( "yaxis tick major %le", (ymaximum-yminimum)/(2*ytick_maj) );
	   ACEgrPrintf( "yaxis tick minor %le", (ymaximum-yminimum)/(2*ytick_min) );
	   ACEgrPrintf( "yaxis tick color 7" );
	   ACEgrPrintf( "yaxis ticklabel prec 4" );
	   ACEgrPrintf( "yaxis ticklabel char size 0.6" );
	   ACEgrPrintf( "yaxis ticklabel format decimal" );
	   ACEgrPrintf( "string def \"\\1        Channel List     \"" );
	   ACEgrPrintf( "string loctype view" );
	   ACEgrPrintf( "string 0.75, 0.78" );
	   ACEgrPrintf( "string font 4" );
	   ACEgrPrintf( "string char size 0.9" );
	   ACEgrPrintf( "string color 1" );
	   ACEgrPrintf( "string on" );
	   ACEgrPrintf( "legend loctype view" );
	   ACEgrPrintf( "legend 0.78, 0.72" );
	   ACEgrPrintf( "legend char size 0.7" );
	   ACEgrPrintf( "legend color 1" );
	   ACEgrPrintf( "legend on" );
	   ACEgrPrintf( "s%d linewidth 2", chSelect );
	   ACEgrPrintf( "s10 linestyle 3" );
	   ACEgrPrintf( "s11 linestyle 3" );
	   ACEgrPrintf( "s12 linestyle 3" );
	   ACEgrPrintf( "s13 linestyle 3" );
	   ACEgrPrintf( "s14 linestyle 3" );
	   ACEgrPrintf( "s15 linestyle 3" );
	}
	ACEgrPrintf( "with g0" ); /* main graph */
	ACEgrPrintf( "world xmin %d", from );
	ACEgrPrintf( "world xmax %d", to );
	ACEgrPrintf( "xaxis tick major %f", (to-from)/5.0 );
	ACEgrPrintf( "xaxis tick minor %f", (to-from)/20.0 );
	ACEgrPrintf( "subtitle \"\\1 Trend Multiple from %s to %s\"", trendtimestring, timestring );
	for ( i=0; i<windowNum; i++ ) {
	   ACEgrPrintf( "s%d color %d", i, gColor[i] );      
	   ACEgrPrintf( "legend string %d \"Ch %d:%s \"", i,(i+1),chName[i] );
	}
	for ( i=0; i<windowNum; i++ ) {
	   if ( autoon[i] ) ACEgrPrintf( "autoscale" );
	}
	ACEgrPrintf( "clear stack" );

	for ( i=1; i<=16; i++ ) { 
	  ACEgrPrintf( "kill g%d", i );         
	}
	ACEgrPrintf( "redraw" );
	firsttime = 0;
        if ( warning ) {
	   sprintf ( msgout,"  0 as input of log\n"  );
	   printmessage(-1);
	}
	warning = 0;
        if ( screenChange ) 
	   screenChange = 0;
	fflush(stdout);
	ACEgrFlush();
}





/* switch color: switch/adjust backgroud and frame colors while quitting. */
void  switchcolor()
{
int i, j;

       ACEgrPrintf( "background color 7" );
       switch ( graphMethod ) {
       case GMODESTAND: 
	 for ( i=0; i<=windowNum; i++ ) {
	   if ( i == windowNum )    
	     j = 16;
	   else
	     j = i;
	   ACEgrPrintf( "with g%d", j );
	   /*ACEgrPrintf( "s0 color %d", gqColor[j] );*/
	   ACEgrPrintf( "frame background color 0" );
	   ACEgrPrintf( "xaxis tick color 1" );
	   ACEgrPrintf( "yaxis tick color 1" );
	 }
	 break;
       case GMODEMULTI: 
	 ACEgrPrintf( "with g0" ); 
	 /*for ( i=0; i<windowNum; i++ ) {
	   ACEgrPrintf( "s%d color %d", i, mqColor[i] );
	   }*/
	 ACEgrPrintf( "frame background color 0" );
	 ACEgrPrintf( "xaxis tick color 1" );
	 ACEgrPrintf( "yaxis tick color 1" );
	 ACEgrPrintf( "string color 1" );
	 ACEgrPrintf( "legend color 1" );
	 
	 ACEgrPrintf( "with g1" ); 
	 ACEgrPrintf( "frame background color 0" );
	 ACEgrPrintf( "xaxis tick color 1" );
	 ACEgrPrintf( "yaxis tick color 1" );
	 break;
       }
       
       ACEgrPrintf( "with g16" ); 
       ACEgrPrintf( "with string 0" );
       ACEgrPrintf( "string color 1" );
       ACEgrPrintf( "with g0" );
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
       ACEgrPrintf( "print to file \"%s\"", printfile );
       ACEgrPrintf( "hardcopy" );
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

       fprintf ( stderr, "Print Hardcopy\n" );
       switchcolor(printing);
       ACEgrPrintf( "redraw" );
       sprintf ( printfile, "%s.ps", timestring );
       if ( printing == 1 ) {
	  fprintf ( stderr, "printing %s to printer\n", printfile );
	  ACEgrPrintf( "print hardcopy \"lpr\"" ); this one or next--dont work
	  ACEgrPrintf( "print to hardcopy" );
       }
       else {
	  fprintf ( stderr, "printing %s to file\n", printfile );
	  ACEgrPrintf( "print to file \"%s\"", printfile );
       }
       ACEgrPrintf( "hardcopy" );
       switchcolor(-1);
       ACEgrPrintf( "redraw" );
       
       fflush(stdout);
       printing = -1;
       firsttime = 1;
       return;
}
*/


/* quit display: how=0 quit xmgr; how=1 exit */
void  quitdisplay(int how) 
{
       sprintf ( msgout, "Quit data display\n" );
       printmessage(2);
       if ( !stopped ) {
	  DataWriteStop(processID);
	  stopped = 1;
       }
       //sleep(1);
       if ( how == 1 )
	  ACEgrClose();  /* close xmgr */
       else {
	  switchcolor();
	  ACEgrPrintf( "redraw" );
	  ACEgrFlush ();        
	  //sleep(2);
	  /* close named-pipe - this is not a standard xmgr command */
	  ACEgrPrintf( "close pipe" ); 
       }
       msgctl(msqid, IPC_RMID, (struct msqid_ds *) 0);
       DataQuit();
       fflush(stdout);
       fclose(fwarning);
       fclose(stdout);
       //sleep(1);
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
	       ACEgrPrintf( "g0.s%d point %2f, %le", j, (float)tcount, trms );
	     else
	       ACEgrPrintf( "g%d.s0 point %2f, %le", j, (float)tcount, trms );
	   }
	   i++;
	}
	fclose(fppause);
	sprintf ( msgout,"Resume Done\n" );
	printmessage(0);
	return;
}


/* initial Xmgr */
void  graphini()
{
int j;
        ACEgrPrintf( "doublebuffer true" );
        ACEgrPrintf( "background color 1" );
        ACEgrPrintf( "view ymin 0.001" );
        ACEgrPrintf( "view ymax 0.999" );
        ACEgrPrintf( "view xmin 0.001" );
        ACEgrPrintf( "view xmax 0.999" );
        ACEgrPrintf( "with string 0" );
        ACEgrPrintf( "string loctype view" );
        ACEgrPrintf( "string 0.3, 0.7" );
        ACEgrPrintf( "string font 1" );
        ACEgrPrintf( "string char size 3.0" );
        ACEgrPrintf( "string color 2" );
        ACEgrPrintf( "string def \"DATAVIEWER\"" );
        ACEgrPrintf( "string on" );
        ACEgrPrintf( "with string 1" );
        ACEgrPrintf( "string loctype view" );
        ACEgrPrintf( "string 0.35, 0.6" );
        ACEgrPrintf( "string font 2" );
        ACEgrPrintf( "string char size 1.5" );
        ACEgrPrintf( "string color 5" );
        ACEgrPrintf( "string def \"LIGO Project\"" );
        ACEgrPrintf( "string on" );
	for ( j=0; j<16; j++ ) {
           ACEgrPrintf( "g%d off", j );
        }
        ACEgrPrintf( "redraw" );
	ACEgrFlush ();        
}

void  printmessage(int stat)
/* stat: 1-don't print (reset) */
{
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
	  fprintf ( stderr, "Unexpected error happened while sending request to server. Please re-connect to the server.\n"  );
	  fprintf ( fwarning, "ERROR: Unexpected error happened while sending request to server. Please re-connect to the server.\n" );
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
