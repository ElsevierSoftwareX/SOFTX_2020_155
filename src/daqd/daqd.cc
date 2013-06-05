/*  
  Use following command to generate TNF trace file for the library calls :-
  prex -o /tmp/a.tnf -l $TNFHOME/lib/libc_probe.so.1 ./a.out -f ~/40m-frames/C1-97_01_12_18_49_51
  > trace $all
  > enable $all
  > continue
  Wait 15 seconds and press Ctrl-C
  > quit
  Start TNF viewer :-
  $TNFHOME/bin/tnfview /tmp/a.tnf

  To generate trace on the source code probes only do this:

  setenv LD_PRELOAD libtnfprobe.so.1
  prex -o /tmp/b.tnf ./a.out -f ~/40m-frames/C1-97_01_12_18_49_51
  > trace $all
  > enable $all
  > continue
  ...
  > quit
  $TNFHOME/bin/tnfview /tmp/b.tnf
*/

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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <string>
#include <iostream>
#if __GNUC__ >= 4
#include <ext/hash_map>
#else
#include <hash_map>
#endif
#include <fstream>
#include <vector>

using namespace std;

#include "circ.hh"
#include "y.tab.h"
#include "FlexLexer.h"
#include "channel.hh"
#include "daqc.h"
#include "daqd.hh"
#include "sing_list.hh"
#include "net_writer.hh"

#include "../../src/drv/crc.c"

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

#ifdef USE_SYMMETRICOM
#ifndef USE_IOP
//#include <bcuser.h>
#include <../drv/symmetricom/symmetricom.h>
#endif
#endif

#if 0
int printf(const char * s,...)
{
        int i;
        va_list ap;
        time_t t;
        char buf[32];

        time(&t);
        ctime_r(&t, buf);
        buf[strlen(buf)-1]=0;
        fprintf(stdout, "%s [%d]:", buf, getpid());
        va_start(ap, s);

        i = vprintf(s, ap);
        va_end(ap);
	fflush(stdout);
        return i;
}
#endif

int
daqd_c::configure_archive_channels(char *archive_name, char *file_name, char *f1) {
  archive_c *arc = 0;
  // Find archive
  for (s_link *clink = archive.first (); clink; clink = clink -> next()) {
    archive_c *a = (archive_c *)clink;
    if (!strcmp (a->fsd.get_path(), archive_name)) arc = a;
  }
  if (!arc) return DAQD_NOT_FOUND;
  
  // Read config file
  int res = arc->load_config(file_name);
  if (res == 0 && f1) res = arc->load_old_config(f1);
  return res? DAQD_NOT_FOUND: DAQD_OK;
}

// Remove an archive from the list of known archives
// :TODO: have to use locking here to avoid deleteing "live" archives
int
daqd_c::delete_archive(char *name) {
  for (s_link *clink = archive.first (); clink; clink = clink -> next()) {
    archive_c *a = (archive_c *)clink;
    if (!strcmp (a->fsd.get_path(), name)) {
      archive.remove(clink);
      return DAQD_OK;
    }
  }
  return DAQD_NOT_FOUND;
}

// Create new archive if it's not created already
// Set archive's prefix, suffix, number of Data dirs; scan archive file names
int
daqd_c::scan_archive(char *name, char *prefix, char *suffix, int ndirs) {
  archive_c *arc = 0;
  for (s_link *clink = archive.first (); clink; clink = clink -> next()) {
    archive_c *a = (archive_c *)clink;
    if (!strcmp (a->fsd.get_path(), name)) arc = a;
  }

  if (!arc) {
    void *mptr = malloc (sizeof(archive_c));
    if (!mptr) return DAQD_MALLOC;
    archive.insert(arc = new (mptr) archive_c());
  }

  return arc->scan(name, prefix, suffix, ndirs);
}

// Update file directory info for the archive
// Called when new file is written into the archive by an external archive writer
//
int
daqd_c::update_archive(char *name, unsigned long gps, unsigned long dt, unsigned int dir_num) {
  archive_c *arc = 0;
  for (s_link *clink = archive.first (); clink; clink = clink -> next()) {
    archive_c *a = (archive_c *)clink;
    if (!strcmp (a->fsd.get_path(), name)) arc = a;
  }
  if (!arc) return DAQD_NOT_FOUND;
  int res = arc->fsd.update_dir(gps, 0, dt, dir_num);
  if (res)
    return DAQD_MALLOC;
  else
    return DAQD_OK;
}

extern void *interpreter_no_prompt (void *);
int shutdown_server ();

daqd_c daqd; // root object

/* Set to the program's executable name during run time */
char *programname;

// Mutual exclusion on Frames library calls
pthread_mutex_t framelib_lock;

#ifndef NDEBUG
// Controls volume of the debugging messages that is printed out
int _debug = 10;
#endif

// Controls volume of log messages
int _log_level;

#include "../../src/drv/param.c"

struct cmp_struct {bool operator()(char *a, char *b) { return !strcmp(a,b); }};

// Sort on IFO number and DCU id
// Do not change channel order within a DCU
int chan_dcu_eq(const void *a, const void *b) {
  unsigned int dcu1, dcu2;
  dcu1 = ((channel_t *)a)->dcu_id + DCU_COUNT * ((channel_t *)a)->ifoid;
  dcu2 = ((channel_t *)b)->dcu_id + DCU_COUNT * ((channel_t *)b)->ifoid;
  if (dcu1 == dcu2) return ((channel_t *)a)->seq_num - ((channel_t *)b)->seq_num;
  else return dcu1 - dcu2;
}

// DCU id of the current configuration file (ini file)
int ini_file_dcu_id = 0;

int bcstConfigCallback(char *name, struct CHAN_PARAM *parm, void *user) {
	printf("Broadcast channel %s configured\n", name);
	daqd.broadcast_set.insert(name);
	return 1;
}

void RemoveSpaces(char* source)
{
  char* i = source;
  char* j = source;
  while(*j != 0)
  {
    *i = *j++;
    if(*i != ' ')
      i++;
  }
  *i = 0;
}

// Configure data channel info from config files
int
daqd_c::configure_channels_files ()
{
  // See if we have configured broadcast channel file
  // where the set of channels to broadcast to the DMT is specified
  //
  if (broadcast_config.compare("")) {
    unsigned long crc = 0;
    if (0 == parseConfigFile((char *)broadcast_config.c_str(), &crc,
    	bcstConfigCallback, 0, 0, 0)) {
	printf("Failed to parse broadcast config file %s\n", broadcast_config.c_str());
	return 1;
    }
  }

  // File names are specified in `master_config' file
  FILE *mcf = NULL;
  mcf = fopen(master_config.c_str(), "r");
  if (mcf == NULL) {
    system_log(1, "failed to open `%s' for reading: errno=%d", master_config.c_str(), errno);
    return 1;
  }

  num_channels = 0;
  num_active_channels = 0;
  num_science_channels = 0;
  memset(channels, 0, sizeof(channels[0]) * daqd_c::max_channels);

  for(;;){
    unsigned long crc = 0;
    int chanConfigCallback(char *, struct CHAN_PARAM *, void *user);
    int testpoint = 0;
    char buf[1024];
    char *c = fgets(buf, 1024, mcf);
    if (feof(mcf)) break;
    if (*buf == '#') continue;
    RemoveSpaces(buf);
    if (strlen(buf) > 0) {
      if (buf[strlen(buf) - 1] == '\n') buf[strlen(buf) - 1] = 0;
    }
    if (strlen(buf) == 0) continue;

    if (strlen(buf) > 4) {
	testpoint = !strcmp(buf + strlen(buf) - 4, ".par");
    }
#ifndef GDS_TESTPOINTS
    // Skip GDS config files
    if (testpoint) continue;
    if (!strcmp(buf + strlen(buf) - 7, "GDS.ini")) continue;
#endif

    ini_file_dcu_id = 0;
    if (0 == parseConfigFile(buf, &crc, chanConfigCallback, testpoint, 0, 0)) {
	printf("Failed to parse config file %s\n", buf);
	return 1;
    }
    //DEBUG(1, cerr << "Channel config: dcu " << daqd.channels[daqd.num_channels - 1].dcu_id << " crc=0x" << hex << crc  << dec << endl);
    //printf("%s has dcuid=%d\n", buf, ini_file_dcu_id);
    if(daqd.num_channels) {
      daqd.dcuConfigCRC[daqd.channels[daqd.num_channels - 1].ifoid][daqd.channels[daqd.num_channels - 1].dcu_id] = crc;
    }
    if (ini_file_dcu_id > 0 && ini_file_dcu_id < DCU_COUNT) {
      // only set DCU name if this is an INI file (*.ini)
      if (!strcmp(buf + strlen(buf) - 4, ".ini")) {
	char *slp = strrchr(buf,'/');
	if (slp) {
	  slp += 3;
	  buf[strlen(buf) - 4] = 0;
          sprintf(daqd.dcuName[ini_file_dcu_id], "%.31s", slp);
          sprintf(daqd.fullDcuName[ini_file_dcu_id], "%.31s", slp-2);
#ifdef EPICS_EDCU
	  extern char epicsDcuName[DCU_COUNT][40];
          sprintf(epicsDcuName[ini_file_dcu_id], "%.39s", slp-2);
#endif
	  buf[strlen(buf) - 4] = '.';
	}
      }
    }
  }
  fclose(mcf);

  // See if we have duplicate names
  {
#if __GNUC__ >= 4
     __gnu_cxx::hash_map<char *, int, __gnu_cxx::hash<char *>, cmp_struct> m;
#else
     hash_map<char *, int, hash<char *>, cmp_struct> m;
#endif

     for (int i = 0; i < daqd.num_channels; i++) {
	if (m[daqd.channels[i].name]) {
	  system_log(1, "Fatal error: channel `%s' is duplicated %d", daqd.channels[i].name, m[daqd.channels[i].name]);
	  return 1;
	}
	m[daqd.channels[i].name] = i;
     }
  }

  /* Sort channels on the IFO ID and then on DCU ID */
  qsort(daqd.channels, daqd.num_channels, sizeof(daqd.channels[0]), chan_dcu_eq);
  
  /* Update sequence number */
  for (int i = 0; i < daqd.num_channels; i++) daqd.channels[i].seq_num = i;

#if EPICS_EDCU == 1
  /* Epics display */
  extern unsigned int pvValue[1000];
  pvValue[1] = daqd.num_channels
#ifdef GDS_TESTPOINTS
	 - daqd.num_gds_channel_aliases
#endif
	- daqd.num_epics_channels;
#endif
  system_log(1, "finished configuring data channels");
  return 0;
}


