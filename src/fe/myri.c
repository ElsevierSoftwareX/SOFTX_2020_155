#include <linux/types.h>
#include <linux/pci.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#if !defined(NO_DAQ) && defined(USE_GM)
#include <drv/gmnet.h>
#include <drv/cdsHardware.h>
#include <drv/myri.h>
#include <drv/fb.h>

// Myrinet Variables
static void *netOutBuffer;			/* Buffer for outbound myrinet messages */
static void *netInBuffer;			/* Buffer for incoming myrinet messages */
static void *netDmaBuffer;			/* Buffer for outbound myrinet DMA messages */
gm_remote_ptr_t directed_send_addr[2];		/* Pointer to FB memory block */
gm_remote_ptr_t directed_send_subaddr[2][16];	/* Pointers to FB 1/16 sec memory blocks */
gm_recv_event_t *event;
gm_remote_ptr_t test_send_addr;		/* Pointer to FB memory block */

int my_board_num = 0;                 		/* Default board_num is 0 */
gm_u32_t receiver_node_id[4];
gm_u32_t receiver_global_id;
gm_status_t main_status;
void *recv_buffer;
unsigned int size;
gm_s_e_id_message_t *id_message;
gm_u32_t recv_length;
static int expected_callbacks[2] = {0,0};
static struct gm_port *netPort = 0;
daqMessage *daqSendMessage;
daqData *daqSendData;

/* Local GM port we are using to send DAQ data */
int local_gm_port = 0;

int fbStat[2] = {0,0};
enum FB_STATS {
	FB_NOT_ON_NET = 0,
	FB_ON_NET,
	FB_CONNECTED,
	FB_ONLINE,
	FB_NEEDS_RECON,
	FB_INIT,
	FB_NOOP1,
	FB_NOOP2,
	FB_PENDING
};

// *****************************************************************
// Called by gm_unknown().
// Decriments expected_callbacks variable ((*(int *)the_context)--),
// which is a network health monitor for the FE software.
// *****************************************************************
static void
my_send_callback (struct gm_port *port, void *the_context,
                  gm_status_t the_status)
{         
  /* One pending callback has been received */
  (*(int *)the_context)--;
          
  switch (the_status)
    {                                
    case GM_SUCCESS:
       // printk ("**** SUCC \n");

      break;
        
    case GM_SEND_DROPPED:
       // cdsNetStatus = (int)the_status;
       printk ("**** Send dropped in callaback!\n");
      break;
          
    default:
	cdsNetStatus = (int)the_status;
  	gm_drop_sends 	(netPort,
                         GM_DAQ_PRIORITY,
                         receiver_node_id[0],
                         GM_PORT_NUM_RECV,
                         my_send_callback,
                         &expected_callbacks[0]);
       printk ("**** Send error! %d\n",the_status);
       gm_perror ("Send completed with error", the_status);
    }
}


int myriNetDrop()
{
gm_drop_sends   (netPort,
                         GM_DAQ_PRIORITY,
                         receiver_node_id[0],
                         GM_PORT_NUM_RECV,
                         my_send_callback,
                         &expected_callbacks[0]);
return(0);
}


