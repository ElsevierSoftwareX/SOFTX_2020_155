#ifndef DAQD_HH
#define DAQD_HH

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <semaphore.h>

#ifdef sun
#include <sys/priocntl.h>
#include <sys/rtpriocntl.h>
#include <sys/tspriocntl.h>
#include <sys/lwp.h>
#endif

#include <fstream>
#include <string>

#include "debug.h"
#include "config.h"

#ifdef USE_FRAMECPP
#include "framecpp/frame.hh"
#include "framecpp/detector.hh"
#include "framecpp/adcdata.hh"
#include "framecpp/framereader.hh"
#include "framecpp/framewriter.hh"
#include "framecpp/framewritertoc.hh"
#include "framecpp/imageframewriter.hh"

#if FRAMECPP_DATAFORMAT_VERSION > 4

#include "framecpp/Version6/FrameH.hh"
#include "framecpp/Version6/FrCommon.hh"
#include "framecpp/Version6/Functions.hh"
#include "framecpp/Version6/IFrameStream.hh"
#include "framecpp/Version6/OFrameStream.hh"
#include "framecpp/Version6/Util.hh"

#endif
#endif

#if FRAMECPP_DATAFORMAT_VERSION >= 6

#include "ldas/ldasconfig.hh"
#include "framecpp/Common/FrameSpec.hh"
#include "framecpp/Common/CheckSum.hh"
#include "framecpp/Common/IOStream.hh"
#include "framecpp/Common/FrameStream.hh"

#include "framecpp/FrameCPP.hh"

#include "framecpp/FrameH.hh"
#include "framecpp/FrAdcData.hh"
#include "framecpp/FrRawData.hh"
#include "framecpp/FrVect.hh"

#include "framecpp/Dimension.hh"

#endif


using namespace std;

#include "io.h"
#include "channel.hh"
#include "daqc.h"
#include "sing_list.hh"
#include "filesys.hh"
#include "raw_filesys.hh"
#include "circ.hh"
#include "archive.hh"

#ifdef VMICRFM_PRODUCER
#if VMICRFMTYPE == 5565
#include <rfm2g_api.h>
#else
#include <rfm_io.h>
#include <rfmApi.h>
#include <rfmErrno.h>
#endif

#if defined(AUX_5579) && !defined(__linux__)
#include <rfm_io.h>
#include <rfmApi.h>
#include <rfmErrno.h>
#endif
#endif

#define SHMEM_DAQ 1

#ifdef FILE_CHANNEL_CONFIG
#if defined(_ADVANCED_LIGO)
#if !defined(USE_GM) && !defined(USE_MX) 
#define SHMEM_DAQ 1
#endif
#include "../../src/include/daqmap.h"
#ifdef USE_GM
#include "../../src/include/drv/gmnet.h"
#include "gm_rcvr.hh"
#endif
#else
#if !defined(USE_GM) && !defined(USE_MX) && !defined(USE_UDP)
#define SHMEM_DAQ 1
#endif
#include "../../../rts/src/include/daqmap.h"
#ifdef USE_GM
#include "../../src/include/drv/gmnet.h"
#include "gm_rcvr.hh"
#endif
#endif
#else
#include <map10.h>
#endif


#ifndef	VMIC_RFM_DEV
#define	VMIC_RFM_DEV	"/dev/daqd-rfm"
#define	VMIC_RFM_DEV1	"/dev/daqd-rfm1"
#endif	/* VMIC_RFM_DEV */

#if !defined(NO_BROADCAST) || defined(USE_BROADCAST)
#include <framexmit.hh>
#endif

#ifdef GDS_TESTPOINTS
#include "gds.hh"
#endif

#include "profiler.hh"
#include "channel.hh"
#include "trend.hh"
#include "net_listener.hh"
#include "producer.hh"
#ifdef _ADVANCED_LIGO
#include "../../src/include/param.h"
#else
#include "param.h"
#endif
#if EPICS_EDCU == 1
#include "edcu.hh"
#include "epicsServer.hh"
extern unsigned int pvValue[1000];
#endif

