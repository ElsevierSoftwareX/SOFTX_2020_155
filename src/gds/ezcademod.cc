static const char *versionId = "Version $Id$" ;
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: ezcademod						*/
/*                                                         		*/
/* Module Description: ezca demodulation and servoing			*/
/*									*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 20Jan04  S. Ballmer   	First release                           */
/* 0.2	 05Mar04  S. Ballmer   	Added feature:                          */
/*                               - Coherence, # cycles, #average        */
/*                               - skip servoing if can't keep up       */
/*                                 with real time                       */
/*                               - Settling time                        */
/*                               - fixed channel ordering bug           */
/*                               - print option:                        */
/*                                 - print servoDelay, servoSkip and    */
/*                                   ezcaPut time                       */
/*                                 - lower case letters for number only */
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: none					                */
/*	References: none						*/
/*                                                         		*/
/* Author Information:							*/
/* Name          Telephone       Fax             e-mail 		*/
/* Stefan Ballmer (509)783-9046	  -              sballmer@ligo.mit.edu  */
/* Daniel Sigg    (509)372-8336  (509) 372-2178  sigg_d@ligo.mit.edu	*/
/*                                                         		*/
/* Code Compilation and Runtime Specifications:				*/
/*	Code Compiled on: Ultra-Enterprise, Solaris 5.6			*/
/*	Compiler Used: sun workshop C 4.2				*/
/*	Runtime environment: sparc/solaris				*/
/*                                                         		*/
/* Code Standards Conformance:						*/
/*	Code Conforms to: LIGO standards.	OK			*/
/*			  Lint.			TBD			*/
/*			  ANSI			TBD			*/
/*			  POSIX			TBD			*/
/*									*/
/* Known Bugs, Limitations, Caveats:					*/
/*								 	*/
/*									*/
/*                                                         		*/
/*                      -------------------                             */
/*                                                         		*/
/*                             LIGO					*/
/*                                                         		*/
/*        THE LASER INTERFEROMETER GRAVITATIONAL WAVE OBSERVATORY.	*/
/*                                                         		*/
/*                     (C) The LIGO Project, 1996.			*/
/*                                                         		*/
/*                                                         		*/
/* California Institute of Technology			   		*/
/* LIGO Project MS 51-33				   		*/
/* Pasadena CA 91125					   		*/
/*                                                         		*/
/* Massachusetts Institute of Technology		   		*/
/* LIGO Project MS 20B-145				   		*/
/* Cambridge MA 01239					   		*/
/*                                                         		*/
/* LIGO Hanford Observatory				   		*/
/* P.O. Box 1970 S9-02					   		*/
/* Richland WA 99352					   		*/
/*                                                         		*/
/* LIGO Livingston Observatory		   				*/
/* 19100 LIGO Lane Rd.					   		*/
/* Livingston, LA 70754					   		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <signal.h>
#include <complex>
#include <iostream>
#include <iomanip>
#include "tconv.h"
#include "dtt/testpoint.h"
#include "dtt/awgtype.h"
#include "DAQSocket.hh"

// #define GDS_NO_EPICS
#ifndef GDS_NO_EPICS
/*#include "ezca.h"*/
/* Data Types */
#define ezcaByte   0
#define ezcaString 1
#define ezcaShort  2
#define ezcaLong   3
#define ezcaFloat  4
#define ezcaDouble 5
#define VALID_EZCA_DATA_TYPE(X) (((X) >= 0)&&((X)<=(ezcaDouble)))

/* Return Codes */
#define EZCA_OK                0
#define EZCA_INVALIDARG        1
#define EZCA_FAILEDMALLOC      2
#define EZCA_CAFAILURE         3
#define EZCA_UDFREQ            4
#define EZCA_NOTCONNECTED      5
#define EZCA_NOTIMELYRESPONSE  6
#define EZCA_INGROUP           7
#define EZCA_NOTINGROUP        8

/* Functions */
extern "C" {
   void ezcaAutoErrorMessageOff(void);
   int ezcaGet(char *pvname, char ezcatype, int nelem, void *data_buff);
   int ezcaPut(char *pvname, char ezcatype, int nelem, void *data_buff);
   int ezcaSetRetryCount(int retry);
   int ezcaSetTimeout(float sec);
}
#endif
#define MAXCH 16
   const timespec waittime = {0, 10000000};

using namespace std;
static const int ListSz = 20000;

const double twopi=2*3.14159265358979;


