/* dv_status.c                                                           */
/* Channel status checker                                                   */
/* compile with:
    gcc -o dv_status dv_status.c datasrv.o daqc_access.o -L/home/hding/Try/Lib/UTC_GPS -ltai -lm -lnsl -lsocket -lpthread

./dv_status <Server IP> <Server Port> <output file> <second> <channles> (optional)
*/

#include "datasrv.h"

char  *optarg;
int   c, optind, opterr;

int   CHANNUM=20, TIMELIMIT=30;
char  outputFile[240], excludeFile[240];
int   exfile=0, nex=0;
char  excludeSig[MAX_CHANNELS+1][MAX_CHANNEL_NAME_LENGTH+80];
FILE  *fde;

int   totalchan, windowNum;
char  chName[20][80], chUnit[20][80];
int   chstatus[20];
float slope[20], offset[20];

unsigned long processID=0;
short   finished;
int     timecount, badchan;


void*   read_data();
int     excludech (char chanN[]);
int     test_substring_beg (char sub[], char s[]);
int     test_substring_end (char sub[], char s[]);


int main(int argc, char *argv[])
{
static char optstring[] = "s:p:o:e:n:t:m:h";
struct DChList allChan[MAX_CHANNELS];
int     serverPort = 0;
char    serverIP[80];

int     i, j, skipped, ip=0, ofile=0, maxch=0; 

FILE    *fd, *fde;

        for ( i=1; i<argc; i++ ){
           if ( (strcmp(argv[i], "h" ) == 0) || (strcmp(argv[i], "-h" ) == 0)){
              printf("Usage: dv_status \n        [-s server IP address (required)] \n        [-p server port] \n        [-o output file (required] \n        [-e exclude file] \n        [-n no. of chans per checking] \n        [-t running seconds] \n");        
              exit(0);
	   }
	}

        while (  (c = getopt(argc, argv, optstring)) != -1 ) {
           switch ( c ) {
             case 's': 
                 strcpy(serverIP, optarg);
		 ip = 1;
                 break;
             case 'p': 
                 serverPort = atoi(optarg);
                 break;
             case 'o': 
                 strcpy(outputFile, optarg);
		 ofile = 1;
                 break;
             case 'e': 
                 strcpy(excludeFile, optarg);
		 exfile = 1;
                 break;
             case 'n': 
	         CHANNUM = atoi(optarg);
		 if ( CHANNUM > 20 ) {
		   CHANNUM = 20;
		   fprintf ( stderr, "Warning: the max number of channels per checking is 20\n"); 
		 }
                 break;
             case 't': 
	         TIMELIMIT = atoi(optarg);
                 break;
             case 'm': /* testing only */
                 maxch = atoi(optarg);
                 break;
             case '?':
                 printf ( "Invalid option found.\n" );
           }           
        }
	if ( !ip ) {
	  fprintf ( stderr, "Error: No server is specified. Exit.\n" );
	  printf("Usage: dv_status \n        [-s server IP address (required)] \n        [-p server port] \n        [-o output file (required)] \n        [-e exclude file] \n        [-n no. of chans per checking] \n        [-t running seconds] \n");        
	  exit(0);
	}
	if ( !ofile ) {
	  fprintf ( stderr, "Error: No output file is specified. Exit.\n" );
	  exit(0);
	}

	fprintf ( stderr, "Starting dv_status:\n" ); 
	fprintf ( stderr, "    Server: %s-%d\n", serverIP, serverPort); 

	fd = fopen( outputFile, "w" );
	if ( fd == NULL ) {
	  fprintf ( stderr, "Error: Can't open writing file %s. Exit.\n", outputFile );
	  return -1;
	}
	fprintf ( stderr, "    Output File: %s\n", outputFile); 
	if ( exfile ) {
	  fde = fopen( excludeFile, "r" );
	  if ( fde == NULL ) {
	    fprintf ( stderr, "Error: Can't open reading file %s. Exit.\n",excludeFile );
	    fclose(fd);
	    return -1;
	  }
	  fprintf ( stderr, "    Exclude File: %s\n", excludeFile);
	}
	else
	  fprintf ( stderr, "    No exclude file.\n");
	fprintf ( stderr, "    Time period: %d seconds\n", TIMELIMIT); 
	fprintf ( stderr, "    No. channels per checking: %d\n\n", CHANNUM); 
	

	/*
printf ( "excluded: %d\n", nex );
fclose(fd);
return 0;
	*/
	/* Connect to Data Server */
	fprintf ( stderr,"Connecting to Server %s-%d\n", serverIP, serverPort );
	if ( DataConnect(serverIP, serverPort, 7777, read_data) != 0 ) {
	  fprintf ( stderr, "Error: can't connect to the data server %s-%d. Make sure the server is running. Exit.\n", serverIP, serverPort );
	  fclose(fd);
	  if ( exfile ) 
	    fclose(fde);
	  exit(1);
	}

        totalchan = DataChanList(allChan);
	if ( maxch > 0 ) /* hidden for testing purpose */
	  totalchan = maxch;
	if ( exfile ) { /* load names of all exclused chans */
	  nex = 0;
	  while ( fscanf ( fde, "%s", excludeSig[nex] ) != EOF) {
              nex++;
          }
	  fclose(fde);
	}

	badchan = 0;
	skipped = 0;

	for ( i=0; i<totalchan; i+=CHANNUM ) {
	  if ( i+CHANNUM <= totalchan )
	    printf ( "\nLoadinging %d - %d of %d channels\n", i+1, i+CHANNUM, totalchan);
	  else
	    printf ( "\nLoadinging %d - %d of %d channels\n", i+1, totalchan, totalchan);
	  DataChanDelete("all");
	  windowNum = 0;
	  finished = 0;
	  timecount = 0;
	  for ( j=0; j<CHANNUM; j++ ) {
	    if ( i+j < totalchan ) {
	      /* check if in excludeFile */
	      if ( exfile ) {
		if ( !excludech (allChan[i+j].name) ) {
		  strcpy(chName[windowNum], allChan[i+j].name);
		  windowNum++;
		}
	      }
	      else {
		strcpy(chName[windowNum], allChan[i+j].name);
		windowNum++;
	      }
	    }
	  }
	  if ( windowNum > 0 ) {
	    for ( j=0; j<windowNum; j++ ) {
	      DataChanAdd(chName[j], 0);
	      chstatus[j] = 0;
	    }
	    processID = DataWriteRealtime();
	    if ( processID < 0 ) {
	      fprintf ( stderr,"Error: Can't start DataWriteRealtime. Skipping these channels\n" );
	      skipped = skipped + windowNum; 
	      for ( j=0; j<windowNum; j++ ) { /* recording skipped */
		fprintf ( fd, "%s skipped\n", chName[j] );
	      }
	    }
	    else {
	      fprintf ( stderr, "Checking status " );
	      while ( !finished ) ;
	      sleep(1);	    
	      for ( j=0; j<windowNum; j++ ) { /* recording status */
		if ( chstatus[j] > 0 ) {
		  fprintf ( fd, "%s %d\n", chName[j], chstatus[j] );
		  badchan++;
		}
	      }
	    }
	  }
	  

	}

	sleep(2);
        fclose(fd);
	fprintf ( stderr, "Finished checking. Total channels in the config: %d\n", totalchan );
	fprintf ( stderr, "%d bad. %d skipped.\n", badchan, skipped);
	DataQuit();

	return 0;
}