class daqd_c {
 public:
  enum {
    max_channel_groups = MAX_CHANNEL_GROUPS,
    max_channels = MAX_CHANNELS,
    max_listeners = 3,
    max_password_len = 8,
    frame_buf_size = 10 * 1024 * 1024 // Should it be calculated based on saved data size?
  };

 private:

  /*
    Locking on the instance of the class can be done with the
    scoped locking.

    void
    daqd_c::foo()
     {
       locker mon (this);   // lock is held as long as the mon exists
       ...
     }
  */
  pthread_mutex_t bm;
  void lock (void) {pthread_mutex_lock (&bm);}
  void unlock (void) {pthread_mutex_unlock (&bm);}
  class locker;
  friend class daqd_c::locker;
  class locker {
    daqd_c *dp;
  public:
    locker (daqd_c *objp) {(dp = objp) -> lock ();}
    ~locker () {dp -> unlock ();}
  };

 public:
  daqd_c () :
    b1 (0), producer1 (0),  num_channels (0), num_channel_groups (0),
    num_epics_channels (0),

#if EPICS_EDCU == 1
    edcu1 (0),
    epics1 (),
#endif

#ifdef GDS_TESTPOINTS
    num_gds_channels (0),
    num_gds_channel_aliases (0),
#endif

    frame_saver_tid(0),
    num_active_channels(0),

#if defined(VMICRFM_PRODUCER)
    cycles_lost(0),
    rm_mem_ptr(0),
    rm_mem_ptr1(0),
    fb_ipc (FB0_IPC),
    rfmFn(0), rfmFn1(0), rh(0), rh1(0), vmic_pv_len(0),
#endif
    data_feeds(1),
    dcu_status_check(0),
#ifdef AUX_5579
    rfmFn_aux("/dev/daqd-rfm-aux"),
    rfmFn_aux1("/dev/daqd-rfm-aux1"),
    rm_aux_ptr(0),
    rm_aux_ptr1(0),
#endif

    writer_sleep_usec (1000000),
    main_buffer_size (16), shutting_down (0), num_listeners (0),
    block_size (0), thread_stack_size (10 * 1024 * 1024), config_file_name (0),
    offline_disabled (0),  profile ("main"), 
    do_scan_frame_reads (0), fsd (1), nds_jobs_dir(),
    detector_name (""),
    detector_prefix (""),
    detector_longitude (0.),
    detector_latitude (0.),
    detector_elevation (.0),
    detector_arm_x_azimuth (.0),
    detector_arm_y_azimuth (.0),
    detector_arm_x_altitude (.0),
    detector_arm_y_altitude (.0),
    detector_arm_x_midpoint (.0),
    detector_arm_y_midpoint (.0),

    detector_name1 (""),
    detector_prefix1 (""),
    detector_longitude1 (0.),
    detector_latitude1 (0.),
    detector_elevation1 (.0),
    detector_arm_x_azimuth1 (.0),
    detector_arm_y_azimuth1 (.0),
    detector_arm_x_altitude1 (.0),
    detector_arm_y_altitude1 (.0),
    detector_arm_x_midpoint1 (.0),
    detector_arm_y_midpoint1 (.0),

