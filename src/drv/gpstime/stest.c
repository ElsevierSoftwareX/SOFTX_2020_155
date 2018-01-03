#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include "gpstime.h"

/* this is a test program that opens the mbuf_drv.
   It reads out values of the kmalloc() and vmalloc()
   allocated areas and checks for correctness.

   Please install driver with "sudo make install" first.
*/

int main (void)
{
  int fd;

  if ((fd = open ("/dev/gpstime", O_RDWR | O_SYNC)) < 0) {
      perror ("open");
      exit (-1);
  }

  unsigned long req = 0;
  unsigned long t[3];
  ioctl (fd, IOCTL_SYMMETRICOM_STATUS, &req);
  printf("%ld\n", req);
  ioctl (fd, IOCTL_SYMMETRICOM_TIME, &t);
  printf("%ds %du %dn\n", t[0], t[1], t[2]);
  return(0);
}
