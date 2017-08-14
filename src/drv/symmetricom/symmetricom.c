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
#include "../../include/proc.h"


CDS_HARDWARE cdsPciModules;

#include "../../include/drv/spectracomGPS.c"
#include "../../include/drv/symmetricomGps.c"

// /proc/gps entry
struct proc_dir_entry *proc_gps_entry;

/* character device structures */
static dev_t symmetricom_dev;
static struct cdev symmetricom_cdev;
static int card_present;
static int card_type; /* 0 - symmetricom; 1 - spectracom */

/* methods of the character device */
static int symmetricom_open(struct inode *inode, struct file *filp);
static int symmetricom_release(struct inode *inode, struct file *filp);
#ifdef HAVE_UNLOCKED_IOCTL
static int symmetricom_ioctl(struct inode *inode, unsigned int cmd, unsigned long arg);
#else
static int symmetricom_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);
#endif

/* the file operations, i.e. all character device methods */
static struct file_operations symmetricom_fops = {
        .open = symmetricom_open,
        .release = symmetricom_release,
#ifdef HAVE_UNLOCKED_IOCTL
        .unlocked_ioctl = symmetricom_ioctl,
#else
        .ioctl = symmetricom_ioctl,
#endif
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

// Read current GPS time from the card
int get_cur_time(unsigned long *req) {
  unsigned int timeSec;
  unsigned int timeUsec;
  if (card_present && card_type == 1) {
	int sync = getGpsTimeTsync(&timeSec, &timeUsec);
	req[0] = timeSec; req[1] = timeUsec; req[2] = 0;
	return sync;
  } else if (card_present && card_type == 0) {
  	lockGpsTime();
	int sync = getGpsTime(&timeSec, &timeUsec);
	req[0] = timeSec; req[1] = timeUsec; req[2] = 0;
	return sync;
  } else {
	// Get current kernel time (in GPS)
        struct timespec t;
        extern struct timespec current_kernel_time(void);
        t = current_kernel_time();
        t.tv_sec += - 315964819 + 37;
 	req[0] = t.tv_sec; req[1] = 0; req[2] = t.tv_nsec;
	return 1;
  }
  // Never reached
}

#ifdef HAVE_UNLOCKED_IOCTL
static int symmetricom_ioctl(struct inode *inode, unsigned int cmd, unsigned long arg)
#else
static int symmetricom_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
#endif
{

        switch(cmd){
        case IOCTL_SYMMETRICOM_STATUS:
                {
        	  unsigned long req[3];
		  unsigned long res = get_cur_time(req);
                  if (copy_to_user ((void *) arg, &res,  sizeof (res))) return -EFAULT;
                }
                break;
        case IOCTL_SYMMETRICOM_TIME:
		{
        	  unsigned long req[3];
		  get_cur_time(req);
                  if (copy_to_user ((void *) arg, req,  sizeof (req))) return -EFAULT;
		}
		break;
        default:
                return -EINVAL;
        }
        return -EINVAL;
}

/// Routine to read the /proc/{model}/gps file. \n \n
///
int
procfile_gps_read(char *buffer,
              char **buffer_location,
              off_t offset, int buffer_length, int *eof, void *data)
{
        *buffer = 0;

        if (offset > 0) {
                /* we have finished to read, return 0 */
                return 0;
        } else {
                /* fill the buffer, return the buffer size */
		unsigned long req[3];
		get_cur_time(req);
                return sprintf(buffer, "%ld.%02ld\n", req[0], req[1]/10000);
        }
        // Never reached
}

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

        // Create /proc/gps filesystem tree
        proc_gps_entry = create_proc_entry("gps", PROC_MODE, NULL );
        if (proc_gps_entry == NULL) {
                printk(KERN_ALERT "Error: Could not initialize /proc/gps\n");
                goto out_unalloc_region;
        }

        proc_gps_entry->read_proc = procfile_gps_read;
        proc_gps_entry->mode     = S_IFREG | S_IRUGO;
        proc_gps_entry->uid      = PROC_UID;
        proc_gps_entry->gid      = 0;
        proc_gps_entry->size     = 128;

	/* find the Symmetricom device */
	struct pci_dev *symdev = pci_get_device (SYMCOM_VID, SYMCOM_BC635_TID, 0);
	if (symdev) {
		printk("Symmetricom GPS card on bus %x; device %x\n", symdev->bus->number, PCI_SLOT(symdev->devfn));
		card_present = 1;
		card_type = 0;
	} else {
		ret = 0;
                //printk("Symmetricom GPS card not found\n");
                //goto out_unalloc_region;
		card_present = 0;
	}
	if (!card_present) {
		/* Try looking for Spectracom GPS card */
		symdev = pci_get_device (TSYNC_VID, TSYNC_TID, 0);
		if (symdev) {
			printk("Spectracom GPS card on bus %x; device %x\n", symdev->bus->number, PCI_SLOT(symdev->devfn));
			card_present = 1;
			card_type = 1;
		} else {
			ret = 0;
                	//printk("Symmetricom GPS card not found\n");
                	//goto out_unalloc_region;
			card_present = 0;
		}
	}

	if (!card_present) {
        	printk("Symmetricom/Spectracom GPS card not found\n");
	} else if (card_type == 1) {
		spectracomGpsInit(&cdsPciModules, symdev);
	} else if (card_type == 0) {
		symmetricomGpsInit(&cdsPciModules, symdev);
	}
        return ret;
  out_remove_proc_entry:
	remove_proc_entry("gps", NULL);
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
	remove_proc_entry("gps", NULL);
}

module_init(symmetricom_init);
module_exit(symmetricom_exit);
MODULE_DESCRIPTION("Symmetricom GPS card driver");
MODULE_AUTHOR("Alex Ivanov aivanov@ligo.caltech.edu");
MODULE_LICENSE("Dual BSD/GPL");