    frames_per_file(1), blocks_per_frame(1),
    cksum_file(""), zero_bad_data(1)

#ifdef FILE_CHANNEL_CONFIG
    , master_config("")
#endif
    , crc_debug(0), cit_40m(0), nleaps(0)
    , do_fsync(0), do_directio(0)
#ifdef _ADVANCED_LIGO
    , controller_dcu(DCU_ID_SUS_1), avoid_reconnect(0)
#endif
    , tp_allow(-1), no_myrinet(0), allow_tpman_connect_failure(0), no_compression(0)
    , symm_gps_offset(0)
    {
      // Initialize frame saver startup synchronization semaphore
      sem_init (&frame_saver_sem, 0, 1);

      pthread_mutex_init (&bm, NULL);

      // Set password initially to empty string
      password [0] = 0;

#ifndef NDEBUG
      _debug = 10;
#endif

      _log_level = 2;

#if defined(VMICRFM_PRODUCER) 
      sweptsine_filename [0] = 0;
      rfmFn = VMIC_RFM_DEV;
      rfmFn1 = VMIC_RFM_DEV1;
#if VMICRFMTYPE == 5565
      //rfm_event_id = RFM2GEVENT_INTR1;
#else
      //rfm_event_id = RFM_EVENTID_A;
#endif
#endif

      compile_regexp();

#ifdef FILE_CHANNEL_CONFIG
#if EPICS_EDCU == 1
      extern unsigned int epicsDcuStatus[3][2][DCU_COUNT];
      dcuStatus = epicsDcuStatus[0];
      dcuCrcErrCntPerSecond = epicsDcuStatus[1];
      dcuCrcErrCnt = epicsDcuStatus[2];
#endif

      for (int i = 0; i < 2; i++) { // Ifo number
	for (int j = 0; j < DCU_COUNT; j++) {
	  dcuStatus[i][j] = 1;
	  dcuCrcErrCnt[i][j] = 0;
	  dcuCrcErrCntPerSecond[i][j] = 0;
  	  dcuCrcErrCntPerSecondRunning[i][j] = 0;

	  dcuSize[i][j] = 0;
	  dcuCycle[i][j] = 1;
	  dcuIpc[i][j] = 0;
#ifdef _ADVANCED_LIGO
	  dcuTPOffset[i][j] = 0;
	  dcuTPOffsetRmem[i][j] = 0;
	  dcuDAQsize[i][j] = 0;

	  // Default dcu rate settings
  	  dcuRate[i][j] = (IS_2K_DCU(j) || j == DCU_ID_EX_2K || j == DCU_ID_TP_2K )? 2048:16384;

	  dcuName[j][0] = 0;
	  fullDcuName[j][0] = 0;
#ifdef EPICS_EDCU
          extern char epicsDcuName[DCU_COUNT][40];
          epicsDcuName[j][0] = 0;
#endif
#endif
	}
      }
#endif
    }
  
  ~daqd_c () {
    sem_destroy (&frame_saver_sem);
    pthread_mutex_destroy (&bm);
  };

  circ_buffer *b1;

  trender_c trender;

  int num_listeners; // number of network listeners
  net_listener listeners [max_listeners];

  //  net_writer_c net_writer;
  s_list net_writers; // singly linked list of network writers
  
  producer producer1;

#if EPICS_EDCU == 1
  edcu edcu1;
  epicsServer epics1;
#endif

  /* Some generic consumer data --
     probably will be factored out later on */
  pthread_t consumer [MAX_CONSUMERS];

  int cnum; /* Consumer number for the directory frame saver */
  void *framer ();
  static void *framer_static (void *a) { return ((daqd_c *)a) -> framer (); };


  // Active channels are framed. Both types are served online.
  int num_active_channels;
  channel_t active_channels [max_channels];

#ifdef GDS_TESTPOINTS
  int num_gds_channels;
  int num_gds_channel_aliases;
#endif

  char frame_fname [filesys_c::filename_max]; /* Name of input frame file */

  /* Channels config data */
  int num_channels;
  channel_t channels [max_channels];

  int num_channel_groups;
  int num_epics_channels;
  channel_group_t channel_groups [max_channel_groups];

  int start_main (int, ostream *);
  int start_producer (ostream *);
  int start_frame_saver (ostream *);
#if EPICS_EDCU == 1
  int start_edcu (ostream *);
  int start_epics_server (ostream *, char *, char *, char *);
#endif
  sem_t frame_saver_sem;

  pthread_t frame_saver_tid;

  long writer_sleep_usec; /* pause in usecs for the main producer (-s option) */
  long main_buffer_size; /* number of blocks in main circular buffer */

  int shutting_down; // set when the whole program goes down

  int block_size; // size of the main circular buffer block

  // time to filename map and place to keep timestamps describing the data we have
  filesys_c fsd;

  // directory name where NDS jobs created
  string nds_jobs_dir;

  static int power_of (int value, int r) {
    int rm;

    assert (value > 0 && r > 1);
    if (value == 1)
      return 1;

    do {
      if (value % r)
	return 0;
      value /= r;
    } while (value > 1);

    return 1;
  }

  int thread_stack_size; // Size of the stack in kilobytes for the threads
  char password [max_password_len + 1]; // Most of the commands are password protected

