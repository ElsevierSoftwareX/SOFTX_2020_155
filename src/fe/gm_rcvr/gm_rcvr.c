#include <rtl_time.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <drv/myri.h>
#include <drv/gmnet.h>

#define MMAP_SIZE 1024*1024*64-5000

extern void *manage_network(void *);
extern int stop_net_manager;

unsigned char *_ipc_shm;

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
        _ipc_shm = (unsigned char *)rtl_mmap(NULL,MMAP_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,wfd,0);
        if (_ipc_shm == MAP_FAILED) {
           printf("mmap failed for IPC shared memory area\n");
           rtl_perror("mmap()");
           return -1;
	}

	/* Open Myrinet */
        if (myriNetInit(-1) != 1) return 1;

#if 0
  /* Register IPC memory; Only first 4K are registered  */
  gm_status_t status = gm_register_memory(netPort, _ipc_shm, GM_PAGE_LEN);
  if (status != GM_SUCCESS) {
      printk ("Couldn't register memory for DMA; error=%d", status);
      return 0;
  }
#endif


        /* create the thread */
	rtl_pthread_t thread;
        pthread_create(&thread, NULL, manage_network, 0);

	/* wait for us to be removed or killed */
	rtl_main_wait();
	stop_net_manager = 1;
        rtl_pthread_cancel(thread);
        rtl_pthread_join(thread, NULL);

	//gm_deregister_memory(netPort, _ipc_shm, GM_PAGE_LEN);

        myriNetClose();

	return 0;
}
