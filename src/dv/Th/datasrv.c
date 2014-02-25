/*----------------------------------------------------------------------*/
/*                                                                      */
/* Module Name: datasrv.c                                               */
/* For Linux                                                            */
/*----------------------------------------------------------------------*/


#include <string.h> /* declarations of strcpy and others.*/
#include "datasrv.h"
#include "UTC_GPS/leapsecs.h"
#include "UTC_GPS/tai.h"
#include "UTC_GPS/caltime.h"
#include "math.h"

struct caltime ctin;
struct caltime ctout;
struct gps  gpsTime;
struct gps gps;

static int debug = 1 ; /* For debugging - JCB */

#define  COMMANDSIZE   2048

char          configure[COMMANDSIZE]; 
int           listenerPort, totalgroup;
daq_t         DataDaq;
int           bread, trendLength=1;

daq_channel_group_t groups[MAX_CHANNEL_GROUPS];
daq_channel_t chanList[MAX_CHANNELS]; 
int           rateActual[MAX_CHANNELS];
             /*dataRate; to store the actual rate user requested */
int           NchanList = 0;
short         all = -1;  /* 1 all channels; 0 chan list; -1 none */

int           NchannelAll;
daq_channel_t channelAll[MAX_CHANNELS];

short         Fast = 1; /* 1 or 16 */

/* Debug fprintf().  */
void dfprintf(FILE *file, ...) {}

/* int DataConnect(char* serverName, int serverPort, int lPort, void* read_data()) */ /* JCB */
int DataConnect(char* serverName, int serverPort, int lPort, void* (*read_data)())
{
      int  j;

      if (debug != 0) fprintf(stderr, "DataConnect()\n") ; /* JCB */

       listenerPort = lPort;
       if (debug != 0) fprintf(stderr, "DataConnect() - calling daq_initialize()\n") ; /* JCB */
       if (0 == daq_initialize(&DataDaq, &listenerPort, read_data)) {
	  perror("Couldn't initialize DAQ threads");
	  return 1;
       }
       if (debug != 0) fprintf(stderr, "DataConnect() - calling daq_connect()\n") ; /* JCB */
       if ( daq_connect(&DataDaq, serverName, serverPort) ) {
          //perror( "ERROR: Couldn't connect to the NDS server" );
          return 1;
       }
       if (debug != 0) fprintf(stderr, "DataConnect() - calling daq_recv_channels()\n") ; /* JCB */
       if ( daq_recv_channels(&DataDaq, channelAll, MAX_CHANNELS, &NchannelAll) ) {
          //perror( "ERROR: Failed to receive channel information" );
          return 1;
       }

       dfprintf ( stderr, "datasrv: DataConnect: total %d channels configured\n", NchannelAll );



       /*for ( j = 0; j<NchannelAll; j++ )
	 fprintf ( stderr, "%s\t%d\n", channelAll[j].name, channelAll[j].rate );*/
       
       if (debug != 0) fprintf(stderr, "   protocol revision = %d\n", DataDaq.rev) ; /* JCB */
       if (debug != 0) fprintf(stderr, "DataConnect() - returning\n") ; /* JCB */

       return 0;
}

int DataSimpleConnect(char* serverName, int serverPort)
{
      int  j;

       if ( daq_connect(&DataDaq, serverName, serverPort) ) {
          //perror( "ERROR: Couldn't connect to the NDS server" );
          return 1;
       }
       if ( daq_recv_channels(&DataDaq, channelAll, MAX_CHANNELS, &NchannelAll) ) {
          //perror( "ERROR: Failed to receive channel information" );
          return 1;
       }
       dfprintf ( stderr, "datasrv: DataSimpleConnect: total %d channels configured\n", NchannelAll );
       
       return 0;
}


int DataReConnect(char* serverName, int serverPort)
{
      int  j;

       daq_disconnect(&DataDaq);
       //sleep(1);
       if ( daq_connect(&DataDaq, serverName, serverPort) ) {
          //perror( "ERROR: Failed to connect to the NDS server" );
          return 1;
       }
       if ( daq_recv_channels(&DataDaq, channelAll, MAX_CHANNELS, &NchannelAll) ) {
          //perror( "ERROR: Failed to receive NDS channel information");
          return 1;
       }

#if 0
       dfprintf ( stderr, "datasrv: DataReConnect: total %d channels configured\n", NchannelAll );
#endif
       
       return 0;
}


void DataQuit()
{
#if 0
       daq_shutdown(&DataDaq);
#endif 
       daq_disconnect(&DataDaq);
       dfprintf ( stderr,"datasrv: DataQuit\n" );
       return;
}


/* returns total channel number in the list  */
int DataChanAdd(const char* chName, int dataRate)
{
      int  index, j;
    
      if ( all == 1 ) 
         return NchannelAll;
      if ( strcmp(chName, "all") == 0 ) {
         all = 1;
         return NchannelAll;
      }
      all = 0;
      index = SearchChList(chName, channelAll, NchannelAll);
      if ( index< 0 ) {
	 dfprintf( stderr,"datasrv: DataChanAdd: cann't add channel %s\n", chName);
	 fprintf(stderr, "Error: Bad channel name `%s`\n", chName);
	 return -1;
      }
      if ( dataRate > channelAll[index].rate ) {
	 dataRate = channelAll[index].rate;

	 dfprintf (stderr,
		  "rate requested too large, use the sample rate %d\n",
		  dataRate);

      }
      for ( j=0; j<NchanList; j++ ) {
	 if ( strcmp(chName, chanList[j].name) == 0) { /* if on the list already  */

	    if ( dataRate == 0 ) {
	       chanList[j].rate = channelAll[index].rate;
	       rateActual[j] = channelAll[index].rate;
	    }
	    else {
	       chanList[j].rate = dataRate;
	       rateActual[j] = dataRate;
	    }
            return NchanList;
	 }
      }
      /* if it's a new channel */
      strcpy( chanList[NchanList].name, chName );
      chanList[NchanList].data_type  = channelAll[index].data_type ;
      if ( dataRate == 0 ) {
	 chanList[NchanList].rate = channelAll[index].rate;
	 rateActual[NchanList] = channelAll[index].rate;
      }
      else {
	 chanList[NchanList].rate = dataRate;
	 rateActual[NchanList] = dataRate;
      }
      NchanList++;
      /*fprintf(stderr,"channel %s has been added\n", chName);*/
      return NchanList;
}


