#ifndef INTERVAL_HH
#define INTERVAL_HH

#ifndef __CINT__
#define TICKS
#endif
#include <iosfwd>

#define DNSTOS 0.000000001
#define DSTONS 1000000000.

/** The Interval class represents time intervals. This class complements
  * and is compatible with the Time class which represents absolute GPS 
  * times. At present, intervals are represented as double-precision 
  * floating point numbers. This gives about 53-bits to the representation 
  * which corresponds to a precision of 1 in $9x10^15$ or <1ns for 
  * intervals of up to ~100 days.
  * @memo Time interval class.
  * @author John. G. Zweizig
  * @version 1.1; Last Modified: June 15, 1999
  */
class Interval {
  public:
    typedef unsigned long ulong_t;
    /** Default constructor.
     */
    Interval() {mSec=0.0;}

    /** Constructor from seconds and nanoseconds.
     */
    Interval(long sec, ulong_t nsec) {
          mSec = double(nsec) * DNSTOS + double(sec);}

    /** Constructor from a double (in seconds).
     */
    Interval(double sec) {mSec = sec;}

    /** Copy constructor
     */
    Interval(const Interval& dt) {mSec = dt.mSec;}

    //----------------------------------  Overload various operators.
    /** Assignment operator
     */
    Interval& operator= (const Interval& dt) {
	mSec = dt.mSec;
	return (*this);}

    /** Cast to double
     */
    operator double(void) const {
	return mSec;}

    /** Format the Interval to ostream.
     */
    friend std::ostream &operator <<(std::ostream &out, const Interval& t);

    /**  Add an interval to another.
     */
    Interval operator +=(const Interval& dt) {
        mSec += dt.mSec;
	return (*this);}

    /**  Subtract an Interval.
     */
    Interval operator -=(const Interval& dt) {
        mSec -= dt.mSec;
	return (*this);}

    /**  Multiply Interval by a double constant
      */
    Interval operator *=(double dt) {
        mSec *= dt;
	return (*this);}

    /**  Divide an Interval by an Interval.
     */
    double operator /(const Interval& div) const {
        return mSec / div.mSec;}

    /**  Divide a Interval by a double constant
     */
    Interval operator /=(double dt) {
        mSec /= dt;
        return (*this);}

    /**  Test for zero
     */
    bool operator!(void) const {return (ticks() == 0);}

    /**  Compare Intervals (equality)
     */
    bool operator ==(const Interval& rhs) const {
        return (ticks() == rhs.ticks());}

    /**  Compare Intervals (inequality)
     */
    bool operator !=(const Interval& rhs) const {
        return (ticks() != rhs.ticks());}

    /**  Compare Intervals (Greater or Equal)
     */
    bool operator >=(const Interval& rhs) const {
        return (ticks() >= rhs.ticks());}

    /**  Compare Intervals (Less or equal)
     */
    bool operator <=(const Interval& rhs) const {
        return (ticks() <= rhs.ticks());}

    /**  Compare Intervals (Greater)
     */
    bool operator >(const Interval& rhs) const {
        return (ticks() > rhs.ticks());}

    /**  Compare Intervals (Less than)
     */
    bool operator <(const Interval& rhs) const {
        return (ticks() < rhs.ticks());}

    //------------------------------------  Accessors.
    /** Returns the floor of the number of seconds in the interval. Note that
      * if the number is negative, the absolute value of the seconds field is
      * GREATER than the absolute value of the interval.
      * @memo Get seconds field.
      * @return The integer floor of the interval in seconds.
      */
    long GetS() const {
        if (mSec >=0) return (long)mSec;
	return (long) mSec - 1;
    }

    /** Offset in nanoseconds from the seconds field defined by GetS().
      * Note that the nanosecond field is always positive.
      * @memo Get nano-second.
      * @return Number of nanoseconds offset from the seconds field.
      */
    ulong_t GetN() const {
        return (ulong_t) ((mSec - (double) GetS()) * DSTONS + 0.5);}

    /**  Return the interval time in seconds.
      */
    double GetSecs() const {
        return mSec;}

    /** Set second
     */
    void   SetS(ulong_t s) {mSec = s;}

    /** Set nano-second
     */
    void   SetN(ulong_t n) {mSec = (double) GetS() + (DSTONS * (double) n);}

    //------------------------------------  Data members.
  private:
    /** Get interval in an easily manipulable representation.
     */
#ifdef TICKS
    long long ticks(void) const {return (long long) (mSec*DSTONS + 0.5);}
#else
    double ticks(void) const {return mSec;}
#endif
    double mSec;     // seconds
};

/**  @name Interval Functions
  *  The non-member interval functions allow further manipulation of 
  *  time interval data.
  *  @memo Manipulation of time intervals.
  *  @author John G Zweizig
  *  @version 1.1; Last modified June 15, 1999
  */
//@{ 

/**  Two intervals are added together producing an Interval.
  *  @memo Add two intervals.
  */
inline Interval operator +(const Interval& t, const Interval& dt) {
    Interval r = t;
    r += dt;
    return r;}

/**  Difference of two intervals.
 */
inline Interval operator -(const Interval& t, const Interval& dt) {
    Interval r = t;
    r -= dt;
    return r;}

/**  Multiply an Interval by a double.
  *  The time interval is multiplied by an arbitrary number. Both the
  *  Interval*double and double*Interval operators exist.
  */
inline Interval operator *(const Interval& t, double x) {
    Interval r = t;
    r *= x;
    return r;}

inline Interval operator *(double x, const Interval& t) {
    Interval r = t;
    r *= x;
    return r;}

/**  Divide a time Interval by a double.
  *  The time interval is divided by a number.
  */ 
inline Interval operator /(const Interval& t, const double x) {
    Interval r = t;
    r /= x;
    return r;}

//  Operator function for:  ostream << Interval
std::ostream &operator <<(std::ostream &out, const Interval& t);

//@}

#endif // INTERVAL_HH
