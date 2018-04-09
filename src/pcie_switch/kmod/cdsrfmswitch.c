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
#include <linux/ctype.h>

#define MAX_UDELAY    19999
#define ENTRY_NAME "cdsrfm"
#define PERM 0644
#define PARENT NULL
#define RFMX_NUM_DOLPHIN_CARDS	3
#define RFMX_NUM_DOLPHIN_NETS	4
#define RFMX_MAX_CHANS_PER_RFM	100
#define RFMX_CHANS_PER_THREAD	32
#define RFMX_MAX_XFER_CNT	100000000

// struct for using /proc files
static struct file_operations fops;

// if eventually use mbuf shared memory for EPICS interface
#if 0
extern void *kmalloc_area[16];
extern int mbuf_allocate_area(char *name, int size, struct file *file);
char * _ipc_shm;
#endif

// CPU locking routines
extern void set_fe_code_idle(void *(*ptr)(void *), unsigned int cpu);
extern int cpu_down(unsigned int);

CDS_DOLPHIN_INFO mdi;
CDS_IPC_COMMS *pIpcDataRead[RFMX_NUM_DOLPHIN_NETS];
CDS_IPC_COMMS *pIpcDataWrite[RFMX_NUM_DOLPHIN_NETS];

// Dolphin driver interface
sci_l_segment_handle_t segment[4];
sci_map_handle_t client_map_handle[4];
sci_r_segment_handle_t  remote_segment_handle[4];
sci_device_info_t	sci_dev_info[4];

// Structs for kernel threads
static struct task_struct *sthread[5];
struct params {char name[100]; int delay; int idx; int netFrom; int netTo; };
struct params threads[5];

static unsigned long mycounter[10];
static int read_p;
static char *message;

// IPC block arrays for monitoring and switching
// IPC_BLOCKS = 64 and MAX_IPC = 512
static unsigned long syncArray[4][IPC_BLOCKS][MAX_IPC];
static unsigned long lastSyncWord[RFMX_NUM_DOLPHIN_NETS][RFMX_MAX_CHANS_PER_RFM];
static unsigned int myactive[RFMX_NUM_DOLPHIN_NETS][10];
static unsigned int mytraffic[RFMX_NUM_DOLPHIN_NETS];
static int ipcActive[RFMX_NUM_DOLPHIN_NETS][RFMX_MAX_CHANS_PER_RFM];
static int stop_working_threads;
static int mysysstatus;

// End of globals *********************************************************
//
//
//
// ************************************************************************
inline unsigned long current_time(void) {
// ************************************************************************
    struct timespec t;
        extern struct timespec current_kernel_time(void);
	        t = current_kernel_time();
	        // Added leap second for July 1, 2015
		t.tv_sec += - 315964819 + 33 + 3 + 1;
                return t.tv_sec;
}

// ************************************************************************
// Code thread monitors all switch ports for activity.
// Marks channels by network as active or not for use by data xfer threads.
int monitorActiveConnections(void *data) 
// ************************************************************************
{
  int indx = ((struct params*)data)->idx;
  int delay = ((struct params*)data)->delay;

  unsigned long syncWord;
  int ii,jj,kk,mm;
  static int traffic[RFMX_NUM_DOLPHIN_NETS];
  int swstatus;

  if(pIpcDataRead[0] != NULL && pIpcDataRead[1] != NULL && pIpcDataRead[2] != NULL && pIpcDataRead[3] != NULL)
  {
	while(!kthread_should_stop()) {
		// Check port activity once per second
		msleep(delay);
		for (jj=0;jj<RFMX_NUM_DOLPHIN_NETS;jj++) {
			mycounter[jj] = 0;
			for(ii=0;ii<indx;ii++) {
				// Will send port activity info to EPICS via 32 bit ints
				kk = ii/32;
				mm = ii % 32;
				// Determine active channels by change in data timestamp
				// Only checking first data block of each channel, as should have
				// changed in the last second if active.
				syncWord = pIpcDataRead[jj]->dBlock[0][ii].timestamp;
				if(syncWord != lastSyncWord[jj][ii]) {
					// Indicate channel active for data xfer threads
					ipcActive[jj][ii] = 1;
					lastSyncWord[jj][ii] = syncWord;
					// Increment the active channel counter for this network
					mycounter[jj] += 1;
					// Set the active port info for EPICS
					myactive[jj][kk] |= (1<<mm);
				} else {
					// Indicate channel NOT active for data xfer threads
					ipcActive[jj][ii] = 0;
					// Unset active port info for EPICS
					myactive[jj][kk] &= ~(1<<mm);
				}
			}
		}
		// Get current GPS time
		// Sent to EPICS as indicator switch code running
		mycounter[9] = current_time();
		// Check traffic for all data xfer threads
		// and set thread status bits.
		swstatus = 0;
		for (jj=0;jj<RFMX_NUM_DOLPHIN_NETS;jj++) {
			if(traffic[jj] != mytraffic[jj]) {
				traffic[jj] = mytraffic[jj];
				swstatus |= (1 << jj);
			}
		}
		// Copy data xfer thread status to be sent to EPICS
		mysysstatus = swstatus;
	}
	printk("%s thread has terminated %d\n",((struct params*)data)->name,indx);
	return 0;
  } else {
  	printk("Do not have pointers to Dolphin read - %s exiting \n",((struct params*)data)->name);
	return -1;
  }
}

