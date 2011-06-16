//#include <linux/config.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/pci.h>

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/vmalloc.h>

#ifdef MODVERSIONS
#  include <linux/modversions.h>
#endif
#include <asm/io.h>
#include "symmetricom.h"
#include "../../include/drv/cdsHardware.h"

/* character device structures */
static dev_t symmetricom_dev;
static struct cdev symmetricom_cdev;
static int card_present;

/* methods of the character device */
static int symmetricom_open(struct inode *inode, struct file *filp);
static int symmetricom_release(struct inode *inode, struct file *filp);
static int symmetricom_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);

/* the file operations, i.e. all character device methods */
static struct file_operations symmetricom_fops = {
        .open = symmetricom_open,
        .release = symmetricom_release,
        .ioctl = symmetricom_ioctl,
        .owner = THIS_MODULE,
};

/* character device open method */
static int symmetricom_open(struct inode *inode, struct file *filp)
{
        return 0;
}


/* character device last close method */
static int symmetricom_release(struct inode *inode, struct file *filp)
{
        return 0;
}

static volatile unsigned int *gps;

static int symmetricom_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{

        switch(cmd){
        case IOCTL_SYMMETRICOM_STATUS:
                {
        	  unsigned long req;
		  if (card_present) {
		  unsigned int time0 = gps[0x30/4];
		  if (time0 & (1<<24)) {
		  		//printk("Symmetricom unlocked\n");
				req = 0;
		  } else  {
		  		//printk ("Symmetricom locked!\n");
				req = 1;
		  }
		  } else req = 1;
                  if (copy_to_user ((void *) arg, &req,  sizeof (req))) return -EFAULT;
                }
                break;
        case IOCTL_SYMMETRICOM_TIME:
		{
        	  unsigned long req[3];
		  if (card_present) {
	      	  gps[0] = 1;
	      	  //printk("Current time %ds %dus %dns \n", gps[0x34/4], 0xfffff & gps[0x30/4], 100 * ((gps[0x30/4] >> 20) & 0xf));
		  req[0] = gps[0x34/4];
		  req[1] = 0xfffff & gps[0x30/4];
		  req[2] =  100 * ((gps[0x30/4] >> 20) & 0xf);
		  } else {
		  // Get current kernel time (in GPS)
		           struct timespec t;
		           extern struct timespec current_kernel_time(void);
		           t = current_kernel_time();
		           t.tv_sec += - 315964819 + 33;
		  	req[0] = t.tv_sec; req[1] = 0; req[2] = t.tv_nsec;
		  }
                  if (copy_to_user ((void *) arg, req,  sizeof (req))) return -EFAULT;
		}
		break;
        default:
                return -EINVAL;
        }
        return -EINVAL;
}


#define SYMCOM_VID		0x12e2
#define SYMCOM_BC635_TID	0x4013
#define SYMCOM_BC635_TIMEREQ	0
#define SYMCOM_BC635_EVENTREQ	4
#define SYMCOM_BC635_CONTROL	0x10
#define SYMCOM_BC635_TIME0	0x30
#define SYMCOM_BC635_TIME1	0x34
#define SYMCOM_BC635_EVENT0	0x38
#define SYMCOM_BC635_EVENT1	0x3C
#define SYMCOM_RCVR		0x1


