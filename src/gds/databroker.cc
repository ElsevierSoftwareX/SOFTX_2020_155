static char *versionId = "Version $Id$" ;
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: databroker						*/
/*                                                         		*/
/* Module Description: reads in channel data				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

//#define DEBUG

// Header File List:
#include <time.h>
#include "dtt/databroker.hh"
#include <iostream>
#include <algorithm>
#include <stdio.h>

namespace diag {
   using namespace std;
   using namespace thread;


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Constants: __ONESEC		  one second (in nsec)			*/
/*            _MIN_NDS_DELAY	  minimum delay allowed for NDS (sec)	*/
/*            _MAX_NDS_DELAY	  maximum delay allowed for NDS (sec)	*/
/*            _NDS_DELAY	  NDS delay for slow data (sec)		*/
/*            taskNdsName	  nds task priority			*/
/*            taskNdsPriority	  nds task name				*/
/*            taskNdsUpdateName	  nds update task priority		*/
/*            taskNdsUpdatePriority nds update task name		*/
/*            taskCleanupName	  nds clenaup task priority		*/
/*            taskCleanupPriority nds cleanup task name			*/
/*            decimationflag	  flags used for decimation filter	*/
/*            daqBufLen		  length of receiving socket buffer	*/
/*            								*/
/*----------------------------------------------------------------------*/
#define _CHNLIST_SIZE		200
#define __ONESEC		1E9
#define _PREPROC_STARTUP	10
#define _PREPROC_CONTINUE	2
#define _MIN_NDS_DELAY		0.0
#define _MAX_NDS_DELAY		1.0
#define _NDS_DELAY		1

   const char	taskCleanupName[] = "tNDScleanup";
   const int	taskCleanupPriority = 20;
   const int 	daqBufLen = 1024*1024;
   const long	taskNdsGetDataTimeout = 10;


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Class Name: dataBroker						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int dataBroker::cleanuptask (dataBroker& RTDDMgr) 
   {
      const struct timespec delay = {1, 0};
   #ifndef OS_VXWORKS
      int 		oldtype;
      pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);
   #endif
   
      while (true) {
         nanosleep (&delay, 0);
         // cerr << "rtdd cleanup start" << endl;
         RTDDMgr.cleanup ();
         // cerr << "rtdd cleanup stop" << endl;
      }
      return 0;
   }


   dataBroker::dataBroker (gdsStorage* dat, 
                     testpointMgr* TPMgr, double Lazytime) 
   : storage (dat), tpMgr (TPMgr), lazytime (Lazytime), cleartime (0), 
   nexttimestamp (0), starttime (0), stoptime (0), TID (0), cleanTID (0)
   {
      semlock		lockit (mux);	// lock mutex */
   
      // start cleanup task
      if (lazytime > 0) {
         int		attr;	// task create attribute
      #ifdef OS_VXWORKS
         attr = VX_FP_TASK;
      #else
         attr = PTHREAD_CREATE_DETACHED | PTHREAD_SCOPE_PROCESS;
      #endif
         taskCreate (attr, taskCleanupPriority, &cleanTID, 
                    taskCleanupName, (taskfunc_t) cleanuptask, 
                    (taskarg_t) this);
      }
   }


   dataBroker::~dataBroker () 
   {
      // cancel cleanup task
      mux.lock();
      taskCancel (&cleanTID);
      mux.unlock();
      // delete all test points
      del();
   }


   bool dataBroker::init (gdsStorage* dat)
   {
      semlock		lockit (mux);
   
      if (storage == 0) {
         remove (false);
      }
      storage = dat;
      return true;
   }


   bool dataBroker::init (testpointMgr* TPMgr)
   {
      semlock		lockit (mux);
   
      if (TPMgr == 0) {
         remove (false);
      }
      tpMgr = TPMgr;
      return true;
   }


   bool dataBroker::areSet () const
   {
      semlock		lockit (mux);	// lock mutex */
   
      for (channellist::const_iterator iter = channels.begin();
          iter != channels.end(); iter++) {
         // if not set return false
         if (!iter->isSet()) {
            return false;
         }
      }
      // all set
      return true;
   }


   bool dataBroker::areUsed () const
   {
      semlock		lockit (mux);	// lock mutex */
   
      for (channellist::const_iterator iter = channels.begin();
          iter != channels.end(); iter++) {
         // if not used return false
         if (iter->inUseCount() <= 0) {
            return false;
         }
      }
      // all set
      return true;
   }


   bool dataBroker::busy () const
   {
      semlock		lockit (mux);	// lock mutex */
      return (TID != 0);
   }


   bool dataBroker::clear (bool lazy)
   {
      // mark only if lazy clear
      if (lazy) {
         semlock		lockit (mux);
         cleartime = (double) TAInow() / (double) _ONESEC;
      }
      else {
         // stop nds
         //cerr << "stop nds" << endl;
         dataStop ();
         semlock		lockit (mux);
         del();
         cleartime = 0;
      }
      return true;
   }


   bool dataBroker::add (const string& name, int* inUseCount)
   {
      semlock		lockit (mux);
   
      string		chnname (channelName (name));
      gdsChnInfo_t	info;
   
      // test if valid channel
      if (!channelInfo (name, info)) {
         return false;
      }
      dataChannel	chn (chnname, *storage, 
                      info.dataRate, info.dataType);
   
      return add (chn, inUseCount);
   }


   bool dataBroker::add (const dataChannel& chn, int* inUseCount)
   {
      semlock		lockit (mux);
   
      // test if already in list
      channellist::iterator iter = find (chn.getChnName());
      if ((iter != channels.end()) && (*iter == chn)) {
         // increase in use count
         iter->useCount (true);
         if (inUseCount != 0) {
            *inUseCount = iter->inUseCount();
         }
         // set test point if required
         if ((iter->inUseCount() == 1) && (iter->isTestpoint()) && 
            (tpMgr != 0)) {
            tpMgr->add (iter->getChnName());
         }
      }
      else {
         // add to channel list
         iter = channels.insert (iter, chn);
         iter->inUseSet (1);
         if (inUseCount != 0) {
            *inUseCount = 1;
         }
         // add to test point list
         iter->setTestpoint ((tpMgr == 0) ? 
                            false : tpMgr->add (chn.getChnName()));
      }
   
      return true;
   }


   bool dataBroker::add (const string& name, 
                     const dataChannel::partitionlist& partitions,
                     bool useActiveTime)
   {
      semlock		lockit (mux);
      string		chnname (channelName (name));
   
      // test if already in list
      channellist::iterator iter = find (chnname);
      // add channel if not in list
      if ((iter == channels.end()) || (*iter != chnname)) {
         int		temp;
         if (!add (chnname, &temp)) {
            return false;
         }
         iter = find (chnname); 
         if ((iter == channels.end()) || (*iter != chnname)) {
            return false;
         }
      }
   
      // add partitions to channel object
      iter->addPartitions (partitions, useActiveTime);
   
      return true;
   }


   bool dataBroker::add (const string& name, 
                     int Decimate1, int Decimate2, 
                     tainsec_t Zoomstart, double Zoomfreq, 
                     bool rmvDelay)
   {
      semlock		lockit (mux);
      string		chnname (channelName (name));
   
      // test if already in list
      channellist::iterator iter = find (chnname);
      // add channel if not in list
      if ((iter == channels.end()) || (*iter != chnname)) {
         if (!add (chnname, 0)) {
            return false;
         }
         iter = find (chnname); 
         if ((iter == channels.end()) || (*iter != chnname)) {
            return false;
         }
      }
   
      // add partitions to channel object
      iter->addPreprocessing (Decimate1, Decimate2, Zoomstart, Zoomfreq,
                           rmvDelay);
   
      return true;
   }


   bool dataBroker::del (const string& chnname)
   {
      semlock		lockit (mux);	// lock mutex
   
      // test if in list
      channellist::iterator iter = find (chnname);
      if ((iter == channels.end()) || (*iter != chnname)) {
         // not found
         return false;
      }
   
      // check inUse
      iter->useCount (false);
      if (iter->inUseCount() <= 0) {
         // remove test point if in use count reaches zero
         if ((iter->isTestpoint()) && (tpMgr != 0)) {
            tpMgr->del (iter->getChnName());
         }
         // remove from list if not lazy clear
         if (cleartime == 0) {
            channels.erase (iter);
         }
      }
   
      return true;
   }


   bool dataBroker::del ()
   {
      // clear all channels
      // clear (false);
      semlock		lockit (mux);	// lock mutex
      // loop through channel list and delete test points
      for (channellist::iterator iter = channels.begin();
          iter != channels.end(); iter++) {
         if ((iter->isTestpoint()) && (tpMgr != 0)) {
            tpMgr->del (iter->getChnName());
         }
      }
      channels.clear();
      return true;
   }


   bool dataBroker::reset (const string& chnname)
   {
      semlock		lockit (mux);	// lock mutex
   
      // test if in list
      channellist::iterator iter = find (chnname);
      if ((iter == channels.end()) || (*iter != chnname)) {
         // not found
         return false;
      }
   
      // reset
      iter->reset();
      return true;
   }


   bool dataBroker::reset ()
   {
      semlock		lockit (mux);	// lock mutex
      // loop through channel list and reset
      for (channellist::iterator iter = channels.begin();
          iter != channels.end(); iter++) {
         iter->reset();
      }
      return true;
   }


   tainsec_t dataBroker::timeStamp () const
   {
      tainsec_t		timestamp = -1;
      semlock		lockit (mux);	// lock mutex
      for (channellist::const_iterator iter = channels.begin();
          iter != channels.end(); iter++) {
         if (timestamp == -1) {
            timestamp = iter->timeStamp();
         }
         else {
            timestamp = min (timestamp, iter->timeStamp());
         }
      }
      return timestamp;
   }


   tainsec_t dataBroker::maxDelay () const
   {
      tainsec_t		delay = 0;
      semlock		lockit (mux);	// lock mutex
   
      for (channellist::const_iterator iter = channels.begin();
          iter != channels.end(); iter++) {
         delay = max (delay, iter->maxDelay());
      }
      return delay;
   }


   dataBroker::channellist::const_iterator dataBroker::find (
                     const string& name) const
   {
      semlock		lockit (mux);	// lock mutex */
      dataChannel	chn (channelName (name), *storage, 0, 0);
   
      return lower_bound (channels.begin(), channels.end(), chn);
   }

   dataBroker::channellist::iterator dataBroker::find (
                     const string& name)
   {
      semlock		lockit (mux);	// lock mutex */
      dataChannel	chn (channelName (name), *storage, 0, 0);
   
      return lower_bound (channels.begin(), channels.end(), chn);
   }


   bool dataBroker::dataCheckEnd ()
   {
      cerr << "dataCheckEnd: stop = " << stoptime << endl;
      if (stoptime == 0) {
         return true;
      }
      cerr << "dataCheckEnd: expected = " << nexttimestamp << endl;
      if (fabs ((double)(stoptime - nexttimestamp)/__ONESEC) < 1E-6) {
         return true;
      }
      else {
         // notify channels on error
         cerr << "dataCheckEnd: skip data" << endl;
         for (channellist::iterator iter = channels.begin();
             iter != channels.end(); iter++) {
            iter->skip (stoptime);
         }
         return false;
      }
   }


   void dataBroker::cleanup ()
   {
      semlock		lockit (mux);	// lock mutex */
   
      // check time
      if ((lazytime > 0) && (cleartime > 0) &&
         ((double) TAInow() / _ONESEC > cleartime + lazytime)) {
         cleartime = 0;
         // stop nds
         mux.unlock();
         dataStop ();
         mux.lock();
         // cleanup
         //cerr << "CLEAN RTDD" << endl;
         del();
      }
   }


}
