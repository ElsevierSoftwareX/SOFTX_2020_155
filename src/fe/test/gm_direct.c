/* test gm_direct_send() latency */
#include <stdio.h>
#include <rtl_time.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <drv/gmnet.h>

/* gm receiver host name */
char receiver_nodename[64];

/* if we are running in slave mode */
int slave = 0;

struct gm_port *netPort = 0;
void *netInBuffer = 0;
static void *netOutBuffer = 0;
gm_status_t status;
gm_u32_t receiver_node_id;
gm_s_e_id_message_t *id_message = 0;
unsigned int my_global_id = 0;
gm_u32_t my_node_id;

void
cleanup() {
  if (netPort) {
    if (netInBuffer) {
      gm_dma_free (netPort, netInBuffer);
    }
    if (netOutBuffer) {
      gm_dma_free (netPort, netOutBuffer);
    }
    if (id_message) {
      gm_dma_free (netPort, id_message);
    }
    gm_close (netPort);
    gm_finalize();
  }
  gm_exit (status);
}


typedef struct
{
  int messages_expected;
  int callbacks_pending;
} gm_s_e_context_t;

gm_s_e_context_t context;

/* This function is called inside gm_unknown() when there is a callback
   ready to be processed.  It tells us that a send has completed, either
   successfully or with error. */
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
      printf ("**** Send dropped!\n");
      break;

    default:
      gm_perror ("Send completed with error", the_status);
    }
}

int
main(int argc, char *argv[])
{

  if (argc != 2) {
	printf ("Usage: %s --slave | %s <receive node name>\n",
		 argv[0], argv[0]);
	return 1;
  }
  if (!strcmp(argv[1], "--slave")) slave = 1;
  else strcpy(receiver_nodename, argv[0]);

  // Initialize interface
  gm_init();

  status = gm_open (&netPort, 0 /* board */, 2 /*port */, "blah",
		    (enum gm_api_version) GM_API_VERSION_1_1);
  if (status != GM_SUCCESS) {
	gm_perror ("[send] Couldn't open GM port", status);
	cleanup();
	return 1;
  }

  netInBuffer = gm_dma_calloc (netPort, GM_RCV_BUFFER_COUNT,
                               GM_RCV_BUFFER_LENGTH);
  if (netInBuffer == 0) {
      printf ("Couldn't allocate netInBuffer\n");
      cleanup();
      return 1;
  }
  gm_provide_receive_buffer (netPort, netInBuffer, GM_RCV_MESSAGE_SIZE,
                             GM_DAQ_PRIORITY);

  netOutBuffer = gm_dma_calloc (netPort, GM_RCV_BUFFER_COUNT,
                              GM_RCV_BUFFER_LENGTH);
  if (netOutBuffer == 0) {
      printf("[send] Couldn't allocate out_buffer\n");
      cleanup();
      return 1;
  }

  id_message = (gm_s_e_id_message_t *)gm_dma_calloc (netPort, 1,
						     sizeof(*id_message));
  if (id_message == 0) {
      printf ("Couldn't allocate output buffer for id_message\n");
      cleanup();
      return 1;
  }

  gm_get_node_id (netPort, &my_node_id);
  status = gm_node_id_to_global_id (netPort, my_node_id, &my_global_id);
  if (status != GM_SUCCESS) {
      gm_perror ("Couldn't convert node ID to global ID", status);
      cleanup();
      return 1;
  }

  id_message->directed_recv_buffer_addr =
    gm_hton_u64((gm_size_t)netInBuffer);
  id_message->global_id = gm_hton_u32(my_global_id);

  if (!slave) {
       status = gm_host_name_to_node_id_ex (netPort, 10000000,
					    receiver_nodename,
					    &receiver_node_id);
	if (status == GM_SUCCESS)
          printk ("[send] receiver node ID is %d\n", receiver_node_id);
  	else {
      	  printk ("[send] Conversion of nodename %s to node id failed\n",
                 receiver_nodename);
          gm_perror ("[send]", status);
          cleanup();
          return 1;
	}
     /* Transmit init message to slave */  
     {
	unsigned long send_length;
	daqMessage *daqSendMessage;
	daqSendMessage = (daqMessage *)netOutBuffer;
        sprintf (daqSendMessage->message, "STT");
	send_length = (unsigned long) sizeof(*daqSendMessage);
	gm_send_with_callback (netPort,
                         netOutBuffer,
                         GM_RCV_MESSAGE_SIZE,
                         send_length,
                         GM_DAQ_PRIORITY,
                         receiver_node_id,
                         GM_PORT_NUM_RECV,
                         my_send_callback,
                         &context);
        context.callbacks_pending++;
     }
  } else {
     gm_recv_event_t *event;
     daqMessage *rcvData;

     /* Slave receives init message from master */
     event = gm_blocking_receive_no_spin (netPort);
     switch (GM_RECV_EVENT_TYPE(event)) {
       case GM_RECV_EVENT:
       case GM_HIGH_RECV_EVENT:
       //case GM_PEER_RECV_EVENT:
       //case GM_FAST_PEER_RECV_EVENT:
           rcvData = (daqMessage *)gm_ntohp(event->recv.buffer);
           if (rcvData == 0) {
               printf("received zero pointer\n");
               break;
           }
           // Received startup message
           // All clients must send this message on startup
           if (strcmp(rcvData->message,"STT") == 0) {
              gm_u32_t node_id = gm_ntoh_u16(event->recv.sender_node_id);
              printf("Recv'd init from node %d\n", node_id);
              gm_send_with_callback (netPort,
                                 id_message,
                                 GM_RCV_MESSAGE_SIZE,
                                 sizeof(*id_message),
                                 GM_DAQ_PRIORITY,
                                 node_id,
                                 GM_PORT_NUM_SEND,
                                 my_send_callback,
                                 &context);
               context.callbacks_pending++;
	   }
      }
  }
  return 0;
}