/* returns total channel number in the list  */
int DataChanDelete(const char* chName)
{
      int  j, pos=-1;

      if ( strcmp(chName, "all") == 0 ) {
         NchanList = 0;
	 all = -1;
	 return 0;
      }
      for ( j=0; j<NchanList; j++ ) {
         if ( strcmp(chName, chanList[j].name) == 0 ) {
            pos = j;
            break;
         }
      }
      if ( pos >= 0 ) {
         for ( j=pos; j<NchanList-1; j++ ) {
            strcpy( chanList[j].name, chanList[j+1].name);
	    chanList[j].rate = chanList[j+1].rate;
	    chanList[j].data_type = chanList[j+1].data_type;
	    rateActual[j] = rateActual[j+1];
         }
         dfprintf(stderr,"datasrv: channel %s has been deleted\n", chName);
         NchanList--;
      }
      else fprintf(stderr,"Error: Bad channel name `%s`\n", chName);
      return NchanList;
}


unsigned long DataWriteRealtime()
{
      unsigned long ID;
      int    j;
      char   temp[COMMANDSIZE];
      int    isend;

       if ( all < 0 ) {
          perror( "datasrv: DataChanSet failed: no channel selected" );
	  return -1;
       }
       sprintf ( configure, "start net-writer \"%d\" ", listenerPort );
       if ( all == 1 ) {
	  strcat( configure, "all ");
       }
       else {
	  strcat( configure, "{");
          for ( j=0; j<NchanList; j++ ) {
             strcpy( temp, configure);
	     if ( rateActual[j] == 0 ) {
	        strcat( temp, " \"%s\"");
		sprintf( configure, temp, chanList[j].name );
	     }
	     else {
	        strcat( temp, " \"%s\" %d");
		sprintf( configure, temp, chanList[j].name, rateActual[j] );
             }
	  }
	  strcat( configure, " }");
       }
       strcat(configure, ";");
       dfprintf ( stderr,"datasrv: configure chan = %s\n", configure );
       isend = daq_send(&DataDaq, configure);
       if ( isend ) 
          perror( "datasrv: DataWriteRealtime failed: daq_send" );
       if ( isend == 1 || isend == 10 ) { /* need to re-connect */
          return -1;
       }
       else if ( isend == 14 ) { 
	  fprintf ( stderr,"Error: Test point(s) not found.\n" );
          return -2;
       }
       else if ( isend ) { 
          return -2;
       }
       Fast = 1;

       ID = daq_recv_id (&DataDaq);
       dfprintf ( stderr,"datasrv: DataWriteRealtime ID=%d\n", ID );
       return ID;
}


unsigned long DataWriteRealtimeFast()
{
      unsigned long ID;
      int    j;
      char   temp[COMMANDSIZE];
      int    isend;

       if ( all < 0 ) {
          perror( "datasrv: DataChanSet failed: no channel selected" );
	  return -1;
       }
       sprintf ( configure, "start fast-writer \"%d\" ", listenerPort );
       if ( all == 1 ) {
	  strcat( configure, "all ");
       }
       else {
	  strcat( configure, "{");
          for ( j=0; j<NchanList; j++ ) {
             strcpy( temp, configure);
	     if ( rateActual[j] == 0 ) {
	        strcat( temp, " \"%s\"");
		sprintf( configure, temp, chanList[j].name );
	     }
	     else {
	        strcat( temp, " \"%s\" %d");
		sprintf( configure, temp, chanList[j].name, rateActual[j] );
             }
	  }
	  strcat( configure, " }");
       }
       strcat(configure, ";");
       dfprintf ( stderr,"datasrv: configure chan = %s\n", configure );
       isend = daq_send(&DataDaq, configure);
       if ( isend ) 
          perror( "datasrv: DataWriteRealtimeFast failed: daq_send" );
       if ( isend == 1 || isend == 10 ) { /* need to re-connect */
          return -1;
       }
       else if ( isend == 14 ) { 
	  fprintf ( stderr,"Error: Test point(s) not found.\n" );
          return -2;
       }
       else if ( isend ) { 
          return -2;
       }
       Fast = 16;

       ID = daq_recv_id (&DataDaq);
       fprintf ( stderr,"datasrv: DataWriteRealtimeFast ID=%lu\n", ID );
       return ID;
}


