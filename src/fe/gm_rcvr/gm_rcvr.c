#include <rtl_time.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <drv/myri.h>

#define MMAP_SIZE 1024*1024*64-5000

extern void *manage_network(void *);
extern int stop_net_manager;

int main(int argc, char **argv)
{
	int wfd;
	char *file_name = "/rtl_mem_ipc";

	wfd = shm_open(file_name, RTL_O_RDWR, 0666);
	if (wfd == -1) {
	  printf("open r/w failed on %s (%d)\n", file_name, errno);
	  rtl_perror("shm_open()");
	  return -1;
	}

        myriNetInit(-1);
        /* create the thread */
	rtl_pthread_t thread;
        pthread_create(&thread, NULL, manage_network, 0);

	/* wait for us to be removed or killed */
	rtl_main_wait();
	stop_net_manager = 1;
        rtl_pthread_cancel(thread);
        rtl_pthread_join(thread, NULL);
        myriNetClose();

	return 0;
}
