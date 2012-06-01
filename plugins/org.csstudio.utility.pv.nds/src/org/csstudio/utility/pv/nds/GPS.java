package org.csstudio.utility.pv.nds;

import java.util.*;

/*
  JDK 1.1 `DATE' CLASS DESCRIPTION:  

  The class Date represents a specific instant in time, with millisecond precision. 

  Prior to JDK 1.1, the class Date had two additional functions. It allowed the interpretation of dates as
  year, month, day, hour, minute, and second values. It also allowed the formatting and parsing of date
  strings. Unfortunately, the API for these functions was not amenable to internationalization. As of JDK 1.1,
  the Calendar class should be used to convert between dates and time fields and the DateFormat
  class should be used to format and parse date strings. The corresponding methods in Date are
  deprecated. 

  Although the Date class is intended to reflect coordinated universal time (UTC), it may not do so exactly,
  depending on the host environment of the Java Virtual Machine. Nearly all modern operating systems
  assume that 1 day = 24 Å◊ 60 Å◊ 60 = 86400 seconds in all cases. In UTC, however, about once every year
  or two there is an extra second, called a "leap second." The leap second is always added as the last
  second of the day, and always on December 31 or June 30. For example, the last minute of the year 1995
  was 61 seconds long, thanks to an added leap second. Most computer clocks are not accurate enough to
  be able to reflect the leap-second distinction. 

  Some computer standards are defined in terms of Greenwich mean time (GMT), which is equivalent to
  universal time (UT). GMT is the "civil" name for the standard; UT is the "scientific" name for the same
  standard. The distinction between UTC and UT is that UTC is based on an atomic clock and UT is based
  on astronomical observations, which for all practical purposes is an invisibly fine hair to split. Because the
  earth's rotation is not uniform (it slows down and speeds up in complicated ways), UT does not always flow
  uniformly. Leap seconds are introduced as needed into UTC so as to keep UTC within 0.9 seconds of UT1,
  which is a version of UT with certain corrections applied. There are other time and date systems as well;
  for example, the time scale used by the satellite-based global positioning system (GPS) is synchronized to
  UTC but is not adjusted for leap seconds. An interesting source of further information is the U.S. Naval
  Observatory, particularly the Directorate of Time at: 

  http://tycho.usno.navy.mil 

  and their definitions of "Systems of Time" at: 

  http://tycho.usno.navy.mil/systime.html 

  In all methods of class Date that accept or return year, month, date, hours, minutes, and seconds
  values, the following representations are used: 

  A year y is represented by the integer y - 1900. 
  A month is represented by an integer form 0 to 11; 0 is January, 1 is February, and so forth; thus
  11 is December. 
  A date (day of month) is represented by an integer from 1 to 31 in the usual manner. 
  An hour is represented by an integer from 0 to 23. Thus, the hour from midnight to 1 a.m. is hour 0,
  and the hour from noon to 1 p.m. is hour 12. 
  A minute is represented by an integer from 0 to 59 in the usual manner. 
  A second is represented by an integer from 0 to 60; the value 60 occurs only for leap seconds and
  even then only in Java implementations that actually track leap seconds correctly. 

  In all cases, arguments given to methods for these purposes need not fall within the indicated ranges; for
  example, a date may be specified as January 32 and is interpreted as meaning February 1. 

*/

