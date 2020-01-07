#include "leapsecs.h"
#include "tai.h"
#include "caltime.h"


void utc_to_gps(ct,g)
struct caltime *ct;
struct gps *g;
{
  long day;
  long s;
  struct tai t;
  int leap;

  day = caldate_mjd(&ct->date);
  s = ct->hour * 60 + ct->minute;
  s = (s - ct->offset) * 60 + ct->second;

  t.x = day * 86400ULL + 4611686014920671114ULL + (long long) s; 
  leap = leapsecs_add(&t,ct->second == 60);
  g->sec = t.x - 4611686018743352723ULL;
  g->leap = leap - 9;
}
