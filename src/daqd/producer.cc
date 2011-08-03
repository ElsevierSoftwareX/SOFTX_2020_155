
#ifdef __linux__
#define _XOPEN_SOURCE 600
#define _BSD_SOURCE 1
#endif
#include <config.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <sys/time.h>
#include <signal.h>
#ifdef sun
#include <sys/processor.h>
#include <sys/procset.h>
#endif
#include <sys/mman.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <limits.h>
#ifdef sun
#include <sys/priocntl.h>
#include <sys/rtpriocntl.h>
#include <sys/lwp.h>
#endif

#ifdef __linux__
//ssize_t pread(int fd, void *buf, size_t count, off_t offset);
#endif

#ifdef __linux__
#include <sys/timeb.h>
#endif

#include <string.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cctype>      // old <ctype.h>

#ifdef USE_SYMMETRICOM
#ifndef USE_IOP
//#include <bcuser.h>
#include <../drv/symmetricom/symmetricom.h>
#endif
#endif


#ifdef USE_FRAMECPP
#include "framecpp/frame.hh"
#include "framecpp/framereader.hh"
#include "framecpp/adcdata.hh"
#endif

using namespace std;

#include "circ.hh"
#include "daqd.hh"
#include "sing_list.hh"
#include "drv/cdsHardware.h"
#include "gm_rcvr.hh"
#include <netdb.h>
#include "net_writer.hh"
#include "drv/cdsHardware.h"

#include <sys/ioctl.h>
#include "../drv/rfm.c"

extern daqd_c daqd;
extern int shutdown_server ();
extern unsigned int crctab[256];

#if __GNUC__ >= 3
extern long int altzone;
#endif

struct ToLower { char operator() (char c) const  { return std::tolower(c); }};

/* GM and shared memory communication area */
#if defined(USE_GM) || defined(USE_MX) || defined(USE_UDP)
struct rmIpcStr gmDaqIpc[DCU_COUNT];
// DMA memory area pointers
void *directed_receive_buffer[DCU_COUNT];
int controller_cycle = 0;
#elif defined(USE_BROADCAST)
int controller_cycle = 0;
#else
#if defined(_ADVANCED_LIGO)
#ifdef USE_SYMMETRICOM
int controller_cycle = 0;
#else
// Point into shared memory for the controller DCU cycle
#define controller_cycle (shmemDaqIpc[daqd.controller_dcu]->cycle)
#endif
#endif
#endif

#if defined(VMICRFM_PRODUCER)

#define PRIVATE_BUFFERS DAQ_NUM_DATA_BLOCKS

/* Producer data read offsets */
static long read_from [DCU_COUNT][PRIVATE_BUFFERS];

/* Inter-thred communication */
static int currentBlock;

void
producer::grabIfoDataThread() {
  daqd_c::realtime("Producer's helper", 1);
  unsigned char *ifo2DataOffset = daqd.move_buf;

  // Calculate offset to the second IFO data
  for (int j = DCU_ID_EDCU; j < DCU_COUNT; j++)  ifo2DataOffset += daqd.dcuSize[0][j];

  for (;;) {
#if 0
    for (int res = sem_wait(&go); res;) {
	  system_log(1, "sem_wait_go errno=%d", res);
    }
#else
    pthread_mutex_lock(&prod_mutex);
    while (prod_go == 0) 
	pthread_cond_wait(&prod_cond_go, &prod_mutex);
    pthread_mutex_unlock(&prod_mutex);
#endif
    grabIfoData(1, currentBlock, ifo2DataOffset);
#if 0
    sem_post(&done);
#else
    pthread_mutex_lock(&prod_mutex);
    prod_done = 1;
    prod_go = 0;
    pthread_mutex_unlock(&prod_mutex);
    pthread_cond_signal(&prod_cond_done);
#endif
  }
}

void *
producer::grabIfoData(int ifo, int cblk, unsigned char *read_dest) {
  unsigned long crc = 0;
#if defined(_ADVANCED_LIGO)
  int dcu_cycle = 0;
#endif

  for (int j = DCU_ID_EDCU; j < DCU_COUNT; j++) {
    if (daqd.dcuSize[ifo][j]) {
      long read_size = daqd.dcuSize[ifo][j];
#if EPICS_EDCU
      if (IS_EPICS_DCU(j)) {
#if 0
	/* Caveat: Make sure that the memcpy() here does not corrupt the data.
		So, there is no race condition between this thread and the EDCU callback function
	   If everything looks normal, get rid of edcu.channel_value[] and make EDCU value callback
		put new values directly into the move_buf */
	for (int k = daqd.edcu1.fidx; k < daqd.edcu1.num_chans; k++) {
	  float v = daqd.edcu1.channel_value[k];
	}
#endif
	memcpy(read_dest, (char *)(daqd.edcu1.channel_value + daqd.edcu1.fidx), daqd.dcuSize[ifo][j]);
	if (daqd.edcu1.num_chans) {
	  assert(daqd.edcu1.num_chans*4 == daqd.dcuSize[ifo][j]);
 	} 
	read_dest += read_size;
	continue; // Move ot the next DCU in the loop
      }
#endif

      //      cerr << "pread (daqd.rh -> rh_fd, " << hex << (int)read_dest << ", " << read_size << ", " << read_from[j][cblk] << ")" << endl;

#ifdef FILE_CHANNEL_CONFIG
#ifdef AUX_5579
      if (j == DCU_ID_TP_16K || j == DCU_ID_TP_2K) {
	/* Read one half of the DCU data from one RFM net and
	   second half from the other */
	read_size /= 2;

#if defined(DATA_CONCENTRATOR) && 0
	for (int k = 0; k < 2; k++) {
          volatile unsigned char * read_from_base = k? (ifo ? daqd.rm_aux_ptr1: daqd.rm_aux_ptr): (ifo? daqd.rm_mem_ptr1: daqd.rm_mem_ptr);
	  read_from_base += read_from[j][cblk];
	  memcpy((char *)read_dest + k*read_size, (char *)read_from_base, read_size);
	  //memset((char *)read_dest + k*read_size, htonl(j), read_size);

#if 0
	char *p = ((char *)read_from_base);
	char *r = (char *)read_dest + k*read_size;
	int s = read_size/4;
	for (int l = 0; l < s; l++) {
	  ((int *)r)[l] = ((int *)p)[l];
	}
#endif

	}
#else

	for (int k = 0; k < 2; k++) {
	  for (;;) {
	    int nread;
#ifdef __linux__
	    int fd = k? (ifo ? daqd.rh5579_1 -> fd: daqd.rh5579 -> fd): (ifo? daqd.rh1 -> fd: daqd.rh -> fd);
#else
	    int fd = k? (ifo ? daqd.rh5579_1 -> rh_fd: daqd.rh5579 -> rh_fd): (ifo? daqd.rh1 -> rh_fd: daqd.rh -> rh_fd);
#endif
	    if ((nread = pread (fd, read_dest + k*read_size, read_size, read_from[j][cblk])) < 0)
	      {
		if (errno == EINTR) {
		  system_log(1, "pread() errno %d (EINTR); k=%d", errno, k);
		  continue;
		} else {
		  system_log(1, "pread() errno %d; k=%d", errno, k);
		  break;
		}
	      } else {
		if (nread != read_size) {
		  system_log(1, "read() %d bytes; k=%d", nread, k);
		}
		break;
	      }
	  }
//	  if (k == 1) memset(read_dest + k*read_size, 0xff, read_size);
	}
#endif

	read_dest += 2*read_size;
        continue; // Move ot the next DCU in the loop
      } else
#endif
#if defined(USE_GM) || defined(USE_MX) || defined(USE_UDP)
      if (IS_MYRINET_DCU(j)) {
#if defined(COMPAT_INITIAL_LIGO)
	//dcu_cycle = gmDaqIpc[j].cycle - 1;
	//if (dcu_cycle < 0) dcu_cycle = 15;
	dcu_cycle = cblk;
#else
// Do not use the IPC cycle as doing so causes data misses
#if 0
	if (daqd.cit_40m) dcu_cycle = cblk;
   	else dcu_cycle = gmDaqIpc[j].cycle;
#endif
	dcu_cycle = cblk;
#endif

	memcpy((void *)read_dest,
	       ((char *)directed_receive_buffer[j]) + dcu_cycle*2*DAQ_DCU_BLOCK_SIZE,
	       read_size);
#if 0

	char *p = ((char *)directed_receive_buffer[j]) + dcu_cycle*2*DAQ_DCU_BLOCK_SIZE;
	int s = read_size/4;
	for (int k = 0; k < s; k++) {
	  ((int *)read_dest)[k] = ((int *)p)[k];
	}

#endif

      }
     else 
#endif

     {


#if  defined(DATA_CONCENTRATOR) && 0
	volatile unsigned char * read_from_base = ((daqd.cit_40m? dcuNet40m: dcuNet)[j]&1)? (ifo? daqd.rm_mem_ptr1: daqd.rm_mem_ptr) : (ifo ? daqd.rm_aux_ptr1: daqd.rm_aux_ptr);
	read_from_base += read_from[j][cblk];

	memcpy((void *)read_dest, (char *)read_from_base, read_size);
#else
	for (;;) {
	  int nread;
#ifndef AUX_5579
#ifdef __linux__
	  int fd = ifo? daqd.rh1 -> fd: daqd.rh -> fd;
#else
	  int fd = ifo? daqd.rh1 -> rh_fd: daqd.rh -> rh_fd;
#endif
#else
#ifdef __linux__
	  int fd = ((daqd.cit_40m? dcuNet40m: dcuNet)[j]&1)? (ifo? daqd.rh1 -> fd: daqd.rh -> fd) : (ifo ? daqd.rh5579_1 -> fd: daqd.rh5579 -> fd);
#else
	  int fd = ((daqd.cit_40m? dcuNet40m: dcuNet)[j]&1)? (ifo? daqd.rh1 -> rh_fd: daqd.rh -> rh_fd) : (ifo ? daqd.rh5579_1 -> rh_fd: daqd.rh5579 -> rh_fd);
#endif
#endif

	  if ((nread = pread (fd, read_dest, read_size, read_from[j][cblk])) < 0) 
	    {
	      if (errno == EINTR) {
		system_log(1, "pread() errno %d (EINTR)", errno);
		continue;
	      } else {
		system_log(1, "pread() errno %d", errno);
		break;
	      }
	    } else {
	      if (nread != read_size) {
		system_log(1, "read() %d bytes", nread);
	      }
	      break;
	    }
	}
#endif
      }

      volatile struct rmIpcStr *ipc = daqd.dcuIpc[ifo][j];

      int cblk1 = (cblk + 1)%PRIVATE_BUFFERS;
      // Calculate DCU status, if needed
      // Assign this status into the RFM
#if defined(USE_GM) || defined(USE_MX) || defined(USE_UDP)
      if (IS_MYRINET_DCU(j)) {
        ipc = &gmDaqIpc[j];
      }
#endif
      if (daqd.dcu_status_check & (1 << ifo)) {
	if (cblk1 % 16 == 0) {
	  /* DCU checking mask (Which DCUs to check for SYNC fault) */
	  unsigned int dcm = 0xfffffff0;

	  int lastStatus = dcuStatus[ifo][j];
	  dcuStatus[ifo][j] = DAQ_STATE_FAULT;

          /* Check if DCU is in sync */
          if (dcm & (1 << j))
      	    if (dcuCycleStatus[ifo][j] < 4)
              dcuStatus[ifo][j] = DAQ_STATE_SYNC_ERR;

	  /* Check if DCU running at all */
	  if (dcuStatCycle[ifo][j] == 0) dcuStatus[ifo][j] = DAQ_STATE_SYNC_ERR;

	  /* Check if DCU running and in sync */
	  if ((dcuCycleStatus[ifo][j] > 3 || j < 5) && dcuStatCycle[ifo][j] > 4)
 	    dcuStatus[ifo][j] = DAQ_STATE_RUN;

	  if (/* (lastStatus == DAQ_STATE_RUN) && */ (dcuStatus[ifo][j] != DAQ_STATE_RUN)) {
	    //	  if (debug > 0)
	    //	    printf("Lost %s (ifo %d; dcu %d); status %d %d\n", dcuName[j], ifo, j, dcuCycleStatus[ifo][j], dcuStatCycle[ifo][j]);
	    ipc->status = DAQ_STATE_FAULT;
	  }

	  if ((dcuStatus[ifo][j] == DAQ_STATE_RUN) /* && (lastStatus != DAQ_STATE_RUN) */) {
	    //	  if (debug > 0)
	    //	    printf("New %s (dcu %d)\n", dcuName[i], i);
	    ipc->status = DAQ_STATE_RUN;
	  }

	  dcuCycleStatus[ifo][j] = 0;
	  dcuStatCycle[ifo][j] = 0;
	//  ipc->status = ipc->status; // refresh in the RFM
	}

	{
	  int match, match1, match2;
	  int intCycle = ipc->cycle % DAQ_NUM_DATA_BLOCKS;
	  match = cblk1;
	  match1 = cblk1 - 1;
	  match2 = cblk1 + 2;
  	  match %= DAQ_NUM_DATA_BLOCKS;
          match1 %= DAQ_NUM_DATA_BLOCKS;
  	  match2 %= DAQ_NUM_DATA_BLOCKS;
    
	  /* Check if DCU is in sync       */
    	  if (intCycle == match
              || intCycle == match1
              || intCycle == match2)
		dcuCycleStatus[ifo][j] ++;
    
	  /* Check if DCU is running at all  */
	  if (intCycle != dcuLastCycle[ifo][j]) dcuStatCycle[ifo][j] ++;
	  dcuLastCycle[ifo][j] = intCycle;
	}
      }

      // Update DCU status
      int newStatus = ipc -> status != DAQ_STATE_RUN? 0xbad: 0;
      int newCrc = ipc -> crc;
#if defined(USE_GM) || defined(USE_MX) || defined(USE_UDP)
      if (IS_MYRINET_DCU(j)) {
        newCrc = gmDaqIpc[j].crc;
	//newStatus = 0;
      }
#endif
      if (!IS_EXC_DCU(j)) {
	if (newCrc != daqd.dcuConfigCRC[ifo][j]) newStatus |= 0x2000;
      }
      if (newStatus != daqd.dcuStatus[ifo][j]) {
	//system_log(1, "DCU %d IFO %d (%s) %s", j, ifo, dcuName[j], newStatus? "fault": "running");
	if (newStatus & 0x2000) {
	  //system_log(1, "DCU %d IFO %d (%s) reconfigured (crc 0x%x rfm 0x%x)", j, ifo, dcuName[j], daqd.dcuConfigCRC[ifo][j], newCrc);
	}
      }
      daqd.dcuStatus[ifo][j] = newStatus;
//#if defined(_ADVANCED_LIGO)
      //if (IS_MYRINET_DCU(j))
        //daqd.dcuCycle[ifo][j] = dcu_cycle;
      //else
//#endif
      daqd.dcuCycle[ifo][j] = ipc -> cycle;

      /* Check DCU data checksum */
      crc = 0;
      long bytes = daqd.dcuDAQsize[ifo][j];
      unsigned char *cp = (unsigned char *)read_dest;
#if defined(_ADVANCED_LIGO)
      if (IS_MYRINET_DCU(j)) {
        while (bytes--) {
          crc = (crc << 8) ^ crctab[((crc >> 24) ^ *(cp++)) & 0xFF];
	}
      } else
#endif
#ifdef __linux__
      while (bytes--) {
        crc = (crc << 8) ^ crctab[((crc >> 24) ^ *(cp++)) & 0xFF];
      }
#else
      while (bytes--) {
	/* Two bits flipped to swap four bytes */
	crc = (crc << 8) ^ crctab[((crc >> 24) ^ *((char *)((unsigned long)cp^3))) & 0xFF];
	cp++;
      }
#endif
      bytes = daqd.dcuDAQsize[ifo][j];
      while (bytes > 0) {
	crc = (crc << 8) ^ crctab[((crc >> 24) ^ bytes) & 0xFF];
	bytes >>= 8;
      }
      crc = ~crc & 0xFFFFFFFF;

#if 0
#if defined(_ADVANCED_LIGO) && defined(__linux__) && defined(COMPAT_INITIAL_LIGO) && defined(DATA_CONCENTRATOR)
// Byteswap data
// At the 40m, the DUCs are sending the big-endian data
// Here we want to swap that  so we can do trend calculation on it (
// :TODO: this hsould be based on channel configuration (chan rates and data sizes)
// :TODO: we want to save data into frames as is, but byteswap when sending it online
// and before trend gets calculated
	{
		int i;
		int nshorts = read_size / 2;
		short *s = (short *)read_dest;
		for (i = 0; i < nshorts; i++) {
			*s = ntohs(*s);
			s++;
		}
	}
#endif
#endif
      // Reset CRC/second variable for this DCU
      if (cblk % 16 == 0)  {
	daqd.dcuCrcErrCntPerSecond[ifo][j] = daqd.dcuCrcErrCntPerSecondRunning[ifo][j];
	daqd.dcuCrcErrCntPerSecondRunning[ifo][j] = 0;
      }

      if (j >= DCU_ID_ADCU_1 && (!IS_TP_DCU(j)) && daqd.dcuStatus[ifo][j] == 0) {
	unsigned int rfm_crc = ipc -> bp[cblk].crc;

#if defined(USE_GM) || defined(USE_MX) || defined(USE_UDP)
      if (IS_MYRINET_DCU(j)) rfm_crc = gmDaqIpc[j].bp[cblk].crc;
#endif

	if (rfm_crc != crc) {
	  system_log(3, "dcu %d (%s); crc[%d]=%x; computed crc=%x\n",
		     j, dcuName[j], cblk, rfm_crc, crc);

	  /* Set DCU status to BAD, all data will be marked as BAD 
	     because of the CRC mismatch */
	  daqd.dcuStatus[ifo][j] = 0x1000;
	  daqd.dcuCrcErrCnt[ifo][j]++;
	  daqd.dcuCrcErrCntPerSecondRunning[ifo][j]++;
	}
      }
      read_dest += daqd.dcuSize[ifo][j];
    }
  }

  /* :TODO: need to put together nice error reporting and recovery here */
  return 0;
}
#endif


