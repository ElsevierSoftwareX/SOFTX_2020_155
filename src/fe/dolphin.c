#include <genif.h>
#include "commData2.h"

sci_l_segment_handle_t segment;
sci_map_handle_t client_map_handle;
sci_r_segment_handle_t  remote_segment_handle;
sci_device_info_t	sci_dev_info;

/*

R_OK,
       SR_DISABLED,
       SR_WAITING,
       SR_CHECKING,
       SR_CHECK_TIMEOUT,
       SR_LOST,
       SR_OPEN_TIMEOUT,
       SR_HEARTBEAT_RECEIVED
      } session_cb_reason_t;
*/
signed32 session_callback(session_cb_arg_t IN arg,
			  session_cb_reason_t IN reason,
			  session_cb_status_t IN status,
			  unsigned32 IN target_node,
			  unsigned32 IN local_adapter_number) {
  printkl("Session callback reason=%d status=%d target_node=%d\n", reason, status, target_node);
  if (reason == SR_OK) iop_rfm_valid = 1;
  // This is being called when the one of the other nodes is prepared for shutdown
  // :TODO: may need to check target_node == <our local node>
  //if (reason == SR_DISABLED || reason == SR_LOST) iop_rfm_valid = 0;
  return 0;
}

signed32 connect_callback(void IN *arg,
			  sci_r_segment_handle_t IN remote_segment_handle,
			  unsigned32 IN reason, unsigned32 IN status) {
  printkl("Connect callback reason=%d status=%d\n", reason, status);
  if (reason == 1) iop_rfm_valid = 1;
  if (reason == 3) iop_rfm_valid = 0;
  if (reason == 5) iop_rfm_valid = 1;
  return 0;
}

signed32 create_segment_callback(void IN *arg,
				 sci_l_segment_handle_t IN local_segment_handle,
				 unsigned32 IN reason,
				 unsigned32 IN source_node,
				 unsigned32 IN local_adapter_number)  {
  printkl("Create segment callback reason=%d source_node=%d\n", reason, source_node);
  return 0;
}

int
init_dolphin(void) {
  scierror_t err;
  char *addr;
  char *read_addr;
  cdsPciModules.dolphinCount = 0;
  err = sci_create_segment(NO_BINDING,
		       0,
		       1,
		       DIS_BROADCAST,
		       IPC_TOTAL_ALLOC_SIZE,
		       create_segment_callback,
		       0,
		       &segment);
  printk("DIS segment alloc status %d\n", err);
  if (err) return -1;

  err = sci_set_local_segment_available(segment, 0);
  printk("DIS segment making available status %d\n", err);
  if (err) {
    sci_remove_segment(&segment, 0);
    return -1;
  }
  
  err = sci_export_segment(segment, 0, DIS_BROADCAST);
  printk("DIS segment export status %d\n", err);
  if (err) {
    sci_remove_segment(&segment, 0);
    return -1;
  }
  
  read_addr = sci_local_kernel_virtual_address(segment);
  if (read_addr == 0) {
    printk("DIS sci_local_kernel_virtual_address returned 0\n");
    sci_remove_segment(&segment, 0);
    return -1;
  } else {
    printk("Dolphin memory read at 0x%p\n", read_addr);
    cdsPciModules.dolphin[0] = (volatile unsigned long *)read_addr;
  }
  udelay(MAX_UDELAY);
  udelay(MAX_UDELAY);
  
  err = sci_connect_segment(NO_BINDING,
			    4, // DIS_BROADCAST_NODEID_GROUP_ALL
			    0,
			    0,
			    1, 
			    DIS_BROADCAST,
			    connect_callback, 
			    0,
			    &remote_segment_handle);
  printk("DIS connect segment status %d\n", err);
  if (err) {
    sci_remove_segment(&segment, 0);
    return -1;
  }

  // usleep(20000);
  udelay(MAX_UDELAY);
  udelay(MAX_UDELAY);
  err = sci_map_segment(remote_segment_handle,
			DIS_BROADCAST,
			0,
			IPC_TOTAL_ALLOC_SIZE,
			&client_map_handle);
  printk("DIS segment mapping status %d\n", err);
  if (err) {
    sci_disconnect_segment(&remote_segment_handle, 0);
    sci_remove_segment(&segment, 0);
    return -1;
  }
  
  addr = sci_kernel_virtual_address_of_mapping(client_map_handle);
  if (addr == 0) {
    printk ("Got zero pointer from sci_kernel_virtual_address_of_mapping\n");
    sci_disconnect_segment(&remote_segment_handle, 0);
    sci_remove_segment(&segment, 0);
    return -1;
  } else {
    printk ("Dolphin memory at 0x%p\n", addr);
    cdsPciModules.dolphin[1] = (volatile unsigned long *)addr;
  }

  sci_register_session_cb(0,0,session_callback,0);
  cdsPciModules.dolphinCount = 1;

/*
IRM19_sci_get_device_info(unsigned32 IN local_adapter_number,
                    unsigned32 IN flags,
                    sci_device_info_t OUT *info_ptr)
*/
  if (ESCI_OK != IRM19_sci_get_device_info(0, 0, &sci_dev_info)) {
	printk ("Could not do IRM19_sci_get_device_info()\n");
	return -1;
  }

  
  return 0;
}

void 
finish_dolphin(void) {
  sci_unmap_segment(&client_map_handle, 0);
  sci_disconnect_segment(&remote_segment_handle, 0);
  sci_unexport_segment(segment, 0, 0);
  sci_remove_segment(&segment, 0);
  sci_cancel_session_cb(0, 0);
}