unsigned long    DataWrite(char* starttime, int duration, int isgps)
{
      unsigned long ID;
      int    j;
      char   temp[COMMANDSIZE];

      long   starttimeInsec, mjd;
      int    yr,mo,da,hr,mn,sc;
      int    isend;
     
       if ( isgps == 0 ) { /* UTC time */
	 sscanf (starttime, "%d-%d-%d-%d-%d-%d", &yr,&mo,&da,&hr,&mn,&sc );
	 /*----- enter time in UTC ------------*/
	 if ( yr == 98 || yr == 99 )
	   ctin.date.year  = yr + 1900;
	 else
	   ctin.date.year  = yr + 2000;
	 ctin.date.month = mo;       /* [1,12] */
	 ctin.date.day   = da;       /* [1,31] */
	 ctin.hour       =hr;        /* [0,23] */
	 ctin.minute     =mn;        /* [0,59] */
	 ctin.second     =sc;        /* [0,60] */
	 ctin.offset     =0;         /* offset in minutes from UTC [-5999,5999] */
	 mjd=caldate_mjd(&ctin.date); /* modified Julian day */
	 utc_to_gps(&ctin,&gps);
	 starttimeInsec = gps.sec;
       }
       else {
	 sscanf (starttime, "%ld", &starttimeInsec );
       }
       if ( all < 0 ) {
          perror( "datasrv: DataChanSet failed: no channel selected" );
	  return -1;
       }
       sprintf ( configure, "start net-writer \"%d\" %ld %d ", listenerPort, starttimeInsec, duration );
       if ( all == 1 ) {
	  strcat( configure, "all ");
       }
       else {
          strcat( configure, "{");
          for ( j=0; j<NchanList; j++ ) {
             strcpy( temp, configure);
	     if ( rateActual[j] == 0 ) {
	        strcat( temp, " \"%s\"");
		sprintf( configure, temp, chanList[j].name );
	     }
	     else {
	        strcat( temp, " \"%s\" %d");
		sprintf( configure, temp, chanList[j].name, rateActual[j] );
	     }
	  }
          strcat( configure, " }");
       }
       strcat(configure, ";");
       dfprintf ( stderr,"datasrv: configure chan = %s\n", configure );
       isend = daq_send(&DataDaq, configure);
       if ( isend ) 
          perror( "datasrv: DataWrite failed: daq_send" );
       if ( isend == 1 || isend == 10 ) { /* need to re-connect */
          return -1;
       }
       else if ( isend == 14 ) { 
	  fprintf ( stderr,"Error: Test point(s) not found.\n" );
          return -2;
       }
       else if ( isend ) { 
          return -2;
       }

      Fast = 1;
      ID = daq_recv_id (&DataDaq);
      dfprintf ( stderr,"\n" );
      dfprintf ( stderr,"DataWrite ID=%d starttime=%s(%d) duration=%d\n", ID,starttime,starttimeInsec, duration );
      return ID;
}


unsigned long DataWriteTrendRealtime()
{
      unsigned long ID;
      int    j;
      char   temp[COMMANDSIZE];
      int    isend;

       if ( all < 0 ) {
          perror( "datasrv: DataChanSet failed: no channel selected" );
	  return -1;
       }
       trendLength = 1;
       sprintf ( configure, "start trend net-writer \"%d\" ", listenerPort ); 
       if ( all == 1 ) {
	  strcat( configure, "all ");
       }
       else {
          strcat( configure, "{");
          for ( j=0; j<NchanList; j++ ) {
             strcpy( temp, configure);
	     strcat( temp, " \"%s.min\" \"%s.max\" \"%s.mean\"");
	     sprintf( configure, temp, chanList[j].name, chanList[j].name, chanList[j].name );
	  }
          strcat( configure, " }");
       }
       strcat(configure, ";");
       dfprintf ( stderr,"datasrv: configure chan = %s\n", configure );
       isend = daq_send(&DataDaq, configure);
       if ( isend ) 
          perror( "datasrv: DataWriteTrendRealtime failed: daq_send" );
       if ( isend == 1 || isend == 10 ) { /* need to re-connect */
          return -1;
       }
       else if ( isend == 14 ) { 
	  fprintf ( stderr,"Error: Test point(s) not found.\n" );
          return -2;
       }
       else if ( isend ) { 
          return -2;
       }

       ID = daq_recv_id (&DataDaq);
       dfprintf ( stderr,"datasrv: DataWriteTrendRealtime ID=%d\n", ID );
       return ID;
}


