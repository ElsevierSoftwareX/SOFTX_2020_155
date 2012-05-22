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


#include <rfm2g_api.h>
#if 0
#include <rfm_io.h>
#include <rfmApi.h>
#include <rfmErrno.h>
#endif
//#include <map10.h>
//#include <circ.h>
#include "../../../rts/src/include/daqmap.h"

unsigned long crctab[256] =
{
  0x0,
  0x04C11DB7, 0x09823B6E, 0x0D4326D9, 0x130476DC, 0x17C56B6B,
  0x1A864DB2, 0x1E475005, 0x2608EDB8, 0x22C9F00F, 0x2F8AD6D6,
  0x2B4BCB61, 0x350C9B64, 0x31CD86D3, 0x3C8EA00A, 0x384FBDBD,
  0x4C11DB70, 0x48D0C6C7, 0x4593E01E, 0x4152FDA9, 0x5F15ADAC,
  0x5BD4B01B, 0x569796C2, 0x52568B75, 0x6A1936C8, 0x6ED82B7F,
  0x639B0DA6, 0x675A1011, 0x791D4014, 0x7DDC5DA3, 0x709F7B7A,
  0x745E66CD, 0x9823B6E0, 0x9CE2AB57, 0x91A18D8E, 0x95609039,
  0x8B27C03C, 0x8FE6DD8B, 0x82A5FB52, 0x8664E6E5, 0xBE2B5B58,
  0xBAEA46EF, 0xB7A96036, 0xB3687D81, 0xAD2F2D84, 0xA9EE3033,
  0xA4AD16EA, 0xA06C0B5D, 0xD4326D90, 0xD0F37027, 0xDDB056FE,
  0xD9714B49, 0xC7361B4C, 0xC3F706FB, 0xCEB42022, 0xCA753D95,
  0xF23A8028, 0xF6FB9D9F, 0xFBB8BB46, 0xFF79A6F1, 0xE13EF6F4,
  0xE5FFEB43, 0xE8BCCD9A, 0xEC7DD02D, 0x34867077, 0x30476DC0,
  0x3D044B19, 0x39C556AE, 0x278206AB, 0x23431B1C, 0x2E003DC5,
  0x2AC12072, 0x128E9DCF, 0x164F8078, 0x1B0CA6A1, 0x1FCDBB16,
  0x018AEB13, 0x054BF6A4, 0x0808D07D, 0x0CC9CDCA, 0x7897AB07,
  0x7C56B6B0, 0x71159069, 0x75D48DDE, 0x6B93DDDB, 0x6F52C06C,
  0x6211E6B5, 0x66D0FB02, 0x5E9F46BF, 0x5A5E5B08, 0x571D7DD1,
  0x53DC6066, 0x4D9B3063, 0x495A2DD4, 0x44190B0D, 0x40D816BA,
  0xACA5C697, 0xA864DB20, 0xA527FDF9, 0xA1E6E04E, 0xBFA1B04B,
  0xBB60ADFC, 0xB6238B25, 0xB2E29692, 0x8AAD2B2F, 0x8E6C3698,
  0x832F1041, 0x87EE0DF6, 0x99A95DF3, 0x9D684044, 0x902B669D,
  0x94EA7B2A, 0xE0B41DE7, 0xE4750050, 0xE9362689, 0xEDF73B3E,
  0xF3B06B3B, 0xF771768C, 0xFA325055, 0xFEF34DE2, 0xC6BCF05F,
  0xC27DEDE8, 0xCF3ECB31, 0xCBFFD686, 0xD5B88683, 0xD1799B34,
  0xDC3ABDED, 0xD8FBA05A, 0x690CE0EE, 0x6DCDFD59, 0x608EDB80,
  0x644FC637, 0x7A089632, 0x7EC98B85, 0x738AAD5C, 0x774BB0EB,
  0x4F040D56, 0x4BC510E1, 0x46863638, 0x42472B8F, 0x5C007B8A,
  0x58C1663D, 0x558240E4, 0x51435D53, 0x251D3B9E, 0x21DC2629,
  0x2C9F00F0, 0x285E1D47, 0x36194D42, 0x32D850F5, 0x3F9B762C,
  0x3B5A6B9B, 0x0315D626, 0x07D4CB91, 0x0A97ED48, 0x0E56F0FF,
  0x1011A0FA, 0x14D0BD4D, 0x19939B94, 0x1D528623, 0xF12F560E,
  0xF5EE4BB9, 0xF8AD6D60, 0xFC6C70D7, 0xE22B20D2, 0xE6EA3D65,
  0xEBA91BBC, 0xEF68060B, 0xD727BBB6, 0xD3E6A601, 0xDEA580D8,
  0xDA649D6F, 0xC423CD6A, 0xC0E2D0DD, 0xCDA1F604, 0xC960EBB3,
  0xBD3E8D7E, 0xB9FF90C9, 0xB4BCB610, 0xB07DABA7, 0xAE3AFBA2,
  0xAAFBE615, 0xA7B8C0CC, 0xA379DD7B, 0x9B3660C6, 0x9FF77D71,
  0x92B45BA8, 0x9675461F, 0x8832161A, 0x8CF30BAD, 0x81B02D74,
  0x857130C3, 0x5D8A9099, 0x594B8D2E, 0x5408ABF7, 0x50C9B640,
  0x4E8EE645, 0x4A4FFBF2, 0x470CDD2B, 0x43CDC09C, 0x7B827D21,
  0x7F436096, 0x7200464F, 0x76C15BF8, 0x68860BFD, 0x6C47164A,
  0x61043093, 0x65C52D24, 0x119B4BE9, 0x155A565E, 0x18197087,
  0x1CD86D30, 0x029F3D35, 0x065E2082, 0x0B1D065B, 0x0FDC1BEC,
  0x3793A651, 0x3352BBE6, 0x3E119D3F, 0x3AD08088, 0x2497D08D,
  0x2056CD3A, 0x2D15EBE3, 0x29D4F654, 0xC5A92679, 0xC1683BCE,
  0xCC2B1D17, 0xC8EA00A0, 0xD6AD50A5, 0xD26C4D12, 0xDF2F6BCB,
  0xDBEE767C, 0xE3A1CBC1, 0xE760D676, 0xEA23F0AF, 0xEEE2ED18,
  0xF0A5BD1D, 0xF464A0AA, 0xF9278673, 0xFDE69BC4, 0x89B8FD09,
  0x8D79E0BE, 0x803AC667, 0x84FBDBD0, 0x9ABC8BD5, 0x9E7D9662,
  0x933EB0BB, 0x97FFAD0C, 0xAFB010B1, 0xAB710D06, 0xA6322BDF,
  0xA2F33668, 0xBCB4666D, 0xB8757BDA, 0xB5365D03, 0xB1F740B4
};

