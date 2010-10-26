/* tocLongFile.cc.c                                                     */
/* read data from TOC and write to LongPlayback files                   */
/* compile with                                                           
/ldcg/bin/gcc -o tocLongFile -I/ldas/ldas-0.0/include -I/ldcg/include tocLongFile.cc  -L/ldas/ldas-0.0/lib -L/ldcg/lib  -lgeneral -lframecpp -lz
                                                                          */
#include <framecpp/framereader.hh>
#include <framecpp/frame.hh>
#include <fstream.h>
#include <iostream.h>
#include <iomanip.h>
#include <stdiostream.h>
#include <framecpp/adcdata.hh>
#include <framecpp/tocreader.hh>
#include <framecpp/types.hh>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
//#include "datasrv.h"

#define CHANNUM     16
#define PLAYDATA    0
#define PLAYTREND   1
#define PLAYTREND60 60
#define XAXISDATE   0
#define XAXISTOTAL  1

#define FAST        16384
#define SLOW        16

#define ZEROLOG         1.0e-20
using namespace FrameCPP;

long  fgps0, fgps1;

char  chName[CHANNUM][80], chUnit[CHANNUM][80];;
int   chNum[CHANNUM];
float slope[CHANNUM], offset[CHANNUM];
int   chstatus[CHANNUM];
short warning; /* for log0 case */
int   copies, startstop, trend;

int   longauto;
int   xyType[CHANNUM];  /* 0-linear; 1-Log; 2-Ln; 3-exp */


short   playMode, dcstep=1;
short   multiple, decimation, windowNum;
short   dmin=0, dmax=0, dmean=0;
short   xaxisFormat;

char    filename[100];
char    origDir[1024];



