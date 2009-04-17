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

// This is the default board rank
static const uint32_t default_board_rank = 1;

// How long to wait for connection is milliseconds
static const uint32_t connection_wait_time = 1000;

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
  int i;

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
  for (i = 0; i < cds_remote_ipc_nodes; i++) {
    mx_endpoint_addr_t dest;
    uint64_t his_nic_id;
    uint16_t his_eid = remote_nodes[i].port;
    mx_return_t ret;

    char node[128];
    sprintf(node,"%s:%d", remote_nodes[i].nodename, default_board_rank);
    ret = mx_hostname_to_nic_id(node, &his_nic_id);
    if (ret != MX_SUCCESS) {
	printk("Mx host \"%s\" not found\n", remote_nodes[i].nodename);
  	continue;
    }
    ret = mx_connect(ep, his_nic_id, his_eid, filter, connection_wait_time, &dest);
    if (ret != MX_SUCCESS) {
	printk("Mx connect failed to node \"%s\", %s\n", node, mx_strerror(ret));
	continue;
    } else {
	printk("Sucessfully connected to node %s\n", node);
    }
  }

  cds_mx_finalize();
}

void cds_mx_finalize() {
  mx_close_endpoint(ep);
  mx_finalize();
}

#endif
