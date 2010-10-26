#include <stdio.h>
#include "leapsecs.h"
#include "tai.h"
#include "caltime.h"

struct tai t;
struct caltime ctin;
struct caltime ctout;


main()
{
  if (leapsecs_init() == -1) {
    fprintf(stderr,"utcnow: fatal: unable to init leapsecs\n");
    exit(111);
  }
  /*----- enter time in UTC ------------*/
    ctin.date.year  = 1970;
    ctin.date.month = 1;
    ctin.date.day   = 1;
    ctin.hour       =0;
    ctin.minute     =0;
    ctin.second     =0;
    ctin.offset     =0;  
  /*-----------------------------------
   ----- now convert ctin to t -------*/

    caltime_tai(&ctin,&t);
     printf("%llu\n",t.x-4611686018743352714ULL);

  /*----- then convert t back to utc---*/
  caltime_utc(&ctout,&t,(int *) 0,(int *) 0);

  printf("%d-%02d-%02d %02d:%02d:%02d\n"
    ,ctout.date.year
    ,ctout.date.month
    ,ctout.date.day
    ,ctout.hour
    ,ctout.minute
    ,ctout.second
    );

  exit(0);
}
