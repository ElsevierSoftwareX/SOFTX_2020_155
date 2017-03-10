#include "linux/types.h"
#include <linux/module.h>       /* Needed by all modules */
#include <linux/kernel.h>       /* Needed for KERN_INFO */
#include <linux/kthread.h>       /* Needed for KERN_INFO */
#include <genif.h>
#include "../../include/commData3.h"
#include <asm/cacheflush.h>
#include <asm/uaccess.h>
#include <asm/delay.h>
#include <linux/ctype.h>
#include <linux/spinlock_types.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/string.h>
#include </usr/src/linux/arch/x86/include/asm/processor.h>
#include </usr/src/linux/arch/x86/include/asm/cacheflush.h>
#include <linux/timer.h>
#include <linux/spinlock_types.h>
#include <linux/ctype.h>

#define MAX_UDELAY    19999
#define ENTRY_NAME "counter"
#define PERM 0644
#define PARENT NULL
#define NUM_DOLPHIN_CARDS	2
#define NUM_DOLPHIN_NETS	4
static struct file_operations fops;

extern void *kmalloc_area[16];
extern int mbuf_allocate_area(char *name, int size, struct file *file);
char * _ipc_shm;
extern void set_fe_code_idle(void *(*ptr)(void *), unsigned int cpu);
extern int cpu_down(unsigned int);

CDS_DOLPHIN_INFO mdi;
CDS_IPC_COMMS *pIpcDataRead[NUM_DOLPHIN_NETS];
CDS_IPC_COMMS *pIpcDataWrite[NUM_DOLPHIN_NETS];

// Dolphin driver interface
sci_l_segment_handle_t segment[4];
sci_map_handle_t client_map_handle[4];
sci_r_segment_handle_t  remote_segment_handle[4];
sci_device_info_t	sci_dev_info[4];

static struct task_struct *sthread[5];
struct params {char name[100]; int delay; int idx; int netFrom; int netTo; };
struct params threads[5];

static unsigned long mycounter[10];
static int read_p;
static char *message;
// IPC_BLOCKS = 64 and MAX_IPC = 512
static unsigned long syncArray[2][IPC_BLOCKS][MAX_IPC];
static unsigned long lastSyncWord[NUM_DOLPHIN_NETS][100];
static unsigned int myactive[NUM_DOLPHIN_NETS][10];
static int ipcActive[NUM_DOLPHIN_NETS][100];
static int stop_working_threads;


// End of globals ****************************************
inline unsigned long current_time(void) {
    struct timespec t;
        extern struct timespec current_kernel_time(void);
	        t = current_kernel_time();
	        // Added leap second for July 1, 2015
		t.tv_sec += - 315964819 + 33 + 3 + 1;
                return t.tv_sec;
}

// ************************************************************************
int monitorActiveConnections(void *data) 
// ************************************************************************
{
  int indx = ((struct params*)data)->idx;
  int delay = ((struct params*)data)->delay;

  unsigned long syncWord;
  int ii,jj,kk,mm;

  if(pIpcDataRead[0] != NULL && pIpcDataRead[1] != NULL && pIpcDataRead[2] != NULL && pIpcDataRead[3] != NULL)
  {
	while(!kthread_should_stop()) {
		msleep(delay);
		for (jj=0;jj<NUM_DOLPHIN_NETS;jj++) {
			mycounter[jj] = 0;
			for(ii=0;ii<indx;ii++) {
				kk = ii/32;
				mm = ii % 32;
				syncWord = pIpcDataRead[jj]->dBlock[0][ii].timestamp;
				if(syncWord != lastSyncWord[jj][ii]) {
					ipcActive[jj][ii] = 1;
					lastSyncWord[jj][ii] = syncWord;
					// mycounter[jj] |= (1<<ii);
					mycounter[jj] += 1;
					myactive[jj][kk] |= (1<<mm);
				} else {
					ipcActive[jj][ii] = 0;
				}
			}
		}
	}
	printk("%s thread has terminated %d\n",((struct params*)data)->name,indx);
	return 0;
  } else {
  	printk("Do not have pointers to Dolphin read - %s exiting \n",((struct params*)data)->name);
	return -1;
  }
}