#if defined(_ADVANCED_LIGO)


  // Pointer to GDS TP tables
  struct cdsDaqNetGdsTpNum *gdsTpNum[2][DCU_COUNT];


void*
gm_receiver_thread(void *)
{
#if defined(USE_GM) || defined(USE_MX) || defined(USE_BROADCAST) || defined(USE_UDP)
  // Allocate local test point tables
  static struct cdsDaqNetGdsTpNum gds_tp_table[2][DCU_COUNT];

for (int ifo = 0; ifo < daqd.data_feeds; ifo++) {
  for (int j = DCU_ID_EDCU; j < DCU_COUNT; j++) {
    if (daqd.dcuSize[ifo][j] == 0) continue; // skip unconfigured DCU nodes
    if (IS_MYRINET_DCU(j)) {
      gdsTpNum[ifo][j] = gds_tp_table[ifo] + j;
#if defined(USE_BROADCAST)
    } else if (IS_TP_DCU(j)) {
      gdsTpNum[ifo][j] = gds_tp_table[ifo] + j;
#endif
    } else {
      gdsTpNum[ifo][j] = 0;
    }
  }
}
#endif

#ifdef USE_GM
	gm_recv();
#elif defined(USE_MX)
  void receiver_mx(int);
	receiver_mx(1);
#elif defined(USE_UDP)
  receiver_udp();
#endif
}

