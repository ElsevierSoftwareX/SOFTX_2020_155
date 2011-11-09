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
#include "mbuf.h"

#include "kvmem.c"

/* character device structures */
static dev_t mbuf_dev;
static struct cdev mbuf_cdev;

/* methods of the character device */
static int mbuf_open(struct inode *inode, struct file *filp);
static int mbuf_release(struct inode *inode, struct file *filp);
static int mbuf_mmap(struct file *filp, struct vm_area_struct *vma);
#if HAVE_UNLOCKED_IOCTL
static long mbuf_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
#else
static int mbuf_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);
#endif

/* the file operations, i.e. all character device methods */
static struct file_operations mbuf_fops = {
        .open = mbuf_open,
        .release = mbuf_release,
        .mmap = mbuf_mmap,
#if HAVE_UNLOCKED_IOCTL
	.unlocked_ioctl = mbuf_ioctl,
#else
        .ioctl = mbuf_ioctl,
#endif
        .owner = THIS_MODULE,
};

// internal data
// How many memory areas we will support
#define MAX_AREAS 64

// pointer to the kmalloc'd area, rounded up to a page boundary
void *kmalloc_area[MAX_AREAS];

EXPORT_SYMBOL(kmalloc_area);

// To be used by the IOP to store Dolphin memory state
int iop_rfm_valid;
EXPORT_SYMBOL(iop_rfm_valid);

// Memory area tags (OM1, OM2, etc)
char mtag[MAX_AREAS][MBUF_NAME_LEN + 1];

// Memory usage counters
unsigned int usage_cnt[MAX_AREAS];

// Memory area sizes
unsigned int kmalloc_area_size[MAX_AREAS];


/* character device open method */
static int mbuf_open(struct inode *inode, struct file *filp)
{
        return 0;
}


int mbuf_release_area(char *name, struct file *file) {
	int i;

	// See if allocated
	for (i = 0; i < MAX_AREAS; i++) {
		if (0 == strcmp (mtag[i], name)) {
		// Found the area
		usage_cnt[i]--;
		if (usage_cnt[i] <= 0 ){
				mtag[i][0] = 0;
				usage_cnt[i] = 0;
				if (file) file->private_data = 0;
				rvfree(kmalloc_area[i], kmalloc_area_size[i]);
				kmalloc_area[i] = 0;
				kmalloc_area_size[i] = 0;
			}
			return 0;
		}
	}
        return -1;
}

EXPORT_SYMBOL(mbuf_release_area);

// Returns index of allocated area
// -1 if no slots
int mbuf_allocate_area(char *name, int size, struct file *file) {
	int i;
	// See if already allocated
	for (i = 0; i < MAX_AREAS; i++) {
		if (0 == strcmp (mtag[i], name)) {
			// Found the area
			usage_cnt[i]++;
               		if (file) file -> private_data = mtag [i];
			return i;
		}
	}
	
	// Find first free slot
	for (i = 0; i < MAX_AREAS; i++) {
		if (kmalloc_area[i] == 0) break;
	}
	
	// Out of slots
	if (i >= MAX_AREAS) return -1;

	int s = size;
	kmalloc_area[i] = 0;
	kmalloc_area[i] = rvmalloc (size); //rkmalloc (&s, GFP_KERNEL);
	//printk("rvmalloc() returned %p\n", kmalloc_area[i]);
	//printk("rkmalloc() returned %p %d\n", kmalloc_area[i], s);
	//rkfree(kmalloc_area[i], s);
	//kmalloc_area[i] = 0;
	if (kmalloc_area[i] == 0) {
		printk("malloc() failed\n");
	       	return -1;
	}

	kmalloc_area_size[i] = size;
	strncpy(mtag[i], name, MBUF_NAME_LEN);
	mtag[i][MBUF_NAME_LEN] = 0;
	usage_cnt[i] = 1;
        if (file) file -> private_data = mtag [i];
        return i;
}

EXPORT_SYMBOL(mbuf_allocate_area);

/* character device last close method */
static int mbuf_release(struct inode *inode, struct file *filp)
{
	char *name;
        if (filp -> private_data == 0) return 0;
	name = (char *) filp -> private_data;
	mbuf_release_area(name, filp);
        return 0;
}

