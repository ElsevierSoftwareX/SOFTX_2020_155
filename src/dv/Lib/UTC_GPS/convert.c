#include <stdio.h>
#include <stdlib.h> /* Pick up declaration of exit() */
#include "leapsecs.h"
#include "tai.h"
#include "caltime.h"

struct caltime ctin;
struct caltime ctout;
struct gps gps;


int
main(int argc, char* argv[])
{
  long mjd;
  char stin[100],stout[100];

  if (leapsecs_init() == -1) {
    fprintf(stderr,"fatal: unable to init leapsecs\n");
    exit(111);
  }

 /*----- enter time in UTC ------------*/
    ctin.date.year  = 1999;
    ctin.date.month = 8;       /* [1,12] */
    ctin.date.day   = 27;       /* [1,31] */
    ctin.hour       =5;         /* [0,23] */
    ctin.minute     =11;        /* [0,59] */
    ctin.second     =0;        /* [0,60] */
    ctin.offset     =0;         /* offset in minutes from UTC [-5999,5999] */
 /*-----------------------------------*/

    caltime_fmt(stin,&ctin);      /* writes ctin in ISO format */
    printf("Input Time:  %s UTC  \n",stin);

    mjd=caldate_mjd(&ctin.date); /* modified Julian day */
    printf("MJD = %ld \n",mjd);

 /* -----  convert ctin to gps -------*/
   utc_to_gps(&ctin,&gps);
   printf("FrameL->GTimeS: %llu sec \n",gps.sec);
   printf("FrameL->ULeapS: %d sec \n",gps.leap);

 /*-----  convert gps back to utc---*/
   gps_to_utc(&gps,&ctout);
   caltime_fmt(stout,&ctout);
   printf("Output Time:  %s UTC \n ",stout);

  exit(0);
}
