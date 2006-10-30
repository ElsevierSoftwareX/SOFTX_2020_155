#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <msr.h>


#define MMAP_SIZE 1024*1024*64-5000

pthread_t rthread, wthread; 
int rfd, wfd;
unsigned char *raddr, *waddr;

#if 0
void *writer(void *arg)
{
	struct timespec next;
	int i;
	int *walker;
	int clock1, clock2;
	unsigned int ioaddr_ptr;
	unsigned char *addr;

	/* map the shared memory read */
	waddr = (unsigned char *)rtl_mmap(NULL,MMAP_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,wfd,0);
	if (waddr == MAP_FAILED) {
		printf("mmap failed for writer\n");
		rtl_perror("mmap()");
		return (void *)-1;
	}

	/* get the current time and setup for the first tick */
	clock_gettime(CLOCK_REALTIME, &next);

	walker = (int*)waddr;
	walker[1] = 0;
	walker[2] =  NSECS_PER_SEC;
	walker[3] = 0;
	walker[4] = 0;
	rdtscl(clock1);
	clock2 = clock1;
	for(i = 0;; i++) {
		/* sleep... */
		timespec_add_ns(&next, NSECS_PER_SEC/16384);
		clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME,
			&next, NULL);

		rdtscl(clock1);
		walker[0] = clock1 - clock2;
		walker[1] = max(walker[0], walker[1]);
		walker[2] = min(walker[0], walker[2]);
		clock2 = clock1;

		addr[0x40] = 0;
#if 0
		/* write to the shared region */
		for (i = 0; i < MMAP_SIZE / sizeof(int); i++) {
			*walker = i;
			walker++;
		}
#endif
		if (i == 16384) {
		  walker[3] = walker[1] / 2393;
		  walker[1] = 0;
		  walker[4] = walker[2] / 2393;
		  walker[2] = NSECS_PER_SEC;
		  i = 0;
		}
	}

}
#endif

#if 0
void *reader(void *arg) 
{
	struct timespec next;
	int i;
	int *walker;

	/* map the shared memory read */
	raddr = (unsigned char *)rtl_mmap(NULL,MMAP_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,rfd,0);
	if (raddr == MAP_FAILED) {
		printf("failed mmap for reader\n");
		rtl_perror("rtl_mmap()");
		return (void *)-1;
	}

	/* setup for the first tick */
	clock_gettime(CLOCK_REALTIME, &next);

	while (1) {
		/* sleep... */
		timespec_add_ns(&next, 1000000000);
		clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME,
			&next, NULL);

		/* read from the shared memory region */
		walker = (int*)raddr;
		printf("walker = %x, raddr = %x\n",walker, raddr);
		for (i = 0; i < MMAP_SIZE / sizeof(int); i++) {
			if (*walker != i) 
				printf("Mismatch at point %d (%d)\n",i,*walker);
			walker++;
		}
	}
}
#endif

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

	/* Our initial create above handled the GPOS node creation and perms */
	rfd = shm_open("/rtl_epics", 0, 0);
	if (rfd == -1) {
		printf("open failed for read on /rtl_epics (%d)\n",errno);
		rtl_perror("sm_open()");
		return -1;
	}

	/* Set the shared area to the right size */
	if (0 != ftruncate(wfd,MMAP_SIZE)) {
                printf("ftruncate failed (%d)\n",errno);
                rtl_perror("ftruncate()");
                return -1;
	}

        pthread_attr_init(&attr);

#if 0
        pthread_attr_setcpu_np(&attr, rtl_getcpuid());
        /* mark this CPU as reserved - only RTLinux runs on it */
        pthread_attr_setreserve_np(&attr, rtl_getcpuid());

	pthread_create(&wthread, &attr, writer, 0);
	//pthread_create(&rthread, NULL, reader, 0);
#endif

	/* wait for us to be removed or killed */
	rtl_main_wait();

	/* kill the threads */
	pthread_cancel(wthread);
	pthread_join(wthread, NULL);
	pthread_cancel(rthread);
	pthread_join(rthread, NULL);

	/* unmap the shared memory areas */
	munmap(waddr, MMAP_SIZE);
	munmap(raddr, MMAP_SIZE);
	
	/* Note that this is a shared area created with shm_open() - we close
	 * it with close(), but use shm_unlink() to actually destroy the area
	 */
	close(wfd);
	close(rfd);
	shm_unlink("/rtl_epics");

	return 0;
}