  char *config_file_name; // Name of the startup config file (~/.daqdrc is used if not set)
  int offline_disabled; // If set, off-line data requests are not allowed  

#ifdef FILE_CHANNEL_CONFIG
  int configure_channels_files ();
#endif

  int           data_feeds;             /* The number of data feeds: 1 -default; 2 -Hanford (2 ifos) */

#if defined(VMICRFM_PRODUCER)
  void init_vmicrfm();
  //ushort_t rfm_event_id;   // to wait for
#if VMICRFMTYPE == 5565
  RFM2GHANDLE	rh;			/* Where RFM gets opened	 */
  RFM2GHANDLE	rh1;			/* Where second RFM gets opened	 */
#else
  RFMHANDLE	rh;			/* Where RFM gets opened	 */
  RFMHANDLE	rh1;			/* Where RFM gets opened	 */
#endif
#if VMICRFMTYPE == 5565
  uint32_t	rfm_size;		/* Size of shared memory area	 */
#else
  ulong_t	rfm_size;		/* Size of shared memory area	 */
#endif
#ifdef AUX_5579
#ifdef __linux__
  RFM2GHANDLE rh5579;
  RFM2GHANDLE rh5579_1;
#else
  RFMHANDLE rh5579;
  RFMHANDLE rh5579_1;
#endif
  char		*rfmFn_aux;		        /* Name of the AUX 5579 RFM card device */
  char		*rfmFn_aux1;		        /* Name of the second AUX 5579 RFM card device */
  volatile unsigned char *rm_aux_ptr;
  volatile unsigned char *rm_aux_ptr1;
#endif
  char		*rfmFn;		        /* Name of the device		 */
  char		*rfmFn1;		/* Name of the second device	 */
  volatile unsigned char *rm_mem_ptr;
  volatile unsigned char *rm_mem_ptr1;
  int fb_ipc; // frame builder IPC area offset
#if !defined(VMIC_MMAP)
  unsigned char *move_buf;
#endif // #if !defined(VMIC_MMAP)

#ifdef EDCU_SHMEM
  unsigned char *edcu_shptr;      /* pointer to share memory buffer where EDCU write its data */
  unsigned long edcu_chan_idx[2]; /* Offset for the first and last channel EDCU channels within RFM data block */
#endif

  // One scatterred put array is used if private rfm network used
  int vmic_pv_len;
  struct put_dpvec vmic_pv [max_channels];

#ifndef FILE_CHANNEL_CONFIG
  int configure_channels_rfm ();
#endif


  int cycles_lost;
#else /* !VMICRFM_PRODUCER */
  unsigned char *move_buf;
  int vmic_pv_len;
  struct put_dpvec vmic_pv [max_channels];
#endif /* VMICRFM_PRODUCER */
  unsigned int  dcu_status_check;       /* 1 - check ifo 0; 2- ifo 1; 3 - both; 0 - none */

  inline static int
    data_type_size (short dtype) {
    switch (dtype) {
    case _16bit_integer: // 16 bit integer
      return 2;
    case _32bit_integer: // 32 bit integer
    case _32bit_float: // 32 bit float
      return 4;
    case _64bit_integer: // 64 bit integer
    case _64bit_double: // 64 bit double
      return 8;
    case _32bit_complex: // 32 bit complex
      return 4*2;
    default:
      return _undefined;
    }
  }

  int find_channel_group (const char* channel_name);

#if !defined(VMICRFM_PRODUCER)
  int configure_channels_reference_frame ();
#endif

