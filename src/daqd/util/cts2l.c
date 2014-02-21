#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define STRUCT_SIZE 48

/* raw minute data structure is as follows:

*/
typedef struct s_48 {
  unsigned int gps;
  unsigned int pad1;
  union {int I; double D; float F; unsigned int U} min;
  union {int I; double D; float F; unsigned int U} max;
  int n; // the number of valid points used in calculating min, max, rms and mean 
  unsigned int pad2;
  double rms;
  double mean;
} s_48;

typedef struct s_40 {
  unsigned int gps;
  union {int I; double D; float F; unsigned int U} min;
  union {int I; double D; float F; unsigned int U} max;
  int n; // the number of valid points used in calculating min, max, rms and mean 
  double rms;
  double mean;
} s_40;

// output data only up to this time
unsigned int cutoff = 0xffffffff; // large gps 

// 
inline int makesSense(unsigned int i) { i < cutoff; }


/* this program only works when compiled 32-bit */

main(int argc, char *argv[])
{
	int ifd, ofd;
	int fpos;
	unsigned long i;
	unsigned long ifsize = 0;

//printf("%d\n", sizeof(s_48));
//printf("%d\n", sizeof(s_40));

if (sizeof(s_40) != 40) { printf("This program needs to be 32-bit\n"); exit(1); }


	if ( argc != 3 && argc != 4 ) {
		fprintf(stderr, "Usage: cpstriped <infile> <outfile> [gps cut off]\n");
		exit(1);
	}
	if (argc == 4) {
		cutoff = atoi(argv[3]);
		printf("Cutoff set to 0x%x\n", cutoff);
	}
	ifd = open( argv[1], O_RDONLY );
	if (ifd < 0 ) {
		fprintf(stderr, "Cannot open input file `%s'\n", argv[1]);
		exit(1);
	}
	{
		struct stat buf;
		int res = fstat(ifd, &buf);
		if (res < 0) {
			fprintf(stderr, "Cannot do an fstat()\n");
			exit(1);
		}
		ifsize = buf.st_size;
	}
	ofd = open( argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0666 );
	if (ifd < 0 ) {
		fprintf(stderr, "Cannot open output file `%s'\n", argv[2]);
		exit(1);
	}
	
	s_40 s40;
	memset(&s40, 0, sizeof(s40));
	for (i = 0; i < ifsize; i+= STRUCT_SIZE) {
		int n;
		unsigned char buf[STRUCT_SIZE];
		off_t offs = lseek(ifd, i, SEEK_SET);
		if (offs != i) {
			fprintf(stderr, "Failed lseek(%d)\n", i);
			exit(1);
		}	
		n = read(ifd, buf, STRUCT_SIZE);
		if (n != STRUCT_SIZE) {
			fprintf(stderr,"read failed at %d\n", i);
		} else {
			s_48 *s48 = buf;
#define byteswap4(a,b) ((char *)&a)[0] = ((char *)&b)[3]; ((char *)&a)[1] = ((char *)&b)[2];((char *)&a)[2] = ((char *)&b)[1];((char *)&a)[3] = ((char *)&b)[0];
#define byteswap8(a,b) ((char *)&a)[0] = ((char *)&b)[7]; ((char *)&a)[1] = ((char *)&b)[6];((char *)&a)[2] = ((char *)&b)[5];((char *)&a)[3] = ((char *)&b)[4]; ((char *)&a)[4] = ((char *)&b)[3]; ((char *)&a)[5] = ((char *)&b)[2];((char *)&a)[6] = ((char *)&b)[1];((char *)&a)[7] = ((char *)&b)[0];
			byteswap4(s40.gps, s48->gps);
			byteswap4(s40.min.I, s48->min.I);
			byteswap4(s40.max.I, s48->max.I);
			byteswap4(s40.n, s48->n);
			byteswap8(s40.rms, s48->rms);
			byteswap8(s40.mean, s48->mean);
			if (makesSense(s40.gps)) {
			    //fprintf(stderr,"GPS time %u %u\n", s40.gps, cutoff);
			    n = write(ofd, &s40, 40);
			    if (n !=  40) {
				fprintf(stderr, "write failed; errno=%d\n", errno);
				exit(1);
			    }
			} else {
			    fprintf(stderr,"GPS time %u at 0x%x makes no sense\n", s40.gps, i);
			    //close(ofd);
			    //sync();
			    //exit(0);
			}
		}
		
	}
	close(ofd);
	sync();
	exit(0);
}
