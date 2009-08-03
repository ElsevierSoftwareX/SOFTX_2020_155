#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include "mbuf.h"

int main (void)
{
  int fd;
  unsigned int *vadr;
  unsigned int *kadr;


  if ((fd = open ("node", O_RDWR | O_SYNC)) < 0) {
      perror ("open");
      exit (-1);
  }

  struct mbuf_request_struct req = {0, "alx"};
  ioctl (fd, IOCTL_MBUF_INFO, &req);
}

