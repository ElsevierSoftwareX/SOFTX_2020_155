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
#include <linux/delay.h>
#include <linux/vmalloc.h>

#ifdef MODVERSIONS
#  include <linux/modversions.h>
#endif
#include <asm/io.h>
#include <genif.h>

static int client = 0;
module_param (client, int, 0);
MODULE_PARM_DESC (client, "Be a client, not server");

static int target_node = 0;
module_param (target_node, int, 0);
MODULE_PARM_DESC (target_node, "Target node ID");


sci_l_segment_handle_t segment;
sci_map_handle_t client_map_handle;
sci_r_segment_handle_t  remote_segment_handle;

/* module initialization - called at module load time */

static int __init drfm_init(void)
{
	segment = 0;
	
	if (client) {
			printk ("Dolphin Client");

/*
scierror_t DLL
sci_connect_segment(sci_binding_t binding,
                    unsigned32 IN target_node,
                    unsigned32 IN local_adapter_number,
                    unsigned32 IN module_id,
                    unsigned32 IN segment_id,
                    unsigned32 IN flags,
                    sci_remote_segment_cb_t IN callback_func,
                    void IN *callback_arg,
                    sci_r_segment_handle_t IN_OUT *remote_segment_handle);

*/
		signed32 func(void IN *arg,
                        sci_r_segment_handle_t IN remote_segment_handle,
                        unsigned32 IN reason,
                        unsigned32 IN status) {
				printk("Connect callback %d\n", reason);
				return 0;
		}

		scierror_t err = 
		sci_connect_segment(NO_BINDING,
				target_node,
				0,
				0,
				1,
				0, /* FLAGS */
				func, 0,
				&remote_segment_handle);
		printk("DIS connect segment status %d\n", err);
		if (err) return -1;
		
/*
scierror_t DLL
sci_map_segment(sci_r_segment_handle_t IN_OUT remote_segment_handle,
                unsigned32 IN flags,
                unsigned32 IN offset,
                unsigned32 IN size,
                sci_map_handle_t IN_OUT *map_handle);
*/

	/* TODO: has to wait here for the connection to happen, i.e.
	   the callback gets called */
	udelay(20000);
	err = sci_map_segment(remote_segment_handle,
				PHYS_MAP_ATTR_STRICT_IO,
				0,
				16,
				&client_map_handle);
	printk("DIS segment mapping status %d\n", err);
	if (err) {
		sci_disconnect_segment(&remote_segment_handle, 0);
		return -1;
	}

	
/*
vkaddr_t DLL
sci_kernel_virtual_address_of_mapping (sci_map_handle_t IN map_handle);

*/
	double *addr = sci_kernel_virtual_address_of_mapping(client_map_handle);
	if (addr == 0) {
		printk ("Got zero pointer from sci_kernel_virtual_address_of_mapping\n");
		return -1;
	} else {
		/* Write RFM data */
		*addr = 123456789.123456789;
		int i;
		for (i = 0; i < 30; i++) {
			msleep(1000);
			printk ("data = %f\n", *addr);
		}
	}
	} else {
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
	signed32 func(void IN *arg,
                       sci_l_segment_handle_t IN local_segment_handle,
                       unsigned32 IN reason,
                       unsigned32 IN source_node,
                       unsigned32 IN local_adapter_number)  {
				printk("Connect callback %d\n", reason);
				return 0;
	}
	scierror_t err = 
	sci_create_segment(NO_BINDING,
				0,
				1,
				0,
				16,
				func,
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

/*
vkaddr_t DLL
sci_local_kernel_virtual_address (sci_l_segment_handle_t IN local_segment_handle);
*/
	double *addr = sci_local_kernel_virtual_address(segment);
	if (addr == 0) {
		printk("DIS sci_local_kernel_virtual_address returnes 0 \n");
                sci_remove_segment(&segment, 0);
                return -1;
	} else {
		int i;
		for (i = 0; i < 30; i++) {
			msleep(1000);
			printk ("data = %f\n", *addr);
		}
		*addr = 987654321.987654321;
	}
	}
        return 0;
}

/* module unload */
static void __exit drfm_exit(void)
{
	if (client) {
		sci_unmap_segment(&client_map_handle, 0);
		sci_disconnect_segment(&remote_segment_handle, 0);
	} else {
/*
scierror_t DLL
sci_set_local_segment_unavailable (sci_l_segment_handle_t IN local_segment_handle,
                                   unsigned32 uLocalAdapter);
*/
		sci_set_local_segment_unavailable(segment, 0);
/*
scierror_t DLL
sci_unexport_segment(sci_l_segment_handle_t local_segment_handle,
                     unsigned32 IN local_adapter_number,
                     unsigned32 IN flags);

*/
		sci_unexport_segment(segment, 0, 0);
		sci_remove_segment(&segment, 0);
	}
}

module_init(drfm_init);
module_exit(drfm_exit);
MODULE_DESCRIPTION("Dolphin RFM test kernel module");
MODULE_AUTHOR("Alex Ivanov aivanov@ligo.caltech.edu");
MODULE_LICENSE("Dual BSD/GPL");

