#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <msr.h>


#define MMAP_SIZE 1024*1024*64-5000

int sys_count = 0;

char *
get_fname(int s) {
	static char fname[128] = "/rtl_epics";
	switch (sys_count) {
	case 0:
		break;
	default:
		sprintf(fname, "/rtl_mem%d", s);
		break;
	}
	return fname;
}

int main(int argc, char **argv)
{
	int cnt, i;

	if (argc > 1) sys_count = atoi(argv[1]);

	cnt = sys_count;
	if (cnt == 0) cnt = 1;

	for (i = 0; i < cnt; i++) { 
	  int wfd;
	  /*
	   * Create the shared memory area.  By passing a non-zero value
	   * for the mode, this means we also create a node in the GPOS.
	   */
	  char *file_name = get_fname(i);
	  wfd = shm_open(file_name, RTL_O_CREAT, 0666);
	  if (wfd == -1) {
		printf("open failed for write on %s (%d)\n", file_name, errno);
		rtl_perror("shm_open()");
		return -1;
	  }

	  /* Set the shared area to the right size */
	  if (0 != ftruncate(wfd,MMAP_SIZE)) {
                printf("ftruncate failed (%d)\n",errno);
                rtl_perror("ftruncate()");
                return -1;
	  }

	  close(wfd);
	}

	/* wait for us to be removed or killed */
	rtl_main_wait();

	/* Note that this is a shared area created with shm_open() - we close
	 * it with close(), but use shm_unlink() to actually destroy the area
	 */
	for (i = 0; i < cnt; i++) { 
	  shm_unlink(get_fname(i));
	}

	return 0;
}
