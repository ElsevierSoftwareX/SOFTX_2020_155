//#include <linux/config.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
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
#include "gpstime.h"
#include "../../include/drv/cdsHardware.h"
#include "../../include/proc.h"


CDS_HARDWARE cdsPciModules;

#define IN_LIGO_GPS_KERNEL_DRIVER 1

#include "../../include/drv/spectracomGPS.c"
#include "../../include/drv/symmetricomGps.c"

#define CDS_LOCAL_GPS_BUFFER_SIZE 30

/* /proc/gps entry */
struct proc_dir_entry *proc_gps_entry;
/* /proc/gps_offset entry */
struct proc_dir_entry *proc_gps_offset_entry;

atomic64_t gps_offset = ATOMIC_INIT(0);

/* character device structures */
static dev_t symmetricom_dev;
static struct cdev symmetricom_cdev;
static int card_present;
static int card_type; /* 0 - symmetricom; 1 - spectracom */

/* methods of the character device */
static int symmetricom_open(struct inode *inode, struct file *filp);
static int symmetricom_release(struct inode *inode, struct file *filp);
#ifdef HAVE_UNLOCKED_IOCTL
static long symmetricom_ioctl(struct file *inode, unsigned int cmd, unsigned long arg);
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
  long offset = 0;
  unsigned int timeSec = 0;
  unsigned int timeUsec = 0;
  int sync = 0;

  offset = atomic64_read(&gps_offset);

  if (card_present && card_type == 1) {
	sync = getGpsTimeTsync(&timeSec, &timeUsec);
	req[0] = timeSec; req[1] = timeUsec; req[2] = 0;
  } else if (card_present && card_type == 0) {
  	lockGpsTime();
	sync = getGpsTime(&timeSec, &timeUsec);
	req[0] = timeSec; req[1] = timeUsec; req[2] = 0;
  } else {
	// Get current kernel time (in GPS)
        struct timespec t;
        extern struct timespec current_kernel_time(void);
        t = current_kernel_time();
        //t.tv_sec += - 315964819 + 37;
	  req[0] = t.tv_sec; req[1] = t.tv_nsec/1000; req[2] = t.tv_nsec%1000;
	  sync = 1;
  }
  req[0] += offset;
  return sync;
}

#ifdef HAVE_UNLOCKED_IOCTL
static long symmetricom_ioctl(struct file *inode, unsigned int cmd, unsigned long arg)
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

/* This is called to translate an item in the sequence to output.
 * In the /proc/gps, there is only one item, and it is the current
 * gps time.
 *
 * So this is called to read the GPS time
 */
static int gps_seq_show(struct seq_file *s, void *v)
{
	unsigned long req[3];
	get_cur_time(req);
	seq_printf(s, "%ld.%02ld\n", req[0], req[1]/10000);
	return 0;
}

static int gps_open(struct inode *inode, struct file *file)
{
	return single_open(file, gps_seq_show, NULL);
}

static struct file_operations gps_file_ops = {
	.owner = THIS_MODULE,
	.open = gps_open,
	.read = seq_read,
	.llseek = seq_lseek,
//	.release = seq_release
	.release = single_release
};


static int gps_offset_seq_show(struct seq_file *s, void *v)
{
	seq_printf(s, "%ld\n", atomic64_read(&gps_offset));
	return 0;
}

