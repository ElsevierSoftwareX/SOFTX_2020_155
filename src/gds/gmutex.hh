/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: gmutex.hh						*/
/*                                                         		*/
/* Module Description: mutex objects		 			*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 1.0	 21Nov98  D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: gmutex.html  					*/
/*	References: none						*/
/*                                                         		*/
/* Author Information:							*/
/* Name          Telephone       Fax             e-mail 		*/
/* Daniel Sigg   (509) 372-8132  (509) 372-8137  sigg_d@ligo.mit.edu	*/
/*                                                         		*/
/* Code Compilation and Runtime Specifications:				*/
/*	Code Compiled on: Ultra-Enterprise, Solaris 5.6			*/
/*	Compiler Used: sun workshop C 4.2				*/
/*	Runtime environment: sparc/solaris				*/
/*                                                         		*/
/* Code Standards Conformance:						*/
/*	Code Conforms to: LIGO standards.	OK			*/
/*			  Lint.			TBD			*/
/*			  ANSI			OK			*/
/*			  POSIX			OK (for UNIX)		*/
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
#ifndef _GDS_GMUTEX_H
#define _GDS_GMUTEX_H

//#include "PConfig.h"
#ifdef P__SOLARIS
#include <time.h>
#endif
#include <pthread.h>


namespace thread {

/** @name Mutex objects
    This library defines objects dealing with mutual exclusion
    semaphores and with read-write locks.

    @memo Classes for handling mutex
    @author Written November 1998 by Daniel Sigg
    @version 1.0
************************************************************************/

/*@{*/	

/** This class is used as an abstract base class for mutex and locks.

    @memo Abstract class to manage a semaphore.
    @author DS, November 98
    @see Mutex objects
************************************************************************/
   class abstractsemaphore { 
   public:
      /// type of lock
      typedef enum locktype {
      /// read lock
      rdlock = 0,
      /// write lock
      wrlock = 1
      };
      typedef enum locktype locktype;
   
   
      /** Abstract virtual destructor.
          @memo Default destructor.
          @return void
      *****************************************************************/
      virtual ~abstractsemaphore () {
      }
   
      /** Locks the semaphore (abstract virtual method).
          @memo Semaphore lock function.
          @return void
      *****************************************************************/
      virtual void lock () = 0;
   
      /** Locks the semaphore to allow read access 
          (abstract virtual method).
          @memo Semaphore lock function.
          @return void
      *****************************************************************/
      virtual void readlock () = 0;
   
      /** Locks the semaphore to allow wrire access
          (abstract virtual method).
          @memo Semaphore lock function.
          @return void
      *****************************************************************/
      virtual void writelock () = 0;
   
      /** Unlocks the semaphore (abstract virtual method).
          @memo Semaphore unlock function.
          @return void
      *****************************************************************/
      virtual void unlock () = 0;
   
      /** Tries to lock the semaphore (abstract virtual method). 
          The return argument indicates whether the semaphore was 
          successfully locked, or whether the semaphore was already 
          taken by somebody else.
          @memo Semaphore trylock function.
          @param lck Lock type
          @return true if semaphore locked
      *****************************************************************/
      virtual bool trylock (locktype lck = rdlock) = 0;

      /** Tries to lock the semaphore within the given time. 
          The return argument indicates whether the semaphore was 
          successfully locked, or whether the semaphore was already 
          taken by somebody else. This routine will try to lock 
	  the mutex 11 times, sleeping 1/10 of the specified timeout
	  everytime the lock fails.
          @memo Semaphore trylock function.
          @param timeout Timeout in usec
          @param lck Lock type
          @return true if semaphore locked
      *****************************************************************/
      virtual bool trylock_timed (int timeout, locktype lck = rdlock);
   };


/** This class is used as a wrapper around a system defined mutex.
    During construction the mutex is created and during desctruction 
    it is automatically destroyed. A mutex object describes a unique
    mutex which can not be copied. When passing a mutex object to
    a function it has to be passed by reference or by pointer (never
    by value). There are methods to lock, unlock and trylock the mutex.
    On Unix the object uses the POSIX standard, whereas under VxWorks
    it uses the native mutex Lock and unlock methods must always be 
    used in pairs within the same context.

    @memo Class to store a mutex.
    @author DS, November 98
    @see Mutex objects
************************************************************************/
   class mutex : public abstractsemaphore {
   public:
   
