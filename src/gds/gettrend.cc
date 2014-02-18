static const char *versionId = "Version $Id$" ;
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: gettrend						*/
/*                                                         		*/
/* Module Description: implements the MathLink interface for NDS	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#undef _CONFIG_DYNAMIC

#include <math.h>
#include <time.h>
#include <vector>
#include <string>
#include <strings.h>
#include <stdio.h>
#include <iostream>
#include "tconv.h"
#include "DAQSocket.hh"

//#define DEBUG
#define USE_N
#define _ONESEC			1000000000LL
#define __ONESEC		1E9
#define _KEEPALIVEINTERVAL	60
#define DAQD_PORT		8088

/* dataType Defintions */
#define DAQ_DATATYPE_16BIT_INT	1	/* Data type signed 16bit integer */
#define DAQ_DATATYPE_32BIT_INT	2	/* Data type signed 32bit integer */
#define DAQ_DATATYPE_64BIT_INT	3	/* Data type signed 64bit integer */
#define DAQ_DATATYPE_FLOAT	4	/* Data type 32bit floating point */
#define DAQ_DATATYPE_DOUBLE	5	/* Data type 64bit double float */
#define DAQ_DATATYPE_STRING	6	/* Data type 32 char string */
#define DAQ_DATATYPE_32BIT_UINT 7       /* Data type unsigned 32bit integer */

   using namespace std;


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Globals								*/
/*                                                         		*/
/*----------------------------------------------------------------------*/


   typedef vector<string> ndschnlist;
   
   DAQSocket 	nds;
   
   

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Utility functions							*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   char* strecpy (char* dest, const char* source)
   {
      strcpy (dest, source);
      return dest + strlen (dest);
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* NDS functions							*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

   bool connectNDS (const char* server, int port) 
   {
      nds.close();
   
      int 	status;
   
      if (port <= 0) {
         port = DAQD_PORT;
      }
   
      // connect to NDS
      status = nds.open (server, port);
      return (status == 0);
   }


   bool addChannelsNDS (ndschnlist& clist) 
   {   
      for (ndschnlist::iterator p = clist.begin(); p != clist.end(); ++p) {
         // get name      
         char	buf[1024];
#ifdef USE_N
         strcpy (strecpy (buf, p->c_str()), ".n");
         if (!nds.AddChannel (buf, DAQSocket::rate_bps_pair (1, 4))) {
            return false;
         }
#endif	 
         strcpy (strecpy (buf, p->c_str()), ".mean");
         if (!nds.AddChannel (buf, DAQSocket::rate_bps_pair (1, 8))) {
            return false;
         }
         strcpy (strecpy (buf, p->c_str()), ".min");
         if (!nds.AddChannel (buf, DAQSocket::rate_bps_pair (1, 4))) {
            return false;
         }
         strcpy (strecpy (buf, p->c_str()), ".max");
         if (!nds.AddChannel (buf, DAQSocket::rate_bps_pair (1, 4))) {
            return false;
         }
         strcpy (strecpy (buf, p->c_str()), ".rms");
         if (!nds.AddChannel (buf, DAQSocket::rate_bps_pair (1, 8))) {
            return false;
         }
      }
      return true;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Interface: NDSGetTrend						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

   bool ndsgettrendgps (const char* server, int port,
   		     ndschnlist& clist, double rate,
                     double gpssec, double gpsnsec,
                     double duration, double*& data, int& ndata)
   {
      // check parameters
      if (clist.empty()) {
         return false;
      }
      if (gpssec < 0) {
         return false;
      }
      if (rate <= 0) {
         return false;
      }
      if (duration <= 0) {
         return false;
      }
      // setup NDS
      if (!connectNDS (server, port)) {
         return false;
      }
      if (!addChannelsNDS (clist)) {
         nds.close();
         return false;
      }
   
      // determine decimation factor
      bool mintrend;
      int dec;
      int dt;
      rate = fabs (rate);
      if (rate <= 1./60.+1E-12) {
         mintrend = true;
         dec = (int) (1./60. / rate + 0.5);
         if (dec < 1) dec = 1;
         dt = 60 * dec;
      }
      else {
         mintrend = false;
         dec = (int) (1. / rate + 0.5);
         if (dec < 1) dec = 1;
         dt = dec;
      }
   
      // determine number of data points
      ndata = (int) duration / dt;
   
      // allocate memory for return
      data = new (nothrow) double [clist.size() * ndata * 6];
      if (data == 0) {
         clist.clear();
         nds.close();
         return false;
      }
      memset (data, 0, clist.size() * ndata * 6 * sizeof (double));
   
      // start NDS trend reader
      unsigned long start = (int) (gpssec + 0.5);
      if (mintrend) {
         //utc_t	utc;
         //TAItoUTC (start + 30, &utc);
         //utc.tm_sec = 0;
         //start = UTCtoTAI (&utc);
         start = 60 * ((start + 30) / 60);
      }      
   #ifdef DEBUG
      cout << "START is " << start << " for " << ndata*dt << endl;
   #endif
      if (nds.RequestTrend (start, ndata * dt, mintrend) != 0) {
         delete [] data;
         clist.clear();
         nds.close();
	 cerr << "Request failed" << endl;
         return false;
      }
      // wait for data
      char* buf = 0;
      int num = 0;
      while (nds.GetData (&buf) > 0) {
         DAQDRecHdr*	head = (DAQDRecHdr*) buf;
         char*		dptr = buf + sizeof (DAQDRecHdr);
      
      #ifdef DEBUG
         cout << "recv trend @ " << head->GPS << ":" << head->NSec <<
            " for " << head->Secs << " (len = " << head->Blen - 16 << 
            ", seq = " << head->SeqNum << ")" << endl;
      #endif
      
         // copy data into return array directly
         unsigned int index1;
         int index2;
#ifdef USE_N
         int* n;
#endif
         float* min;
         float* max;
         double* mean;
         double* rms;
         int dttrend = (mintrend ? 60 : 1);
#ifdef USE_N
	 const int adv = 5;
#else
	 const int adv = 4;
#endif
         for (DAQSocket::Channel_iter iter = nds.mChannel.begin();
             iter != nds.mChannel.end(); advance (iter, adv)) {
            // compute data pointers
            max = (float*) dptr;
            dptr += sizeof (float) * head->Secs / dttrend;
            mean = (double*) dptr;
            dptr += sizeof (double) * head->Secs / dttrend;
            min = (float*) dptr;
            dptr += sizeof (float) * head->Secs / dttrend;
#ifdef USE_N
            n = (int*) dptr;
            dptr += sizeof (int) * head->Secs / dttrend;
#endif
            rms = (double*) dptr;
            dptr += sizeof (double) * head->Secs / dttrend;
            // get 1st index
            string::size_type pos = iter->first.find_last_of (".");
            if (pos == string::npos) {
               break;
            }
            string name = iter->first.substr (0, pos);
            index1 = 0;
            for (ndschnlist::iterator it = clist.begin(); 
                it != clist.end(); it++, index1++) {
               if (strcasecmp (it->c_str(), name.c_str()) == 0) {
                  break;
               }
            }
            if (index1 >= clist.size()) {
               break;
            }
            // loop through result list
            for (int i = 0; i < head->Secs / dttrend; ++i) {
               // calulate index into result
               index2 = (head->GPS - start + i * dttrend) / dt;
               if ((index2 < 0) || (index2 >= ndata)) {
                  continue;
               }
               double* dest = data + index1 * (ndata * 6) + index2 * 6;
               // check if copy or add/min/max formulae
               if ((dec == 1) || (dest[1] == 0)) {
                  dest[4] = min[i];
                  dest[5] = max[i];
               }
               else {
                  dest[4] = std::min ((double)min[i], dest[4]);
                  dest[5] = std::max ((double)max[i], dest[5]);
               }
#ifdef USE_N
               dest[1] += n[i];
               dest[2] += mean[i]*n[i];
               dest[3] += rms[i]*rms[i]*n[i];
#else
               dest[1] += 1;
               dest[2] += mean[i];
               dest[3] += rms[i]*rms[i];
#endif
               ++num;
            } 
         }
         delete [] buf;
         buf = 0;
      }
      delete [] buf;
      nds.close();

      if (num == 0) {
	 cerr << "No data received" << endl;
         return false;
      }
         
      // normalize result and caluclate std dev instead of rms
      for (int j = 0; j < (int)clist.size(); j++) {
         for (int i = 0; i < ndata; i++) {
            double* dest = data + j * (ndata * 6) + i * 6;
            dest[0] = dt * i;
            if (dest[1] > 0) {
               dest[2] = dest[2] / dest[1];
            }
            if (dest[1] > 1.5) {
               dest[3] = sqrt (fabs(dest[1]/(dest[1]-1.) * 
                                   (dest[3]/dest[1] - dest[2]*dest[2])));
            }
            else {
               dest[3] = 0;
            }
         }
      }
   
      return true;
   }


   bool ndsgettrendutc (const char* server, int port,
    		     ndschnlist& channels, double rate,
                     int year, int month, int day,
                     int hour, int min, double sec, double duration,
		     double*& data, int& ndata)
   {
      double 		gpssec;
      double		gpsnsec;
      utc_t		utc;
      utc.tm_year = year - 1900;
      utc.tm_mon = month - 1;
      utc.tm_mday = day;
      utc.tm_hour = hour;
      utc.tm_min = min;
      utc.tm_sec = (int) sec;
      gpssec = (double) UTCtoTAI (&utc);
      gpsnsec = (sec - utc.tm_sec) * 1E9;
   
      /*printf ("t channels = %s\nrate = %g year=%i "
             "month=%i day=%i hour=%i min=%i, sec=%g duration=%g\n", 
             channels, rate, year, month, day, hour, min, sec, duration);*/
      return ndsgettrendgps (server, port, channels, rate, 
                             gpssec, gpsnsec, duration, data, ndata);
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Interface: WriteTrend						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

   bool WriteTrend (const double* data, int ndata, int cnum, 
                    const char* ofile, const char* fmt, bool binary)
   {
   
      FILE* out = 0;
      if (ofile && *ofile) {
         out = fopen (ofile, "w");
      }
      else {
         out = stdout;
      }
      if (!out) {
         return false;
      }
      
      for (int i = 0; i < ndata; ++i) {
         for (const char* p = fmt; *p; ++p) {
   	    for (int j = 0; j < cnum; ++j) {
               const double* y = data + j * (ndata * 6) + i * 6;
	       switch (*p) {
		  case 't':
		  case 'T':
        	     if (j == 0) {
		        if (binary) fwrite (y, sizeof(double), 1, out);
			else fprintf (out, "%15.12g ", y[0]);
	             }
		     break;
		  case 'n':
		  case 'N':
		     if (binary) fwrite (y+1, sizeof(double), 1, out);
        	     else fprintf (out, "%15.12g ", y[1]);
		     break;
		  case 'y':
		  case 'Y':
        	     if (binary) fwrite (y+2, sizeof(double), 1, out);
        	     else fprintf (out, "%18.12g ", y[2]);
		     break;
		  case 's':
		  case 'S':
        	     if (binary) fwrite (y+3, sizeof(double), 1, out);
        	     else fprintf (out, "%18.12g ", y[3]);
		     break;
		  case 'm':
        	     if (binary) fwrite (y+4, sizeof(double), 1, out);
        	     else fprintf (out, "%18.12g ", y[4]);
		     break;
		  case 'M':
        	     if (binary) fwrite (y+5, sizeof(double), 1, out);
        	     else fprintf (out, "%18.12g ", y[5]);
		     break;
	       }
	    }
	 }
	 if (!binary) fprintf (out, "\n");
      }
      fclose (out);
      
      return true;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Interface: Make histogram						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

   bool MakeHist (const double* data, int ndata, int cnum, 
                  double start, double stop, int N,
                  const char* ofile, const char* fmt, bool binary)
   {
      if ((N < 0) || (start == stop)) {
         return false;
      }
      FILE* out = 0;
      if (ofile && *ofile) {
         out = fopen (ofile, "w");
      }
      else {
         out = stdout;
      }
      if (!out) {
         return false;
      }

      // make histogram
      double dh = (stop - start) / (double)N;
      int num = 0;
      for (const char* p = fmt; *p; ++p) {
         if (strchr ("NnYySsMm", *p) != 0) num += cnum;
      }
      if (num <= 0) {
         fclose (out);
	 return false;
      }
      double** h = new double*[num];
      for (int i = 0; i < num; ++i) {
         h[i] = new double[N];
	 memset (h[i], 0, N*sizeof (double));
      }
      int hi = 0;
      for (const char* p = fmt; *p; ++p) {
         if (strchr ("NnYySsMm", *p) == 0) continue;
	 int ofs = 0;
         switch (*p) {
	    case 'n':
	    case 'N':
	       ofs = 1;
	       break;
	    case 'y':
	    case 'Y':
	       ofs = 2;
	       break;
	    case 's':
	    case 'S':
	       ofs = 3;
	       break;
	    case 'm':
	       ofs = 4;
	       break;
	    case 'M':
	       ofs = 5;
	       break;
	 }
   	 for (int j = 0; j < cnum; ++j) {
            for (int i = 0; i < ndata; ++i) {
               double y = data[j * (ndata * 6) + i * 6 + ofs];
	       int bin = (y - start) / dh;
	       if ((bin >= 0) && (bin < N)) ++h[hi][bin];
            }
  	    ++hi;
	 }
      }
      
      // write histogram
      bool wb = (strchr (fmt, 't') != 0);
      for (int j = 0; j < N; ++j) {
         double x = start + ((double)j + 0.5) * dh;
         if (wb) {
            if (binary) fwrite (&x, sizeof(double), 1, out);
            else fprintf (out, "%18.12g ", x);
	 }
         for (int i = 0; i < num; ++i) {
            if (binary) fwrite (h[i]+j, sizeof(double), 1, out);
            else fprintf (out, "%18.12g ", h[i][j]);
	 }
	 if (!binary) fprintf (out, "\n");
      }
      
      // cleanup
      for (int i = 0; i < num; ++i) {
         delete [] h[i];
      }
      delete [] h;
      return true;
   }
   
   
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Main program								*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

   int main (int argc, char* argv[])
   {
      int 		c;		/* option */
      extern char*	optarg;		/* option argument */
      extern int	optind;		/* option ind */
      int		errflag = 0;	/* error flag */
      char 		daqServer[256];
      int		daqPort = DAQD_PORT;
      ndschnlist 	clist;
      bool		binary = false;
      char 		format[11];
      string 		datestr;
      double		rate = 1./60.;
      unsigned long	gps;
      double		duration = 3600;
      string		hist;
      double		histStart = -1;
      double		histStop  = +1;
      int		histN = 100;
      string		ofile;
      
      strcpy (daqServer, "");
      strcpy (format, "ty");
      while ((c = getopt (argc, argv, "hs:p:bf:t:d:r:H:o:")) != EOF) {
         switch (c) {
            /* server address */
            case 's':
               {
                  strncpy (daqServer, optarg, sizeof(daqServer)-1);
                  daqServer[sizeof(daqServer)-1] = 0;
                  break;
               }
            /* server port */
            case 'p':
               {
                  daqPort = atoi (optarg);
                  break;
               }
             /* binary? */
            case 'b':
               {
                  binary = true;
                  break;
               }
            /* output format */
            case 'f':
               {
                  strncpy (format, optarg, sizeof(format)-1);
                  format[sizeof(format)-1] = 0;
                  break;
               }
            /* histogram format */
            case 'H':
               {
	          hist = optarg;
		  if (sscanf (optarg, "%lf,%lf,%d", &histStart, 
		              &histStop, &histN) != 3) {
		     cerr << "Illegal histogram option" << endl;
		     errflag = 1;
		  }
                  break;
               }
            /* start time */
            case 't':
               {
                  datestr = optarg;
		  if (strchr (optarg, '/') == 0) {
		     gps = atoi (optarg);
		  }
		  else {
                     utc_t		utc;
		     strptime (optarg, "%D,%T", &utc);
		     gps = UTCtoTAI (&utc);
		     //cout << "GPS = " << gps << endl;
		  }
		  if (gps <= 0) {
		     cerr << "Illegal start time option" << endl;
		     errflag = 1;
		  }
                  break;
               }
            /* duration */
            case 'd':
               {
                  duration = atof (optarg);
                  break;
               }
            /* data interval */
            case 'r':
               {
                  double dt = atof (optarg);
		  rate = (dt >= 1.0) ? 1./dt : 1.;
                  break;
               }
            /* output filename */
            case 'o':
               {
	          ofile = optarg;
                  break;
               }
            /* help */
            case 'h':
            case '?':
               {
                  errflag = 1;
                  break;
               }
         }
      }
      for (int i = optind; i < argc; ++i) {
         clist.push_back (argv[i]);
      }
      if (clist.empty()) {
         errflag = 1;
      }
	             
      /* help */
      if (errflag) {
         printf ("Usage: gettrend [options] {'channel name'}\n"
                "       -s 'addr' : NDS server address\n"
                "       -p 'port' : NDS server port (default 8088)\n"
                "       -f 'frmt' : output format string (default ty)\n"
		"                   t - time\n"
		"                   y - mean value\n"
		"                   s - stdandard deviation\n"
		"                   n - number of points\n"
		"                   M - maximum\n"
		"                   m - minimum\n"
		"                   all but t are repeated for each channel\n"
		"       -t 'time' : start time (gps or utc)\n"
		"                   utc format: MM/DD/YY,hh:mm:ss\n"
		"       -d 'dur'  : duartion in seconds (default is 3600)\n"
		"       -r 'intv' : data time interval (default 60)\n"
                "       -H 'hist' : histogram format\n"
		"                   start,stop,N\n"
                "       -b : binary output format (default ASCII)\n"
                "       -o 'file' : output file (default terminal)\n"
                "       -h : help\n");
         return 1;
      }
      
      double* data = 0;
      int ndata = 0;
      if (!ndsgettrendgps (daqServer, daqPort, clist,
   		     rate, gps, 0, duration, data, ndata)) {
	 cerr << "Error while obtaining trends" << endl;
         return 1;
      }
      //cout << "Channel N = " << clist.size() << "  ndata = " << ndata << endl;

      if (hist.size() > 0) {
         if (!MakeHist (data, ndata, clist.size(), histStart, histStop,
	                histN, ofile.c_str(), format, binary)) {
	    cerr << "Error while making histogram" << endl;
            delete [] data;
	    return 1;
	 }
      }
      else {
         if (!WriteTrend (data, ndata, clist.size(), ofile.c_str(), 
	                  format, binary)) {
	    cerr << "Error while writing trends" << endl;
            delete [] data;
	    return 1;
	 }
      }
      
      delete [] data;
      return 0;
   }

