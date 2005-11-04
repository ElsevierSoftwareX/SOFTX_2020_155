/* Library file for using Myricom network on CDS front ends */

#include "gm.h"

#define GM_PORT_NUM_RECV 4
#define GM_PORT_NUM_SEND 2

#define GM_DAQ_PRIORITY GM_LOW_PRIORITY

#define GM_RCV_MESSAGE_SIZE	9 
#define GM_16HZ_SIZE	21 
#define GM_16HZ_SEND_SIZE	17 
#define GM_RCV_BUFFER_COUNT 1
#define GM_16HZ_BUFFER_COUNT 1
#define GM_RCV_BUFFER_LENGTH \
 (gm_max_length_for_size(GM_RCV_MESSAGE_SIZE))
#define GM_16HZ_BUFFER_LENGTH \
 (gm_max_length_for_size(GM_16HZ_SIZE))
#define GM_16HZ_SEND_BUFFER_LENGTH \
 (gm_max_length_for_size(GM_16HZ_SEND_SIZE))


typedef struct				/* Receiver-to-sender ID message */
{
  gm_u64_n_t directed_recv_buffer_addr;	/* UVA of directed-receive buffer */
  gm_u32_n_t global_id;			/* Receiver's GM global ID */
  gm_u32_n_t slack;			/* Make length a multiple of 64 */
} gm_s_e_id_message_t;

static void *netOutBuffer;
static void *netInBuffer;
static void *netDmaBuffer;
gm_remote_ptr_t directed_send_addr;
gm_remote_ptr_t directed_send_subaddr[16];
gm_recv_event_t *event;

int my_board_num = 0;                 /* Default board_num is 0 */
gm_u32_t receiver_node_id;
gm_u32_t receiver_global_id;
gm_status_t main_status;
void *recv_buffer;
unsigned int size;
gm_s_e_id_message_t *id_message;
gm_u32_t recv_length;
static int expected_callbacks = 0;
static struct gm_port *netPort = 0;

#define GM_DAQ_XFER_SIZE       1024
#define GM_DAQ_XFER_BYTE       GM_DAQ_XFER_SIZE * 4

/* Definition of message struct sent to Framebuilder each 16Hz cycle */
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
}daqMessage;

/* Definition of data sent to Framebuilder */
typedef struct
{
  unsigned int data[GM_DAQ_XFER_SIZE];
}daqData;

daqMessage *daqSendMessage;
daqData *daqSendData;

