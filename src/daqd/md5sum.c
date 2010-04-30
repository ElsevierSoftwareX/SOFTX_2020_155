#include <sys/stat.h>

#include "md5.h"

main(int argc, char *argv[])
{
	struct stat buf;
	unsigned char *mbuf;
	int fd;
 	unsigned int nread;
	unsigned char resblock[16];
	int k;

	if (argc < 2) {
		fputs("Usage: md5sum filename\n", stderr);
		exit(1);
	}
	if (stat (argv[1], &buf)) {
		perror("stat failed");
		exit(1);
	}
	mbuf = (unsigned char *)malloc(buf.st_size);
	if (! mbuf) {
		perror("malloc failed");
		exit(1);
	}
	fd = open(argv[1]);
	if (fd < 0) {
		perror(argv[1]);
		exit(1);
	}
	nread = read (fd, mbuf, buf.st_size);
	if (nread != buf.st_size) {
		perror("read failed");
		exit(1);
	}
        md5_buffer (mbuf, buf.st_size, resblock);
        for (k = 0; k < 16; k++)
             printf("%02x",resblock[k]);
	puts("\n");

}
