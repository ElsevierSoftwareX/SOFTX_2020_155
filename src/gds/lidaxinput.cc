/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: lidaxinput						*/
/*                                                         		*/
/* Module Description: reads in channel data through LiDaX		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

//#define DEBUG

// Header File List:
#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <iostream>
#include "dtt/lidaxinput.hh"
#include "dtt/diagnames.h"
#include "dtt/diagdatum.hh"
#include "framefast/framefast.hh"


namespace diag {
   using namespace std;
   using namespace dfm;
   using namespace thread;


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Constants: __ONESEC		  one second (in nsec)			*/
/*            taskLidaxName	  nds task priority			*/
/*            taskLidaxPriority	  nds task name				*/
/*            decimationflag	  flags used for decimation filter	*/
/*            								*/
/*----------------------------------------------------------------------*/
#define __ONESEC		1E9

   const char	taskLidaxName[] = "tLidax";
   const int	taskLidaxPriority = 1;
   const double epsilon = 1E-7;
   const unsigned int	frameFormatThreshold = 31;
   const char	frameFormat1[] = "FF1N1C0";
   const char	frameFormat2[] = "FF16N1C0";

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Class Name: lidaxManager						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int lidaxManager::ldxtask (lidaxManager& ldxMgr) 
   {
      // int		err;
   
      // process data
      cerr << "PROCESS LIDAX REQUEST..." << endl;
      pthread_setcancelstate (PTHREAD_CANCEL_DISABLE, 0);
      // bool success = ldxMgr.lidax.processAll();
      // cerr << "PROCESS LIDAX REQUEST " << 
         // (success ? "success" : "failed") << endl;
      // ldxMgr.ldxmux.lock();
      // ldxMgr.TID = 0;
      // ldxMgr.ldxmux.unlock();
      // return success ? 0 : -1;
   
      //start processing frames
      const timespec tick = {0, 1000000}; // 1ms
      dataaccess&	ldx = ldxMgr.lidax;
      ldxMgr.ldxmux.lock();
      // Time T0 = ldx.sel().selectedTime();
      Interval dT = ldx.sel().selectedDuration();
      Time T1 = ldx.sel().selectedStop() - Interval (epsilon);
      ldxMgr.ldxmux.unlock();
      if ((double)dT <= 0) {
         ldxMgr.TID = 0;
         return 0;
      }
      Time t;// = ldx.processTime();
      do {
         // get the mutex
         while (!ldxMgr.ldxmux.trylock()) {
            pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, 0);
            nanosleep (&tick, 0);
            pthread_testcancel();
            pthread_setcancelstate (PTHREAD_CANCEL_DISABLE, 0);
         }
         // process next frame
         if (ldx.process() <= Interval (0.)) {
            ldx.flush();
            timespec wait = {5, 0}; // wait till frames have been processed
            nanosleep (&wait, 0);
            ldxMgr.dataCheckEnd();
            ldxMgr.ldxmux.unlock();
            break; // incomplete!
         }
         t = ldx.processTime();
         // end of processing?
         if (t >= T1) {
            ldx.flush();
         }
         ldxMgr.ldxmux.unlock();
         pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, 0);
         pthread_testcancel();
         pthread_setcancelstate (PTHREAD_CANCEL_DISABLE, 0);
      } 
      while (t < T1);
      ldxMgr.TID = 0;
      //ldxMgr.lidax.abort();
      return 0;
   }


//______________________________________________________________________________
   lidaxManager::lidaxManager (gdsStorage* dat, double Lazytime) 
   : dataBroker (dat, 0, Lazytime), fAbort (false),
   lidax (dataaccess::kSuppAll)
   {
      // no shared memory or tape support
      lidax.support (dfm::st_SM, false);
      lidax.support (dfm::st_Tape, false);
      // add support for function callback
      lidax.support (dfm::st_Func, true);
      lidax.setAbort (&fAbort);
   }

//______________________________________________________________________________
   lidaxManager::~lidaxManager() 
   {
      dataStop();
   }

//______________________________________________________________________________
   static string lidax_parameter (const char* name, int num, 
                     int i = -1)
   {
      char buf[1024];
      if (i < 0) {
         sprintf (buf, "%s.%s[%i]", stLidax, name, num);
      }
      else {
         sprintf (buf, "%s.%s[%i][%i]", stLidax, name, num, i);
      }
      return string (buf);
   }

