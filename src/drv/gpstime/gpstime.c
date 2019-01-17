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
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>

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

/* What type of syncing does the driver need */
static int gps_module_sync_type = STATUS_SYMMETRICOM_NO_SYNC;

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
    unsigned long req[3];
    unsigned long res = 0;

    switch(cmd){
        case IOCTL_SYMMETRICOM_STATUS:
            res = get_cur_time(req);
            if (copy_to_user ((void *) arg, &res,  sizeof (res))) return -EFAULT;
            break;
        case IOCTL_SYMMETRICOM_TIME:
            get_cur_time(req);
            if (copy_to_user ((void *) arg, req,  sizeof (req))) return -EFAULT;
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

/* The output buffer provided to all sysfs calls here is PAGE_SIZED (ie typically 4k bytes) */
static ssize_t gpstime_sysfs_card_present_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", (int)card_present);
}

static ssize_t gpstime_sysfs_card_type_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", (int)card_type);
}

static ssize_t gpstime_sysfs_gps_offset_type_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", (int)gps_module_sync_type);
}

static ssize_t gpstime_sysfs_gps_offset_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%lld\n", (long long)atomic64_read(&gps_offset));
}

static ssize_t gpstime_sysfs_gps_offset_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	long long new_offset = 0;
	char localbuf[CDS_LOCAL_GPS_BUFFER_SIZE+1];
    size_t full_count = count;
    int conv_ret = 0;

    memset(localbuf, 0, CDS_LOCAL_GPS_BUFFER_SIZE+1);
    // The symmetricom driver doesn't need a tweak on the timing, so don't allow non-zero tweaks
    if (card_present && card_type == 0) {
        printk("gpstime_sysfs_gps_offset_store called when a symmetricom card is present, ignoring the value");
        return full_count;
    }
    // just ignore input when it is too big
	if (count >= CDS_LOCAL_GPS_BUFFER_SIZE) {
        printk("gpstime_sysfs_gps_offset_store called with too much input, ignoring the value");
        return -EFAULT;
    }
    memcpy(localbuf, buf, count);
	localbuf[count] = '\0';

    if ((conv_ret = kstrtoll(localbuf, 10, &new_offset)) != 0) {
        printk("gpstime_sysfs_gps_offset_store error converting offset into an integer - %s %d", (conv_ret == -ERANGE ? "range" : "overflow"), conv_ret);
        return -EFAULT;
    }
    atomic64_set(&gps_offset, new_offset);
    printk("gpstime_sysfs_gps_offset_store success - new value = %lld", new_offset);
    /* tell it we consumed everything */
	return (ssize_t)count;
}

static ssize_t gpstime_sysfs_status_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    unsigned long req[3];
    int status = (int)get_cur_time(req);
    return sprintf(buf, "%d\n", status);
}

static ssize_t gpstime_sysfs_gpstime_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    unsigned long req[3];
    get_cur_time(req);
    return sprintf(buf, "%ld.%02ld\n", req[0], req[1]/10000);
}


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


/* sysfs related structures */
static struct kobject *gpstime_sysfs_dir = NULL;

/* Individual sysfs attributes (ie files) */
static struct kobj_attribute sysfs_card_present_attr = __ATTR(card_present, 0444, gpstime_sysfs_card_present_show, NULL);
static struct kobj_attribute sysfs_card_type_attr = __ATTR(card_type, 0444, gpstime_sysfs_card_type_show, NULL);
static struct kobj_attribute sysfs_gps_offset_type_attr = __ATTR(offset_type, 0444, gpstime_sysfs_gps_offset_type_show, NULL);
static struct kobj_attribute sysfs_gps_offset_attr = __ATTR(offset, 0644, gpstime_sysfs_gps_offset_show, gpstime_sysfs_gps_offset_store);
static struct kobj_attribute sysfs_status_attr = __ATTR(status, 0444, gpstime_sysfs_status_show, NULL);
static struct kobj_attribute sysfs_gpstime_attr = __ATTR(time, 0444, gpstime_sysfs_gpstime_show, NULL);

/* group the attributes together for bulk operations */
static struct attribute *gpstime_fields[] = {
        &sysfs_card_present_attr.attr,
        &sysfs_card_type_attr.attr,
        &sysfs_gps_offset_type_attr.attr,
        &sysfs_gps_offset_attr.attr,
        &sysfs_status_attr.attr,
        &sysfs_gpstime_attr.attr,
        NULL,
};

static struct attribute_group gpstime_attr_group = {
        .attrs = gpstime_fields,
};



/* module initialization - called at module load time */
static int __init symmetricom_init(void)
{
    int ret = 0;
    int card_needs_sync = STATUS_SYMMETRICOM_NO_SYNC;
    struct pci_dev *symdev = NULL;
    proc_gps_entry = NULL;

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

    gpstime_sysfs_dir = kobject_create_and_add("gpstime", kernel_kobj);
    if (gpstime_sysfs_dir == NULL) {
        printk(KERN_ERR "Could not create /sys/kernel/gpstime directory!\n");
        goto out_remove_proc_entry;
    }
    if (sysfs_create_group(gpstime_sysfs_dir, &gpstime_attr_group) != 0) {
        printk(KERN_ERR "Could not create /sys/kernel/gpstime/... fields!\n");
        goto out_remove_proc_entry;
    }

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
        gps_module_sync_type = STATUS_SYMMETRICOM_LOCALTIME_SYNC;
	} else if (card_type == 1) {
        spectracomGpsInitCheckSync(&cdsPciModules, symdev, &card_needs_sync);
        gps_module_sync_type = ( card_needs_sync ? STATUS_SYMMETRICOM_YEAR_SYNC : STATUS_SYMMETRICOM_NO_SYNC);
	} else if (card_type == 0) {
		symmetricomGpsInit(&cdsPciModules, symdev);
        gps_module_sync_type = STATUS_SYMMETRICOM_NO_SYNC;
	}


        return ret;
out_remove_proc_entry:
    if (gpstime_sysfs_dir != NULL) {
        kobject_del(gpstime_sysfs_dir);
        gpstime_sysfs_dir = NULL;
    }
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
    if (gpstime_sysfs_dir != NULL) {
        kobject_del(gpstime_sysfs_dir);
        gpstime_sysfs_dir = NULL;
    }
}

EXPORT_SYMBOL(ligo_get_gps_driver_offset);
module_init(symmetricom_init);
module_exit(symmetricom_exit);
MODULE_DESCRIPTION("LIGO GPS time driver.  Contains support for using Symmetricom/Spectricom cards and system time.");
MODULE_AUTHOR("Alex Ivanov aivanov@ligo.caltech.edu");
MODULE_LICENSE("Dual BSD/GPL");

