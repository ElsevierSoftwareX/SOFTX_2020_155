#include <linux/types.h>
#include <linux/pci.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <drv/gmnet.h>
#include <drv/myri.h>
#include <rtl_time.h>

int stop_net_manager = 0;
extern unsigned char *_ipc_shm;

typedef struct
{
  int messages_expected;
  int callbacks_pending;
} gm_s_e_context_t;

gm_s_e_context_t context;

static void
my_send_callback (struct gm_port *port, void *the_context,
                  gm_status_t the_status)
{
  /* One pending callback has been received */
  ((gm_s_e_context_t *)the_context)->callbacks_pending--;

  switch (the_status)
    {
    case GM_SUCCESS:
      break;

    case GM_SEND_DROPPED:
      printk ("**** Send dropped!\n");
      break;

    default:
      printk ("Send completed with error %d", the_status);
    }
}


void *manage_network(void *foo)
{
  gm_s_e_id_message_t *id_message;

  gm_u32_t my_node_id;
  unsigned int my_global_id = 0;
  gm_get_node_id (netPort, &my_node_id);
  gm_status_t status = gm_node_id_to_global_id (netPort, my_node_id, &my_global_id);
  if (status != GM_SUCCESS) {
      printk ("Couldn't convert node ID to global ID; error=%d", status);
      return 0;
  }

  /* FIXME: Program appears to crash badly on a 4-way x86_64 system */
  /* Since I can't register shared memory, I can't use this program */
  /* Register IPC memory; Only first 4K are registered  */
  status = gm_register_memory(netPort, _ipc_shm, GM_PAGE_LEN);
  if (status != GM_SUCCESS) {
      printk ("Couldn't register memory for DMA; error=%d", status);
      return 0;
  }

  /* Allocate message buffer */
  id_message = (gm_s_e_id_message_t *)gm_dma_calloc (netPort, 1, sizeof(gm_s_e_id_message_t));
  if (id_message == 0) {
      printk ("Couldn't allocate output buffer for id_message\n");
      return 0;
  }

  id_message->directed_recv_buffer_addr = gm_hton_u64((gm_size_t)_ipc_shm);
  id_message->global_id = gm_hton_u32(my_global_id);

  /* Receive messages */
  for (;!stop_net_manager;) {
    gm_recv_event_t *event = gm_receive (netPort);
    switch (GM_RECV_EVENT_TYPE(event)) {
	case GM_RECV_EVENT:
	case GM_HIGH_RECV_EVENT:
		{
        	  daqMessage *rcvData = (daqMessage *)gm_ntohp(event->recv.buffer);
        	  if (rcvData == 0) {
               		printk("Received zero pointer!\n");
               		break;
		  }
        	  if (strcmp(rcvData->message,"STT") == 0) {
          	    gm_u32_t node_id = gm_ntoh_u16(event->recv.sender_node_id);
          	    int dcuId = ntohl(rcvData->dcuId);
          	    printk("Recv'd init from dcuId = %d GM node id %d\n", dcuId, node_id);
          	    // Send back the id_message, which includes the DMA memory address info
          	    gm_send_with_callback (netPort,
                                 	id_message,
                                 	GM_RCV_MESSAGE_SIZE,
                                 	sizeof(gm_s_e_id_message_t),
                                 	GM_DAQ_PRIORITY,
                                 	node_id,
                                 	GM_PORT_NUM_SEND,
                                 	my_send_callback,
                                 	&context);
	            context.callbacks_pending++;
		  }
        	}
	case GM_NO_RECV_EVENT:
		break;
	default:
		// This is where sent message callbacks go
		gm_unknown (netPort, event);  /* gm_unknown calls the callback */
    }
    rtl_usleep(100000);
  }

  gm_dma_free (netPort, id_message);
  return 0;
}
