/* frLongFile.cc.c                                                     */
/* read data from each Frame file in the directory and write to        */ 
/* LongPlayback files                                                  */
/* compile with                                                           

c++ -c frLongFile -I/usr1/ldas/framecpp-0.4.14Nov12/include -I/ldcg/include frLongFile.cc

c++ -o frLongFile -I/usr1/ldas/framecpp-0.4.14Nov12/include -I/ldcg/include frLongFile.cc -L/usr1/ldas/framecpp-0.4.14Nov12/lib -L/ldcg/lib -lgeneral -lframecpp -lz -lbz2

/ldcg/bin/gcc -o frLongFile -I/ldas/ldas-0.0/include -I/ldcg/include frLongFile.cc  -L/ldas/ldas-0.0/lib -L/ldcg/lib  -lgeneral -lframecpp -lz
                                                                          */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <iostream>
#include <fstream>

#include "framecpp/Version6/FrameH.hh"
#include "framecpp/Version6/FrCommon.hh"
#include "framecpp/Version6/Functions.hh"
#include "framecpp/Version6/IFrameStream.hh"
#include "framecpp/Version6/OFrameStream.hh"
#include "framecpp/Version6/Util.hh"

using namespace std;

#define CHANNUM     16
#define PLAYDATA    0
#define PLAYTREND   1
#define PLAYTREND60 60
#define XAXISDATE   0
#define XAXISTOTAL  1

#define ZEROLOG         1.0e-20
//using namespace FrameCPP;

long  fgps0, fgps1;

char  chName[CHANNUM][80], chUnit[CHANNUM][80];
int   chNum[CHANNUM], frCh[CHANNUM];
short frMin, frMax, frMean;
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

int getFrCH(char* filtfile); 
int getFrameInfo(char* frfilename, int* frgps, int* frlength);

