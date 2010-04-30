#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <errno.h>
// #include <linux/slab.h>
#define GM_STRONG_TYPES 1
#include "gm.h"
#include "config.h"
#include "debug.h"
#include "daqd.hh"
#include "gm_rcvr.hh"

#define MMAP_SIZE (64*1024*1024 - 5000)
#define XFER_SIZE	16384

extern daqd_c daqd;

#if 0
/*
 * Inter-processor communication structures for DAQ
 */

typedef struct {
  unsigned int status;
  unsigned int timeSec;
  unsigned int timeNSec;
  unsigned int run;
  unsigned int cycle;
  unsigned int crc; /* block data CRC checksum */
} blockPropT;

typedef struct {       /* IPC area structure                   */
  unsigned int cycle;  /* Copy of latest cycle num from blocks */
  unsigned int dcuId;          /* id of unit, unique within each site  */
  unsigned int crc;            /* Configuration file's checksum        */
  unsigned int command;        /* Allows DAQSC to command unit.        */
  unsigned int cmdAck;         /* Allows unit to acknowledge DAQS cmd. */
  unsigned int request;        /* DCU request of controller            */
  unsigned int reqAck;         /* controller acknowledge of DCU req.   */
  unsigned int status;         /* Status is set by the controller.     */
  unsigned int channelCount;   /* Number of data channels in a DCU     */
  unsigned int dataBlockSize; /* Num bytes actually written by DCU within a 1/16 data block */
  blockPropT bp [16];  /* An array of block property structures */
}rmIpcStr;

#endif

typedef struct
{
  int messages_expected;
  int callbacks_pending;
} gm_s_e_context_t;

#if 0
// Rcv message structure
typedef struct
{
  char message[8];
  unsigned int cycle;
  unsigned int dcuId;
  unsigned int fileCrc;
  unsigned int offset;
  int channelCount;
  int dataBlockSize;
  unsigned int blockCrc;
  unsigned int dataCount;
}myMessage;
#endif

// Rcv data structure
typedef struct
{
  unsigned int data[XFER_SIZE];
}myData;


// GM port we are talking to
struct gm_port *my_port;
void *in_buffer;

int nreCounter = 0;
int wfd;
// id message pointer - sends DMA memory area ptr to sender
  gm_s_e_id_message_t *id_message[DCU_COUNT];
// Node name of expected sender - needs addition for mult. senders
  char sender_nodename[64];
// Node id variables
  gm_u32_t my_node_id;
  gm_s_e_context_t context;

static void wait_for_events (struct gm_port *p,
                             gm_s_e_context_t *the_context);
static void my_send_callback (struct gm_port *port,
                              void *the_context,
                              gm_status_t the_status);
//int dcuCycles[DCU_COUNT];



