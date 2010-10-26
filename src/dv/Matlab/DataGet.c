/*
 * DataGet.C	.MEX file 
 */

#include <math.h>
#include "mex.h"


#define CHANNUM     16

int   fixchname(char* name);
static void helpmessage()
{
  mexPrintf ( "Arguement list for DataGet:\n" );
  mexPrintf ( "cell array of char*: channel name list\n" );
  mexPrintf ( "char*: starting time yy-mm-dd-hh-mn-ss\n" );
  mexPrintf ( "int: duration\n" );
  mexPrintf ( "int: resolution\n" );
  mexPrintf ( "int: filter flag\n" );
  mexPrintf ( "char*: server IP\n" );
  mexPrintf ( "int: server Port\n" );
  mexPrintf ( "char*: name of output file (no extension)\n" );
  mexPrintf ( "char: file mode (optional)\n" );
  mexPrintf ( "See README for more details\n" );

  return;
}  

void mexFunction(
                 int nlhs,       mxArray *plhs[],
                 int nrhs, const mxArray *prhs[]
		 )
{
int   j, i, m, Nchan; 
int   DAQD_PORT_NUM, duration, resolution, filFlag=0, filemode;
char  chName[CHANNUM][80], timestart[24], DAQD_HOST[80], filename[100];
char  *arg[100], tempst[100];
mxArray *cellpr;
  /* Check for proper number of arguments */
  
  if (nrhs < 8) {
    helpmessage();
    mexErrMsgTxt("Input arguments missing");
  }

  if ( !mxIsCell(prhs[0]) ) {
    mexPrintf("The 2nd argument should be a cell array contains channel names.");
    mexPrintf("format: {'ch1';'ch2';...,'chN'}");
    mexErrMsgTxt(" ");
  }

  Nchan = mxGetM(prhs[0]);
  if ( Nchan == 0 ) 
    mexErrMsgTxt("No input channle.\n");
  if ( Nchan > CHANNUM ) {
    mexPrintf("Number of input channels must be <= %d", CHANNUM);
    mexErrMsgTxt("\n");
  }
  for ( j=0; j<Nchan; j++ ) {
    cellpr = mxGetCell(prhs[0], j);
    if (!mxIsChar(cellpr)) 
      mexErrMsgTxt("error in channel name list");
    if ( mxGetString (cellpr, chName[j], 80))
      mexErrMsgTxt("can't convert name list");
  }

  if ( !mxIsChar(prhs[1]) ) {
    mexPrintf("The 2nd argument should be a time string yy-mm-dd-hh-mn-ss");
    helpmessage();
    mexErrMsgTxt(" ");
  }
  mxGetString (prhs[1], timestart, 24);

  if ( !mxIsNumeric((prhs[2])) ) {
    mexPrintf("The 3rd argument should be an integer (duration)");
    helpmessage();
    mexErrMsgTxt(" ");
  }
  duration = (int)mxGetScalar(prhs[2]);

  if ( !mxIsNumeric((prhs[3])) ) {
    mexPrintf("The 4th argument should be 0 or an integer of power of 2");
    helpmessage();
    mexErrMsgTxt(" ");
  }

  resolution = (int)mxGetScalar(prhs[3]);
  m = resolution;
  while ( m > 1 ) {
    if ( m % 2 == 0 ) {
       m=m/2;
    }
    else {
      mexErrMsgTxt("The 4th must be 0 or an integer of power of 2");
    }
  }

  if ( !mxIsNumeric((prhs[4])) ) {
    mexPrintf("The 5th argument should be an integer between 0 and 4");
    helpmessage();
    mexErrMsgTxt(" ");
  }
  filFlag = (int)mxGetScalar(prhs[4]);
  if ( filFlag > 4 || filFlag < 0 )
    mexErrMsgTxt("filter flag: should be an integer between (include) 0 and 4");

  if ( !mxIsChar(prhs[5]) ) {
    mexPrintf("The 6th argument should be the server IP");
    helpmessage();
    mexErrMsgTxt(" ");
  }
  mxGetString (prhs[5], DAQD_HOST, 80);

  if ( !mxIsNumeric((prhs[6])) ) {
    mexPrintf("The 7th argument should be an integer (server port. use 0 as default)");
    helpmessage();
    mexErrMsgTxt(" ");
  }
  DAQD_PORT_NUM = (int)mxGetScalar(prhs[6]);

  if ( nrhs>8 && mxIsChar(prhs[8]) ) { 
    mxGetString (prhs[8], tempst, 4);
    if ( strcmp(tempst, "b") == 0 )
      filemode = 1;
    else
      filemode = 0;
  }
  else
    filemode = 0;

  if ( mxIsChar(prhs[7]) ) { 
    mxGetString (prhs[7], filename, 100);
    i = fixchname(filename);
    if ( i > 0 ) {
      strncpy(tempst, filename, i );
      tempst[i] = '\0';
      strcpy(filename, tempst);
    }
    if ( filemode == 0 )
      strcat(filename, ".m");
    else
      strcat(filename, ".dat");
  }
  else
    mexErrMsgTxt("The 8th argument should be a file name");


  printf ( "Server IP=%s\n", DAQD_HOST );
  printf ( "Server Port=%d\n", DAQD_PORT_NUM );

  for ( j=0; j<nrhs+Nchan+1; j++ ) {
    arg[j] = (char *) malloc(100*sizeof(char));
    if ( arg[j] == NULL )
      mexErrMsgTxt("not enough space for strings");
  }
  strcpy(arg[0],"frameMat");
  sprintf ( arg[1], "%d", Nchan );
  printf ( "Total %d channels requested:\n", Nchan );
  for ( j=0; j<Nchan; j++ ) {
    strcpy(arg[j+2],chName[j]);
    printf ( "%s\n", chName[j] );
  }
  for ( j=1; j<nrhs; j++ ) {
    if ( mxIsNumeric((prhs[j])) ) 
      sprintf ( arg[Nchan+j+1], "%d", (int)mxGetScalar(prhs[j]) );
    else
      mxGetString (prhs[j], arg[Nchan+j+1], 80);
  }
  strcpy(arg[Nchan+8], filename);
  if ( nrhs >= 9 )
    sprintf ( arg[Nchan+9],"%d", filemode );
  /*      
for ( j=1; j<nrhs+Nchan+1; j++ ) {
  printf ( "arg%d=%s\n",j,arg[j] );
}
  printf ( "last=%d\n",nrhs+Nchan+1 );
return; 
  */     
  if ( !fork() ) {
    if ( !fork() ) {
      arg[nrhs+Nchan+1] = (char*)NULL;
      execvp("frameMat", arg );
      perror("exec frameMat error:");
      exit(0); 
    }
    else {
      exit(0);
    }
  }
  else {
    wait(0);
  }
  
  return;
}

/* grab the position of "." in the line if exists */
int fixchname(char* name)
{
int len, j;
  len = strlen(name);
  for ( j=1; j<len; j++ ) {
    if ( name[j] == '.' ) 
      return j;
  }
  return 0;  
}

