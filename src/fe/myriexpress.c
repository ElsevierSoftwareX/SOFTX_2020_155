#ifdef USE_MX
#include <linux/types.h>
#include <linux/pci.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include <myriexpress.h>

#include <drv/cdsHardware.h>

/*
  Rmote IPC Part will have a remote node name specified (hostname).
  Part will also have remote address specified (e.g. 0, 1, 2). Sizeof double.

  The code will establish connections to all nodes specified in IPC parts upon startup
  and will try to reestablish all lost connections at the end of a cycle once a second or so.

*/

// Defaults came from mx_stream
#define DFLT_EID   1
#define FILTER     0x12345
#define MATCH_VAL  0xabcdef


uint32_t board_id = MX_ANY_NIC;
uint32_t filter = FILTER;

// This node's MX end-point handle
// All MX communication is done using this handle
mx_endpoint_t ep;


// This function will transmit all signal buffers to all nodes we have assigned
void cds_mx_send() {
};

// This one will do the receive (if any); This will really only initiate a DMA into 
// receive buffers
void cds_mx_rcv() {
};

int cds_mx_init () {
  mx_init();
  extern CDS_REMOTE_NODES remote_nodes[];
  extern unsigned int cds_remote_ipc_nodes;
  printk ("We have %d MX nodes\n", cds_remote_ipc_nodes);

  // Do not open anything if are not connecting to remote nodes
  if (cds_remote_ipc_nodes <= 0) return 0;

  // Open my endpoint
  extern unsigned int remote_ipc_mx_port;
  mx_return_t ret = mx_open_endpoint(board_id, remote_ipc_mx_port, filter, 0, 0, &ep);
  if (ret != MX_SUCCESS) {
     printk("Failed to open endpoint %s\n", mx_strerror(ret));
     return 1;
  }
  printk("Opened MX endpoint\n");

  // Connect to all nodes
}

void cds_mx_finalize() {
  mx_close_endpoint(ep);
  mx_finalize();
}

#endif