int
chanConfigCallback(char *channel_name, struct CHAN_PARAM *params, void *user)
{
  if (daqd.num_channels >= daqd_c::max_channels) {
    system_log(1, "Too many channels. Hard limit is %d", daqd_c::max_channels);
    return 0;
  }

  channel_t *ccd = &daqd.channels [daqd.num_channels++];
  ccd -> seq_num = daqd.num_channels-1;
  ccd -> id = 0;
  if (params -> dcuid >= DCU_COUNT || params -> dcuid < 0) {
    system_log(1, "channel `%s' has bad DCU id %d", ccd -> name, params -> dcuid);
    return 0;
  }
  ccd -> dcu_id = params->dcuid;
  ini_file_dcu_id = params->dcuid;

  if (params->ifoid == 0 || params->ifoid == 1)
    ccd -> ifoid = 0; // The 4K Ifo
  else
    ccd -> ifoid = 1; // The 2K Ifo

  // We use rm id now to set the system
  ccd -> tp_node = params-> rmid;
  // printf("channel %s has node id %d\n", channel_name, ccd -> tp_node);

  strncpy (ccd -> name, channel_name, channel_t::channel_name_max_len - 1);
  ccd -> name [channel_t::channel_name_max_len - 1] = 0;
  ccd -> chNum = params->chnnum;
  ccd -> bps = daqd_c::data_type_size (params->datatype);
  ccd -> data_type = (daq_data_t) params->datatype;
  ccd -> sample_rate = params->datarate;

  //  ccd -> rm_offset = bsw(dinfo -> dataOffset) + sizeof (int);
  //  ccd -> rm_block_size = bsw(mmap -> dataBlockSize);

  // Activate channels for saving into full frames
  // Do not save 1 Hz slow channels
  ccd -> active = 0;
  if (ccd -> sample_rate > 1) {
  	ccd -> active = params->acquire;
  }
  if (ccd -> active) {
  	daqd.num_active_channels++;
  	if (ccd -> active & 2) {
  		daqd.num_science_channels++;
	}
  }

  // 1Hz channels will be acquired at 16Hz
  if (ccd -> sample_rate == 1) {
    ccd -> sample_rate = 16;
  }

  ccd -> group_num = 0;

#ifdef GDS_TESTPOINTS
  //  ccd -> rm_dinfo = (dataInfoStr *) dinfo;
  // GDS_CHANNEL --> 1
  // GDS_ALIAS   --> 2
  // If channel (test point) number is specified, then this is an alias channel
  if (IS_TP_DCU(ccd -> dcu_id)) ccd -> gds = params -> testpoint? 2: 1;
  //printf("channel %s has gds=%d\n", ccd -> name, ccd -> gds);

  // GDS channels not trended
  if (IS_GDS_ALIAS (*ccd) || IS_GDS_SIGNAL(*ccd))
    ccd -> trend = 0;
  else
    ccd -> trend = ccd -> active; // Trend all active channels;

#if 0
  /* Complex channels are not trended */
  if (ccd -> data_type == _32bit_complex)  ccd -> trend = 0;
#endif

  if (IS_GDS_ALIAS (*ccd)) {
    ccd -> active = 0;
    daqd.num_gds_channel_aliases++;
  } else {
    if (IS_GDS_SIGNAL(*ccd))
      daqd.num_gds_channels++;
  }
#else
  // ccd -> active = 1;
  ccd -> trend = ccd -> active; // Trend all active channels;
#endif
  if (IS_EPICS_DCU(ccd -> dcu_id)) {
    daqd.num_epics_channels++;
    if (ccd -> data_type != DAQ_DATATYPE_FLOAT) {
        system_log(1, "EDCU channel `%s' has unsupported data type %d", ccd -> name, ccd -> data_type);
	return 0;
    }
  }

  // assign conversion data
  ccd -> signal_gain = params->gain;
  ccd -> signal_slope = params->slope;
  ccd -> signal_offset = params->offset;
  strncpy (ccd -> signal_units, params->units, channel_t::engr_unit_max_len - 1);
  ccd -> signal_units [channel_t::engr_unit_max_len - 1] = 0;

  // set DCU rate
  extern int default_dcu_rate;
  daqd.dcuRate[ccd -> ifoid][ccd -> dcu_id] = default_dcu_rate;
  //printf("dcu %d rate %d\n", ccd -> dcu_id, default_dcu_rate);
  return 1;
}

int daqd_c::find_channel_group (const char* channel_name)
{
  for (int i = 0; i < num_channel_groups; i++) {
    if (!strncasecmp (channel_groups[i].name, channel_name,
		      strlen (channel_groups[i].name)))
      return channel_groups[i].num;
  }
  return 0;
}