// *****************************************************************
// Initialize the myrinet connection to Framebuilder.
int myriNetInit(int fbId)
{
  int i;
  char receiver_nodename[4][64];
  int status;

  // Initialize interface
  gm_init();

  gm_strncpy (receiver_nodename[0], "fb", sizeof (receiver_nodename[0]) - 1);
  gm_strncpy (receiver_nodename[1], "fb1", sizeof (receiver_nodename[1]) - 1);

  /* Try upto four GM ports */
  for (i = 0; i < 4; i++) {
    main_status = gm_open (&netPort, my_board_num,
                         (local_gm_port = GM_PORT_NUM_SEND + i),
                         "gm_simple_example_send",
                         (enum gm_api_version) GM_API_VERSION_1_1);
    if (main_status == GM_SUCCESS) {
      printk ("[send] Opened board %d port %d\n", my_board_num, GM_PORT_NUM_SEND);
      break;
    } else {
      ;
    }
  }
  if (main_status != GM_SUCCESS) {
      gm_perror ("[send] Couldn't open GM port", main_status);
      return 0;
  }

  /* Allocate DMAable message buffers. */

  netOutBuffer = gm_dma_calloc (netPort, GM_RCV_BUFFER_COUNT,
                              GM_RCV_BUFFER_LENGTH);
  if (netOutBuffer == 0)
    {
      printk ("[send] Couldn't allocate out_buffer\n");
      main_status = GM_OUT_OF_MEMORY;
    }

  netInBuffer = gm_dma_calloc (netPort, GM_RCV_BUFFER_COUNT,
                             GM_RCV_BUFFER_LENGTH);
  if (netInBuffer == 0)
    {
      printk ("[send] Couldn't allocate netInBuffer\n");
      main_status = GM_OUT_OF_MEMORY;
    }

  netDmaBuffer = gm_dma_calloc (netPort,
                                        GM_16HZ_BUFFER_COUNT,
                                        GM_16HZ_SEND_BUFFER_LENGTH);
  if (netDmaBuffer == 0)
    {
      printk ("[send] Couldn't allocate directed_send_buffer\n");
      main_status = GM_OUT_OF_MEMORY;
    }

  /* Tell GM where our receive buffer is */

  gm_provide_receive_buffer (netPort, netInBuffer, GM_RCV_MESSAGE_SIZE,
                             GM_DAQ_PRIORITY);

  status = 0;
  for (i = 0; i < 2; i++) {
	  fbStat[i] = FB_NOT_ON_NET;
	  main_status = gm_host_name_to_node_id_ex
	    (netPort, 1000000, receiver_nodename[i], &receiver_node_id[i]);
	  if (main_status == GM_SUCCESS)
	    {
		status |= (1 << i);
		fbStat[i] = FB_INIT;
	      	printk ("receiver node ID is %d %d\n", receiver_node_id[i],fbStat[i]);
	    }
	  else
	    {
	      printk ("[send] Conversion of nodename %s to node id failed\n",
			 receiver_nodename[i]);
	      gm_perror ("[send]", main_status);
		fbStat[i] = 0;
	    }
    }
    return(status);
}

// *****************************************************************
// Cleanup the myrinet connection when code is killed.
// *****************************************************************
int myriNetClose()
{
  main_status = GM_SUCCESS;

  gm_dma_free (netPort, netOutBuffer);
  gm_dma_free (netPort, netInBuffer);
  gm_dma_free (netPort, netDmaBuffer);
  gm_close (netPort);
  gm_finalize();

  //gm_exit (main_status);

  return(0);

}

// *****************************************************************
// Network health check.
// If expected_callbacks get too high, FE will know to take
// corrective action.
// *****************************************************************
int myriNetCheckCallback()
{
int ii,jj;
int status;
int rnid;

      event = gm_receive (netPort);

      switch (GM_RECV_EVENT_TYPE(event))
	{
	case GM_RECV_EVENT:
	case GM_PEER_RECV_EVENT:
	case GM_FAST_PEER_RECV_EVENT:

	  recv_buffer = gm_ntohp (event->recv.buffer);
	  size = (unsigned int)gm_ntoh_u8 (event->recv.size);
	  gm_provide_receive_buffer (netPort, recv_buffer, size,
				     GM_DAQ_PRIORITY);
          id_message = gm_ntohp (event->recv.message);
          receiver_global_id = gm_ntoh_u32(id_message->global_id);
          main_status = gm_global_id_to_node_id(netPort,
                                                receiver_global_id,
                                                &rnid);
          if (main_status != GM_SUCCESS)
            {
              gm_perror ("[send] Couldn't convert global ID to node ID",
                         main_status);
		break;
            }
	  jj=0;
	  for(ii=0;ii<2;ii++)
	  {
		if(receiver_node_id[ii] == rnid) 
		{
			jj=ii;
			fbStat[ii] = FB_ONLINE;
		}
	  }
          directed_send_addr[jj] =
            gm_ntoh_u64(id_message->directed_recv_buffer_addr);
          for(ii=0;ii<16;ii++) directed_send_subaddr[jj][ii] = directed_send_addr[jj] + GM_DAQ_XFER_BYTE * ii;

		case GM_NO_RECV_EVENT:
		  break;

		default:
		  gm_unknown (netPort, event);  /* gm_unknown calls the callback */
		}
  status = (expected_callbacks[1] << 8) + expected_callbacks[0];
  return(status);

}

