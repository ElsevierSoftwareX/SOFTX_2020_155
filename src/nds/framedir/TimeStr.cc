//
//    Time associated functions
//
#include "PConfig.h"
#include <time.h>
#ifndef HAVE_CLOCK_GETTIME
#include <sys/time.h>
#endif
#include <stdlib.h>
#include <iostream>
#include "Time.hh"
#include "Interval.hh"

   using namespace std;

//--------------------------------------  difference between GPS and ITC
   static Time::ulong_t GPS2UTC(const Time& t0) {
    // Number of seconds in a day
      const Time::ulong_t secpd = 24*60*60;
    // offset from Unix UTC to GPS Jan 1,1970-Jan 6,1980 = 10y + 5d + 2 leap-d
      const Time::ulong_t offset0 = (10*365+7)*secpd;
   
      return offset0 - LeapS(t0);
   }

   Time::ulong_t LeapS(const Time& t0) {
    // Number of seconds in a day
      const Time::ulong_t secpd = 24*60*60;
   
    // Offset in days from the t=0
      const int nLeaps=13;
      const Time::ulong_t leapd[nLeaps] = {
         // 1980 + Std years + Leap years + Jan-Jul
         361                          +  181,  // Jul 1, 1981
         361 +      365               +  181,  // Jul 1, 1982
         361 +    2*365               +  181,  // Jul 1, 1983
         361 +    3*365  +     366    +  181,  // Jul 1, 1985
         361 +    6*365  +     366,            // Jan 1, 1988
         361 +    7*365  +   2*366,            // Jan 1, 1990
         361 +    8*365  +   2*366,            // Jan 1, 1991
         361 +    8*365  +   3*366    +  181,  // Jul 1, 1992
         361 +    9*365  +   3*366    +  181,  // Jul 1, 1993
         361 +   10*365  +   3*366    +  181,  // Jul 1, 1994
         361 +   12*365  +   3*366,            // Jan 1, 1996
         361 +   12*365  +   4*366    +  181,  // Jul 1, 1997
         361 +   14*365  +   4*366             // Jan 1, 1999
         };
   
    //----------------------------------  Count the leap seconds in effect
      Time::ulong_t r(0);
      Time::ulong_t nDay = t0.getS()/secpd;
      for (int i=0 ; i<nLeaps ; i++) {
         if (nDay > leapd[i]) r++;
      }
      return r;
   }

//-------------------------------------- << operator for output
   ostream& operator<<(ostream &out, const Time& t) {
      char string[40];
      TimeStr(t, string, "%s:%n");
      out << string;
      return out;
   }

//--------------------------------------  Convert to a Unix time_t
   Time::ulong_t
   getUTC(const Time& t) {
      return t.getS() + GPS2UTC(t);
   }

//--------------------------------------  Convert from a Unix time_t
   Time
   fromUTC(Time::ulong_t t) {
      Time::ulong_t t0 = GPS2UTC(Time(0));
      if (t <= t0) 
         return Time(0, 0);
      Time::ulong_t t1 = GPS2UTC(Time(t-t0));
      return Time(t-t1, 0);
   }

//--------------------------------------  Get the approximate current time
   Time
   Now(void) {
   #if !defined(HAVE_CLOCK_GETTIME)
      struct timeval now;
      if (gettimeofday (&now, 0) != 0) {
         return Time (0, 0);
      }
      return fromUTC(now.tv_sec) + Interval (now.tv_usec / 1E6);
   #else
      struct timespec 	now;
      if (clock_gettime (CLOCK_REALTIME, &now) != 0) {
         return Time(0, 0);
      }
      return fromUTC(now.tv_sec) + Interval (now.tv_nsec / 1E9);
   #endif
   }

//--------------------------------------  String conversion function
   static char* puti(char* s, unsigned int i, int w=0, char pad=' ');
   static const char* Mon[12] = {"January", "February", "March", "April", 
   "May", "June", "July", "August", "September", 
   "October", "November", "December"};

   inline void scopy (const char* in, char*& out, int max = 0) {
      int num = max;
      for (const char* p=in ; *p && (!max || num) ; --num) *out++ = *p++;
   }

   static char*
   FmtTime(struct tm* date, const Time& t, char* s, const char* fmt) {
      char* str = s;
      char* tempf;
      const char* form = "%s:%n";
      if (fmt)    form = fmt;
   
      for ( ; *form ; ) {
         if (*form == '%') {
            form++;
         
         //--------------------------  Select ad character
            char pad = ' ';
            if (*form == '0') {
               form++;
               pad = '0';
            }
         
         //--------------------------  Get field width
            int width = 0;
            if (*form >= '0' && *form <= '9') {
               width = strtol(form, const_cast<char**>(&form),0);
            }
         
         //--------------------------  Interpret the format field.
            switch (*form++) {
               case 'y':
                  if (!width) width = 2;
                  str=puti(str, date->tm_year%100, width, '0');
                  break;
               case 'Y':
                  str=puti(str, date->tm_year+1900, width, pad);
                  break;
               case 'm':
                  str=puti(str, date->tm_mon+1, width, pad);
                  break;
               case 'M':
                  scopy(Mon[date->tm_mon],str, width);
                  break;
               case 'd':
                  str=puti(str, date->tm_mday, width, pad);
                  break;
               case 'D':
                  str=puti(str, date->tm_yday, width, pad);
                  break;
               case 'H':
                  str=puti(str, date->tm_hour, width, pad);
                  break;
               case 'N':
                  if (!width) width = 2;
                  str=puti(str, date->tm_min, width, '0');
                  break;
               case 'n':
                  if (!width) width = 9;
                  str=puti(str, t.getN(), width, '0');
                  break;
               case 'S':
                  if (!width) width = 2;
                  str=puti(str, date->tm_sec, width, '0');
                  break;
               case 's':
                  str=puti(str, t.getS(), width, pad);
                  break;
               case 'w':
                  str=puti(str, date->tm_wday, width, pad);
                  break;
               case 'Z':
                  scopy("UTC",str);
                  break;
               default:
                  return 0;
            }
         } 
         else if (*form == '\\') {
            form++;
            if (*form == '\\') {
               *str++ = '\\';
               form++;
            } 
            else if (*form == 'n') {
               *str++ = '\n';
               form++;
            } 
            else if (*form >= '0' && *form <= '9') {
               *str++ = strtol(form, &tempf, 0);
               form = tempf;
            } 
            else {
               *str++ = '\\';
               *str++ = *form++;
            }
         } 
         else {
            *str++ = *form++;
         }
      }
      *str = 0;
      return s;
   }

   char*
   LocalStr(const Time& t, char* str, const char* fmt) {
      time_t tSec     = getUTC(t);
      return FmtTime(localtime(&tSec), t, str, fmt);
   }

   char*
   TimeStr(const Time& t, char* str, const char* fmt) {
      time_t tSec     = getUTC(t);
      return FmtTime(gmtime(&tSec), t, str, fmt);
   }

   static char* puti(char* s, unsigned int n, int w, char pad) {
      if (n/10) s = puti(s, n/10, w-1, pad);
      else      
         for ( ; w>1 ; w--) *s++ = pad;
      *s++ = '0' + (n%10);
      return s;
   }
