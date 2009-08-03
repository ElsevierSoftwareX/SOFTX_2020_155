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
#ifdef MODVERSIONS
#  include <linux/modversions.h>
#endif
#include <asm/io.h>

/* character device structures */
static dev_t mbuf_dev;
static struct cdev mbuf_cdev;

/* methods of the character device */
static int mbuf_open(struct inode *inode, struct file *filp);
static int mbuf_release(struct inode *inode, struct file *filp);
static int mbuf_mmap(struct file *filp, struct vm_area_struct *vma);

/* the file operations, i.e. all character device methods */
static struct file_operations mbuf_fops = {
        .open = mbuf_open,
        .release = mbuf_release,
        .mmap = mbuf_mmap,
        .owner = THIS_MODULE,
};

// internal data
// pointer to the kmalloc'd area, rounded up to a page boundary
unsigned int kmalloc_area[8];

// reflective memory pointers
static int rfm_cnt;
static void *rfm_ptr[8];

/* character device open method */
static int mbuf_open(struct inode *inode, struct file *filp)
{
        return 0;
}

/* character device last close method */
static int mbuf_release(struct inode *inode, struct file *filp)
{
        return 0;
}

// helper function, mmap's the kmalloc'd area which is physically contiguous
int mmap_kmem(struct file *filp, struct vm_area_struct *vma, int card)
{
        int ret;
        long length = vma->vm_end - vma->vm_start;

	printk("mbuf mmap() length is 0x%lx\n", length);

        /* map the whole physically contiguous area in one piece */
        if ((ret = remap_pfn_range(vma,
                                   vma->vm_start,
                                   kmalloc_area[card] >> PAGE_SHIFT,
                                   length,
                                   vma->vm_page_prot)) < 0) {
                return ret;
        }
        
        return 0;
}

/* character device mmap method */
static int mbuf_mmap(struct file *filp, struct vm_area_struct *vma)
{
#if 0
	int offs  = vma->vm_pgoff >> PAGE_SHIFT;
	if (offs < rfm_cnt) return mmap_kmem(filp, vma, offs);
	else return -EIO;
#endif
	return -EIO;
}

/* module initialization - called at module load time */
static int __init mbuf_init(void)
{
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
