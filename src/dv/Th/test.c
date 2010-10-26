/* test.c  */
/* Compile with gcc -o test test.c datasrv.o daqc_access.o 
   -L/home/hding/Cds/Frame/Lib/UTC_GPS -ltai -lsocket -lpthread */
#include "datasrv.h"

/*#define DAQD_HOST  "127.0.0.1"*/         /* localhost  */
/*#define DAQD_HOST  "131.215.114.15"*/    /* daqd machine cdssol5 */
/*#define DAQD_HOST  "131.215.125.77"*/    /* daqd machine topcat */
#define  BUFSIZE      2000000
#define  DATA         0 
#define  TREND        1 
#define  TREND60      60 
#define  SOUND        2 

char *optarg;
int  optind, opterr;
char timestring[24], timestring0[24], name[50];
int  srate, PLAY;

double data[16500];
struct DTrend trenddata[200];

/* Data-receiving thread prototype */
void* read_data();

int main(int argc, char *argv[])
{
static char optstring[] = "l:s:w:";
unsigned long processID = 0;
int  LISTENER_PORT, DAQD_PORT_NUM; 
 char DAQD_HOST[80], temp[24], tempst[32], ch[2];
int  i, j, c;
time_t tgps;

struct DChList allChan[MAX_CHANNELS];
struct DChGroup allGroup[MAX_CHANNEL_GROUPS];

       opterr = 0;
       LISTENER_PORT = 7777;
       DAQD_PORT_NUM = 8088;
       strcpy(DAQD_HOST, "131.215.114.15");
       while (  (c = getopt(argc, argv, optstring)) != -1 ) {
           switch ( c ) {
             case 'l': 
	         LISTENER_PORT = atoi(optarg);
                 break;
             case 's': 
	         strcpy(DAQD_HOST, optarg);
                 break;
             case 'w': 
	         DAQD_PORT_NUM = atoi(optarg);
                 break;
             case '?':
	         printf ( "Invalid option found.\n" );
           }
       }
       if ( DataConnect(DAQD_HOST, DAQD_PORT_NUM, LISTENER_PORT, read_data) != 0 ) {
	  fprintf ( stderr,"connection failed\n" );
	  exit(1);
       } 

       i = DataChanList(allChan);
       if (i>500) i=500;
       for ( j=0; j<i; j++ ) 
          printf ( "test chName= %s rate=%d units=%s type=%d group=%d\n", allChan[j].name, allChan[j].rate, allChan[j].units, allChan[j].data_type, allChan[j].group_num );
       i= DataGroup(allGroup);
       for ( j=0; j<i; j++ ) 
          printf ( "test Group%d %s: total chan %d\n", allGroup[j].group_num, allGroup[j].name, allGroup[j].total_chan );

       while ( 1 ) {
          printf("TYPE YOUR CHOICE: ");
          scanf("%s",ch);
          switch ( ch[0] ) {
            case 'a': 
                printf("ADD CHANNEL: ");
                scanf("%s",name);
                printf("CHANNEL Rate: ");
                scanf("%s",temp);
		i = atoi(temp);
		j = DataChanRate(name);
		printf ( "Actual rate = %d\n", j );
		j = DataChanAdd(name, i);
                break;
            case 'd': 
                printf("DELETE CHANNEL: ");
                scanf("%s",name);
                DataChanDelete(name);
                break;
            case 'f': /* real time fast */
		PLAY = DATA;
	        processID = DataWriteRealtimeFast();
                break;
            case 'F': /* given time */
		PLAY = DATA;
	        processID = DataWrite(timestring, 3, 0);
               break;
            case 'r': /* real time */
		PLAY = DATA;
	        processID = DataWriteRealtime();
                break;
            case 'R': /* given time */
		PLAY = DATA;
	        processID = DataWrite(timestring, 3, 0);
	        processID = DataWrite("99-02-21-18-24-11", 3, 0);
                break;
            case 's': /* sound */
	        /*
                printf("SOUND Rate: ");
                scanf("%s",temp);
		srate = atoi(temp);
		PLAY = SOUND;
	        processID = DataWriteRealtime();
		*/
                break;
            case 't': /* trend */
		PLAY = TREND;
	        processID = DataWriteTrendRealtime();
                break;
            case 'T': /* trend */
		PLAY = TREND;
	        /*processID = DataWriteTrend("0", 20, 1);*/
	        /*processID = DataWriteTrend(timestring, 3, 1);*/
	        processID = DataWriteTrend("03-10-7-21-1-44", 20, 1, 0);
                break;
            case '6': /* trend60 */
		PLAY = TREND60;
	        processID = DataWriteTrend("03-10-7-21-1-44", 120, 60, 0);
                break;
            case 'k': /* stop  */
                DataWriteStop(processID);
		fprintf ( stderr,"Stopped\n");
                break;
            case 'c': /* test */
	        i= DataTimeNow(DAQD_HOST, DAQD_PORT_NUM);
	        DataTimestamp(tempst);
	        tgps = DataTimeGps();
		fprintf ( stderr,"Done equesting time: %s - %d:%d\n", tempst, tgps, i);
                break;
            case 'q': /* quit  */
                DataQuit();
		fprintf ( stderr,"Quit\n");
                break;
            case 'e': /* exit  */
		fprintf ( stderr,"Exit\n");
		exit(0);
                break;
            default: 
                printf("unknown input\n");
                break;
          }
       }
       return 0;
}


