#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
extern int errno;
#include "tai.h"
#include "leapsecs.h"

struct tai *leapsecs = 0;
int leapsecs_num = 0;

int leapsecs_read()
{
  int fd;
  struct stat st;
  struct tai *t;
  int n;
  int i;
  struct tai u;

  char *inv, leapfile[24];

  inv = getenv("DVPATH=");
  if (inv == NULL)
    strcpy(leapfile, "leapsecs.dat");
  else
    sprintf(leapfile, "%s/leapsecs.dat", inv);
  //printf ( "leapfile %s\n", leapfile  );
  fd = open(leapfile, O_RDONLY | O_NDELAY);
  if (fd == -1) {
    if (errno != ENOENT) return -1;
    if (leapsecs) free(leapsecs);
    leapsecs = 0;
    leapsecs_num = 0;
    return 0;
  }

  if (fstat(fd,&st) == -1) { close(fd); return -1; }

  t = (struct tai *) malloc(st.st_size);
  if (!t) { close(fd); return -1; }

  n = read(fd,(char *) t,st.st_size);
  close(fd);
  if (n != st.st_size) { free(t); return -1; }

  n /= sizeof(struct tai);

  for (i = 0;i < n;++i) {
    tai_unpack((char *) &t[i],&u);
    t[i] = u;
  }

  if (leapsecs) free(leapsecs);

  leapsecs = t;
  leapsecs_num = n;
}
