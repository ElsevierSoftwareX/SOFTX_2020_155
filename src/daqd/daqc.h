/*
  Data acquisition daemon access
*/

#ifndef DAQC_H
#define DAQC_H

#include "channel.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <limits.h>

#define DAQD_PROTOCOL_VERSION 12
#define DAQD_PROTOCOL_REVISION 1

#define DAQD_PORT 8088

/* response codes */

#define DAQD_OK 0x0000
#define S_DAQD_OK ((char *)"0000")

#define DAQD_ERROR 0x0001
#define S_DAQD_ERROR "0001"
#define DAQD_NOT_CONFIGURED 0x0002
#define S_DAQD_NOT_CONFIGURED "0002"
#define DAQD_INVALID_IP_ADDRESS 0x0003
#define S_DAQD_INVALID_IP_ADDRESS "0003"
#define DAQD_INVALID_CHANNEL_NAME 0x0004
#define S_DAQD_INVALID_CHANNEL_NAME "0004"
#define DAQD_SOCKET 0x0005
#define S_DAQD_SOCKET "0005"
#define DAQD_SETSOCKOPT 0x0006
#define S_DAQD_SETSOCKOPT "0006"
#define DAQD_CONNECT 0x0007
#define S_DAQD_CONNECT "0007"
#define DAQD_BUSY 0x0008
#define S_DAQD_BUSY "0008"
#define DAQD_MALLOC 0x0009
#define S_DAQD_MALLOC "0009"
#define DAQD_WRITE 0x000a
#define S_DAQD_WRITE "000a"
#define DAQD_VERSION_MISMATCH 0x000b
#define S_DAQD_VERSION_MISMATCH "000b"
#define DAQD_NO_SUCH_NET_WRITER 0x000c
#define S_DAQD_NO_SUCH_NET_WRITER "000c"
#define DAQD_NOT_FOUND 0x000d
#define S_DAQD_NOT_FOUND "000d"
#define DAQD_GETPEERNAME 0x000e
#define S_DAQD_GETPEERNAME "000e"
#define DAQD_DUP 0x000f
#define S_DAQD_DUP "000f"
#define DAQD_INVALID_CHANNEL_DATA_RATE 0x0010
#define S_DAQD_INVALID_CHANNEL_DATA_RATE "0010"
#define DAQD_SHUTDOWN 0x0011
#define S_DAQD_SHUTDOWN "0011"
#define DAQD_NO_TRENDER 0x0012
#define S_DAQD_NO_TRENDER "0012"
#define DAQD_NO_MAIN 0x0013
#define S_DAQD_NO_MAIN "0013"
#define DAQD_NO_OFFLINE 0x0014
#define S_DAQD_NO_OFFLINE "0014"
#define DAQD_THREAD_CREATE 0x0015
#define S_DAQD_THREAD_CREATE "0015"
#define DAQD_TOO_MANY_CHANNELS 0x0016
#define S_DAQD_TOO_MANY_CHANNELS "0016"

/* IMPORTANT: leave NOT_SUPORTED msg the last */
#define DAQD_NOT_SUPPORTED 0x0017
#define S_DAQD_NOT_SUPPORTED "0017"

/* pieces of the communication protocol */

#define DAQD_LENGTH_UNKNOWN 0
#define HEADER_LEN 16

/* data block structure */
typedef struct {
  time_t secs;   /* data spans this many seconds */

  /* Timestamp applies to the samples in this block */
  time_t gps;    /* seconds */
  time_t gpsn;   /* nanoseconds residual */

  /* Block sequence number; shows if any blocks were dropped */
  unsigned long seq_num;

  char data [1]; /* samples; more than 1 byte usually */
} daq_block_t;

typedef struct {
  float signal_gain;
  float signal_slope;
  float signal_offset;
  char signal_units [MAX_ENGR_UNIT_LENGTH]; /* Engineering units  */  
} signal_conv_t;


typedef struct {
  float signal_slope;
  float signal_offset;
  unsigned int   signal_status;
} signal_conv_t1;

/* client access structure */
typedef struct {
  int sockfd; /* DAQD server socket */
  struct sockaddr_in srvr_addr; /* DAQD server address */
  int datafd; /* data connection socket */
  struct sockaddr_in listener_addr; /* listener address */
  void * (*interpreter)(void *); /* processing thread */

#ifndef VXWORKS
  pthread_t listener_tid; /* Network listener thread ID */
  pthread_t interpreter_tid;
  pthread_mutex_t lock; /* Used to synchronize main thread with the listener thread initialization */
#endif

  int shutting_down;

  int blocks; /* zero for on-line transmission; positive for off-line */

  daq_block_t *tb; /* transmission block; dynamically allocated and reallocated as needed; freed in daq_recv_shutdown() */
  int tb_size; /* size of the above malloced data */  
  signal_conv_t1 *s; /* signal conversion data; dynamically allocated and reallocated as needed; freed in daq_recv_shutdown() */
  int s_size; /* size of the above malloced data in elements (sizeof(signal_conv_t1) each) */

  int rev; /* server protocol revision received by 'revision' command */
} daq_t;

/* channel description structure */

typedef struct {
  char name [MAX_LONG_CHANNEL_NAME_LENGTH + 1]; /* Channel name */
  int rate;  /* Sampling rate */
  int tpnum; /* Test point number; 0 for normal channels */
  int group_num; /* Channel group number */
  int bps; /* Bytes per sample */
  int chNum; /* test point number */
  daq_data_t data_type; /* Sample data type */
  signal_conv_t s;
} daq_channel_t;

/* Channel group structure */

typedef struct {
  int group_num;
  char name [MAX_CHANNEL_NAME_LENGTH + 1]; /* Channel group name */
} daq_channel_group_t;

/* public functions' prototypes */
daq_t *daq_initialize (daq_t *, int*, void * (*)(void *));
int daq_connect (daq_t *, char *, int);
int daq_disconnect (daq_t *);
int daq_send (daq_t *, char *);
int daq_shutdown (daq_t *);
int daq_recv_block_num (daq_t *);
int daq_recv_block (daq_t *);
int daq_recv_shutdown (daq_t *);
int daq_recv_channels (daq_t *daq, daq_channel_t *, int, int *);
int daq_recv_channel_groups (daq_t *daq, daq_channel_group_t *, int, int *);
unsigned long daq_recv_id (daq_t *daq);
#endif
