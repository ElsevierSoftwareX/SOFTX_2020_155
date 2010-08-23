#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include "symmetricom.h"

/* this is a test program that opens the mbuf_drv.
   It reads out values of the kmalloc() and vmalloc()
   allocated areas and checks for correctness.

   Please install driver with "sudo make install" first.
*/

int main (void)
{
  int fd, i;
  unsigned int *vadr;
  unsigned int *kadr;

  if ((fd = open ("/dev/symmetricom", O_RDWR | O_SYNC)) < 0) {
      perror ("open");
      exit (-1);
  }

  int req = 0;
  ioctl (fd, IOCTL_SYMMETRICOM_STATUS, &req);
  printf("%d\n", req);
  return(0);
}
