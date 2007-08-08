#include <linux/config.h>
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

#define VMIC_VID                0x114a
#define VMIC_TID                0x5565

#define TURN_OFF_5565_FAIL      0x80000000
typedef struct VMIC5565_CSR{
        unsigned char BRV;      /* 0x0 */
        unsigned char BID;      /* 0x1 */
        unsigned char rsv0;     /* 0x2 */
        unsigned char rsv1;     /* 0x3 */
        unsigned char NID;      /* 0x4 */
        unsigned char rsv2;     /* 0x5 */
        unsigned char rsv3;     /* 0x6 */
        unsigned char rsv4;     /* 0x7 */
        unsigned int LCSR1;     /* 0x8 */
        unsigned int rsv5;      /* 0xC */
        unsigned int LISR;      /* 0x10 */
        unsigned int LIER;      /* 0x14 */
        unsigned int NTD;       /* 0x18 */
        unsigned char NTN;      /* 0x1C */
        unsigned char NIC;      /* 0x1D */
        unsigned char rsv15[2]; /* 0x1E */
        unsigned int ISD1;      /* 0x20 */
        unsigned char SID1;     /* 0x24 */
        unsigned char rsv6[3];  /* 0x25 */
        unsigned int ISD2;      /* 0x28 */
        unsigned char SID2;     /* 0x2C */
        unsigned char rsv7[3];  /* 0x2D */
        unsigned int ISD3;      /* 0x30 */
        unsigned char SID3;     /* 0x34 */
        unsigned char rsv8[3];  /* 0x35 */
        unsigned int INITD;     /* 0x38 */
        unsigned char INITN;    /* 0x3C */
        unsigned char rsv9[3];  /* 0x3D */
}VMIC5565_CSR;

typedef struct VMIC5565DMA{
        unsigned int pad[32];
        unsigned int DMA0_MODE;         /* 0x80 */
        unsigned int DMA0_PCI_ADD;      /* 0x84 */
        unsigned int DMA0_LOC_ADD;      /* 0x88 */
        unsigned int DMA0_BTC;          /* 0x8C */
        unsigned int DMA0_DESC;         /* 0x90 */
        unsigned int DMA1_MODE;         /* 0x94 */
        unsigned int DMA1_PCI_ADD;      /* 0x98 */
        unsigned int DMA1_LOC_ADD;      /* 0x9C */
        unsigned int DMA1_BTC;          /* 0xA0 */
        unsigned int DMA1_DESC;         /* 0xA4 */
        unsigned int DMA_CSR;           /* 0xA8 */
        unsigned int DMA_ARB;
        unsigned int DMA_THRESHOLD;
        unsigned int DMA0_PCI_DUAL;
        unsigned int DMA1_PCI_DUAL;
}VMIC5565DMA;

typedef struct VMIC5565RTR{
        unsigned int pad[16];
        unsigned int MBR0;
        unsigned int MBR1;
        unsigned int MBR2;
        unsigned int MBR3;
        unsigned int MBR4;
        unsigned int MBR5;
        unsigned int MBR6;
        unsigned int MBR7;
        unsigned int P2L_DOORBELL;
        unsigned int L2P_DOORBELL;
        unsigned int INTCSR;
}VMIC5565RTR;


#define TURN_OFF_FAIL_LITE	0x80
#define PMC_RAISE_INTR1_ALL	0x41
#define BUS_ID		0
#define PCI_ADDR	2