  // Put calling thread into the realtime scheduling class, if possible
  // Set threads realtime priority to max/`priority_divider'
  //
  inline static void realtime (char *thread_name, int priority_divider) {
#ifdef sun
    seteuid (0); // Try to switch to superuser effective uid
    if (! geteuid ()) {
      int ret;
      pcinfo_t pci;
      pcparms_t pcp;
      rtparms_t rtp;
      
      strcpy (pci.pc_clname, "RT");
      ret = priocntl (P_LWPID, _lwp_self(), PC_GETCID, (char *) &pci);
      if (ret == -1) {
	system_log(1, "RT class is not configured");
      } else {
	DEBUG1(cerr << "RT class id = " << pci.pc_cid << endl);
	DEBUG1(cerr << "maximum RT priority = " << *((short *) &pci.pc_clinfo [0]) << endl);
	
	pcp.pc_cid = pci.pc_cid;
	rtp.rt_pri = (*((short *) &pci.pc_clinfo [0])) / priority_divider;
	rtp.rt_tqsecs = 0;// igonored if rt_tqnsecs == RT_TQINF
	rtp.rt_tqnsecs = RT_TQINF;
	
	memcpy ((char *) pcp.pc_clparms, &rtp, sizeof (rtparms_t));
	ret = priocntl (P_LWPID, _lwp_self(), PC_SETPARMS, (char *) &pcp);
	if (ret == -1) {
	  system_log(1, "change to RT class failed; errno=%d", errno);
	} else if (thread_name) {
	  system_log(1, "%s is running in RT class", thread_name);
	}
      }
      seteuid (getuid ()); // Go back to real uid
    }
#endif
  }

  // Put calling thread into the timesharing class
  // Set time-sharing priority to max/`priority_divider'
  //
  inline static void time_sharing (char *thread_name, int priority_divider) {
#ifdef sun
    seteuid (0); // Try to switch to superuser effective uid
    if (! geteuid ()) {
      int ret;
      pcinfo_t pci;
      pcparms_t pcp;
      tsparms_t tsp;

      strcpy (pci.pc_clname, "TS");
      ret = priocntl (P_LWPID, _lwp_self(), PC_GETCID, (char *) &pci);
      if (ret == -1) {
	system_log(1, "TS class is not configured");
      } else {
	short max_upri =  *((short *) &pci.pc_clinfo [0]);

	DEBUG1(cerr << "TS class id = " << pci.pc_cid << endl);
	DEBUG1(cerr << "limit of user priority range = -" << max_upri << " -- +" << max_upri << endl);

	pcp.pc_cid = pci.pc_cid;
	tsp.ts_uprilim = 2*max_upri / priority_divider;
	tsp.ts_uprilim -= max_upri;
	tsp.ts_upri = tsp.ts_uprilim; // Initially set to highest possible value

	DEBUG1(cerr << "setting TS priority ; limit =" << tsp.ts_uprilim << "; initial = " << tsp.ts_upri << endl);

	memcpy ((char *) pcp.pc_clparms, &tsp, sizeof (tsparms_t));
	ret = priocntl (P_LWPID, _lwp_self(), PC_SETPARMS, (char *) &pcp);
	if (ret == -1) {
	  system_log(1, "change to TS class failed; errno=%d", errno);
	} else if (thread_name) {
	  system_log(1, "%s is running in TS class", thread_name);
	}
      }
      seteuid (getuid ()); // Go back to real uid
    }
#endif
  }

  char sweptsine_filename [FILENAME_MAX + 1];

  profile_c profile;

  void compile_regexp ();
  static int is_valid_ip_address (char *);
  static int is_valid_dec_number (char *);

  // See if the data type is valid; constants defined in channel.h
  inline static int datatype_ok (int tin) {
    return MIN_DATA_TYPE <= tin && MAX_DATA_TYPE >= tin;
  }

#ifdef GDS_TESTPOINTS
  // GDS server
  gds_c gds;
#endif // defined GDS_TESTPOINTS

  void set_fault () { 
#if EPICS_EDCU == 1
	pvValue[14] = 1;
#endif
	exit(1);
  }
  void clear_fault () {
#if EPICS_EDCU == 1
	pvValue[14] = 0;
#endif
  }
  int is_fault () {
#if EPICS_EDCU == 1
	return pvValue[14];
#else
	return 0;
#endif
  }

  // set if the scanners should read frame
  // on the boundaries of ranges, but assume that all frames
  // have the same length as set for the current run
  int do_scan_frame_reads;

  // Detector information
  string detector_name;
  string detector_prefix; // Channel prefix
  double detector_longitude;
  double detector_latitude;
  float detector_elevation;
  float detector_arm_x_azimuth;
  float detector_arm_y_azimuth;
  float detector_arm_x_altitude;
  float detector_arm_y_altitude;
  float detector_arm_x_midpoint;
  float detector_arm_y_midpoint;