// *****************************************************************
// Send a connection message to the Framebuilder requesting its
// DMA memory location for the writing of DAQ data.
// *****************************************************************
int myriNetReconnect(int dcuId)
{
  unsigned long send_length;
  int ii,status;
  
  daqSendMessage = (daqMessage *)netOutBuffer;
  sprintf (daqSendMessage->message, "STT");
  daqSendMessage->dcuId = htonl(dcuId);
  daqSendMessage->port = htonl(local_gm_port);
  daqSendMessage->channelCount = htonl(16);
  daqSendMessage->fileCrc = htonl(0x3879d7b);
  daqSendMessage->dataBlockSize = htonl(GM_DAQ_XFER_BYTE);

  send_length = (unsigned long) sizeof(*daqSendMessage);
  status = 0;
  for(ii=0;ii<2;ii++)
  {
	if(fbStat[ii] & FB_NEEDS_RECON)
	{
	  gm_send_with_callback (netPort,
                         netOutBuffer,
                         GM_RCV_MESSAGE_SIZE,
                         send_length,
                         GM_DAQ_PRIORITY,
                         receiver_node_id[ii],
                         GM_PORT_NUM_RECV,
                         my_send_callback,
                         &expected_callbacks[ii]);
	  expected_callbacks[ii] ++;
	  fbStat[ii] &= ~(FB_NEEDS_RECON);
	  status ++;
	}
  }
  return(status);
}

// *****************************************************************
// Check if the connection request message has been received.
// Returns (0) if success, (-1) if fail.
// *****************************************************************
int myriNetCheckReconnect()
{

int eMessage;

  eMessage = -1;
  int ii,jj;
  int rnid;

      jj = 1;
      event = gm_receive (netPort);
      recv_length = gm_ntoh_u32 (event->recv.length);
// printk("rcv leng in calback = %d\n",recv_length);

      switch (GM_RECV_EVENT_TYPE(event))
        {
        case GM_RECV_EVENT:
        case GM_PEER_RECV_EVENT:
        case GM_FAST_PEER_RECV_EVENT:
          if (recv_length != sizeof (gm_s_e_id_message_t))
            {
              printk ("[send] *** ERROR: incoming message length %d "
                         "incorrect; should be %ld\n",
                         recv_length, sizeof (gm_s_e_id_message_t));
              main_status = GM_FAILURE; /* Unexpected incoming message */
            }


          id_message = gm_ntohp (event->recv.message);
          receiver_global_id = gm_ntoh_u32(id_message->global_id);
          main_status = gm_global_id_to_node_id(netPort,
                                                receiver_global_id,
                                                &rnid);
          if (main_status != GM_SUCCESS)
            {
              gm_perror ("[send] Couldn't convert global ID to node ID",
                         main_status);
		return(0);
            }
	  for(ii=0;ii<2;ii++)
	  {
		if(receiver_node_id[ii] == rnid) 
		{
			jj=ii;
			fbStat[ii] = FB_ONLINE;
		}
	  }
printk("rcvd reply from fb %d %d %d %d\n",rnid,jj,fbStat[0],fbStat[1]);
          directed_send_addr[jj] =
            gm_ntoh_u64(id_message->directed_recv_buffer_addr);
printk("rcvd reply from fb  %d 0x%x\n",jj,directed_send_addr[jj]);
          for(ii=0;ii<16;ii++) directed_send_subaddr[jj][ii] = directed_send_addr[jj] + GM_DAQ_XFER_BYTE * ii;

          eMessage = 0;

          /* Return the buffer for reuse */

          recv_buffer = gm_ntohp (event->recv.buffer);
          size = (unsigned int)gm_ntoh_u8 (event->recv.size);
          gm_provide_receive_buffer (netPort, recv_buffer, size,
                                     GM_DAQ_PRIORITY);
          break;

        case GM_NO_RECV_EVENT:
          break;

        default:
          gm_unknown (netPort, event);  /* gm_unknown calls the callback */
        }
  eMessage = 0;
  if(fbStat[0] == FB_ONLINE) eMessage ++;
  if(fbStat[1] == FB_ONLINE) eMessage +=2;
  return(eMessage);
}