// ************************************************************************
// Common code to xfer data between 2 RFM links
inline int copyIpcData (int indx, int netFrom, int netTo)
// ************************************************************************
{
  unsigned long syncWord;
  int ii,jj;
  int cblock,dblock,ttcache;
  int eor = indx + RFMX_CHANS_PER_THREAD;
  double tmp;
  int xfers = 0;

	// Reset cache flushing indices and flag
	cblock = 0;
	dblock = 0;
	ttcache = 0;
	// Scan thru switching ports for new data
	for(ii=indx;ii<eor;ii++) {
		// Copy data only if port is active, as marked by switch monitor task
		if(ipcActive[netFrom][ii]) {
			// Scan thru 64 data blocks associated with a channel for new data
			// Only check every 4th block, as max RFM channel rate is 16K
			for(jj=0;jj<IPC_BLOCKS;jj+=4) {
				// Determine new data by comparing present timestamp with last xfer timestamp
				syncWord = pIpcDataRead[netFrom]->dBlock[jj][ii].timestamp;
				if(syncWord != syncArray[netFrom][jj][ii]) {
					xfers ++;
					// Copy data and time stamp to next Dolphin switch
					tmp = pIpcDataRead[netFrom]->dBlock[jj][ii].data;
					pIpcDataWrite[netTo]->dBlock[jj][ii].data = tmp;
					pIpcDataWrite[netTo]->dBlock[jj][ii].timestamp = syncWord;
					syncArray[netFrom][jj][ii] = syncWord;
					// Setup locations and flag for cache flushing at end of xfers
					cblock = jj;
					dblock = ii;
					ttcache += 1;
				}
			}
		}
	}
		
	// If anything was copied, we need to flush the buffer
	if (ttcache) clflush_cache_range (&(pIpcDataWrite[netTo]->dBlock[cblock][dblock].data), 16);
	// return total xfers, which is used as code running diagnostic
	return xfers;
}

// DATA XFER Threads  *****************************************************
// Following 4 code modules are the switch data xfer threads. Each thread:
// 	- Is locked to a CPU core (3-6)
// 	- Transfers 32 channels of data between EX and CS or CS and EY.
// ************************************************************************
void *copyRfmDataEX2CS0(void *arg) 
// Thread to copy data chans 0-31 between EX and CS (RFM0)
// ************************************************************************
{
  int indx = 0;
  static int totalxfers;

  if(pIpcDataRead[0] != NULL && pIpcDataWrite[1] != NULL)
  {
	while(!stop_working_threads) {
		totalxfers += copyIpcData (indx, 0, 1);
		totalxfers += copyIpcData (indx, 1, 0);
		totalxfers %= RFMX_MAX_XFER_CNT;
		mytraffic[0] = totalxfers;
	}
	printk("%s thread has terminated %d\n","copyRfmDataEX2CS0",1);
	return (void *)0;
  } else {
  	printk("Do not have pointers to Dolphin read - %s exiting \n","copyRfmDataEX2CS0");
	return (void *)-1;
  }
}