//______________________________________________________________________________
   bool lidaxManager::setup()
   {
      dataStop();
      semlock		lockit (mux);	// lock mutex */
      // Clear old selection
      lidax.clear (false);
      if (!storage) {
         return false;
      }
      diagStorage* gds = dynamic_cast<diagStorage*> (storage);
      if (!gds) {
         return false;
      }
      // Read source parameters
      int success = 0;
      for (int num = 0; ; ++num) {
         // server
         string var = lidax_parameter (stLidaxServer, num);
         string server;
         if (!gds->get (var, server, 0)) {
            if (num == 0) {
               return false;
            }
            else {
               break;
            }
         }
         // UDN
         var = lidax_parameter (stLidaxUDN, num);
         string udn;
         if (!gds->get (var, udn, 0)) {
            continue;
         }
         // Channel name and rate
         fantom::channellist chns;
         int gap = 0;
         for (int i = 0; gap < 20; ++i) {
            // Channel name
            var = lidax_parameter (stLidaxChannel, num, i);
            string cname;
            if (!gds->get (var, cname, 0)) {
               ++gap;
               continue;
            }
            // Rate
            var = lidax_parameter (stLidaxRate, num, i);
            string val;
            float rate = 0;
            if (gds->get (var, val, 0)) {
               rate = atof (val.c_str());
            }
            // Add channel to list
            chns.push_back (fantom::channelentry (cname.c_str(), rate));
         }
         // check server
         if (server.empty() || udn.empty()) {
            continue;
         }
         if (lidax.addServer (server, udn, chns)) {
            ++success;
         }
      }
      lidax.getInputChannelList (chnList);
      cerr << "LiDaX channel list length " << chnList.size() << endl;
   
      return (success > 0);
   }

//______________________________________________________________________________
   bool lidaxManager::set (taisec_t start, taisec_t duration)
   {
      // make sure lidax is stopped
      if (!dataStop ()) {
         return false;
      }
      semlock		lockit (mux);	// lock mutex */
      cerr << "TIME STAMP BEFORE START = " << timeStamp() << endl;
      cleartime = 0;
      fAbort = false;
   
      // set time
      if (!lidax.sel().selectTime (Time (start, 0), 
                           Interval (duration), &lidax.list())) {
         cerr << "FAILED TO SET TIME = " << start << ":" << duration << endl;
         return false;
      }
      // set destination client
      lidax.dest().clear();
      dataservername dname (st_Func, "");
      char udn[1024];
      sprintf (udn, "func://%p -d %p", (void*)&dfm_callback, this);
      fantom::channellist clist;
      for (channellist::iterator iter = channels.begin();
          iter != channels.end(); iter++) {
         clist.push_back (fantom::channelentry (iter->getChnName(),
                                           iter->getDatarate()));
      }
      const char* ff = (duration > frameFormatThreshold) ?
         frameFormat2 : frameFormat1;
      if (!lidax.addClient ((const char*)dname, string (udn), clist, ff)) {
         cerr << "FAILED TO ADD CLIENT = " << udn << endl;
         return false;
      }
   
      // send lidax request
      cerr << "lidax send request" << endl;
      if (!lidax.request()) {
         cerr << "FAILED TO SEND REQUEST" << endl;
         return false;
      }
      cerr << "lidax send request done" << endl;
   
      // all set: start fantom
      cerr << "start Lidax @ " << start << ":" << duration << endl;
      if (!dataStart (start, duration)) {
         return false;
      }
   
      cerr << "start Lidax @ " << start << ":" << duration << " done" << endl;
      return true;
   }

//______________________________________________________________________________
   bool lidaxManager::channelInfo (const string& name, 
                     gdsChnInfo_t& info) const
   {
      fantom::const_chniter chn = 
         fantom::FindChannelConst (chnList, name.c_str());
      memset (&info, 0, sizeof (gdsChnInfo_t));
      if (chn == chnList.end()) {
         return false;
      }
      else {
         strncpy (info.chName, chn->Name(), sizeof (info.chName)-1);
         info.chName[sizeof(info.chName)-1] = 0;
         info.dataRate = (int)chn->Rate();
         return true;
      }
   }

//______________________________________________________________________________
   string lidaxManager::id (gdsStorage& storage)
   {
      string s;
      diagStorage* gds = dynamic_cast<diagStorage*>(&storage);
      if (!gds) {
         return s;
      }
      // Read source parameters
      for (int num = 0; ; ++num) {
         // server
         string var = lidax_parameter (stLidaxServer, num);
         string server;
         if (!gds->get (var, server, 0)) {
            if (num == 0) {
               return s;
            }
            else {
               break;
            }
         }
         // UDN
         var = lidax_parameter (stLidaxUDN, num);
         string udn;
         if (!gds->get (var, udn, 0)) {
            continue;
         }
         if (!s.empty()) s += " ";
         s += server + "(" + udn + ")";
      }
      return s;
   }


//______________________________________________________________________________
   tainsec_t lidaxManager::timeoutValue (bool online) const
   {
      // wait till error or end of data
      return TID ? 0 : 10 * _ONESEC;
   }

//______________________________________________________________________________
   void lidaxManager::clearCache()
   {
      dfm::dataaccess::ClearCache();
   }

