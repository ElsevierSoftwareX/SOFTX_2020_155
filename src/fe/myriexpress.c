// Wed Apr 22 15:18:53 PDT 2009
// We thought and decided to abondon the idea of using MX in real-time
// in favour of good old reflective memory 
// mx_kisend() call tested in various MX driver modes and it always
// crashed the kernel

#ifdef USE_MX
#include <linux/types.h>
#include <linux/pci.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include <myriexpress.h>

#include <drv/cdsHardware.h>

#include "feSelectHeader.h"


/*
  Rmote IPC Part will have a remote node name specified (hostname).
  Part will also have remote address specified (e.g. 0, 1, 2). Sizeof double.

  The code will establish connections to all nodes specified in IPC parts upon startup
  and will try to reestablish all lost connections at the end of a cycle once a second or so.

*/

// Defaults came from mx_stream
#define FILTER     0x12345
#define MATCH_VAL  0xabcdef


// This is set to 1 since we also have older Myrinet card here
// So the new fast Myrinet board is number 1 here
// This needs to be MX_ANY_NIC if we assume we only have single card
uint32_t board_id = 1;
uint32_t filter = FILTER;

// This is the default board rank
static const uint32_t default_board_rank = 1;

// How long to wait for connection is milliseconds
static const uint32_t connection_wait_time = 1000;

// This node's MX end-point handle
// All MX communication is done using this handle
mx_endpoint_t ep;
int ep_opened = 0;

// MX requests buffers
// In order to send we need to get an empty request object
// There will be a thread to go through requests and call mx_test() on them
// If mx_test() returns success, then the request slot is freed.
#define MAX_MXR 16
mx_request_t mxr[MAX_MXR];
unsigned int  mxr_free[MAX_MXR];

unsigned int cur_mxr;

// Send/receive destinations
mx_endpoint_addr_t dest[MAX_REMOTE_IPC_HOSTS];

// If the node is connected or not
unsigned int dest_connected[MAX_REMOTE_IPC_HOSTS];

// Data buffers
extern unsigned int cds_remote_ipc_nodes;
//extern unsigned int cds_remote_ipc_size; // Second dimension
extern double remote_ipc_send[MAX_REMOTE_IPC_HOSTS][MAX_REMOTE_IPC_VARS];


// This function will transmit all signal buffers to all nodes we have assigned
void cds_mx_send() {
  int i;

  for (i = 0; i < cds_remote_ipc_nodes; i++) {
    if (!dest_connected[i]) continue;

#if 0
    if (!mxr_free[cur_mxr]) {
	mx_status_t   status;
	uint32_t    result = 0;
 	mx_return_t rc = mx_test(ep, &mxr[cur_mxr], &status, &result);
        //printk("called mx_test()\n");
        if (rc != MX_SUCCESS) {
      	  printk("mx_test failed; %s\n", mx_strerror(rc));
        } else if (result == 0) {
		// Request is still not complete, cancel it
		mx_cancel(ep, &mxr[cur_mxr], &result);
        	//printk("called mx_cancel()\n");
	}
	mxr_free[cur_mxr] = 1;
    }
#endif

    unsigned int bsize = sizeof(double)*MAX_REMOTE_IPC_VARS; // Buffers size for single host
    void *data = (void *)&remote_ipc_send[i][0];
    mx_ksegment_t seg;
    seg.segment_ptr = data;
    seg.segment_length =  bsize;
    mx_request_t req;

    //printk("send MX stuff\n");
    mx_return_t ret = mx_kisend(ep, &seg, 1, MX_PIN_KERNEL, dest[i], MATCH_VAL, NULL, &req /*&mxr[cur_mxr]*/);
    if (ret != MX_SUCCESS) {
      printk("mx_kisend failed; %s\n", mx_strerror(ret));
    } else {
	mx_forget(ep, &req);
/*
        mxr_free[cur_mxr] = 0;
        cur_mxr++;
        cur_mxr %= MAX_MXR;
*/
    }
  };
}

// This one will do the receive (if any); This will really only initiate a DMA into 
// receive buffers
void cds_mx_rcv() {
};

int cds_mx_init () {
  int i;

  cur_mxr = 0;

  for (i = 0; i < MAX_MXR; i++) {
  	mxr_free[i] = 1;
  }
  for (i = 0; i < MAX_REMOTE_IPC_HOSTS; i++) {
	dest_connected[i] = 0;
  }
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
  ep_opened = 1;

  // Connect to all nodes
  for (i = 0; i < cds_remote_ipc_nodes; i++) {
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
    ret = mx_connect(ep, his_nic_id, his_eid, filter, connection_wait_time, &dest[i]);
    if (ret != MX_SUCCESS) {
	printk("Mx connect failed to node \"%s\" port %d, %s\n", node, his_eid, mx_strerror(ret));
	continue;
    } else {
	printk("Sucessfully connected to node %s\n", node);
	dest_connected[i] = 1;
    }
  }
}

void cds_mx_finalize() {
  printk("Closing MX\n");
  if (ep_opened) mx_close_endpoint(ep);
  ep_opened = 0;
  mx_finalize();
}

#endif