/*----------------------------------------------------------------*\
  byte swaping for linux
\*----------------------------------------------------------------*/
void swap4char(char *ch) {
  char tmp;
  tmp= *(ch+3);
  *(ch+3)=*ch;
  *ch=tmp;
  
  tmp= *(ch+2);
  *(ch+2)=*(ch+1);
  *(ch+1)=tmp;  
}
/*----------------------------------------------------------------*\
  getExitcondState (0: continue, 1: exit, 2: suspend)
\*----------------------------------------------------------------*/
int
getExitcondState(char *Channel, bool Max,double ValRef,bool Suspend) {
    int State=0;
    double Val;
    if(Channel) {
    #ifdef GDS_NO_EPICS
      cerr << "Epics channel access not supported" <<endl;
      return (Suspend ? 2 : 1);
    #else
      // get channel value Val;
      if (ezcaGet (Channel, ezcaDouble, 1, &Val) != EZCA_OK) {
	cerr <<"channel "<<Channel<<" not accessible"<<endl;
	return (Suspend ? 2 : 1);
      }    
      // check condition
      State= ( (Max && (Val > ValRef)) || (!Max && (Val < ValRef)) ) ? (Suspend ? 2 : 1) : 0;
    #endif
    }
    return State;
}

/*----------------------------------------------------------------*\
  Formated output function
\*----------------------------------------------------------------*/
bool printIQ(int nChan,const char *format,double I,double Q,double IM,double QM,\
             double IE,double QE,double coh,double servoDelay,double PutDT,int servoSkip){
  char buf[2048];
  char *p=buf;
  if(nChan<2) { I=IM; Q=QM; };
  complex<double> c(I,Q);
  *p=0;
  for(;*format!=0;format++) {
    switch(*format) {
      case 'I':
          sprintf(p,"I = %10g   ",I);
        break;
      case 'Q':
          sprintf(p,"Q = %10g   ",Q);
        break;
      case 'A':
          sprintf(p,"abs = %10g   ",abs(c));
        break;
      case 'D':
          sprintf(p,"deg =%10g   ",1.0/3.14159265358979*180.0*arg(c));
	break;
      case 'R':
          sprintf(p,"rad =%10g   ",arg(c));
        break;
      case 'C':
          sprintf(p,"coherence = %10g   ",coh);
        break;
      case 'S':
          sprintf(p,"servoDelay = %10g   ",servoDelay);
        break;
      case 'P':
          sprintf(p,"ezcaPut-time = %10g   ",PutDT);
        break;
      case 'K':
          sprintf(p,"servoSkip = %5i   ",servoSkip);
        break;
      case 'X':
        if(nChan<2) sprintf(p,"I =%10g   Q =%g   ",I,Q);
	  else  sprintf(p,"I =%10g   Q =%10g   IM=%10g   QM=%10g   IE=%10g   QE=%10g   ",I,Q,IM,QM,IE,QE);
        break;
      case 'i':
          sprintf(p,"%10g   ",I);
        break;
      case 'q':
          sprintf(p,"%10g   ",Q);
        break;
      case 'a':
          sprintf(p,"%10g   ",abs(c));
        break;
      case 'd':
          sprintf(p,"%10g   ",1.0/3.14159265358979*180.0*arg(c));
	break;
      case 'r':
          sprintf(p,"%10g   ",arg(c));
        break;
      case 'c':
          sprintf(p,"%10g   ",coh);
        break;
      case 's':
          sprintf(p,"%10g   ",servoDelay);
        break;
      case 'p':
          sprintf(p,"%10g   ",PutDT);
        break;
      case 'k':
          sprintf(p,"%5i   ",servoSkip);
        break;
      case 'x':
        if(nChan<2) sprintf(p,"%10g   %g   ",I,Q);
	  else  sprintf(p,"%10g   %10g   %10g   %10g   %10g   %10g   ",I,Q,IM,QM,IE,QE);
        break;
    }
    p+=strlen(p);
  }
  if(buf<=p-3) *(p-3)=0;
  if(buf<p) cout<<buf<<endl;
  return false;
}

/*----------------------------------------------------------------*\
  Signal Handler
\*----------------------------------------------------------------*/

int caughtSignal = 0;		/* global signal variable */
int tpSelected=-1;
int awgActive=-1;
const char *activeChannel=0;
extern "C" {
void catchSignal(int sig)
{
    caughtSignal = sig;
    if(awgActive!=-1) {
      if(awgRemoveChannel(awgActive)) {cerr << "Can't remove channel from awg"<<endl;};
      awgActive=-1;
    }
    if(tpSelected!=-1) {
      if(tpClearName(activeChannel))      {cerr << "Can't clear testpoint "<<activeChannel<<endl;};
      testpoint_cleanup();
      tpSelected=-1;
    }
  //cerr << "caught signal "<<sig<<endl;
  exit(1);
}
}