void *
producer::frame_writer ()
{
#ifdef USE_SYMMETRICOM
	unsigned long gps, prev_gps, frac, prev_frac;
	gps = prev_gps = frac = prev_frac = 0;
#endif
   pthread_t gm_tid;

// :TODO: gds_signal_refresh has to set by the gds.cc when signals are set or unset.
#ifndef NO_BROADCAST
    int gds_signal_refresh = 0;
#endif
    circ_buffer_block_prop_t prev_prop;
    prev_prop.gps = 0;
    int prev_cycle = 0;

#ifdef not_def
  // Bind to processor three -- No SIGBUS on Ultra 450 with this option

  if (processor_bind (P_LWPID, _lwp_self(), 3, NULL)) {
     system_log(1, "processor_bind(3,) failed; errno = %d", errno);
     exit (1);
  }
#endif

  // Put this thread into the realtime scheduling class
  // if we are running as root
  daqd_c::realtime ("VMIC RFM producer", 1);

  //  pthread_mutex_lock (&framelib_lock);

 if (!daqd.no_myrinet) {
#if !defined(sun) && (defined(USE_GM) || defined(USE_MX))
    int res = gm_setup();
    if (res != 0) {
      system_log(1, "couldn't setup Myrinet\n");
      exit (1);
    }
#endif

    {
     pthread_attr_t attr;
     pthread_attr_init (&attr);
     pthread_attr_setstacksize (&attr, daqd.thread_stack_size);
     pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
     //  pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
     int err_no;
  
     if (err_no = pthread_create (&gm_tid, &attr,
				  gm_receiver_thread, 0)) {
     	pthread_attr_destroy (&attr);
     	system_log(1, "pthread_create() err=%d", err_no);
	exit(1);
     }
     pthread_attr_destroy (&attr);
     system_log(1, "Network receiver thread started");
    }
    sleep(1);
 }


  // SC IPC
  volatile struct rmIpcStr *ipc = (struct rmIpcStr *) (daqd.rm_mem_ptr + DAQSC_IPC);
#ifdef AUX_5579
  volatile struct rmIpcStr *ipc1 = (struct rmIpcStr *) (daqd.rm_aux_ptr + DAQSC_IPC);
#else
  volatile struct rmIpcStr *ipc1 = ipc;
#endif

  // Testing on one of the DCU's IPC
  // struct rmIpcStr *ipc = (struct rmIpcStr *) (daqd.rm_mem_ptr + 0xa0000);

  // Frame builder IPC
  volatile struct rmIpcStr *fb_ipc
    = (struct rmIpcStr *) (daqd.rm_mem_ptr + daqd.fb_ipc/* FB0_IPC */ );

#ifdef AUX_5579
  if (daqd.cit_40m) {
     /* Point frame builder IPC into 5579 reflective memory */
     fb_ipc = (struct rmIpcStr *) (daqd.rm_aux_ptr + daqd.fb_ipc);
  }
#endif


  volatile struct rmIpcStr *fb_ipc1
    = (struct rmIpcStr *) (daqd.rm_mem_ptr1 + daqd.fb_ipc/* FB0_IPC */ );

  //  requestRFMUpdate ();

  // Fill in my IPC structure
  fb_ipc -> dcuId = bsw((short)FB0_ID);
  //fb_ipc -> dcuType = bsw((short)DAQS_UTYPE_FB);
  //fb_ipc -> dcuNodeId = bsw((short)FB0_ID);
  fb_ipc -> channelCount = bsw((short)daqd.num_channels);
  fb_ipc -> status = bsw((short)DAQ_STATE_CONFIG);
  fb_ipc -> command = bsw((short)DAQS_CMD_NO_CMD);

  unsigned int lastCycle = 0xffffffff;
  int count;

  unsigned int accum_cycles = 0;

  int bs;
  char *buf = 0; // (char *) malloc (bs = daqd.b1 -> block_size ());

  //  pthread_mutex_unlock (&framelib_lock);

#undef rfm_interrupt

#ifdef rfm_interrupt
  rfmGetNevents_t rge;
  rfmNevents (daqd.rh, &rge, 1);                 /* Get initial counts and clear */
  rfmSetTimeout (daqd.rh, daqd.rfm_event_id, 0);  /* No timeout   */

  rfmDisable (daqd.rh, daqd.rfm_event_id);
  rfmEnable (daqd.rh, daqd.rfm_event_id);
#endif

  //fb_ipc -> status = bsw((short)DAQ_STATE_RUN);

  // pread() is used to read from the private RFM network just one block every 16th of a second
#ifdef FILE_CHANNEL_CONFIG
#else
  volatile struct mainMapStr *mmap = (struct mainMapStr *)(daqd.rm_mem_ptr + MAIN_MAP);
  system_log(1,"RFM has %d blocks specified in MMAP", bsw(mmap->numBlocks));
  int numBlocks = bsw(mmap->numBlocks);
#endif
  if (PRIVATE_BUFFERS > 32 || PRIVATE_BUFFERS%16!=0) {
    system_log(1,"The number of blocks is invalid: %d; FATAL", PRIVATE_BUFFERS);
    _exit(1);
  }


#ifdef FILE_CHANNEL_CONFIG
  for (int i = 0; i < PRIVATE_BUFFERS; i++)
    for (int j = 0; j < DCU_COUNT; j++) {
      read_from[j][i] = DATA_OFFSET_DCU(j) + i*DAQ_DCU_BLOCK_SIZE;
    }
#else
  long read_from [PRIVATE_BUFFERS];
  void *read_dest = daqd.move_buf;
  long read_size = daqd.channels [0].rm_block_size;
  volatile struct rmIpcStr *ipcs [PRIVATE_BUFFERS];
  {
    for (int i = 0; i < PRIVATE_BUFFERS; i++) {
      read_from [i] = bsw(mmap -> baseOffset) + i*bsw(mmap->partitionSize);
      ipcs [i] = (struct rmIpcStr *) (daqd.rm_mem_ptr + read_from[i]);
      system_log(5, "producer is reading RFM from 0x%x to +%d", read_from [i], read_size);
    }
  }
#endif

#if defined(sun)
#undef DUMMY_CONTROLLER
#else
//#define DUMMY_CONTROLLER 1
#undef DUMMY_CONTROLLER
#endif

#if defined(DUMMY_CONTROLLER)
   // Wait until a second boundary
   {
#if 0
      struct timeb timb;
      timb.millitm = 1;
      while (timb.millitm != 0) {
        ftime(&timb);
      }
#endif
      system_log(1, "Waiting for DCU %d to show up", daqd.controller_dcu);
      gmDaqIpc[daqd.controller_dcu].cycle = 1;
      for (int i = 0; gmDaqIpc[daqd.controller_dcu].cycle % 16; i++) {
         struct timespec tspec = {0,1000000000/16/100}; // seconds, nanoseconds
         nanosleep (&tspec, NULL);
         if (i > 1600*4*100) {
                /* OK the front end isn't there, let's get going on our own */
                system_log(1, "Looks like dcu %d is not there. Running on my own.", daqd.controller_dcu);
                break;
         }       }
      controller_cycle = 0;
   }
#else
#ifdef USE_SYMMETRICOM
   int zero_time;
   if (cycle_input == 8) {
waiting:
	if (daqd.symm_ok() == 0) {
		printf("The Symmetricom IRIG-B timing card is not synchronized, waiting\n");
		//exit(10);
		sleep(1);
		goto waiting;
	}
	const unsigned int c = 1000000000/16;
	unsigned long track_gps = 0;
	for(;;) {
		unsigned long f;
		gps = daqd.symm_gps(&f);
		if (gps < track_gps) {
			printf("GPS time as returned by the Symmetricom card goes back. Now %d before %d.\n", gps, track_gps);
			exit(1);
		}
		track_gps = gps;
	 	prev_frac = 1000000000 - c;
		// Starting at this time
		prev_gps = gps - 1;
		frac = f;
		//if (f > 994493000) break;
		if (f < (4 * c)  && f > (3 * c)) break; // Three cycles after a second
 	}
	frac = 3*c; prev_frac = 2*c;
        printf("Starting at gps %d frac %d\n", gps, frac);
        controller_cycle = 1;
	zero_time = gps;
  } else 
#endif
  // Wait for the controller before starting up
  if ((cycle_input & 2) == 0) {
    for (;ipc1 -> status != DAQ_STATE_RUN;) {
	system_log(1,"Waiting for controller on 5579; ipc ptr=0x%x", ipc1);
	sleep(1);
    }
  } else {
    for (;ipc -> status != DAQ_STATE_RUN;) {
	system_log(1,"Waiting for controller on 5565; ipc ptr=0x%x", ipc);
	sleep(1);
    }
  }
#endif

#if EPICS_EDCU == 1
  extern unsigned int pvValue[1000];
  pvValue[5] = 0;
  pvValue[17] = 0;
#endif

#if defined(DUMMY_CONTROLLER)
   int zero_time = time(0) - 315964819 + 33;
   int prev_controller_cycle = -1;
#endif

  // for each 16th of a second
  for (count = 0;;) {
#ifdef rfm_interrupt
    rfmGetNevents_t newRge;
    rfmErrorMsg_t   oldErrorMsg;
#endif

    //ushort_t        fromNode;
    //uint_t          eventNo;


    // FIXME: put() usage here is invalid, should do a number of put16th() callswith prop block length 0 or similar. How to do loseless reconfigure?

    if (daqd.shutting_down)
      {
	(void) daqd.b1 -> put (buf, 0);
	break;
      }

    //fb_ipc -> status = bsw((short)DAQ_STATE_RUN);

    long cur_cycle;
    unsigned int cycleInput = 0; // Over which network the cycle number read
#if rfm_interrupt
    (void) rfmReceive (daqd.rh, daqd.rfm_event_id, &fromNode, 0);

#else
    if (lastCycle == 0xffffffff) { 
#ifdef USE_SYMMETRICOM
      if (cycle_input == 8)  {
	cur_cycle = controller_cycle;
	printf("controller_cycle=%d  ipc cycle=%d\n", controller_cycle, ipc1->cycle);
      } else 
#endif
	cur_cycle = ipc1->cycle;
    } else { 
      int np;
      struct timespec tspec = {0,1000000000/16}; // seconds, nanoseconds
      unsigned long cycle2;


#if defined(DUMMY_CONTROLLER)
if (0)	{
		struct timespec tspec1 = {0,1000000000/16};
		nanosleep (&tspec1, NULL);
		cur_cycle = lastCycle + 1;
	}

       for (int j = 0; prev_controller_cycle == controller_cycle; j++) {
         struct timespec tspec = {0,1000000000/16/100}; // seconds, nanoseconds
         nanosleep (&tspec, NULL);
         if (j > 150) {
           printf("lost sync source\n");
        //   resync = 1;
           controller_cycle = 0;
           prev_controller_cycle = -1; // so we can break out of the loop
         }
       }
     prev_controller_cycle = controller_cycle;
	//cur_cycle = lastCycle + 1;
	//printf("controller_cycle = %d\n", controller_cycle);
	cur_cycle++;
	
  
#else
#if defined(USE_SYMMETRICOM)
    if (cycle_input == 8) {
       unsigned long track_gps = 0;
       for (;;) {
         gps = daqd.symm_gps(&frac);
	 if (gps < track_gps) {
		printf("GPS time as returned by the Symmetricom card goes back. Now %d before %d.\n", gps, track_gps);
		_exit(1);
	}
	track_gps = gps;
//printf("%d %d %d %d\n", gps, frac, prev_gps, prev_frac);
         if (prev_frac == 937500000) {
	   if (gps == prev_gps + 1) {
                frac = 0;
                break;
	    }
         } else if (frac >= prev_frac + 62500000) {
                frac = prev_frac + 62500000;
                break;
         }

         struct timespec tspec = {0,1000000000/16/100}; // seconds, nanoseconds
         nanosleep (&tspec, NULL);
       }
       controller_cycle++;
       //printf("GPS gps=%d %d cycle %d\n", gps, frac, controller_cycle);
	cur_cycle = controller_cycle;
    } else 
#endif
     {
      for(np = 0;;np++) {
	unsigned long cycle1 = ipc1->cycle;
	unsigned long cycle2 = ipc->cycle;
	/* First check 5579 network for cycle increment */
	if (cycle_input & 1) {
	  if (cycle1 > lastCycle) {
	    cur_cycle = cycle1;
	    cycleInput = 0;
	    break; 
	  }
	}
	if (cycle_input & 2) {
	  if (cycle2 > lastCycle) {
	    cur_cycle = cycle2;
  	    cycleInput = 1;
	    break; 
	  }
	}
	if (np == 100) break;
        nanosleep (&tspec, NULL);
      }
      system_log(5, "%d %d\n", cur_cycle, cycleInput);
      if (np == 100) {
	system_log(1, "Controller timeout");
	ipc -> status =  DAQ_STATE_FAULT;
	sleep(1);
	_exit(1); 
      } 
     }
#if EPICS_EDCU == 1
      pvValue[0] = cur_cycle;
      // Count how many seconds we were acquiring data
      if (cur_cycle % 16 == 0) {
		extern unsigned long dmt_retransmit_count;
		extern unsigned long dmt_failed_retransmit_count;
     		// Count how many seconds we were acquiring data
		pvValue[5]++;

#ifdef USE_SYMMETRICOM
  		pvValue[17] = gps;
#else
   		pvValue[17] = prev_prop.gps;
#endif
		// Display DMT retransmit channels every second
		pvValue[15] = dmt_retransmit_count;
		pvValue[16] = dmt_failed_retransmit_count;
		dmt_retransmit_count = 0;
		dmt_failed_retransmit_count = 0;
      }
#endif
#endif

    //    DEBUG(3, cerr << "cycle = " << cur_cycle << "status = " << lastCycle << endl);
    }
#endif

    circ_buffer_block_prop_t prop;
#ifndef NO_BROADCAST
    prop.gds_signal_refresh = 0;
#endif


#ifdef FILE_CHANNEL_CONFIG
    ;
#else
    if (DAQS_CMD_RFM_REFRESH == bsw(fb_ipc -> command)) {
	system_log(1, "DAQS_CMD_RFM_REFRESH command #%d received", bsw(fb_ipc -> command));
	fb_ipc -> command = bsw((short)DAQS_CMD_NO_CMD);
#ifdef GDS_TESTPOINTS
	daqd.gds.signal ();
#endif
#ifndef NO_BROADCAST
	gds_signal_refresh = 1;
#endif
    // Shutdown on an unknown command
    } else if (DAQS_CMD_NO_CMD != bsw(fb_ipc -> command)) {
	system_log(1, "command %d received; ignored", bsw(fb_ipc -> command));
	fb_ipc -> cmdAck = bsw(fb_ipc -> command);
	fb_ipc -> command = bsw((short)DAQS_CMD_NO_CMD);
    }
#endif


    if (daqd.shutting_down)
      {
	(void) daqd.b1 -> put (buf, 0);
	break;
      }

    int from_cycle;

    count++;

#ifdef rfm_interrupt
    // Skipped queued interrupts
    if (cur_cycle == lastCycle)
      continue;
#endif

#ifdef USE_SYMMETRICOM
if (cycle_input == 8) {
	from_cycle = cur_cycle;
} else 
#endif
{
    if (cur_cycle != (lastCycle + 1)) {
      system_log(5, "cycle = %d status = %d", cur_cycle, lastCycle);
      //      DEBUG(3, cerr << "RESYNC: cycle = " << cur_cycle << "status = " << lastCycle << endl);

      if (lastCycle == 0xffffffff)  // First time
	{
	if (cur_cycle % PRIVATE_BUFFERS != 1) // Skip to the buffer's boundary plus 1
	  continue;
	from_cycle = cur_cycle;
      } else if (cur_cycle > lastCycle + PRIVATE_BUFFERS - 1) {
	system_log(1, "Lost cycles: %d; shutdown", cur_cycle - lastCycle);
	// shutdown_server ();
	sleep (1);
	_exit (1);
      } else if (cur_cycle < lastCycle) { // Handle VxWorks code restart
	// Do not attempt to recover when cycle number goes back
        system_log(1, "cur_cycle: %d; lastCycle: %d; shutdown", cur_cycle, lastCycle);
        // shutdown_server ();
        sleep (1);
        _exit (1);
      } else {
	from_cycle = lastCycle + 1;
      }
    } else {
      from_cycle = cur_cycle;
    }

}
    lastCycle = cur_cycle;

    // Loop through all cycles (including lost cycles)
    for (int i = from_cycle;
	 i <= cur_cycle;
	 (daqd.is_fault() ? : fb_ipc -> cycle = bsw(i)), i++) // Reflect processed cycle into the FB IPC
      {

	if (daqd.data_feeds == 2) if (!daqd.is_fault()) fb_ipc1 -> cycle = bsw(i);

	accum_cycles++;

	unsigned int cblk = (i-1)%PRIVATE_BUFFERS;
	{


	    fb_ipc -> bp[cblk].timeSec = bsw(ipc -> bp[cblk].timeSec);
	    fb_ipc -> bp[cblk].timeNSec = bsw(ipc -> bp[cblk].timeNSec);

#ifdef FILE_CHANNEL_CONFIG
            if (!parallel) grabIfoData(0, cblk, daqd.move_buf); 
            if (daqd.data_feeds > 1) {
		currentBlock = cblk;
		/* Start data read for the second ifo */
#if 0
		sem_post(&go);
#else
		pthread_mutex_lock(&prod_mutex);
		prod_go = 1;
		prod_done = 0;
		pthread_mutex_unlock(&prod_mutex);
		pthread_cond_signal(&prod_cond_go);
#endif
            } 
            if (parallel) grabIfoData(0, cblk, daqd.move_buf); 
            if (daqd.data_feeds > 1) {
		// Wait for the second ifo 
#if 0
    		for (int res = sem_wait(&done); res;) {
	  		system_log(1, "sem_wait_done errno=%d", res);
    		}
#else
		pthread_mutex_lock(&prod_mutex);
		while (prod_done == 0) 
			pthread_cond_wait(&prod_cond_done, &prod_mutex);
		pthread_mutex_unlock(&prod_mutex);
#endif
	    }
#else
	    for (;;) {
	      int nread;
	      if ((nread = pread (daqd.rh -> rh_fd, read_dest, read_size, read_from [cblk])) < 0) 
	      {
		if (errno == EINTR) {
		  system_log(1, "pread() errno %d (EINTR)", errno);
		  continue;
		} else {
		  system_log(1, "pread() errno %d", errno);
		  break;
		}
	      } else {
		if (nread != read_size) {
		  system_log(1, "read() %d bytes", nread);
		}
		break;
	      }
	    }
#endif


#ifdef FILE_CHANNEL_CONFIG
#if defined(DUMMY_CONTROLLER)
     prop.gps = i/16 + zero_time;
     prop.gps_n = 1000000000/16 * (i % 16);
#else

#ifdef USE_SYMMETRICOM
	if (cycle_input == 8) {
	    //prop.gps = gps;
	    //prop.gps_n = frac;
     	    prop.gps = (i-1)/16 + zero_time;
	    prop.gps_n = 1000000000/16 * ((i-1) % 16);
	    prop.run = 0;
	   system_log(5, "Symmetricom timing: gps=%d.%d block=%d", prop.gps, prop.gps_n, cblk);
	} else 
#endif
	{
	    prop.gps = (cycleInput? ipc: ipc1) -> bp[cblk].timeSec;
	    prop.gps_n = (cycleInput? ipc: ipc1) -> bp [cblk].timeNSec;
	    prop.run = (cycleInput? ipc: ipc1) -> bp [cblk].run;

	   system_log(5, "Timing: gps=%d.%d\n block=%d", prop.gps, prop.gps_n, cblk);

	  // check for timing errors
	  if (prev_prop.gps != 0) {
	    if (((prop.gps==prev_prop.gps)&&(62500000 == (prop.gps_n - prev_prop.gps_n)))
	 	 || ((prop.gps==(prev_prop.gps+1))&&(prop.gps_n==0)&&(prev_prop.gps_n==937500000)))
		;
	    else {
		/* Try getting time data from another network */
		cycleInput ^= 1;
		
            prop.gps = (cycleInput? ipc: ipc1) -> bp[cblk].timeSec;
            prop.gps_n = (cycleInput? ipc: ipc1) -> bp [cblk].timeNSec;
            prop.run = (cycleInput? ipc: ipc1) -> bp [cblk].run;

	  if (prev_prop.gps != 0) {
	    if (((prop.gps==prev_prop.gps)&&(62500000 == (prop.gps_n - prev_prop.gps_n)))
	 	 || ((prop.gps==(prev_prop.gps+1))&&(prop.gps_n==0)&&(prev_prop.gps_n==937500000))) {
		;
	    }
	    else {
		system_log(1, "timing error: gps=%d.%d, prev_gps=%d.%d\n",
				prop.gps, prop.gps_n, prev_prop.gps, prev_prop.gps_n);
		 shutdown_server ();
	    }
	    }
	  }
	  }
	}
#endif
#else
	    prop.gps = bsw(ipcs [cblk] -> bp [0].timeSec);
	    prop.gps_n = bsw(ipcs [cblk] -> bp [0].timeNSec);
	    prop.run = bsw(ipcs [cblk]-> bp [0].run);

	    // Check for commands
    	    // Shutdown on an unknown command
    	    if (DAQS_CMD_NO_CMD != bsw(fb_ipc -> command)
    		&& DAQS_CMD_RFM_REFRESH != bsw(fb_ipc -> command)) {
	      system_log(1, "command %d received; ignored", bsw(fb_ipc -> command));
	      fb_ipc -> cmdAck = bsw(fb_ipc -> command);
	      fb_ipc -> command = bsw((short)DAQS_CMD_NO_CMD);
	    }
#endif
	  }
	  prop.leap_seconds = daqd.gps_leap_seconds(prop.gps);
	  tzset ();
#ifndef __linux__
	  prop.altzone = -altzone;
#endif
	  prop.cycle = cur_cycle;

#ifdef DATA_CONCENTRATOR
	// Assign per-DCU data we need to broadcast out
	//
	for (int ifo = 0; ifo < daqd.data_feeds; ifo++) {
          for (int j = DCU_ID_EDCU; j < DCU_COUNT; j++) {
                if (IS_TP_DCU(j)) continue;     // Skip TP and EXC DCUs
                if (daqd.dcuSize[ifo][j] == 0) continue; // Skip unconfigured DCUs
  		prop.dcu_data[j + ifo*DCU_COUNT].cycle = daqd.dcuCycle[ifo][j];
		volatile struct rmIpcStr *ipc = daqd.dcuIpc[ifo][j];
#if defined(USE_GM) || defined(USE_MX) || defined(USE_UDP)
		// Do not support Myrinet DCUs on H2
      		if (IS_MYRINET_DCU(j) && ifo == 0) {
		  prop.dcu_data[j].crc = gmDaqIpc[j].bp[cblk].crc;
		  //printf("dcu %d crc=0x%x\n", j, prop.dcu_data[j].crc);
  		  prop.dcu_data[j].status = daqd.dcuStatus[0 /* IFO */][j];
      		} else
#endif
		// EDCU is "attached" to H1, not H2
		if (j == DCU_ID_EDCU && ifo == 0) {
			// See if the EDCU thread is running and assign status
			if (0x0 == (prop.dcu_data[j].status = daqd.edcu1.running? 0x0: 0xbad)) {
				// If running calculate the CRC
				
	//memcpy(read_dest, (char *)(daqd.edcu1.channel_value + daqd.edcu1.fidx), daqd.dcuSize[ifo][j]);
			
			  unsigned int bytes = daqd.dcuSize[0][DCU_ID_EDCU];
			  unsigned char *cp = daqd.move_buf; // The EDCU data is in front
			  unsigned long crc = 0;
      			  while (bytes--) {
        		    crc = (crc << 8) ^ crctab[((crc >> 24) ^ *(cp++)) & 0xFF];
      			  }
      			  bytes = daqd.dcuDAQsize[0][DCU_ID_EDCU];
      			  while (bytes > 0) {
			    crc = (crc << 8) ^ crctab[((crc >> 24) ^ bytes) & 0xFF];
			    bytes >>= 8;
      			  }
      			  crc = ~crc & 0xFFFFFFFF;
  			  prop.dcu_data[j].crc = crc;
			}
		} else {
  			prop.dcu_data[j + ifo*DCU_COUNT].crc = ipc -> bp[cblk].crc;
  			prop.dcu_data[j + ifo*DCU_COUNT].status = daqd.dcuStatus[ifo][j];
		}
	  }
	}

#endif

#ifndef FILE_CHANNEL_CONFIG
	  // check for timing errors
	  if (prev_prop.gps != 0) {
	    if (((prop.gps==prev_prop.gps)&&(62500000 == (prop.gps_n - prev_prop.gps_n)))
	 	 || ((prop.gps==(prev_prop.gps+1))&&(prop.gps_n==0)&&(prev_prop.gps_n==937500000)))
		;
	    else {
		system_log(1, "Timing error: gps=%d.%d, prev_gps=%d.%d\n",
				prop.gps, prop.gps_n, prev_prop.gps, prev_prop.gps_n);
//		 shutdown_server ();
	    }
	  }
#endif
	  
	  prev_prop = prop;
	  prev_cycle = cur_cycle;

#ifdef USE_SYMMETRICOM
     prev_gps = gps;
     prev_frac = frac;
#endif


	  
	  //	  DEBUG(1, cerr << "accum_ccyles=" << accum_cycles << "; daqd.vmic_pv_len=" << daqd.vmic_pv_len << endl);

#ifndef NO_BROADCAST
	  // full second property is set if there was refresh within this second
	  if (accum_cycles == 1) {
		prop.gds_signal_refresh = gds_signal_refresh;
//		system_log(1,"gds signal passed %d", gds_signal_refresh);
		gds_signal_refresh = 0;
	  }
#endif


	  TNF_PROBE_0(producer_frame_writer_start, "producer::frame_writer", "before put()");
	  int nbi;

	  nbi = daqd.b1 -> put16th_dpscattered (daqd.vmic_pv, daqd.vmic_pv_len, &prop);

	  TNF_PROBE_1(producer_frame_writer_end, "producer::frame_writer", "after put()",
		      tnf_long,   block_no,    nbi);


	  if (accum_cycles == 16) {
	    accum_cycles = 0;
	  }
	} // all cycles loop (inluding lost cycles)
      } // initial `for' loop


    fb_ipc -> status = bsw((short)DAQ_STATE_CONFIG);

  // Shutdown cleanup
#ifdef not_def
  free ((void *) buf);
#endif

#ifdef rfm_interrupt
  rfmDisable(daqd.rh, daqd.rfm_event_id);
#endif

#if VMICRFMTYPE == 5565
  RFM2gClose(&daqd.rh);
#else
  rfmClose(daqd.rh);
#endif

  DEBUG1(cerr << "producer #" << pnum << " finished" << endl);
  return NULL;
}