/* Thread receives data from DAQD */
void* read_data()
{
  /*short data[16500];
    struct DTrend trenddata;*/
int rate, j, byterv, secn, sta, firsttime=1;
time_t ngps,  ngps0;
long   total;
unsigned long seqnum;
float try[16], chslope, choffset;

       DataReadStart();
       while ( 1 ) {
	  if ( (byterv = DataRead()) == -2 ) {
	     j=DataGetChSlope(name, &chslope, &choffset, &sta);
	     printf ( "slope changed %d: slope=%f, offset=%f, status=%d\n", j, chslope, choffset, sta  );
	  }
	  else if ( byterv < 0 ) {
	     printf ( "transmission error\n" );
             DataReadStop();
	     break;
	  }
	  else if ( byterv == 0 ) {
	     printf ( "trailer received\n" );
	     break;
	     /*printf ( "cann't read file %s\n", timestring );*/
	  }
	  else {

	  /* Process received data here */
	  DataTimestamp(timestring);
	  ngps = DataTimeGps();
	  if (firsttime)  { ngps0 = ngps; firsttime=0;}
	  seqnum = DataSequenceNum();
	  total = ngps-ngps0;
          printf ( "GPS %d - %d = %d\n", ngps, ngps0, total );
          printf ( "timestring=%s, seq=%d\n", timestring, seqnum );

          switch ( PLAY ) {
            case DATA: 
		rate = DataGetCh(name, data, 0);
		j = DataChanType(name);
		fprintf (stderr, "%s rate=%d type=%d\n", name, rate, j);
		if ( j == 5 ) { /* 64 bit-double */
		   for ( j=0; j<16; j++ ) { 
		      printf ( "data[%d]=%le ", j, data[j] );
		   }
                }
                else {
		   for ( j=0; j<16; j++ ) {
		      printf ( "data[%d]=%le ", j, data[j] );
		   }
                }
		printf ( "\n\n" );
                break;
            case TREND: 
	  case TREND60: 
	        secn = DataTrendGetCh(name, trenddata );
		rate = secn/PLAY;
		j = DataChanType(name);
		printf ( "trend %s type=%d, %d secs %d data\n", name,j,secn,rate );
		if ( j == 5 ) {
		   for ( j=0; j<rate; j++ ) 
		      printf ( "%dth data: min=%e, max=%e, mean=%e\n", j+1, trenddata[j].min, trenddata[j].max, trenddata[j].mean );
                }
                else {
		   for ( j=0; j<rate; j++ ) 
		      printf ( "%dth data: min=%f, max=%f, mean=%f\n", j+1, trenddata[j].min, trenddata[j].max, trenddata[j].mean );
                }
		printf ( "\n\n" );
                break;
            case SOUND: 
                break;
	    default: 
                break;
          }
       }
       }
       
       fflush (stdout);
       return NULL;
}
