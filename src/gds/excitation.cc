static const char *versionId = "Version $Id$" ;
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: excitation						*/
/*                                                         		*/
/* Module Description: implements a excitation channel management	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

/* Header File List: */
#include <time.h>
#include <unistd.h>
#include "dtt/gdsutil.h"
#include "dtt/excitation.hh"
#include "dtt/testpoint.h"
#include "dtt/testpointinfo.h"
#include "dtt/rmorg.h"
#include "dtt/awgfunc.h"
#include "dtt/epics.h"
#include "dtt/gdsdatum.hh"
#include "dtt/testpointmgr.hh"
#include "FilterDesign.hh"
#include "iirutil.hh"
#include <iostream>

namespace diag {
   using namespace std;
   using namespace thread;


   const double excitation::syncDelay = 0.20; // sec
   const double excitation::syncUncertainty = 0.25; // sec
   const double excitation::linkSpeed[2] = 
   {500E3, 900.0}; // char/sec

   excitation::excitation (const string& Chnname, double Wait)
   : chnname (""), channeltype (invalid), writeaccess (false), 
   wait (Wait), slot (-1), inUse (1), isTP (false)
   {
      setup (Chnname);
   }


   excitation::excitation (const excitation& exc)
   {
      *this = exc;
   }


   excitation::~excitation ()
   {
      reset (true, _ONESEC);
   }


   excitation& excitation::operator= (const excitation& exc)
   {
      if (this != &exc) {
         semlock		lockit (mux);
         semlock		lockit2 (exc.mux);
         chnname = exc.chnname;
         channeltype = exc.channeltype;
         writeaccess = exc.writeaccess;
         chninfo = exc.chninfo;
         wait = exc.wait;
         signals = exc.signals;
         points = exc.points;
         slot = exc.slot;
         epicsvalue = exc.epicsvalue;
         /* transfer owner ship of channel */
         exc.inUse = 0;
         exc.slot = -1;
      }
      return *this;
   }


   int excitation::capability (capabilityflag cap) const
   {
      semlock		lockit (mux);
      if (channeltype == invalid) {
         return 0;
      }
      switch (cap) {
         case output:
            return (int) writeaccess;
         case GPSsync:
            return (int) ((channeltype == testpoint) || 
                         (channeltype == DAC));
         case periodicsignal:
         case randomsignal:
            return (int) ((channeltype == testpoint) || 
                         (channeltype == DAC) ||
                         (channeltype == DSG));
         case waveform:
            return (int) ((channeltype == testpoint) || 
                         (channeltype == DAC) || 
                         (channeltype == DSG));
         case multiplewaveforms:
            return (int) ((channeltype == testpoint) || 
                         (channeltype == DAC)); 
         default:
            return 0;
      }
   }


   bool excitation::setup (const string& Chnname)
   {
      semlock		lockit (mux);
   
      if ((channeltype != invalid) && (slot >= 0)) {
         awgRemoveChannel (slot);
      }
      channeltype = invalid;
      if (gdsChannelInfo (Chnname.c_str(), &chninfo) < 0) {
         // not a nds channel: check epics
         if (epicsGet (Chnname.c_str(), NULL) != 0) {
            // not a channel
            return false;
         }
         chninfo.dataRate = 1;
      }
      chnname = Chnname;
      slot = -1;
   
      // EPICS channel?
      if (chninfo.dataRate < 16) {
         if (epicsGet (chnname.c_str(), &epicsvalue) != 0) {
            // not accessible	
            return false;
         }
         channeltype = EPICS;
         writeaccess = true;
         slot = 0;
         return true;
      }
      // test point channel?
      else if (tpIsValid (&chninfo, 0, 0)) {
         channeltype = testpoint;
         writeaccess = true;
         slot = awgSetChannel (chnname.c_str());
         return (slot >= 0);
      }
      // not a valid channel
      else {
         return false;
      }
   }


   bool excitation::add (const AWG_Component& comp)
   {
      semlock		lockit (mux);
   
      if ((channeltype == invalid) || 
         (!capability (output))) {
         return false;
      }
      /* check for valid waveform component */
      if (comp.wtype == awgNone) {
         return true;
      }
      if (awgIsValidComponent (&comp) == 0) {
         return false;
      }
      if (!capability (multiplewaveforms) &&
         (signals.size() > 0)) {
         return false;
      }
      if (!capability (periodicsignal) && 
         ((comp.wtype == awgSine) ||
         (comp.wtype == awgSquare) ||
         (comp.wtype == awgRamp) ||
         (comp.wtype == awgTriangle) ||
         (comp.wtype == awgImpulse))) {
         return false;
      }
      if (!capability (randomsignal) && 
         ((comp.wtype == awgNoiseN) ||
         (comp.wtype == awgNoiseU))) {
         return false;
      }
      /* make sure valid parameters for non-AWG devices */
      AWG_Component 	c (comp);
      if (!capability (waveform)) {
         /* advanced features */
         c.duration = -1;
         c.restart = -1;
         c.ramptype = RAMP_TYPE (AWG_PHASING_STEP, 
                              AWG_PHASING_STEP, AWG_PHASING_STEP);
         c.ramptime[0] = 0;
         c.ramptime[1] = 0;	
      }
      /* add waveform */
      signals.push_back (c);
      return true;
   }