#else /* _ADVANCED_LIGO is not defined */

void *
producer::frame_writer ()
{
// :TODO: gds_signal_refresh has to set by the gds.cc when signals are set or unset.
#ifndef NO_BROADCAST
    int gds_signal_refresh = 0;
#endif
    circ_buffer_block_prop_t prev_prop;
    prev_prop.gps = 0;
    int prev_cycle = 0;

#ifdef not_def
  // Bind to processor three -- No SIGBUS on Ultra 450 with this option

  if (processor_bind (P_LWPID, _lwp_self(), 3, NULL)) {
     system_log(1, "processor_bind(3,) failed; errno = %d", errno);
     exit (1);
  }
#endif

  // Put this thread into the realtime scheduling class
  // if we are running as root
  daqd_c::realtime ("VMIC RFM producer", 1);

  //  pthread_mutex_lock (&framelib_lock);


  // SC IPC
  volatile struct rmIpcStr *ipc = (struct rmIpcStr *) (daqd.rm_mem_ptr + DAQSC_IPC);
#ifdef AUX_5579
  volatile struct rmIpcStr *ipc1 = (struct rmIpcStr *) (daqd.rm_aux_ptr + DAQSC_IPC);
#else
  volatile struct rmIpcStr *ipc1 = ipc;
#endif

  // Testing on one of the DCU's IPC
  // struct rmIpcStr *ipc = (struct rmIpcStr *) (daqd.rm_mem_ptr + 0xa0000);

  // Frame builder IPC
  volatile struct rmIpcStr *fb_ipc
    = (struct rmIpcStr *) (daqd.rm_mem_ptr + daqd.fb_ipc/* FB0_IPC */ );

#ifdef AUX_5579
  if (daqd.cit_40m) {
     /* Point frame builder IPC into 5579 reflective memory */
     fb_ipc = (struct rmIpcStr *) (daqd.rm_aux_ptr + daqd.fb_ipc);
  }
#endif


  volatile struct rmIpcStr *fb_ipc1
    = (struct rmIpcStr *) (daqd.rm_mem_ptr1 + daqd.fb_ipc/* FB0_IPC */ );

  //  requestRFMUpdate ();

  // Fill in my IPC structure
  fb_ipc -> dcuId = bsw((short)FB0_ID);
  //fb_ipc -> dcuType = bsw((short)DAQS_UTYPE_FB);
  //fb_ipc -> dcuNodeId = bsw((short)FB0_ID);
  fb_ipc -> channelCount = bsw((short)daqd.num_channels);
  fb_ipc -> status = bsw((short)DAQ_STATE_CONFIG);
  fb_ipc -> command = bsw((short)DAQS_CMD_NO_CMD);

  unsigned int lastCycle = 0xffffffff;
  int count;

  unsigned int accum_cycles = 0;

  int bs;
  char *buf = 0; // (char *) malloc (bs = daqd.b1 -> block_size ());

  //  pthread_mutex_unlock (&framelib_lock);

#undef rfm_interrupt

#ifdef rfm_interrupt
  rfmGetNevents_t rge;
  rfmNevents (daqd.rh, &rge, 1);                 /* Get initial counts and clear */
  rfmSetTimeout (daqd.rh, daqd.rfm_event_id, 0);  /* No timeout   */

  rfmDisable (daqd.rh, daqd.rfm_event_id);
  rfmEnable (daqd.rh, daqd.rfm_event_id);
#endif

  //fb_ipc -> status = bsw((short)DAQ_STATE_RUN);

  // pread() is used to read from the private RFM network just one block every 16th of a second
#ifdef FILE_CHANNEL_CONFIG
#else
  volatile struct mainMapStr *mmap = (struct mainMapStr *)(daqd.rm_mem_ptr + MAIN_MAP);
  system_log(1,"RFM has %d blocks specified in MMAP", bsw(mmap->numBlocks));
  int numBlocks = bsw(mmap->numBlocks);
#endif
  if (PRIVATE_BUFFERS > 32 || PRIVATE_BUFFERS%16!=0) {
    system_log(1,"The number of blocks is invalid: %d; FATAL", PRIVATE_BUFFERS);
    _exit(1);
  }


#ifdef FILE_CHANNEL_CONFIG
  for (int i = 0; i < PRIVATE_BUFFERS; i++)
    for (int j = 0; j < DCU_COUNT; j++) {
      read_from[j][i] = DATA_OFFSET_DCU(j) + i*DAQ_DCU_BLOCK_SIZE;
    }
#else
  long read_from [PRIVATE_BUFFERS];
  void *read_dest = daqd.move_buf;
  long read_size = daqd.channels [0].rm_block_size;
  volatile struct rmIpcStr *ipcs [PRIVATE_BUFFERS];
  {
    for (int i = 0; i < PRIVATE_BUFFERS; i++) {
      read_from [i] = bsw(mmap -> baseOffset) + i*bsw(mmap->partitionSize);
      ipcs [i] = (struct rmIpcStr *) (daqd.rm_mem_ptr + read_from[i]);
      system_log(5, "producer is reading RFM from 0x%x to +%d", read_from [i], read_size);
    }
  }
#endif

  // Wait for the controller before starting up
  if ((cycle_input & 2) == 0) {
    for (;ipc1 -> status != DAQ_STATE_RUN;) {
	system_log(1,"Waiting for controller; ipc ptr=0x%x", (unsigned int)ipc1);
	sleep(1);
    }
  } else {
    for (;ipc -> status != DAQ_STATE_RUN;) {
	system_log(1,"Waiting for controller; ipc ptr=0x%x", (unsigned int)ipc);
	sleep(1);
    }
  }

#if EPICS_EDCU == 1
  extern unsigned int pvValue[1000];
  pvValue[5] = 0;
  pvValue[17] = 0;
#endif

  // for each 16th of a second
  for (count = 0;;) {
#ifdef rfm_interrupt
    rfmGetNevents_t newRge;
    rfmErrorMsg_t   oldErrorMsg;
#endif

    //ushort_t        fromNode;
    //uint_t          eventNo;


    // FIXME: put() usage here is invalid, should do a number of put16th() callswith prop block length 0 or similar. How to do loseless reconfigure?

    if (daqd.shutting_down)
      {
	(void) daqd.b1 -> put (buf, 0);
	break;
      }

    //fb_ipc -> status = bsw((short)DAQ_STATE_RUN);

    long cur_cycle;
    unsigned int cycleInput = 0; // Over which network the cycle number read
#ifdef rfm_interrupt
    (void) rfmReceive (daqd.rh, daqd.rfm_event_id, &fromNode, 0);

#else
    if (lastCycle == 0xffffffff)
	cur_cycle = ipc1->cycle;
    else
    { 
      int np;
      struct timespec tspec = {0,1000000000/16/6}; // seconds, nanoseconds
      unsigned long cycle2;

#if 1
      for(np = 0;;np++) {
	unsigned long cycle1 = ipc1->cycle;
	unsigned long cycle2 = ipc->cycle;
	/* First check 5579 network for cycle increment */
	if (cycle_input & 1) {
	  if (cycle1 > lastCycle) {
	    cur_cycle = cycle1;
	    cycleInput = 0;
	    break; 
	  }
	}
	if (cycle_input & 2) {
	  if (cycle2 > lastCycle) {
	    cur_cycle = cycle2;
  	    cycleInput = 1;
	    break; 
	  }
	}
	if (np == 100) break;
        nanosleep (&tspec, NULL);
      }
      /*system_log(1, "%d %d\n", cur_cycle, cycleInput);*/
#if EPICS_EDCU == 1
      pvValue[0] = cur_cycle;
      // Count how many seconds we were acquiring data
      if (cur_cycle % 16 == 0) {
     	  // Count how many seconds we were acquiring data
	  pvValue[5]++;
	  pvValue[17] = gps;
#ifndef NO_BROADCAST
	  {
		extern unsigned long dmt_retransmit_count;
		extern unsigned long dmt_failed_retransmit_count;
		// Display DMT retransmit channels every second
		pvValue[15] = dmt_retransmit_count;
		pvValue[16] = dmt_failed_retransmit_count;
		dmt_retransmit_count = 0;
		dmt_failed_retransmit_count = 0;
	  }
#endif
      }
#endif
#else
      for (np = 0;lastCycle == (cur_cycle = ipc1 -> cycle) && DAQS_CMD_NO_CMD == bsw(fb_ipc -> command); np++) 
      {
	if (np == 100) break;
        nanosleep (&tspec, NULL);
      }
#endif
      if (np == 100) {
	system_log(1, "Controller timeout");
	ipc -> status =  DAQ_STATE_FAULT;
	sleep(1);
	_exit(1); 
      } 

    //    DEBUG(3, cerr << "cycle = " << cur_cycle << "status = " << lastCycle << endl);
    }
#endif

    circ_buffer_block_prop_t prop;
#ifndef NO_BROADCAST
    prop.gds_signal_refresh = 0;
#endif


#ifdef FILE_CHANNEL_CONFIG
    ;
#else
    if (DAQS_CMD_RFM_REFRESH == bsw(fb_ipc -> command)) {
	system_log(1, "DAQS_CMD_RFM_REFRESH command #%d received", bsw(fb_ipc -> command));
	fb_ipc -> command = bsw((short)DAQS_CMD_NO_CMD);
#ifdef GDS_TESTPOINTS
	daqd.gds.signal ();
#endif
#ifndef NO_BROADCAST
	gds_signal_refresh = 1;
#endif
    // Shutdown on an unknown command
    } else if (DAQS_CMD_NO_CMD != bsw(fb_ipc -> command)) {
	system_log(1, "command %d received; ignored", bsw(fb_ipc -> command));
	fb_ipc -> cmdAck = bsw(fb_ipc -> command);
	fb_ipc -> command = bsw((short)DAQS_CMD_NO_CMD);
    }
#endif


    if (daqd.shutting_down)
      {
	(void) daqd.b1 -> put (buf, 0);
	break;
      }

    int from_cycle;

    count++;

#ifdef rfm_interrupt
    // Skipped queued interrupts
    if (cur_cycle == lastCycle)
      continue;
#endif

    if (cur_cycle != (lastCycle + 1)) {
      //      system_log(1, "cycle = %d status = %d", cur_cycle, lastCycle);
      //      DEBUG(3, cerr << "RESYNC: cycle = " << cur_cycle << "status = " << lastCycle << endl);

      if (lastCycle == 0xffffffff)  // First time
	{
	if (cur_cycle % PRIVATE_BUFFERS != 1) // Skip to the buffer's boundary plus 1
	  continue;
	from_cycle = cur_cycle;
      } else if (cur_cycle > lastCycle + PRIVATE_BUFFERS - 1) {
	system_log(1, "Lost cycles: %d; shutdown", cur_cycle - lastCycle);
	// shutdown_server ();
	sleep (1);
	_exit (1);
      } else if (cur_cycle < lastCycle) { // Handle VxWorks code restart
	// Do not attempt to recover when cycle number goes back
        system_log(1, "cur_cycle: %d; lastCycle: %d; shutdown", cur_cycle, lastCycle);
        // shutdown_server ();
        sleep (1);
        _exit (1);
      } else {
	from_cycle = lastCycle + 1;
      }
    } else {
      from_cycle = cur_cycle;
    }

    lastCycle = cur_cycle;

    // Loop through all cycles (including lost cycles)
    for (int i = from_cycle;
	 i <= cur_cycle;
	 (daqd.is_fault() ? : fb_ipc -> cycle = bsw(i)), i++) // Reflect processed cycle into the FB IPC
      {

	if (daqd.data_feeds == 2) if (!daqd.is_fault()) fb_ipc1 -> cycle = bsw(i);

	accum_cycles++;

	{

	    unsigned int cblk = (i-1)%PRIVATE_BUFFERS;

	    fb_ipc -> bp[cblk].timeSec = bsw(ipc -> bp[cblk].timeSec);
	    fb_ipc -> bp[cblk].timeNSec = bsw(ipc -> bp[cblk].timeNSec);

#ifdef FILE_CHANNEL_CONFIG
            if (!parallel) grabIfoData(0, cblk, daqd.move_buf); 
            if (daqd.data_feeds > 1) {
		currentBlock = cblk;
		/* Start data read for the second ifo */
#if 0
		sem_post(&go);
#else
		pthread_mutex_lock(&prod_mutex);
		prod_go = 1;
		prod_done = 0;
		pthread_mutex_unlock(&prod_mutex);
		pthread_cond_signal(&prod_cond_go);
#endif
            } 
            if (parallel) grabIfoData(0, cblk, daqd.move_buf); 
            if (daqd.data_feeds > 1) {
		// Wait for the second ifo 
#if 0
    		for (int res = sem_wait(&done); res;) {
	  		system_log(1, "sem_wait_done errno=%d", res);
    		}
#else
		pthread_mutex_lock(&prod_mutex);
		while (prod_done == 0) 
			pthread_cond_wait(&prod_cond_done, &prod_mutex);
		pthread_mutex_unlock(&prod_mutex);
#endif
	    }
#else
	    for (;;) {
	      int nread;
	      if ((nread = pread (daqd.rh -> rh_fd, read_dest, read_size, read_from [cblk])) < 0) 
	      {
		if (errno == EINTR) {
		  system_log(1, "pread() errno %d (EINTR)", errno);
		  continue;
		} else {
		  system_log(1, "pread() errno %d", errno);
		  break;
		}
	      } else {
		if (nread != read_size) {
		  system_log(1, "read() %d bytes", nread);
		}
		break;
	      }
	    }
#endif


#ifdef FILE_CHANNEL_CONFIG
	    prop.gps = (cycleInput? ipc: ipc1) -> bp[cblk].timeSec;
	    prop.gps_n = (cycleInput? ipc: ipc1) -> bp [cblk].timeNSec;
	    prop.run = (cycleInput? ipc: ipc1) -> bp [cblk].run;

	  // check for timing errors
	  if (prev_prop.gps != 0) {
	    if (((prop.gps==prev_prop.gps)&&(62500000 == (prop.gps_n - prev_prop.gps_n)))
	 	 || ((prop.gps==(prev_prop.gps+1))&&(prop.gps_n==0)&&(prev_prop.gps_n==937500000)))
		;
	    else {
		/* Try getting time data from another network */
		cycleInput ^= 1;
		
            prop.gps = (cycleInput? ipc: ipc1) -> bp[cblk].timeSec;
            prop.gps_n = (cycleInput? ipc: ipc1) -> bp [cblk].timeNSec;
            prop.run = (cycleInput? ipc: ipc1) -> bp [cblk].run;

	  if (prev_prop.gps != 0) {
	    if (((prop.gps==prev_prop.gps)&&(62500000 == (prop.gps_n - prev_prop.gps_n)))
	 	 || ((prop.gps==(prev_prop.gps+1))&&(prop.gps_n==0)&&(prev_prop.gps_n==937500000)))
		;
	    else {
		system_log(1, "Timing error: gps=%d.%d, prev_gps=%d.%d\n",
				prop.gps, prop.gps_n, prev_prop.gps, prev_prop.gps_n);
		 shutdown_server ();
	    }
	    }
	  }
	  }
#else
	    prop.gps = bsw(ipcs [cblk] -> bp [0].timeSec);
	    prop.gps_n = bsw(ipcs [cblk] -> bp [0].timeNSec);
	    prop.run = bsw(ipcs [cblk]-> bp [0].run);

	    // Check for commands
    	    // Shutdown on an unknown command
    	    if (DAQS_CMD_NO_CMD != bsw(fb_ipc -> command)
    		&& DAQS_CMD_RFM_REFRESH != bsw(fb_ipc -> command)) {
	      system_log(1, "command %d received; ignored", bsw(fb_ipc -> command));
	      fb_ipc -> cmdAck = bsw(fb_ipc -> command);
	      fb_ipc -> command = bsw((short)DAQS_CMD_NO_CMD);
	    }
#endif
	  }
	  prop.leap_seconds = daqd.gps_leap_seconds(prop.gps);
	  tzset ();
#ifndef __linux__
	  prop.altzone = -altzone;
#endif
	  prop.cycle = cur_cycle;

#ifndef FILE_CHANNEL_CONFIG
	  // check for timing errors
	  if (prev_prop.gps != 0) {
	    if (((prop.gps==prev_prop.gps)&&(62500000 == (prop.gps_n - prev_prop.gps_n)))
	 	 || ((prop.gps==(prev_prop.gps+1))&&(prop.gps_n==0)&&(prev_prop.gps_n==937500000)))
		;
	    else {
		system_log(1, "Timing error: gps=%d.%d, prev_gps=%d.%d\n",
				prop.gps, prop.gps_n, prev_prop.gps, prev_prop.gps_n);
//		 shutdown_server ();
	    }
	  }
#endif
	  
	  prev_prop = prop;
	  prev_cycle = cur_cycle;



	  
	  //	  DEBUG(1, cerr << "accum_ccyles=" << accum_cycles << "; daqd.vmic_pv_len=" << daqd.vmic_pv_len << endl);

#ifndef NO_BROADCAST
	  // full second property is set if there was refresh within this second
	  if (accum_cycles == 1) {
		prop.gds_signal_refresh = gds_signal_refresh;
//		system_log(1,"gds signal passed %d", gds_signal_refresh);
		gds_signal_refresh = 0;
	  }
#endif


	  TNF_PROBE_0(producer_frame_writer_start, "producer::frame_writer", "before put()");
	  int nbi;

	  nbi = daqd.b1 -> put16th_dpscattered (daqd.vmic_pv, daqd.vmic_pv_len, &prop);

	  TNF_PROBE_1(producer_frame_writer_end, "producer::frame_writer", "after put()",
		      tnf_long,   block_no,    nbi);


	  if (accum_cycles == 16) {
	    accum_cycles = 0;
	  }
	} // all cycles loop (inluding lost cycles)
      } // initial `for' loop


    fb_ipc -> status = bsw((short)DAQ_STATE_CONFIG);

  // Shutdown cleanup
#ifdef not_def
  free ((void *) buf);
#endif

#ifdef rfm_interrupt
  rfmDisable(daqd.rh, daqd.rfm_event_id);
#endif

#if VMICRFMTYPE == 5565
  RFM2gClose(&daqd.rh);
#else
  rfmClose(daqd.rh);
#endif

  DEBUG1(cerr << "producer #" << pnum << " finished" << endl);
  return NULL;
}