inline void copyIpcData (int indx, int netFrom, int netTo)
{
  unsigned long syncWord;
  int ii,jj;
  int cblock,dblock,ttcache;
  int eor = indx + 32;
  double tmp;

	cblock = 0;
	dblock = 0;
	ttcache = 0;
	for(ii=indx;ii<eor;ii++) {
		if(ipcActive[netFrom][ii]) {
			for(jj=0;jj<IPC_BLOCKS;jj++) {
				syncWord = pIpcDataRead[netFrom]->dBlock[jj][ii].timestamp;
				if(syncWord != syncArray[0][jj][ii]) {
					tmp = pIpcDataRead[netFrom]->dBlock[jj][ii].data;
					pIpcDataWrite[netTo]->dBlock[jj][ii].data = tmp;
					pIpcDataWrite[netTo]->dBlock[jj][ii].timestamp = syncWord;
					syncArray[0][jj][ii] = syncWord;
					cblock = jj;
					dblock = ii;
					ttcache += 1;
				}
			}
		}
	}
		
	// If anything was copied, we need to flush the buffer
	if (ttcache) clflush_cache_range (&(pIpcDataWrite[netTo]->dBlock[cblock][dblock].data), 16);
}
// ************************************************************************
void *copyRfmDataEX2CS0(void *arg) 
// ************************************************************************
{
  int indx = 0;
  int delay = 2;

  if(pIpcDataRead[0] != NULL && pIpcDataWrite[1] != NULL)
  {
	while(!stop_working_threads) {
		udelay(delay);
		copyIpcData (indx, 0, 1);
		copyIpcData (indx, 1, 0);
	}
	// printk("%s thread has terminated %d\n",((struct params*)data)->name,indx);
	printk("%s thread has terminated %d\n","copyRfmDataEX2CS0",1);
	return (void *)0;
  } else {
  	// printk("Do not have pointers to Dolphin read - %s exiting \n",((struct params*)data)->name);
  	printk("Do not have pointers to Dolphin read - %s exiting \n","copyRfmDataEX2CS0");
	return (void *)-1;
  }
}
// ************************************************************************
void *copyRfmDataEX2CS1(void *arg) 
// ************************************************************************
{
  int indx = 32;
  int delay = 2;

  if(pIpcDataRead[0] != NULL && pIpcDataWrite[1] != NULL)
  {
	while(!stop_working_threads) {
		udelay(delay);
		copyIpcData (indx, 0, 1);
		copyIpcData (indx, 1, 0);
	}
	// printk("%s thread has terminated %d\n",((struct params*)data)->name,indx);
	printk("%s thread has terminated %d\n","copyRfmDataEX2CS1",1);
	return (void *)0;
  } else {
  	// printk("Do not have pointers to Dolphin read - %s exiting \n",((struct params*)data)->name);
  	printk("Do not have pointers to Dolphin read - %s exiting \n","copyRfmDataEX2CS1");
	return (void *)-1;
  }
}

// ************************************************************************
int counter_run(void *data) {
// ************************************************************************
  int delay = ((struct params*)data)->delay;
  int indx = ((struct params*)data)->idx;
  while(!kthread_should_stop()) {
  	ssleep(delay);
	mycounter[indx] += 1;
	mycounter[indx] %= 1000000000;
	// printk("Counter %s = %d\n",((struct params*)data)->name,mycounter[indx]);
  }
  printk("%s thread has terminated %d\n",((struct params*)data)->name,indx);
  return mycounter[indx];
}