   bool excitation::add (const pointlist& Points)
   {
      points = Points;
      return true;
   }


   bool excitation::add (const_sigiterator begin, 
                     const_sigiterator end) 
   {
      for (const_sigiterator iter = begin; iter != end; iter++) {
         if (!add (*iter)) {
            return false;
         }
      }
      return true;
   }


   double excitation::dwellTime () const
   {
      double dwell = wait;
   
      /* add uncertainly for no GPS units */
      if (capability (GPSsync)) {
         dwell += syncDelay;
      }
      else {
         dwell += syncUncertainty;
      }
      /* estimate download time for arbitrary waveform */
      if (capability (waveform) && !points.empty()) {
         switch (channeltype) {
            case DSG : 
               dwell += sizeof (short) * points.size() / linkSpeed[2]; 
               break;
            case testpoint:
            case DAC:
               dwell += sizeof (float) * points.size() / linkSpeed[1];
               break;
            default:
               break;
         }
      }
      return dwell;
   }


   bool excitation::start (tainsec_t start, tainsec_t timeout)
   {
      semlock		lockit (mux);
   
      if ((channeltype == invalid) || 
         (!capability (output))) {
         return 0;
      }
      if (signals.size() == 0) {
         return true;
      }
      /* sort awg components according to time */
      awgSortComponents (&(*signals.begin()), signals.size());
      /* patch start time */
      if (capability (GPSsync) && (start >= 0)) {
         tainsec_t		dt = 0;
         if (signals.size() > 0) {
            dt = start - signals.front().start;
         }
         for (signallist::iterator iter = signals.begin(); 
             iter != signals.end(); iter++) {
            iter->start += dt;
         }
      }
   
      /* download waveform points */
      if ((channeltype == testpoint) || (channeltype == DAC) ||
         (channeltype == DSG)) {
         if (!points.empty()) {
            if (awgSetWaveform (slot, &(*points.begin()), points.size()) < 0) {
               return false;
            }
         }
      }
   
      /* set filter */
      if ((channeltype == testpoint) || (channeltype == DAC)) {
         // reset if empty
         if (filtercmd.empty()) {
            double y;
            awgSetFilter (slot, &y, 0);
         }
         // set otherwise
         else {
            FilterDesign ds ((double)chninfo.dataRate);
            if (!ds.filter (filtercmd.c_str()) || !isiir (ds())) {
               return false;
            }
            int nba = 4 * iirsoscount (ds()) + 1;
            double* ba = new double[nba];
            if (!iir2z (ds(), nba, ba)) {
               return false;
            }
            if (awgSetFilter (slot, ba, nba) < 0) {
               delete [] ba;
               return false;
            }
            delete [] ba;
         }
      }
   
      /* switch on excitation signal */
      switch (channeltype) {
         case EPICS:
            {
               /* epics channel */
               double d = signals.empty() ? 0 : signals.front().par[3];
               cerr << "set EPICS channel to " << d << endl;
               if (epicsPut (chnname.c_str(), d) != 0) {
                  cerr << "channel " << chnname << 
                     " not accessible" << endl;
                  return false;
               }
               break;
            }
         case DSG:
         case testpoint:
         case DAC:
            {
               cerr << "download " << signals.size() << endl;
               for (const_sigiterator iter = signals.begin();
                   iter != signals.end(); iter++) {
                  cerr << "   start = " << (double)(iter->start % (100*_ONESEC))/1E9;
                  cerr << "   durat = " << (double)iter->duration/1E9 << endl;
               }
               if (awgAddWaveform (slot, &(*signals.begin()),
                                  signals.size()) < 0) {
                  return false;
               }
               break;
            }
         default:
            {
               return false;
            }
      }
      return true;
   }


