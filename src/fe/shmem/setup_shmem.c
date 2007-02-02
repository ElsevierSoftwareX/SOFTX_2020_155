#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>

#define MMAP_SIZE 1024*1024*64-5000


/* How many systems configured */
int sys_count = 0;

/* Systems information (configuration) */
struct sys_info{
	char name[128]; /* System name */	
	int adcs;	/* ADC boards count */
	int dacs;	/* DAC board count */
} sys_info[128];

char *
get_fname(int s) {
	static char fname[128] = "/rtl_epics";
	switch (sys_count) {
	case 0:
		break;
	default:
		sprintf(fname, "/rtl_mem_%s", sys_info[s].name);
		break;
	}
	return fname;
}

/*  ./setup_shmem.rtl 'das,adc=1,dac=1' 'pde,adc=1,dac=1' */

int main(int argc, char **argv)
{
	int cnt, i;
	sys_count = 0;
	memset(sys_info, 0, sizeof(sys_info));

	if (argc > 1) {
		for (i = 0; i < argc-1; i++) {
			strcpy(sys_info[i].name, argv[i+1]);
			sys_info[i].adcs = 1;
			sys_info[i].dacs = 0;
		}
		sys_count = argc - 1;
	}
#if 0
	for (i = 0; i < sys_count; i++) {
		printf("%s: adcs=%d, dacs=%d\n", sys_info[i].name, sys_info[i].adcs, sys_info[i].dacs);
	}
#endif

	cnt = sys_count;
	if (cnt == 0) cnt = 1;

#if defined(RTL_BUILD)
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
#endif

	return 0;
}
