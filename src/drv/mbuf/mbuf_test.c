#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include "mbuf.h"

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

  int len = 64 * 1024 * 1024;

  if ((fd = open ("/dev/mbuf", O_RDWR | O_SYNC)) < 0) {
      perror ("open");
      exit (-1);
  }

  struct mbuf_request_struct req = {len, "alx"};
  ioctl (fd, IOCTL_MBUF_INFO, &req);
  ioctl (fd, IOCTL_MBUF_ALLOCATE, &req);

  vadr = mmap(0, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0*getpagesize());
  printf("mmap() returned addr=%lx\n", vadr);
  if (vadr == MAP_FAILED) {
          perror ("mmap");
          exit (-1);
  }

#if 1
  //ioctl (fd, IOCTL_MBUF_INFO, &req);
  //unsigned char *a = (unsigned char *)vadr + 0x12200cc;
  //float f = ((float *)a)[0];
  //printf("%f\n", f);

  for (i = 0; i < len; i++)
	  *((unsigned char *)vadr  + i) = i;
#endif
   sleep(100);

  //printf("0x%x 0x%x\n", vadr[0x10], vadr[0x11]);
  //vadr[0x11] = 0xdeadbeef;
  //printf("0x%x 0x%x\n", vadr[0x10], vadr[0x11]);
#if 0
  if ((vadr[0]!=0xaffe0000) || (vadr[1]!=0xbeef0000)
      || (vadr[len/sizeof(int)-2]!=(0xaffe0000+len/sizeof(int)-2))
      || (vadr[len/sizeof(int)-1]!=(0xbeef0000+len/sizeof(int)-2)))
  {
       printf("0x%x 0x%x\n", vadr[0], vadr[1]);
       printf("0x%x 0x%x\n", vadr[len/sizeof(int)-2], vadr[len/sizeof(int)-1]);
  }
#endif
  
#if 0
  kadr = mmap(0, len, PROT_READ|PROT_WRITE, MAP_SHARED| MAP_LOCKED, fd, len);
  
  if (kadr == MAP_FAILED)
  {
          perror("mmap");
          exit(-1);
  }

  if ((kadr[0]!=0xdead0000) || (kadr[1]!=0xbeef0000)
      || (kadr[len / sizeof(int) - 2] != (0xdead0000 + len / sizeof(int) - 2))
      || (kadr[len / sizeof(int) - 1] != (0xbeef0000 + len / sizeof(int) - 2)))
  {
      printf("0x%x 0x%x\n", kadr[0], kadr[1]);
      printf("0x%x 0x%x\n", kadr[len / sizeof(int) - 2], kadr[len / sizeof(int) - 1]);
  }
#endif
  
  close(fd);
  return(0);
}