void
gm_recv(void)
{
  gm_recv_event_t *event;
  void *buffer;
  unsigned int size;
  daqMessage *rcvData;
  myData *drData;
//  static volatile char * daqDataAddress[16][16];	/* pointer to data blocks in shm  */

  unsigned int *daqData;				/* Data mover variable */
  int ii;
  int dcuId;						/* node id of data sender */

for(;;) {
      
  ////printf("gm waiting event \n");
  // block, waiting on receive event ie interrupt
  event = gm_blocking_receive_no_spin (my_port);
 
  //printf("gm event\n");

  switch (GM_RECV_EVENT_TYPE(event)) {
    case GM_RECV_EVENT:
    case GM_HIGH_RECV_EVENT:
    case GM_PEER_RECV_EVENT:
    case GM_FAST_PEER_RECV_EVENT:
	rcvData = (daqMessage *)gm_ntohp(event->recv.buffer);
        if (rcvData == 0) {
	       system_log(1, "received zero pointer\n");
               break;
        }
	// Received shutdown message
	if (strcmp(rcvData->message,"END") == 0) {
	  system_log(1, "My request to STOP %d\n",nreCounter);
  	  //for(ii=0;ii<4;ii++) printf("cycle rcvd %d = %d\n",ii,dcuCycles[ii]);
	  context.messages_expected--;
	} else
	// Received startup message
	// All clients must send this message on startup
	if (strcmp(rcvData->message,"STT") == 0) {
	  int bsw = 0;
  	  gm_u32_t node_id = gm_ntoh_u16(event->recv.sender_node_id);

	  dcuId = ntohl(rcvData->dcuId);
	  if (dcuId >0 && dcuId < DCU_COUNT) bsw = 1;
	  else {
	    dcuId = rcvData->dcuId; bsw = 0;
	  }
	  system_log(1, "Recv'd init from dcuId = %d\n",dcuId);
	 if (dcuId > 0 && dcuId < DCU_COUNT) {
	  // Initialize dcu info
	  gmDaqIpc[dcuId].dcuId = dcuId;
	  gmDaqIpc[dcuId].crc = bsw? ntohl(rcvData->fileCrc):rcvData->fileCrc;
	  gmDaqIpc[dcuId].channelCount = bsw? ntohl(rcvData->channelCount): rcvData->channelCount;
	  gmDaqIpc[dcuId].dataBlockSize = bsw? ntohl(rcvData->dataBlockSize): rcvData->dataBlockSize;
	  int port = bsw? ntohl(rcvData->port): rcvData->port;
//	  for(ii=0;ii<16;ii++) 
//		daqDataAddress[dcuId][ii] = (char *)_epics_shm + ((0x2000000 + 0x100000 * (dcuId - 5))+ii*0x10000);
	  // Send back the id_message, which includes the DMA memory address info
	  system_log(1, "GM node id %d\n", node_id);
	  gm_send_with_callback (my_port,
				 id_message[dcuId],
				 GM_RCV_MESSAGE_SIZE,
				 sizeof(*id_message[0]),
				 GM_DAQ_PRIORITY,
				 node_id,
				 port /* GM_PORT_NUM_SEND */,
				 my_send_callback,
				 &context);
	  context.callbacks_pending++;
	 } else {
	  system_log(1, "Invalid DCU Id\n");
	 }
	} else 
	// Received data
  	if (strcmp(rcvData->message,"DAT") == 0) {
	        int bsw = 0;
	  	dcuId = ntohl(rcvData->dcuId);
	  	if (dcuId >0 && dcuId < DCU_COUNT) bsw = 1;
	  	else {
	    	  dcuId = rcvData->dcuId; bsw = 0;
	  	}
		//dcuCycles[dcuId] ++;
		drData = (myData *)directed_receive_buffer[dcuId];
		drData += bsw?ntohl(rcvData->cycle) : rcvData->cycle;
		// Copy pertinent info to daqIpc area
	        gmDaqIpc[dcuId].crc = bsw?ntohl(rcvData->fileCrc): rcvData->fileCrc;
	  	gmDaqIpc[dcuId].cycle = bsw?ntohl(rcvData->cycle): rcvData->cycle;
	  	gmDaqIpc[dcuId].bp[gmDaqIpc[dcuId].cycle].crc = bsw?ntohl(rcvData->blockCrc): rcvData->blockCrc;
		DEBUG(3, printf("dcu %d cycle %d crc=0x%x\n", dcuId,  gmDaqIpc[dcuId].cycle, gmDaqIpc[dcuId].bp[gmDaqIpc[dcuId].cycle].crc));
		drData = (myData *)directed_receive_buffer[dcuId];

		extern struct cdsDaqNetGdsTpNum * gdsTpNum[2][DCU_COUNT];

// FIXME: This assumes all Myrinet DUCs are on IFO 0
		if (gdsTpNum[0][dcuId] == 0) {
		//	printf("Unconfigured DCU %d\n", dcuId);
			//break; // unconfigured DCU
		} else {
		gdsTpNum[0][dcuId]->count = bsw?ntohl(rcvData->tpCount): rcvData->tpCount;
		if (gdsTpNum[0][dcuId]->count > GM_DAQ_MAX_TPS || gdsTpNum[0][dcuId]->count < 0) {
			system_log(1, "bad number of testpoints received from DCU %d\n", dcuId);
			gdsTpNum[0][dcuId]->count = GM_DAQ_MAX_TPS;
		}
		if (gdsTpNum[0][dcuId]->count > 0) {
			int i;
			system_log(5, "dcu %d tp count= %d; tpnums=%d %d %d %d\n", dcuId, bsw?ntohl(rcvData->tpCount): rcvData->tpCount, bsw? ntohl(rcvData->tpNum[0]): rcvData->tpNum[0],  bsw?ntohl(rcvData->tpNum[1]): rcvData->tpNum[1], bsw?ntohl(rcvData->tpNum[2]): rcvData->tpNum[2], bsw?ntohl(rcvData->tpNum[3]): rcvData->tpNum[3]);
			for (i = 0; i < gdsTpNum[0][dcuId]->count; i++) 
			  gdsTpNum[0][dcuId]->tpNum[i] = bsw?ntohl(rcvData->tpNum[i]): rcvData->tpNum[i];
		}

		//dcuCycles[dcuId] ++;
#if 0
		daqData = (unsigned int *)daqDataAddress[dcuId][ntohl(rcvData->cycle)];
		daqData += ntohl(rcvData->offset);
		// Move data from message buffer to shm
		for(ii=0;ii < ntohl(rcvData->dataCount);ii++) {
		  *daqData = drData->data[ii];
		  daqData ++;
		}
#endif
#ifdef _ADVANCED_LIGO
		if (daqd.controller_dcu == dcuId) {
         	   controller_cycle = gmDaqIpc[daqd.controller_dcu].cycle;
		   DEBUG(3, printf("Timing dcu=%d cycle=%d\n", dcuId, controller_cycle));
		}
#endif

		}
	} 
	else 
	// Received time
  	if (strcmp(rcvData->message,"TIM") == 0) {
	  system_log(5, "controller time\n");
	}
          /* Return the buffer for reuse */
          buffer = gm_ntohp (event->recv.buffer);
          size = (unsigned int)gm_ntoh_u8 (event->recv.size);
          gm_provide_receive_buffer (my_port, buffer, size,
                                     GM_DAQ_PRIORITY);
          break;

        case GM_NO_RECV_EVENT:
		nreCounter ++;
          break;

        default:
	  //printf("unknown event\n");
	  // This is where sent message callbacks go
          gm_unknown (my_port, event);  /* gm_unknown calls the callback */
        }
}
}