// ************************************************************************
signed32 session_callback(session_cb_arg_t IN arg,
			  session_cb_reason_t IN reason,
			  session_cb_status_t IN status,
			  unsigned32 IN target_node,
			  unsigned32 IN local_adapter_number) {
// ************************************************************************
  printkl("Session callback reason=%d status=%d target_node=%d\n", reason, status, target_node);
  // if (reason == SR_OK) iop_rfm_valid = 1;
  // This is being called when the one of the other nodes is prepared for shutdown
  // :TODO: may need to check target_node == <our local node>
  //if (reason == SR_DISABLED || reason == SR_LOST) iop_rfm_valid = 0;
  return 0;
}

// ************************************************************************
signed32 connect_callback(void IN *arg,
			  sci_r_segment_handle_t IN remote_segment_handle,
			  unsigned32 IN reason, unsigned32 IN status) {
// ************************************************************************
  printkl("Connect callback reason=%d status=%d\n", reason, status);
#if 0
  if (reason == 1) iop_rfm_valid = 1;
  if (reason == 3) iop_rfm_valid = 0;
  if (reason == 5) iop_rfm_valid = 1;
#endif
  return 0;
}

// ************************************************************************
signed32 create_segment_callback(void IN *arg,
				 sci_l_segment_handle_t IN local_segment_handle,
				 unsigned32 IN reason,
				 unsigned32 IN source_node,
				 unsigned32 IN local_adapter_number)  {
// ************************************************************************
  printkl("Create segment callback reason=%d source_node=%d\n", reason, source_node);
  return 0;
}