#endif /* _ADVANCED_LIGO not defined */

#else /* ! defined(VMICRFM_PRODUCER) */
#if defined(_ADVANCED_LIGO)

#if !defined(USE_GM) && !defined(USE_MX) && !defined(USE_UDP)
#define SHMEM_DAQ 1
#include "../../src/include/daqmap.h"
#include "../../src/include/drv/fb.h"

  // Memory mapped addresses for the DCUs
  volatile unsigned char *dcu_addr[DCU_COUNT];

  // Pointers to IPC areas for each DCU
  struct rmIpcStr *shmemDaqIpc[DCU_COUNT];

  // Pointers into the shared memory for the cycle and time (coming from the IOP (e.g. x00))
  volatile int *ioMemDataCycle;
  volatile int *ioMemDataGPS;
  volatile IO_MEM_DATA *ioMemData;

#endif

  // Pointer to GDS TP tables
  struct cdsDaqNetGdsTpNum *gdsTpNum[2][DCU_COUNT];

#if 0
#ifdef USE_MX
void*
gm_receiver_thread1(void *)
{
  void receiver_mx(int);
  receiver_mx(1);
}
#endif
#endif

#ifdef USE_UDP
static void
diep(char *s) {
  perror(s);
  exit(1);
}

struct daqMXdata {
        struct rmIpcStr mxIpcData;
        cdsDaqNetGdsTpNum mxTpTable;
        char mxDataBlock[DAQ_DCU_BLOCK_SIZE];
};

static const int buf_size = DAQ_DCU_BLOCK_SIZE * 2;

void
receiver_udp(int my_dcu_id) {

      #define BUFLEN 512
      #define PORT 9930
    
     struct sockaddr_in si_me, si_other;
     int s, i;
     char buf[buf_size];
#if 0
     socklen_t slen = sizeof(si_other);
    
     if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1) diep("socket");
    
     memset((char *) &si_me, 0, sizeof(si_me));
     si_me.sin_family = AF_INET;
     si_me.sin_port = htons(PORT);
     si_me.sin_addr.s_addr = htonl(INADDR_ANY);
     if (bind(s, (struct sockaddr *)&si_me, sizeof(si_me))==-1) diep("bind");
#endif

     // Open radio receiver on port 7090
     diag::frameRecv* NDS = new diag::frameRecv(0);
     //NDS->logging(false);
     //if (!NDS->open("225.0.0.1", "192.168.1.0", 7090)) {
     if (!NDS->open("225.0.0.1", "10.12.0.0", 7000 + my_dcu_id)) {
        perror("Multicast receiver open failed.");
        exit(1);
     }
     printf("Multicast receiver opened on port %d\n", 7000 + my_dcu_id);

     for (i=0;;i++) {
	int br;

#if 0
        if ((br = recvfrom(s, buf, buf_size, 0, (struct sockaddr *)&si_other, &slen)) == -1) diep("recvfrom()");
#endif

	unsigned int seq, gps, gps_n;
	char *bufptr = buf;
    	int length = NDS->receive(bufptr, buf_size, &seq, &gps, &gps_n);
    	if (length < 0) {
        	printf("Allocated buffer too small; required %d, size %d\n", -length, buf_size);
        	exit(1);
    	}
    	//printf("%d %d %d %d\n", length, seq, gps, gps_n);


         struct daqMXdata *dataPtr = (struct daqMXdata *) buf;
         int dcu_id = dataPtr->mxIpcData.dcuId;
	 if (dcu_id != my_dcu_id) {

         	printf("Received packet from th foreign dcu_id %d %s:%d bytes=%d, dcu_id=%d\n\n", dcu_id, inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), br, dcu_id);
		continue;
	 }
