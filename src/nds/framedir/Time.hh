#ifndef TIME_HH
#define TIME_HH

#include <iosfwd>

   class Interval;

/**  The time class contains a GPS time in seconds and nanoseconds since 
  *  midnight, January 6, 1980. The Time class is closely coupled with the 
  *  Interval class which represents time intervals. All basic arithmetic 
  *  operations are defined between the two classes. Ancillary (non-member) 
  *  functions convert the GPS time to other representations
  *  @memo GPS time container class.
  *  @author John G. Zweizig
  *  @version 1.1; Modified: August 23, 1999
  */
   class Time
   {
   public:
      typedef unsigned long ulong_t;
   
    /** A Time instance is created with no initialization performed.
      * @memo Default constructor
      */
      Time(void);
   
    /** GPS time is constructed from seconds and nanoseconds. If the 
      * nanosecond field isn't specified, it is assumed to be zero.
      * @memo Constructor from seconds and nanoseconds.
      * @param sec Number of GPS seconds since midnight, Jan 6, 1980.
      * @param nsec Number of nanoseconds since last GPS second.
      */
      Time(ulong_t sec, ulong_t nsec=0);
   
    /// Copy constructor
      Time(const Time& t);
   
    //----------------------------------  Overload various operators.
    /// Assignment operator
      Time& operator=(const Time&); 
   
    ///  Add an Interval to a Time.
      Time operator+=(const Interval& dt);
   
    ///  Subtract a Time rhs.
      Interval operator -(const Time& dt) const;
   
    ///  Subtract a constant rhs
      Time operator-=(const Interval& dt);
   
    ///  Compare to zero
      bool operator!(void) const;
   
    ///  Compare times (equality)
      bool operator==(const Time& rhs) const;
   
    ///  Compare times (inequality)
      bool operator!=(const Time& rhs) const;
   
    ///  Compare times (Greater or Equal)
      bool operator>=(const Time& rhs) const;
   
    ///  Compare times (Less or equal)
      bool operator<=(const Time& rhs) const;
   
    ///  Compare times (Greater)
      bool operator>(const Time& rhs) const;
   
    ///  Compare times (Less than)
      bool operator<(const Time& rhs) const;
   
    //------------------------------------  Accessors.
    /**  Get the GPS second boundary.
      *  @memo Number of nanoseconds.
      *  @return Then number of nanoseconds since a GPS secon.
      */
      ulong_t getS(void) const;
   
    /**  Get the number of nanoseconds since a GPS second boundary.
      *  @memo Number of nanoseconds.
      *  @return Then number of nanoseconds since a GPS secon.
      */
      ulong_t getN(void) const;
   
    /**  Get the GPS time in seconds since t0.
      *  @memo GPS seconds since t0.
      *  @return The number of GPS seconds.
      */
      double  totalS(void) const;
   
    /**  Get the GPS time in nanoseconds since t0.
      *  @memo GPS nanoseconds since t0.
      *  @return The number of GPS nanoseconds.
      */
      double  totalNS(void) const;
   
    /**  Get the fraction of a second after an integer number of GPS seconds.
      *  @memo Get the fractional seconds.
      */
      double fracS(void) const;
   
    /** Set the number of seconds past the start of the second GPS epoch.
      * @memo set seconds field.
      */
      void setS(ulong_t s) {
         mSec = s;}
   
    /** Set the number of nanoseconds offset from the seconds field.
      * @memo Set nanosecon field.
      */
      void setN(ulong_t n);
   
    //------------------------------------  Data members.
   private:
      ulong_t mSec;     // seconds
      ulong_t mNsec;    // nano-seconds
   };

//----------------------------------------  Non-member operators & functions 
/** @name Time Functions
  * Functions for the translation and manipulation of Time objects.
  * @memo Time translation and manipulation functions.
  * @author John G Zweizig
  * @version 1.2; Last Modified: September 29, 1999
  */
//@{

/**  An interval is added to a GPS time resulting in a GPS Time.
  *  @memo Add an interval to a GPS time.
  *  @return Starting time offset by the time increment.
  *  @param t  Starting Time.
  *  @param dt Signed time increment.
  */
   Time operator +(const Time& t, const Interval& dt);

/**  An interval is added to a GPS time resulting in a GPS Time.
  *  @memo Add an interval to a GPS time.
  *  @return Starting time offset by the time increment.
  *  @param t  Starting Time.
  *  @param dt Signed time increment.
  */
   Time operator +(const Interval& dt, const Time& t);

/**  An Interval is subtracted from a GPS Time resulting in a GPS Time.
  *  @memo subtract an interval from a GPS time.
  */
   Time operator -(const Time& t, const Interval& dt);

