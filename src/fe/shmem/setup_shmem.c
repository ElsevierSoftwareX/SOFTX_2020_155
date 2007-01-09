#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <msr.h>


#define MMAP_SIZE 1024*1024*64-5000

int wfd;
unsigned char *raddr, *waddr;

int main(int argc, char **argv)
{
        pthread_attr_t attr;

	/*
	 * Create the shared memory area.  By passing a non-zero value
	 * for the mode, this means we also create a node in the GPOS.
	 */
	wfd = shm_open("/rtl_epics", RTL_O_CREAT, 0666);
	if (wfd == -1) {
		printf("open failed for write on /rtl_epics (%d)\n",errno);
		rtl_perror("shm_open()");
		return -1;
	}

	/* Set the shared area to the right size */
	if (0 != ftruncate(wfd,MMAP_SIZE)) {
                printf("ftruncate failed (%d)\n",errno);
                rtl_perror("ftruncate()");
                return -1;
	}

        pthread_attr_init(&attr);

	/* wait for us to be removed or killed */
	rtl_main_wait();

	/* Note that this is a shared area created with shm_open() - we close
	 * it with close(), but use shm_unlink() to actually destroy the area
	 */
	close(wfd);
	shm_unlink("/rtl_epics");

	return 0;
}