/*
  JDK 1.1 `CALENDAR' CLASS DESCRIPTION:

  public abstract class Calendar 
  extends Object 
  implements Serializable, Cloneable 

  Calendar is an abstract base class for converting between a Date object and a set of integer fields such as
  YEAR, MONTH, DAY, HOUR, and so on. (A Date object represents a specific instant in time with millisecond
  precision. See java.util.Date for information about the Date class.) 

  Subclasses of Calendar interpret a Date according to the rules of a specific calendar system. The JDK
  provides one concrete subclass of Calendar: GregorianCalendar. Future subclasses could represent
  the various types of lunar calendars in use in many parts of the world. 

  Like other locale-sensitive classes, Calendar provides a class method, getInstance, for getting a
  generally useful object of this type. Calendar's getInstance method returns a
  GregorianCalendar object whose time fields have been initialized with the current date and time: 

  Calendar rightNow = Calendar.getInstance();
      

  A Calendar object can produce all the time field values needed to implement the date-time formatting for a
  particular language and calendar style (for example, Japanese-Gregorian, Japanese-Traditional). 

  When computing a Date from time fields, two special circumstances may arise: there may be insufficient
  information to compute the Date (such as only year and month but no day in the month), or there may be
  inconsistent information (such as "Tuesday, July 15, 1996" -- July 15, 1996 is actually a Monday). 

  Insufficient information. The calendar will use default information to specify the missing fields. This may vary by
  calendar; for the Gregorian calendar, the default for a field is the same as that of the start of the epoch: i.e.,
  YEAR = 1970, MONTH = JANUARY, DATE = 1, etc. 

  Inconsistent information. If fields conflict, the calendar will give preference to fields set more recently. For
  example, when determining the day, the calendar will look for one of the following combinations of fields. The
  most recent combination, as determined by the most recently set single field, will be used. 

  MONTH + DAY_OF_MONTH
  MONTH + WEEK_OF_MONTH + DAY_OF_WEEK
  MONTH + DAY_OF_WEEK_IN_MONTH + DAY_OF_WEEK
  DAY_OF_YEAR
  DAY_OF_WEEK + WEEK_OF_YEAR
      

  For the time of day: 

  HOUR_OF_DAY
  AM_PM + HOUR
      

  Note: for some non-Gregorian calendars, different fields may be necessary for complete disambiguation. For
  example, a full specification of the historial Arabic astronomical calendar requires year, month, day-of-month
  and day-of-week in some cases. 

  Note: There are certain possible ambiguities in interpretation of certain singular times, which are resolved in the
  following ways: 

  1.24:00:00 "belongs" to the following day. That is, 23:59 on Dec 31, 1969 < 24:00 on Jan 1, 1970 <
  24:01:00 on Jan 1, 1970 
  2.Although historically not precise, midnight also belongs to "am", and noon belongs to "pm", so on the
  same day, 12:00 am (midnight) < 12:01 am, and 12:00 pm (noon) < 12:01 pm 

  The date or time format strings are not part of the definition of a calendar, as those must be modifiable or
  overridable by the user at runtime. Use java.text.DateFormat to format dates. 

  Calendar provides an API for field "rolling", where fields can be incremented or decremented, but wrap
  around. For example, rolling the month up in the date "September 12, 1996" results in "October 12, 1996". 

  Calendar also provides a date arithmetic function for adding the specified (signed) amount of time to a
  particular time field. For example, subtracting 5 days from the date "September 12, 1996" results in "September 7,
  1996". 

*/


public class GPS extends GregorianCalendar {
  public GPS (long gpsTimeSeconds) {
    setTimeInMillis (gpsTimeSeconds * 1000);
    computeFields ();
  }

  public GPS (int newYear, int newMonth, int newDay, int newHour, int newMinute, int newSecond) {
    super (newYear, newMonth, newDay, newHour, newMinute, newSecond);
    computeTime ();
  }

  public long getTimeInSeconds () {
    return getTimeInMillis () / 1000;
  }
  /*
  public final static int gps (int year, int month, int day, int hour, int minute, int second) 
    throws IllegalArgumentException {
    return second;
  }
  */
  /*
    As of 1 January 1999,
    TAI is ahead of UTC   by 32 seconds.
    TAI is ahead of GPS   by 19 seconds.
    GPS is ahead of UTC   by 13 seconds.
  */

  /*
  public final static int utcToGps (int utc) {
    return utc + 13 - 315961200;
  }

  public final static int gpsToUtc (int gps) {
    return gps - 13 + 315961200;
  }

  // The GPS epoch is 0000 UT (midnight) on January 6, 1980. 
  */  
}