      /** Constructs a mutex object and creates a new mutex.
          @memo Default constructor.
          @return void
      *****************************************************************/
      mutex () {
         pthread_mutex_init (&mux, 0); };
   
      /** Destructs a mutex object and destroyes the mutex.
          @memo Default destructor.
          @return void
      *****************************************************************/
      virtual ~mutex () {
         pthread_mutex_destroy (&mux); }
   
      /** Constructs a mutex object, overwritting the default
          copy constructor by creating a new mutex.
          @memo Copy constructor.
          @return void
      *****************************************************************/
      mutex (const mutex&) {
         pthread_mutex_init (&mux, 0); }
   
      /** Overrides the default assignment behaviour. Does nothing.
          @memo Assignment operator.
          @param mutex copy argument
          @return refrence to object
      *****************************************************************/
      mutex& operator= (const mutex&) {
         return *this; }
   
      /** Locks the mutex. If the mutex is not available waits until it
          becomes free.
          @memo Mutex lock function.
          @return void
      *****************************************************************/
      virtual void lock () {
         pthread_mutex_lock (&mux); }
   
      /** Locks the mutex for read, same as lock().
          @memo Mutex readlock function.
          @return void
      *****************************************************************/
      virtual void readlock () {
         lock(); }
   
      /** Locks the mutex for write, same as lock().
          @memo Mutex writelock function.
          @return void
      *****************************************************************/
      virtual void writelock () {
         lock (); }
   
      /** Unlocks the mutex. The mutex becomes free.
          @memo Mutex unlock function.
          @return void
      *****************************************************************/
      virtual void unlock () {
         pthread_mutex_unlock (&mux); }
   
      /** Trys to lock the mutex. If the mutex is free, it gets locked
          and the method returns true. If the mutex is already taken,
          the method returns false.
          @memo Mutex trylock function.
          @param writeaccess ignored
          @return true if locked, false otherwise
      *****************************************************************/
      virtual bool trylock (locktype lck = rdlock) {
         return (pthread_mutex_trylock (&mux) == 0); }
   
   protected:
      pthread_mutex_t	mux;
   };


/** This class is used as a wrapper around a system defined mutex.
    A recursive mutex is similar to a normal mutex object, but it 
    allows a single task/thread to take a mutex multiple times. This 
    is useful for a set of routines/methods that must call each
    other but that also require mutually exclusive access to a 
    resource. The recursive mutex keeps track which task/thraad
    currently owns the mutex and also keeps a reference count on
    how often it was locked. The system mutex will only be released
    after the last unlock call. Under Unix this mutex is less effcient
    than the normal mutex, but sometimes convinient. Under VxWorks
    this is the default behaviour.

    @memo Class to store a recuresive mutex.
    @author DS, November 98
    @see Recursive mutex objects
************************************************************************/
   class recursivemutex : public mutex {
   public:
   
      /** Constructs a recuresive mutex object and creates a new mutex.
          @memo Default constructor.
          @return void
      *****************************************************************/   
      recursivemutex () : refcount (0) {
      };
   
      /** Constructs a recuresive mutex object by creating a new mutex
          with reference count zero, rather than copying it.
          @memo Copy constructor.
          @param another recursive mutex
          @return void
      *****************************************************************/   
      recursivemutex (const recursivemutex&) : refcount (0) {
      };
   
      /** Overrides the default assignment behaviour. Does nothing.
          @memo Assignment operator.
          @param recursivemutex copy argument
          @return refrence to object
      *****************************************************************/
      recursivemutex& operator= (const recursivemutex&) {
         return *this; }
   
      /** Locks the mutex. If the mutex is alreday taken it checks
          if the current thread is the same as the one which took the
          mutex originally. If no, it waits until the mutex becomes 
          free. If yes, it increases the reference count and returns.
          @memo Mutex lock function.
          @return void
      *****************************************************************/
      virtual void lock () {
         if ((refcount > 0) && (threadID == pthread_self())) {
            refcount++;
            return;
         }
         pthread_mutex_lock (&mux);
         threadID = pthread_self();
         refcount = 1;
      }
   
