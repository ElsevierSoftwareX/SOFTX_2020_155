#ifndef __DAQD_ATOMIC_OPS_HH___
#define __DAQD_ATOMIC_OPS_HH___

#include "config.h"

#ifdef DAQD_CPP11

    #ifdef HAVE_ATOMIC
        #include <atomic>
    #else
        #ifdef HAVE_CSTDATOMIC
            #include <cstdatomic>
        #endif /* HAVE_CSTDATOMIC */
    #endif /* HAVE_ATOMIC */

    /*typedef std::atomic<int> daqd_atomic_int;
     *typedef std::atomic<unsigned int> daqd_atomic_uint;
     *typedef std::atomic<bool> daqd_atomic_bool;
     *template <typename A>
     *void daqd_store_val(A& dest, A::T val) { dest.store(val); } 
     *template <typename A>
     *void daqd_fetch_add(A& dest, A::T n) { dest.fetch_add(n); }
     */
     typedef int daq_atomic_int;
     typedef unsigned int daqd_atomic_uint;
     typedef bool daqd_atomic_bool;

     template <class T>
     void daqd_store_val(T& dest, T val) { dest = val; }

     template <class T>
     void daqd_fetch_add(T& dest, int val) { dest += val; }

#else  /* DAQD_CPP11 */

/* does this even work? */

typedef int daqd_atomic_int;
typedef unsigned int daqd_atomic_uint;
typedef bool daqd_atomic_bool;

template <class T, class U>
void daqd_store_val(T& dest, U val) { dest = val; }

template <class T>
void daqd_fetch_add(T& dest, int val) { dest += val; }

#endif /* DAQD_CPP11 */

#endif /* __DAQD_ATOMIC_HH__ */