#if 0
         printf("Received packet from %s:%d bytes=%d, dcu_id=%d\n\n", 
			inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), br, dcu_id);
	
	continue;
#endif

         int cycle = dataPtr->mxIpcData.cycle;
         int len = dataPtr->mxIpcData.dataBlockSize;

         char *dataSource = (char *)dataPtr->mxDataBlock;
         char *dataDest = (char *)((char *)(directed_receive_buffer[dcu_id]) + buf_size * cycle);

         // Move the block data into the buffer
         memcpy (dataDest, dataSource, len);

         // Assign IPC data
         gmDaqIpc[dcu_id].crc = dataPtr->mxIpcData.crc;
         gmDaqIpc[dcu_id].dcuId = dataPtr->mxIpcData.dcuId;
         gmDaqIpc[dcu_id].bp[cycle].timeSec = dataPtr->mxIpcData.bp[cycle].timeSec;
         gmDaqIpc[dcu_id].bp[cycle].crc = dataPtr->mxIpcData.bp[cycle].crc;
         gmDaqIpc[dcu_id].bp[cycle].cycle = dataPtr->mxIpcData.bp[cycle].cycle;
         gmDaqIpc[dcu_id].dataBlockSize = dataPtr->mxIpcData.dataBlockSize;

         // Assign test points table
         *gdsTpNum[0][dcu_id] = dataPtr->mxTpTable;

         gmDaqIpc[dcu_id].cycle = cycle;
         if (daqd.controller_dcu == dcu_id)  {
              controller_cycle = cycle;
              DEBUG(6, printf("Timing dcu=%d cycle=%d\n", dcu_id, controller_cycle));
         }
    }
    close(s);
}
#endif

void*
gm_receiver_thread(void *p)
{
#if defined(USE_GM) || defined(USE_MX) || defined(USE_BROADCAST) || defined(USE_UDP)
  long dcu_id = (long)p;
#ifdef USE_GM
  gm_recv();
#elif defined(USE_MX)
  void receiver_mx(int);
  receiver_mx(0);
#elif defined(USE_UDP)
  void receiver_udp(int);
  receiver_udp(dcu_id);
#endif

#else

  int fd;

  // Open and map all "Myrinet" DCUs
  for (int j = DCU_ID_EDCU; j < DCU_COUNT; j++) {
    if (daqd.dcuSize[0][j] == 0) continue; // skip unconfigured DCU nodes
    if (IS_MYRINET_DCU(j)) {
      std::string s(daqd.fullDcuName[j]);
      std::transform (s.begin(),s.end(), s.begin(), ToLower()); 
 #if 0
      s = "/rtl_mem_" + s + "_daq";
      if ((fd = open(s.c_str(), O_RDWR))<0) {
        system_log(1, "Couldn't open `%s' read/write\n", s.c_str());
        exit(1);
      }
      dcu_addr[j] = (unsigned char *)mmap(0, 64*1024*1024-5000, PROT_READ | PROT_WRITE, MAP_SHARED, fd
, 0);
      if (dcu_addr[j] == MAP_FAILED) {
        system_log(1, "Couldn't mmap `%s'; errno=%d\n", s.c_str(), errno);
        exit(1);
      }
#endif
      s = s + "_daq";
      dcu_addr[j] = (volatile unsigned char *)findSharedMemory((char *)s.c_str());
      if (dcu_addr[j] == 0) {
              system_log(1, "Couldn't mmap `%s'; errno=%d\n", s.c_str(), errno);
              exit(1);
      }
      system_log(1, "Opened %s\n", s.c_str());
      shmemDaqIpc[j] = (struct rmIpcStr *)(dcu_addr[j] + CDS_DAQ_NET_IPC_OFFSET);
      gdsTpNum[0][j] = (struct cdsDaqNetGdsTpNum *)(dcu_addr[j] + CDS_DAQ_NET_GDS_TP_TABLE_OFFSET);
    } else {
      gdsTpNum[0][j] = 0;
    }
  }

  // Open the IPC shared memory
#if 0
  if ((fd = open("/rtl_mem_ipc", O_RDWR)) < 0) {
        system_log(1, "Couldn't open /rtl_mem_ipc read/write\n");
        exit(1);
  }
  void *ptr = mmap(0, 64*1024*1024-5000, PROT_READ | PROT_WRITE, MAP_SHARED, fd , 0);
  if (ptr == MAP_FAILED) {
     system_log(1, "Couldn't mmap /rtl_mem_ipc; errno=%d\n", errno);
     exit(1);
  }
#endif

  volatile void *ptr = findSharedMemory("ipc");
  if (ptr == 0) {
     system_log(1, "Couldn't open shared memory IPC area");
     exit(1);
  }
  system_log(1, "Opened shared memory ipc area\n");
  ioMemData = (volatile IO_MEM_DATA *)(((char *)ptr) + IO_MEM_DATA_OFFSET);

  CDS_HARDWARE cdsPciModules;

  // Find the first ADC card
  // Master will map ADC cards first, then DAC and finally DIO
  printf("Total PCI cards from the master: %d\n", ioMemData -> totalCards);
  for (int ii = 0; ii < ioMemData -> totalCards; ii++) {
    printf("Model %d = %d\n",ii,ioMemData->model[ii]);
    switch (ioMemData -> model [ii]) {
      case GSC_16AI64SSA:
         printf("Found ADC at %d\n", ioMemData -> ipc[ii]);
         cdsPciModules.adcType[0] = GSC_16AI64SSA;
         cdsPciModules.adcConfig[0] = ioMemData->ipc[ii];
         cdsPciModules.adcCount = 1;
         break;
      }
    }
   if (!cdsPciModules.adcCount) {
     printf("No ADC cards found - exiting\n");
     exit(1);
   }

   int ll = cdsPciModules.adcConfig[0];
   ioMemDataCycle = &ioMemData->iodata[ll][0].cycle;
   printf("ioMem Cycle from %d\n", ll);
   ioMemDataGPS = &ioMemData->gpsSecond;
#endif
}

