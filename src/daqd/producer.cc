
#define _XOPEN_SOURCE 600
#define _BSD_SOURCE 1
#define _DEFAULT_SOURCE 1

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
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <limits.h>
#include <sys/timeb.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cctype>      // old <ctype.h>
#include <sys/prctl.h>

#ifdef USE_SYMMETRICOM
#ifndef USE_IOP
//#include <bcuser.h>
#include <../drv/symmetricom/symmetricom.h>
#endif
#endif

using namespace std;

#include "circ.hh"
#include "daqd.hh"
#include "sing_list.hh"
#include "drv/cdsHardware.h"
#include <netdb.h>
#include "net_writer.hh"
#include "drv/cdsHardware.h"

#include <sys/ioctl.h>
#include "../drv/rfm.c"

#if EPICS_EDCU == 1
#include "epics_pvs.hh"
#endif

#include "raii.hh"
#include "conv.hh"

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
/// DMA memory area pointers
void *directed_receive_buffer[DCU_COUNT];
int controller_cycle = 0;
#else

#if defined(USE_SYMMETRICOM) || defined(USE_LOCAL_TIME)
int controller_cycle = 0;
#else
/// Point into shared memory for the controller DCU cycle
#define controller_cycle (shmemDaqIpc[daqd.controller_dcu]->cycle)
#endif

#endif

#if !defined(USE_GM) && !defined(USE_MX) && !defined(USE_UDP)
#include "../../src/include/daqmap.h"
#include "../../src/include/drv/fb.h"

  /// Memory mapped addresses for the DCUs
  volatile unsigned char *dcu_addr[DCU_COUNT];

  /// Pointers to IPC areas for each DCU
  struct rmIpcStr *shmemDaqIpc[DCU_COUNT];

  /// Pointers into the shared memory for the cycle and time (coming from the IOP (e.g. x00))
  volatile int *ioMemDataCycle;
  volatile int *ioMemDataGPS;
  volatile IO_MEM_DATA *ioMemData;

#endif

  /// Pointer to GDS TP tables
  struct cdsDaqNetGdsTpNum *gdsTpNum[2][DCU_COUNT];