// ************************************************************************
void *copyRfmDataEX2CS1(void *arg) 
// Thread to copy data chans 31-63 between EX and CS (RFM0)
// ************************************************************************
{
  int indx = 32;
  static int totalxfers;

  if(pIpcDataRead[0] != NULL && pIpcDataWrite[1] != NULL)
  {
	while(!stop_working_threads) {
		totalxfers += copyIpcData (indx, 0, 1);
		totalxfers += copyIpcData (indx, 1, 0);
		totalxfers %= RFMX_MAX_XFER_CNT;
		mytraffic[1] = totalxfers;
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
void *copyRfmDataEY2CS0(void *arg) 
// Thread to copy data chans 0-31 between EY and CS (RFM1)
// ************************************************************************
{
  int indx = 0;
  static int totalxfers;

  if(pIpcDataRead[2] != NULL && pIpcDataWrite[3] != NULL)
  {
	while(!stop_working_threads) {
		totalxfers += copyIpcData (indx, 2, 3);
		totalxfers += copyIpcData (indx, 3, 2);
		totalxfers %= RFMX_MAX_XFER_CNT;
		mytraffic[2] = totalxfers;
	}
	// printk("%s thread has terminated %d\n",((struct params*)data)->name,indx);
	printk("%s thread has terminated %d\n","copyRfmDataEY2CS0",1);
	return (void *)0;
  } else {
  	// printk("Do not have pointers to Dolphin read - %s exiting \n",((struct params*)data)->name);
  	printk("Do not have pointers to Dolphin read - %s exiting \n","copyRfmDataEY2CS0");
	return (void *)-1;
  }
}

// ************************************************************************
void *copyRfmDataEY2CS1(void *arg) 
// Thread to copy data chans 31-63 between EY and CS (RFM1)
// ************************************************************************
{
  int indx = 32;
  static int totalxfers;

  if(pIpcDataRead[2] != NULL && pIpcDataWrite[3] != NULL)
  {
	while(!stop_working_threads) {
		totalxfers += copyIpcData (indx, 2, 3);
		totalxfers += copyIpcData (indx, 3, 2);
		totalxfers %= RFMX_MAX_XFER_CNT;
		mytraffic[3] = totalxfers;
	}
	// printk("%s thread has terminated %d\n",((struct params*)data)->name,indx);
	printk("%s thread has terminated %d\n","copyRfmDataEY2CS1",1);
	return (void *)0;
  } else {
  	// printk("Do not have pointers to Dolphin read - %s exiting \n",((struct params*)data)->name);
  	printk("Do not have pointers to Dolphin read - %s exiting \n","copyRfmDataEY2CS1");
	return (void *)-1;
  }
}

// ************************************************************************
// Following 3 routines are required Dolphin connection callbacks.
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
// Dolphin NIC initialization software.
// ************************************************************************
int
init_dolphin(int modules,CDS_DOLPHIN_INFO *pInfo) {
// Dolphin NIC intialization software.
// Will set up connections and return memory pointers for all threads on
// code start.
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

    // NIC 0 should be connection to corner station Dolphin switch.
    // Need Dolphin NIC read/write pointers to both RFM0 (EX) net and RFM1 (EY) net.
    if(ii == 0) {
	pIpcDataRead[1] = (CDS_IPC_COMMS *)(read_addr + IPC_PCIE_BASE_OFFSET + RFM0_OFFSET);
	pIpcDataRead[2] = (CDS_IPC_COMMS *)(read_addr + IPC_PCIE_BASE_OFFSET + RFM1_OFFSET);
	pIpcDataWrite[1] = (CDS_IPC_COMMS *)(addr + IPC_PCIE_BASE_OFFSET + RFM0_OFFSET);
	pIpcDataWrite[2] = (CDS_IPC_COMMS *)(addr + IPC_PCIE_BASE_OFFSET + RFM1_OFFSET);
    }
    // NIC 1 should be connection to EX Dolphin switch (RFM0)
    if(ii == 1) {
	pIpcDataRead[0] = (CDS_IPC_COMMS *)(read_addr + IPC_PCIE_BASE_OFFSET + RFM0_OFFSET);
	pIpcDataWrite[0] = (CDS_IPC_COMMS *)(addr + IPC_PCIE_BASE_OFFSET + RFM0_OFFSET);
    }
    // NIC 2 should be connection to EY Dolphin switch (RFM1)
    if(ii == 2) {
	pIpcDataRead[3] = (CDS_IPC_COMMS *)(read_addr + IPC_PCIE_BASE_OFFSET + RFM1_OFFSET);
	pIpcDataWrite[3] = (CDS_IPC_COMMS *)(addr + IPC_PCIE_BASE_OFFSET + RFM1_OFFSET);
    }
}
  // Print read/write addresses to dmesg for diagnostics
  for(ii=0;ii<4;ii++) {
    printk ("Dolphin %d memory read  at 0x%p\n", ii, pIpcDataRead[ii]);
  }
  for(ii=0;ii<4;ii++) {
    printk ("Dolphin %d memory write at 0x%p\n", ii, pIpcDataWrite[ii]);
  }


  return 0;
}

// ************************************************************************
void finish_dolphin(int dolphinCount) {
// Routine to disconnect code from Dolphin NICs when existing.
// ************************************************************************
int ii;
int err;
  
  printk("Disconnecting Dolphin Cards\n");
  for(ii=0;ii<dolphinCount;ii++) {
	  err = sci_unmap_segment(&client_map_handle[ii], 0);
	  printk("Card %d unmap = 0x%x\n",ii,err);
	  err = sci_disconnect_segment(&remote_segment_handle[ii], 0);
	  printk("Card %d disconnect = 0x%x\n",ii,err);
	  err = sci_unexport_segment(segment[ii], ii, 0);
	  printk("Card %d unexport = 0x%x\n",ii,err);
	  err = sci_remove_segment(&segment[ii], 0);
	  printk("Card %d remove = 0x%x\n",ii,err);
	  err = sci_cancel_session_cb(ii, 0);
	  printk("Card %d cancel session = 0x%x\n\n",ii,err);
  }
}

// ************************************************************************
// ************************************************************************
// Switching code communicates info with user space via a /proc file.
// The following routines provide the necessary open/read/close file functions.
// ************************************************************************
int cdsrfm_proc_open(struct inode *sp_inode,struct file *sp_file) {
// ************************************************************************
	// printk("proc called open \n");
	read_p = 1;
	message = kmalloc(sizeof(char)*128,__GFP_WAIT|__GFP_IO|__GFP_FS);
	if(message == NULL) {
		printk("ERROR counter proc open\n");
		return -ENOMEM;
	}
	sprintf(message,"%ld %ld %ld %ld %ld %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
						mycounter[0],mycounter[1],mycounter[2],mycounter[3],mycounter[9],
						myactive[0][0],myactive[0][1],myactive[0][2],
						myactive[1][0],myactive[1][1],myactive[1][2],
						myactive[2][0],myactive[2][1],myactive[2][2],
						myactive[3][0],myactive[3][1],myactive[3][2],
						mysysstatus);
	return 0;
}

// ************************************************************************
ssize_t cdsrfm_proc_read(struct file *sp_file, char __user *buf,size_t size,loff_t *offset) {
// ************************************************************************
  int ret;
	int len = strlen(message);
	read_p = !read_p;
	if(read_p) return 0;
	// if(offset > 0) return 0;
	// printk("proc called read \n");
	ret = copy_to_user(buf,message,len);
	return len;
}

// ************************************************************************
int cdsrfm_proc_release(struct inode *sp_inode,struct file *sp_file) {
// ************************************************************************
	// printk("proc called release \n");
	kfree(message);
	return 0;
}

static int test_data __initdata = RFMX_NUM_DOLPHIN_CARDS;

// ************************************************************************
// ************************************************************************
// ************************************************************************
static int __init lr_switch_init(void)
// Kernel module initialization function.
// ************************************************************************
// ************************************************************************
// ************************************************************************
{
	printk(KERN_INFO "Starting CDS RFM SWITCH %d\n", test_data);
	// Initialize Dolphin NICs and get data pointers.
	init_dolphin(RFMX_NUM_DOLPHIN_CARDS,&mdi);
	// Reset variable used to stop data xfer threads on rmmod.
	stop_working_threads = 0;

// Following code not used, but left here in case want to xfer data
// to EPICS via shared memory at some point in the future.
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


	// Set thread parameters for IPC channel monitoring thread
	strcpy(threads[0].name,"cdsrfmnetmon");
	// Set thread delays
	threads[0].delay = 1000;
	// Set thread number of IPC channels to monitor per RFM network
	threads[0].idx = RFMX_MAX_CHANS_PER_RFM;
	// Set thread netFrom (NOT USED)
	threads[0].netFrom = 0;
	// Set thread netto (NOT USED)
	threads[0].netTo = 0;
	//
	// Create thread that monitors switch port activity.
	sthread[0] = kthread_create(monitorActiveConnections,(void *)&threads[0],"cdsrfmnetmon");
	if(IS_ERR(sthread[0])) {
		printk("ERROR! kthread_run\n");
		return PTR_ERR(sthread[0]);
	}
	// Bind thread to CPU 2
	kthread_bind(sthread[0],2);
	// Start thread
	wake_up_process(sthread[0]);

	// Starting data switching threads   *****************************************************
	// Start thread which moves data for RFM0 IPC channels 0 - 31
	printk("Shutting down CPU 3 at %ld\n",current_time());
	set_fe_code_idle(copyRfmDataEX2CS0,3);
	msleep(100);
	cpu_down(3);

	// Start thread which moves data for RFM0 IPC channels 32 -63 
	printk("Shutting down CPU 4 at %ld\n",current_time());
	set_fe_code_idle(copyRfmDataEX2CS1,4);
	msleep(100);
	cpu_down(4);

	// Start thread which moves data for RFM1 IPC channels 0 - 31
	printk("Shutting down CPU 5 at %ld\n",current_time());
	set_fe_code_idle(copyRfmDataEY2CS0,5);
	msleep(100);
	cpu_down(5);

	// Start thread which moves data for RFM1 IPC channels 32 - 63
	printk("Shutting down CPU 6 at %ld\n",current_time());
	set_fe_code_idle(copyRfmDataEY2CS1,6);
	msleep(100);
	cpu_down(6);

	// Setup /proc file to move diag info out to user space for EPICS
	fops.open = cdsrfm_proc_open;
	fops.read = cdsrfm_proc_read;
	fops.release = cdsrfm_proc_release;

	// Create the /proc file
	if(!proc_create(ENTRY_NAME,PERM,NULL,&fops)) {
		printk("ERROR! proc_create\n");
		remove_proc_entry(ENTRY_NAME,NULL);
		return -ENOMEM;
	}
	return 0;
}

// ************************************************************************
// ************************************************************************
// ************************************************************************
static void __exit lr_switch_exit(void)
// Kernel module exit routine.
// ************************************************************************
{
  int ret;
  extern int __cpuinit cpu_up(unsigned int cpu);
	printk(KERN_INFO "Goodbye, cdsrfmswitch 3 is shutting down\n");

	// Stop the Active Channel monitor thread
	ret = kthread_stop(sthread[0]);
	if (ret != -EINTR)
		printk("RFM thread has stopped %ld\n",mycounter[1]);
	ssleep(2);


	// Stop switching threads and bring CPUs back on line *************
	set_fe_code_idle(0, 3);
	msleep(1000);
	set_fe_code_idle(0, 4);
	msleep(1000);
	set_fe_code_idle(0, 5);
	msleep(1000);
	set_fe_code_idle(0, 6);
	msleep(1000);

	// Set variable to stop the CPU locked switching tasks
	stop_working_threads = 1;
	msleep(1000);

	// Bring CPU 3 back on line
	set_fe_code_idle(0, 3);
	printkl("Will bring back CPU %d\n", 3);
	msleep(1000);
	cpu_up(3);
	printkl("Brought CPU 3 back up\n");

	// Bring CPU 4 back on line
	set_fe_code_idle(0, 4);
	printkl("Will bring back CPU %d\n", 4);
	msleep(1000);
	cpu_up(4);
	printkl("Brought CPU 4 back up\n");

	// Bring CPU 5 back on line
	set_fe_code_idle(0, 5);
	printkl("Will bring back CPU %d\n", 5);
	msleep(1000);
	cpu_up(5);
	printkl("Brought CPU 5 back up\n");

	// Bring CPU 6 back on line
	set_fe_code_idle(0, 6);
	printkl("Will bring back CPU %d\n", 6);
	msleep(1000);
	cpu_up(6);
	printkl("Brought CPU 6 back up\n");
	msleep(1000);

	// Remove /proc file entries
	printk("Removing /proc/%s.\n",ENTRY_NAME);
	remove_proc_entry(ENTRY_NAME,NULL);

	// Cleanup the dolphin NIC connections
	finish_dolphin(mdi.dolphinCount);
}

module_init(lr_switch_init);
module_exit(lr_switch_exit);

MODULE_AUTHOR("R.Bork");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Long Range PCIe Switch");
MODULE_SUPPORTED_DEVICE("Long Range PCIe Switch");