General::SharedPtr<FrameCPP::Version::FrameH>
daqd_c::full_frame(int frame_length_seconds, int science,
		   adc_data_ptr_type &dptr)
  throw() {
  unsigned long nchans = 0;

  if (science) {
  	nchans = num_science_channels;
  } else {
  	nchans = num_active_channels;
  }
  FrameCPP::Version::FrAdcData *adc  = new FrameCPP::Version::FrAdcData[nchans];
  General::SharedPtr< FrameCPP::Version::FrRawData > rawData 
  	= General::SharedPtr< FrameCPP::Version::FrRawData > (new FrameCPP::Version::FrRawData);
  General::SharedPtr<FrameCPP::Version::FrameH> frame;
  FrameCPP::Version::FrHistory history ("", 0, "framebuilder, framecpp-" + string(LDAS_VERSION));
  FrameCPP::Version::FrDetector detector = daqd.getDetector1();

  // Create frame
  //
  try {
    frame = General::SharedPtr<FrameCPP::Version::FrameH> (new FrameCPP::Version::FrameH ("LIGO",
					     0, // run number ??? buffpt -r> block_prop (nb) -> prop.run;
					     1, // frame number
					     FrameCPP::Version::GPSTime (0, 0),
					     0, // leap seconds
					     frame_length_seconds // dt
					     ));
    frame -> RefDetectProc ().append (detector);

    // Append second detector if it is defined
    if (daqd.detector_name1.length() > 0) {
      FrameCPP::Version::FrDetector detector1 = daqd.getDetector2();
      frame -> RefDetectProc ().append (detector1);
    }

    frame -> SetRawData (rawData);
    frame -> RefHistory ().append (history);
  } catch (bad_alloc) {
    system_log(1, "Couldn't create full frame");
    //shutdown_server ();
    //return NULL;
    abort();
  }

  // Create ADCs
  try {
    // Fast channels
    unsigned int cur_chn = 0;

    for (int i = 0; i < num_channels; i++) {
      // Skip chanels we don't want to save
      if (science? 0 == (channels[i].active & 2): !channels[i].active) continue;

      FrameCPP::Version::FrAdcData adc
	  = FrameCPP::Version::FrAdcData (std::string(channels [i].name),
					  channels [i].group_num,
					  i, // channel ???
					  CHAR_BIT * channels [i].bps,
					  channels [i].sample_rate,
					  channels [i].signal_offset,
					  channels [i].signal_slope,
					  std::string(channels [i].signal_units),
					  channels [i].data_type == _32bit_complex? channels [i].signal_gain: .0, /* Freq shift */
					  0,
					  0,
					  .0); /* heterodyning phase in radians */

      if (channels [i].sample_rate > 16) {
        /* Append ADC AUX vector to store 16 status words per second */
        FrameCPP::Version::Dimension  aux_dims [1]
	  = { FrameCPP::Version::Dimension (16 * frame_length_seconds,
					      1. / 16,
					      "") };
	FrameCPP::Version::FrVect *aux_vect 
        	= new FrameCPP::Version::FrVect("dataValid", 1, aux_dims, new INT_2S [16 * frame_length_seconds], "");
	adc.RefAux().append (*aux_vect);
      }

      /* Append ADC data vector */
      INT_4U nx = channels [i].sample_rate * frame_length_seconds;
      FrameCPP::Version::Dimension  dims [1] = { FrameCPP::Version::Dimension (nx, 1. / channels [i].sample_rate, "time") };
      FrameCPP::Version::FrVect *vect;
      switch (channels [i].data_type) { 
      case _32bit_complex:
	{
	  vect = new FrameCPP::Version::FrVect(channels [i].name, 1, dims, new COMPLEX_8[nx], "counts");
	  break;
	}
      case _64bit_double:
	{
	  vect = new FrameCPP::Version::FrVect(channels [i].name, 1, dims, new REAL_8[nx], "counts");
	  break; 
	}
      case _32bit_float: 
	{
	  vect = new FrameCPP::Version::FrVect(channels [i].name, 1, dims, new REAL_4[nx], "counts");
	  break;
	}
      case _32bit_integer:
	{
	  vect = new FrameCPP::Version::FrVect(channels [i].name, 1, dims, new INT_4S[nx], "counts");
	  break;
	}
      case _64bit_integer:
	{
	  abort();
	}
      default:
	{
	  vect = new FrameCPP::Version::FrVect(channels [i].name, 1, dims, new INT_2S[nx], "counts");
	  break;
	}
      }
      adc.RefData().append (*vect);
      frame -> GetRawData () -> RefFirstAdc ().append (adc);
      unsigned char *dptr_fast_data = frame -> GetRawData () -> RefFirstAdc ()[cur_chn] -> RefData()[0] -> GetData().get();
      INT_2U *dptr_aux_data = 0;
      if (channels [i].sample_rate > 16) {
        dptr_aux_data = (INT_2U*) frame -> GetRawData () -> RefFirstAdc ()[cur_chn] -> RefAux()[0] -> GetData().get();
      }
      dptr.push_back(pair<unsigned char*, INT_2U*>(dptr_fast_data, dptr_aux_data));
      cur_chn++;
    }
  } catch (bad_alloc) {
    system_log(1, "Couldn't create ADC channel data");
    //delete frame;
    //    shutdown_server ();
    //return NULL;
    abort();
  }
  
  return frame;
}

