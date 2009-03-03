#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_shm.h>
#include <rtai_nam2num.h>

MODULE_DESCRIPTION("Create OM1 shared memory");
MODULE_AUTHOR("Alex Ivanov<aivanov@ligo.caltech.edu>");
MODULE_LICENSE("GPL");

#define MMAP_SIZE 1024*1024*64
int init_module() {
  char *sysname = "om1";
  void *addr = rtai_kmalloc(nam2num(sysname), MMAP_SIZE);
  printk("nam2num(%s)=%d; returned addr = 0x%x\n", sysname, nam2num(sysname), addr);
  return 0;
}
void cleanup_module (void) { rtai_kfree(nam2num("om1")); }