int main(int argc, char *argv[])
{
int     sum; 
char    rdata[32], framefile[200], tempch[80];

time_t  duration, gpstimest;

int     dataRecv, hr=1;
double  chData[FAST], fvalue;
int     sRate[CHANNUM];
long    totaldata = 0, skip = 0, skipstatus; 
char    tempfile[100];
FILE    *fd0, *fd[16], *fdM[16], *fdm[16];
int     j, l, bytercv;

printf ( "Entering tocLongFile\n" );
        strcpy(framefile, argv[1]);
        strcpy(origDir, argv[3]);
	windowNum = atoi(argv[4]);
        strcpy(filename, argv[5]);
	fgps0 = atoi(argv[6]);
	fgps1 = atoi(argv[7]);
	trend = atoi(argv[8]); /* 0-full frame input, 1, 60-trend frame */
	for ( j=0; j<windowNum; j++ ) {
	   slope[j] = 1.0;
	   offset[j] = 0.0;
	}

	fd0 = fopen( argv[2], "r" );
	fscanf ( fd0,"%s", rdata );
	longauto = atoi(rdata);
	fscanf ( fd0,"%s", rdata );
	for ( j=0; j<windowNum; j++ ) {
	   fscanf ( fd0,"%s", chName[j] );
	   fscanf ( fd0,"%s", chUnit[j] );
	   fscanf ( fd0,"%s", rdata );
	   chNum[j] = atoi(rdata);
	   fscanf ( fd0,"%s", rdata );
	   xyType[j] = atoi(rdata);
	   if ( longauto == 0 ) { /* not auto, record y settings */
	     fscanf ( fd0,"%s", rdata );
	     fscanf ( fd0,"%s", rdata );
	   }
	}
	for ( j=0; j<windowNum; j++ ) {
	  fscanf( fd0, "%s", rdata );
	}
	fscanf( fd0, "%s", rdata );
	multiple = atoi(rdata);
	fscanf( fd0, "%s", rdata );
	xaxisFormat = atoi(rdata);
	fscanf( fd0, "%s", rdata );
	decimation = atoi(rdata);
	fscanf( fd0, "%s", rdata );
	fscanf( fd0, "%s", rdata );
	fscanf( fd0, "%s", rdata );
	dmean = atoi(rdata);
	fscanf( fd0, "%s", rdata );
	dmax = atoi(rdata);
	fscanf( fd0, "%s", rdata );
	dmin = atoi(rdata);
	fscanf ( fd0, "%s", rdata ); //isgps
	fscanf ( fd0,"%s", rdata );
	gpstimest = atoi(rdata);
	fscanf ( fd0, "%s", rdata);
	duration = atoi(rdata);
	fscanf ( fd0, "%s", rdata);
	startstop = atoi(rdata);
	fclose(fd0);

	/*
	if ( multiple < 0 ) { // multiple copies 
	   copies = atoi(argv[7]);;
	   multiple = 0;
	   windowNum = 1;
	   strcpy(chName[0], chName[copies]);
	   xyType[0] = xyType[copies];
	   chNum[0] = chNum[copies];
	}
	else
	  copies = -1;
	*/
	copies = -1;




	if ( decimation == 0 ) { //full data choice
          switch ( trend ) {
            case 0: 
                playMode = PLAYDATA;
                break;
            case 1: 
                playMode = PLAYTREND;
		cout << "Warning: The input file is not full data file. Display as second trend data." << endl;
                break;
            case 60: 
                playMode = PLAYTREND60;
		cout << "Warning: The input file is not full data file. Display as minute trend data." << endl;
                break;
          }	  
	}
	else if ( decimation == 1 ) { //second data display choice
	  playMode = PLAYTREND;
          switch ( trend ) {
            case 0: 
	      cout << "Warning:  second trend display for full datafiles hasn't been implemented. Quit." << endl;
	      exit(10);
                break;
            case 1: 
                break;
            case 60: 
                playMode = PLAYTREND60;
		cout << "Warning: The input file is minute trend data." << endl;
                break;
          }	  
	}
	else { //display above second
	   playMode = PLAYTREND60;  /* play minute-trend */
	   if ( trend == 0 || trend ==1 ) {
	     cout << "Warning
:  Minute trend display hasn't been implemented for full or second trend files. Quit." << endl;
	     exit(10);
	   }
	}
        switch ( decimation ) {
          case 10: 
              dcstep = 10;
              break;
          case 100: 
              dcstep = 60;
              break;
          default: 
              dcstep = 1;
              break;
        }	

	sum = dmin + dmax + dmean;
	if ( sum == 0 )
	  dmean = 1;
	else if ( multiple && sum > 1 ) {
	  dmean = 1; dmin = 0; dmax = 0;
	}

	if ( startstop == 1) {
	  gpstimest = gpstimest - duration;
	}
	cout << "Requesting data from " << gpstimest << " to " 
             << gpstimest + duration -1 <<endl;
	INT_4S  time0 = gpstimest, time1 = gpstimest + duration -1;
	/* determine the real time range */
	if (time0 != time1)
	  if ( time0 >= fgps1 || time1 <= fgps0 ) {
	    cout << "Warning: The request is out of available range. Quit." << endl;
	    exit(1);
	  }
	if ( time0 < fgps0 ) {
	  time0 = fgps0;
	  cout << "Out of range. Reset starting time to " << time0 << endl;  
	}
	if ( time1 > fgps1 -1 ) {
	  time1 = fgps1 - 1;
	  cout << "Out of range. Reset ending time to " << time1 << endl;  
	}
	
	

	if ( copies < 0 )
	  fprintf ( stderr, "LongPlayBack starting\n" );
	else
	  fprintf ( stderr, "LongPlayBack Ch.%d starting\n", chNum[0] );
	fprintf ( stderr,"read file %s\n", framefile );

	for (j=0; j<windowNum; j++ ) {
	  if ( playMode == PLAYDATA ) {
	    sprintf( tempfile, "%s%d", filename, j );
	    fd[j] = fopen( tempfile, "w" );
	    if ( fd[j] == NULL ) {
	      fprintf ( stderr, "Unexpected error when opening files. Exit.\n" );
	      return -1;
	    }
	  }
	  else {
	    if ( dmax ) {
	      sprintf( tempfile, "%s%dM", filename, j );
	      fdM[j] = fopen( tempfile, "w" );
	      if ( fdM[j] == NULL ) {
		fprintf ( stderr, "Unexpected error when opening files. Exit.\n" );
		return -1;
	      }
	    }
	    if ( dmin ) {
	      sprintf( tempfile, "%s%dm", filename, j );
	      fdm[j] = fopen( tempfile, "w" );
	      if ( fdm[j] == NULL ) {
		fprintf ( stderr, "Unexpected error when opening files. Exit.\n" );
		return -1;
	      }
	    }
	    if ( dmean ) {
	      sprintf( tempfile, "%s%d", filename, j );
	      fd[j] = fopen( tempfile, "w" );
	      if ( fd[j] == NULL ) {
		fprintf ( stderr, "Unexpected error when opening files. Exit.\n" );
		return -1;
	      }
	    }
	  }
	}

	//Getting data from the Toc file
	/*
	skip = ? skipstatus=?; firsttimestring
	*/
	double data[200000];

	ifstream in(framefile);
	TOCReader fin(in);
	Frame* fr;
	AdcData* adc;
	INT_4S gpssec;
	int count_start, count_total, frlength;
	
	/*int totalfr = (int)fin.getFrameNumber();*/
	int totalfr = fin.TOC::getFrameNumber();
	vector<Time> vTime = fin.getGTime();
	vector<REAL_8> vDt = fin.getDt();
	int i0, count;
	REAL_8 dt;
	skip = 0;
	for ( int i=0; i<totalfr; i++ ) { //which Frame to start
	  if ((vTime)[i].getSec() + (int)vDt[i] -1 >= time0 ) {
	    i0 = i;
	    break;
	  }
	}
	count = i0;
	while ( (vTime)[count].getSec() <= time1 ) {
          if (count >= totalfr) break;
	  fr = fin.readFrame((unsigned int)(count));
	  gpssec = fr->getGTime().getSec();
	  dt = vDt[count];
	  if (count > i0) {
	    skip = skip + (vTime)[count].getSec() - (vTime)[count-1].getSec() - (int)dt;
	  }
	  if ( playMode == PLAYDATA ) {
	    frlength = (int)dt;
	    count_start = 0;
	    count_total = frlength;
	    if ( gpssec < time0 ) {
	      count_start = time0 - gpssec;
	      count_total = frlength - count_start;
	    }
	    if ( gpssec + frlength - 1 > time1 )
	      count_total = time1 - gpssec +1 - count_start;
	    if ( frlength > 1 )
	      cout << "Time Stamp: " << gpssec << " - Length " << frlength << endl;
	    else
	      cout << "Time Stamp: " << gpssec << endl;
	    if ( count_start > 0 || count_total < frlength) {
	      gpssec = gpssec + count_start;
	      cout << "  -start at " << gpssec << " for " << 
                       count_total << " seconds" << endl;	      
	    }
	    hr = 1;
	    totaldata = totaldata + count_total;	    
	    for ( j=0; j<windowNum; j++ ) {
	      adc = fin.readADC( count, chName[j] );
	      sRate[j] = (int)adc->getSampleRate();
	      INT_2U typeno = adc->refData()[0]->getType();
	      /*cout << " -Name: " << adc->getName() 
	       << " -Rate: " << sRate[j] 
	       << " -Type: " << typeno 
	       << endl;*/
	      //INT_2U *data2u = (INT_2U *) adc->refData()[0]->getData();
		
              switch ( typeno ) {
	      case FR_VECT_2S:
		for ( int i=0; i<count_total*sRate[j]; i++ ) {
		  data[i] = (double)(((INT_2S *) adc->refData()[0]->getData())[i+count_start]);
		}
		break;
	      case FR_VECT_2U:
		for ( int i=0; i<count_total*sRate[j]; i++ ) {
		  data[i] = (double)(((INT_2U *) adc->refData()[0]->getData())[i+count_start]);
		}
		break;
	      case FR_VECT_4S:
		for ( int i=0; i<count_total*sRate[j]; i++ ) {
		  data[i] = (double)(((INT_4S *) adc->refData()[0]->getData())[i+count_start]);
		}
		break;
	      case FR_VECT_4U:
		for ( int i=0; i<count_total*sRate[j]; i++ ) {
		  data[i] = (double)(((INT_4U *) adc->refData()[0]->getData())[i+count_start]);
		}
		break;
	      case FR_VECT_8S:
		for ( int i=0; i<count_total*sRate[j]; i++ ) {
		  data[i] = (double)(((INT_8S *) adc->refData()[0]->getData())[i+count_start]);
		}
		break;
	      case FR_VECT_8U:
		for ( int i=0; i<count_total*sRate[j]; i++ ) {
		  data[i] = (double)(((INT_8U *) adc->refData()[0]->getData())[i+count_start]);
		}
		break;
	      case FR_VECT_4R:
		for ( int i=0; i<count_total*sRate[j]; i++ ) {
		  data[i] = (double)(((REAL_4 *) adc->refData()[0]->getData())[i+count_start]);
		}
		break;
	      case FR_VECT_8R:
		for ( int i=0; i<count_total*sRate[j]; i++ ) {
		  data[i] = (double)(((REAL_8 *) adc->refData()[0]->getData())[i+count_start]);
		}
		break;
	      default:
		cout << "unknown data type" << endl;
		return -2;
		break;
	      }	      
		
              switch ( xyType[j] ) {
	      case 1: // Log 
		for ( int i=0; i<count_total*sRate[j]; i++ ) {
		  fvalue = data[i];
		  if ( fvalue == 0.0 ) {
		    fvalue = ZEROLOG;
		    warning = 1;
		  }
		  else
		    fvalue = fabs(fvalue);
		  fprintf ( fd[j], "%2f\t%le\n", gpssec+(i*1.0)/sRate[j], offset[j]+slope[j]*fvalue );
		}
		break;
	      case 2: // Ln 
		for ( int i=0; i<count_total*sRate[j]; i++ ) {
		  fvalue = data[i];
		  if ( fvalue == 0.0 ) {
		    fvalue = ZEROLOG;
		    warning = 1;
		  }
		  else
		    fvalue = log(fabs(fvalue));
		  fprintf ( fd[j], "%2f\t%le\n", gpssec+(i*1.0)/sRate[j], offset[j]+slope[j]*fvalue );
		}
		break;
	      default: 
		for ( int i=0; i<count_total*sRate[j]; i++ ) {
		  fprintf ( fd[j], "%2f\t%le\n", gpssec+(i*1.0)/sRate[j], offset[j]+slope[j]*data[i] );
		}
		break;
	      }		 
	    }
	  }
	  else { //trend
	    if ( trend ) { //1(second trend) or 60(minute trend)
	      cout << "Time Stamp: " << gpssec << " - Length " << dt << endl;
	      hr = trend;
	      dcstep = 1; //only consider this case for now
	      totaldata = totaldata + (int)dt;
	      for ( j=0; j<windowNum; j++ ) {
		if ( dmax ) {
		  sprintf ( tempch, "%s.max", chName[j] );
		  adc = fin.readADC( count, tempch );
		  INT_2U typeno = adc->refData()[0]->getType();
		  switch ( typeno ) {
		  case FR_VECT_2S:
		    for ( int i=0; i<dt/hr; i++ ) {
		      data[i] = (double)(((INT_2S *) adc->refData()[0]->getData())[i]);
		    }
		    break;
		  case FR_VECT_2U:
		    for ( int i=0; i<dt/hr; i++ ) {
		      data[i] = (double)(((INT_2U *) adc->refData()[0]->getData())[i]);
		    }
		    break;
		  case FR_VECT_4S:
		    for ( int i=0; i<dt/hr; i++ ) {
		      data[i] = (double)(((INT_4S *) adc->refData()[0]->getData())[i]);
		    }
		    break;
		  case FR_VECT_4U:
		    for ( int i=0; i<dt/hr; i++ ) {
		      data[i] = (double)(((INT_4U *) adc->refData()[0]->getData())[i]);
		    }
		    break;
		  case FR_VECT_8S:
		    for ( int i=0; i<dt/hr; i++ ) {
		      data[i] = (double)(((INT_8S *) adc->refData()[0]->getData())[i]);
		    }
		    break;
		  case FR_VECT_8U:
		    for ( int i=0; i<dt/hr; i++ ) {
		      data[i] = (double)(((INT_8U *) adc->refData()[0]->getData())[i]);
		    }
		    break;
		  case FR_VECT_4R:
		    for ( int i=0; i<dt/hr; i++ ) {
		      data[i] = (double)(((REAL_4 *) adc->refData()[0]->getData())[i]);
		    }
		    break;
		  case FR_VECT_8R:
		    for ( int i=0; i<dt/hr; i++ ) {
		      data[i] = (double)(((REAL_8 *) adc->refData()[0]->getData())[i]);
		    }
		    break;
		  default:
		    cout << "unknown data type" << endl;
		    return -2;
		    break;
		  }	      
		  
		  switch ( xyType[j] ) {
		  case 1: // Log 
		    for ( int i=0; i<dt/hr; i++ ) {
		      fvalue = data[i];
		      if ( fvalue == 0.0 ) {
			fvalue = ZEROLOG;
			warning = 1;
		      }
		      else
			fvalue = fabs(fvalue);
		      fprintf ( fdM[j], "%2f\t%le\n", gpssec+(i*hr*1.0), offset[j]+slope[j]*fvalue );
		    }
		    break;
		  case 2: // Ln 
		    for ( int i=0; i<dt/hr; i++ ) {
		      fvalue = data[i];
		      if ( fvalue == 0.0 ) {
			fvalue = ZEROLOG;
			warning = 1;
		      }
		      else
			fvalue = log(fabs(fvalue));
		      fprintf ( fdM[j], "%2f\t%le\n", gpssec+(i*hr*1.0), offset[j]+slope[j]*fvalue );
		    }
		    break;
		  default: 
		    for ( int i=0; i<dt/hr; i++ ) {
		      fprintf ( fdM[j], "%2f\t%le\n", gpssec+(i*hr*1.0), offset[j]+slope[j]*data[i] );
		    }
		    break;
		  }	
		}	
		if ( dmean ) {
		  sprintf ( tempch, "%s.mean", chName[j] );
		  adc = fin.readADC( count, tempch );
		  INT_2U typeno = adc->refData()[0]->getType();
		  switch ( typeno ) {
		  case FR_VECT_2S:
		    for ( int i=0; i<dt/hr; i++ ) {
		      data[i] = (double)(((INT_2S *) adc->refData()[0]->getData())[i]);
		    }
		    break;
		  case FR_VECT_2U:
		    for ( int i=0; i<dt/hr; i++ ) {
		      data[i] = (double)(((INT_2U *) adc->refData()[0]->getData())[i]);
		    }
		    break;
		  case FR_VECT_4S:
		    for ( int i=0; i<dt/hr; i++ ) {
		      data[i] = (double)(((INT_4S *) adc->refData()[0]->getData())[i]);
		    }
		    break;
		  case FR_VECT_4U:
		    for ( int i=0; i<dt/hr; i++ ) {
		      data[i] = (double)(((INT_4U *) adc->refData()[0]->getData())[i]);
		    }
		    break;
		  case FR_VECT_8S:
		    for ( int i=0; i<dt/hr; i++ ) {
		      data[i] = (double)(((INT_8S *) adc->refData()[0]->getData())[i]);
		    }
		    break;
		  case FR_VECT_8U:
		    for ( int i=0; i<dt/hr; i++ ) {
		      data[i] = (double)(((INT_8U *) adc->refData()[0]->getData())[i]);
		    }
		    break;
		  case FR_VECT_4R:
		    for ( int i=0; i<dt/hr; i++ ) {
		      data[i] = (double)(((REAL_4 *) adc->refData()[0]->getData())[i]);
		    }
		    break;
		  case FR_VECT_8R:
		    for ( int i=0; i<dt/hr; i++ ) {
		      data[i] = (double)(((REAL_8 *) adc->refData()[0]->getData())[i]);
		    }
		    break;
		  default:
		    cout << "unknown data type" << endl;
		    return -2;
		    break;
		  }	      
		  
		  switch ( xyType[j] ) {
		  case 1: // Log 
		    for ( int i=0; i<dt/hr; i++ ) {
		      fvalue = data[i];
		      if ( fvalue == 0.0 ) {
			fvalue = ZEROLOG;
			warning = 1;
		      }
		      else
			fvalue = fabs(fvalue);
		      fprintf ( fd[j], "%2f\t%le\n", gpssec+(i*hr*1.0), offset[j]+slope[j]*fvalue );
		    }
		    break;
		  case 2: // Ln 
		    for ( int i=0; i<dt/hr; i++ ) {
		      fvalue = data[i];
		      if ( fvalue == 0.0 ) {
			fvalue = ZEROLOG;
			warning = 1;
		      }
		      else
			fvalue = log(fabs(fvalue));
		      fprintf ( fd[j], "%2f\t%le\n", gpssec+(i*hr*1.0), offset[j]+slope[j]*fvalue );
		    }
		    break;
		  default: 
		    for ( int i=0; i<dt/hr; i++ ) {
		      fprintf ( fd[j], "%2f\t%le\n", gpssec+(i*hr*1.0), offset[j]+slope[j]*data[i] );
		    }
		    break;
		  }	
		}	
		if ( dmin ) {
		  sprintf ( tempch, "%s.min", chName[j] );
		  adc = fin.readADC( count, tempch );
		  INT_2U typeno = adc->refData()[0]->getType();
		  switch ( typeno ) {
		  case FR_VECT_2S:
		    for ( int i=0; i<dt/hr; i++ ) {
		      data[i] = (double)(((INT_2S *) adc->refData()[0]->getData())[i]);
		    }
		    break;
		  case FR_VECT_2U:
		    for ( int i=0; i<dt/hr; i++ ) {
		      data[i] = (double)(((INT_2U *) adc->refData()[0]->getData())[i]);
		    }
		    break;
		  case FR_VECT_4S:
		    for ( int i=0; i<dt/hr; i++ ) {
		      data[i] = (double)(((INT_4S *) adc->refData()[0]->getData())[i]);
		    }
		    break;
		  case FR_VECT_4U:
		    for ( int i=0; i<dt/hr; i++ ) {
		      data[i] = (double)(((INT_4U *) adc->refData()[0]->getData())[i]);
		    }
		    break;
		  case FR_VECT_8S:
		    for ( int i=0; i<dt/hr; i++ ) {
		      data[i] = (double)(((INT_8S *) adc->refData()[0]->getData())[i]);
		    }
		    break;
		  case FR_VECT_8U:
		    for ( int i=0; i<dt/hr; i++ ) {
		      data[i] = (double)(((INT_8U *) adc->refData()[0]->getData())[i]);
		    }
		    break;
		  case FR_VECT_4R:
		    for ( int i=0; i<dt/hr; i++ ) {
		      data[i] = (double)(((REAL_4 *) adc->refData()[0]->getData())[i]);
		    }
		    break;
		  case FR_VECT_8R:
		    for ( int i=0; i<dt/hr; i++ ) {
		      data[i] = (double)(((REAL_8 *) adc->refData()[0]->getData())[i]);
		    }
		    break;
		  default:
		    cout << "unknown data type" << endl;
		    return -2;
		    break;
		  }	      
		  
		  switch ( xyType[j] ) {
		  case 1: // Log 
		    for ( int i=0; i<dt/hr; i++ ) {
		      fvalue = data[i];
		      if ( fvalue == 0.0 ) {
			fvalue = ZEROLOG;
			warning = 1;
		      }
		      else
			fvalue = fabs(fvalue);
		      fprintf ( fdm[j], "%2f\t%le\n", gpssec+(i*hr*1.0), offset[j]+slope[j]*fvalue );
		    }
		    break;
		  case 2: // Ln 
		    for ( int i=0; i<dt/hr; i++ ) {
		      fvalue = data[i];
		      if ( fvalue == 0.0 ) {
			fvalue = ZEROLOG;
			warning = 1;
		      }
		      else
			fvalue = log(fabs(fvalue));
		      fprintf ( fdm[j], "%2f\t%le\n", gpssec+(i*hr*1.0), offset[j]+slope[j]*fvalue );
		    }
		    break;
		  default: 
		    for ( int i=0; i<dt/hr; i++ ) {
		      fprintf ( fdm[j], "%2f\t%le\n", gpssec+(i*hr*1.0), offset[j]+slope[j]*data[i] );
		    }
		    break;
		  }	
		}	
	      } //end of for j=0,...,windowNum
	    }
            else { //trend = 0
	      cout << "Not implemented" << endl;
	      exit(10);
            }
	    //trend case end
	  }
	  count++;
	  delete fr; 
        }  // end of while loop 
	if (totaldata > 0)
	  dataRecv = 1;
	cout << "Total data: " << totaldata << " Skip: " << skip <<endl;
	in.close();

	warning = 0;
	if ( dataRecv ) {
	  //close the writing data files
           switch ( playMode ) {
             case PLAYDATA:
		 for (j=0; j<windowNum; j++ ) {
		    fprintf ( fd[j], "&\n" );
		    fclose( fd[j] );
		 }
	       break;
             case PLAYTREND: 
             case PLAYTREND60: 
		 for ( j=0; j<windowNum; j++ ) {
		   if ( dmax ) {
		     if ( dcstep == 1 && xaxisFormat == XAXISTOTAL )
		       fprintf ( fdM[j], "&\n" );
		     fclose( fdM[j] );
		   }
		   if ( dmean ) {
		     if ( dcstep == 1 && xaxisFormat == XAXISTOTAL )
		       fprintf ( fd[j], "&\n" );
		     fclose( fd[j] );
		   }
		   if ( dmin ) {
		     if ( dcstep == 1 && xaxisFormat == XAXISTOTAL )
		       fprintf ( fdm[j], "&\n" );
		     fclose( fdm[j] );
		   }
		 }
                 break;
             default: 
                 break;
           }
	   
	   if ( copies < 0 )
	     fprintf ( stderr,"totla data skipped: %d second(s)\n", skip );
	   else
	     fprintf ( stderr,"Ch.%d totla data skipped: %d second(s)\n", chNum[0], skip );
           if ( playMode == PLAYTREND60 ) {
	     if ( copies < 0 )
	       fprintf ( stderr, "LongPlayBack: total %9.1f minutes (%d seconds) of data displayed\n", (float)totaldata/hr, totaldata );
	     else
	       fprintf ( stderr, "LongPlayBack Ch.%d: total %9.1f minutes (%d seconds) of data displayed\n", chNum[0], (float)totaldata/hr, totaldata );
	   }
	   else {
	     if ( copies < 0 )
	       fprintf ( stderr, "LongPlayBack: total %d seconds of data displayed\n", totaldata );
	     else
	       fprintf ( stderr, "LongPlayBack Ch.%d: total %d seconds of data displayed\n", chNum[0], totaldata );
	   }
	}
	else {
	   if ( copies < 0 )
	     fprintf ( stderr, "LongPlayBack: no data received\n" );
	   else
	     fprintf ( stderr, "LongPlayBack Ch.%d: no data received\n", chNum[0] );
	   fflush(stdout);
	   return -1;
	}
	/* append to the playset file */
	fd0 = fopen( argv[2], "a" );
	fprintf (  fd0, "%d\n", totaldata );
	fprintf (  fd0, "%d\n", skip );
	fclose(fd0);


        if ( warning ) {
	   if ( copies < 0 )
	     fprintf ( stderr,"Warning: 0 as input of log, put 1.0e-20 instead\n"  );
	   else
	     fprintf ( stderr,"Warning Ch.%d: 0 as input of log, put 1.0e-20 instead\n", chNum[0] );
	}

	/* Finishing */
	fflush(stdout);
	return 0;
}



 
