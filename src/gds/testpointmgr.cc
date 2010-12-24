static const char *versionId = "Version $Id$" ;
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: testpointmgr						*/
/*                                                         		*/
/* Module Description: manages test points				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/


#include <vector>
#include <iostream>
#include "dtt/testpointmgr.hh"
#include "dtt/testpoint.h"


namespace diag {
   using namespace std;
   using namespace thread;

   const char	taskCleanupName[] = "tTPclean";
   const int	taskCleanupPriority = 20;


   int testpointMgr::cleanuptask (testpointMgr& tpMgr) 
   {
      const struct timespec delay = {1, 0};
   #ifndef OS_VXWORKS
      int 		oldtype;
      pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);
   #endif
   
      while (true) {
         nanosleep (&delay, 0);
         tpMgr.cleanup ();
      }
      return 0;
   }


   testpointMgr::testpointMgr (double Lazytime) 
   : active (true), lazytime (Lazytime), cleartime (0), cleanTID (0)
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


   testpointMgr::~testpointMgr () 
   {
      semlock		lockit (mux);	// lock mutex */
   
      // cancel cleanup task
      taskCancel (&cleanTID);
   
      // delete all test points
      del ();
   }


   void testpointMgr::setState (bool Active)
   {
      semlock		lockit (mux);	// lock mutex */
   
      if (active != Active) {
         clear (true);
         active = Active;
      }
   }

   bool testpointMgr::areSet () const
   {
      semlock		lockit (mux);	// lock mutex */
   
      for (testpointrecord::const_iterator iter = testpoints.begin();
          iter != testpoints.end(); iter++) {
         // if not set return false
         if (!iter->second.isSet) {
            return false;
         }
      }
      // all set
      return true;
   }


   bool testpointMgr::areUsed () const
   {
      semlock		lockit (mux);	// lock mutex */
   
      for (testpointrecord::const_iterator iter = testpoints.begin();
          iter != testpoints.end(); iter++) {
         // if not used return false
         if (iter->second.inUse <= 0) {
            return false;
         }
      }
      // all set
      return true;
   }


   bool testpointMgr::set (tainsec_t* activeTime) 
   {
      semlock		lockit (mux);	// lock mutex */
   
      // check if lazy clears have to be committed
      if ((cleartime > 0) && !areUsed()) {
         testpointrecord::iterator iter = testpoints.begin();
         while (iter != testpoints.end()) {
            // if not used delete
            if (iter->second.inUse <= 0) {
               // remove from list
               if (iter->second.isSet) {
                  if (active) {
                     tpClear (iter->first.first, &iter->first.second, 1);
                  }
                  iter->second.isSet = false;
               }
               testpoints.erase (iter);
               iter = testpoints.begin();
            }
            else {
               iter++;
            }
         }
      }
   
      // check if already set
      if (areSet ()) {
         if (activeTime != 0) {
            *activeTime = TAInow();
         }
         return true;
      }
   
      // set test points
      vector<testpoint_t>	tp;
      taisec_t			tai = (taisec_t)-1;
      int			epoch = 0;
      testpointrecord::iterator next;
   
      // loop through test point list
      for (testpointrecord::iterator iter = testpoints.begin();
          iter != testpoints.end(); iter++) {
         // add to tp list if unset
         if (!iter->second.isSet) {
            tp.push_back (iter->first.second);
         }
         (next = iter)++;
         if ((next == testpoints.end()) || 
            (iter->first.first != next->first.first)) {
            // new node
            if (tp.size() > 0) {
               // set list 
               if (active && 
                  tpRequest (iter->first.first, &(*tp.begin()), tp.size(), 
                            -1, &tai, &epoch) < 0) {
                  // failed
                  clear (false);
                  return false;
               }
               // update isSet flag
               testpointrecord::iterator iter2 = iter;
               while (true) {
                  iter2->second.isSet = true;
                  if (iter2 == testpoints.begin()) {
                     break;
                  }
                  iter2--;
               }
               // empty list
               tp.clear();
            }
         }
      }
   
      // set active time
      if (activeTime != 0) {
         if (!active) {
            *activeTime = 0;
         }
         else if (tai == (taisec_t)-1) {
            *activeTime = TAInow();
         }
         else {
            *activeTime = (tainsec_t) tai * _ONESEC + 
               (tainsec_t) epoch * _EPOCH;
         }
      }
      cleartime = 0;
      return true;
   }


   bool testpointMgr::clear (bool lazy)
   {
      semlock		lockit (mux);	// lock mutex */
   
      // mark only if lazy clear
      if (lazy) {
         cleartime = (double) TAInow() / (double) _ONESEC;
         return true;
      }
   
      // clear test points
      vector<testpoint_t>	tp;
      testpointrecord::iterator next;
   
      // loop through test point list
      for (testpointrecord::iterator iter = testpoints.begin();
          iter != testpoints.end(); iter++) {
         // add to tp list if set
         if (iter->second.isSet) {
            tp.push_back (iter->first.second);
         }
         (next = iter)++;
         if ((next == testpoints.end()) || 
            (iter->first.first != next->first.first)) {
            // new node
            if (tp.size() > 0) {
               // clear list
               if (active) {
                  tpClear (iter->first.first, &(*tp.begin()), tp.size());
               }
               // update isSet flag
               testpointrecord::iterator iter2 = iter;
               while (true) {
                  iter2->second.isSet = false;
                  if (iter2 == testpoints.begin()) {
                     break;
                  }
                  iter2--;
               }
               // empty list
               tp.clear();
            }
         }
      }
      cleartime = 0;
      return true;
   }


   bool testpointMgr::add (const string& chnname)
   {
      int		node;	// rm node
      testpoint_t	tp;	// testpoint id
      semlock		lockit (mux);	// lock mutex */
   
      // make sure it is valid
      if (!tpIsValidName (chnname.c_str(), &node, &tp)) {
         return false;
      }
   
      // test if already in list
      nodetestpoint	nodetp (node, tp); // node/test point pair
      testpointrecord::iterator iter = testpoints.find (nodetp);
      if (iter != testpoints.end()) {
         // increase in use count
         iter->second.inUse++;
      }
      else {
         // add to list
         testpointinfo 	tpinfo (chnname); // test point info
         testpoints.insert (testpointrecord::value_type (nodetp, tpinfo));
      }
   
      return true;
   }


   bool testpointMgr::del (const string& chnname)
   {
      int		node;	// rm node
      testpoint_t	tp;	// testpoint id
      semlock		lockit (mux);	// lock mutex */
   
      // make sure it is valid
      if (!tpIsValidName (chnname.c_str(), &node, &tp)) {
         return false;
      }
   
      // test if already in list
      nodetestpoint	nodetp (node, tp); // node/test point pair
      testpointrecord::iterator iter = testpoints.find (nodetp);
      if (iter == testpoints.end()) {
         // not found
         return false;
      }
   
      // check inUse
      if ((--iter->second.inUse <= 0) && (cleartime == 0)) {
         // remove from list
         if (iter->second.isSet) {
            if (active) {
               tpClear (iter->first.first, &iter->first.second, 1);
            }
            iter->second.isSet = false;
         }
         testpoints.erase (iter);
      }
   
      return true;
   }


   bool testpointMgr::del ()
   {
      semlock		lockit (mux);	// lock mutex */
   
      // clear all test points
      clear (false);
      testpoints.clear();
      return true;
   }


   void testpointMgr::cleanup ()
   {
      semlock		lockit (mux);	// lock mutex */
   
      // check time
      if ((lazytime > 0) && (cleartime > 0) &&
         ((double) TAInow() / _ONESEC > cleartime + lazytime)) {
         cleartime = 0;
         // cleanup
         testpointrecord::iterator iter;
         do {
            for (iter = testpoints.begin(); iter != testpoints.end();
                iter++) {
               if (iter->second.inUse <= 0) {
                  if (!del (iter->second.name)) {
                     return;
                  }
                  iter = testpoints.begin();
                  break;
               }
            }
         } while (iter != testpoints.end());
      }
   }


}
