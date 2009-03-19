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

static char *sysname = "om1";
static char *sysname_daq = "om1_daq";
#define MMAP_SIZE 1024*1024*64

int init_module() {
  // Allocate FE to Epics comm area
  void *addr = rtai_kmalloc(nam2num(sysname), MMAP_SIZE);
  printk("nam2num(%s)=%d; returned addr = 0x%x\n", sysname, nam2num(sysname), addr);
  // Allocate FE DAQ area
  addr = rtai_kmalloc(nam2num(sysname_daq), MMAP_SIZE);
  printk("nam2num(%s)=%d; returned addr = 0x%x\n", sysname_daq, nam2num(sysname), addr);
  // Allocate IPC area
  //rtai_kmalloc(nam2num("ipc"), MMAP_SIZE);
  return 0;
}
void cleanup_module (void) {
  rtai_kfree(nam2num(sysname));
  rtai_kfree(nam2num(sysname_daq));
  rtai_kfree(nam2num("ipc"));
}
