/*
 * DataGet.C	.MEX file 
 */

#include "mex.h"


void mexFunction(
                 int nlhs,       mxArray *plhs[],
                 int nrhs, const mxArray *prhs[]
		 )
{
char  severIP[80], *arg[100];
int processID = 0;
int j;

  /* Check for proper number of arguments */
  
  if (nrhs < 2) {
    mexErrMsgTxt("Input arguments missing");
  }

  if ( !mxIsChar(prhs[0]) ) {
    mexPrintf("The 1st argument should be Server IP");
    mexErrMsgTxt(" ");
  }
  mxGetString (prhs[0], severIP, 80);

  if ( !mxIsNumeric((prhs[1])) ) {
    mexPrintf("The 2nd argument should be an integer (process ID)");
    mexErrMsgTxt(" ");
  }
  processID = (int)mxGetScalar(prhs[1]);

  for ( j=0; j<=2; j++ ) {
    arg[j] = (char *) malloc(80*sizeof(char));
    if ( arg[j] == NULL )
      mexErrMsgTxt("not enough space for strings");
  }
  strcpy(arg[0],"StopDataGet");
  strcpy(arg[1],severIP);
  sprintf ( arg[2], "%d", processID );
  arg[3] = (char*)NULL;
   if ( !fork() ) {
    if ( !fork() ) {
      execvp("StopDataGet", arg );
      perror("exec StopDataGet error");
      exit(0); 
    }
    else
      exit(0);
  }
  else
    wait(0);
  
  return;
}