/**  The date and time (to the nearest second) are printed to the 
  *  output stream.
  *  @memo Output formatted Time to ostream.
  */
   std::ostream& operator<<(std::ostream &out, const Time& t);

/**  Two times are compared for equality within dT nanoseconds.
  *  @memo Compare two times for almost equal.
  *  @return true if the two times are equal within the specified tolerance.
  *  @param t1 First time.
  *  @param t2 Second time.
  *  @param dT Maximum time difference in namoseconds.
  */
   bool Almost(const Time& t1, const Time& t2, Time::ulong_t dT=1);

/**  The current system time (UTC) is converted to a GPS time and returned 
  *  a time object. Note that since the system clock is used, the accuracy
  *  is highly system dependent and the the time returned is at best an 
  *  approximate current time.
  *  @memo Approximate GPS time.
  *  @return The approximate system time as a Time object.
  */
   Time Now(void);

/**  The Time argument is converted to a UTC date and time string and stored
  *  in 'str'. The optional format string describes how the string is to be 
  *  converted. The format characters cause the appropriate date information 
  *  to be inserted. If no format is specified, "%s:%n" is assumed. The 
  *  following table lists the format codes and their meaning:
  *  \begin{tabular}{clclcl}
  *  Format & Meaning    & Format & Meaning     & Format & Meaning \\
  *  %d & Day of month   & %D & Day of year     & %H & Hour of day \\ 
  *  %m & Month number   & %M & Month name      & %n & Nanoseconds \\
  *  %N & Minute in hour & %S & seconds         & %s & GPS seconds \\ 
  *  %w & Week day       & %y & Year in century & %Y & Year        \\
  *  %Z & Time zone   \end{tabular}
  *  @memo Convert the specified time to a date string.
  *  @return Returns the output string pointer or Null upon failure.
  *  @param t Time to be converted
  *  @param str Character array into which the string will be stored.
  *  @param format Format string.
  */
   char* 
   TimeStr(const Time& t, char *str, const char *format=0);

/**  The Time argument is converted to a local date and time string and 
  *  stored in 'str'. The format is specified as for TimeStr().
  *  @memo Convert the specified time to a local time string.
  *  @return Returns the output string pointer or Null upon failure.
  *  @param t Time to be converted
  *  @param str Character array into which the string will be stored.
  *  @param format Format string.
  */
   char* 
   LocalStr(const Time& t, char *str, const char *format);

/**  Convert GPS Time to a UTC (e.g. Unix time_t) time
  *  @memo convert to UTC time.
  *  @param t Time to be converted to UTC
  *  @return UTC seconds in Unix time_t format.
  */
   Time::ulong_t getUTC(const Time& t);

/**  Get the number of leap seconds for a given GPS time.
  *  @memo Number of leap seconds.
  *  @return Number of leap seconds in effect at the specified time.
  *  @param t Time at which leap seconds are evaluated.
  */
   Time::ulong_t LeapS(const Time& t);

/**  Get a Time from a UTC (e.g. Unix time_t) time.
  *  @memo UTC time from Unix time.
  *  @param t t Time to convert from UTC.
  *  @return GPS Time.
  */
   Time fromUTC(Time::ulong_t t);

//@}

//----------------------------------------  Compare to zero
   inline bool 
   Time::operator!(void) const {
      return (!mSec && !mNsec);
   }

//----------------------------------------  Compare (equal, greater, less)
   inline bool 
   Time::operator==(const Time& rhs) const {
      return (mSec == rhs.mSec && mNsec == rhs.mNsec);
   }

   inline bool 
   Time::operator>(const Time& rhs) const {
      return (mSec > rhs.mSec || (mSec == rhs.mSec && mNsec > rhs.mNsec));
   }

   inline bool 
   Time::operator<(const Time& rhs) const {
      return (mSec < rhs.mSec || (mSec == rhs.mSec && mNsec < rhs.mNsec));
   }

//----------------------------------------  Compare (derived from above)
   inline bool 
   Time::operator!=(const Time& rhs) const {
      return !operator==(rhs);
   }

   inline bool 
   Time::operator>=(const Time& rhs) const {
      return !(operator<(rhs));
   }

   inline bool 
   Time::operator<=(const Time& rhs) const {
      return !(operator>(rhs));
   }

//----------------------------------------  Accessors.
   inline Time::ulong_t 
   Time::getS(void) const {
      return mSec;
   }

   inline Time::ulong_t 
   Time::getN(void) const {
      return mNsec;
   }

#endif  //  TIME_HH