// ************************************************************************
int
init_dolphin(int modules,CDS_DOLPHIN_INFO *pInfo) {
// ************************************************************************
  scierror_t err;
  char *addr;
  char *read_addr;
  int ii;
  pInfo->dolphinCount = 0;
  for(ii=0;ii<modules;ii++) {
  err = sci_create_segment(NO_BINDING,
		       ii,
		       1,
		       DIS_BROADCAST,
		       IPC_TOTAL_ALLOC_SIZE,
		       create_segment_callback,
		       0,
		       &segment[ii]);
  printk("DIS segment alloc status %d\n", err);
  if (err) return -1;

  err = sci_set_local_segment_available(segment[ii], ii);
  printk("DIS segment making available status %d\n", err);
  if (err) {
    sci_remove_segment(&segment[ii], ii);
    return -1;
  }
  
  err = sci_export_segment(segment[ii], ii, DIS_BROADCAST);
  printk("DIS segment export status %d\n", err);
  if (err) {
    sci_remove_segment(&segment[ii], ii);
    return -1;
  }
  
  read_addr = sci_local_kernel_virtual_address(segment[ii]);
  if (read_addr == 0) {
    printk("DIS sci_local_kernel_virtual_address returned 0\n");
    sci_remove_segment(&segment[ii], ii);
    return -1;
  } else {
    printk("Dolphin memory read at 0x%p\n", read_addr);
    pInfo->dolphinRead[ii] = (volatile unsigned long *)read_addr;
  }
  udelay(MAX_UDELAY);
  udelay(MAX_UDELAY);
  
  err = sci_connect_segment(NO_BINDING,
			    4, // DIS_BROADCAST_NODEID_GROUP_ALL
			    ii,
			    0,
			    1, 
			    DIS_BROADCAST,
			    connect_callback, 
			    0,
			    &remote_segment_handle[ii]);
  printk("DIS connect segment status %d\n", err);
  if (err) {
    sci_remove_segment(&segment[ii], ii);
    return -1;
  }

  // usleep(20000);
  udelay(MAX_UDELAY);
  udelay(MAX_UDELAY);
  err = sci_map_segment(remote_segment_handle[ii],
			DIS_BROADCAST,
			ii,
			IPC_TOTAL_ALLOC_SIZE,
			&client_map_handle[ii]);
  printk("DIS segment mapping status %d\n", err);
  if (err) {
    sci_disconnect_segment(&remote_segment_handle[ii], ii);
    sci_remove_segment(&segment[ii], ii);
    return -1;
  }
  
  addr = sci_kernel_virtual_address_of_mapping(client_map_handle[ii]);
  if (addr == 0) {
    printk ("Got zero pointer from sci_kernel_virtual_address_of_mapping\n");
    sci_disconnect_segment(&remote_segment_handle[ii], ii);
    sci_remove_segment(&segment[ii], ii);
    return -1;
  } else {
    printk ("Dolphin memory write at 0x%p\n", addr);
    pInfo->dolphinWrite[ii] = (volatile unsigned long *)addr;
  }

  sci_register_session_cb(ii,0,session_callback,0);
  pInfo->dolphinCount += 1;

    if(ii == 0) {
	pIpcDataRead[0] = (CDS_IPC_COMMS *)(read_addr + IPC_PCIE_BASE_OFFSET + RFM0_OFFSET);
	pIpcDataWrite[0] = (CDS_IPC_COMMS *)(addr + IPC_PCIE_BASE_OFFSET + RFM0_OFFSET);
    }
    if(ii == 0 && modules == 1) {
	pIpcDataRead[1] = (CDS_IPC_COMMS *)(read_addr + IPC_PCIE_BASE_OFFSET + RFM1_OFFSET);
	pIpcDataRead[2] = (CDS_IPC_COMMS *)(read_addr + IPC_PCIE_BASE_OFFSET);
	pIpcDataRead[3] = (CDS_IPC_COMMS *)(read_addr + IPC_PCIE_BASE_OFFSET + RFM1_OFFSET);
	pIpcDataWrite[1] = (CDS_IPC_COMMS *)(addr + IPC_PCIE_BASE_OFFSET + RFM1_OFFSET);
	pIpcDataWrite[2] = (CDS_IPC_COMMS *)(addr + IPC_PCIE_BASE_OFFSET + RFM1_OFFSET);
	pIpcDataWrite[3] = (CDS_IPC_COMMS *)(addr + IPC_PCIE_BASE_OFFSET + RFM1_OFFSET);
    }
    if(ii == 1 && modules == 2) {
	pIpcDataRead[1] = (CDS_IPC_COMMS *)(read_addr + IPC_PCIE_BASE_OFFSET + RFM0_OFFSET);
	pIpcDataRead[2] = (CDS_IPC_COMMS *)(read_addr + IPC_PCIE_BASE_OFFSET + RFM1_OFFSET);
	pIpcDataRead[3] = (CDS_IPC_COMMS *)(read_addr + IPC_PCIE_BASE_OFFSET + RFM1_OFFSET);
	pIpcDataWrite[1] = (CDS_IPC_COMMS *)(addr + IPC_PCIE_BASE_OFFSET + RFM0_OFFSET);
	pIpcDataWrite[2] = (CDS_IPC_COMMS *)(addr + IPC_PCIE_BASE_OFFSET + RFM1_OFFSET);
	pIpcDataWrite[3] = (CDS_IPC_COMMS *)(addr + IPC_PCIE_BASE_OFFSET + RFM1_OFFSET);
    }
    if(ii == 1 && modules == 3) {
	pIpcDataRead[1] = (CDS_IPC_COMMS *)(read_addr + IPC_PCIE_BASE_OFFSET + RFM0_OFFSET);
	pIpcDataRead[2] = (CDS_IPC_COMMS *)(read_addr + IPC_PCIE_BASE_OFFSET + RFM1_OFFSET);
	pIpcDataWrite[1] = (CDS_IPC_COMMS *)(addr + IPC_PCIE_BASE_OFFSET + RFM0_OFFSET);
	pIpcDataWrite[2] = (CDS_IPC_COMMS *)(addr + IPC_PCIE_BASE_OFFSET + RFM1_OFFSET);
    }
    if(ii == 2 && modules == 3) {
	pIpcDataRead[3] = (CDS_IPC_COMMS *)(read_addr + IPC_PCIE_BASE_OFFSET + RFM1_OFFSET);
	pIpcDataWrite[3] = (CDS_IPC_COMMS *)(addr + IPC_PCIE_BASE_OFFSET + RFM1_OFFSET);
    }
}


  return 0;
}

