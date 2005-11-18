/* Library file for using Myricom network on CDS front ends */

#include "gm.h"

#define GM_PORT_NUM_RECV 4
#define GM_PORT_NUM_SEND 2

#define GM_DAQ_PRIORITY GM_HIGH_PRIORITY

#define GM_RCV_MESSAGE_SIZE	 9
#define GM_16HZ_SIZE	22 
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

#define GM_DAQ_XFER_SIZE        2048
#define GM_DAQ_BLOCK_SIZE	GM_DAQ_XFER_SIZE * 64
#define GM_DAQ_XFER_BYTE        GM_DAQ_XFER_SIZE * 4

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
  int tpCount;
  int tpNum[32];
}daqMessage;

/* Definition of data sent to Framebuilder */
typedef struct
{
  unsigned int data[GM_DAQ_XFER_SIZE];
}daqData;
