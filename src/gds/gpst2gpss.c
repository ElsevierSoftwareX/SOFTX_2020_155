static char *versionId = "Version $Id$" ;
#include "gpstime.h"

   void gpstime_to_gpssec (const gpstime_t* gt, tais_t* t)
   {
      caldate_t		cd;
      long 		day;
      long		s;
   
      cd.year = gt->year;
      cd.month = 1;
      cd.day = 1;
   
      day = (gt->yearday - 1) + caldate_mjd(&cd);
   
      s = gt->hour * 60 + gt->minute;
      s = s  * 60 + gt->second;
   
      t->x = day * 86400ULL + 4611686014920671104ULL + (long long) s;
   /* this is the number of atomic clock ticks since 17 Nov 1858,
     that is, TAI seconds */
   /* now, take it to the gps epoch of 1980-01-06 00:00:00 UTC*/
      t->s = t->x - 4611686018743352704ULL; 
   
   }