/* BASE 1 Register Structure */
struct VMIC5579_MEM_REGISTER {
	unsigned char rsv0;	/* 0x0 */
	unsigned char BID;	/* 0x1 */
	unsigned char rsv2;	/* 0x2 */
	unsigned char rsv3;	/* 0x3 */
	unsigned char NID;	/* 0x4 */
	unsigned char rsv5;	/* 0x5 */
	unsigned char rsv6;	/* 0x6 */
	unsigned char rsv7;	/* 0x7 */
	unsigned char IRS;	/* 0x8 */
	unsigned char CSR1;	/* 0x9 */
	unsigned char rsvA;	/* 0xa */
	unsigned char rsvB;	/* 0xb */
	unsigned char CSR2;	/* 0xc */
	unsigned char CSR3;	/* 0xd */
	unsigned char rsvE;	/* 0xe */
	unsigned char rsvF;	/* 0xf */
	unsigned char CMDND;	/* 0x10 */
	unsigned char CMD;	/* 0x11 */
	unsigned char CDR1;	/* 0x12 */
	unsigned char CDR2;	/* 0x13 */
	unsigned char ICSR;	/* 0x14 */
	unsigned char rsv15;	/* 0x15 */
	unsigned char rsv16;	/* 0x16 */
	unsigned char rsv17;	/* 0x17 */
	unsigned char SID1;	/* 0x18 */
	unsigned char IFR1;	/* 0x19 */
	unsigned short IDR1;	/* 0x1a & 0x1b */
	unsigned char SID2;	/* 0x1c */
	unsigned char IFR2;	/* 0x1d */
	unsigned short IDR2;	/* 0x1e & 0x1f */
	unsigned char SID3;	/* 0x20 */
	unsigned char IFR3;	/* 0x21 */
	unsigned short IDR3;	/* 0x22 & 0x23 */
	unsigned long DADD;	/* 0x24 to 0x27 */
	unsigned char EIS;	/* 0x28 */
	unsigned char ECSR3;	/* 0x29 */
	unsigned char rsv2A;	/* 0x2a */
	unsigned char rsv2B;	/* 0x2b */
	unsigned char rsv2C;	/* 0x2c */
	unsigned char MACR;	/* 0x2d */
	unsigned char rsv2E;	/* 0x2e */
	unsigned char rsv2F;	/* 0x2f */
};

/* BASE 0 Register Structure */
struct VMIC5579_PCI_REGISTER {
	unsigned int OMB1;
	unsigned int OMB2;
	unsigned int OMB3;
	unsigned int OMB4;
	unsigned int IMB1;
	unsigned int RSV1;
	unsigned int RSV2;
	unsigned int RSV3;
	unsigned int RSV4;
	unsigned int MWAR;
	unsigned int MWTC;
	unsigned int MRAR;
	unsigned int MRTC;
	unsigned int RSV5;
	unsigned int INTCSR;
	unsigned int MCSR;
};

/* character device structures */
static dev_t mmap_dev;
static struct cdev mmap_cdev;

/* methods of the character device */
static int mmap_open(struct inode *inode, struct file *filp);
static int mmap_release(struct inode *inode, struct file *filp);
static int mmap_mmap(struct file *filp, struct vm_area_struct *vma);

/* the file operations, i.e. all character device methods */
static struct file_operations mmap_fops = {
        .open = mmap_open,
        .release = mmap_release,
        .mmap = mmap_mmap,
        .owner = THIS_MODULE,
};

// internal data
// pointer to the kmalloc'd area, rounded up to a page boundary
unsigned int kmalloc_area;
// reflective memory pointers
static int rfm_cnt;
static void *rfm_ptr[8];

/* character device open method */
static int mmap_open(struct inode *inode, struct file *filp)
{
        return 0;
}
/* character device last close method */
static int mmap_release(struct inode *inode, struct file *filp)
{
        return 0;
}

// helper function, mmap's the kmalloc'd area which is physically contiguous
int mmap_kmem(struct file *filp, struct vm_area_struct *vma)
{
        int ret;
        long length = vma->vm_end - vma->vm_start;

	//printk("mmap() length is 0x%lx\n", length);

        /* map the whole physically contiguous area in one piece */
        if ((ret = remap_pfn_range(vma,
                                   vma->vm_start,
                                   kmalloc_area >> PAGE_SHIFT,
                                   length,
                                   vma->vm_page_prot)) < 0) {
                return ret;
        }
        
        return 0;
}

/* character device mmap method */
static int mmap_mmap(struct file *filp, struct vm_area_struct *vma)
{
	if (vma->vm_pgoff < rfm_cnt) return mmap_kmem(filp, vma);
	else return -EIO;
}