// *****************************************************************
// Send a 1/256 sec block of data from FE to DAQ Framebuilder.
// *****************************************************************
int myriNetDaqSend(	int dcuId, 
			int cycle, 
			int subCycle, 
			unsigned int fileCrc, 
			unsigned int blockCrc,
			int crcSize,
			int tpCount,
			int tpNum[],
			int xferSize,
			char *dataBuffer)
{

  unsigned int *daqDataBuffer;
  int send_length;
  int ii,kk;
  int xferWords;

  // Load data into xmit buffer from daqLib buffer.
  daqSendData = (daqData *)netDmaBuffer;
  daqDataBuffer = (unsigned int *)dataBuffer;
  xferWords = xferSize / 4;
  daqDataBuffer += subCycle * xferWords;
  for(kk=0;kk<xferWords;kk++) 
  {
	daqSendData->data[kk] = *daqDataBuffer;
	daqDataBuffer ++;
  }
  for(ii=0;ii<2;ii++)
  {
      if(fbStat[ii] == FB_ONLINE)
      {
// printk("Sending data to %d %d\n",ii,fbStat[ii]);
	  directed_send_subaddr[ii][0] = directed_send_addr[ii] + xferSize * subCycle;

	  // Send data directly to designated memory space in Framebuilder.
	  gm_directed_send_with_callback (netPort,
                                  netDmaBuffer,
                                  (gm_remote_ptr_t) (directed_send_subaddr[ii][0] + GM_DAQ_BLOCK_SIZE * cycle),
                                  (unsigned long)
                                  xferSize,
                                  GM_DAQ_PRIORITY,
                                  receiver_node_id[ii],
                                  GM_PORT_NUM_RECV,
                                  my_send_callback,
                                  &expected_callbacks[ii]);
	  expected_callbacks[ii]++;
      }
  }


// Once every 1/16 second, send a message to signal FB that data is ready.
if(subCycle == 15) {
  sprintf (daqSendMessage->message, "DAT");
  daqSendMessage->dcuId = htonl(dcuId);
  if(cycle > 0)  {
    int mycycle = cycle -1;
    daqSendMessage->cycle = htonl(mycycle);
  }
  if(cycle == 0) daqSendMessage->cycle = htonl(15);
  daqSendMessage->offset = htonl(subCycle);
  daqSendMessage->fileCrc = htonl(fileCrc);
  daqSendMessage->blockCrc = htonl(blockCrc);
  daqSendMessage->dataCount = htonl(crcSize);
  if (GM_DAQ_MAX_TPS < tpCount) tpCount = GM_DAQ_MAX_TPS;
  daqSendMessage->tpCount = htonl(tpCount);
  for(kk=0;kk<GM_DAQ_MAX_TPS;kk++) daqSendMessage->tpNum[kk] = htonl(tpNum[kk]);
  send_length = (unsigned long) sizeof(*daqSendMessage);
  for(ii=0;ii<2;ii++)
  {
      if(fbStat[ii] == FB_ONLINE)
      {
	  gm_send_with_callback (netPort,
                         netOutBuffer,
                         GM_RCV_MESSAGE_SIZE,
                         send_length,
                         GM_DAQ_PRIORITY,
                         receiver_node_id[ii],
                         GM_PORT_NUM_RECV,
                         my_send_callback,
                         &expected_callbacks[ii]);
	  expected_callbacks[ii]++;
      }
  }
  }
  return(0);
}
#endif
