#include "tai.h"
#include "leapsecs.h"
#include "caldate.h"
#include "caltime.h"

void gps_to_utc(g,ct)
struct caltime *ct;
struct gps *g;
{
  struct tai t;
  uint64 u;
  int leap=0;
  long s;

  t.x = g->sec + 4611686018743352723ULL - (g->leap+9); 
  /*  t.x = g->sec + 4611686018743352723ULL;
      leap = leapsecs_sub(&t);*/ 

  u = t.x;
  u += 58486;
  s = u % 86400ULL;

  ct->second = (s % 60) + leap; s /= 60;
  ct->minute = s % 60; s /= 60;
  ct->hour = s;

  u /= 86400ULL;
  caldate_frommjd(&ct->date,(long) (u - 53375995543064ULL),0,0);

  ct->offset = 0;
}