void *
producer::frame_writer ()
{
   daqd_c::realtime("Producer", 1);
   unsigned char *read_dest;
   circ_buffer_block_prop_t prop;
   pthread_t gm_tid;
#ifdef USE_SYMMETRICOM
   unsigned long prev_gps, prev_frac;
   unsigned long gps, frac;
#endif

 if (!daqd.no_myrinet) {

#if !defined(sun) && defined(_ADVANCED_LIGO) 
#if defined(USE_GM)
   int res = gm_setup();
   if (res != 0) {
     system_log(1, "couldn't setup Myrinet\n");
     exit (1);
   }
#elif defined(USE_MX)
   void open_mx(void);
   open_mx();
#endif
#endif

#if defined(USE_MX) || defined(USE_UDP)
        // Allocate receive buffers for each configured DCU
        for (int i = 5; i < DCU_COUNT; i++) {
                if (0 == daqd.dcuSize[0][i]) continue;

                directed_receive_buffer[i] = malloc(2*DAQ_DCU_BLOCK_SIZE*DAQ_NUM_DATA_BLOCKS);
                if (directed_receive_buffer[i] == 0) {
                        system_log (1, "[MX recv] Couldn't allocate recv buffer\n");
                        exit(1);
                }
        }
#endif

#if defined(USE_GM) || defined(USE_MX) || defined(USE_BROADCAST) || defined(USE_UDP)
  // Allocate local test point tables
  static struct cdsDaqNetGdsTpNum gds_tp_table[2][DCU_COUNT];

for (int ifo = 0; ifo < daqd.data_feeds; ifo++) {
  for (int j = DCU_ID_EDCU; j < DCU_COUNT; j++) {
    if (daqd.dcuSize[ifo][j] == 0) continue; // skip unconfigured DCU nodes
    if (IS_MYRINET_DCU(j)) {
      gdsTpNum[ifo][j] = gds_tp_table[ifo] + j;
#if defined(USE_BROADCAST)
    } else if (IS_TP_DCU(j)) {
      gdsTpNum[ifo][j] = gds_tp_table[ifo] + j;
#endif
    } else {
      gdsTpNum[ifo][j] = 0;
    }
  }
}
#endif

#ifdef USE_UDP
   {
     pthread_attr_t attr;
     pthread_attr_init (&attr);
     pthread_attr_setstacksize (&attr, daqd.thread_stack_size);
     pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
     //  pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
     int err_no;
  
     for (int dcu_id = DCU_ID_EDCU; dcu_id < DCU_COUNT; dcu_id++) {
       int ifo = 0;
       if (daqd.dcuSize[ifo][dcu_id] == 0) continue; // skip unconfigured DCU nodes
       if (!IS_MYRINET_DCU(dcu_id)) continue;

       if (err_no = pthread_create (&gm_tid, &attr,
				  gm_receiver_thread, (void *)dcu_id)) {
     	  pthread_attr_destroy (&attr);
     	  system_log(1, "pthread_create() err=%d", err_no);
	  exit(1);
       }
       system_log(1, "UDP receiver thread started for dcu %d", dcu_id);
     }
     pthread_attr_destroy (&attr);
   }
#else
   {
     pthread_attr_t attr;
     pthread_attr_init (&attr);
     pthread_attr_setstacksize (&attr, daqd.thread_stack_size);
     pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
     //  pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
     int err_no;
  
     if (err_no = pthread_create (&gm_tid, &attr,
				  gm_receiver_thread, 0)) {
     	pthread_attr_destroy (&attr);
     	system_log(1, "pthread_create() err=%d", err_no);
	exit(1);
     }
     pthread_attr_destroy (&attr);
#ifdef USE_GM
     system_log(1, "Myrinet receiver thread started");
#elif defined(USE_MX)
     system_log(1, "MX receiver thread started");
#elif defined(USE_UDP)
     system_log(1, "UDP receiver thread started");
#endif
   }
#endif

#if 0
#ifdef USE_MX
   {
     pthread_attr_t attr;
     pthread_attr_init (&attr);
     pthread_attr_setstacksize (&attr, daqd.thread_stack_size);
     pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
     //  pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
     int err_no;

     if (err_no = pthread_create (&gm_tid, &attr,
                                  gm_receiver_thread1, 0)) {
        pthread_attr_destroy (&attr);
        system_log(1, "pthread_create() err=%d", err_no);
        exit(1);
     }
     pthread_attr_destroy (&attr);
     system_log(1, "second MX receiver thread started");
   }
#endif
#endif

   sleep(1);

  }

#ifdef USE_BROADCAST
  diag::frameRecv* NDS = new diag::frameRecv(0);
  if (!NDS->open("225.0.0.1", "10.110.144.0", net_writer_c::concentrator_broadcast_port)) {
        perror("Multicast receiver open failed.");
        exit(1);
  }
#ifdef GDS_TESTPOINTS
  diag::frameRecv* NDS_TP = new diag::frameRecv(0);
  if (!NDS_TP->open("225.0.0.1", "10.110.144.0", net_writer_c::concentrator_broadcast_port_tp)) {
        perror("Multicast receiver open failed 1.");
        exit(1);
  }
#endif

  char *bufptr = (char *)daqd.move_buf - 2048;
  int buflen = daqd.block_size / DAQ_NUM_DATA_BLOCKS_PER_SECOND;
  buflen += 1024*100 + 2048; // Extra overhead for the headers
  if (buflen < 64000) buflen = 64000;
  unsigned int seq, gps, gps_n;
  printf("Opened broadcaster receiver\n");
  gps_n = 1;

  static const int tpbuflen = 10*1024*1024;
  static char tpbuf[tpbuflen];
  char *tpbufptr = tpbuf;

  // Wait until start of a second
  while (gps_n) {
    int length = NDS->receive(bufptr, buflen, &seq, &gps, &gps_n);
    if (length < 0) {
	printf("Allocated buffer too small; required %d, size %d\n", -length, buflen);
	exit(1);
    }
    printf("%d %d %d %d\n", length, seq, gps, gps_n);

#ifdef GDS_TESTPOINTS

    unsigned int tp_seq, tp_gps, tp_gps_n;
    int length_tp = NDS_TP->receive(tpbufptr, tpbuflen, &tp_seq, &tp_gps, &tp_gps_n);
    printf("%d %d %d %d\n", length_tp, tp_seq, tp_gps, tp_gps_n);
#endif
  }
  prop.gps = gps-1;
  prop.gps_n = (1000000000/16) * 15;
#endif

// No waiting here if compiled as broadcasts receiver
#ifndef USE_BROADCAST
static const int cycle_delay = 4;
   // Wait until a second boundary
   {
#if 0
      struct timeb timb;
      timb.millitm = 1;
      while (timb.millitm != 0) {
        ftime(&timb);
      }
#endif

      if ((daqd.dcu_status_check & 4) == 0) {
#ifdef USE_SYMMETRICOM
	if (daqd.symm_ok() == 0) {
		printf("The Symmetricon IRIG-B timing card is not synchronized\n");
		//exit(10);
	}
	unsigned long f;
	const unsigned int c = 1000000000/16;
	// Wait for the beginning of a second
	for(;;) {
		prev_gps = daqd.symm_gps(&f);
#ifdef USE_IOP
	 	//prev_frac = 1000000000 - 1000000000/16;
		prev_frac = 0;
		// Starting at this time
		gps = prev_gps + 1;
		frac = 0;
		if (f > 990000000) break;
#else
		gps = prev_gps;
		//if (f > 999493000) break;
		if (f < ((cycle_delay+2) * c)  && f > ((cycle_delay+1) * c)) break; // Three cycles after a second
#endif

	        struct timespec wait = {0, 10000000UL }; // 10 milliseconds
         	nanosleep (&wait, NULL);

 	}
#if 0
	// Wait until 2 cycles elapse
	for (;;) {
		gps = daqd.symm_gps(&f);
		if (f >= 1000000000/8) break;
	}
#endif
	prev_gps = gps;
	prev_frac = c * cycle_delay;
	frac = c * (cycle_delay+1);
        printf("Starting at gps %d prev_gps %d frac %d f %d\n", gps, prev_gps, frac, f);
        controller_cycle = 1;
#else
      system_log(1, "Waiting for DCU %d to show Up", daqd.controller_dcu);
#if defined(USE_GM) || defined(USE_MX) || defined(USE_UDP)
      //gmDaqIpc[daqd.controller_dcu].cycle = 0;
      controller_cycle = 0;
      //for (int i = 0; 2 != (gmDaqIpc[daqd.controller_dcu].cycle % 16); i++)
      for (int i = 0; 1 != (controller_cycle % 16); i++)
#else
      shmemDaqIpc[daqd.controller_dcu]->cycle = 1;
      for (int i = 0; 1 != (shmemDaqIpc[daqd.controller_dcu]->cycle % 16); i++)
#endif
       {
         struct timespec tspec = {0,1000000000/16/100}; // seconds, nanoseconds
         nanosleep (&tspec, NULL);
	 if (i > 1600*4 && !daqd.avoid_reconnect) {
		/* OK the front end isn't there, let's get going on our own */
		system_log(1, "Looks like dcu %d is not there. Running on my own.", daqd.controller_dcu);
		//exit(1);
		break;
	 }
      }
      //controller_cycle = 1;
      system_log(1, "Detected controller DCU %d\n", daqd.controller_dcu);
#endif
      }
   }
#endif

#if EPICS_EDCU == 1
   extern unsigned int pvValue[1000];
   pvValue[5] = 0;
   pvValue[17] = 0;
#endif
#ifndef USE_SYMMETRICOM
   time_t zero_time = time(0);//  - 315964819 + 33;
#endif
   int prev_controller_cycle = -1;
   int dcu_cycle = 0;
   int resync = 0;

   if (daqd.dcu_status_check & 4) resync = 1;

   for (unsigned long i = 0;;i++) { // timing
#ifdef USE_SYMMETRICON
     DEBUG(6, printf("Timing %d gps=%d frac=%d\n", i, gps, frac));
#endif
#ifndef USE_BROADCAST
     read_dest = daqd.move_buf;
     for (int j = DCU_ID_EDCU; j < DCU_COUNT; j++) {
      //printf("DCU %d is %d bytes long\n", j, daqd.dcuSize[0][j]);
      if (daqd.dcuSize[0][j] == 0) continue; // skip unconfigured DCU nodes
      long read_size = daqd.dcuDAQsize[0][j];
      if (IS_EPICS_DCU(j)) {
#if EPICS_EDCU
	memcpy((void *)read_dest, (char *)(daqd.edcu1.channel_value + daqd.edcu1.fidx), read_size);
	daqd.dcuStatus[0][j] = 0;
#endif
        read_dest += read_size;
      } else if (IS_MYRINET_DCU(j)) {
	dcu_cycle = i%DAQ_NUM_DATA_BLOCKS;
#if defined(USE_GM) || defined(USE_MX) || defined(USE_UDP)
   	//dcu_cycle = gmDaqIpc[j].cycle;
	//printf("cycl=%d ctrl=%d dcu=%d\n", gmDaqIpc[j].cycle, controller_cycle, j);
	// Get the data from myrinet
	memcpy((void *)read_dest,
	       ((char *)directed_receive_buffer[j]) + dcu_cycle*2*DAQ_DCU_BLOCK_SIZE,
	       2*DAQ_DCU_BLOCK_SIZE);
#else
   	//dcu_cycle = shmemDaqIpc[j]->cycle;
	// Get the data from myrinet
	unsigned char *read_src = (unsigned char *)(dcu_addr[j] + CDS_DAQ_NET_DATA_OFFSET);
	memcpy((void *)read_dest,
	        read_src + dcu_cycle*2*DAQ_DCU_BLOCK_SIZE,
		2*DAQ_DCU_BLOCK_SIZE);
#endif
#if 0
	memcpy((void *)read_dest,
	       ((char *)directed_receive_buffer[j]) + dcu_cycle*2*DAQ_DCU_BLOCK_SIZE,
	       read_size);
	// Copy the rest of the block to testpoints buffer
        fprintf(stderr, "memcpy(dest=0x%x, src=0x%x, size=0x%x)\n", 
		(void *)(read_dest + read_size),                ((char *)directed_receive_buffer[j]) + dcu_cycle*2*DAQ_DCU_BLOCK_SIZE + read_size, 2*DAQ_DCU_BLOCK_SIZE - read_size);	
#endif

#if 0
	memcpy((void *)(read_dest + read_size),
	       ((char *)directed_receive_buffer[j]) + dcu_cycle*2*DAQ_DCU_BLOCK_SIZE + read_size, 2*DAQ_DCU_BLOCK_SIZE - read_size);
#endif

      volatile struct rmIpcStr *ipc;
#if defined(USE_GM) || defined(USE_MX) || defined(USE_UDP)
      ipc = &gmDaqIpc[j];
#else
      ipc = shmemDaqIpc[j];
#endif

      int cblk1 = (i+1)%DAQ_NUM_DATA_BLOCKS;
      static const int ifo = 0; // For now

      // Calculate DCU status, if needed
      if (daqd.dcu_status_check & (1 << ifo)) {
	if (cblk1 % 16 == 0) {
	  /* DCU checking mask (Which DCUs to check for SYNC fault) */
	  unsigned int dcm = 0xfffffff0;

	  int lastStatus = dcuStatus[ifo][j];
	  dcuStatus[ifo][j] = DAQ_STATE_FAULT;
	
	  /* Check if DCU running at all */
          if ( 1 /*dcm & (1 << j)*/) {
	    if (dcuStatCycle[ifo][j] == 0) dcuStatus[ifo][j] = DAQ_STATE_SYNC_ERR;
	    else dcuStatus[ifo][j] = DAQ_STATE_RUN;
	  }

#ifndef NDEBUG
	if (_debug) {
		// dcuCycleStatus shows how many matches of cycle number we got
		printf("dcuid=%d; dcuCycleStatus=%d; dcuStatCycle=%d\n", j, dcuCycleStatus[ifo][j], dcuStatCycle[ifo][j]);
	}
#endif
	  /* Check if DCU running and in sync */
	  if ((dcuCycleStatus[ifo][j] > 3 || j < 5) && dcuStatCycle[ifo][j] > 4)
	  {
 	    dcuStatus[ifo][j] = DAQ_STATE_RUN;
	  }

	  if (/* (lastStatus == DAQ_STATE_RUN) && */ (dcuStatus[ifo][j] != DAQ_STATE_RUN)) {
#ifndef NDEBUG
	    	  if (_debug > 0)
	    	    printf("Lost %s (ifo %d; dcu %d); status %d %d\n", daqd.dcuName[j], ifo, j, dcuCycleStatus[ifo][j], dcuStatCycle[ifo][j]);
#endif
	    ipc->status = DAQ_STATE_FAULT;
	  }

	  if ((dcuStatus[ifo][j] == DAQ_STATE_RUN) /* && (lastStatus != DAQ_STATE_RUN) */) {
#ifndef NDEBUG
	    	  if (_debug > 0)
	    	    printf("New %s (dcu %d)\n", daqd.dcuName[j], j);
#endif
	    ipc->status = DAQ_STATE_RUN;
	  }

	  dcuCycleStatus[ifo][j] = 0;
	  dcuStatCycle[ifo][j] = 0;
	  ipc->status = ipc->status; 
	}

	{
	  int intCycle = ipc->cycle % DAQ_NUM_DATA_BLOCKS;
	  if (intCycle != dcuLastCycle[ifo][j]) dcuStatCycle[ifo][j] ++;
	  dcuLastCycle[ifo][j] = intCycle;
	}
      }

      // Update DCU status
      int newStatus = ipc -> status != DAQ_STATE_RUN? 0xbad: 0;

#if 0
	// out of sync
	if (dcu_cycle != controller_cycle && ((dcu_cycle + 1) % 16) != controller_cycle) {
		newStatus = 1;
	}
#endif
#if defined(USE_GM) || defined(USE_MX) || defined(USE_UDP)
        int newCrc = gmDaqIpc[j].crc;
#else
	int newCrc = shmemDaqIpc[j]->crc;
#endif
	//printf("%x\n", *((int *)read_dest));
        if (!IS_EXC_DCU(j)) {
	  if (newCrc != daqd.dcuConfigCRC[0][j]) newStatus |= 0x2000;
        }
	if (newStatus != daqd.dcuStatus[0][j]) {
	  //system_log(1, "DCU %d IFO %d (%s) %s", j, 0, daqd.dcuName[j], newStatus? "fault": "running");
	  if (newStatus & 0x2000) {
	    //system_log(1, "DCU %d IFO %d (%s) reconfigured (crc 0x%x rfm 0x%x)", j, 0, daqd.dcuName[j], daqd.dcuConfigCRC[0][j], newCrc);
	  }
        }
        daqd.dcuStatus[0][j] = newStatus;
#if defined(USE_GM) || defined(USE_MX) || defined(USE_UDP)
        daqd.dcuCycle[0][j] = gmDaqIpc[j].cycle;
#else
        daqd.dcuCycle[0][j] = shmemDaqIpc[j]-> cycle;
#endif

	/* Check DCU data checksum */
      	unsigned long crc = 0;
      	unsigned long bytes = read_size;
	unsigned char *cp = (unsigned char *)read_dest;
	while (bytes--) {
	  crc = (crc << 8) ^ crctab[((crc >> 24) ^ *(cp++)) & 0xFF];
        }
        bytes = read_size;
        while (bytes > 0) {
	  crc = (crc << 8) ^ crctab[((crc >> 24) ^ bytes) & 0xFF];
	  bytes >>= 8;
        }
        crc = ~crc & 0xFFFFFFFF;
	int cblk = i % 16;
        // Reset CRC/second variable for this DCU
        if (cblk == 0) {
	  daqd.dcuCrcErrCntPerSecond[0][j] = daqd.dcuCrcErrCntPerSecondRunning[0][j];
	  daqd.dcuCrcErrCntPerSecondRunning[0][j] = 0;
        }

        if (j >= DCU_ID_ADCU_1 && (!IS_TP_DCU(j)) && daqd.dcuStatus[0][j] == 0)
	{
#if defined(USE_GM) || defined(USE_MX) || defined(USE_UDP)
	  unsigned int rfm_crc = gmDaqIpc[j].bp[cblk].crc;
	  unsigned int dcu_gps = gmDaqIpc[j].bp[cblk].timeSec;
#else
	  unsigned int rfm_crc = shmemDaqIpc[j]->bp[cblk].crc;
	  unsigned int dcu_gps = shmemDaqIpc[j]->bp[cblk].timeSec;
	  shmemDaqIpc[j]->bp[cblk].crc = 0;
#endif

#ifdef USE_SYMMETRICOM

	  //system_log(5, "dcu %d block %d cycle %d  gps %d symm %d\n", j, cblk, gmDaqIpc[j].bp[cblk].cycle,  dcu_gps, gps);
	  unsigned long mygps = gps;
	  if (cblk > (15 - cycle_delay)) mygps--;
	  if (dcu_gps != mygps) {
	    daqd.dcuStatus[0][j] |= 0x4000;
	    daqd.dcuCrcErrCnt[0][j]++;
	    daqd.dcuCrcErrCntPerSecondRunning[0][j]++;
	  }
#endif

	  if (rfm_crc != crc) {
	    system_log(5, "MISS dcu %d (%s); crc[%d]=%x; computed crc=%x\n",
		       j, daqd.dcuName[j], cblk, rfm_crc, crc);

	    /* Set DCU status to BAD, all data will be marked as BAD 
	       because of the CRC mismatch */
	    daqd.dcuStatus[0][j] |= 0x1000;
	    daqd.dcuCrcErrCnt[0][j]++;
	    daqd.dcuCrcErrCntPerSecondRunning[0][j]++;
	  } else {
	    system_log(6, " MATCH dcu %d (%s); crc[%d]=%x; computed crc=%x\n",
		       j, daqd.dcuName[j], cblk, rfm_crc, crc);
	  }
        }

        read_dest += 2*DAQ_DCU_BLOCK_SIZE ;
      }
     }

	int cblk = i % 16;
#ifdef DATA_CONCENTRATOR
	// Assign per-DCU data we need to broadcast out
	//
	for (int ifo = 0; ifo < daqd.data_feeds; ifo++) {
          for (int j = DCU_ID_EDCU; j < DCU_COUNT; j++) {
                if (IS_TP_DCU(j)) continue;     // Skip TP and EXC DCUs
                if (daqd.dcuSize[ifo][j] == 0) continue; // Skip unconfigured DCUs
  		prop.dcu_data[j + ifo*DCU_COUNT].cycle = daqd.dcuCycle[ifo][j];
		volatile struct rmIpcStr *ipc = daqd.dcuIpc[ifo][j];

		// Do not support Myrinet DCUs on H2
      		if (IS_MYRINET_DCU(j) && ifo == 0) {
#if defined(USE_GM) || defined(USE_MX) || defined(USE_UDP)
		  prop.dcu_data[j].crc = gmDaqIpc[j].bp[cblk].crc;
#else
		  prop.dcu_data[j].crc = shmemDaqIpc[j]->bp[cblk].crc;
#endif
		  //printf("dcu %d crc=0x%x\n", j, prop.dcu_data[j].crc);
  		  prop.dcu_data[j].status = daqd.dcuStatus[0 /* IFO */][j];
      		} else
		// EDCU is "attached" to H1, not H2
		if (j == DCU_ID_EDCU && ifo == 0) {
			// See if the EDCU thread is running and assign status
			if (0x0 == (prop.dcu_data[j].status = daqd.edcu1.running? 0x0: 0xbad)) {
				// If running calculate the CRC
				
	//memcpy(read_dest, (char *)(daqd.edcu1.channel_value + daqd.edcu1.fidx), daqd.dcuSize[ifo][j]);
			
			  unsigned int bytes = daqd.dcuSize[0][DCU_ID_EDCU];
			  unsigned char *cp = daqd.move_buf; // The EDCU data is in front
			  unsigned long crc = 0;
      			  while (bytes--) {
        		    crc = (crc << 8) ^ crctab[((crc >> 24) ^ *(cp++)) & 0xFF];
      			  }
      			  bytes = daqd.dcuDAQsize[0][DCU_ID_EDCU];
      			  while (bytes > 0) {
			    crc = (crc << 8) ^ crctab[((crc >> 24) ^ bytes) & 0xFF];
			    bytes >>= 8;
      			  }
      			  crc = ~crc & 0xFFFFFFFF;
  			  prop.dcu_data[j].crc = crc;
			}
		} else {
  			prop.dcu_data[j + ifo*DCU_COUNT].crc = ipc -> bp[cblk].crc;
  			prop.dcu_data[j + ifo*DCU_COUNT].status = daqd.dcuStatus[ifo][j];
		}
	  }
	}

#endif
     //prop.gps = time(0) - 315964819 + 33;
#ifdef USE_SYMMETRICOM
     prop.gps = gps;
     if (cblk > (15 - cycle_delay)) prop.gps--;
#else
     prop.gps = i/16 + zero_time - 315964819 + 33;
#endif
     prop.gps_n = 1000000000/16 * (i % 16);
     //printf("before put %d %d %d\n", prop.gps, prop.gps_n, frac);
#else // USE_BROADCAST defined
     if (((gps == prop.gps) && gps_n != prop.gps_n + 1000000000/16)
	 ||((gps == prop.gps + 1) && (gps_n != 0 || prop.gps_n != (1000000000/16)*15))
	 ||(gps > prop.gps + 1)) {
	fprintf(stderr, "Dropped broadcast block(s); gps now = %d, %d; was = %d, %d\n", gps, gps_n, prop.gps, prop.gps_n);
	exit(1);
     }

#ifdef GDS_TESTPOINTS
     // Update testpoints data in the main buffer
     daqd.gds.update_tp_data((unsigned int *)tpbufptr, (char *)daqd.move_buf);
#endif

     // Parse received broadcast transmission header and
     // check config file CRCs and data CRCs, check DCU size and number
     // Assign DCU status and cycle.
     unsigned int *header = (unsigned int *)(((char *)daqd.move_buf) - 2048);
     int ndcu = ntohl(*header++);
     //printf("ndcu = %d\n", ndcu);
     if (ndcu > 0 && ndcu < 50) {
     int data_offs = 0; // Offset to the current DCU data
     for (int j = 0; j < ndcu; j++) {
	unsigned int dcu_number;
        unsigned int dcu_size; // Data size for this DCU
        unsigned int config_crc; // Configuration file CRC
        unsigned int dcu_crc; // Data CRC
        unsigned int status; // DCU status word bits (0-ok, 0xbad-out of sync, 0x1000-trasm error
                        // 0x2000 - configuration mismatch).
        unsigned int cycle;   // DCU cycle
	dcu_number = ntohl(*header++);
	dcu_size = ntohl(*header++);
	config_crc = ntohl(*header++);
	dcu_crc = ntohl(*header++);
	status = ntohl(*header++);
	cycle = ntohl(*header++);
	int ifo = 0;
 	if (dcu_number > DCU_COUNT) {
		ifo = 1;
		dcu_number -= DCU_COUNT;
	}
	//printf("dcu=%d size=%d config_crc=0x%x crc=0x%x status=0x%x cycle=%d\n",
		//dcu_number, dcu_size, config_crc, dcu_crc, status, cycle);
	if (daqd.dcuSize[ifo][dcu_number]) { // Don't do anything if this DCU is not configured
		daqd.dcuStatus[ifo][dcu_number] = status;
		daqd.dcuCycle[ifo][dcu_number] = cycle;
		if (status == 0) { // If the DCU status is OK from the concentrator
			// Check for local configuration and data mismatch
			if (config_crc != daqd.dcuConfigCRC[ifo][dcu_number]) {
				// Detected local configuration mismach
				daqd.dcuStatus[ifo][dcu_number] |= 0x2000;
			}
			unsigned char *cp = daqd.move_buf + data_offs; // Start of data
			unsigned int bytes = dcu_size; // DCU data size
      			unsigned int crc = 0;
			// Calculate DCU data CRC
      			while (bytes--) {
        			crc = (crc << 8) ^ crctab[((crc >> 24) ^ *(cp++)) & 0xFF];
      			}
        		bytes = dcu_size;
        		while (bytes > 0) {
	  			crc = (crc << 8) ^ crctab[((crc >> 24) ^ bytes) & 0xFF];
	  			bytes >>= 8;
        		}
        		crc = ~crc & 0xFFFFFFFF;
			if (crc != dcu_crc) {
				// Detected data corruption !!!
				daqd.dcuStatus[ifo][dcu_number] |= 0x1000;
				DEBUG1(printf("ifo=%d dcu=%d calc_crc=0x%x data_crc=0x%x\n", ifo, dcu_number, crc, dcu_crc));
			}
		}
	}
	data_offs += dcu_size;
     }
     }

// :TODO: make sure all DCUs configuration matches; restart when the mismatch detected

#if 0
     for (int j = DCU_ID_EDCU; j < DCU_COUNT; j++) {
      if (daqd.dcuSize[0][j]) {
        daqd.dcuStatus[0][j] = 0;
        daqd.dcuCycle[0][j] = i%16;
      }
     }
#endif

     prop.gps = gps;
     prop.gps_n = gps_n;
#endif
     prop.leap_seconds = daqd.gps_leap_seconds(prop.gps);
     int nbi = daqd.b1 -> put16th_dpscattered (daqd.vmic_pv, daqd.vmic_pv_len, &prop);
   //  printf("%d %d\n", prop.gps, prop.gps_n);
     //DEBUG1(cerr << "producer " << i << endl);
#if EPICS_EDCU == 1
     pvValue[0] = i;
     pvValue[17] = prop.gps;
	//DEBUG1(cerr << "gps=" << pvValue[17] << endl);
     if (i % 16 == 0) {
     	  // Count how many seconds we were acquiring data
	  pvValue[5]++;
#ifndef NO_BROADCAST
	  {
		extern unsigned long dmt_retransmit_count;
		extern unsigned long dmt_failed_retransmit_count;
		// Display DMT retransmit channels every second
		pvValue[15] = dmt_retransmit_count;
		pvValue[16] = dmt_failed_retransmit_count;
		dmt_retransmit_count = 0;
		dmt_failed_retransmit_count = 0;
	  }
#endif
     }
#endif

#ifdef USE_BROADCAST
     // Header takes first 2048 bytes
for(;;) {
     int old_seq = seq;
     int length = NDS->receive(bufptr, buflen, &seq, &gps, &gps_n);
     //DEBUG1(printf("%d %d %d %d\n", length, seq, gps, gps_n));
     // Strangely we receiver duplicate blocks on solaris for some reason
     // Looks like this happens when the data is lost...
     if (seq == old_seq) {
	printf("received duplicate NDS DAQ broadcast sequence %d; prevpg = %d %d; gps=%d %d; length = %d\n", seq, prop.gps, prop.gps_n, gps, gps_n, length);
     } else break;
}

#ifdef GDS_TESTPOINTS
     // TODO: check on the continuity of the sequence and GPS time here
     unsigned int tp_seq, tp_gps, tp_gps_n;
     do {
       int tp_length = NDS_TP->receive(tpbufptr, tpbuflen, &tp_seq, &tp_gps, &tp_gps_n);
       //DEBUG1(printf("TP %d %d %d %d\n", tp_length, tp_seq, tp_gps, tp_gps_n));
       //gps = tp_gps; gps_n = tp_gps_n;
     } while (tp_gps < gps || (tp_gps == gps && tp_gps_n < gps_n));
     if (tp_gps != gps || tp_gps_n != gps_n) {
	fprintf(stderr, "Invalid broadcast received; seq=%d tp_seq=%d gps=%d tp_gps=%d gps_n=%d tp_gps_n=%d\n", seq, tp_seq, gps, tp_gps, gps_n, tp_gps_n);
	exit(1);
     }
#endif
#else
#ifdef USE_SYMMETRICOM
	//printf("gps=%d  prev_gps=%d bfrac=%d prev_frac=%d\n", gps, prev_gps, frac, prev_frac);
       const int polls_per_sec = 1600;
       for (int ntries = 0;; ntries++) {
         struct timespec tspec = {0,1000000000/polls_per_sec}; // seconds, nanoseconds
         nanosleep (&tspec, NULL);

	 gps = daqd.symm_gps(&frac);
	 if (prev_frac == 937500000) { 
		if (gps == prev_gps + 1) {
		  frac = 0;
		  break;
		} else {
		   if (gps > prev_gps + 1) {
			fprintf(stderr, "GPS card time jumped from %d to %d\n", prev_gps, gps);
			exit(1);
		   } else if (gps < prev_gps) {
			fprintf(stderr, "GPS card time moved back from %d to %d\n", prev_gps, gps);
			exit(1);
		   }
		}
	 } else if (frac >= prev_frac + 62500000) {
		frac = prev_frac + 62500000;
		// Check if GPS seconds moved for some reason (because of delay)
		if (gps != prev_gps) {
		        fprintf(stderr, "GPS time jumped from %d to %d\n", prev_gps, gps);
		        exit(1);
		}
		break;
	 }

	 if (ntries >= polls_per_sec) {
#ifdef USE_IOP
		fprintf(stderr, "IOP timeout\n");
#else
		fprintf(stderr, "Symmetricom GPS timeout\n");
#endif

	 	exit(1);
	 }
       }
	//printf("gps=%d prev_gps=%d ifrac=%d prev_frac=%d\n", gps,  prev_gps, frac, prev_frac);
       controller_cycle++;
#else
     if (resync) {
       if (controller_cycle > 0) {
	 printf("acquiring sync source\n");
	 resync = 0;
	 prev_controller_cycle = controller_cycle;
	 int sync_diff = 16 - (i % 16) + dcu_cycle;
	 printf("dcu_cycle = %d\n", dcu_cycle);
	 printf("sync diff = %d\n", sync_diff);
	 if (sync_diff) {
	    if (sync_diff < 0) sync_diff = 16 + sync_diff;
	    for (int j = 0; j < sync_diff; j++) {
     		prop.gps = i/16 + zero_time - 315964819 + 33;
     		prop.gps_n = 1000000000/16 * (i % 16);
     		daqd.b1 -> put16th_dpscattered (daqd.vmic_pv, daqd.vmic_pv_len, &prop);
		i++;
	    }
 	 }
       } else {
         struct timeval tv;
         struct timespec tspec = {0,1000000000/16/100}; // seconds, nanoseconds
         time_t sec = zero_time + i/16;
         time_t usec = 1000000/16 * (i % 16);
         time_t next_usec = 1000000/16 * (i % 16 + 1);
	 if (next_usec == 1000000) next_usec = 0;
         do {
           // Sleep a little while
           nanosleep (&tspec, NULL);
           // Check if time expired
           gettimeofday(&tv, 0);
           if (next_usec == 0) { if (tv.tv_sec > sec) break; }
           else { if (tv.tv_usec >= next_usec) break; }
	   //printf("sec=%d new sec=%d\n", sec, tv.tv_sec);
	   //printf("usec=%d new usec=%d\n", next_usec, tv.tv_usec);
         } while (1);
       }
     }
     if (!resync) {
       for (int j = 0; prev_controller_cycle == controller_cycle; j++) {
         struct timespec tspec = {0,1000000000/16/100}; // seconds, nanoseconds
         nanosleep (&tspec, NULL);
         if (j > 150) {
	   printf("lost sync source\n");
	   resync = 1;
	   controller_cycle = 0;
	   prev_controller_cycle = -1; // so we can break out of the loop
           if (daqd.avoid_reconnect) {
		_exit(0);
	   }
         }
       }
     }
#endif
#endif
     prev_controller_cycle = controller_cycle;
#ifdef USE_SYMMETRICOM
     prev_gps = gps;
     prev_frac = frac;
#endif
   }
}
#else /* _ADVANCED_LIGO is not defined */
#error
void *
producer::frame_writer ()
{
}

#endif

#endif /* if ! defined(VMIC_RFM_PRODUCER) */