// ************************************************************************
void finish_dolphin(int dolphinCount) {
// ************************************************************************
int ii;
  for(ii=0;ii<dolphinCount;ii++) {
	  sci_unmap_segment(&client_map_handle[ii], ii);
	  sci_disconnect_segment(&remote_segment_handle[ii], ii);
	  sci_unexport_segment(segment[ii], ii, 0);
	  sci_remove_segment(&segment[ii], ii);
	  sci_cancel_session_cb(ii, 0);
  }
}

// ************************************************************************
int counter_proc_open(struct inode *sp_inode,struct file *sp_file) {
// ************************************************************************
	printk("proc called open \n");
	read_p = 1;
	message = kmalloc(sizeof(char)*128,__GFP_WAIT|__GFP_IO|__GFP_FS);
	if(message == NULL) {
		printk("ERROR counter proc open\n");
		return -ENOMEM;
	}
	sprintf(message,"%ld %ld %ld %ld %d %d %d %d %d %d\n",
						mycounter[0],mycounter[1],mycounter[2],mycounter[3],
						myactive[0][0],myactive[0][1],myactive[0][2],
						myactive[1][0],myactive[1][1],myactive[1][2]);
	return 0;
}

// ************************************************************************
ssize_t counter_proc_read(struct file *sp_file, char __user *buf,size_t size,loff_t *offset) {
// ************************************************************************
  int ret;
	int len = strlen(message);
	read_p = !read_p;
	if(read_p) return 0;
	// if(offset > 0) return 0;
	printk("proc called read \n");
	ret = copy_to_user(buf,message,len);
	return len;
}

// ************************************************************************
int counter_proc_release(struct inode *sp_inode,struct file *sp_file) {
// ************************************************************************
	printk("proc called release \n");
	kfree(message);
	return 0;
}

/*
 *  *  test.c The simplest kernel module.
 *   */
static int test_data __initdata = 3;