#if 0

  // Put calling thread into the realtime scheduling class, if possible
  // Set threads realtime priority to max/`priority_divider'
  //
  inline static void realtime (char *thread_name, int priority_divider) {
    seteuid (0); // Try to switch to superuser effective uid
    if (! geteuid ()) {
      int ret;
      pcinfo_t pci;
      pcparms_t pcp;
      rtparms_t rtp;
      
      strcpy (pci.pc_clname, "RT");
      ret = priocntl (P_LWPID, _lwp_self(), PC_GETCID, (char *) &pci);
      if (ret == -1) {
	fprintf(stderr, "RT class is not configured");
      } else {
	pcp.pc_cid = pci.pc_cid;
	rtp.rt_pri = (*((short *) &pci.pc_clinfo [0])) / priority_divider;
	rtp.rt_tqsecs = 0;// igonored if rt_tqnsecs == RT_TQINF
	rtp.rt_tqnsecs = RT_TQINF;
	
	memcpy ((char *) pcp.pc_clparms, &rtp, sizeof (rtparms_t));
	ret = priocntl (P_LWPID, _lwp_self(), PC_SETPARMS, (char *) &pcp);
	if (ret == -1) {
	  fprintf(stderr, "change to RT class failed; errno=%d", errno);
	} else if (thread_name) {
	  fprintf(stderr, "%s is running in RT class", thread_name);
	}
      }
      seteuid (getuid ()); // Go back to real uid
    }
  }
#endif

volatile unsigned char *rm_mem_ptr = 0;
#if 0
ushort_t rfm_event_id;   // to wait for
RFMHANDLE     rh;                     /* Where RFM gets opened         */
#endif
unsigned long       rfm_size;               /* Size of shared memory area    */
//int fb_ipc; // frame builder IPC area offset

  RFM2GHANDLE   rh1;