void *
daqd_c::framer (int science)
{

  // Put this  thread into the realtime scheduling class with half the priority
  daqd_c::realtime ("full frame saver", 2);

  unsigned long nac = 0; // Number of active channels
  long frame_cntr;
  int nb;
  if (frames_per_file != 1) {
	printf("Not supported frames_per_file=%d\n", frames_per_file);
	abort();
  }

  int frame_file_length_seconds = frames_per_file * blocks_per_frame;
  if (science) {
    system_log(1, "Start up science mode frame writer\n");
  }

  adc_data_ptr_type dptr;
  General::SharedPtr<FrameCPP::Version::FrameH> frame
  	= full_frame (blocks_per_frame, science, dptr);

  if (!frame) {
    // Have to free all already allocated ADC structures at this point
    // to avoid memory leaks, if not shutting down here
    shutdown_server ();
    return NULL;
  }

  // done creating a frame
  if (!science) {
    sem_post (&frame_saver_sem);
    sem_post (&frame_saver_sem);
  } else {
    sem_post (&science_frame_saver_sem);
    sem_post (&science_frame_saver_sem);
  }

  // Store data in the frame 
  // write frame files

  int dir_num = -1;
  //  int tdir_num;

  unsigned long status_ptr = block_size - 17 * sizeof(int) * num_channels;   // Index to the start of signal status memory area

  bool skip_done = false;
  for (frame_cntr = 0;; frame_cntr++)
    {
      int eof_flag = 0;
      unsigned long fast_data_crc = 0;
      unsigned long fast_data_length = 0;
      time_t frame_start;
      unsigned int run, frame_number;
      time_t gps, gps_n;
      int altzone, leap_seconds;
      struct tm tms;
      char tmpf [filesys_c::filename_max + 10];
      char _tmpf [filesys_c::filename_max + 10];

    /* Accumulate frame adc data */
    for (int i = 0; i < 1 /*frames_per_file */; i++)
      for (int bnum = 0; bnum < blocks_per_frame; bnum++)
	{
	  int cnum = science? daqd.science_cnum: daqd.cnum;
	  nb = b1 -> get (cnum);

	  TNF_PROBE_1(daqd_c_framer_start, "daqds_c::framer",
		      "got one block",
		      tnf_long,   block_number,    nb);
	  {
	    circ_buffer_block_t *prop;
	    char *buf;

	    prop =  b1 -> block_prop (nb);

            // restart waiting for GPS MOD fileDt
	    if (!skip_done) { // do it only first time
		while (prop -> prop.gps % (frames_per_file*blocks_per_frame)) {
			b1 -> unlock (cnum);
	  		nb = b1 -> get (cnum);
	    		prop =  b1 -> block_prop (nb);
		}
		skip_done = true;
	    }

	    buf =  b1 -> block_ptr (nb);
	    if (!prop -> bytes)
	      {
		b1 -> unlock (cnum);
		eof_flag = 1;
		DEBUG1(cerr << "framer EOF" << endl);
		break;
	      }
	    else
	      {
		if (!(i || bnum))
		  {
		    run = prop -> prop.run;
		    gps = prop -> prop.gps; gps_n = prop -> prop.gps_n;
		    altzone = prop -> prop.altzone;
		    leap_seconds = prop -> prop.leap_seconds;

		    //		    frame_start = prop -> timestamp;
		    frame_start = gps;
		    // Frame number is based upon the cycle counter
		    frame_number = prop -> prop.cycle / 16 / (frames_per_file * blocks_per_frame);
		  }

		nac = 0;

		// Put data into the ADC structures
		for (int j = 0; j < num_channels; j++) {
			if (channels[j].active) {
				if (science) {
					// Science mode frames are indicated by the second bit
					if (channels[j].active & 2) {
						;
					} else {
						continue; // Skip it, it is commissioning only
					}
				}
			} else {
				continue; // Skip it, it is not active
			}
		  unsigned char *fast_adc_ptr = dptr[nac].first;
#ifdef USE_BROADCAST
		  // Tested at the 40m on 11 jun 08
		  // Short data needed to be sample swapped
		  if (channels [j].bps == 2) {
		    short *dest = (short *)(fast_adc_ptr + bnum*channels [j].bytes);
		    short *src = (short*)(buf + channels [j].offset);
		    unsigned int samples = channels [j].bytes / 2;
		    for (int k = 0; k < samples; k++) dest[k] = src[k^1];
		  } else {
		    memcpy (fast_adc_ptr + bnum*channels [j].bytes,
			  buf + channels [j].offset,
			  channels [j].bytes);
		  }
#else
		  memcpy (fast_adc_ptr + bnum*channels [j].bytes,
			  buf + channels [j].offset,
			  channels [j].bytes);
#endif
		  // Status is ORed blocks_per_frame times
#define	 memor2(dest, tgt) \
 *((unsigned char *)(dest)) |= *((unsigned char *)(tgt)); \
 *(((unsigned char *)(dest)) + 1) |= *(((unsigned char *)(tgt)) + 1);


		  // A pointer to 16 status words for this second
		  char *stptr = buf + status_ptr
			+ 17 * sizeof(int) * channels [j].seq_num;

		  unsigned short data_valid = 0;
		  // This converts integer status into short
		  memor2 (&data_valid, stptr);

		  // Reset data valid to zero in the begining of a second
		  if (! bnum) {
		  	frame -> GetRawData () -> RefFirstAdc ()[nac] -> SetDataValid(0);
		  }

		  // Assign data valid if not zero, so once it gets set it sticks for the duration of a second
		  if (data_valid) {
		  	frame -> GetRawData () -> RefFirstAdc ()[nac] -> SetDataValid(data_valid);
		  }
		  data_valid = frame -> GetRawData () -> RefFirstAdc ()[nac] -> GetDataValid();

		  /* Calculate CRC on fast data only */
		  /* Do not calculate CRC on bad data */
		  if (channels [j].sample_rate > 16
			&& data_valid == 0) {
		    fast_data_crc = crc_ptr (buf + channels [j].offset,
					     channels [j].bytes,
					     fast_data_crc);
	            fast_data_length += channels [j].bytes;
		  }

		  INT_2U *aux_data_valid_ptr = dptr[nac].second;
		  if (aux_data_valid_ptr) {
		    stptr += 4;
		    for (int k = 0; k < 16; k++)  {
		      memset(aux_data_valid_ptr + k + bnum*16, 0, sizeof(INT_2U));
		      memor2(aux_data_valid_ptr + k + bnum*16, stptr + 4 * k);
		    }
		  }
#undef memor2
	  	  nac++;
		}

//		cerr << "saver; block " << nb << " bytes " << prop -> bytes << endl;
	      }
	  }
	  TNF_PROBE_0(daqd_c_framer_end, "daqd_c::framer", "end of block processing");
	  b1 -> unlock (cnum);
	}

      /* finish CRC calculation for the fast data */
#if EPICS_EDCU == 1
      /* Send fast data CRC to Epics for display and checking */
      extern unsigned int pvValue[1000];
      pvValue[13] = crc_len (fast_data_length, fast_data_crc);
#endif

      if (eof_flag)
	break;

      gmtime_r (&frame_start, &tms);
      tms.tm_mon++;

      // FIXME have a function to set them all at once:
      // fw -> setFrameVars (frame_cntr, gps, gps_n, leap_seconds, altzone);
/*
  inline void setFrameFileAttributes(INT_4S run, INT_4U frameNumber,
                                     INT_4U dqual, INT_4U gps, INT_2U gpsInc,
                                     INT_4U gpsn,
                                     INT_2U leapS, INT_4S localTime)
*/

#if 0
      fw -> setFrameFileAttributes (run, frame_number, 0,
				    gps, blocks_per_frame, gps_n,
				    leap_seconds, altzone);
#endif
      frame -> SetGTime(FrameCPP::Version::GPSTime (gps, gps_n));
      //frame -> SetULeapS(leap_seconds);

      DEBUG(1, cerr << "about to write frame @ " << gps << endl);
      if (science) {
      	dir_num = science_fsd.getDirFileNames (gps, _tmpf, tmpf, frames_per_file, blocks_per_frame);
      } else {
      	dir_num = fsd.getDirFileNames (gps, _tmpf, tmpf, frames_per_file, blocks_per_frame);
      }

      int fd = creat (_tmpf, 0644);
      if (fd < 0) {
	system_log(1, "Couldn't open full frame file `%s' for writing; errno %d", _tmpf, errno);
        if (science) {
	  fsd.report_lost_frame ();
	} else {
	  fsd.report_lost_frame ();
	}
	set_fault ();
      } else {
	close (fd);
	/*try*/ {
          FrameCPP::Common::FrameBuffer<filebuf>* obuf
	    = new FrameCPP::Common::FrameBuffer<std::filebuf>(std::ios::out);
          obuf -> open(_tmpf, std::ios::out | std::ios::binary);
          FrameCPP::Common::OFrameStream  ofs(obuf);
          ofs.SetCheckSumFile(FrameCPP::Common::CheckSum::CRC);
          DEBUG(1, cerr << "Begin WriteFrame()" << endl);
	  time_t t = time(0);
          ofs.WriteFrame(frame, 
			//FrameCPP::Version::FrVect::GZIP, 1,
                        daqd.no_compression? FrameCPP::FrVect::RAW:
				 FrameCPP::FrVect::ZERO_SUPPRESS_OTHERWISE_GZIP, 1,
			//FrameCPP::Compression::MODE_ZERO_SUPPRESS_SHORT,
			//FrameCPP::Version::FrVect::DEFAULT_GZIP_LEVEL, /* 6 */
			FrameCPP::Common::CheckSum::CRC);

	  t = time(0) - t;
          DEBUG(1, cerr << "Done in " << t << " seconds" << endl);
          ofs.Close();
          obuf->close();

	  if (rename(_tmpf, tmpf)) {
	    system_log(1, "failed to rename file; errno %d", errno);
            if (science) {
	      science_fsd.report_lost_frame ();
	    } else {
	      fsd.report_lost_frame ();
	    }
	    set_fault ();
	  } else {
	    DEBUG(3, cerr << "frame " << frame_cntr << "(" << frame_number << ") is written out" << endl);
	    // Successful frame write
            if (science) {
	      science_fsd.update_dir (gps, gps_n, frame_file_length_seconds, dir_num);
	    } else {
	      fsd.update_dir (gps, gps_n, frame_file_length_seconds, dir_num);
	    }

	    // Report frame size to the Epics world
            fd = open(tmpf, O_RDONLY);
            if (fd == -1) {
             	system_log(1, "failed to open file; errno %d", errno);
                exit(1);
            }
            struct stat sb;
            if (fstat(fd, &sb) == -1) {
              system_log(1, "failed to fstat file; errno %d", errno);
              exit(1);
            }
	    if (science) pvValue[20] = sb.st_size;
	    else pvValue[19] = sb.st_size;
	    close(fd);
	  }

	  if (!science) {
	  	// Update the EPICS_SAVED value
  	 	 pvValue[18] = nac;
	  }

#ifndef NO_BROADCAST
	 // We are compiled to be a DMT broadcaster
	 //
	  fd = open(tmpf, O_RDONLY);
          if (fd == -1) {
            system_log(1, "failed to open file; errno %d", errno);
            exit(1);
          }
          struct stat sb;
          if (fstat(fd, &sb) == -1) {
            system_log(1, "failed to fstat file; errno %d", errno);
            exit(1);
          }
          void *addr = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
          if (addr == MAP_FAILED) {
            system_log(1, "failed to fstat file; errno %d", errno);
            exit(1);
          }

          net_writer_c *nw = (net_writer_c *)net_writers.first();
          assert(nw);
          assert(nw->broadcast);

          if (nw->send_to_client ((char *) addr, sb.st_size, gps, b1 -> block_period ())) {
            system_log(1, "failed to broadcast data frame");
            exit(1);
          }
          munmap(addr, sb.st_size);
          close(fd);
          unlink(tmpf);

#endif 
  	} /*catch (...) {
	  system_log(1, "failed to write full frame out");
	  fsd.report_lost_frame ();
	  set_fault ();
	}*/
      }

#if 0
      int fd = creat (_tmpf, 0644);
      if (fd < 0) {
	system_log(1, "Couldn't open full frame file `%s' for writing; errno %d", _tmpf, errno);
	fsd.report_lost_frame ();
	set_fault ();
      } else {

#if defined(DIRECTIO_ON) && defined(DIRECTIO_OFF)
        if (daqd.do_directio) directio (fd, DIRECTIO_ON);
#endif


	TNF_PROBE_1(daq_c_framer_frame_write_start, "daqd_c::framer",
		    "frame write",
		    tnf_long,   frame_number,   frame_number);
	  
	// Calculate md5 check sum
	if (cksum_file != "") {
	  unsigned long crc = fr_cksum(cksum_file, tmpf, (unsigned char *)(ost -> str ()), image_size);
	  system_log(5, "%d\t%x\n", gps, crc);
	}

	/* Write out a frame */
	int nwritten = write (fd, ost -> str (), image_size);
        if (nwritten == image_size) {
	  if (rename(_tmpf, tmpf)) {
	    system_log(1, "failed to rename file; errno %d", errno);
	    fsd.report_lost_frame ();
	    set_fault ();
	  } else {
	    DEBUG(3, cerr << "frame " << frame_cntr << "(" << frame_number << ") is written out" << endl);
	    // Successful frame write
	    fsd.update_dir (gps, gps_n, frame_file_length_seconds, dir_num);
	  }
	} else {
	  system_log(1, "failed to write full frame out; errno %d", errno);
	  fsd.report_lost_frame ();
	  set_fault ();
	}

	close (fd);
	TNF_PROBE_0(daqc_c_framer_frame_write_end, "daqd_c::framer", "frame write");
      }
#endif

#if EPICS_EDCU == 1
      if (!science) {
        /* Epics display: full res data look back size in seconds */
        extern unsigned int pvValue[1000];
        pvValue[7] = fsd.get_max() - fsd.get_min();
      }

      if (science) {
        /* Epics display: current full frame saving directory */
        extern unsigned int pvValue[1000];
        pvValue[8] = science_fsd.get_cur_dir();
      }
#endif

    }

  return NULL;
}