// ************************************************************************
static int __init test_3_init(void)
// ************************************************************************
{
	printk(KERN_INFO "Hello, world %d\n", test_data);
	init_dolphin(NUM_DOLPHIN_CARDS,&mdi);
	stop_working_threads = 0;

#if 0
// Setup IPC shared memory with EPICS if needed.
	ret =  mbuf_allocate_area("ipc", 4*1024*1024, 0);
	if (ret < 0) {
	       	printk("mbuf_allocate_area(ipc) failed; ret = %d\n", ret);
	  	return -1;
	}
	_ipc_shm = (unsigned char *)(kmalloc_area[ret]);

        printk(KERN_INFO "IPC    at 0x%x\n",(unsigned int)_ipc_shm);
#endif


	// Set thread names
	strcpy(threads[0].name,"Thread 1");
	strcpy(threads[1].name,"Thread 2");
	strcpy(threads[2].name,"cdsrfmnetmon");
	strcpy(threads[3].name,"cdsrfmcp12");

	// Set thread delays
	threads[0].delay = 1;
	threads[1].delay = 5;
	threads[2].delay = 1000;
	threads[3].delay = 1000;

	// Set thread indexes
	threads[0].idx = 0;
	threads[1].idx = 1;
	threads[2].idx = 100;
	threads[3].idx = 32;

	// Set thread netFrom
	threads[0].netFrom = 0;
	threads[1].netFrom = 1;
	threads[2].netFrom = 0;
	threads[3].netFrom = 1;

	// Set thread netto
	threads[0].netTo = 0;
	threads[1].netTo = 1;
	threads[2].netTo = 0;
	threads[3].netTo = 2;
	//
	// Create threads
#if 0
	sthread[0] = kthread_create(counter_run,(void *)&threads[0],"counter1");
	if(IS_ERR(sthread[0])) {
		printk("ERROR! kthread_run\n");
		return PTR_ERR(sthread[0]);
	}
	sthread[1] = kthread_create(counter_run,(void *)&threads[1],"counter2");
	if(IS_ERR(sthread[1])) {
		printk("ERROR! kthread_run\n");
		return PTR_ERR(sthread[1]);
	}
#endif
	sthread[2] = kthread_create(monitorActiveConnections,(void *)&threads[2],"cdsrfmnetmon");
	if(IS_ERR(sthread[2])) {
		printk("ERROR! kthread_run\n");
		return PTR_ERR(sthread[2]);
	}
#if 0
	sthread[3] = kthread_create(copyRfmData,(void *)&threads[3],"cdsrfmcp12");
	if(IS_ERR(sthread[3])) {
		printk("ERROR! kthread_run\n");
		return PTR_ERR(sthread[2]);
	}

	// Bind threads
	kthread_bind(sthread[0],4);
	kthread_bind(sthread[1],5);
#endif
	kthread_bind(sthread[2],6);
	// kthread_bind(sthread[3],3);

	// Start threads
	// wake_up_process(sthread[0]);
	// wake_up_process(sthread[1]);
	wake_up_process(sthread[2]);
	// wake_up_process(sthread[3]);

	printk("Shutting down CPU 3 at %ld\n",current_time());
	set_fe_code_idle(copyRfmDataEX2CS0,3);
	msleep(100);
	cpu_down(3);

	printk("Shutting down CPU 4 at %ld\n",current_time());
	set_fe_code_idle(copyRfmDataEX2CS1,4);
	msleep(100);
	cpu_down(4);

	fops.open = counter_proc_open;
	fops.read = counter_proc_read;
	fops.release = counter_proc_release;

	if(!proc_create(ENTRY_NAME,PERM,NULL,&fops)) {
		printk("ERROR! proc_create\n");
		remove_proc_entry(ENTRY_NAME,NULL);
		return -ENOMEM;
	}
	return 0;
}

// ************************************************************************
static void __exit test_3_exit(void)
// ************************************************************************
{
  int ret;
  extern int __cpuinit cpu_up(unsigned int cpu);
	printk(KERN_INFO "Goodbye, test 3\n");
#if 0
	ret = kthread_stop(sthread[0]);
	if (ret != -EINTR)
		printk("Counter thread has stopped %ld\n",mycounter[0]);
	ret = kthread_stop(sthread[1]);
	if (ret != -EINTR)
		printk("Counter thread has stopped %ld\n",mycounter[1]);
#endif
	ret = kthread_stop(sthread[2]);
	if (ret != -EINTR)
		printk("RFM thread has stopped %ld\n",mycounter[1]);
	ssleep(2);


	set_fe_code_idle(0, 3);
	msleep(1000);
	set_fe_code_idle(0, 4);
	msleep(1000);

	stop_working_threads = 1;
	msleep(1000);

	set_fe_code_idle(0, 3);
	printkl("Will bring back CPU %d\n", 3);
	msleep(1000);
	cpu_up(3);
	printkl("Brought CPU 3 back up\n");

	set_fe_code_idle(0, 4);
	printkl("Will bring back CPU %d\n", 4);
	msleep(1000);
	cpu_up(4);
	printkl("Brought CPU 4 back up\n");
#if 0
	set_fe_code_idle(0, 4);
	printkl("Will bring back CPU %d\n", 4);
	msleep(1000);
#endif

#if 0
	ret = kthread_stop(sthread[3]);
	if (ret != -EINTR)
		printk("RFM CP thread has stopped %ld\n",mycounter[1]);
#endif

	printk("Removing /proc/%s.\n",ENTRY_NAME);
	remove_proc_entry(ENTRY_NAME,NULL);
	finish_dolphin(mdi.dolphinCount);
}

module_init(test_3_init);
module_exit(test_3_exit);

MODULE_AUTHOR("R.Bork");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Test");
MODULE_SUPPORTED_DEVICE("Long Range PCIe Switch");