/* module initialization - called at module load time */
static int __init symmetricom_init(void)
{
	int i;
        int ret = 0;
	static unsigned int pci_io_addr;


        /* get the major number of the character device */
        if ((ret = alloc_chrdev_region(&symmetricom_dev, 0, 1, "symmetricom")) < 0) {
                printk(KERN_ERR "could not allocate major number for symmetricom\n");
                goto out;
        }

        /* initialize the device structure and register the device with the kernel */
        cdev_init(&symmetricom_cdev, &symmetricom_fops);
        if ((ret = cdev_add(&symmetricom_cdev, symmetricom_dev, 1)) < 0) {
                printk(KERN_ERR "could not allocate chrdev for buf\n");
                goto out_unalloc_region;
        }

	/* find the Symmetricom device */
	struct pci_dev *symdev = pci_get_device (SYMCOM_VID, SYMCOM_BC635_TID, 0);
	if (symdev) {
		printk("Symmetricom GPS card on bus %x; device %x\n", symdev->bus->number, PCI_SLOT(symdev->devfn));
		card_present = 1;
	} else {
		ret = 0;
                printk("Symmetricom GPS card not found\n");
                //goto out_unalloc_region;
		card_present = 0;
		gps = 0xdeadbeaf;
	}
	
if (card_present) {
	pci_enable_device(symdev);
	pci_read_config_dword(symdev, PCI_BASE_ADDRESS_2, &pci_io_addr);
	pci_io_addr &= 0xfffffff0;
	printk("PIC BASE 2 address = %x\n", pci_io_addr);

	unsigned char *addr1 = (unsigned char *)ioremap_nocache((unsigned long)pci_io_addr, 0x40);
	printk("Remapped 0x%x\n", addr1);
	gps = addr1;
	SYMCOM_REGISTER *timeReg = (TSYNC_REGISTER *) addr1;;


	pci_read_config_dword(symdev, PCI_BASE_ADDRESS_3, &pci_io_addr);
	pci_io_addr &= 0xfffffff0;
	unsigned char *addr3 = (unsigned char *)ioremap_nocache((unsigned long)pci_io_addr, 0x200);
	printk("PIC BASE 3 address = 0x%x\n", pci_io_addr);
	printk("PIC BASE 3 address = 0x%p\n", addr3);

  	unsigned int *dramRead = (unsigned int *)(addr3 + 0x82);
    	unsigned int *cmd = (unsigned int *)(addr3 + 0x102);

	// Enable DC level shift input timecode signal
  // Set write and wait *****************************
  *cmd = 0xf6; // Request model ID
        i=0;
        timeReg->ACK = 0x1;  // Trigger module to capture time
        mdelay(1);
        timeReg->ACK = 0x80;  // Trigger module to capture time
        do{
        mdelay(1);
        i++;
        }while((timeReg->ACK == 0) &&(i<20));
        if(timeReg->ACK) printk("SysCom ack received ID %d !!! 0x%x\n",timeReg->ACK,i);
        printk("Model = 0x%x\n",*dramRead);
// End Wait ****************************************
  //
  // Set write and wait *****************************
        *cmd = 0x4915; // Request model ID
        i=0;
        timeReg->ACK = 0x1;  // Trigger module to capture time
        mdelay(1);
        timeReg->ACK = 0x80;  // Trigger module to capture time
        do{
        mdelay(1);
        i++;
        }while((timeReg->ACK == 0) &&(i<20));
        if(timeReg->ACK) printk("SysCom ack received ID %d !!! 0x%x\n",timeReg->ACK,i);
        printk("Model = 0x%x\n",*dramRead);
// End Wait ****************************************
  //
  // Set write and wait *****************************
        *cmd = 0x4416; // Request model ID
        i=0;
        timeReg->ACK = 0x1;  // Trigger module to capture time
        mdelay(1);
        timeReg->ACK = 0x80;  // Trigger module to capture time
        do{
        mdelay(1);
        i++;
        }while((timeReg->ACK == 0) &&(i<20));
        if(timeReg->ACK) printk("SysCom ack received ID %d !!! 0x%x\n",timeReg->ACK,i);
        printk("Model = 0x%x\n",*dramRead);
// End Wait ****************************************
  //
  // Set write and wait *****************************
        *cmd = 0x1519; // Request model ID
        i=0;
        timeReg->ACK = 0x1;  // Trigger module to capture time
        mdelay(1);
        timeReg->ACK = 0x80;  // Trigger module to capture time
        do{
        mdelay(1);
        i++;
        }while((timeReg->ACK == 0) &&(i<20));
        if(timeReg->ACK) printk("SysCom ack received ID %d !!! 0x%x\n",timeReg->ACK,i);
        printk("New Time COde Format = 0x%x\n",*dramRead);
// End Wait ****************************************
  //
  // Set write and wait *****************************
        *cmd = 0x1619; // Request model ID
        i=0;
        timeReg->ACK = 0x1;  // Trigger module to capture time
        mdelay(1);
        timeReg->ACK = 0x80;  // Trigger module to capture time
        do{
        mdelay(1);
        i++;
        }while((timeReg->ACK == 0) &&(i<20));
        if(timeReg->ACK) printk("SysCom ack received ID %d !!! 0x%x\n",timeReg->ACK,i);
        printk("New TC Modulation = 0x%x\n",*dramRead);
// End Wait ****************************************

	for (i = 0; i < 10; i++) {
	      gps[0] = 1;
	      printk("Current time %ds %dus %dns \n", gps[0x34/4], 0xfffff & gps[0x30/4], 100 * ((gps[0x30/4] >> 20) & 0xf));
	}
	gps[0] = 1;
	unsigned int time0 = gps[0x30/4];
	if (time0 & (1<<24)) printk("Flywheeling, unlocked...\n");
	else printk ("Locked!\n");
}

        return ret;
        
  out_unalloc_region:
        unregister_chrdev_region(symmetricom_dev, 1);
  out:
        return ret;
}

/* module unload */
static void __exit symmetricom_exit(void)
{
        /* remove the character deivce */
        cdev_del(&symmetricom_cdev);
        unregister_chrdev_region(symmetricom_dev, 1);
}

module_init(symmetricom_init);
module_exit(symmetricom_exit);
MODULE_DESCRIPTION("Symmetricom GPS card driver");
MODULE_AUTHOR("Alex Ivanov aivanov@ligo.caltech.edu");
MODULE_LICENSE("Dual BSD/GPL");