unsigned long    DataWriteTrend(char* starttime, int duration, int trendlength,  int isgps)
{
      unsigned long ID;
      int    j;
      char   temp[COMMANDSIZE];

      long   starttimeInsec, mjd;
      int    yr,mo,da,hr,mn,sc;
      int    isend=0;
     
       if (debug != 0) fprintf(stderr, "DataWriteTrend()\n") ; /* JCB */
       if ( all < 0 ) {
          perror( "datasrv: DataChanSet failed: no channel selected" );
          return -1;
       }
       trendLength = trendlength;
       if ( strcmp(starttime, "0") != 0 ) {
	  if (isgps == 0) { /* UTC time */
	    sscanf (starttime, "%d-%d-%d-%d-%d-%d", &yr,&mo,&da,&hr,&mn,&sc );
	    if ( mo == 0 ) {
	      strcpy(starttime, "0");
	      starttimeInsec = 0;
	    }
	    else {
	      if ( yr == 98 || yr == 99 )
		ctin.date.year  = yr + 1900;
	      else
		ctin.date.year  = yr + 2000;
	      ctin.date.month = mo;    
	      ctin.date.day   = da;    
	      ctin.hour       =hr;     
	      ctin.minute     =mn;     
	      ctin.second     =sc;     
	      ctin.offset     =0;      
	      mjd=caldate_mjd(&ctin.date); /* modified Julian day */
	      utc_to_gps(&ctin,&gps);
	      starttimeInsec = gps.sec;
	    }
	  }
	  else 
	    sscanf (starttime, "%ld", &starttimeInsec );
       }
       if ( strcmp(starttime, "0") != 0 ) {
	  if ( trendlength == 60 ) {
	     sprintf ( configure, "start trend %d net-writer \"%d\" %ld %d ", trendlength, listenerPort, starttimeInsec, duration );
	  }
	  else
	     sprintf ( configure, "start trend net-writer \"%d\" %ld %d ", listenerPort, starttimeInsec, duration );
       }
       else {
	  starttimeInsec = 0;
	  sprintf ( configure, "start trend net-writer \"%d\" %d ", listenerPort, duration );
       }
       if ( all == 1 ) {
          strcat( configure, "all ");
       }
       else {
          strcat( configure, "{");
          for ( j=0; j<NchanList; j++ ) {
             strcpy( temp, configure);
             strcat( temp, " \"%s.min\" \"%s.max\" \"%s.mean\"");
             sprintf( configure, temp, chanList[j].name, chanList[j].name, chanList[j].name );
          }
          strcat( configure, " }");
       }
       strcat(configure, ";");
       dfprintf ( stderr,"datasrv: configure chan = %s\n", configure );
       if (debug != 0) 
       {
          fprintf(stderr, "DataWriteTrend() - command string = %s\n", configure) ; /* JCB */
          fprintf(stderr, "DataWriteTrend() - Calling daq_send() \n") ; /* JCB */
	 }
       isend = daq_send(&DataDaq, configure);
       if ( isend ) 
          fprintf ( stderr, "datasrv: DataWriteTrend failed in daq_send().\n" );
       if ( isend == 1 || isend == 10 ) { /* need to re-connect */
          return -1;
       }
       else if ( isend == 14 ) { 
	  fprintf ( stderr,"Error: Test point(s) not found.\n" );
          return -2;
       }
       else if ( isend ) { 
	  fprintf( stderr, "unknown error returned from daq_send()") ; /* JCB */
          return -2;
       }

      if (debug != 0) fprintf(stderr, "DataWriteTrend() - calling daq_recv_id()\n") ; /* JCB */
      ID = daq_recv_id (&DataDaq);
      if (debug != 0) fprintf(stderr, "DataWriteTrend() - daq_recv_id() returned %lu\n", ID) ; /* JCB */
      dfprintf ( stderr,"\n" );
      dfprintf ( stderr,"DataWriteTrend ID=%d starttime=%s(%d) duration=%d\n", ID, starttime,starttimeInsec, duration );
      return ID;
}


void DataWriteStop(unsigned long processID)
{
      char  tempstring[COMMANDSIZE];
      int   isend;

       sprintf( tempstring, "kill net-writer %ld;", processID );
       dfprintf ( stderr,"kill net-writer %d;\n", processID );
       isend = daq_send(&DataDaq, tempstring);
#if 0
       if ( isend ) /* error happened */
	 fprintf ( stderr,"datasrv: DataWriteStop ID=%d error %d\n", processID, isend );
       else
	 fprintf ( stderr,"datasrv: DataWriteStop ID=%d\n", processID );
#endif
       return;
}


void DataReadStart()
{
      unsigned long blocknum;

       if ( !(blocknum = daq_recv_block_num(&DataDaq)) ) {
	 dfprintf ( stderr, "receiving data on-line %d \n", blocknum );
       } else {
	 dfprintf ( stderr, "receiving %d data blocks off-line\n", blocknum );
       }
       return;
}


int DataRead()
{
       bread = daq_recv_block(&DataDaq);
       return bread;
}


void DataReadStop()
{
       if ( daq_recv_shutdown(&DataDaq) )
          perror( "Error: Socket shutdown() failed\n");
       dfprintf ( stderr,"datasrv: DataReadStop\n" );
       return;
}


int DataTimeNow(char* serverName, int serverPort)
{
int itemp;

       dfprintf ( stderr,"datasrv: DataTimeNow\n" );
       if ( daq_connect(&DataDaq, serverName, serverPort) ) {
          //perror( "ERROR: Couldn't connect to the NDS server" );
          return 1;
       }
       itemp = daq_send(&DataDaq, "gps;");
       daq_recv_id (&DataDaq);
       DataReadStart();
       DataRead();
       DataQuit();
       return itemp;
}