/// Data receiving thread
void*
gm_receiver_thread(void *this_p)
{
#if defined(USE_GM) || defined(USE_MX) || defined(USE_BROADCAST) || defined(USE_UDP)
#ifdef USE_GM
  gm_recv();
#elif defined(USE_MX)
  void receiver_mx(int);
  int this_eid = *static_cast<int*>(this_p);
  receiver_mx(this_eid);
#elif defined(USE_UDP)
  int this_dcu_id = *static_cast<int*>(this_p);
  void receiver_udp(int);
  receiver_udp(this_dcu_id);
#endif

#else

  int fd;
  // error message buffer
  char errmsgbuf[80]; 
   
  // Open and map all "Myrinet" DCUs
  for (int j = DCU_ID_EDCU; j < DCU_COUNT; j++) {
    if (daqd.dcuSize[0][j] == 0) continue; // skip unconfigured DCU nodes
    if (IS_MYRINET_DCU(j)) {
      std::string s(daqd.fullDcuName[j]);
      std::transform (s.begin(),s.end(), s.begin(), ToLower()); 
      s = s + "_daq";
      dcu_addr[j] = (volatile unsigned char *)findSharedMemory((char *)s.c_str());
      if (dcu_addr[j] == 0) {
              strerror_r(errno, errmsgbuf, sizeof(errmsgbuf));
              system_log(1, "Couldn't mmap `%s'; err = %s\n", s.c_str(), errmsgbuf);
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

/// The main data movement thread (the producer)
void *
producer::frame_writer ()
{
   unsigned char *read_dest;
   circ_buffer_block_prop_t prop;
#if defined(USE_SYMMETRICOM) || defined(USE_LOCAL_TIME)
   unsigned long prev_gps, prev_frac;
   unsigned long gps, frac;
#endif
  // error message buffer
  char errmsgbuf[80]; 
unsigned long stat_cycles = 0;
    stats stat_full, stat_recv, stat_crc, stat_transfer;

// Set thread parameters
   daqd_c::set_thread_priority("Producer","dqprod",PROD_THREAD_PRIORITY,PROD_CPUAFFINITY); 

   unsigned char *move_buf = 0;
   int vmic_pv_len = 0;
   raii::array_ptr<struct put_dpvec> _vmic_pv(new struct put_dpvec[MAX_CHANNELS]);
   struct put_dpvec *vmic_pv = _vmic_pv.get();

   // FIXME: move_buf could leak on errors (but we would probably die anyways.
   daqd.initialize_vmpic(&move_buf, &vmic_pv_len, vmic_pv);
   raii::array_ptr<unsigned char> _move_buf(move_buf);

 if (!daqd.no_myrinet) {

#if defined(USE_GM)
   int res = gm_setup();
   if (res != 0) {
     system_log(1, "couldn't setup Myrinet\n");
     exit (1);
   }
#elif defined(USE_MX)
   unsigned int max_endpoints = open_mx();
   unsigned int nics_available = max_endpoints >> 8;
   max_endpoints &= 0xff;
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
     int my_err_no;
  
     for (int dcu_id = DCU_ID_EDCU; dcu_id < DCU_COUNT; dcu_id++) {
       int ifo = 0;
       if (daqd.dcuSize[ifo][dcu_id] == 0) continue; // skip unconfigured DCU nodes
       if (!IS_MYRINET_DCU(dcu_id)) continue;

       if (my_err_no = pthread_create (&gm_tid, &attr,
				  gm_receiver_thread, &dcu_id)) {
          strerror_r(my_err_no, errmsgbuf, sizeof(errmsgbuf));
     	  pthread_attr_destroy (&attr);
     	  system_log(1, "pthread_create() err=%s", errmsgbuf);
	  exit(1);
       }
       system_log(1, "UDP receiver thread started for dcu %d", dcu_id);
     }
     pthread_attr_destroy (&attr);
   }
#else
   {
     pthread_t gm_tid;
     pthread_attr_t attr;
     pthread_attr_init (&attr);
     pthread_attr_setstacksize (&attr, daqd.thread_stack_size);
     pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
     int my_err_no;
  
     for (int j = 0; j < DCU_COUNT; j++) {
	class stats s;
     	rcvr_stats.push_back(s);
     }
#ifdef USE_MX
/* Create array to hold mx thread card+endpoint data
 We will pass a point to the individual array element.
 This is to avoid a race condition where the 
 gm_receiver_thread gets interleaved values 
 Keith Thorne   2015-07-10 */ 
     int bp_aray[MX_MAX_BOARDS][MX_MAX_ENDPOINTS];
     for (int bnum = 0; bnum < nics_available; bnum++) { // Start
       for (int j = 0; j < max_endpoints; j++) {
         int bp = j;
         bp = bp + (bnum*256);
 /* calculate address within array */  
         bp_aray[bnum][j] = bp;
         void *bpPtr = (int *)(bp_aray + bnum) + j;
         if (my_err_no = pthread_create (&gm_tid, &attr,
                     gm_receiver_thread, bpPtr)) {
                  pthread_attr_destroy (&attr);
                  strerror_r(my_err_no, errmsgbuf, sizeof(errmsgbuf));
                  system_log(1, "pthread_create() err=%s", errmsgbuf);
                  exit(1);
         }
       }
     }
#else
// GM, USE_BROADCAST have a single thread
    if (my_err_no = pthread_create (&gm_tid, &attr,
                     gm_receiver_thread, 0)) {
                  strerror_r(my_err_no, errmsgbuf, sizeof(errmsgbuf));
                  pthread_attr_destroy (&attr);
                  system_log(1, "pthread_create() err=%s", errmsgbuf);
                  exit(1);
    }
#endif
     pthread_attr_destroy (&attr);
   }
#endif

   sleep(1);

  }

 stat_full.sample();
// TODO make IP addresses configurable from daqdrc
#ifdef USE_BROADCAST
 stat_recv.sample();
  diag::frameRecv* NDS = new diag::frameRecv(0);
  if (!NDS->open("225.0.0.1", "10.110.144.0", net_writer_c::concentrator_broadcast_port)) {
        perror("Multicast receiver open failed.");
        exit(1);
  }
  stat_recv.tick();
#ifdef GDS_TESTPOINTS
  diag::frameRecv* NDS_TP = new diag::frameRecv(0);
  if (!NDS_TP->open("225.0.0.1", "10.110.144.0", net_writer_c::concentrator_broadcast_port_tp)) {
        perror("Multicast receiver open failed 1.");
        exit(1);
  }
#endif

  char *bufptr = (char *)move_buf - BROADCAST_HEADER_SIZE;
  int buflen = daqd.block_size / DAQ_NUM_DATA_BLOCKS_PER_SECOND;
  buflen += 1024*100 + BROADCAST_HEADER_SIZE; // Extra overhead for the headers
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
int cycle_delay = daqd.cycle_delay;
   // Wait until a second boundary
   {
      if ((daqd.dcu_status_check & 4) == 0) {
#if defined(USE_SYMMETRICOM) || defined(USE_LOCAL_TIME)
#ifdef USE_SYMMETRICOM
	if (daqd.symm_ok() == 0) {
		printf("The Symmetricom IRIG-B timing card is not synchronized\n");
		//exit(10);
	}
#endif
	unsigned long f;
	const unsigned int c = 1000000000/16;
	// Wait for the beginning of a second
	for(;;) {
#ifdef USE_SYMMETRICOM
		prev_gps = daqd.symm_gps(&f);
#elif defined(USE_LOCAL_TIME)
		struct timeval tv;
		struct timezone tz;
		gettimeofday(&tv, &tz);
		//printf("%d %d \n", tv.tv_sec, tv.tv_usec);
		// This needs cleanup in daqdrc and in symmetricom.c
		tv.tv_sec += - 315964800 - 315964819 + 33 + daqd.symm_gps_offset;
		prev_gps = tv.tv_sec; f = tv.tv_usec * 1000;
#else
#error
#endif
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
	prev_gps = gps;
	prev_frac = c * cycle_delay;
	frac = c * (cycle_delay+1);
        printf("Starting at gps %ld prev_gps %ld frac %ld f %ld\n", gps, prev_gps, frac, f);
        controller_cycle = 1;
#else
      system_log(1, "Waiting for DCU %d to show Up", daqd.controller_dcu);
#if defined(USE_GM) || defined(USE_MX) || defined(USE_UDP)
      //gmDaqIpc[daqd.controller_dcu].cycle = 0;
      controller_cycle = 0;
      //for (int i = 0; 2 != (gmDaqIpc[daqd.controller_dcu].cycle % 16); i++)
      for (int i = 0; 2 != (controller_cycle % 16); i++)
#else
      if (shmemDaqIpc[daqd.controller_dcu] == 0) {
		fprintf(stderr, "DCU %d is not configured\n", daqd.controller_dcu);
		exit(1);
      }
      shmemDaqIpc[daqd.controller_dcu]->cycle = 1;
      for (int i = 0; 1 != (shmemDaqIpc[daqd.controller_dcu]->cycle % 16); i++)
#endif
       {
         struct timespec tspec = {0,1000000000/16/2}; // seconds, nanoseconds
         nanosleep (&tspec, NULL);
	 if (i > 32*10 && !daqd.avoid_reconnect) {
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
   PV::set_pv(PV::PV_UPTIME_SECONDS, 0);
   PV::set_pv(PV::PV_GPS, 0);
#endif
#if !defined(USE_SYMMETRICOM) && !defined(USE_LOCAL_TIME)
   time_t zero_time = time(0);//  - 315964819 + 33;
#endif
   int prev_controller_cycle = -1;
   int dcu_cycle = 0;
   int resync = 0;

   if (daqd.dcu_status_check & 4) resync = 1;

   for (unsigned long i = 0;;i++) { // timing
      tick(); // measure statistics
#ifdef USE_SYMMETRICOM
     //DEBUG(6, printf("Timing %d gps=%d frac=%d\n", i, gps, frac));
#endif
#ifndef USE_BROADCAST
     read_dest = move_buf;
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
	// dcuCycleStatus shows how many matches of cycle number we got
          DEBUG(4,cerr << "dcuid=" <<j << " dcuCycleStatus=" << dcuCycleStatus[ifo][j] << " dcuStatCycle=" << dcuStatCycle[ifo][j] << endl);

	  /* Check if DCU running and in sync */
	  if ((dcuCycleStatus[ifo][j] > 3 || j < 5) && dcuStatCycle[ifo][j] > 4)
	  {
 	    dcuStatus[ifo][j] = DAQ_STATE_RUN;
	  }

	  if (/* (lastStatus == DAQ_STATE_RUN) && */ (dcuStatus[ifo][j] != DAQ_STATE_RUN)) {
	    DEBUG(4,cerr << "Lost "<< daqd.dcuName[j] <<"(ifo "<< ifo <<"; dcu "<< j <<"); status "<< dcuCycleStatus[ifo][j] << dcuStatCycle[ifo][j] << endl);
	    ipc->status = DAQ_STATE_FAULT;
	  }

	  if ((dcuStatus[ifo][j] == DAQ_STATE_RUN) /* && (lastStatus != DAQ_STATE_RUN) */) {
	    DEBUG(4,cerr << "New "<< daqd.dcuName[j] << " (dcu "<< j <<")" << endl);
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

#if defined(USE_SYMMETRICOM) || defined(USE_LOCAL_TIME)
	  //system_log(5, "dcu %d block %d cycle %d  gps %d symm %d\n", j, cblk, gmDaqIpc[j].bp[cblk].cycle,  dcu_gps, gps);
	  unsigned long mygps = gps;
	  if (cblk > (15 - cycle_delay)) mygps--;
#else
	  // FIXME: need to factor out these constants
	  unsigned long mygps = i/16 + zero_time - 315964819 + 33 + 2;
#endif
	  if (daqd.edcuFileStatus[j]) {
	    daqd.dcuStatus[0][j] |= 0x8000;
	    system_log(5, "EDCU .ini FILE CRC MISS dcu %d (%s)", j, daqd.dcuName[j]);
	  }
	  if (dcu_gps != mygps) {
	    daqd.dcuStatus[0][j] |= 0x4000;
	    system_log(5, "GPS MISS dcu %d (%s); dcu_gps=%d gps=%ld\n", j, daqd.dcuName[j], dcu_gps, mygps);
	  }

	  if (rfm_crc != crc) {
	    system_log(5, "MISS dcu %d (%s); crc[%d]=%x; computed crc=%lx\n",
		       j, daqd.dcuName[j], cblk, rfm_crc, crc);

	    /* Set DCU status to BAD, all data will be marked as BAD 
	       because of the CRC mismatch */
	    daqd.dcuStatus[0][j] |= 0x1000;
	  } else {
	    system_log(6, " MATCH dcu %d (%s); crc[%d]=%x; computed crc=%lx\n",
		       j, daqd.dcuName[j], cblk, rfm_crc, crc);
	  }
	  if (daqd.dcuStatus[0][j]) {
	    daqd.dcuCrcErrCnt[0][j]++;
	    daqd.dcuCrcErrCntPerSecondRunning[0][j]++;
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
		  // Remove 0x8000 status from propagating to the broadcast receivers
  		  prop.dcu_data[j].status = daqd.dcuStatus[0 /* IFO */][j] & ~0x8000;
      		} else
		// EDCU is "attached" to H1, not H2
		if (j == DCU_ID_EDCU && ifo == 0) {
			// See if the EDCU thread is running and assign status
			if (0x0 == (prop.dcu_data[j].status = daqd.edcu1.running? 0x0: 0xbad)) {
				// If running calculate the CRC
				
	//memcpy(read_dest, (char *)(daqd.edcu1.channel_value + daqd.edcu1.fidx), daqd.dcuSize[ifo][j]);
			
			  unsigned int bytes = daqd.dcuSize[0][DCU_ID_EDCU];
              unsigned char *cp = move_buf; // The EDCU data is in front
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
#if defined(USE_SYMMETRICOM) || defined(USE_LOCAL_TIME)
     prop.gps = gps;
     if (cblk > (15 - cycle_delay)) prop.gps--;
#else
     prop.gps = i/16 + zero_time - 315964819 + 33 + 2;
#endif
     prop.gps_n = 1000000000/16 * (i % 16);
     //printf("before put %d %d %d\n", prop.gps, prop.gps_n, frac);
#else // USE_BROADCAST defined
     if (((gps == prop.gps) && gps_n != prop.gps_n + 1000000000/16)
	 ||((gps == prop.gps + 1) && (gps_n != 0 || prop.gps_n != (1000000000/16)*15))
	 ||(gps > prop.gps + 1)) {
	fprintf(stderr, "Dropped broadcast block(s); gps now = %d, %d; was = %d, %d\n", gps, gps_n, (int)prop.gps, (int)prop.gps_n);
	exit(1);
     }

#ifdef GDS_TESTPOINTS
     // Update testpoints data in the main buffer
     daqd.gds.update_tp_data((unsigned int *)tpbufptr, (char *)move_buf);
#endif
     stat_crc.sample();
     // Parse received broadcast transmission header and
     // check config file CRCs and data CRCs, check DCU size and number
     // Assign DCU status and cycle.
     unsigned int *header = (unsigned int *)(((char *)move_buf) - BROADCAST_HEADER_SIZE);
     int ndcu = ntohl(*header++);
     //printf("ndcu = %d\n", ndcu);
     if (ndcu > 0 && ndcu <= MAX_BROADCAST_DCU_NUM) {
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
            unsigned char *cp = move_buf + data_offs; // Start of data
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
    stat_crc.tick();
// :TODO: make sure all DCUs configuration matches; restart when the mismatch detected

     prop.gps = gps;
     prop.gps_n = gps_n;
#endif
     prop.leap_seconds = daqd.gps_leap_seconds(prop.gps);

     stat_transfer.sample();
     int nbi = daqd.b1 -> put16th_dpscattered (vmic_pv, vmic_pv_len, &prop);
     stat_transfer.tick();

   //  printf("%d %d\n", prop.gps, prop.gps_n);
     //DEBUG1(cerr << "producer " << i << endl);
#if EPICS_EDCU == 1
     PV::set_pv(PV::PV_CYCLE, i);
     PV::set_pv(PV::PV_GPS, prop.gps);
    //DEBUG1(cerr << "gps=" << PV::pv(PV::PV_GPS) << endl);
     if (i % 16 == 0) {
     	  // Count how many seconds we were acquiring data
      PV::pv(PV::PV_UPTIME_SECONDS)++;
#ifndef NO_BROADCAST
	  {
		extern unsigned long dmt_retransmit_count;
		extern unsigned long dmt_failed_retransmit_count;
		// Display DMT retransmit channels every second
        PV::set_pv(PV::PV_BCAST_RETR, dmt_retransmit_count);
        PV::set_pv(PV::PV_BCAST_FAILED_RETR, dmt_failed_retransmit_count);
		dmt_retransmit_count = 0;
		dmt_failed_retransmit_count = 0;
	  }
#endif
     }
#endif

     stat_full.tick();

     ++stat_cycles;
    if (stat_cycles >= 16) {
        PV::set_pv(PV::PV_PRDCR_TIME_FULL_MIN_MS, conv::s_to_ms_int(stat_recv.getMin()));
        PV::set_pv(PV::PV_PRDCR_TIME_FULL_MAX_MS, conv::s_to_ms_int(stat_recv.getMax()));
        PV::set_pv(PV::PV_PRDCR_TIME_FULL_MEAN_MS, conv::s_to_ms_int(stat_recv.getMean()));

        PV::set_pv(PV::PV_PRDCR_TIME_RECV_MIN_MS, conv::s_to_ms_int(stat_recv.getMin()));
        PV::set_pv(PV::PV_PRDCR_TIME_RECV_MAX_MS, conv::s_to_ms_int(stat_recv.getMax()));
        PV::set_pv(PV::PV_PRDCR_TIME_RECV_MEAN_MS, conv::s_to_ms_int(stat_recv.getMean()));

        PV::set_pv(PV::PV_PRDCR_CRC_TIME_CRC_MEAN_MS, conv::s_to_ms_int(stat_crc.getMin()));
        PV::set_pv(PV::PV_PRDCR_CRC_TIME_CRC_MEAN_MS, conv::s_to_ms_int(stat_crc.getMax()));
        PV::set_pv(PV::PV_PRDCR_CRC_TIME_CRC_MEAN_MS, conv::s_to_ms_int(stat_crc.getMean()));

        PV::set_pv(PV::PV_PRDCR_CRC_TIME_XFER_MIN_MS, conv::s_to_ms_int(stat_transfer.getMin()));
        PV::set_pv(PV::PV_PRDCR_CRC_TIME_XFER_MAX_MS, conv::s_to_ms_int(stat_transfer.getMax()));
        PV::set_pv(PV::PV_PRDCR_CRC_TIME_XFER_MEAN_MS, conv::s_to_ms_int(stat_transfer.getMean()));

        stat_full.clearStats();
        stat_crc.clearStats();
        stat_recv.clearStats();
        stat_transfer.clearStats();
        stat_cycles = 0;
    }

     stat_full.sample();

#ifdef USE_BROADCAST
     stat_recv.sample();
for(;;) {
     int old_seq = seq;
     int length = NDS->receive(bufptr, buflen, &seq, &gps, &gps_n);
     //DEBUG1(printf("%d %d %d %d\n", length, seq, gps, gps_n));
     // Strangely we receiver duplicate blocks on solaris for some reason
     // Looks like this happens when the data is lost...
     if (seq == old_seq) {
	printf("received duplicate NDS DAQ broadcast sequence %d; prevpg = %d %d; gps=%d %d; length = %d\n", seq, (int)prop.gps, (int)prop.gps_n, gps, gps_n, length);
     } else break;
}
    stat_recv.tick();
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
#if defined(USE_SYMMETRICOM) || defined(USE_LOCAL_TIME)
	//printf("gps=%d  prev_gps=%d bfrac=%d prev_frac=%d\n", gps, prev_gps, frac, prev_frac);
       const int polls_per_sec = 320; // 320 polls gives 1 millisecond stddev of cycle time (AEI Nov 2012)
       for (int ntries = 0;; ntries++) {
         struct timespec tspec = {0,1000000000/polls_per_sec}; // seconds, nanoseconds
         nanosleep (&tspec, NULL);

#ifdef USE_SYMMETRICOM
	 gps = daqd.symm_gps(&frac);
#elif defined(USE_LOCAL_TIME)
	struct timeval tv;
	struct timezone tz;
	gettimeofday(&tv, &tz);
	//printf("%d %d \n", tv.tv_sec, tv.tv_usec);
	// This needs cleanup in daqdrc and in symmetricom.c
	tv.tv_sec += - 315964800 - 315964819 + 33 + daqd.symm_gps_offset;
	gps = tv.tv_sec; frac = tv.tv_usec * 1000;
#else
#error
#endif
	 if (prev_frac == 937500000) { 
		if (gps == prev_gps + 1) {
		  frac = 0;
		  break;
		} else {
		   if (gps > prev_gps + 1) {
			fprintf(stderr, "GPS card time jumped from %ld (%ld) to %ld (%ld)\n", prev_gps, prev_frac, gps, frac);
			print(cout);
			_exit(1);
		   } else if (gps < prev_gps) {
			fprintf(stderr, "GPS card time moved back from %ld to %ld\n", prev_gps, gps);
			print(cout);
			_exit(1);
		   }
		}
	 } else if (frac >= prev_frac + 62500000) {
		// Check if GPS seconds moved for some reason (because of delay)
		if (gps != prev_gps) {
		        fprintf(stderr, "WARNING: GPS time jumped from %ld (%ld) to %ld (%ld)\n", prev_gps, prev_frac, gps, frac);
			print(cout);
			gps = prev_gps;
		}
		frac = prev_frac + 62500000;
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
     		prop.gps = i/16 + zero_time - 315964819 + 33 + 2;
     		prop.gps_n = 1000000000/16 * (i % 16);
                daqd.b1 -> put16th_dpscattered (vmic_pv, vmic_pv_len, &prop);
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
#if defined(USE_SYMMETRICOM) || defined(USE_LOCAL_TIME)
     prev_gps = gps;
     prev_frac = frac;
#endif
   }
}

/// A main loop for a producer that does a debug crc operation
/// in a seperate thread
void *
producer::frame_writer_debug_crc ()
{
    // not implemented
    return (void *)NULL;
}

/// A main loop for a producer that does crc  and data transfer
/// in a seperate thread.
void *
producer::frame_writer_crc ()
{
    // not implemented
    return (void *)NULL;
}