  string detector_name1;
  string detector_prefix1; // Channel prefix
  double detector_longitude1;
  double detector_latitude1;
  float detector_elevation1;
  float detector_arm_x_azimuth1;
  float detector_arm_y_azimuth1;
  float detector_arm_x_altitude1;
  float detector_arm_y_altitude1;
  float detector_arm_x_midpoint1;
  float detector_arm_y_midpoint1;


#if FRAMECPP_DATAFORMAT_VERSION >= 6
  // Create the first detector
  FrameCPP::Version::FrDetector getDetector1() {
    FrameCPP::Version::FrDetector detector (detector_name,
					      detector_prefix.c_str(),
					      detector_longitude,
					      detector_latitude,
					      detector_elevation,
					      detector_arm_x_azimuth,
					      detector_arm_y_azimuth,
					      detector_arm_x_altitude,
					      detector_arm_y_altitude,
					      detector_arm_x_midpoint,
					      detector_arm_y_midpoint,
					      0);
    return detector;
  }

  // Create second detector
  FrameCPP::Version::FrDetector getDetector2() {
    FrameCPP::Version::FrDetector detector (detector_name1,
					      detector_prefix1.c_str(),
					      detector_longitude1,
					      detector_latitude1,
					      detector_elevation1,
					      detector_arm_x_azimuth1,
					      detector_arm_y_azimuth1,
					      detector_arm_x_altitude1,
					      detector_arm_y_altitude1,
					      detector_arm_x_midpoint1,
					      detector_arm_y_midpoint1,
					      0);
    return detector;
  }

#elif FRAMECPP_DATAFORMAT_VERSION > 4
  // Create the first detector
  FrameCPP::Version_6::FrDetector getDetector1() {
    FrameCPP::Version_6::FrDetector detector (detector_name,
					      detector_longitude,
					      detector_latitude,
					      detector_elevation,
					      detector_arm_x_azimuth,
					      detector_arm_y_azimuth,
					      detector_arm_x_altitude,
					      detector_arm_y_altitude,
					      detector_arm_x_midpoint,
					      detector_arm_y_midpoint,
					      0,
					      0,
					      "");
    if (detector_prefix.length() == 2)
      memcpy(const_cast<char *>(detector.GetPrefix()), detector_prefix.c_str(), 2);
    return detector;
  }

  // Create second detector
  FrameCPP::Version_6::FrDetector getDetector2() {
    FrameCPP::Version_6::FrDetector detector (detector_name1,
					      detector_longitude1,
					      detector_latitude1,
					      detector_elevation1,
					      detector_arm_x_azimuth1,
					      detector_arm_y_azimuth1,
					      detector_arm_x_altitude1,
					      detector_arm_y_altitude1,
					      detector_arm_x_midpoint1,
					      detector_arm_y_midpoint1,
					      0,
					      0,
					      "");
    if (detector_prefix1.length() == 2)
      memcpy(const_cast<char *>(detector.GetPrefix()), detector_prefix1.c_str(), 2);
    return detector;
  }

#endif
  

#if FRAMECPP_DATAFORMAT_VERSION >= 6
  FrameCPP::Version::FrameH* full_frame(channel_t* , long, int frame_length_seconds = 1) throw();
#endif

#ifdef USE_FRAMECPP
  // Create a full frame
#if FRAMECPP_DATAFORMAT_VERSION > 4
  FrameCPP::Version_6::FrameH*
#else
  FrameCPP::Frame* 
#endif
  full_frame(channel_t* , long, int frame_length_seconds = 1) throw();
#endif

#ifdef USE_FRAMECPP
#if FRAMECPP_DATAFORMAT_VERSION > 4
  FrameCPP::Version_6::FrameH*
#else
  FrameCPP::Frame* 
#endif
  full_frame_long(long_channel_t* , long, int frame_length_seconds = 1) throw();
#endif

  // How many full frames to pack in single file
  int frames_per_file;
  // How many blocks of data pack up into a frame
  int blocks_per_frame;

  // Where to put the checksum data
  string cksum_file;

  // Whether we want to put zeros for bad data
  int zero_bad_data;