/* This function is called inside gm_unknown() when there is a callback
   ready to be processed.  It tells us that a send has completed, either
   successfully or with error. */
static void
my_send_callback (struct gm_port *port, void *the_context,
                  gm_status_t the_status)
{
  //printf("callback\n");
  /* One pending callback has been received */
  ((gm_s_e_context_t *)the_context)->callbacks_pending--;

  switch (the_status)
    {
    case GM_SUCCESS:
      break;

    case GM_SEND_DROPPED:
      system_log (1, "**** Send dropped!\n");
      break;

    default:
      gm_perror ("Send completed with error", the_status);
    }
}

int
gm_setup(void)
{
        pthread_attr_t attr;
        int ret;
        int status;
  gm_status_t main_status;
  int my_board_num = 0;         /* Default board_num is 0 */
  unsigned int my_global_id = 0;
  gm_size_t alloc_length;
  struct timespec next;
  int ii, j;

  context.messages_expected = 0;
  context.callbacks_pending = 0;

  // Initialize gm driver
  gm_init();

    gm_strncpy (sender_nodename,  /* Mandatory 1st parameter */
              "fb",
              sizeof (sender_nodename) - 1);

  // Open a port
  main_status = gm_open (&my_port, my_board_num,
                         GM_PORT_NUM_RECV,
                         "gm_simple_example_recv",
                         (enum gm_api_version) GM_API_VERSION_1_1);
  if (main_status == GM_SUCCESS)
    {
      system_log (1, "[recv] Opened board %d port %d\n",
                   my_board_num, GM_PORT_NUM_RECV);
    }
  else
    {
      gm_perror ("[recv] Couldn't open GM port", main_status);
      goto abort_with_nothing;
    }

  gm_get_node_id (my_port, &my_node_id);
  main_status = gm_node_id_to_global_id (my_port, my_node_id, &my_global_id);
  if (main_status != GM_SUCCESS)
    {
      gm_perror ("[recv] Couldn't convert node ID to global ID", main_status);
      goto abort_with_open_port; 
    }
      
  in_buffer = gm_dma_calloc (my_port, GM_RCV_BUFFER_COUNT,
                             GM_RCV_BUFFER_LENGTH);
  if (in_buffer == 0)
    {
      system_log (1, "[recv] Couldn't allocate in_buffer\n");
      main_status = GM_OUT_OF_MEMORY;
      goto abort_with_id_message;
    }

  //std::cerr << "allocating GM buffers count=" << GM_16HZ_BUFFER_COUNT << " length=" << GM_16HZ_BUFFER_LENGTH << std::endl;

for(ii=9;ii<DCU_COUNT;ii++)
{
  /* Allocate DMAable message buffers. */
  alloc_length = sizeof(*id_message[0]);
  id_message[ii] = (gm_s_e_id_message_t *)gm_dma_calloc (my_port, 1, alloc_length);
  if (id_message[ii] == 0)
    {
      system_log (1, "[recv] Couldn't allocate output buffer for id_message\n");
      main_status = GM_OUT_OF_MEMORY;
      goto abort_with_open_port;
    }

  directed_receive_buffer[ii] = gm_dma_calloc (my_port,
                                           GM_16HZ_BUFFER_COUNT,
                                           GM_16HZ_BUFFER_LENGTH);
  if (directed_receive_buffer[ii] == 0)
    {
      system_log (1, "[recv] Couldn't allocate directed_receive_buffer\n");
      main_status = GM_OUT_OF_MEMORY;
      goto abort_with_in_buffer;
    }


  /* Here we take advantage of the fact that gm_remote_ptr_t is 64 bits for
     all platforms and architectures. */
  id_message[ii]->directed_recv_buffer_addr =
    gm_hton_u64((gm_size_t)directed_receive_buffer[ii]);
  id_message[ii]->global_id = gm_hton_u32(my_global_id);
}

  /* Allow any GM process to modify any of the local DMAable buffers. */
  gm_allow_remote_memory_access (my_port);

  // Provide a receive buffer
  gm_provide_receive_buffer (my_port, in_buffer, GM_RCV_MESSAGE_SIZE,
                             GM_DAQ_PRIORITY);

  context.messages_expected = 1; /* Now, we do expect a message from senders */


  return 0;

 abort_with_in_buffer:
  gm_dma_free (my_port, in_buffer);
 abort_with_id_message:
  for(ii=0;ii<4;ii++)gm_dma_free (my_port, id_message[ii]);
 abort_with_open_port:
  gm_close (my_port);

 abort_with_nothing:
  //gm_finalize();
  //gm_exit (main_status);

  return -1;
}

#if 0
  /* Nothing more to do but wait for callbacks and the incoming messages */
  wait_for_events (my_port, &context);
#endif

void
gm_cleanup()
{
  int ii;
  gm_status_t main_status;

  for(ii=0;ii<4;ii++) gm_dma_free (my_port, directed_receive_buffer[ii]);
  main_status = GM_SUCCESS;

 abort_with_in_buffer:
  gm_dma_free (my_port, in_buffer);
 abort_with_id_message:
  for(ii=0;ii<4;ii++)gm_dma_free (my_port, id_message[ii]);
 abort_with_open_port:
  gm_close (my_port);

 abort_with_nothing:
  gm_finalize();
  gm_exit (main_status);
}