      /** Unlocks the mutex. The mutex becomes free.
          @memo Mutex unlock function.
          @return void
      *****************************************************************/
      virtual void unlock () {
         if (--refcount == 0) {
            threadID = 0;
            pthread_mutex_unlock (&mux);
         }
      }
   
      /** Trys to lock the mutex. If the mutex is alreday taken it checks
          if current thread is the same as the one which locked the
          mutex originally. If no, it returns false. If yes, it 
          increases the reference count and returns true.
   
          @memo Mutex trylock function.
          @param writeaccess ignored
          @return true if locked, false otherwise
      *****************************************************************/
      virtual bool trylock (locktype lck = rdlock);
   
   protected:
      pthread_t		threadID;
      int		refcount;
   };


/** This class is used to implement a read/write lock. A read/write
    lock can be locked by multple readers simultaneously. A writer
    owns the lock exclusively. The maximum number of readers can be
    specified during creation. Writers have priority over readers,
    meaning if a request from a writer is pending, no further
    read access is granted. Then, after all readers have returned the
    lock, the writer will get granted access first. When passing a 
    read/write lock object to a function it has to be passed by 
    reference or by pointer (never by value because the copy operator
    is disabled for read/write locks).

    @memo Class to store a read/write lock.
    @author DS, November 98
    @see Recursive mutex objects
************************************************************************/
   class readwritelock : public abstractsemaphore {
   public:
      /** Constructs a read/write lock. Takes the maxumum number of
          concurrent read locks as argument; a number equal or less
          zero represents unlimited read access.
          @memo Default constructor.
          @param Maxuse maximum number of readers
          @return void
      *****************************************************************/  
      explicit readwritelock (int Maxuse = -1)
      : maxuse (Maxuse), inuse (0), wrwait (0) {
         pthread_mutex_init (&mux, 0);
         pthread_cond_init (&cond, 0);
      }
   
      /** Destructs the read/write lock.
          @memo Default destructor.
          @return void
      *****************************************************************/
      virtual ~readwritelock ();
   
      /** Constructs a read/write lock, overwritting the default
          copy constructor by creating a new read/write lock.
          @memo Copy constructor.
          @return void
      *****************************************************************/
      readwritelock (const readwritelock& rw) 
      : maxuse (rw.maxuse), inuse (0), wrwait (0) {
         pthread_mutex_init (&mux, 0);
         pthread_cond_init (&cond, 0);
      }
   
      /** Overrides the default assignment behaviour. Does nothing.
          @memo Assignment operator.
          @param readwritelock copy argument
          @return refrence to object
      *****************************************************************/
      readwritelock& operator= (const readwritelock&) {              
         return *this; }
   
      /** Locks the lock for read.
          Multiple read locks (up to maxuse) can be granted, but only
          one write lock at any given time. If a write lock is 
          requested while the lock is given to one or more readers,
          no further read locks will be granted to prevent the write
          task from starvation. (Write locks have absolute priority.)
          @memo Read/write-lock lock function.
          @return void
      *****************************************************************/
      virtual void readlock ();
   
      /** Locks the lock for write.
          Multiple read locks (up to maxuse) can be granted, but only
          one write lock at any given time. If a write lock is 
          requested while the lock is given to one or more readers,
          no further read locks will be granted to prevent the write
          task from starvation. (Write locks have absolute priority.)
          @memo Read/write-lock lock function.
          @return void
      *****************************************************************/
      virtual void writelock ();
   
      /** Locks the lock for read; same as readlock().
          @memo Read/write-lock lock function.
          @return void
      *****************************************************************/
      virtual void lock () {
         readlock (); }
   
      /** Unlocks the read/write lock.
          @memo Read/write-lock unlock function.
          @return void
      *****************************************************************/
      virtual void unlock ();
   
