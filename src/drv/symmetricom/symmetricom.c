//#include <linux/config.h>
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

/* character device structures */
static dev_t symmetricom_dev;
static struct cdev symmetricom_cdev;

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

static int symmetricom_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
        int i;
        int req;

        switch(cmd){
        case IOCTL_SYMMETRICOM_STATUS:
                {
		  req = 0x123;
                  if (copy_to_user ((void *) arg, &req,  sizeof (req))) return -EFAULT;
                }
                break;
        default:
                return -EINVAL;
        }
        return -EINVAL;
}


/* module initialization - called at module load time */
static int __init symmetricom_init(void)
{
	int i;
        int ret = 0;

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