int
daqd_c::start_main (int pmain_buffer_size, ostream *yyout)
{
  locker mon (this);

  if (b1) {
    *yyout << "main is already running" << endl;
    return 1;
  }

  main_buffer_size = pmain_buffer_size;

  if (! fsd.get_num_dirs ())
    {
      *yyout << "set the number of directories before running main" << endl;
      return 1;
    }

  void *mptr = malloc (sizeof(circ_buffer));
  if (!mptr) {
    *yyout << "couldn't construct main circular buffer, memory exhausted" << endl;
    return 1;
  }

  // FIXME: This buffer is never freed
  b1 = new (mptr) circ_buffer (0, main_buffer_size, block_size);
  if (! (b1 -> buffer_ptr ())) {
    b1 -> ~circ_buffer();
    free ((void *) b1);
    *yyout << "main couldn't allocate buffer data blocks, memory exhausted" << endl;
    return 1;
  }

  int s = daqd.block_size / DAQ_NUM_DATA_BLOCKS_PER_SECOND;
  if (s < 128*1024) s = 128*1024;
#ifdef USE_BROADCAST
  // Broadcast has 2048 bytes header, so we allocate space for it here
  s += 2048;
  // Broadcast needs extra room for its own header
  s += 100*1024;
#endif
  move_buf = (unsigned char *) malloc (s);
  if (! move_buf) {
    system_log(1,"out of memory allocating move buffer");
    exit (1);
  }
  memset (move_buf, 255, s);
  printf("Allocated move buffer size %d bytes\n", s);
#ifdef USE_BROADCAST
  move_buf += 2048; // Keep broadcast header space in front of data space
#endif

  unsigned long  status_ptr = block_size - 17 * sizeof(int) * num_channels;   // Index to the start of signal status memory area

#ifdef EDCU_SHMEM
  key_t shmak = ftok("/tmp/foo",0);
  if (shmak == -1) {
    system_log(1,"couldn't do ftok(); errno=%d", errno);
    exit(1);
  }

  int shmid = shmget(shmak, edcu_chan_idx[1] - edcu_chan_idx[0] + 8, IPC_CREAT);
  if (shmid == -1) {
    system_log(1,"couldn't do shmget(); errno=%d", errno);
    exit(1);
  }

  edcu_shptr = (unsigned char *)shmat(shmid, 0,0);
  if (edcu_shptr == (unsigned char *)-1) {
    system_log(1,"couldn't do shmat(); errno=%d", errno);
    exit(1);
  }

  /* Set bad status for all EDCU channels in the shared memory */
  for (int i = 0; i < (edcu_chan_idx[1] - edcu_chan_idx[0])/8 + 1; i+=2) {
    ((unsigned int *) edcu_shptr)[i] = 0xbad;
  }

#endif

  int cur_dcu = -1;
  for (int i = 0; i < num_channels + 1; i++) {
    int t = 0;
    if (i == num_channels) t = 1;
    else t = cur_dcu != -1 && cur_dcu != channels[i].dcu_id;

    // Finished with cur_dcu: channels sorted by dcu_id
#ifdef USE_BROADCAST
    if (IS_MYRINET_DCU(cur_dcu) && t) {
	int rate = 4 * dcuRate[0][cur_dcu];
	int tp_size = rate/DAQ_NUM_DATA_BLOCKS_PER_SECOND;
	int n_add = DAQ_GDS_MAX_TP_ALLOWED;

	int actual = 2*DAQ_DCU_BLOCK_SIZE / tp_size;
	if (actual < n_add) n_add = actual;

	for (int j = 0; j < n_add; j++) {
          vmic_pv [vmic_pv_len].src_pvec_addr = move_buf  + dcuTPOffsetRmem[0][cur_dcu] + j*tp_size;
          vmic_pv [vmic_pv_len].dest_vec_idx = dcuTPOffset[0][cur_dcu] + j*tp_size*DAQ_NUM_DATA_BLOCKS_PER_SECOND;
          vmic_pv [vmic_pv_len].dest_status_idx = 0xffffffff;
          static unsigned int zero = 0;
          vmic_pv [vmic_pv_len].src_status_addr = &zero;
          vmic_pv [vmic_pv_len].vec_len = tp_size;
    	  cerr << "Myrinet testpoint " << j << endl;
    	  cerr << "vmic_pv: " << hex << (unsigned long) vmic_pv [vmic_pv_len].src_pvec_addr << dec << "\t" << vmic_pv [vmic_pv_len].dest_vec_idx << "\t" << vmic_pv [vmic_pv_len].vec_len << endl;
	  vmic_pv_len++;
	}
    }
#else
    if (IS_MYRINET_DCU(cur_dcu) && t) {
	// Add testpoints
	int rate = 4 * dcuRate[0][cur_dcu];
	int tp_size = rate/DAQ_NUM_DATA_BLOCKS_PER_SECOND;
	int n_add = (2*DAQ_DCU_BLOCK_SIZE - dcuDAQsize[0][cur_dcu]) / tp_size;
        if (n_add > DAQ_GDS_MAX_TP_ALLOWED) n_add = DAQ_GDS_MAX_TP_ALLOWED;
	for (int j = 0; j < n_add; j++) {
          vmic_pv [vmic_pv_len].src_pvec_addr = move_buf  + dcuTPOffsetRmem[0][cur_dcu] + j*tp_size;
          vmic_pv [vmic_pv_len].dest_vec_idx = dcuTPOffset[0][cur_dcu] + j*tp_size*DAQ_NUM_DATA_BLOCKS_PER_SECOND;
          vmic_pv [vmic_pv_len].dest_status_idx = 0xffffffff;
          static unsigned int zero = 0;
          vmic_pv [vmic_pv_len].src_status_addr = &zero;
          vmic_pv [vmic_pv_len].vec_len = rate/16;
    	DEBUG(10, cerr << "Myrinet testpoint " << j << endl);
    	DEBUG(10, cerr << "vmic_pv: " << hex << (long int) vmic_pv [vmic_pv_len].src_pvec_addr << dec << "\t" << vmic_pv [vmic_pv_len].dest_vec_idx << "\t" << vmic_pv [vmic_pv_len].vec_len << endl);
	  vmic_pv_len++;
	}
    }
#endif
    if (i == num_channels) continue;
    cur_dcu = channels[i].dcu_id;

#ifdef GDS_TESTPOINTS
    // Skip alias channels, they are not physical signals
    if (channels [i].gds & 2) {
      continue;
    }
#endif

#ifdef EDCU_SHMEM
    if (channels [i].dcu_id == ADCU_PMC_40_ID) {
      /* Slow channels point into shared memory partition */
      vmic_pv [vmic_pv_len].src_pvec_addr = edcu_shptr + channels [i].rm_offset - edcu_chan_idx[0];
    } else {
      vmic_pv [vmic_pv_len].src_pvec_addr = move_buf  + channels [i].rm_offset;
    }
#else
    vmic_pv [vmic_pv_len].src_pvec_addr = move_buf  + channels [i].rm_offset;
    vmic_pv [vmic_pv_len].dest_vec_idx = channels [i].offset;
    vmic_pv [vmic_pv_len].dest_status_idx = status_ptr + 17 * sizeof(int) * i;
#if EPICS_EDCU == 1
    if (IS_EPICS_DCU(channels [i].dcu_id)) {
      vmic_pv [vmic_pv_len].src_status_addr = edcu1.channel_status + i;
    } else
#endif
#ifdef USE_BROADCAST
    // Zero out the EXC status as well
    if (IS_TP_DCU(channels [i].dcu_id) || IS_EXC_DCU(channels [i].dcu_id)) 
#else
    if (IS_EXC_DCU(channels [i].dcu_id)) {
      /* The AWG DCUs are using older data format with status word included in front of the data */
      vmic_pv [vmic_pv_len].src_status_addr = (unsigned int *)(vmic_pv[vmic_pv_len].src_pvec_addr - 4);
    } else if (IS_TP_DCU(channels [i].dcu_id)) 
#endif
    {
      /* :TODO: need to pass real DCU status depending on the current test point selection */
      static unsigned int zero = 0;
      vmic_pv [vmic_pv_len].src_status_addr = &zero;
    } else {
      vmic_pv [vmic_pv_len].src_status_addr = dcuStatus[channels [i].ifoid] + channels [i].dcu_id;
    }
#endif
    vmic_pv [vmic_pv_len].vec_len = channels [i].bytes/16;
    // Byteswap all Myrinet data on Sun
    vmic_pv [vmic_pv_len].bsw = 0;
    DEBUG(10, cerr << channels [i].name << endl);
    DEBUG(10, cerr << "vmic_pv: " << hex << (unsigned long) vmic_pv [vmic_pv_len].src_pvec_addr << dec << "\t" << vmic_pv [vmic_pv_len].dest_vec_idx << "\t" << vmic_pv [vmic_pv_len].vec_len << endl);
    vmic_pv_len++;
  }

#if EPICS_EDCU == 1
  /* Epics display (Kb/Sec) */
  extern unsigned int pvValue[1000];

  // Want to have the size of DAQ data only
  pvValue[2] = 0;
  for (int i = 0; i < 2; i++) // Ifo number
     for (int j = 0; j < DCU_COUNT; j++)
          pvValue[2] += dcuDAQsize[i][j];
  pvValue[2] *= 16;
  pvValue[2] /= 1024; // make it kilobytes

  /* Epics display: memory buffer look back */
  pvValue[6] = main_buffer_size;
#endif

  return 0;
}

