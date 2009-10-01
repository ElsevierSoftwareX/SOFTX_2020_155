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
#include <genif.h>

sci_l_segment_handle_t segment;

/* module initialization - called at module load time */
static int __init drfm_init(void)
{
	segment = 0;
/*
scierror_t DLL
sci_create_segment(sci_binding_t binding,
                   unsigned32 IN module_id,
                   unsigned32 IN segment_id,
                   unsigned32 IN flags,
                   unsigned32 IN size,
                   sci_local_segment_cb_t IN callback_func,
                   void IN *callback_arg,
                   sci_l_segment_handle_t OUT *local_segment_handle);

*/
	scierror_t err = 
	sci_create_segment(NO_BINDING,
				0,
				1,
				0,
				16,
				0,
				0,
				&segment);
	printk("DIS segment alloc status %d\n", err);
	if (err) return -1;

/*
scierror_t DLL
sci_export_segment(sci_l_segment_handle_t local_segment_handle,
                                   unsigned32 IN local_adapter_number,
                   unsigned32 IN flags);
*/
	err = sci_export_segment(segment, 0, 0);
	printk("DIS segment export status %d\n", err);
	if (err) {
		sci_remove_segment(&segment, 0);
		return -1;
	}
	
#if 0

Incompatible first argument error

/*
scierror_t DLL
sci_map_segment(sci_r_segment_handle_t IN_OUT remote_segment_handle,
                unsigned32 IN flags,
                unsigned32 IN offset,
                unsigned32 IN size,
                sci_map_handle_t IN_OUT *map_handle);
*/
	sci_map_handle_t map_handle;
	err = sci_map_segment(segment,
				0,
				0,
				16,
				&map_handle);
	printk("DIS segment mapping status %d\n", err);
	if (err) {
		sci_remove_segment(&segment, 0);
		return -1;
	}
#endif

/*
scierror_t DLL
sci_set_local_segment_available (sci_l_segment_handle_t IN local_segment_handle,
                                 unsigned32 uLocalAdapter);

*/
	err = sci_set_local_segment_available(segment, 0);
        printk("DIS segment making available status %d\n", err);
        if (err) {
                sci_remove_segment(&segment, 0);
                return -1;
        }


        return 0;
}

/* module unload */
static void __exit drfm_exit(void)
{
	sci_remove_segment(&segment, 0);
}

module_init(drfm_init);
module_exit(drfm_exit);
MODULE_DESCRIPTION("Dolphin RFM test kernel module");
MODULE_AUTHOR("Alex Ivanov aivanov@ligo.caltech.edu");
MODULE_LICENSE("Dual BSD/GPL");