   bool excitation::stop (tainsec_t timeout, tainsec_t ramptime)
   {
      semlock		lockit (mux);
   
      if (slot < 0) {
         return true;
      }
   
      signals.clear();
      switch (channeltype) {
         case EPICS:
            /* epics channel */
            cerr << "set EPICS channel to " << epicsvalue << endl;
            if (epicsPut (chnname.c_str(), epicsvalue) != 0) {
               cerr << "channel " << chnname << 
                  " not accessible" << endl;
               return false;
            }
            return true;
         case DSG:
         case testpoint:
         case DAC:
            if (ramptime <= 0) {
               if (awgClearWaveforms (slot) < 0) {
                  return false;
               }
            }
            else {
               if (awgStopWaveform (slot, 2, ramptime) < 0) {
                  return false;
               }
            }
            return true;
         default:
            return true;
      }
   }


   bool excitation::freeze ()
   {
      semlock		lockit (mux);
   
      if (slot < 0) {
         return true;
      }
   
      signals.clear();
      switch (channeltype) {
         case EPICS:
            {
            /* epics channel */
               if (epicsGet (chnname.c_str(), &epicsvalue) != 0) {
                  cerr << "channel " << chnname << 
                     " not accessible" << endl;
                  return false;
               }
               return true;
            }
         case DSG:
         case testpoint:
         case DAC: 
            {
               if (awgStopWaveform (slot, 1, 0) < 0) {
                  return false;
               }
               return true;
            }
         default:
            {
               return true;
            }
      }
   }