#if EPICS_EDCU == 1

int
daqd_c::start_edcu (ostream *yyout)
{
  pthread_attr_t attr;
  pthread_attr_init (&attr);
  pthread_attr_setstacksize (&attr, daqd.thread_stack_size);
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
  //  pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);

  int err_no;
  if (err_no = pthread_create (&edcu1.tid, &attr,
			       (void *(*)(void *))edcu1.edcu_static,
			       (void *) &edcu1)) {
    pthread_attr_destroy (&attr);
    system_log(1, "pthread_create() err=%d", err_no);
    return 1;
  }
  pthread_attr_destroy (&attr);
  DEBUG(2, cerr << "EDCU thread created; tid=" <<  edcu1.tid << endl);
  edcu1.running = 1;
  return 0;
}

int
daqd_c::start_epics_server (ostream *yyout, char *prefix, char *prefix1, char *prefix2)
{
  pthread_attr_t attr;
  pthread_attr_init (&attr);
  pthread_attr_setstacksize (&attr, daqd.thread_stack_size);
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
  //  pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);

  int err_no;
  epics1.prefix = prefix;
  epics1.prefix1 = prefix1;
  if (prefix2) epics1.prefix2 = prefix2;
  if (err_no = pthread_create (&epics1.tid, &attr,
			       (void *(*)(void *))epics1.epics_static,
			       (void *) &epics1)) {
    pthread_attr_destroy (&attr);
    system_log(1, "pthread_create() err=%d", err_no);
    return 1;
  }
  pthread_attr_destroy (&attr);
  DEBUG(2, cerr << "Epics server thread created; tid=" <<  epics1.tid << endl);
  epics1.running = 1;
  return 0;
}

#endif


int
daqd_c::start_producer (ostream *yyout)
{
  pthread_attr_t attr;
  pthread_attr_init (&attr);
  pthread_attr_setstacksize (&attr, daqd.thread_stack_size);
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
  //  pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);

  int err_no;
  if (err_no = pthread_create (&producer1.tid, &attr,
			       (void *(*)(void *))producer1.frame_writer_static,
			       (void *) &producer1)) {
    pthread_attr_destroy (&attr);
    system_log(1, "pthread_create() err=%d", err_no);
    return 1;
  }
  pthread_attr_destroy (&attr);
  DEBUG(2, cerr << "producer created; tid=" << producer1.tid << endl);
  return 0;
}

int
daqd_c::start_frame_saver (ostream *yyout, int science)
{
  assert (b1);
#if EPICS_EDCU == 1
  extern unsigned int pvValue[1000];
#endif
  int cn = 0;
  if ((cn = b1 -> add_consumer ()) >= 0)
    {
      if (science) sem_wait (&science_frame_saver_sem);
      else sem_wait (&frame_saver_sem);

      pthread_attr_t attr;
      pthread_attr_init (&attr);
      pthread_attr_setstacksize (&attr, daqd.thread_stack_size);
      pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
      //      pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
      int err_no;
      if (science) {
        err_no = pthread_create (&consumer [cnum], &attr, (void *(*)(void *)) daqd.science_framer_static, (void *) this);
      } else {
        err_no = pthread_create (&consumer [cnum], &attr, (void *(*)(void *)) daqd.framer_static, (void *) this);
      }
      if (err_no) {
	pthread_attr_destroy (&attr);
	system_log(1, "pthread_create() err=%d", err_no);
	return 1;
      }

      pthread_attr_destroy (&attr);
      if (science) {
      	DEBUG(2, cerr << "science frame saver created; tid=" << consumer [cnum] << endl);
        science_frame_saver_tid = consumer [cnum];
	science_cnum = cn;
      } else {
      	DEBUG(2, cerr << "frame saver created; tid=" << consumer [cnum] << endl);
        frame_saver_tid = consumer [cnum];
	cnum = cn;
      }
    }
  else
    {
      if (science) {
        *yyout << "start_science_frame_saver: too many consumers, saver was not started" << endl;
      } else {
        *yyout << "start_frame_saver: too many consumers, saver was not started" << endl;
      }
      return 1;
    }
  return 0;
}

#if 0
void *
drain (void *a)
{
  int nb;
  int ai = (int) a;
  do {
    nb = daqd.b1 -> get (ai);
    basic_io::writen (daqd.files [ai], daqd.b1 -> block_ptr (nb), daqd.b1 -> block_prop (nb) -> bytes);
    daqd.b1 -> unlock (ai);
  } while (daqd.b1 -> block_prop (nb) -> bytes);

  return NULL;
}
#endif

/*
  Print out usage message and exit
*/
void
usage (int status)
{
  cerr << "CDS Data Acquisition Server, Frame Builder, version " << SERVER_VERSION << endl;
  cerr << "California Institute of Technology, LIGO Project" << endl;
  cerr << "Client communication protocol version " << DAQD_PROTOCOL_VERSION << "." << DAQD_PROTOCOL_REVISION << endl << endl;

  cerr << "Usage: \n\t" << programname << endl
       << "\t[-c <configuration file (default -- $HOME/.daqdrc)>]" << endl << endl;
  //cerr << "\t[-s <frame writer pause usec (default -- 1 sec)>]" << endl << endl;
  
  cerr << endl << "This executable compiled on:" << endl;
  cerr << "\t" << PRODUCTION_DATE << endl << "\t" << PRODUCTION_MACHINE << endl;

  exit (status);
}