int DataGetCh(const char* chName, double data[], int bpos, int complexType)
{
int  j, index, pos;
int  seconds;

       if ( all < 0 ) {
	  fprintf ( stderr,"datasrv: DataGetCh failed: %s is not in the channel list\n", chName );
          return -1;
       }
       seconds = DataDaq.tb->secs;
       if ( seconds <= 0 ) {
          fprintf ( stderr,"datasrv: DataGetCh failed:%d no data received\n", seconds );
	  return -1;
       }
       if ( all == 1 ) { /* all channels */
          index = SearchChList(chName,channelAll,NchannelAll);
	  if ( index < 0 ) {
	     fprintf ( stderr,"datasrv: DataGetCh failed: %s is not in the channel list\n", chName );
             return -1;
	  }
	  pos = 0;
	  if ( index > 0 ) {
	     for ( j=0; j<index; j++ ) {
                pos += channelAll[j].rate*getBlockLength(channelAll[j].data_type, 0)*seconds;
             }
	  }
	  pos += channelAll[index].rate*getBlockLength(channelAll[j].data_type, 0)*bpos;
	  pos /= Fast;
          switch ( channelAll[index].data_type ) {
            case 4: /* 32 bit-float */
                for ( j=0; j<channelAll[index].rate/Fast; j++ ) {
//		   data[j] = *((float *)(DataDaq.tb->data+pos+j*sizeof(float)));
		   float v;
		   *((unsigned int *)&v) = ntohl(*((unsigned int *)(DataDaq.tb->data+pos+j*sizeof(unsigned int))));
		   data[j] = v;

                }
                break;
            case 6: /* complex */
                for ( j=0; j<channelAll[index].rate/Fast; j++ ) {
		   float v1 = *((float *)(DataDaq.tb->data+pos+j*8));
		   float v2 = *((float *)(DataDaq.tb->data+pos+j*8 + 4));
		   *((unsigned int *)&v1) = ntohl(*((unsigned int *)&v1));
		   *((unsigned int *)&v2) = ntohl(*((unsigned int *)&v2));
		   switch (complexType) {
		     case 1: data[j] = v1; break;
		     case 2: data[j] = v2; break;
		     case 3: data[j] = sqrt(v1*v1 + v2*v2); break;
		     case 4: data[j] = (180.0/M_PI)*atan2(v2,v1); break;
		     default: data[j] = 0x777; break;
		   }
		
                }
		break;
            case 5: /* 64 bit-double */
                for ( j=0; j<channelAll[index].rate/Fast; j++ ) {
		   data[j] = *((double *)(DataDaq.tb->data+pos+j*sizeof(double)));
                }
                break;
            case 2: /* 32 bit-integer */
                for ( j=0; j<channelAll[index].rate/Fast; j++ ) {
		   data[j] = *((int *)(DataDaq.tb->data+pos+j*sizeof(int)));
                }
                break;
            default: /* 16 bit-integer */
                for ( j=0; j<channelAll[index].rate/Fast; j++ ) {
		   data[j] = *((short *)(DataDaq.tb->data+pos+j*sizeof(short)));
                }
                break;
          }
	  return channelAll[index].rate;
       }
       else { /* selected channels */
          index = SearchChList(chName, chanList, NchanList);
	  if ( index < 0 ) {
	     fprintf ( stderr,"datasrv: DataGetCh failed: %s is not in the channel list\n", chName );
             return -1;
	  }
	  pos = 0;
	  if ( index > 0 ) {
	     for ( j=0; j<index; j++ ) {
                pos += chanList[j].rate*getBlockLength(chanList[j].data_type, 0)*seconds;
             }
	  }
	  pos += chanList[index].rate*getBlockLength(chanList[index].data_type, 0)*bpos;
	  pos /= Fast;
/*fprintf (stderr, "chan=%s, index=%d, pos=%d ", chName, index, pos);*/
          switch ( chanList[index].data_type ) {
            case 4: /* 32 bit-float */
                for ( j=0; j<chanList[index].rate/Fast; j++ ) {
//		   data[j] = *((float *)(DataDaq.tb->data+pos+j*sizeof(float)));
		   float v = 0.0;
		   // make sure there is data buffer there
		   if (DataDaq.tb_size >= pos+j*sizeof(unsigned int)) {
		    *((unsigned int *)&v) = ntohl(*((unsigned int *)(DataDaq.tb->data+pos+j*sizeof(unsigned int))));
		   }
		   data[j] = v;
                }
                break;
            case 6: /* complex */
                for ( j=0; j<chanList[index].rate/Fast; j++ ) {
		   float v1 = 0.0;
		   float v2 = 0.0;
		   if (DataDaq.tb_size >= pos+j*8) { 
	  	     v1 = *((float *)(DataDaq.tb->data+pos+j*8));
		   }
		   if (DataDaq.tb_size >= pos+j*8 + 4) { 
		     v2 = *((float *)(DataDaq.tb->data+pos+j*8 + 4));
		   }
		   *((unsigned int *)&v1) = ntohl(*((unsigned int *)&v1));
		   *((unsigned int *)&v2) = ntohl(*((unsigned int *)&v2));
		   switch (complexType) {
		     case 1: data[j] = v1; break;
		     case 2: data[j] = v2; break;
		     case 3: data[j] = sqrt(v1*v1 + v2*v2); break;
		     case 4: data[j] = (180.0/M_PI)*atan2(v2,v1); break;
		     default: data[j] = 0x777; break;
		   }
                }
		break;
            case 5: /* 64 bit-double */
                for ( j=0; j<chanList[index].rate/Fast; j++ ) {
		   if (DataDaq.tb_size >= pos+j*sizeof(double)) {
		     data[j] = *((double *)(DataDaq.tb->data+pos+j*sizeof(double)));
		   }
                }
                break;
            case 2: /* 32 bit-integer */
                for ( j=0; j<chanList[index].rate/Fast; j++ ) {
		   if (DataDaq.tb_size >= pos+j*sizeof(int)) {
		     data[j] = (int) ntohl(*((int *)(DataDaq.tb->data+pos+j*sizeof(int))) );
		   }
               }
               break;
            default: /* 16 bit-integer */
                for ( j=0; j<chanList[index].rate/Fast; j++ ) {
		   if (DataDaq.tb_size >= pos+j*sizeof(short)) {
		     data[j] = (short) ntohs(*((short *)(DataDaq.tb->data+pos+j*sizeof(short))) );
		   }
               }
               break;
          }
	  /* 
          for ( j=0; j<chanList[index].rate/Fast; j++ ) {
	    data[j] = ((short *)(DataDaq.tb->data))[pos+j];
          }
	  */
	  return rateActual[index];
       }
}

#if __BYTE_ORDER == __LITTLE_ENDIAN

inline
double ntohd(double in) {
    double retVal;
    char* p = (char*)&retVal;
    char* i = (char*)&in;
    p[0] = i[7];
    p[1] = i[6];
    p[2] = i[5];
    p[3] = i[4];

    p[4] = i[3];
    p[5] = i[2];
    p[6] = i[1];
    p[7] = i[0];

    return retVal;
}
#else
#define ntohd(a) a
#endif