   bool excitation::reset (bool hard, tainsec_t timeout)
   {
      semlock		lockit (mux);
   
      signals.clear();
      points.clear();
   
      if (slot < 0) {
         return true;
      }
      switch (channeltype) {
         case EPICS:
            /* epics channel */
            break;
         case DSG:
         case testpoint:
         case DAC:
            if (hard) {
               if (awgRemoveChannel (slot) < 0) {
                  slot = -1;
                  return false;
               }
               slot = -1;
            }
            else {
               if (awgClearWaveforms (slot) < 0) {
                  return false;
               }
            }
            break;
         default:
            return true;
      }
      return true;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Class Name: excitationManager					*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

   excitationManager::excitationManager () :
   silent (false), rampdown (0) {
   }


   bool excitationManager::init (testpointMgr& TPMgr,
                     bool Silent, tainsec_t rdown)
   {
      tpMgr = &TPMgr;
      silent = Silent;
      rampdown = rdown;
      return true;
   }


   bool excitationManager::start (tainsec_t start, tainsec_t timeout)
   {
      if (silent) {
         return true;
      }
   
      bool		err = false;
      semlock		lockit (mux);
   
      if (start == 0) {
         start = TAInow();
      }
   
      for (excitationlist::iterator iter = excitations.begin();
          iter != excitations.end(); iter++) {
         if (!iter->start (start, timeout)) {
            err = true;
         }
      }
      return !err;
   }


   bool excitationManager::stop (tainsec_t timeout, tainsec_t ramptime)
   {
      if (silent) {
         return true;
      }
   
      bool		err = false;
      semlock		lockit (mux);
   
      for (excitationlist::iterator iter = excitations.begin();
          iter != excitations.end(); iter++) {
         if (!iter->stop (timeout, ramptime)) {
            err = true;
         }
      }
      return !err;
   }


   bool excitationManager::freeze ()
   {
      if (silent) {
         return true;
      }
   
      bool		err = false;
      semlock		lockit (mux);
   
      for (excitationlist::iterator iter = excitations.begin();
          iter != excitations.end(); iter++) {
         if (!iter->freeze ()) {
            err = true;
         }
      }
      return !err;
   }

   bool excitationManager::add (const string& channel)
   {
      if (silent) {
         return true;
      }
   
      semlock		lockit (mux);
      string		chnname (channelName (channel));
   
      // check if in list
      excitationlist::iterator 	iter;
      for (iter = excitations.begin(); 
          iter != excitations.end(); iter++) {
         if (*iter == chnname) {
            iter->inUse++;
            // set test point if required
            if ((iter->inUse == 1) && (iter->isTP) && (tpMgr != 0)) {
               tpMgr->add (iter->chnname);
            }
            break;
         }
      }
      // if not add
      if (iter == excitations.end()) {
         excitation 	exc (chnname);
         if (!exc) {
            return false;
         }
         excitations.push_back (exc);
         // add to test point list
         excitations.back().isTP = 
            (tpMgr == 0) ? false : tpMgr->add (exc.chnname);
      
         iter = excitations.end();
      }
      return true;
   }


   bool excitationManager::add (const string& channel, 
                     const string& waveform,
                     double settlingtime)
   {
      if (silent) {
         return true;
      }
   
      semlock		lockit (mux);
      string		chnname (channelName (channel));
      AWG_Component 	comp[2]; 
      int		cnum;
      float* 		points = 0;
      int		num;
   
      // check if in list
      excitationlist::iterator 	iter;
      for (iter = excitations.begin(); 
          iter != excitations.end(); iter++) {
         if (*iter == chnname) {
            break;
         }
      }
      // if not add
      if (iter == excitations.end()) {
         if (!add (chnname)) {
            return false;
         }
         for (iter = excitations.begin(); 
             iter != excitations.end(); iter++) {
            if (*iter == chnname) {
               break;
            }
         }
         if (iter == excitations.end()) {
            return false;
         }
      }
      iter->wait = settlingtime;
   
      // now determine waveforms
      if (awgWaveformCmd (waveform.c_str(), comp, &cnum, 0, 
                         &points, &num, 
                         iter->channeltype == excitation::DSG) < 0) {
         return false;
      }
   
      if (points != 0) {
         if ((num <= 0) ||
            !iter->add (excitation::pointlist (points, points + num))) {
            free (points);
            return false;
         }
         free (points);
      }
   
      if (cnum <= 0) {
         return false;
      }
      for (int i = 0; i < cnum; i++) {
         if (!iter->add (comp[i])) {
            return false;
         }
      }
   
      return true;
   }


   bool excitationManager:: add (const string& channel,
                     const std::vector<AWG_Component>& awglist)
   {
      if (silent) {
         return true;
      }
   
      semlock		lockit (mux);	// lock mutex */
      string		chnname (channelName (channel));
   
      // check if in list
      excitationlist::iterator 	iter;
      for (iter = excitations.begin(); 
          iter != excitations.end(); iter++) {
         if (*iter == chnname) {
            break;
         }
      }
      // if not add
      if (iter == excitations.end()) {
         if (!add (chnname)) {
            return false;
         }
         for (iter = excitations.begin(); 
             iter != excitations.end(); iter++) {
            if (*iter == chnname) {
               break;
            }
         }
         if (iter == excitations.end()) {
            return false;
         }
      }
      // return if empty list
      if (awglist.empty()) {
         return true;
      }
      // now add awg list
      if (!iter->add (awglist.begin(), awglist.end())) {
         return false;
      }
      return true;
   }


   bool excitationManager::addFilter (const string& channel,
                     const std::string& filtercmd)
   {
      if (silent) {
         return true;
      }
   
      semlock		lockit (mux);
      string		chnname (channelName (channel));
   
      // check if in list
      excitationlist::iterator 	iter;
      for (iter = excitations.begin(); 
          iter != excitations.end(); iter++) {
         if (*iter == chnname) {
            break;
         }
      }
      // if not add
      if (iter == excitations.end()) {
         if (!add (chnname)) {
            return false;
         }
         for (iter = excitations.begin(); 
             iter != excitations.end(); iter++) {
            if (*iter == chnname) {
               break;
            }
         }
         if (iter == excitations.end()) {
            return false;
         }
      }
   
      // set filter command
      iter->filtercmd = filtercmd;
      return true;
   }


   bool excitationManager::del (const string& channel)
   {
      if (silent) {
         return true;
      }
   
      semlock		lockit (mux);	// lock mutex */
      string		chnname (channelName (channel));
   
      // check if in list
      excitationlist::iterator 	iter;
      for (iter = excitations.begin(); 
          iter != excitations.end(); iter++) {
         if (*iter == chnname) {
            break;
         }
      }
      if (iter ==excitations.end()) {
         return true;
      }
   
      // check inUse
      if (--iter->inUse <= 0) {
         // remove test point if in use count reaches zero
         if ((iter->isTP) && (tpMgr != 0)) {
            tpMgr->del (iter->chnname);
         }
      }
   
      return true;
   }


   bool excitationManager::del (tainsec_t timeout)
   {
      if (silent) {
         return true;
      }
   
      semlock		lockit (mux);	// lock mutex */
   
     // stop all channels
      if (rampdown <= 0) {
         stop (timeout);
      }
      else {
         stop (timeout, rampdown);
         timespec wait;
         wait.tv_sec = rampdown / _ONESEC;
         wait.tv_nsec = rampdown % _ONESEC;
         nanosleep (&wait, 0);
      }
      // loop through channel list and delete test points
      for (excitationlist::iterator iter = excitations.begin();
          iter != excitations.end(); iter++) {
         if ((iter->isTP) && (tpMgr != 0)) {
            tpMgr->del (iter->chnname);
         }
      }
      excitations.clear();
      return true;
   }


   double excitationManager::dwellTime () const
   {
      if (silent) {
         return 0;
      }
   
      semlock		lockit (mux);	// lock mutex */
      double		dwell = 0;
   
      for (excitationlist::const_iterator iter = excitations.begin();
          iter != excitations.end(); iter++) {
         dwell = max (dwell, iter->dwellTime());
      }
      return dwell;
   }

}
