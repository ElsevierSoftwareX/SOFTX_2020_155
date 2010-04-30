/********************************************************************\
|    Time  Class Implementation                                      |
\********************************************************************/
#include "Interval.hh"
#include "Time.hh"
#include <time.h>
#include <iostream>

static const Time::ulong_t iStoNS = 1000000000;
static const double dStoNS = 1000000000.;
static const double dNStoS = 0.000000001;

//-------------------------------------- Constructor from secs and nanosecs.
Time::Time(ulong_t sec, ulong_t nsec) {
    mNsec = nsec % iStoNS;
    mSec  = sec + nsec/iStoNS;
}

//-------------------------------------- Constructor from secs and nanosecs.
Time::Time(void) {
    mNsec = 0;
    mSec  = 0;
}

//-------------------------------------- Copy constructor
Time::Time(const Time& t) {
    mSec  = t.mSec;
    mNsec = t.mNsec;
}

//-------------------------------------- Assignment operator
Time& Time::operator =(const Time& t) {
    mSec  = t.mSec;
    mNsec = t.mNsec;
    return (*this);
} 

//--------------------------------------  Get GPS time in seconds
double 
Time::totalS(void) const {
    return double(mSec) + double(mNsec) * dNStoS;}

//--------------------------------------  Get GPS time in nano-seconds
double 
Time::totalNS(void) const {
    return double(mSec) * dStoNS + double(mNsec);}

//--------------------------------------  Get Fractional nanoseconds
double 
Time::fracS(void) const {
    return double(mNsec) * dNStoS;}

//--------------------------------------  Add a time to another.
Time 
Time::operator +=(const Interval& dt) {
    long  ssec = dt.GetS();
    mNsec += dt.GetN();
    if (mNsec >= iStoNS) {
        mNsec -= iStoNS;
	ssec++;}
    if (ssec >= 0) {
        mSec  += ulong_t(ssec);
    } else if (mSec >= ulong_t(-ssec)) {
	mSec  -= ulong_t(-ssec);
    } else {
        mNsec = 0;
	mSec  = 0;
    }
    return (*this);
}

//--------------------------------------  Difference of two times.
Interval Time::operator -(const Time& t) const {
    long sec  = long(mSec) - long(t.mSec);
    long nsec = long(mNsec) - long(t.mNsec);
    if (nsec < 0) {
        sec--;
        nsec += iStoNS;
    }
    Interval r(sec, nsec);
    return r;
}

//--------------------------------------  Subtract a time from another.
Time Time::operator -=(const Interval& t) {
    long  sec  = t.GetS();
    ulong_t nsec = t.GetN();
    if (mNsec >= nsec) {
        mNsec -= nsec;
    } else {
        sec++;                 //  Bump a number to be subtracted.
	mNsec += iStoNS - nsec;
    }
    if (sec <= 0) {
        mSec += ulong_t(-sec);
    } else if (mSec >= ulong_t(sec)) {
        mSec -= ulong_t(sec);
    } else {
        mSec  = 0;
	mNsec = 0;
    }
    return (*this);
}

//--------------------------------------  Set nano-second
void Time::setN(ulong_t n) {
    mNsec = n % iStoNS;
}

//--------------------------------------------------------------------------
//
//   Non-member operators.
//
//--------------------------------------------------------------------------

//--------------------------------------  Add an interval lhs
Time operator +(const Interval& dt, const Time& t) {
    Time result = t;
    result += dt;
    return result;
}

//--------------------------------------  Add an interval rhs.
Time operator +(const Time& t, const Interval& dt) {
    Time result = t;
    result += dt;
    return result;
}

//--------------------------------------  Difference of two times.
Time operator -(const Time& t, const Interval& dt) {
    Time result = t;
    result -= dt;
    return result;
}

//--------------------------------------  Almost equal
bool Almost(const Time& t1, const Time& t2, Time::ulong_t dT) {
    Time::ulong_t dS, dNS;
    if (t1 >= t2) {
        dS = t1.getS() - t2.getS();
	if (dS > 1) return false;
	dNS = iStoNS * dS +t1.getN() - t2.getN();
	if (dNS > dT) return false;
    } else {
        dS = t2.getS() - t1.getS();
	if (dS > 1) return false;
	dNS = iStoNS * dS + t2.getN() - t1.getN();
	if (dNS > dT) return false;
    }
    return true;
;}






