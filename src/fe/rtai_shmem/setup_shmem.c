#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_shm.h>
#include <rtai_nam2num.h>

MODULE_DESCRIPTION("Create OM1 shared memory");
MODULE_AUTHOR("Alex Ivanov<aivanov@ligo.caltech.edu>");
MODULE_LICENSE("Postmodern");

#define MMAP_SIZE 1024*1024*64-5000
int init_modules() { (void)rtai_kmalloc(nam2num("om1"), MMAP_SIZE); return 0; }
void cleanup_module (void) { rtai_kfree(nam2num("om1")); }