//______________________________________________________________________________
   bool lidaxManager::dataStart (taisec_t start, taisec_t duration)
   {
      // check if already running 
      if (TID != 0) {
         return true;
      }
      // set last time
      nexttimestamp = start * _ONESEC;
      starttime = start * _ONESEC;
      stoptime = (start + duration) * _ONESEC;
      lasttime = TAInow();
   
      // create lidax task
      if (taskCreate (PTHREAD_CREATE_DETACHED, taskLidaxPriority, &TID, 
                     taskLidaxName, (taskfunc_t) ldxtask, 
                     (taskarg_t) this) != 0) {
         lidax.abort();
         cerr << "lidax: error during task spawn" << endl;
         return false;
      }
      cerr << "lidax started" << endl;
   
      return true;
   }

//______________________________________________________________________________
   bool lidaxManager::dataStop()
   {
      lidax.abort();
   
      const timespec tick = {0, 100000000}; // 100ms
      cerr << "kill lidax task: get mutex" << endl;
      // get the mutex
      int n = 100;
      while ((n >= 0) && !ldxmux.trylock()) {
         nanosleep (&tick, 0);
         n--;
      	 // send a signal to unblock wait
         if (n % 10 == 9) {
            taskID_t tid = TID;
            if (tid) pthread_kill (tid, SIGCONT);
         }
      }
      if (n < 0) {
         return false;
      }
      //ndsmux.lock();
      if (TID != 0) {
         cerr << "kill lidax task" << endl;
         taskCancel (&TID);
         TID = 0;
         cerr << "killed lidax task" << endl;
      }
      ldxmux.unlock();
   
      lidax.done();
      return true;
   }

//______________________________________________________________________________
   bool lidaxManager::dfm_callback (const char* frame, int len,
                     lidaxManager* ldx)
   {
      return ldx->callback (frame, len);
   }

//______________________________________________________________________________
   bool lidaxManager::callback (const char* frame, int len)
   {
      semlock lockit (mux);
      framefast::framereader fr;
      fr.loadFrame (frame, len);
      cerr << "RECEIVED FRAME CALLBACK " << fr.starttime() <<
         " (" << len << "B)" << endl;
      const framefast::toc_t* toc = fr.getTOC();
      if (!toc) {
         return false;
      }
   
      // loop through frames
      framefast::data_t data;
      for (int k = 0; k < fr.nframe(); ++k) {
         // get frame start time
         Time Tbeg (starttime / _ONESEC, starttime % _ONESEC);
         Time Tend (stoptime / _ONESEC, stoptime % _ONESEC);
         Time start = fr.starttime (k);
         Interval dur = fr.duration (k);
         Time stop = start + dur;
         // skip at the beginning
         double skipb = 0; // as fraction of the duration
         if (start + 1E-8 < Tbeg) {
            skipb = (double)(Tbeg - start) / (double)dur;
            start = Tbeg;
            if (skipb >= 1.0) { // skip all?
               continue;
            }
         }
         // compute start time of data
         taisec_t time = start.getS();
         int epoch = (start.getN() + _EPOCH/10) / _EPOCH;
         // skip at the end?
         double skipe = 0; // as fraction of the duration
         if (stop > Tend + 1E-8) {
            skipe = (double)(stop - Tend) / (double)dur;
            if (skipb + skipe >= 1.0) {
               continue;
            }
         }
         if ((skipb != 0) || (skipe != 0)) {
            cerr << "Skip beg = " << skipb << " end = " << skipe << endl;
         }
         // loop through ADC, Proc, Sim, etc.
         for (int i = 0; i < 5; ++i) {
            // loop through toc entries
            for (int j = 0; j < (int)toc->fNData[i]; ++j) {
               // find daq channel
               const framefast::toc_data_t* toc_entry = 
                  toc->fData[i] + j;
               channellist::iterator chn = find (toc_entry->fName);
               if ((chn == channels.end()) || 
                  (*chn != toc_entry->fName)) {
                  continue;
               }
               // get data and check sampling rate
               if (!fr.getData (data, toc_entry->fPosition[k], 
                               (framefast::datatype_t)i,
                               framefast::frvect_t::fv_original) ||
                  (chn->getDatarate() != (int)data.fADC.fSampleRate)) {
                  continue;
               }
               // convert to float
               int n = data.fVect.fNData;
               float* fptr = new (nothrow) float[n];
               if (!fptr || (data.fVect.get (fptr, n) != n)) {
                  delete [] fptr;
                  continue;
               }
               // skip indices
               int ofs = (int)(skipb * (double)n + 0.5);
               int len = (int)((1.0 - skipb - skipe) * (double)n + 0.5);
               if (ofs || (len != n)) {
                  cerr << "Data Offset = " << ofs << " Length = " << len << endl;
               }
               // invoke callback
               int err = 0;
               chn->callback (time, epoch, fptr + ofs, len, err);
               delete [] fptr;
            }
         }
      }
      Time nexttime = fr.nexttime();
      tainsec_t next = nexttime.getS() * _ONESEC + nexttime.getN();
      nexttimestamp = (next < stoptime) ? next : stoptime;
      lasttime = TAInow();
      const struct timespec 	tick = {0, 10000000}; // 10ms
      nanosleep (&tick, 0);
      return true;
   }



}