int    DataTrendGetCh(const char* chName, struct DTrend *trend)
{
int  j, index, pos, typesize;
int  seconds, secrate;
	float min, max;
	double mean, mean1;
       if ( all < 0 ) {
	  fprintf ( stderr,"datasrv: DataTrendGetCh failed: %s is not in the channel list\n", chName );
          return -1;
       }
       seconds = DataDaq.tb->secs;
       if ( seconds <= 0 ) {
          fprintf ( stderr,"datasrv: DataTrendGetCh failed:%d no data received\n", seconds );
	  return -1;
       }
       /* secrate: number of data blocks received; trendLength is 1 or 60 */
       secrate = seconds/trendLength; 
       if ( all == 1 ) { /* all channels */
          index = SearchChList(chName,channelAll,NchannelAll);
	  if ( index < 0 ) 
             return -1;
	  pos = 0;
	  if ( index > 0 ) {
	     for ( j=0; j<index; j++ ) {
                pos += 2*getBlockLength(channelAll[j].data_type, 1) + sizeof(double);
             }
	  }
	  pos = pos*secrate;
	  /*fprintf ( stderr,"pos=%d\n", pos );*/
          switch ( channelAll[index].data_type ) {
            case 4: /* 32 bit-float */
	    case 6: /* complex */
	        for ( j=0; j<secrate; j++ ) {
		   min = *((float *)( DataDaq.tb->data + pos +j*sizeof(float) ));
		   max = *((float *)( DataDaq.tb->data + pos + sizeof(float)*secrate + j*sizeof(float) ));
		   *((unsigned int *)&max) = ntohl(*((unsigned int *)&max));
		   mean = *((double *)( DataDaq.tb->data + pos + 2*sizeof(float)*secrate + j*2*sizeof(float) ));
#if defined __linux__ || defined __APPLE__
		   mean1 = ntohd(mean);
#else
		   mean1 = mean;
#endif
		   (trend+j)->min = min;
		   (trend+j)->max =  max;
		   (trend+j)->mean = mean1;
	        }
                break;
            case 5: /* 64 bit-double */
	        for ( j=0; j<secrate; j++ ) {
		   (trend+j)->min = ntohd(*((double *)( DataDaq.tb->data + pos +j*sizeof(double) )));
		   (trend+j)->max = ntohd(*((double *)( DataDaq.tb->data + pos + sizeof(double)*secrate + j*sizeof(double) )));
		   (trend+j)->mean = ntohd(*((double *)( DataDaq.tb->data + pos + 2*sizeof(double)*secrate + j*2*sizeof(double) )));
	        }
                break;
            default: /* 16 bit-integer */
	        for ( j=0; j<secrate; j++ ) {
		   (trend+j)->min = ntohl(*((int *)( DataDaq.tb->data + pos +j*sizeof(int) )));
		   (trend+j)->max = ntohl(*((int *)( DataDaq.tb->data + pos + sizeof(int)*secrate + j*sizeof(int) )));
		   (trend+j)->mean = ntohd(*((double *)( DataDaq.tb->data + pos + 2*sizeof(int)*secrate + j*2*sizeof(int) )));
	        }
                break;
          }
       }
       else { /* selected channels */
          index = SearchChList(chName, chanList, NchanList);
	  if ( index < 0 ) 
             return -1;
	  pos = 0;
	  if ( index > 0 ) {
	     for ( j=0; j<index; j++ ) {
                pos += 2*getBlockLength(chanList[j].data_type, 1) + sizeof(double);
             }
	  }
	  pos = pos*secrate;
	  /*fprintf ( stderr,"index=%d pos=%d\n", index,pos );*/
          switch ( chanList[index].data_type ) {
            case 4: /* 32 bit-float */
	    case 6: /* complex */
	        for ( j=0; j<secrate; j++ ) {
		   *((unsigned int *)&min) =
			ntohl(*((unsigned int *)( DataDaq.tb->data + pos +j*sizeof(float) )));
		   (trend+j)->min = min;
		   *((unsigned int *)&max) =
			ntohl(*((unsigned int *)( DataDaq.tb->data + pos + sizeof(float)*secrate + j*sizeof(float) )));
		   (trend+j)->max = max;
		   mean = *((double *)( DataDaq.tb->data + pos + 2*sizeof(float)*secrate + j*2*sizeof(float) ));
#if defined __linux__ || defined __APPLE__
		   mean1 = ntohd(mean);
#else
		   mean1 = mean;
#endif
		   (trend+j)->mean = mean1;
	        }
                break;
            case 5: /* 64 bit-double */
	        for ( j=0; j<secrate; j++ ) {
		   (trend+j)->min = *((double *)( DataDaq.tb->data + pos +j*sizeof(double) ));
		   (trend+j)->max = *((double *)( DataDaq.tb->data + pos + sizeof(double)*secrate + j*sizeof(double) ));
		   (trend+j)->mean = *((double *)( DataDaq.tb->data + pos + 2*sizeof(double)*secrate + j*2*sizeof(double) ));
		   (trend+j)->mean = 0;
	        }
                break;
            default: /* 32 bit-integer */
	        for ( j=0; j<secrate; j++ ) {
		   int min1 = *((int *)( DataDaq.tb->data + pos +j*sizeof(int) ));
		   min1 = ntohl(min1);
		   (trend+j)->min = min1;
		   {
		   int max1 = *((int *)( DataDaq.tb->data + pos + sizeof(int)*secrate + j*sizeof(int) ));
		   max1 = ntohl(max1);
		   (trend+j)->max = max1;
		   }
		   mean = *((double *)( DataDaq.tb->data + pos + 2*sizeof(int)*secrate + j*2*sizeof(int) ));
#if defined __linux__ || defined __APPLE__
		   ((unsigned long *)&mean1)[1] = ntohl(((unsigned long *)&mean)[0]);
		   ((unsigned long *)&mean1)[0] = ntohl(((unsigned long *)&mean)[1]);
#else
		   mean1 = mean;
#endif
		   (trend+j)->mean = mean1;
	        }
                break;
          }
       }
       return seconds;
}


