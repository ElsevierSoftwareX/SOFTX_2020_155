#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* for strcpy() */
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

  /* FILENAME_MAX is defined in stdio.h */
  char *inv, leapfile[FILENAME_MAX];

  fprintf(stderr, "leapsecs_read()\n") ;
  inv = getenv("DVPATH=");
  if (inv == NULL)
    strcpy(leapfile, "leapsecs.dat");
  else
  {
    fprintf(stderr, "  inv = %s\n", inv) ;
    sprintf(leapfile, "%s/leapsecs.dat", inv);
   }
  //printf ( "leapfile %s\n", leapfile  );
  fprintf(stderr, "  Opening %s\n", leapfile) ;
  fd = open(leapfile, O_RDONLY | O_NDELAY);
  if (fd == -1) 
  {
    fprintf(stderr, "  Open of %s failed\n", leapfile) ;
    if (errno != ENOENT) 
    {
       fprintf(stderr, "leapsecs_read() returning -1, line %d\n", __LINE__) ;
       return -1;
    }
    if (leapsecs) 
       free(leapsecs);
    leapsecs = 0;
    leapsecs_num = 0;
    fprintf(stderr, "leapsecs_read() returning 0\n") ;
    return 0;
  }

  if (fstat(fd,&st) == -1) 
  { 
      close(fd); 
       fprintf(stderr, "leapsecs_read() returning -1, line %d\n", __LINE__) ;
      return -1; 
  }

  t = (struct tai *) malloc(st.st_size);
  if (!t) 
  { 
     close(fd); 
       fprintf(stderr, "leapsecs_read() returning -1, line %d\n", __LINE__) ;
     return -1; 
  }

  n = read(fd,(char *) t,st.st_size);
  close(fd);
  if (n != st.st_size) 
  { 
     free(t); 
       fprintf(stderr, "leapsecs_read() returning -1, line %d\n", __LINE__) ;
     return -1; 
  }

  n /= sizeof(struct tai);

  fprintf(stderr, "  calling tai_unpack()\n") ;
  for (i = 0;i < n;++i) {
    tai_unpack((char *) &t[i],&u);
    t[i] = u;
  }

  if (leapsecs) 
     free(leapsecs);

  leapsecs = t;
  leapsecs_num = n;
  fprintf(stderr, "leapsecs_read() returning 0, line %d leapsecs_num = %d\n", __LINE__, leapsecs_num) ;
  return 0; /* Added by JCB, as no return value was defined. */
}