      /** Trys to lock the read/write lock. To return true and locked, 
          either a writer attemps to lock a free lock, or a reader 
          attemps to obtain a lock which is not owned by a writer.
          Otherwise the function returns false and without the lock.
   
          @memo Read/write-lock trylock function.
          @param write true for a write lock, false for a read lock
          @return true if locked, false otherwise
      *****************************************************************/
      bool trylock (locktype lck = rdlock);
   
   private:
      pthread_mutex_t	mux;
      pthread_cond_t	cond;
      int		maxuse;
      int		inuse;
      int		wrwait;
   };


/** This class can be used to automatically lock and unlock a semaphore 
    over the duration of a function call. semlock has to be 
    initialized with an mutex, a recursive mutex or a read/write lock. 
    Upon contruction of the object the semaphore is locked. When the 
    object is destroyed the semaphore is automatically freed.

    \begin{verbatim} 
    Example:

    mutex	mux;
    void foobar () {
       mutexlock	lockit (mux);
       // statements of foobar here
    }
    \end{verbatim}

    Since an object is automatically destroyed at the end of its 
    context, semlock can also be used in loops and branches of
    conditional statments, or in any compound statement to protect
    a resource for the duration of the context.

    @memo Class to automatically lock and unlock a mutex.
    @author DS, November 98
    @see Recursive mutex objects
************************************************************************/
   class semlock {
   public:
      /// type of lock
      typedef enum locktype {
      /// read lock
      rdlock = 0,
      /// write lock
      wrlock = 1
      };
      typedef enum locktype locktype;
   
      /** Constructs a semmutex object and locks the semaphore.
          @memo Default constructor.
          @param sem reference to a semaphore to be locked
          @return void
      *****************************************************************/   
      explicit semlock (abstractsemaphore& sem) 
      : _sem (&sem) {
         _sem->lock(); }
   
      /** Constructs a semmutex object and locks the semaphore. The
          second argument specifies whether a read or write lock is
          applied.
          @memo Default constructor.
          @param sem reference to a semaphore to be locked
          @return void
      *****************************************************************/
      semlock (abstractsemaphore& sem, locktype lck) 
      : _sem (&sem) {
         if (lck == wrlock) _sem->writelock();
         else _sem->readlock(); }
   
      /** Destructs the semmutex object and unlocks the semaphore.
          @memo Default destructor.
          @return void
      *****************************************************************/
      virtual ~semlock () {
         _sem->unlock(); }
   
   
   private:
      abstractsemaphore*	_sem;
      semlock();
      semlock (const semlock&);
      semlock& operator= (const semlock&);
   };


/** This class can be used to implement a barrier.
    In many applications, and especially numerical applications, 
    while part of the algorithm can be parallelized, other parts are 
    inherently sequential, as shown in the following:

    \begin{verbatim}
    Thread1                             Thread2 through Threadn
  
    while(many_iterations) {            while(many_iterations) {

      sequential_computation
      --- Barrier ---                     --- Barrier ---
      parallel_computation                parallel_computation
    }                                   }
    \end{verbatim}

    The nature of the parallel algorithms for such a computation is 
    that little synchronization is required during the computation, but
    synchronization of all the threads employed is required to ensure 
    that the sequential computation is finished before the parallel
    computation begins. 

    The barrier forces all the threads that are doing the parallel 
    computation to wait until all threads involved have reached the 
    barrier. When they've reached the barrier, they are released and 
    begin computing together.

    \begin{verbatim} 
    Example:

    \end{verbatim}

    @memo Barrier class.
    @author DS, November 98
    @see Recursive mutex objects
************************************************************************/
   class barrier {
   public:
      /// Create a barrier for count tasks
      barrier (int count);
      /// Destroy a barrier
      ~barrier ();
      /// wait at a barrier for all tasks
      bool wait();
   
   protected:
      /// subbarrier variables
      struct subbarrier {
         /// condition for waiters at barrier
         pthread_cond_t  wait_cv;
         /// mutex for waiters at barrier
         pthread_mutex_t wait_lk;
         /// number of running threads
         int runners;
      };
   
      /// Maximum number of runners
      int maxcnt;  
      /// Subbarriers   
      subbarrier sb[2];
      /// Current sub-barrier
      subbarrier* sbp;  
   };


/*@}*/
}
#endif /* _GDS_GMUTEX_H */