int DataTrendLength()
{
       return DataDaq.tb->secs;
}


int DataGetChSlope(const char* chName, float *slope, float *offset, int *status)
{
int ssize, index, j;
       ssize = DataDaq.s_size;
       if ( all < 0 || all == 1) {
	  fprintf ( stderr,"datasrv: DataGetCh failed: %s is not in the channel list\n", chName );
          return -1;
       }
       else {
          index = SearchChList(chName, chanList, NchanList);
	  if ( index < 0 ) 
             return -1;
	  *slope = DataDaq.s[index].signal_slope;
	  *offset = DataDaq.s[index].signal_offset;
	  *status = DataDaq.s[index].signal_status;
       }
       return ssize;
}


/* returns total number of the configured channels */
int DataChanList(struct DChList allChan[])
{
int j;

       for ( j=0; j<NchannelAll; j++ ) {
          allChan[j].rate = channelAll[j].rate;
	  strcpy(allChan[j].name, channelAll[j].name);
	  strcpy(allChan[j].units, channelAll[j].s.signal_units);
          allChan[j].group_num = channelAll[j].group_num;
          allChan[j].data_type = channelAll[j].data_type;
          allChan[j].tpnum = channelAll[j].tpnum;
       }
       return NchannelAll;
} 


int DataGroup(struct DChGroup allGroup[])
{
  int j, gnum, tempnum[MAX_CHANNEL_GROUPS+1];

       if ( j = daq_recv_channel_groups(&DataDaq, groups, MAX_CHANNEL_GROUPS, &totalgroup) ) {
	  fprintf (stderr, "datasrv: DataGroup failed: daq_recv_channel_groups errno=%d\n", j);
	  return -1;
       }
       for ( j = 0; j < totalgroup; j++ ) {
	  strcpy(allGroup[j].name, groups[j].name);
          allGroup[j].group_num = groups[j].group_num;
          tempnum[j] = 0;
       }
       tempnum[totalgroup] = 0;
       for ( j=0; j<NchannelAll; j++ ) {
	  gnum = channelAll[j].group_num;
	  tempnum[gnum] ++;
       }
       for ( j = 0; j < totalgroup; j++ ) {
	  gnum = allGroup[j].group_num;
	  allGroup[j].total_chan = tempnum[gnum];
       }
       return totalgroup;
}


/* returns the samplerate of the channel or -1 if fails */
int DataChanRate(const char* chName)
{
int index;

       index = SearchChList(chName,channelAll,NchannelAll);
       if ( index < 0 ) {
	  fprintf ( stderr,"datasrv: DataChanRate: cann't find channel %s\n", chName );
	  return -1;
       }
       return channelAll[index].rate;
} 


int DataChanType(const char* chName)
{
int index;

       index = SearchChList(chName,channelAll,NchannelAll);
       if ( index < 0 ) {
	  fprintf ( stderr,"datasrv: DataChanType: cann't find channel %s\n", chName );
	  return 0;
       }
#if 0
       if ( channelAll[index].trend = 0 ) {
	  fprintf ( stderr,"datasrv: DataChanType: Trend data not available for channel %s\n", chName );
	  return -1;
       }
#endif
       return channelAll[index].data_type;
}


time_t DataTimeGps()
{
       return DataDaq.tb->gps;
}


time_t DataTimensec()
{
       return DataDaq.tb->gpsn;
}


void DataTimestamp(char* timestamp)
{
char   temp[100];

       gpsTime.sec = (unsigned long)DataDaq.tb->gps;

       /* If we know the gpsTime, we should know the leap seconds. */
       if (gpsTime.sec < 599184012) /* 1998-12-31 23:59:60 */
	  gpsTime.leap = 12;
       else if (gpsTime.sec < 820108813) /* 2005-12-31 23:59:60 */
	  gpsTime.leap = 13;
       else if (gpsTime.sec < 914803214) /* 2008-12-31 23:59:60 */
	  gpsTime.leap = 14;
       else if (gpsTime.sec < 1025136015) /* 2012-06-60 23:59:60 */
	  gpsTime.leap = 15;
       else
	  gpsTime.leap = 16;
       gps_to_utc(&gpsTime,&ctout);
       if ( ctout.date.year-2000 < 0 )
	 sprintf ( timestamp, "%ld-%02d-%02d-%02d-%02d-%02d", ctout.date.year-1900, ctout.date.month, ctout.date.day, ctout.hour, ctout.minute, ctout.second);
       else if ( ctout.date.year-2000 < 10 )
	 sprintf ( timestamp, "0%ld-%02d-%02d-%02d-%02d-%02d", ctout.date.year-2000, ctout.date.month, ctout.date.day, ctout.hour, ctout.minute, ctout.second);
       else
	 sprintf ( timestamp, "%ld-%02d-%02d-%02d-%02d-%02d", ctout.date.year-2000, ctout.date.month, ctout.date.day, ctout.hour, ctout.minute, ctout.second);
       /*fprintf ( stderr, "datasrv: DataTimestamp: %s (%d)\n", currentTime, DataDaq.tb->gps  );*/
       return;
}