// *****************************************************************************
// Routine to find and map VMIC RFM modules
// *****************************************************************************
unsigned long mapcard(struct pci_dev *pcidev, int memsize) {

  static unsigned int pci_io_addr;
  char *csrAddr;
  static unsigned int csrAddress;
  unsigned int rfmType = pcidev->device;
  static void *pRfmMem;
  VMIC5565_CSR *p5565Csr;
  struct VMIC5579_MEM_REGISTER *p5579Csr;
  

  pci_enable_device(pcidev);
  pci_set_master(pcidev);

  pci_read_config_dword(pcidev,
                 rfmType == 0x5565? PCI_BASE_ADDRESS_3:PCI_BASE_ADDRESS_1,
                 &pci_io_addr);
  printk("VMIC%x PCI bus address=0x%x\n", rfmType, pci_io_addr);

        if (!pci_set_dma_mask(pcidev, DMA_64BIT_MASK)) {
                pci_set_consistent_dma_mask(pcidev, DMA_64BIT_MASK);
                printk("RFM is 64 bit capable\n");
        } else if (!pci_set_dma_mask(pcidev, DMA_32BIT_MASK)) {
                pci_set_consistent_dma_mask(pcidev, DMA_32BIT_MASK);
                printk("RFM is 32 bit capable\n");
        } else {
                printk(
                       "mydev: No suitable DMA available.\n");
        }


  pRfmMem = ioremap_nocache((unsigned long)pci_io_addr, memsize);
  kmalloc_area = pci_io_addr;
  printk("VMIC%x protected address=0x%lx\n", rfmType, (unsigned long)pRfmMem);

  pci_read_config_dword(pcidev,
                 rfmType == 0x5565? PCI_BASE_ADDRESS_2:PCI_BASE_ADDRESS_1,
                 &csrAddress);
  printk("CSR address is 0x%x\n",csrAddress);
  csrAddr = ioremap_nocache((unsigned long)csrAddress, 0x40);

  if(rfmType == 0x5565) {
          p5565Csr = (VMIC5565_CSR *)csrAddr;
          p5565Csr->LCSR1 &= ~TURN_OFF_5565_FAIL;
          printk("Board id = 0x%x\n",p5565Csr->BID);
          printk("Node id = 0x%x\n",p5565Csr->NID);
  } else {
          p5579Csr = (struct VMIC5579_MEM_REGISTER *)pRfmMem;
          p5579Csr->CSR2 = TURN_OFF_FAIL_LITE;
          printk("Node id = 0x%x\n",p5579Csr->NID);
  }
  iounmap ((void *)csrAddr);
  return (unsigned long)pRfmMem;
}

/* module initialization - called at module load time */
static int __init mmap_init(void)
{
        int ret = 0;

        /* get the major number of the character device */
        if ((ret = alloc_chrdev_region(&mmap_dev, 0, 1, "mmap")) < 0) {
                printk(KERN_ERR "could not allocate major number for mmap\n");
                goto out;
        }

        /* initialize the device structure and register the device with the kernel */
        cdev_init(&mmap_cdev, &mmap_fops);
        if ((ret = cdev_add(&mmap_cdev, mmap_dev, 1)) < 0) {
                printk(KERN_ERR "could not allocate chrdev for mmap\n");
                goto out_unalloc_region;
        }

	/* Find reflective memory devices */
	struct pci_dev *rfmdev = NULL;
	static unsigned int VMIC_ID = 0x114a;
	rfm_cnt = 0;
	while((rfmdev = pci_find_device(VMIC_ID, PCI_ANY_ID, rfmdev))) {
		if (rfm_cnt < 8) {
                  printk("rfm card on bus %x; device %x\n", rfmdev->bus->number, PCI_SLOT(rfmdev->devfn));
		  rfm_ptr[rfm_cnt] = (void *) mapcard(rfmdev, 64*1024*1024);
		  rfm_cnt++;
		}
	}

        return ret;
        
  out_unalloc_region:
        unregister_chrdev_region(mmap_dev, 1);
  out:
        return ret;
}

/* module unload */
static void __exit mmap_exit(void)
{
        /* remove the character deivce */
        cdev_del(&mmap_cdev);
        unregister_chrdev_region(mmap_dev, 1);

	struct pci_dev *rfmdev = NULL;
	static unsigned int VMIC_ID = 0x114a;
	rfm_cnt = 0;
	while((rfmdev = pci_find_device(VMIC_ID, PCI_ANY_ID, rfmdev))) {
		if (rfm_cnt < 8) {
                  printk("rfm card on bus %x; device %x\n", rfmdev->bus->number, PCI_SLOT(rfmdev->devfn));
        	  iounmap ((void *)rfm_ptr[rfm_cnt]);
		  rfm_cnt++;
		}
	}
}

module_init(mmap_init);
module_exit(mmap_exit);
MODULE_DESCRIPTION("VMIC rfm mapping driver");
MODULE_AUTHOR("Alex Ivanov aivanov@ligo.caltech.edu");
MODULE_LICENSE("Dual BSD/GPL");