int
parse_args (int argc, char *argv [])
{
  int c;
  extern char *optarg;
  extern int optind;
  FILE *file = 0;

  while ((c = getopt (argc, argv, "n2hHf:s:c:l:")) != -1)
    {
      switch (c) {
      case 'H':
      case 'h':
	usage (0);
	break;
      case 'l':
        file = freopen(optarg, "w", stdout);
	setvbuf(stdout, NULL, _IOLBF, 0);
        stderr = stdout;
        break;
      case 'f':
	strcpy (daqd.frame_fname, optarg);
	break;
      case 's':
	daqd.writer_sleep_usec = atoi (optarg);
	break;
      case 'c': // Config file location
	daqd.config_file_name = strdup (optarg);
	break;
      case '2': // Hanford
	daqd.data_feeds = 2;
	break;
      case 'n': // No Myrinet
	daqd.no_myrinet = 1;
	break;
      default:
	usage (1);
      }
    }
  return optind;
}

int main_exit_status;

// Use gcore command to produce core on fatal signals.
// This is done, so that the program can run suid on execution.
void
shandler (int a) {
        char p[25];
	system_log(1,"going down on signal %d", a);
	seteuid (0); // Try to switch to superuser effective uid
        sprintf (p,"/bin/gcore %d", getpid());
        int error = system (p);
}

#ifdef USE_SYMMETRICOM

#ifndef USE_IOP
#if 0
  BC_PCI_HANDLE hBC_PCI;

// Get current GPS time from the symmetricom IRIG-B card
unsigned long
daqd_c::symm_gps(unsigned long *frac, int *stt) {
         DWORD min, maj;
	 time_t t;
         BYTE stat;
         struct tm *majtime;

 	 extern BC_PCI_HANDLE hBC_PCI;
         //bcReadDecTime (hBC_PCI, dectime, &min, &stat);
	 if ( bcReadBinTime (hBC_PCI, &maj, &min, &stat) == TRUE ) {
		// TODO: handle leap seconds here
#if 0
		printf("%d %d\n", maj, min);
		t = maj;
		majtime = gmtime( &t );
		printf( "\nBinary Time: %02d/%02d/%d %02d:%02d:%02d.%06lu Status: %d\n", majtime->tm_mon+1, majtime->tm_mday, majtime->tm_year+1900, majtime->tm_hour, majtime->tm_min, majtime->tm_sec, min, stat);
#endif
	 }
	maj -= 315964819 - 19;
	min *= 1000;

	 if (frac) *frac = min;
	 if (stt) *stt = stat;
	 return  maj + daqd.symm_gps_offset;
}

bool
daqd_c::symm_ok() {
	int  stat;
	symm_gps(0,&stat);
	printf("Symmetricom status %d\n", stat);
	return stat < 5;
}
#endif

int symmetricom_fd = -1;

// Get current GPS time from the symmetricom IRIG-B card
unsigned long
daqd_c::symm_gps(unsigned long *frac, int *stt) {
    unsigned long t[3];
    ioctl (symmetricom_fd, IOCTL_SYMMETRICOM_TIME, &t);
    //printf("%lds %ldu %ldn\n", t[0], t[1], t[2]);
    t[0] -= 315964800;
    //
    //Without DC level shift feature enabled
    //t[0] -= 345585;
    t[1] *= 1000;
    t[1] += t[2];
    if (frac) *frac = t[1];
    if (stt) *stt = 0;
    return  t[0] + daqd.symm_gps_offset;
}

bool
daqd_c::symm_ok() {
  unsigned long req = 0;
  ioctl (symmetricom_fd, IOCTL_SYMMETRICOM_STATUS, &req);
  printf("Symmetricom status: %s\n", req? "LOCKED": "UNCLOCKED");
  return req;
}

#else // ifdef USE_IOP

// Get current GPS time from the IOP
unsigned long
daqd_c::symm_gps(unsigned long *frac, int *stt) {
	 if (stt) *stt = 0;
	 extern volatile unsigned int *ioMemDataCycle;
	 extern volatile unsigned int *ioMemDataGPS;
	 unsigned long l = *ioMemDataCycle;
	 if (frac) *frac = l * (1000000000 / (64*1024));
	 //printf("%d %d %d\n", *ioMemDataGPS, *frac, l );
	 return  *ioMemDataGPS;
}
bool daqd_c::symm_ok() { return 1; }
#endif
#endif

main (int argc, char *argv [])
{
  int farg;
  int i;
  int stf;
  pthread_t startup_iprt;
  char startup_fname [filesys_c::filename_max];

#ifdef USE_SYMMETRICOM
#ifndef USE_IOP
  symmetricom_fd =  open ("/dev/symmetricom", O_RDWR | O_SYNC);
  if (symmetricom_fd < 0) {
	perror("/dev/symmetricom");
	exit(1);
  }

#if 0
  // Start the device
  hBC_PCI = bcStartPci();
  if (!hBC_PCI){
     printf ("Error Opening Symmetricom Device Driver /dev/windrvr6\n");
     exit(1);
  }

  // Init the time format
  BYTE tmfmt;
  if (bcReqTimeFormat (hBC_PCI, &tmfmt) != TRUE) {
     printf ("Error Getting Time Format!!!\n");
     exit(1);
  }

  // Set the year
  
  time_t tm = time(0);
  struct tm *t = gmtime(&tm);
  DWORD year = t->tm_year;
  if (bcSetYear(hBC_PCI, 1900 + year) != TRUE) {
     printf ("Error Setting the Year on the Symmetricom card.\n");
     exit(1);
  }

  if (bcSetYearAutoIncFlag(hBC_PCI, 1) != TRUE) {
     printf ("Error Setting Year Auto Incr Flag on the Symmetricom card.\n");
     exit(1);
  }
#endif
#endif
#endif


  // see if `/bin/gcore' command has setuid flag
  // give warning if it doesn't
  // `/bin/gcore' is used to generate core file, since core file is not dumped, cause
  // we run as setuid.
  // `/bin/gcore' will not work if it doesn't run with the superuser effective uid
  if (getuid() != geteuid()) {
    // Prepare signals to call core dumping handler
    (void) signal (SIGQUIT, shandler);
    (void) signal (SIGILL, shandler);
    (void) signal (SIGTRAP, shandler);
    (void) signal (SIGABRT, shandler);
    (void) signal (SIGFPE, shandler);
    (void) signal (SIGBUS, shandler);
    (void) signal (SIGSEGV, shandler);
    (void) signal (SIGSYS, shandler);
    (void) signal (SIGXCPU, shandler);
    (void) signal (SIGXFSZ, shandler);
    (void) signal (SIGSEGV, shandler);

    struct stat sbuf;
    if (stat ("/bin/gcore", &sbuf)) {
      system_log(1, "can't stat /bin/gcore: if program crashes, core file will not be generated");
    } else {
      if ( !(sbuf.st_mode & S_ISUID)) {
	system_log(1, "`/bin/gcore' has not SUID bit set: if program crashes, core file will not be generated");
      }
    }
  }

  // Switch effective to real user ID -- can always switch back to saved effective
  int error = nice(-20);
  // seteuid (getuid ());

#if defined(GPS_YMDHS_IN_FILENAME)
  // We want to keep it all in UTC
  putenv("TZ=GMT");
  system_log(1, "time is reported in GMT");
#endif

  /* Determine program name (strip filesystem path) */
  for (programname = argv [0] + strlen (argv [0]) - 1;
       programname != argv [0] && *programname != '/';
       programname--)
    ;

  if (*programname == '/')
    programname++;

  farg = parse_args (argc, argv);

  DEBUG(22, cerr << "entering main\n");
  {
    const struct rlimit lmt = {1024, 1024};
    setrlimit (RLIMIT_NOFILE, &lmt);
    const struct rlimit ulmt = {4294966272UL, 4294966272UL};
    const struct rlimit small = {536870912, 536870912};
#ifndef NDEBUG
    // Want to dump unlimited core for debugging
    setrlimit (RLIMIT_CORE, &ulmt);
#endif
#if 0
    // Setting stack limit to 2G limits process data size to 2G !!!
    setrlimit (RLIMIT_DATA, &ulmt);
    setrlimit (RLIMIT_VMEM, &ulmt);
    setrlimit (RLIMIT_STACK, &small);
    setrlimit (RLIMIT_AS, &ulmt);
#endif
  }
#ifdef DAEMONIC
  {

#if 0
    if (fork ())
      exit (0); // parent terminates

    setsid (); // become session leader

    signal (SIGHUP, SIG_IGN);

    if (fork ())
      exit (0); // first child terminates

    // Want to stay in the current directory to keep the core in it
    //    chdir ("/");

    //    umask (0);

    for (int j = 0; j < 256; j++)
      close (j);

    // Keep standard descriptor, so the program can still safely print
    open("/dev/null", O_RDONLY);
    open("/dev/null", O_WRONLY);
    open("/dev/null", O_RDONLY);
#endif // 0


    openlog (programname, LOG_PID, LOG_USER);
  }
#endif

  signal (SIGPIPE, SIG_IGN);

  fflush (stderr);
  fflush (stdout);

  pthread_mutex_init (&framelib_lock, NULL);

#ifdef USE_MX
void open_mx(void);
open_mx();
#endif

  /* Process startup file */

  if (daqd.config_file_name)
    strcpy (startup_fname, daqd.config_file_name);
  else {
    if (getenv ("HOME"))
      strcat (strcpy (startup_fname, getenv ("HOME")), "/.daqdrc");
    else
      strcpy (startup_fname, ".daqdrc");
  }

  if ((stf = open (startup_fname, O_RDONLY)) >= 0)
    {
      pthread_attr_t attr;
      pthread_attr_init (&attr);
      pthread_attr_setstacksize (&attr, daqd.thread_stack_size);
      pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
      int stderr_dup = dup (2);
      if (stderr_dup < 0)
	stderr_dup = 2;

      int err_no;
      if (err_no = pthread_create (&startup_iprt, &attr, (void *(*)(void *))interpreter_no_prompt, (void *) (stderr_dup << 16 | stf))) {
	pthread_attr_destroy (&attr);
	system_log(1, "unable to spawn startup file interpreter: pthread_create() err=%d", err_no);
	exit (1);
      }

      pthread_attr_destroy (&attr);
      DEBUG(2, cerr << "startup file interpreter thread tid=" << startup_iprt << endl);
      //      pthread_join (startup_iprt, NULL);
    }
  else
    {
      system_log(1, "Couldn't open configuration file `%s'", startup_fname);
      exit (1);
    }

  sleep(0xffffffff);

  main_exit_status = 0;
  pthread_exit (&main_exit_status);
}