static ssize_t gps_offset_write(struct file *f, const char __user *buffer, size_t count,  loff_t *data)
{
	int i = 0;
	long new_offset = 0;
	char localbuf[CDS_LOCAL_GPS_BUFFER_SIZE+1];
    size_t full_count = count;
    size_t remaining = 0;
    int conv_ret = 0;

    memset(localbuf, 0, CDS_LOCAL_GPS_BUFFER_SIZE+1);
    // The symmetricom driver doesn't need a tweak on the timing, so don't allow it
    if (card_present && card_type == 0) {
        printk("gps_offset_write called when a symmetricom card is present, ignoring the value");
        return full_count;
    }
    // just ignore input when it is too big
	if (count >= CDS_LOCAL_GPS_BUFFER_SIZE) {
        printk("gps_offset_write called with too much input, ignoring the value");
        return -EFAULT;
    }
	remaining = copy_from_user(localbuf, buffer, count);
	if (remaining != 0) {
        printk("gps_offset_write was unable to move input to userspace");
        return -EFAULT;
    }
    localbuf[CDS_LOCAL_GPS_BUFFER_SIZE] = 0;
	for (i = 0; localbuf[i] != 0 && i < count; ++i)
	{
        if (!((localbuf[i] >= '0' && localbuf[i] <= '9') || (i == 0 && localbuf[i] == '-')))
        {
            localbuf[i] = 0;
            break;
        }
	}
    localbuf[i] = 0;
    if ((conv_ret = kstrtol(localbuf, 10, &new_offset)) != 0) {
        printk("gps_offset_write error converting offset into an integer - %s", (conv_ret == -ERANGE ? "range" : "overflow"));
        return -EFAULT;
    }
    atomic64_set(&gps_offset, new_offset);
    printk("gps_offset_write success - new value = %ld", new_offset);
    /* tell it we consumed everything */
	return full_count;
}


static int gps_offset_open(struct inode *inode, struct file *file)
{
	return single_open(file, gps_offset_seq_show, NULL);
}

static struct file_operations gps_offset_file_ops = {
	.owner = THIS_MODULE,
	.open = gps_offset_open,
	.read = seq_read,
	.write = gps_offset_write,
	.llseek = seq_lseek,
	.release = single_release
};

static void config_proc_entry(struct proc_dir_entry *proc_entry, int uid)
{
	/* At kernel 3.10+ these helper functions become available
	* and the proc_dir_entry becomes an opaque structure
	*/
#if	LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
	proc_set_size(proc_entry , 128);
	proc_set_user(proc_entry, KUIDT_INIT(uid), KGIDT_INIT(0));
#else
	proc_entry->size = 128;
	proc_entry->uid = uid;
	proc_entry->gid = 0;
#endif
}

/**
 * ligo_get_gps_driver_offset
 * @note Exported interface to return the current gps_offset value within the kernel.
 * @return a long containing the current offset that this driver needs to compute the correct gps time.  This function
 * does not give any information regarding the epoch that is used, it is specific to the hardware in the machine.
 * It is exported to help internal LIGO drivers.
 */
long ligo_get_gps_driver_offset(void)
{
    return atomic64_read(&gps_offset);
}

/* module initialization - called at module load time */
static int __init symmetricom_init(void)
{
        int ret = 0;
    struct pci_dev *symdev = NULL;
    proc_gps_entry = NULL;
    proc_gps_offset_entry = NULL;

        /* get the major number of the character device */
        if ((ret = alloc_chrdev_region(&symmetricom_dev, 0, 1, "gpstime")) < 0) {
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
	proc_gps_entry = proc_create("gps", PROC_MODE | S_IFREG | S_IRUGO, NULL, &gps_file_ops);
        if (proc_gps_entry == NULL) {
                printk(KERN_ALERT "Error: Could not initialize /proc/gps\n");
                goto out_unalloc_region;
        }
	config_proc_entry(proc_gps_entry, PROC_UID);

    proc_gps_offset_entry = proc_create("gps_offset", PROC_MODE | S_IFREG | S_IRUGO, NULL, &gps_offset_file_ops);
    if (proc_gps_entry == NULL) {
        printk(KERN_ALERT "Error: Could not initialize /proc/gps_offset\n");
        goto out_remove_proc_entry;
    }
    config_proc_entry(proc_gps_offset_entry, 0);

	/* find the Symmetricom device */
	symdev = pci_get_device (SYMCOM_VID, SYMCOM_BC635_TID, 0);
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
	remove_proc_entry("gps_offset", NULL);
}

EXPORT_SYMBOL(ligo_get_gps_driver_offset);
module_init(symmetricom_init);
module_exit(symmetricom_exit);
MODULE_DESCRIPTION("LIGO GPS time driver.  Contains support for using Symmetricom/Spectricom cards and system time.");
MODULE_AUTHOR("Alex Ivanov aivanov@ligo.caltech.edu");
MODULE_LICENSE("Dual BSD/GPL");