void
init_vmicrfm ()
{
  char *rfmFn = "/dev/daqd-rfm-aux";
  if (RFM2gOpen (rfmFn, &rh1) != RFM2G_SUCCESS) {
	perror("Failed to open rfm2g");
	exit(1);
  }
#if 0
  if ((rh = rfmOpen (rfmFn)) == 0) {
    fprintf(stderr, "can't open RFM device `%s'; errno=%d", rfmFn, errno);
    exit (1);
  }

  /* Determine how much memory is associated with this device	 */
  rfm_size = rfmSize(rh);
#endif

  fflush (stdout); // Workaround for VMIC RM API lib bug
  //printf("%d=0x%x bytes of VMIC Reflected Memory in `%s'", rfm_size, rfm_size, rfmFn);

  //rm_mem_ptr = rfmRfm(rh)->rfm_ram;
  int ret = RFM2gUserMemory(rh1, (void **)&rm_mem_ptr, 0, 64*1024*1024/getpagesize());
  printf("RFM2gUserMemory() returned %d\n", ret);
  printf("Not setting RFM2g byteswapping on Linux\n");


  if (! rm_mem_ptr) {
    fprintf(stderr, "failed to map VMIC Reflected Memory into the address space");
    exit(1);
  }
}

int
main()
{
  init_vmicrfm();
  

  // Put this thread into the realtime scheduling class
  // if we are running as root
  //realtime ("VMIC RFM producer", 1);

  // SC IPC
  volatile struct rmIpcStr *ipc = (struct rmIpcStr *) (rm_mem_ptr + DAQSC_IPC);

  printf("Cycle offset %x\n", (char *)&(ipc->cycle) - (char *) rm_mem_ptr);
  printf("DAQSC status = %d\n", ipc -> status);
  for (;;) {
  printf("%d\n", ipc->cycle);
  sleep(1);
  }
#if 0
  // Frame builder IPC
  volatile struct rmIpcStr *fb_ipc
    = (struct rmIpcStr *) (rm_mem_ptr + FB1_IPC);

  //  requestRFMUpdate ();

  // Fill in my IPC structure
  fb_ipc -> dcuId = FB0_ID;
  fb_ipc -> dcuType = DAQS_UTYPE_FB;
  fb_ipc -> dcuNodeId = FB0_ID;
  fb_ipc -> status = DAQ_STATE_CONFIG;
  fb_ipc -> command = DAQS_CMD_NO_CMD;
#endif

#ifdef start_with_cycle_1
  unsigned int lastCycle = 0;
#else
  unsigned int lastCycle = 0xffffffff;
#endif
  int count;

  int bs;
  char *buf = 0; // (char *) malloc (bs = daqd.b1 -> block_size ());


  //fb_ipc -> status = DAQ_STATE_RUN;

#if 0
  // pread() is used to read from the private RFM network just one block every 16th of a second
  volatile struct mainMapStr *mmap = (struct mainMapStr *)(rm_mem_ptr + MAIN_MAP);
  printf("RFM has %d blocks specified in MMAP\n", mmap->numBlocks);
  int numBlocks = mmap->numBlocks;
  mmap->numBlocks = 0xdeadbeef;
  printf("RFM has %d blocks specified in MMAP\n", mmap->numBlocks);
#define PRIVATE_BUFFERS numBlocks
  if (PRIVATE_BUFFERS > 32 || PRIVATE_BUFFERS%16!=0) {
    fprintf(stderr,"The number of blocks is invalid: %d; FATAL\n", PRIVATE_BUFFERS);

    exit(1);
  }

  unsigned int rm_block_size = mmap -> dataBlockSize;
  void *move_buf = malloc(rm_block_size);

  long read_from [PRIVATE_BUFFERS];
  void *read_dest = move_buf;
  long read_size = rm_block_size;
  volatile struct rmIpcStr *ipcs [PRIVATE_BUFFERS];
  {
    for (int i = 0; i < PRIVATE_BUFFERS; i++) {
      read_from [i] = mmap -> baseOffset + i*mmap->partitionSize;
      ipcs [i] = (struct rmIpcStr *) (rm_mem_ptr + read_from[i]);
      printf("producer is reading RFM from 0x%x to +%d", read_from [i], read_size);
    }
  }

  circ_buffer_block_prop_t prev_prop;
  prev_prop.gps = 0;
  int prev_cycle = 0;
  

  // for each 16th of a second
  for (count = 0;;) {
#ifdef rfm_interrupt
    rfmGetNevents_t newRge;
    rfmErrorMsg_t   oldErrorMsg;
#endif

    //ushort_t        fromNode;
    //uint_t          eventNo;

#ifdef rfm_interrupt
    //(void) rfmReceive (rh, rfm_event_id, &fromNode, 0);

#else
    struct timespec tspec = {0,1000000000/100}; // seconds, nanoseconds

    for (;lastCycle == ipc -> cycle && DAQS_CMD_NO_CMD == fb_ipc -> command;)
      nanosleep (&tspec, NULL);

#endif

    circ_buffer_block_prop_t prop;

    if (DAQS_CMD_RFM_REFRESH == fb_ipc -> command) {
	printf("DAQS_CMD_RFM_REFRESH command #%d received", ipc -> command);
	fb_ipc -> command = DAQS_CMD_NO_CMD;
    // Shutdown on an unknown command
    } else if (DAQS_CMD_NO_CMD != fb_ipc -> command) {
	printf("command %d received; shutdown", ipc -> command);
	fb_ipc -> cmdAck = fb_ipc -> command; // FIXME: should write out the ACK to the rfm device
	exit(0);
    }

    int from_cycle;
    long cur_cycle = ipc -> cycle;
    count++;

#ifdef rfm_interrupt
    // Skipped queued interrupts
    if (cur_cycle == lastCycle)
      continue;
#endif

    if (cur_cycle != (lastCycle + 1)) {
      printf("cycle = %d status = %d\n", cur_cycle, lastCycle);

      if (lastCycle == 0xffffffff)  // First time
	{
	if (cur_cycle % PRIVATE_BUFFERS != 1) // Skip to the buffer's boundary
	  continue;
	from_cycle = cur_cycle;
      } else if (cur_cycle > lastCycle + PRIVATE_BUFFERS - 1) {
	fprintf(stderr, "Lost cycles: %d; shutdown", cur_cycle - lastCycle);
	exit (1);
      } else if (cur_cycle < lastCycle) { // Handle VxWorks code restart
	// Do not attempt to recover when cycle number goes back
        fprintf(stderr, "cur_cycle: %d; lastCycle: %d; shutdown", cur_cycle, lastCycle);
        exit (1);
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
	 fb_ipc -> cycle = i, i++) // Reflect processed cycle into the FB IPC
      {
        unsigned long crc = 0;
        unsigned int cblk = (i-1)%PRIVATE_BUFFERS;

	for (;;) {
	  int nread;
	  //printf ("pread(%d,%x,%d,%x)\n", rh -> rh_fd, read_dest, read_size, read_from [cblk]);
	  if ((nread = pread (rh1 -> fd, read_dest, read_size, read_from [cblk])) < 0) {
	    if (errno == EINTR) {
	      printf("pread() errno %d (EINTR)", errno);
	      continue;
	    } else {
	      fprintf(stderr, "pread() errno %d", errno);
	      break;
	    }
	  } else {
	    if (nread != read_size) {
	      fprintf(stderr, "read() %d bytes", nread);
	    }
	    break;
	  }
	}

{
      // do CRC sum
      crc = 0;
      long bytes = read_size;
      unsigned char *cp = (unsigned char *)read_dest;
      while(bytes--) {
        crc = (crc << 8) ^ crctab[((crc >> 24) ^ *(cp++)) & 0xFF];
      }
      bytes = read_size;
      while (bytes > 0) {
        crc = (crc << 8) ^ crctab[((crc >> 24) ^ bytes) & 0xFF];
        bytes >>= 8;
      }
      crc = ~crc & 0xFFFFFFFF;
}

	prop.gps = ipcs [cblk] -> bp [0].timeSec;
	prop.gps_n = ipcs [cblk] -> bp [0].timeNSec;
	prop.run = ipcs [cblk]-> bp [0].run;
	unsigned int blockCycle = ipcs [cblk]-> bp [0].cycle;

	printf("gps=%d.%d;i=%d;block=%dCRC=%x;read_from=%x\n",
	       prop.gps, prop.gps_n, i, blockCycle, crc, read_from [cblk]);

	// Check for commands
	// Shutdown on an unknown command
	if (DAQS_CMD_NO_CMD != fb_ipc -> command
	    && DAQS_CMD_RFM_REFRESH != fb_ipc -> command) {
	  printf("command %d received; shutdown", ipc -> command);
	  fb_ipc -> cmdAck = fb_ipc -> command; // FIXME: should write out the ACK to the rfm device
	  exit(0);
	}
	prop.cycle = cur_cycle;

    // check for timing errors
    if (prev_prop.gps != 0) {
      if (((prop.gps==prev_prop.gps)&&(62500000 == (prop.gps_n - prev_prop.gps_n)))
	  || ((prop.gps==(prev_prop.gps+1))&&(prop.gps_n==0)&&(prev_prop.gps_n==937500000)))
	;
      else {
	fprintf(stderr, "Timing error: gps=%d.%d, prev_gps=%d.%d\n",
		   prop.gps, prop.gps_n, prev_prop.gps, prev_prop.gps_n);
	exit(1);
      }
    }
	  
    prev_prop = prop;
    prev_cycle = cur_cycle;

   }  
  } // initial `for' loop
#endif
}
