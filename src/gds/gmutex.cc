static char *versionId = "Version $Id$" ;
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: gmutex							*/
/*                                                         		*/
/* Module Description: implements a mutex and lock objects		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

/* Header File List: */
#include <time.h>
#include "gmutex.hh"

namespace thread {


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// abstractsemaphore	                                                //
//                                                                      //
//////////////////////////////////////////////////////////////////////////
   bool abstractsemaphore::trylock_timed (int timeout, locktype lck)
   {
      bool succ_lock = false;
      for (int i = 0; !succ_lock && (i < 11); ++i) {
	 succ_lock = trylock();
	 if (!succ_lock && (i < 10)) {
	    timespec tick = {(100 * timeout) / 1000000000, 
	                     (100 * timeout) % 1000000000};
	    nanosleep (&tick, 0);
	 }
      }
      return succ_lock;
   }


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// recursivemutex	                                                //
//                                                                      //
//////////////////////////////////////////////////////////////////////////
   bool recursivemutex::trylock (locktype) 
   {
      if ((refcount > 0) && (threadID == pthread_self())) {
         refcount++;
         return true;
      }
      if (pthread_mutex_trylock (&mux) != 0) {
         return false;
      }
      else {
         threadID = pthread_self();
         refcount = 1;
         return true;
      }
   }


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// readwritelock	                                                //
//                                                                      //
//////////////////////////////////////////////////////////////////////////
   readwritelock::~readwritelock () 
   {
      pthread_cond_destroy (&cond);
      pthread_mutex_destroy (&mux);
   }

//______________________________________________________________________________
   void readwritelock::readlock () 
   {
      // can share lock: check if available
      pthread_mutex_lock (&mux);
      while ((inuse < 0) || (wrwait != 0) ||
            ((maxuse > 0) && (inuse >= maxuse))) {
         pthread_cond_wait (&cond, &mux);
      }
      inuse++;
      pthread_mutex_unlock (&mux);
   }

//______________________________________________________________________________
   void readwritelock::writelock () 
   {
      /* need exclusive lock: check if free */
      pthread_mutex_lock (&mux);
      wrwait++;
      while (inuse != 0) {
         pthread_cond_wait (&cond, &mux);
      }
      inuse--;
      pthread_mutex_unlock (&mux);
   }

//______________________________________________________________________________
   void readwritelock::unlock () 
   {
      pthread_mutex_lock (&mux);
      if (inuse == -1) {
         wrwait--;
         inuse = 0;
      }
      else if (inuse > 0) {
         inuse--;
      }
      pthread_cond_broadcast (&cond);
      pthread_mutex_unlock (&mux);
   }

//______________________________________________________________________________
   bool readwritelock::trylock (locktype lck) 
   {      
      bool		success = false;
      pthread_mutex_lock (&mux);
      if (lck == wrlock) {
         // need exclusive lock: check if free
         if (inuse == 0) {
            wrwait++;
            inuse--;
            success = true;
         }
      }
      else {
         // can share lock: check if available
         if ((inuse >= 0) && (wrwait == 0) &&
            ((maxuse <= 0) || (inuse < maxuse))) {
            inuse++;
            success = true;
         }
      }
      pthread_mutex_unlock (&mux);
      return success;
   }



//////////////////////////////////////////////////////////////////////////
//                                                                      //
// barrier		                                                //
//                                                                      //
//////////////////////////////////////////////////////////////////////////
   barrier::barrier (int count)
   {
      maxcnt = count;
      sbp = &sb[0];
      for (int i = 0; i < 2; ++i) {
         sb[i].runners = count;
         pthread_mutex_init (&sb[i].wait_lk, 0);
         pthread_cond_init (&sb[i].wait_cv, 0);
      }
   }

//______________________________________________________________________________
   barrier::~barrier()
   {
      for (int i=0; i < 2; ++ i) {
         pthread_cond_destroy (&sb[i].wait_cv);
         pthread_mutex_destroy (&sb[i].wait_lk);
      }
   } 

//______________________________________________________________________________
   bool barrier::wait()
   {
      register subbarrier* p = sbp;
      if (p->runners < 1) {
         return false;
      }
      pthread_mutex_lock (&p->wait_lk);
      if (p->runners == 1) {   
         if (maxcnt != 1) {
            // reset runner count and switch sub-barriers
            p->runners = maxcnt;
            sbp = (sbp == &sb[0]) ? &sb[1] : &sb[0];
            // wake up the waiters
            pthread_cond_broadcast (&p->wait_cv);
         }
      } 
      else {
         p->runners--;         // one less runner
         while (p->runners != maxcnt)
            pthread_cond_wait (&p->wait_cv, &p->wait_lk);
      }
      pthread_mutex_unlock(&p->wait_lk);
      return true;   
   }

}