  // Debug CRC checksum; print them out for this DCU
  int crc_debug;

#ifdef FILE_CHANNEL_CONFIG
  // Is initialized by daqdrc file command w/master config file name
  string master_config;

  // Indicates whether the DCU is configured or not; holds DCU data size. One for each ifo.
  unsigned int dcuSize[2][DCU_COUNT];

#if EPICS_EDCU == 1
  // DCU status; for each ifo
  // Points to a static array in exServer.cc
  unsigned int (*dcuStatus)[DCU_COUNT];
#else
  unsigned int dcuStatus[2][DCU_COUNT];
#endif

#if EPICS_EDCU == 1
  // DCU CRC error counter; for each ifo
  unsigned int (*dcuCrcErrCnt)[DCU_COUNT];
#else
  unsigned int dcuCrcErrCnt[2][DCU_COUNT];
#endif

#if EPICS_EDCU == 1
  // DCU CRC error count per second; for each ifo
  unsigned int (*dcuCrcErrCntPerSecond)[DCU_COUNT];
#else
  unsigned int dcuCrcErrCntPerSecond[2][DCU_COUNT];
#endif

  // DCU CRC error running counter
  // This one is not tied to the Epics
  // There is one array for each ifo
  unsigned int dcuCrcErrCntPerSecondRunning[2][DCU_COUNT];

  // DCU cycle; for each ifo
  unsigned long dcuCycle[2][DCU_COUNT];

  // DCU config file's CRC checksum; for each ifo
  unsigned int dcuConfigCRC[2][DCU_COUNT];

  // DCU Inter-processor communication area pointer into an RFM board; for each ifo
  volatile rmIpcStr *dcuIpc[2][DCU_COUNT];
#endif

  static unsigned long fr_cksum(string cksum_file, char *tmpf, unsigned char* data, int image_size);

  int delete_archive(char *name);
  int scan_archive(char *name, char *prefix, char *suffix, int ndirs);
  int update_archive(char *name, unsigned long gps, unsigned long dt, unsigned int dir_num);
  int configure_archive_channels(char *archive_name, char *file_name, char *);

  // List of known archives
  s_list archive;

  // 40M flag
  int cit_40m;

  // leapseconds table
  unsigned int gps_leaps[16]; // gps leap seconds
  int nleaps; // leapseconds table size


  inline unsigned int
  gps_leap_seconds(unsigned int gps_time) {
    unsigned int leaps = LEAP_SECONDS;
    for (int i = 0; i < nleaps; i++)
     if (gps_time >= gps_leaps[i]) leaps++;
    return leaps;
  }

  // Whether to do fsync() calls or not when saving raw minute trend files
  int do_fsync;

  // Whether to do directio() calls or not when saving data files
  int do_directio;

#ifdef _ADVANCED_LIGO
  // Which DCU to sync up to
  int controller_dcu;

  // Offset in the main buffer for the testpoints data foreach DCU
  unsigned long dcuTPOffset[2][DCU_COUNT];
  unsigned long dcuTPOffsetRmem[2][DCU_COUNT];
  unsigned long dcuDAQsize[2][DCU_COUNT];

  // DCU rate settings
  unsigned long dcuRate[2][DCU_COUNT];

  // Do not keep running when controller DCU quits
  unsigned int avoid_reconnect;

  // DCU names assigned from configuration files (file names)
  char dcuName[DCU_COUNT][32];
  char fullDcuName[DCU_COUNT][32];
#endif

  // bitmask of IP addresses allowed to select testpoints
  int tp_allow;

  // Set to ignore Myrinet
  int no_myrinet;

  // See if 'ip' is allowed to request testpoints
  int tp_allowed (int ip) {
    return ip == (ip & tp_allow);
  }

  // Set if want to ignore tpman connection failure on startup
  // Default is not to start if one of the test point mangers is not there
  int allow_tpman_connect_failure;

#ifdef USE_SYMMETRICOM
  // Read GPS time from the symmetricom board
  unsigned long symm_gps(unsigned long *frac = 0, int *stt = 0);

  // See if the symmetricon IRIG-B board is synchronized
  bool symm_ok();
#endif

  // Set if we do not want to compress frames
  int no_compression;