unsigned long DataSequenceNum()
{
       return DataDaq.tb->seq_num;
}


/*** Utility Functions ***/

/* transfer GPS time to UTC time  */
void DataGPStoUTC(long gpsin, char* utcout)
{
char   temp[100];

       gpsTime.sec = (unsigned long)gpsin;
       /* If we know the gpsTime, we should know the leap seconds. */
       if (gpsTime.sec < 599184012) /* 1998-12-31 23:59:60 */
	  gpsTime.leap = 12;
       else if (gpsTime.sec < 820108813) /* 2005-12-31 23:59:60 */
	  gpsTime.leap = 13;
       else if (gpsTime.sec < 914803214) /* 2008-12-31 23:59:60 */
	  gpsTime.leap = 14;
       else if (gpsTime.sec < 1025136015) /* 2012-06-60 23:59:60 */
	  gpsTime.leap = 15;
       else
	  gpsTime.leap = 16;
       gps_to_utc(&gpsTime,&ctout);
       if ( ctout.date.year-2000 < 0 )
	 sprintf ( utcout, "%ld-%d-%d-%d-%d-%d", ctout.date.year-1900, ctout.date.month, ctout.date.day, ctout.hour, ctout.minute, ctout.second);
       else if ( ctout.date.year-2000 < 10 )
	 sprintf ( utcout, "0%ld-%d-%d-%d-%d-%d", ctout.date.year-2000, ctout.date.month, ctout.date.day, ctout.hour, ctout.minute, ctout.second);
       else
	 sprintf ( utcout, "%ld-%d-%d-%d-%d-%d", ctout.date.year-2000, ctout.date.month, ctout.date.day, ctout.hour, ctout.minute, ctout.second);
       return;
}


/* transfer UTC time to GPS time  */
long  DataUTCtoGPS(char* utctime)
{
long   mjd;
int    yr,mo,da,hr,mn,sc;
     
	 if (debug != 0) fprintf(stderr, "DataUTCtoGPS() - utctime = %s\n", utctime) ; /* JCB */
	 sscanf (utctime, "%d-%d-%d-%d-%d-%d", &yr,&mo,&da,&hr,&mn,&sc );
	 /*----- enter time in UTC ------------*/
	 if ( yr == 98 || yr == 99 )
	   ctin.date.year  = yr + 1900;
	 else
	   ctin.date.year  = yr + 2000;
	 ctin.date.month = mo;       /* [1,12] */
	 ctin.date.day   = da;       /* [1,31] */
	 ctin.hour       =hr;        /* [0,23] */
	 ctin.minute     =mn;        /* [0,59] */
	 ctin.second     =sc;        /* [0,60] */
	 ctin.offset     =0;         /* offset in minutes from UTC [-5999,5999] */
	 if (debug != 0) 
	 {
	    fprintf(stderr, " ctin.date.month = %d\n", ctin.date.month) ; /* JCB */
	     fprintf(stderr, " ctin.date.day = %d\n", ctin.date.day) ; /* JCB */
	     fprintf(stderr, " ctin.hour = %d\n", ctin.hour) ; /* JCB */
	     fprintf(stderr, " ctin.minute = %d\n", ctin.minute) ; /* JCB */
	     fprintf(stderr, " ctin.second = %d\n", ctin.second) ; /* JCB */
	     fprintf(stderr, " ctin.offset = %ld\n", ctin.offset) ; /* JCB */
	 }
	 mjd=caldate_mjd(&ctin.date); /* modified Julian day */
	 fprintf(stderr, " mjd = %ld\n", mjd) ;
	 utc_to_gps(&ctin,&gps);
	 return gps.sec;
}

/* transfer UTC time to GPS time  */
long  DataUTCtoGPS1(int yr, int mo, int da, int hr, int mn, int sc)
{
long   mjd;
	 if ( yr == 98 || yr == 99 )
	   ctin.date.year  = yr + 1900;
	 else
	   ctin.date.year  = yr + 2000;
	 ctin.date.month = mo;       /* [1,12] */
	 ctin.date.day   = da;       /* [1,31] */
	 ctin.hour       =hr;        /* [0,23] */
	 ctin.minute     =mn;        /* [0,59] */
	 ctin.second     =sc;        /* [0,60] */
	 ctin.offset     =0;         /* offset in minutes from UTC [-5999,5999] */
	 mjd=caldate_mjd(&ctin.date); /* modified Julian day */
	 utc_to_gps(&ctin,&gps);
	 return gps.sec;
}



/*** the followings are not available to public ***/

/* returns the index of the channel in chanList  */
int SearchChList(const char* chName, daq_channel_t chList[], int listLength)
{
int   j;

       for ( j=0; j<listLength; j++ ) {
	 if ( strcmp(chName, chList[j].name) == 0 ) {
             return j;
	 }
       }
       return -1;
} 


/* translate data type to the length of the data block 
   trend=1 while request trend data. note that when type=1, trend data 
   being transfered by int, but general data by short */
int getBlockLength(int datatype, int trend)
{
       switch ( datatype ) {
         case 6: 
             return 2*sizeof(float);
             break;
         case 4: 
             return sizeof(float);
             break;
         case 5: 
             return sizeof(double);
             break;
         case 2: 
             return sizeof(int);
             break;
         default: 
	     if ( trend == 1 )
	        return sizeof(int);
	     else
	        return sizeof(short);
             break;
       }
       
}