// helper function, mmap's the kmalloc'd area which is physically contiguous
int mmap_kmem(unsigned int i, struct vm_area_struct *vma)
{
        int ret;
        long length = vma->vm_end - vma->vm_start;

	if (kmalloc_area_size[i] < length) {
		//printk("mbuf mmap() request to map 0x%lx bytes; allocated 0x%lx\n", length, kmalloc_area_size[i]);
		return -EINVAL;
	}
	//printk("mbuf mmap() length is 0x%lx\n", length);

	return rvmmap(kmalloc_area[i], length, vma);

#if 0
        /* map the whole physically contiguous area in one piece */
        if ((ret = remap_pfn_range(vma,
                                   vma->vm_start,
                                   ((unsigned int )(kmalloc_area[i])) >> PAGE_SHIFT,
                                   length,
                                   vma->vm_page_prot)) < 0) {
                return ret;
        }
        
        return 0;
#endif
}

/* character device mmap method */
static int mbuf_mmap(struct file *file, struct vm_area_struct *vma)
{
	int i;
	char *name;

        if (file -> private_data == 0) return -EINVAL;
	name = (char *) file -> private_data;
	// Find our memory area
        for (i = 0; i < MAX_AREAS; i++) {
             if (0 == strcmp (mtag[i], name)) {
		return mmap_kmem (i, vma);
	     }
	}
	return -EINVAL;
}


#if HAVE_UNLOCKED_IOCTL
static long mbuf_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
#else
static int mbuf_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
#endif
{
	int i;
	struct mbuf_request_struct req;
        void __user *argp = (void __user *)arg;


	//printk("mbuf_ioctl: command=%d\n", cmd);
        switch(cmd){
        case IOCTL_MBUF_ALLOCATE:
		{
        	  if (copy_from_user (&req, (void *) argp, sizeof (req))) return -EFAULT;
        	  //printk("mbuf_ioctl: name:%.32s, size:%d, cmd:%d, file:%p\n", req.name, req.size, cmd, file);
		  int res = mbuf_allocate_area(req.name, req.size, file);
		  if (res >= 0) return kmalloc_area_size[res];
		  else return -EINVAL;
		}
		break;
        case IOCTL_MBUF_DEALLOCATE:
		{
        	  if (copy_from_user (&req, (void *) argp, sizeof (req))) return -EFAULT;
        	  //printk("mbuf_ioctl: name:%.32s, size:%d, cmd:%d, file:%p\n", req.name, req.size, cmd, file);
		  int res = mbuf_release_area(req.name, file);
		  if (res >= 0) return  0;
		  else return -EINVAL;
		} 
                break;

        case IOCTL_MBUF_INFO:
#if 0
		for (i = 0; i < MAX_AREAS; i++) {
			if (kmalloc_area[i]) {
        		  printk("mbuf %d: name:%.32s, size:%d, usage:%d\n",
				 i, mtag[i], kmalloc_area_size[i], usage_cnt[i]);
			}
		}
#endif
                return 1;
		break;
        default:
                return -EINVAL;
        }
        return -EINVAL;
}

/* module initialization - called at module load time */
static int __init mbuf_init(void)
{
	int i;
        int ret = 0;

        /* get the major number of the character device */
        if ((ret = alloc_chrdev_region(&mbuf_dev, 0, 1, "mbuf")) < 0) {
                printk(KERN_ERR "could not allocate major number for mbuf\n");
                goto out;
        }

        /* initialize the device structure and register the device with the kernel */
        cdev_init(&mbuf_cdev, &mbuf_fops);
        if ((ret = cdev_add(&mbuf_cdev, mbuf_dev, 1)) < 0) {
                printk(KERN_ERR "could not allocate chrdev for buf\n");
                goto out_unalloc_region;
        }

	// Init local data structs
	for ( i = 0; i < MAX_AREAS; i++) {
		kmalloc_area[i] = 0;
		mtag[i][0] = 0;
		usage_cnt[i] = 0;
	}
        return ret;
        
  out_unalloc_region:
        unregister_chrdev_region(mbuf_dev, 1);
  out:
        return ret;
}

/* module unload */
static void __exit mbuf_exit(void)
{
        /* remove the character deivce */
        cdev_del(&mbuf_cdev);
        unregister_chrdev_region(mbuf_dev, 1);
}

module_init(mbuf_init);
module_exit(mbuf_exit);
MODULE_DESCRIPTION("Kernel memory buffer driver");
MODULE_AUTHOR("Alex Ivanov aivanov@ligo.caltech.edu");
MODULE_LICENSE("Dual BSD/GPL");