void* read_data()
{
int     bytercv;
float   slope[CHANNUM], offset[CHANNUM];
int     j, status[CHANNUM];

	DataReadStart();

	while ( 1 ) {
	   if ( (bytercv = DataRead()) == -2 ) {
	     /*fprintf ( stderr, "Reset slopes and offsets.\n" )*/;
	   }
	   else if ( bytercv < 0 ) {
	     DataReadStop();
	     break;
	   }
	   else if ( bytercv == 0 ) {
	     break;
	   }
	   else { /* receiving date */
	     fprintf ( stderr, "." );
	     for ( j=0; j<windowNum; j++ ) {
	       DataGetChSlope(chName[j], &slope[j], &offset[j], &status[j]);
	       if (status[j])
		 chstatus[j]++;
	     }
	     timecount++;
	     if (timecount >= TIMELIMIT) {
	       fprintf ( stderr, "\n" );
	       DataWriteStop(processID);
	       finished = 1;
	     }
	   }

        }  /* end of while loop  */


	fflush(stdout);
	return NULL;
}


int  excludech (char chanN[])
{
char tempSig[MAX_CHANNEL_NAME_LENGTH+80];
int  i, j, len;

    for ( j=0; j<nex; j++ ) {
      len = strlen(excludeSig[j]);
      if ( excludeSig[j][0]== '*' && excludeSig[j][len -1]== '*' ) {
	for ( i=0; i<len-2; i++ ) {
	  tempSig[i] = excludeSig[j][i+1];
	}
	tempSig[len-2] = '\0';
	if ( strstr(chanN, tempSig ) != NULL )
	  return 1;
      }
      else if ( excludeSig[j][0]== '*' ) {
	if ( test_substring_end(excludeSig[j], chanN) ==0)
	  return 1;
      }
      else if ( excludeSig[j][len -1]== '*' ) {
	if ( test_substring_beg(excludeSig[j], chanN) ==0)
	  return 1;
      }
      else {
	if (strcmp(chanN, excludeSig[j])==0)
	  return 1;
      }
    }
    return 0;
}

/* returns 0 if sub is a substring of s starting from the beginning */
int test_substring_beg (char sub[], char s[])
{
int len, j;

  len = strlen(sub);
  if (len > strlen(s))  return 1;
  for ( j=0; j<len-1; j++ ) {
    if (sub[j] != s[j]) return 1;
  }
  return 0;
}

/* returns 0 if sub is a substring of s at the end */
int test_substring_end (char sub[], char s[])
{
int len, len1, j;

  len = strlen(sub);
  len1 = strlen(s);
  if (len > len1)  return 1;
  for ( j=0; j<len-1; j++ ) {
    if (sub[len-1-j] != s[len1-1-j]) return 1;
  }
  return 0;
}


