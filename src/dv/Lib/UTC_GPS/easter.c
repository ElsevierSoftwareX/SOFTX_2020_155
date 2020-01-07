#include <stdio.h>
#include <stdlib.h> /* Pick up declaration of exit() */
#include "caldate.h"

int easter(cd)
struct caldate *cd; /* cd->year is filled in */
{
  long y;
  long c;
  long t;
  long j;
  long n;

  y = cd->year;
  if (y < 1) return 0;

  c = (y / 100) + 1;
  t = 210 - (((c * 3) / 4) % 210);
  j = y % 19;
  n = 57 - ((14 + j * 11 + (c * 8 + 5) / 25 + t) % 30);
  if ((n == 56) && (j > 10)) --n;
  if (n == 57) --n;
  n -= ((((y % 28) * 5) / 4 + t + n + 2) % 7);

  if (n < 32) { cd->month = 3; cd->day = n; }
  else { cd->month = 4; cd->day = n - 31; }

  return 1;
}

char *dayname[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" } ;

char out[101];

int
main(argc,argv)
int argc;
char **argv;
{
  struct caldate cd;
  long day;
  int weekday;
  int yearday;
  int i;

  while (*++argv) {
    cd.year = atoi(*argv);
    if (!easter(&cd)) exit(1);
    day = caldate_mjd(&cd);
    caldate_frommjd(&cd,day,&weekday,&yearday);
    if (caldate_fmt((char *) 0,&cd) + 1 >= sizeof out) exit(1);
    out[caldate_fmt(out,&cd)] = 0;
    printf("%s %s  yearday %d  mjd %ld\n",dayname[weekday],out,yearday,day);
  }
  exit(0);
}