/*----------------------------------------------------------------*\
  Main function
\*----------------------------------------------------------------*/
int 
main(int argc, const char *argv[]) {
//   #ifdef GDS_NO_EPICS
//      printf ("Epics channel access not supported\n");
//   #else
    DAQDChannel* List = new DAQDChannel[ListSz];

//------------------------------ local variables ------------------
    const char*   ndsname = getenv("LIGONDSIP");
    const char*   measChan = "H1:LSC-AS_Q";
    const char*   readbkChan  = 0;
    const char*   excChan  = 0;
    const char*   formatStr  = "IQAD";
    char          exitcondBuffer[2048];
    char*         exitcondChannel=0;
    const char*   exitcondCheck="min";
    bool          exitcondMax=false;
    double        exitcondVal=0;
    const char*   exitcondAction="exit";
    bool          exitcondSuspend=false;
    bool          suspendFlag=false;
    int           exitcondState=0; // 0: continue, 1: exit, 2: suspend
    double        ampl = 0;
    bool          byteSwap = false;
    double        settleTime=3; //seconds
    double        f  =   1; //Hz
    double        phaseME = 0;
    double        phase = 0;
    double        phaseLO = 0;
    float         tau = 1; //sec
    bool          expAv=false;
    int           Dav=1; // averages
    int           DavCt=0; // averages
    bool          DavExit=false;
    bool          DavExitNow=false;
    int           Ncyc=1; // cycles
    int           nChan = 0;
    if (!ndsname) ndsname = "fb0";
    bool debug = false, testdup = false;
    bool reverseorder=false;
    bool useservo=false;
    bool window=false;
    int Nmeas, Nreadbk;
 
    int awgslot;
    
    double *errPoint;
    double I=0;
    double Q=0;
    double ABS=0;
    int 		c;		/* option */
    extern char*	optarg;		/* option argument */
    extern int	optind;		/* option ind */
    int		errflag = 0;	/* error flag */
    char 		readback[256];	/* readback channel string */
    bool		havereadback = false; /* have readback ? */
    int                 Nch = 0;
    char 		channel[MAXCH][256];	/* channel string */
    int                 Ng = 0;
    double		gain[MAXCH] = {1.0};     /* gain */
    double		gain2[MAXCH]= {0.0};	/* gain of second channel */
    double		setval = 0.0;	/* set value */
    double		ugf = 1.0;	/* unity gain frequency */
    double		timeout = -1.0;	/* timeout */
    double              servoDelay=0;
    double              biggestServoDelay=0;
    double              maxServoDelay=0.25;
    double              PutDT=0;
    int                 servoSkip=0;
    
    strcpy (readback, "");
    strcpy (channel[0], "");
    
   
//---------------------------------  Get the arguments ------------------
     for (int i=1 ; i<argc ; i++) {
        if (!strcmp(argv[i], "nds")) {
	    ndsname = argv[++i];
        } else if (!strcmp(argv[i], "meas")) {
	    measChan = argv[++i];
        } else if (!strcmp(argv[i], "readbk") ||
	           !strcmp(argv[i], "ref")) {
	    readbkChan = argv[++i];
        } else if (!strcmp(argv[i], "exc")) {
	    excChan = argv[++i];
        } else if (!strcmp(argv[i], "excampl")) {
	    ampl = atof(argv[++i]);
        } else if (!strcmp(argv[i], "settle")) {
	    settleTime  = atof(argv[++i]);
        } else if (!strcmp(argv[i], "window")) {
	    window=true;
        } else if (!strcmp(argv[i], "f")) {
	    f  = atof(argv[++i]);
        } else if (!strcmp(argv[i], "tau")) {
	    tau  = atof(argv[++i]);
	    expAv=true;
        } else if (!strcmp(argv[i], "D")) {
	    Dav= atof(argv[++i]);
	    DavExit=false;
	    expAv=false;
	    if(Dav<1) {errflag = 1; cerr << "Error: number of averages must be positive" << endl;}
        } else if (!strcmp(argv[i], "E")) {
	    Dav= atof(argv[++i]);
	    DavExit=true;
	    expAv=false;
	    if(Dav<1) {errflag = 1; cerr << "Error: number of averages must be positive" << endl;}
        } else if (!strcmp(argv[i], "N")) {
	    Ncyc= atof(argv[++i]);
	    if(Ncyc<1) {errflag = 1; cerr << "Error: number of cycles must be positive" << endl;}
        } else if (!strcmp(argv[i], "phase")) {
	    phaseME  = twopi/360.0*atof(argv[++i]);
        } else if (!strcmp(argv[i], "phaseLO")) {
	    phase=phaseLO  = twopi/360.0*atof(argv[++i]);
        } else if (!strcmp(argv[i], "exitcond")) {
	    strcpy(exitcondBuffer,argv[++i]);
	    exitcondChannel=exitcondBuffer;
	    exitcondCheck=argv[++i];
	    exitcondVal=atof(argv[++i]);
	    if(!strcmp(exitcondCheck,"min")) {
	      exitcondMax=false;
	    } else if(!strcmp(exitcondCheck,"max")) {
	      exitcondMax=true;
	    } else { errflag = 1; cerr << "Invalid argument: " << exitcondCheck << endl; }
        } else if (!strcmp(argv[i], "exitaction")) {
	    exitcondAction=argv[++i];
	    if(!strcmp(exitcondAction,"exit")) {
	      exitcondSuspend=false;
	    } else if(!strcmp(exitcondAction,"wait") || 
	              !strcmp(exitcondAction,"suspend")) {
	      exitcondSuspend=true;
	    } else { errflag = 1; cerr << "Invalid argument: " << exitcondAction << endl; }
        } else if (!strcmp(argv[i], "format") ||
	           !strcmp(argv[i], "print")) {
	    formatStr=argv[++i];
        } else if (!strcmp(argv[i], "servo")) {
	    if(Nch < MAXCH) {
	      useservo=true;
	      strncpy (channel[Nch], argv[++i], sizeof(channel[Nch])-1);
              channel[Nch][sizeof(channel[Nch])-1] = 0;
	      Nch++;
	    } else { errflag = 1; cerr << "Too many 'servo' arguments, only "<<MAXCH<<" are allowed." << endl; }
        } else if (!strcmp(argv[i], "-r")) {
                  strncpy (readback, argv[++i], sizeof(readback) - 1);
                  readback[sizeof(readback)-1] = 0;
                  havereadback = true;
        } else if (!strcmp(argv[i], "-g")) {
	    if(Ng < MAXCH) {
           /* gain between radback and control */
                   gain[Ng] = atof (argv[++i]);
		   gain2[Ng]= 0.0;
		   Ng++;
	    } else { errflag = 1; cerr << "Too many '-g' arguments, only "<<MAXCH<<" are allowed." << endl; }
        } else if (!strcmp(argv[i], "-s")) {
            /* set value */
                  setval = atof (argv[++i]);
        } else if (!strcmp(argv[i], "-f")) {
            /* unity gain frequency */
                  ugf = fabs (atof (argv[++i]));
        } else if (!strcmp(argv[i], "-t")) {	  
            /* timeout */
                  timeout = atof (argv[++i]);
        } else if (!strcmp(argv[i], "-c")) {
            /* 2nd channel (common) */
	         if(Nch >= MAXCH || Ng >= MAXCH) {
		   errflag = 1; cerr << "Too many output channels arguments, only "<<MAXCH<<" are allowed." << endl;
		 } else if(Ng!=Nch) {
		   errflag = 1; cerr << "For -c and -d the other 'servo' and '-g' arguments have to be grouped together" << endl;
		 } else {
	            strncpy (channel[Nch], argv[++i], sizeof(channel[Nch])-1);
                    channel[Nch][sizeof(channel[Nch])-1] = 0;
	            Nch++;
                    gain[Ng] = 0.0;
		    gain2[Ng]= +1.0;
		    Ng++;
		 }
        } else if (!strcmp(argv[i], "-d")) {	  
             /* 2nd channel (differential) */
	         if(Nch >= MAXCH || Ng >= MAXCH) {
		   errflag = 1; cerr << "Too many output channels arguments, only "<<MAXCH<<" are allowed." << endl;
		 } else if(Ng!=Nch) {
		   errflag = 1; cerr << "For -c and -d the other 'servo' and '-g' arguments have to be grouped together" << endl;
		 } else {
	            strncpy (channel[Nch], argv[++i], sizeof(channel[Nch])-1);
                    channel[Nch][sizeof(channel[Nch])-1] = 0;
	            Nch++;
                    gain[Ng] = 0.0;
		    gain2[Ng]= -1.0;
		    Ng++;
		 }
        } else if (!strcmp(argv[i], "help")   || 
	           !strcmp(argv[i], "-help")  ||
		   !strcmp(argv[i], "--help") ||
		   !strcmp(argv[i], "-h")     ||
		   !strcmp(argv[i], "-?")     ||
		   !strcmp(argv[i], "?")) {	  
            /* help */
                  errflag = 1;
        } else if (!strcmp(argv[i], "swap")) {
	    byteSwap=true;
	} else if (!strcmp(argv[i], "-d")) {
	    testdup = true;
	} else if (!strcmp(argv[i], "debug")) {
	    debug = true;
	} else if (!strcmp(argv[i], "maxServoDelay")) {
	    maxServoDelay=atof(argv[++i]);
	} else {
                  errflag = 1;	    
	    cerr << "Invalid argument: " << argv[i] << endl;
	}
    }
    if(excChan && !readbkChan) readbkChan=excChan;
      if (!havereadback) {
         strcpy (readback, "I");
      }
      if(!strcmp(readback,"I")) { errPoint=&I;
      } else if(!strcmp(readback,"Q")) { errPoint=&Q;
      } else if(!strcmp(readback,"ABS")) { errPoint=&ABS;
      } else errflag = 1;
      if(Nch!=Ng) {
	errflag = 1;
	cerr << "Unequal number of control channels and gains specified";
	cerr << " (# channels: "<<Nch<<"   # gains: "<<Ng<<")"<<endl;
      }	    

      /* help */
      if (errflag || argc==1) {
         printf ("Usage: %s [options] \n"
	        "       nds 'nds-ip-address': nds server\n"
	        "       meas 'channel': demodulated channel\n"
		"       readbk 'channel': excitation readback channel for reference\n"
		"       exc 'channel': excitation channel (optional)\n"
		"       excampl 'amplitude': excitation amplitude\n"
		"       settle 'time': settling time in seconds - default is 3sec\n"
		"       window: use Hanning window (2*sin(pi*t/T))^2\n"
		"       f 'demod frequency': demodulation frequency\n"
		"       tau 'time': exponential average time in seconds\n"
		"       D '# averages': fixed number of averages\n"
		"       E '# averages': same as D, but quit when done\n"
		"       N 'cycles': number of cycles per measurement\n"
		"       phase 'phase': phase rotation in I-Q space in deg\n"
		"       print 'string': string consists of the following letters:\n"
		"                          I: print the I phase\n"
		"                          Q: print the Q phase\n"
		"                          A: print sqrt(I^2+Q^2)\n"
		"                          D: print angle(I+i*Q) in degree\n"
		"                          R: print angle(I+i*Q) in radiants\n"
		"                          C: print coherence\n"
		"                          X: print I,Q,IM,QM,IE,QE where (I+iQ)=(IM+iQM)/(IE+iQE)\n"
		"                          S: print servo lag behing real time in seconds\n"
		"                          P: print time  in seconds used up for ezcaPut since last print\n"
		"                          K: print # of ezcaPut's per channel skipped since last print \n"
		"                             in order to keep up with real time.\n"
		"                             (Normally there are 16 ezcaPut's per channel per second.)\n"
		"                       default string is \"IQAD\"\n"
		"                       lower case letters print the number only\n"
		"       debug: enable debug flag for DAQSocket class\n"
		"       maxServoDelay: max lag behind real time in seconds - default is 0.25sec\n"
		"                      If exceeded it stops writing to the control channels.\n"
		"       exitcond 'channel' [min|max] 'value': stop demodulation and servo if\n"
		"                                             'channel' [falls below|exceeds] 'value'\n"
		"       exitaction [exit|wait]: action when exitcond is true, default is exit\n"
		"       servo 'channel': Implements a simple integrator (pole at zero)\n"
 		"                        (For multiple output channels repeat the arguments\n"
		"                        servo 'channel' and -g 'gain', up to %i times)\n"
                "        -r 'readback': demodulated readback (error) signal; only I, Q or ABS, default is I\n"
                "        -g 'gain' : gain between readback and channel\n"
                "        -s 'value' : set value\n"
                "        -f 'ugf' : unity gain frequency (Hz)\n"
                "        -t 'duration' : timeout (sec)\n"
                "        -c 'channel' : 2nd control channel (common)\n"
                "        -d 'channel' : 2nd control channel (differential)\n"
                "        -h : help\n\n"
		"Example:\n"
		"       1) Aligning ETMX pitch:\n"
		"          ezcademod meas H1:LSC-LA_NPTRX exc H1:SUS-ETMX_OLPIT_EXC excampl 0.05 f 1 tau 1 phase -17 \\\n"
                "                    exitcond H1:LSC-LA_PTRX_NORM min $minArmPower exitaction wait \\\n"
                "                    servo H1:SUS-ETMX_PIT_COMM -g -0.03 -f 0.1 -t 30\n",argv[0],MAXCH);
         return 1;
      }

      if (timeout == 0) {
         return 0;
      }

    #ifndef GDS_NO_EPICS
      ezcaAutoErrorMessageOff();
      ezcaSetTimeout (0.001);
      ezcaSetRetryCount (100);
    #endif

//-------------------------- set up signal handler ------------------
   for( int sigIndex = 1; sigIndex < 18; ++sigIndex )
    if( sigIndex != SIGALRM && sigIndex != SIGKILL )
      signal(sigIndex, catchSignal);

//---------------------------------- start excitation ------------------
   if(excChan){
     char cmdString[1024];
     char *response;
     taisec_t tptime;
     if(tpRequestName (excChan,-1,NULL, NULL)) { cerr <<"Can't get testpoint "<<excChan<<endl; return 1;}
     activeChannel=excChan;
     tpSelected=1;
     awgActive=awgslot=awgSetChannel(excChan);

     sprintf(cmdString, "set %d sine %f %f 0.0 0.0",awgslot,f,ampl);
     if(debug) cout << cmdString<<endl;
     response=awgCommand(cmdString);
     if(debug) cout<<response<<endl;
     delete response;
   }
//---------------------------------- wait for the system to settle ------
   tainsec_t SettleT0 = TAInow();
   while ((double)((TAInow()-SettleT0)/1E9)<=settleTime);

//--------------------------------- Open a daqd socket ------------------
    if(debug) cout << "Opening a socket to ND server on node " << ndsname << endl;
    DAQSocket nds(ndsname);
    if (!nds.TestOpen()) {
        cerr << "Unable to open socket to NDS on node " << ndsname << endl;
	return 0;
    }

    if(debug) cout << "Getting available channels" << endl;
    int nc = nds.Available(List, ListSz);
    if(debug) cout << "Number of channels returned = " << nc << endl;
    if (nc > ListSz) {
        cout << "Warning: List holds only " << ListSz << " entries." << endl;
	nc = ListSz;
    }

//---------------------------------- Add channels to DAQSocket -----------
   nds.setDebug(debug);
   int i=0 ;
   for (; i < nc  && strcasecmp(List[i].mName,measChan); i++);
   if(i==nc) { cerr << measChan << " not a valid channel name" << endl; return 1;} 
   if(debug) printf("%s    %i\n",List[i].mName,i);
   //nds.setDebug(true);
   nds.AddChannel(List[i]);
   Nmeas=i;
   nChan++;
   if(readbkChan) {
     for (i=0; i < nc  && strcasecmp(List[i].mName,readbkChan); i++);
     if(i==nc) { cerr << readbkChan << " not a valid channel name" << endl; return 1;}
     if(debug) printf("%s    %i\n",List[i].mName,i);
     if(i!=Nmeas) nds.AddChannel(List[i]);
     Nreadbk=i;
     reverseorder=(nds.mChannel.begin()) == nds.mChannel.find(readbkChan);
     //if(reverseorder) {cout << "reverse order"<< endl; } else { cout << "regular order"<< endl; }
     nChan++;
   }
   
      // start time
      tainsec_t t0 = TAInow();
//---------------------------------- Set up servo stuff -----------
      double t = 0;
      double prev = 0;
      double dt;
      int servoCycle=-1;
      bool servoFirst=true;
      double err;
      double ctrl[MAXCH];
      // get previous control value
      if(useservo){
        for(int iii=0;iii<Nch;iii++) {
          #ifdef GDS_NO_EPICS
            printf ("channel %s not accessible\n", channel[iii]);
	    return 1;
	  #else
          if (ezcaGet (channel[iii], ezcaDouble, 1, &(ctrl[iii]) ) != EZCA_OK) {
             printf ("channel %s not accessible\n", channel[iii]);
             return 1;
          }
	  #endif
	}
      }
   
   
//---------------------------------- request the data -----------
   int erno = nds.RequestOnlineData(true,-1);
   if(erno) cout << "Server error #"<<erno << endl;
   
//---------------------------------- local variables for demodulation -----------
   char *buf;
   char *b2;
   float *fbuf;
   int ret;
   float IcM=0;
   float QcM=0;
   float IdM=0;
   float QdM=0;
   float IM=0;
   float QM=0;
   float IcE=0;
   float QcE=0;
   float IdE=0;
   float QdE=0;
   float IE=0;
   float QE=0;
   float wval=1;
   complex<double> cdbar(0,0);
   complex<double> ccbar(0,0);
   complex<double> ddbar(0,0);
   double coherence=0;
   float exDt=exp(-1.0/f/tau*Ncyc);
   double w= twopi * f; //rad/sec

   bool DemodUpdate=false;	 
   int headerOffset=sizeof(DAQDRecHdr);
   int reconf;
   int seqNum=-1;
   int errD;
   
//----------------------------- start demodulation loop --------------------
      do {
         // wait a little while
         //nanosleep (&waittime, 0);
         // get error signal
     ret=nds.GetData(&buf,-1);
     if(byteSwap) for(int ii=0;ii<ret+headerOffset;ii+=4) { swap4char(buf+ii); } 

         reconf = 0;
	 errD=0;
         if ((ret > 0) && 
            (((DAQDRecHdr*) buf)->GPS == (int)0x0FFFFFFFF)) {
            reconf = 1;
         }
         // check sequence number
         else if (ret > 0) {
            errD = (seqNum >= 0) && 
               (((DAQDRecHdr*) buf)->SeqNum != seqNum + 1) ? 1 : 0;
            seqNum = ((DAQDRecHdr*) buf)->SeqNum; 
         }
         if (errD || (ret < 0)) {
	    cerr << "seq # = " << seqNum << endl;
            cerr << "DATA RECEIVING ERROR " << ret << endl;
	 }


     if(!reconf) {
       b2=buf+headerOffset;
       fbuf=(float*)(buf+headerOffset);
       //printf("Read %i bytes   ",ret);
       //if(nChan==2) printf("%e %e %e %e\n",fbuf[1024-1],fbuf[1024+0],fbuf[2*1024-1],fbuf[2*1024+0]);
       //printf("%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n",b2[0],b2[1],b2[2],b2[3],b2[4],b2[5],b2[6],b2[7],b2[8],b2[9],b2[10],b2[11],b2[12],b2[13],b2[14],b2[15]);

       const int basefreq=16384;
       int Nval = basefreq/16;
       int mi,ri;
       bool mDo,rDo;
       int mDS,rDS;
       int mOff,rOff;
       mDS=basefreq/List[Nmeas].mRate;
       if(nChan==2) {
         rDS=basefreq/List[Nreadbk].mRate;
	 if(Nmeas == Nreadbk)       { mOff=          rOff=0; }
	   else if((!reverseorder)) { mOff=0;        rOff=Nval/mDS; }
	   else                     { mOff=Nval/rDS; rOff=0;}
       }
//----------------------------- loop through received data --------------------
       for(int ii=0;ii<Nval && !DavExitNow ;ii++) {
         if(nChan!=2) {
	   mi=ii/mDS;  mDo=!(ii%mDS);
	   ri=-1; rDo=false;
	 } else {
	   mi=mOff + ii/mDS;  mDo=!(ii%mDS);
	   ri=rOff + ii/rDS;  rDo=!(ii%rDS);
	 }
	 if(window) {
	   wval=sin((phase-phaseLO)/(2*Ncyc));
	   wval=2*wval*wval;
	 } else {
	   wval=1;
	 }
         if(mDo) {IcM+=wval*cos(phase)*fbuf[mi];  QcM+=wval*sin(phase)*fbuf[mi];}
         if(rDo) {IcE+=wval*cos(phase)*fbuf[ri];  QcE+=wval*sin(phase)*fbuf[ri];}
	 
         phase+=w/basefreq;
//----------------------------- check if next measurement is complete --------------------
	 if(phase>=twopi*Ncyc+phaseLO) {
	   phase-=twopi*Ncyc;
	   //----------------------------- check whether demodulation is suspended -----------------------------
	   if(suspendFlag || exitcondState) {
             IcM=QcM=IdM=QdM=IM=QM=IcE=QcE=IdE=QdE=IE=QE=I=Q=ABS=0;
             cdbar=ccbar=ddbar=complex<double>(0,0);
	     // reset timeout
             t0 = TAInow();
	     prev=0;
             if(!exitcondState) {
	       // we are fine again, restart demodulation
	       suspendFlag=false;
	       // read the ctrl values back in case the user had to adjust it manually
               if(useservo){
                 for(int iii=0;iii<Nch;iii++) {
                   #ifdef GDS_NO_EPICS
                     printf ("channel %s not accessible\n", channel[iii]);
	             return 1;
	           #else
                     if (ezcaGet (channel[iii], ezcaDouble, 1, &(ctrl[iii])) != EZCA_OK) {
                        printf ("channel %s not accessible\n", channel[iii]);
                        return 1;
                     }
		   #endif
		 }
               }
	     }
	     DemodUpdate=true; // print the zeros
	   } else {
	   //----------------------------- demodulation not suspended-----------------------------
             if(expAv) { IdM*=exDt;  QdM*=exDt; IdE*=exDt;   QdE*=exDt;} else {DavCt++;}
	     IdM+=IcM;  QdM+=QcM;  IdE+=IcE;  QdE+=QcE;
	     cdbar+=complex<double>(IcM,QcM) * complex<double>(IcE,-QcE);
	     ccbar+=complex<double>(IcM,QcM) * complex<double>(IcM,-QcM);
	     ddbar+=complex<double>(IcE,QcE) * complex<double>(IcE,-QcE);	     
             IcM=0;  QcM=0;  IcE=0;  QcE=0;
	     if(expAv || DavCt>=Dav) {
	       double norm= expAv ? (1-exDt) : (1.0/Dav);
	       IdM*=norm; QdM*=norm; IdE*=norm; QdE*=norm;
	       IM=IdM/List[Nmeas].mRate;     QM=QdM/List[Nmeas].mRate;
	       if(nChan==2) {IE=IdE/List[Nreadbk].mRate; QE=QdE/List[Nreadbk].mRate;}
	       if(!expAv) { IdM=0;  QdM=0;  IdE=0;  QdE=0;  DavCt=0; DavExitNow=DavExit;}
	       if(nChan==2) {
		 complex<double> Ctmp;
		 Ctmp=complex<double>(IM,QM)/complex<double>(IE,QE)*complex<double>(cos(phaseME),sin(phaseME));
		 I=Ctmp.real(); Q=Ctmp.imag(); ABS=abs(Ctmp);
		 coherence=((cdbar*conj(cdbar))/(ccbar*ddbar)).real();
	       }
	       DemodUpdate=true;
	     }
	   }
 	 } //----------------------------- end of measurement complete --------------------
       } //----------------------------- end of loop through received data ----------------
       if(DemodUpdate) {
         DemodUpdate=printIQ(nChan,formatStr,I,Q,IM,QM,IE,QE,coherence,biggestServoDelay,PutDT,servoSkip);
	 biggestServoDelay=-1e30;
	 PutDT=0;
	 servoSkip=0;
       }
 
//----------------------------- start servoing part ----------------     
         // get time
	 if(servoFirst) {
	   servoFirst=false;
	   t0 = TAInow();
	   prev=0;
	 }
         t = (TAInow() - t0) / 1E9;
	 servoCycle++;
         dt = t - prev;
         //printf("DELTA %10f   Time %10f  Cycle %10f  dt %10f\n",(double)t-(double)1.0/16.0*servoCycle,(double)t,(double)1.0/16.0*servoCycle,(double)dt);
     // check the exit condition
     exitcondState=getExitcondState(exitcondChannel,exitcondMax,exitcondVal,exitcondSuspend);
     if(exitcondState==2) {
       // reset demodulation and servo history
       IcM=QcM=IdM=QdM=IM=QM=IcE=QcE=IdE=QdE=IE=QE=I=Q=ABS=0;
       cdbar=ccbar=ddbar=complex<double>(0,0);
       // reset timeout
       t0 = TAInow();
       prev=0;
       suspendFlag=true;
     }
     //check whether we keep up with real time
     servoDelay=t - (double)servoCycle/16.0;
     if(biggestServoDelay < servoDelay) biggestServoDelay=servoDelay;
     if(servoDelay<maxServoDelay && !exitcondState && !suspendFlag) {
       prev = t;
       // do the servoing
       if(useservo){
           err =*errPoint -  setval;
           err *= -1;
           for(int iii=0;iii<Nch;iii++) {
             // compute control
	     if(gain2[iii]==0.0) {
               ctrl[iii] -= gain[iii] * ugf * dt * err;
	     } else {
               ctrl[iii] -= gain2[iii] * gain[0] * ugf * dt * err;
	     }
             // set new control value 
             #ifdef GDS_NO_EPICS
               printf ("channel %s not accessible\n", channel[iii]);
	       return 1;
	     #else
	       tainsec_t T1=TAInow();
	       if (ezcaPut (channel[iii], ezcaDouble, 1, &(ctrl[iii])) != EZCA_OK) {
                  printf ("channel %s not accessible\n", channel[iii]);
                  return 1;
               }
	       PutDT+= (TAInow() - T1) / 1E9;
	       //printf("eczaPUT time = %f\n",DT);
             #endif
	   }
       }
     } else {
       if(!exitcondState && !suspendFlag)
         //printf("Can't keep up with realtime servoing, skipping servo output\n");
         servoSkip++;
       }
      } else {t = (TAInow() - t0) / 1E9; if(debug) cout << "Received a reconfig block"<<endl;}
      delete buf;
         // loop until timeout
      } while (((timeout < 0) || (t < timeout)) && (!caughtSignal) && 
               (exitcondState!=1) && (!DavExitNow) );
//----------------------------- clean up --------------------
   if(excChan){
     if(awgRemoveChannel(awgslot)) {cerr << "Can't remove channel from awg"<<endl;};
     awgActive=-1;
     if(tpClearName(excChan))      {cerr << "Can't clear testpoint "<<excChan<<endl;};
     testpoint_cleanup();
     tpSelected=-1;
   }
   if(!DavExitNow) printf("done\n");
//   #endif
   exit(0);
}