int
shutdown_server ()
{
  daqd.shutting_down = 1;
  DEBUG1(cerr << "shutting down\n");

  _exit (0);

  //  pthread_cancel (daqd.producer1.tid);
  pthread_join (daqd.producer1.tid, NULL);

  struct sockaddr_in srvr_addr;
  int sockfd;

  // For each listener thread: cancel it then connect to
  // kick off the accept()
  for (int i = 0; i < daqd.num_listeners; i++)
    {
      pthread_cancel (daqd.listeners [i].tid);
      //  close (daqd.listener.listenfd);

      srvr_addr.sin_family = AF_INET;
      srvr_addr.sin_port = htons (daqd.listeners [i].listener_port);
      srvr_addr.sin_addr.s_addr = inet_addr ("127.0.0.1");
      if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
	{
	  system_log(1, "shutdown: socket(); errno=%d", errno);
	  exit (1);
	}
      connect (sockfd, (struct sockaddr *) &srvr_addr, sizeof (srvr_addr));
      pthread_join (daqd.listeners [i].tid, NULL);
    }

  // FIXME: join trend framer here too
  if (daqd.frame_saver_tid)
    pthread_join (daqd.frame_saver_tid, NULL);

  DEBUG1(cerr << "shut\n");
  exit(0);
  return 0;
}

void regerr() { abort();};
#define ESIZE 1024
char ipexpbuf [ESIZE];
char dec_num_expbuf [ESIZE];

#if 0
char *ipregexp = "^[0-9]\\{1,3\\}\\.[0-9]\\{1,3\\}\\.[0-9]\\{1,3\\}\\.[0-9]\\{1,3\\}$";
#endif

char *cur_regexp = 0;
char *ipregexp ="^([0-9]\\{1,3\\}\\.)\\{3\\}[0-9]\\{1,3\\}$";
char *dec_num_regexp = "^[0-9]\\{1,\\}$";

#define INIT         register char *sp = cur_regexp;
#define GETC()       (*sp++)
#define PEEKC()      (*sp)
#define UNGETC(c)    (--sp)
#define RETURN(c)    ;
#define __DO_NOT_DEFINE_COMPILE
#define ERROR(c)     ;
#include <regexp.h>

// This was snatched from /usr/include/regexp.h on a redhat 8 system
// The reason for this was a signed/unsigned char C++ compile bug.

/* Get and compile the user supplied pattern up to end of line or
   string or until EOF is seen, whatever happens first.  The result is
   placed in the buffer starting at EXPBUF and delimited by ENDBUF.

   This function cannot be defined in the libc itself since it depends
   on the macros.  */
char *
compile (char *__restrict instring, char *__restrict expbuf,
	 __const char *__restrict endbuf, int eof)
{
  char *__input_buffer = NULL;
  size_t __input_size = 0;
  size_t __current_size = 0;
  int __ch;
  int __error;
  INIT

  /* Align the expression buffer according to the needs for an object
     of type `regex_t'.  Then check for minimum size of the buffer for
     the compiled regular expression.  */
  regex_t *__expr_ptr;
# if defined __GNUC__ && __GNUC__ >= 2
  const size_t __req = __alignof__ (regex_t *);
# else
  /* How shall we find out?  We simply guess it and can change it is
     this really proofs to be wrong.  */
  const size_t __req = 8;
# endif
  expbuf += __req;
  expbuf -= (expbuf - ((char *) 0)) % __req;
  if (endbuf < expbuf + sizeof (regex_t))
    {
      ERROR (50);
    }
  __expr_ptr = (regex_t *) expbuf;
  /* The remaining space in the buffer can be used for the compiled
     pattern.  */
  __expr_ptr->buffer = (unsigned char *)expbuf + sizeof (regex_t);
  __expr_ptr->allocated = endbuf -  (char *) __expr_ptr->buffer;

  while ((__ch = (GETC ())) != eof)
    {
      if (__ch == '\0' || __ch == '\n')
	{
	  UNGETC (__ch);
	  break;
	}

      if (__current_size + 1 >= __input_size)
	{
	  size_t __new_size = __input_size ? 2 * __input_size : 128;
	  char *__new_room = (char *) alloca (__new_size);
	  /* See whether we can use the old buffer.  */
	  if (__new_room + __new_size == __input_buffer)
	    {
	      __input_size += __new_size;
	      __input_buffer = (char *) memcpy (__new_room, __input_buffer,
					       __current_size);
	    }
	  else if (__input_buffer + __input_size == __new_room)
	    __input_size += __new_size;
	  else
	    {
	      __input_size = __new_size;
	      __input_buffer = (char *) memcpy (__new_room, __input_buffer,
						__current_size);
	    }
	}
      __input_buffer[__current_size++] = __ch;
    }
  __input_buffer[__current_size++] = '\0';

  /* Now compile the pattern.  */
  __error = regcomp (__expr_ptr, __input_buffer, REG_NEWLINE);
  if (__error != 0)
    /* Oh well, we have to translate POSIX error codes.  */
    switch (__error)
      {
      case REG_BADPAT:
      case REG_ECOLLATE:
      case REG_ECTYPE:
      case REG_EESCAPE:
      case REG_BADRPT:
      case REG_EEND:
      case REG_ERPAREN:
      default:
	/* There is no matching error code.  */
	RETURN (36);
      case REG_ESUBREG:
	RETURN (25);
      case REG_EBRACK:
	RETURN (49);
      case REG_EPAREN:
	RETURN (42);
      case REG_EBRACE:
	RETURN (44);
      case REG_BADBR:
	RETURN (46);
      case REG_ERANGE:
	RETURN (11);
      case REG_ESPACE:
      case REG_ESIZE:
	ERROR (50);
      }

  /* Everything is ok.  */
  RETURN ((char *) (__expr_ptr->buffer + __expr_ptr->used));
}

void
daqd_c::compile_regexp () 
{
  cur_regexp = ipregexp;
  (void) compile (0, ipexpbuf, &ipexpbuf[ESIZE],'\0');
  cur_regexp = dec_num_regexp;
  (void) compile (0, dec_num_expbuf, &dec_num_expbuf[ESIZE],'\0');
}

int
daqd_c::is_valid_ip_address (char *str)
{
  return step (str, ipexpbuf);
}

int
daqd_c::is_valid_dec_number (char *str)
{
  // Somehow the regular expression stuff is broken on amd64 linux... 
  char *endptr;
  long int res = strtol(str, &endptr, 10);
  return endptr == (str + strlen(str));
  //return step (str, dec_num_expbuf);
}