  // An offset to be added to the time read off the Symmetricom card
  // This is introduced to fix a problem with IRIG-B signal lacking 
  // leap seconds information
  int symm_gps_offset;

}; // class daqd_c




#if defined(VMICRFM_PRODUCER)


#if VMICRFMTYPE == 5565

#ifdef FILE_CHANNEL_CONFIG
#define bsw(a) a
#else
inline short bsw(short v) {
  short re;
  ((char *)&re)[1] =   ((char *)&v)[0];
  ((char *)&re)[0] =   ((char *)&v)[1];
  return re;
}

inline unsigned short bsw(unsigned short v) {
  short re;
  ((char *)&re)[1] =   ((char *)&v)[0];
  ((char *)&re)[0] =   ((char *)&v)[1];
  return re;
}

inline int bsw(int v) {
  int re;
  ((char *)&re)[3] =   ((char *)&v)[0];
  ((char *)&re)[2] =   ((char *)&v)[1];
  ((char *)&re)[1] =   ((char *)&v)[2];
  ((char *)&re)[0] =   ((char *)&v)[3];
  return re;
}

inline unsigned int bsw(unsigned int v) {
  int re;
  ((char *)&re)[3] =   ((char *)&v)[0];
  ((char *)&re)[2] =   ((char *)&v)[1];
  ((char *)&re)[1] =   ((char *)&v)[2];
  ((char *)&re)[0] =   ((char *)&v)[3];
  return re;
}

inline long bsw(long v) {
  long re;
  ((char *)&re)[3] =   ((char *)&v)[0];
  ((char *)&re)[2] =   ((char *)&v)[1];
  ((char *)&re)[1] =   ((char *)&v)[2];
  ((char *)&re)[0] =   ((char *)&v)[3];
  return re;
}

inline unsigned long bsw(unsigned long v) {
  unsigned long re;
  ((char *)&re)[3] =   ((char *)&v)[0];
  ((char *)&re)[2] =   ((char *)&v)[1];
  ((char *)&re)[1] =   ((char *)&v)[2];
  ((char *)&re)[0] =   ((char *)&v)[3];
  return re;
}

inline float bsw(float v) {
  float re;
  ((char *)&re)[3] =   ((char *)&v)[0];
  ((char *)&re)[2] =   ((char *)&v)[1];
  ((char *)&re)[1] =   ((char *)&v)[2];
  ((char *)&re)[0] =   ((char *)&v)[3];
  return re;
}
#endif
#else
#define bsw(a) a
#endif


// FIXME: shouldn't this use its own IPC area

#ifndef FILE_CHANNEL_CONFIG
#define  requestRFMUpdate() \
{ \
  volatile struct rmIpcStr *ipc = (struct rmIpcStr *) (daqd.rm_mem_ptr + FB0_IPC); \
  volatile struct rmIpcStr *this_ipc = (struct rmIpcStr *) (daqd.rm_mem_ptr + daqd.fb_ipc); \
  ipc->reqAck = bsw((short)DAQS_CMD_NO_CMD); \
  ipc->request = bsw((short)DAQS_CMD_RFM_REFRESH); \
  this_ipc -> status = bsw((short)DAQ_STATE_CONFIG); \
  callback: \
  int i = 0; \
  while (bsw(ipc->reqAck) != DAQS_CMD_RFM_REFRESH)  { \
        ipc->request = bsw((short)DAQS_CMD_RFM_REFRESH); \
        system_log(1, "RFM refresh command sent, spinning on `reqAck'"); \
    	struct timespec tspec = {1,0}; \
	if (bsw(ipc->reqAck) == DAQS_REQ_CALLBACK) { \
		system_log(1, "callback on refresh command"); \
      		nanosleep (&tspec, NULL); \
		goto callback; \
	} \
        nanosleep (&tspec, NULL); \
	i++; \
	if ( i == 15 ) { \
		system_log(1, "No response from the controller after 15 seconds, retrying again..."); \
		goto callback; \
  	} \
  } \
  system_log(1, "RFM refresh complete"); \
}
#else
#define  requestRFMUpdate() ;
#endif

#endif /* VMICRFM_PRODUCER */

#endif