int main(int argc, char *argv[])
{
int     sum; 
char    rdata[32], filterfile[200], tempch[80], playsetfile[200];

time_t  duration, gpstimest;
INT_4S  time0, time1;

int     dataRecv, hr=1;
double  fvalue;
int     sRate[CHANNUM];
long    totaldata = 0, skip = 0; 
char    tempfile[100];
FILE    *fd0, *fd[16], *fdM[16], *fdm[16];
int     j, l, bytercv;

printf ( "Entering frLongFile\n" );
	sprintf ( playsetfile, "%splayset", argv[2] );
	sprintf ( filterfile, "%sfilter_fr", argv[2] );
        strcpy(origDir, argv[3]);
	windowNum = atoi(argv[4]);
        strcpy(filename, argv[5]);
	fgps0 = atoi(argv[6]);
	fgps1 = atoi(argv[7]);
	trend = atoi(argv[8]); /* 0-full frame input, 1, 60-trend frame */
	if (trend == 0)
	  printf ( "full frame file(s)\n" );
	else
	  printf ( "trend file(s): %d\n", trend );
	for ( j=0; j<windowNum; j++ ) {
	   slope[j] = 1.0;
	   offset[j] = 0.0;
	}

	fd0 = fopen( playsetfile, "r" );
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
	//if ( trend == 0 ) {
	  fscanf ( fd0, "%s", rdata ); //isgps
	  fscanf ( fd0,"%s", rdata );
	  gpstimest = atoi(rdata);
	  fscanf ( fd0, "%s", rdata);
	  duration = atoi(rdata);
	  fscanf ( fd0, "%s", rdata);
	  startstop = atoi(rdata);
	//}
	fclose(fd0);

        //Get the ADC number for each given channel
	if (getFrCH(filterfile)) {
	  cout << "No Frame file opened. Quit" << endl;
	  return -2;
	}

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
	     cout << "Warning:  Minute trend display hasn't been implemented for full or second trend files. Quit." << endl;
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

	if ( trend == 0 ) {
	  if ( startstop == 1) {
	    gpstimest = gpstimest - duration;
	  }
	  cout << "Requesting data from " << gpstimest << " to " 
	       << gpstimest + duration -1 <<endl;
	  time0 = gpstimest; 
	  time1 = gpstimest + duration -1;
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
	}

	if ( copies < 0 )
	  fprintf ( stderr, "LongPlayBack starting\n" );
	else
	  fprintf ( stderr, "LongPlayBack Ch.%d starting\n", chNum[0] );

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

	//Open each Frame file in filter_fr to get data
	double data[200000];

	INT_4S gpssec;
	//Frame* fr;
	FrameCPP::Version_6::FrameH* fr;
	//AdcData* adc;
	FrameCPP::Version_6::FrAdcData *adc;
	INT_4U chan;
	char inputfile[200];
	int namest_time, namest_time_last, frgps, frlength, tmpi;
	int count_start, count_total;
	FILE *fp;
	REAL_8 dt;
	
	fp = fopen(filterfile, "r");
	if ( fp == NULL ) {
	  cout << "Cann't open reading file: " << filterfile << endl;
	  exit(1);
	}
	fscanf ( fp, "%s", inputfile); 
	while ( fscanf ( fp, "%s", inputfile) != EOF ) {
	  tmpi = getFrameInfo(inputfile, &frgps, &frlength);
	  if ( !trend && ( (tmpi < 0) || (frgps > time1) || 
	       (frgps+frlength-1 < time0) )) {
	    ; //don't display this Frame file
	  }
	  else { //is a Frame file which intersects with requested time
	    //cout << "Reading Frame file: " << inputfile << endl;
	  count_start = 0;
	  count_total = frlength;
	  if ( frgps < time0 ) {
	    count_start = time0 - frgps;
	    count_total = frlength - count_start;
	  }
	  if ( frgps + frlength - 1 > time1 )
	    count_total = time1 - frgps +1 - count_start;
	  if ( playMode == PLAYDATA ) {
	    //determine if within the requested time range
	    namest_time_last = namest_time;
	    namest_time = frgps;
	    if ( totaldata > 0 && namest_time - namest_time_last > 1 )
	      skip = skip + namest_time - namest_time_last -1;
	    namest_time = namest_time + frlength;  
	    totaldata = totaldata + count_total;	    
	    ifstream in( inputfile );
	    //FrameReader fin(in);
	    FrameCPP::Version_6::IFrameStream fin (in);
	    //fr = fin.readFrame();
	    fr = fin.ReadNextFrame();
	    fr->getRawData()->refAdc().rehash();
	    gpssec = fr->getGTime().getSec();
	    if ( frlength > 1 )
	      cout << "Time Stamp: " << gpssec << " - Length " << frlength << endl;
	    else
	      cout << "Time Stamp: " << gpssec << endl;
	    if ( count_start > 0 || count_total < frlength) {
	      gpssec = gpssec + count_start;
	      cout << "  -start at " << gpssec << " for " << 
                       count_total << " seconds" << endl;	      
	    }
	    for ( j=0; j<windowNum; j++ ) {
	      adc = fr->getRawData()->refAdc()[frCh[j]];
	      //cout << "ch" << j <<": " << adc->getName() << endl;
	      sRate[j] = (int)adc->getSampleRate();
	      INT_2U typeno = adc->refData()[0]->getType();
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
	    delete fr; 
	    in.close();
	  }
	  else { //trend
	    totaldata++;
	    ifstream in( inputfile );
	    FrameCPP::Version_6::IFrameStream fin (in);
	    //FrameReader fin(in);
	    //fr = fin.readFrame();
	    fr = fin.ReadNextFrame();
	    fr->getRawData()->refAdc().rehash();
	    gpssec = fr->getGTime().getSec();
	    dt = fr->getDt();
	    if ( trend ) { //1(second trend) or 60(minute trend)
	      cout << "Time Stamp: " << gpssec << " - Length " << dt << endl;
	      hr = trend;
	      dcstep = 1; //only consider this case for now
	      totaldata = totaldata + (int)dt;
	      for ( j=0; j<windowNum; j++ ) {
		if ( dmax ) {
		  sprintf ( tempch, "%s.max", chName[j] );
		  adc = fr->getRawData()->refAdc()[frCh[j]+frMax];
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
		  adc = fr->getRawData()->refAdc()[frCh[j]+frMean];
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
		  adc = fr->getRawData()->refAdc()[frCh[j]+frMin];
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
	    delete fr; 
	    in.close();
	  } //trend case end
	}
	} // end of while loop
	fclose(fp);

	if (totaldata > 0)
	  dataRecv = 1;
	cout << "Total data: " << totaldata << " Skip: " << skip << endl;

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
	fd0 = fopen( playsetfile, "a" );
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


int getFrCH(char* filterfile) 
{

    INT_4U chan;
    //AdcData* adc;
    FrameCPP::Version_6::FrAdcData *adc;
    char inputfile[200], tempname[100];
    string tempst, firstch, tempbst;
    FILE *fp;
    int j, pos;
    int nofile=1;
   
    fp = fopen(filterfile, "r");
    if ( fp == NULL ) {
      cout << "Cann't open reading file: " << filterfile << endl;
      exit(1);
    }

    fscanf ( fp, "%s", inputfile); 
    fscanf ( fp, "%s", inputfile); 
    ifstream in( inputfile );
    //FrameReader fin(in);
    FrameCPP::Version_6::IFrameStream fin (in);
    fclose(fp);

    frMin=0; frMax=0; frMean=0;
    //Frame* fr = fin.readFrame();
    FrameCPP::Version_6::FrameH* fr = fin.ReadNextFrame();
    fr->getRawData()->refAdc().rehash();
    chan = fr->getRawData()->refAdc().getSize();
    for (int j=0; j<chan; ++j) {
      adc = fr->getRawData()->refAdc()[j];
      tempst = adc->getName();
      if ( trend == 0 ) {
	for ( int i=0; i<windowNum; i++ ) {
	  if ( tempst.compare(chName[i]) == 0) {
	    frCh[i] = j;
	    break;
	  }
        }
      }
      else { //trend   position of .n, frMin, frMax, frMean
	for ( int i=0; i<windowNum; i++ ) {
	  sprintf ( tempname, "%s.n", chName[j] );
	  if ( tempst.compare(tempname) == 0) {
	    frCh[i] = j;
	    break;
	  }
        }
	if (j == 0) {
	  pos = tempst.find(".n");
	  firstch = tempst.erase(pos, tempst.length()-pos);
	}
	if ( j <6  ) {
	  tempbst = firstch;
	  tempbst = tempbst.append(".min");
	  if ( tempst.compare(tempbst) == 0) {
	    frMin = j;
	  }
	  tempbst = firstch;
	  tempbst = tempbst.append(".max");
	  if ( tempst.compare(tempbst) == 0) {
	    frMax = j;
	  }
	  tempbst = firstch;
	  tempbst = tempbst.append(".mean");
	  if ( tempst.compare(tempbst) == 0) {
	    frMean = j;
	  }
	}
      }
    }
    delete fr; 
    in.close();
    return 0;
} 


/* Get info from a frame file name: Gps time, total length in seconds.
   return 1 (second trend), 60 ((minute trend), or 0 (count as raw data) 
   return -1 if not a Frame file
*/
int getFrameInfo(char* frfilename, int* frgps, int* frlength)
{
int j;
string tempst, tempst1;
int trend1 = 0, pos;

    ifstream in( frfilename );
    FrameCPP::Version_6::IFrameStream fin (in);
    FrameCPP::Version_6::FrameH* fr = fin.ReadNextFrame();
    *frgps = fr->getGTime().getSec();
    *frlength = (int)(fr->getDt());

    /* trend information */
    fr->getRawData()->refAdc().rehash();
    FrameCPP::Version_6::FrAdcData *adc;
    adc = fr->getRawData()->refAdc()[0];
    tempst = adc->getName();
    pos = tempst.find(".n");
    if ( pos > 0) { /* is trend */
      adc = fr->getRawData()->refAdc()[1];
      if ( adc->getSampleRate() > 0 && adc->getSampleRate() < 1 )
	trend1 = 60;
      else
	trend1 =1;
    }
    return trend1;
}
