/* daqd.cc - Main daqd source code file */

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

#include <sys/syscall.h>
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
#include <fstream>
#if __GNUC__ >= 4
#ifdef DAQD_CPP11
#include <unordered_map>
#else
#include <ext/hash_map>
#endif
#else
#include <hash_map>
#endif
#include <fstream>
#include <vector>
#include <memory>

#include "framecpp/Common/MD5SumFilter.hh"
#include "run_number_client.hh"

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

#include "epics_pvs.hh"
#include "work_queue.hh"

#ifdef USE_SYMMETRICOM
#ifndef USE_IOP
//#include <bcuser.h>
#include <../drv/gpstime/gpstime.h>
#endif
#endif

namespace {
    struct framer_buf {
        daqd_c::adc_data_ptr_type dptr;
        ldas_frame_h_type frame;
        time_t gps, gps_n;
        unsigned long nac;
        int frame_file_length_seconds;
        unsigned int frame_number;
        int dir_num;
        // buffers for filenames
        char tmpf [filesys_c::filename_max + 10];
        char _tmpf [filesys_c::filename_max + 10];
    };

    typedef work_queue::work_queue<framer_buf> framer_work_queue;
}

/// Helper function to deal with the archive channels.
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

/// Remove an archive from the list of known archives
/// :TODO: have to use locking here to avoid deleteing "live" archives
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

/// Create new archive if it's not created already
/// Set archive's prefix, suffix, number of Data dirs; scan archive file names
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

/// Update file directory info for the archive
/// Called when new file is written into the archive by an external archive writer
///
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

/// Server shutdown flag
bool server_is_shutting_down = false;

daqd_c daqd; ///< root object

/// Is set to the program's executable name during run time 
char *programname;

/// Mutual exclusion on Frames library calls
pthread_mutex_t framelib_lock;

#ifndef NDEBUG
/// Controls volume of the debugging messages that is printed out
int _debug = 10;
#endif

/// Controls volume of log messages
int _log_level;

#include "../../src/drv/param.c"

struct cmp_struct {bool operator()(char *a, char *b) { return !strcmp(a,b); }};

/// Sort on IFO number and DCU id
/// Do not change channel order within a DCU
int chan_dcu_eq(const void *a, const void *b) {
  unsigned int dcu1, dcu2;
  dcu1 = ((channel_t *)a)->dcu_id + DCU_COUNT * ((channel_t *)a)->ifoid;
  dcu2 = ((channel_t *)b)->dcu_id + DCU_COUNT * ((channel_t *)b)->ifoid;
  if (dcu1 == dcu2) return ((channel_t *)a)->seq_num - ((channel_t *)b)->seq_num;
  else return dcu1 - dcu2;
}

/// DCU id of the current configuration file (ini file)
int ini_file_dcu_id = 0;

/// Broadcast channel configuration callback function.
int bcstConfigCallback(char *name, struct CHAN_PARAM *parm, void *user) {
	printf("Broadcast channel %s configured\n", name);
	daqd.broadcast_set.insert(name);
	return 1;
}

/// Remove spaces in place
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

/// Configure data channel info from config files
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

  // error message buffer
  char errmsgbuf[80];

  // File names are specified in `master_config' file
  FILE *mcf = NULL;
  mcf = fopen(master_config.c_str(), "r");
  if (mcf == NULL) {
    strerror_r(errno, errmsgbuf, sizeof(errmsgbuf));
    system_log(1, "failed to open `%s' for reading: %s", master_config.c_str(),errmsgbuf);
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

    if (enable_fckrs) {
    // Data concentrator needs to check on the EDCU .ini files periodically
    // to see if they were changed from the underneath by a recompilation of a model.
    // Save edcu file and CRC info for future checking
    if (ini_file_dcu_id == DCU_ID_EDCU) {
    	// Find the corresponding DCU id.
	// For this to work the EDCU file name needs to come in the master config after the DCU file.
	char *slp = strrchr(buf,'/'); // Find the last backslash; master file has only fully-qualified names.
	// This assumes EDCU file names of the format
	// /some/file/path/X1EDCU_ATS.ini
	// This +8 puts the pointer to ATS.ini
	if (slp && strlen(slp) > (8 + 5)) { // 8 of "X1EDCU" and 5 of ".ini" + at least one character
	  slp += 8;
	  char save_c = buf[strlen(buf) - 4];
	  buf[strlen(buf) - 4] = 0;
	  DEBUG1(cerr << "Registering CRC checker for " << slp << endl);
	  // Find the needed DCU number
	  int dcu_id;
	  for (dcu_id = 0; dcu_id < DCU_COUNT; dcu_id++) {
		if (!strcmp(dcuName[dcu_id], slp)) break;
	  }
	  if (dcu_id == DCU_COUNT) {
    	    system_log(1, "Failed to find corr. DCU for %s, did not register the checker", buf);
	  } else {
	    DEBUG1(cerr << "Registered CRC checker for " << slp << " at DCU id " << dcu_id << endl);
	    buf[strlen(buf)] = save_c; // Restore the cleared character
	    file_checker f(buf, crc, dcu_id);
    	    daqd.edcu_ini_fckrs.push_back(f);
	  }
	} else {
    	  system_log(1, "Did not register CRC checker for %s", buf);
	}
    }
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
#if DAQD_CPP11
     std::unordered_map<char *, int> m;
#else
     __gnu_cxx::hash_map<char *, int, __gnu_cxx::hash<char *>, cmp_struct> m;
#endif
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
  {
    int chan_count_total = daqd.num_channels
#ifdef GDS_TESTPOINTS
	 - daqd.num_gds_channel_aliases
#endif
	- daqd.num_epics_channels;
    PV::set_pv(PV::PV_TOTAL_CHANS, chan_count_total);

    int chan_count_science = daqd.num_science_channels
#ifdef GDS_TESTPOINTS
	 - daqd.num_gds_channel_aliases
#endif
        - daqd.num_epics_channels;
    PV::set_pv(PV::PV_SCIENCE_TOTAL_CHANS, chan_count_science);
  }
#endif
  system_log(1, "finished configuring data channels");
  return 0;
}


/// Channel configuration callback function.
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

void daqd_c::update_configuration_number(const char *source_address)
{
    if (_configuration_number != 0 || !source_address) return;
    if (num_channels == 0) return;

    FrameCPP::Common::MD5Sum check_sum;

    channel_t *cur = channels;
    channel_t *end = channels + num_channels;
    for (; cur < end; ++cur) {

        check_sum.Update(&(cur->chNum), sizeof(cur->chNum));
        check_sum.Update(&(cur->seq_num), sizeof(cur->seq_num));
        size_t name_len = strnlen(cur->name, channel_t::channel_name_max_len);
        check_sum.Update(cur->name, sizeof(name_len));
        check_sum.Update(&(cur->sample_rate), sizeof(cur->sample_rate));
        check_sum.Update(&(cur->active), sizeof(cur->active));
        check_sum.Update(&(cur->trend), sizeof(cur->trend));
        check_sum.Update(&(cur->group_num), sizeof(cur->group_num));
        check_sum.Update(&(cur->bps), sizeof(cur->bps));
        check_sum.Update(&(cur->dcu_id), sizeof(cur->dcu_id));
        check_sum.Update(&(cur->data_type), sizeof(cur->data_type));
        check_sum.Update(&(cur->signal_gain), sizeof(cur->signal_gain));
        check_sum.Update(&(cur->signal_slope), sizeof(cur->signal_slope));
        check_sum.Update(&(cur->signal_offset), sizeof(cur->signal_offset));
        size_t unit_len = strnlen(cur->signal_units, channel_t::engr_unit_max_len);
        check_sum.Update(cur->signal_units, sizeof(unit_len));

    }
    check_sum.Finalize();

    std::ostringstream ss;
    ss << check_sum;
    std::string hash = ss.str();

    _configuration_number = daqd_run_number::get_run_number(source_address, hash);
    system_log(0, "configuration/run number = %d", (int)_configuration_number);
    PV::set_pv(PV::PV_CONFIGURATION_NUMBER, _configuration_number);
}

/// Linear search for a channel group name in channel_groups array.
int daqd_c::find_channel_group (const char* channel_name)
{
  for (int i = 0; i < num_channel_groups; i++) {
    if (!strncasecmp (channel_groups[i].name, channel_name,
		      strlen (channel_groups[i].name)))
      return channel_groups[i].num;
  }
  return 0;
}

/// Create full resolution frame object.
ldas_frame_h_type
daqd_c::full_frame(int frame_length_seconds, int science,
		   adc_data_ptr_type &dptr)
{
  unsigned long nchans = 0;

  if (science) {
  	nchans = num_science_channels;
  } else {
  	nchans = num_active_channels;
  }
  FrameCPP::Version::FrAdcData *adc  = new FrameCPP::Version::FrAdcData[nchans];
  FrameCPP::Version::FrameH::rawData_type rawData
        =  FrameCPP::Version::FrameH::rawData_type(new FrameCPP::Version::FrRawData);
  ldas_frame_h_type frame;
#if USE_LDAS_VERSION
  FrameCPP::Version::FrHistory history ("", 0, "framebuilder, framecpp-" + string(LDAS_VERSION));
#else
#if USE_FRAMECPP_VERSION
  FrameCPP::Version::FrHistory history ("", 0, "framebuilder, framecpp-" + string(FRAMECPP_VERSION));
#else
  FrameCPP::Version::FrHistory history ("", 0, "framebuilder, framecpp-unknown");
#endif
#endif
  FrameCPP::Version::FrDetector detector = daqd.getDetector1();

  // Create frame
  //
  try {
    frame = ldas_frame_h_type (new FrameCPP::Version::FrameH ("LIGO",
                         configuration_number(), // run number ??? buffpt -r> block_prop (nb) -> prop.run;
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
	  vect = new FrameCPP::Version::FrVect(std::string(channels [i].name), 1, dims, new COMPLEX_8[nx], std::string(channels [i].signal_units));
	  break;
	}
      case _64bit_double:
	{
	  vect = new FrameCPP::Version::FrVect(std::string(channels [i].name), 1, dims, new REAL_8[nx], std::string(channels [i].signal_units));
	  break; 
	}
      case _32bit_float: 
	{
	  vect = new FrameCPP::Version::FrVect(std::string(channels [i].name), 1, dims, new REAL_4[nx], std::string(channels [i].signal_units));
	  break;
	}
      case _32bit_integer:
	{
	  vect = new FrameCPP::Version::FrVect(std::string(channels [i].name), 1, dims, new INT_4S[nx], std::string(channels [i].signal_units));
	  break;
	}
      case _32bit_uint:
	{
	  vect = new FrameCPP::Version::FrVect(std::string(channels [i].name), 1, dims, new INT_4U[nx], std::string(channels [i].signal_units));
	  break;
	}
      case _64bit_integer:
	{
	  abort();
	}
      default:
	{
	  vect = new FrameCPP::Version::FrVect(std::string(channels [i].name), 1, dims, new INT_2S[nx], channels [i].signal_units);
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

/// IO Thread for the full resolution frame saver
/// This is the thread that does the actual writing
void *
daqd_c::framer_io(int science)
{
   const int STATE_NORMAL = 0;
   const int STATE_WRITING = 1;
   const int STATE_BROADCAST = 2;
   bool shmem_bcast_frame = true;

#ifdef DAQD_BUILD_SHMEM
   shmem_bcast_frame = parameters().get<int>("GDS_BROADCAST", 0) == 1;
#endif
   framer_work_queue *_work_queue = 0;
   if (science) {
       daqd_c::set_thread_priority("Science frame saver IO","dqscifrio",SAVER_THREAD_PRIORITY,SCIENCE_SAVER_IO_CPUAFFINITY);
       daqd_c::locker(this);
       _work_queue = reinterpret_cast<framer_work_queue*>(_science_framer_work_queue);
   } else {
       daqd_c::set_thread_priority("Frame saver IO","dqfulfrio",SAVER_THREAD_PRIORITY,FULL_SAVER_IO_CPUAFFINITY);
       daqd_c::locker(this);
       _work_queue = reinterpret_cast<framer_work_queue*>(_framer_work_queue);
   }
   enum PV::PV_NAME epics_state_var = (science ? PV::PV_SCIENCE_FW_STATE : PV::PV_RAW_FW_STATE);

   PV::set_pv(epics_state_var, STATE_NORMAL);
   for (long frame_cntr = 0;; frame_cntr++) {
       framer_buf *cur_buf = _work_queue->get_from_queue(1);

       DEBUG(1, cerr << "About to write " << (science ? "science" : "full") << " frame @" << cur_buf->gps << endl);
       if (science) {
           cur_buf->dir_num = science_fsd.getDirFileNames (cur_buf->gps,
                                                           cur_buf->_tmpf,
                                                           cur_buf->tmpf,
                                                           frames_per_file,
                                                           blocks_per_frame);
       } else {
           cur_buf->dir_num = fsd.getDirFileNames (cur_buf->gps,
                                                   cur_buf->_tmpf,
                                                   cur_buf->tmpf,
                                                   frames_per_file,
                                                   blocks_per_frame);
       }


       int fd = creat (cur_buf->_tmpf, 0644);
       if (fd < 0) {
           system_log(1, "Couldn't open full frame file `%s' for writing; errno %d", cur_buf->_tmpf, errno);
           if (science) {
               science_fsd.report_lost_frame ();
           } else {
               fsd.report_lost_frame ();
           }
           set_fault ();
       } else {
           close (fd);
           /*try*/
           {

               PV::set_pv(epics_state_var, STATE_WRITING);
               time_t t = 0;
               {
                   FrameCPP::Common::MD5SumFilter md5filter;
                   FrameCPP::Common::FrameBuffer<filebuf>* obuf
                            = new FrameCPP::Common::FrameBuffer<std::filebuf>(std::ios::out);
                   obuf -> open(cur_buf->_tmpf, std::ios::out | std::ios::binary);
                   obuf -> FilterAdd( &md5filter );
                   FrameCPP::Common::OFrameStream  ofs(obuf);
                   ofs.SetCheckSumFile(FrameCPP::Common::CheckSum::CRC);
                   DEBUG(1, cerr << "Begin WriteFrame()" << endl);
                   t = time(0);
                   ofs.WriteFrame(cur_buf->frame,
                            //FrameCPP::Version::FrVect::GZIP, 1,
                            daqd.no_compression? FrameCPP::FrVect::RAW:
                            FrameCPP::FrVect::ZERO_SUPPRESS_OTHERWISE_GZIP, 1,
                            //FrameCPP::Compression::MODE_ZERO_SUPPRESS_SHORT,
                            //FrameCPP::Version::FrVect::DEFAULT_GZIP_LEVEL, /* 6 */
                            FrameCPP::Common::CheckSum::CRC);

                   ofs.Close();
                   obuf->close();
                   md5filter.Finalize();

                   queue_frame_checksum(cur_buf->tmpf,
                                        (science ? daqd_c::science_frame : daqd_c::raw_frame),
                                        md5filter);

                   PV::set_pv( (science ? PV::PV_SCIENCE_FRAME_CHECK_SUM_TRUNC : PV::PV_FRAME_CHECK_SUM_TRUNC),
                              *reinterpret_cast<const unsigned int*>(md5filter.Value()));
               }
               t = time(0) - t;
               PV::set_pv(epics_state_var, STATE_NORMAL);
               DEBUG(1, cerr << (science ? "Science" : "Full") << " frame done in " << t << " seconds" << endl);
               /* Record frame write time */
               if (science) {
                   PV::set_pv(PV::PV_SCIENCE_FRAME_WRITE_SEC, t);
               } else {
                   PV::set_pv(PV::PV_FRAME_WRITE_SEC, t);
               }

               if (rename(cur_buf->_tmpf, cur_buf->tmpf)) {
                   system_log(1, "failed to rename file; errno %d", errno);
                   if (science) {
                       science_fsd.report_lost_frame ();
                   } else {
                       fsd.report_lost_frame ();
                   }
                   set_fault ();
               } else {

                   DEBUG(3, cerr << "frame " << frame_cntr << "(" << cur_buf->frame_number << ") is written out" << endl);
                   // Successful frame write
                   if (science) {
                       science_fsd.update_dir (cur_buf->gps, cur_buf->gps_n, cur_buf->frame_file_length_seconds, cur_buf->dir_num);
                   } else {
                       fsd.update_dir (cur_buf->gps, cur_buf->gps_n, cur_buf->frame_file_length_seconds, cur_buf->dir_num);
                   }

                   // Report frame size to the Epics world
                   fd = open(cur_buf->tmpf, O_RDONLY);
                   if (fd == -1) {
                       system_log(1, "failed to open file; errno %d", errno);
                       exit(1);
                   }
                   struct stat sb;
                   if (fstat(fd, &sb) == -1) {
                       system_log(1, "failed to fstat file; errno %d", errno);
                       exit(1);
                   }
                   if (science) PV::set_pv(PV::PV_SCIENCE_FRAME_SIZE, sb.st_size);
                   else PV::set_pv(PV::PV_FRAME_SIZE, sb.st_size);
                   close(fd);
               }

               // Update the EPICS_SAVED value
               if (!science) {
                  PV::set_pv(PV::PV_CHANS_SAVED, cur_buf->nac);
               } else {
                  PV::set_pv(PV::PV_SCIENCE_CHANS_SAVED, cur_buf->nac);
               }

  #ifndef NO_BROADCAST
               if (shmem_bcast_frame) {
                   // We are compiled to be a DMT broadcaster
                   //
                   fd = open(cur_buf->tmpf, O_RDONLY);
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

                   net_writer_c *nw = (net_writer_c *) net_writers.first();
                   assert(nw);
                   assert(nw->broadcast);

                   PV::set_pv(epics_state_var, STATE_BROADCAST);
                   if (nw->send_to_client((char *) addr, sb.st_size, cur_buf->gps, b1->block_period())) {
                       system_log(1, "failed to broadcast data frame");
                       exit(1);
                   }
                   PV::set_pv(epics_state_var, STATE_NORMAL);
                   munmap(addr, sb.st_size);
                   close(fd);
                   unlink(cur_buf->tmpf);
               }
  #endif
           } /*catch (...) {
        system_log(1, "failed to write full frame out");
        fsd.report_lost_frame ();
        set_fault ();
       }*/
       }

       if (!science) {
           /* Epics display: full res data look back size in seconds */
           PV::set_pv(PV::PV_LOOKBACK_FULL, fsd.get_max() - fsd.get_min());
       }

       if (science) {
           /* Epics display: current full frame saving directory */
           PV::set_pv(PV::PV_LOOKBACK_DIR, science_fsd.get_cur_dir());
       }
       _work_queue->add_to_queue(0, cur_buf);
   }
   return NULL;
}

/// Full resolution frame saving thread.
void *
daqd_c::framer (int science)
{
  const int STATE_NORMAL = 0;
  const int STATE_PROCESSING = 1;
  enum PV::PV_NAME epics_state_var = (science ? PV::PV_SCIENCE_FW_DATA_STATE : PV::PV_RAW_FW_DATA_STATE);
  enum PV::PV_NAME epics_sec_var = (science ? PV::PV_SCIENCE_FW_DATA_SEC : PV::PV_RAW_FW_DATA_SEC);

  framer_work_queue *_work_queue = 0;
  // create work queues
  if (science) {
      daqd_c::locker _l(this);
      if (!_science_framer_work_queue)
          _science_framer_work_queue = reinterpret_cast<void *>(new framer_work_queue(2));
      _work_queue = reinterpret_cast<framer_work_queue *>(_science_framer_work_queue);
  } else {
      daqd_c::locker _l(this);
      if (!_framer_work_queue)
          _framer_work_queue = reinterpret_cast<void *>(new framer_work_queue(2));
      _work_queue = reinterpret_cast<framer_work_queue *>(_framer_work_queue);
  }

  unsigned long nac = 0; // Number of active channels
  long frame_cntr;
  int nb;
  if (frames_per_file != 1) {
	printf("Not supported frames_per_file=%d\n", frames_per_file);
	abort();
  }

  if (science) {
    system_log(1, "Start up science mode frame writer\n");
  }

  // create buffers for the queue
  for (int i = 0; i < 2; ++i) {
      auto_ptr<framer_buf> _buf(new framer_buf);
      _buf->frame_file_length_seconds = frames_per_file * blocks_per_frame;
      _buf->dir_num = -1;
      _buf->frame = full_frame (blocks_per_frame, science, _buf->dptr);

      if (!(_buf->frame)) {
        // Have to free all already allocated ADC structures at this point
        // to avoid memory leaks, if not shutting down here
        shutdown_server ();
        return NULL;
      }
      _work_queue->add_to_queue(0, _buf.get());
      _buf.release();
  }

    // error message buffer
    char errmsgbuf[80];

    // Startup the IO thread
    {
        DEBUG(4, cerr << "starting " << (science ? "science" : "full") << " framer IO thread" << endl);
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, daqd.thread_stack_size);
        pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

        int err = 0;
        if (science) {
            err = pthread_create( &science_frame_saver_io_tid, &attr,
                                  (void *(*)(void *))daqd_c::science_framer_io_static,
                                  (void *)this);
        } else {
            err = pthread_create( &frame_saver_io_tid, &attr,
                                  (void *(*)(void *))daqd_c::framer_io_static,
                                  (void *)this);
        }
        if (err) {
            pthread_attr_destroy(&attr);
            system_log(1, "pthread_create() err=%d while creating %s IO thread", err, (science ? "science" : "full"));
            exit(1);
        }
        pthread_attr_destroy(&attr);
    }


    // Set thread parameters. Make sure this is done after starting the io threads.
  if (science) {
      daqd_c::set_thread_priority("Science frame saver","dqscifr",SAVER_THREAD_PRIORITY,SCIENCE_SAVER_CPUAFFINITY);
  } else {
      daqd_c::set_thread_priority("Full frame saver","dqfulfr",SAVER_THREAD_PRIORITY,FULL_SAVER_CPUAFFINITY);
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

  unsigned long status_ptr = block_size - 17 * sizeof(int) * num_channels;   // Index to the start of signal status memory area

  bool skip_done = false;
  for (frame_cntr = 0;; frame_cntr++)
    {
      framer_buf *cur_buf = _work_queue->get_from_queue(0);
      int eof_flag = 0;
      unsigned long fast_data_crc = 0;
      unsigned long fast_data_length = 0;
      time_t frame_start;
      unsigned int run;
      time_t gps, gps_n;
      int altzone, leap_seconds;
      struct tm tms;

      time_t tdata = time(0);
      PV::set_pv(epics_state_var, STATE_PROCESSING);

    /* Accumulate frame adc data */
    for (int i = 0; i < 1 /*frames_per_file */; i++)
      for (int bnum = 0; bnum < blocks_per_frame; bnum++)
	{
	  int cnum = science? daqd.science_cnum: daqd.cnum;
	  nb = b1 -> get (cnum);
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
            cur_buf->frame_number = prop -> prop.cycle / 16 / (frames_per_file * blocks_per_frame);
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
          unsigned char *fast_adc_ptr = cur_buf->dptr[nac].first;
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
            cur_buf->frame -> GetRawData () -> RefFirstAdc ()[nac] -> SetDataValid(0);
		  }

		  // Assign data valid if not zero, so once it gets set it sticks for the duration of a second
		  if (data_valid) {
            cur_buf->frame -> GetRawData () -> RefFirstAdc ()[nac] -> SetDataValid(data_valid);
		  }
          data_valid = cur_buf->frame -> GetRawData () -> RefFirstAdc ()[nac] -> GetDataValid();

		  /* Calculate CRC on fast data only */
		  /* Do not calculate CRC on bad data */
		  if (channels [j].sample_rate > 16
			&& data_valid == 0) {
		    fast_data_crc = crc_ptr (buf + channels [j].offset,
					     channels [j].bytes,
					     fast_data_crc);
	            fast_data_length += channels [j].bytes;
		  }

          INT_2U *aux_data_valid_ptr = cur_buf->dptr[nac].second;
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
          b1 -> unlock (cnum);
	}

      /* finish CRC calculation for the fast data */
#if EPICS_EDCU == 1
      /* Send fast data CRC to Epics for display and checking */
    PV::set_pv(PV::PV_FAST_DATA_CRC, crc_len (fast_data_length, fast_data_crc));
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

      cur_buf->frame -> SetGTime(FrameCPP::Version::GPSTime (gps, gps_n));
      //frame -> SetULeapS(leap_seconds);

      DEBUG(1, cerr << "adding frame @ " << gps << " to " << (science ? "science" : "full") << " frame queue" << endl);

      cur_buf->gps = gps;
      cur_buf->gps_n = gps_n;
      cur_buf->nac = nac;

      _work_queue->add_to_queue(1, cur_buf);
      cur_buf = 0;
      PV::set_pv(epics_state_var, STATE_NORMAL);
      PV::set_pv(epics_sec_var, (int)(time(0) - tdata));
    }

  return NULL;
}

void
daqd_c::queue_frame_checksum(const char *frame_filename, daqd_c::frame_type type,
                             FrameCPP::Common::MD5SumFilter &md5sum) {
    if (!frame_filename) return;

    raii::lock_guard<pthread_mutex_t> lock(_checksum_lock);
    if (!_checksum_file_transform_initted) {
        std::string src, dest;
        daqd_c::frame_type frame_types[] = {
                daqd_c::raw_frame,
                daqd_c::science_frame,
                daqd_c::minute_trend_frame,
                daqd_c::second_trend_frame,
        };
        dest = parameters().get("full_frame_checksum_dir", "");
        if (!dest.empty()) {
            _string_pair p(fsd.get_path(), dest);
            _checksum_file_transform.insert(_path_transform::value_type(daqd_c::raw_frame, p));
        }
        dest = parameters().get("science_frame_checksum_dir", "");
        if (!dest.empty()) {
            _string_pair p(science_fsd.get_path(), dest);
            _checksum_file_transform.insert(_path_transform::value_type(daqd_c::science_frame, p));
        }
        dest = parameters().get("minute_trend_frame_checksum_dir", "");
        if (!dest.empty()) {
            _string_pair p(trender.minute_fsd.get_path(), dest);
            _checksum_file_transform.insert(_path_transform::value_type(daqd_c::minute_trend_frame, p));
        }
        dest = parameters().get("trend_frame_checksum_dir", "");
        if (!dest.empty()) {
            _string_pair p(trender.fsd.get_path(), dest);
            _checksum_file_transform.insert(_path_transform::value_type(daqd_c::second_trend_frame, p));
        }
        _checksum_file_transform_initted = true;
    }

    _path_transform::iterator cur = _checksum_file_transform.find(type);
    if (cur == _checksum_file_transform.end())
        return;

    const static std::string md5(".md5");
    std::string filename(frame_filename);
    std::string::size_type dot = filename.rfind('.');
    std::string::size_type slash = filename.rfind('/');
    // if there is an extension, don't count a leading '.' for the extension
    if (dot != std::string::npos &&
        dot != 0 &&
        (slash == std::string::npos || slash < dot-1))
    {
        // erase the extension
        filename.erase(dot);
    }
    // add .md5
    filename += md5;

    if (cur->second.second != "" && cur->second.first.size() < filename.size()) {
        filename = cur->second.second + filename.substr(cur->second.first.size());
    }
    std::ostringstream os;
    os << md5sum;
    _checksum_queue.push_back(std::make_pair(filename, os.str()));
}

bool
daqd_c::dequeue_frame_checksum(std::string& filename, std::string& checksum)
{
    raii::lock_guard<pthread_mutex_t> lock(_checksum_lock);
    if (_checksum_queue.empty())
        return false;
    filename = _checksum_queue.front().first;
    checksum = _checksum_queue.front().second;
    _checksum_queue.pop_front();
    return true;
}

// Allocate and initialize main circular buffer.
int
daqd_c::start_main (int pmain_buffer_size, ostream *yyout)
{
  locker mon (this);

  if (b1) {
    *yyout << "main is already running" << endl;
    return 1;
  }

  main_buffer_size = pmain_buffer_size;

// If not using GPS defined directories, don't start if number of directories not set
  if (! filesys_c::gps_time_dirs)
  {
    if (! fsd.get_num_dirs ())
    {
      *yyout << "set the number of directories before running main" << endl;
      return 1;
    }
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

#if EPICS_EDCU == 1
  /* Epics display (Kb/Sec) */

  {
      unsigned int data_rate = 0;
      // Want to have the size of DAQ data only
      for (int i = 0; i < 2; i++) // Ifo number
         for (int j = 0; j < DCU_COUNT; j++)
              data_rate += dcuDAQsize[i][j];
      data_rate *= 16;
      data_rate /= 1024; // make it kilobytes
      PV::set_pv(PV::PV_DATA_RATE, data_rate);
  }
  /* Epics display: memory buffer look back */
  PV::set_pv(PV::PV_LOOKBACK_RAM, main_buffer_size);
#endif

  return 0;
}

#if EPICS_EDCU == 1

/// Start Epics DCU thread.
int
daqd_c::start_edcu (ostream *yyout)
{
  // error message buffer
  char errmsgbuf[80];
  pthread_attr_t attr;
  pthread_attr_init (&attr);
  pthread_attr_setstacksize (&attr, daqd.thread_stack_size);
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
  //  pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);

  int err_no;
  if (err_no = pthread_create (&edcu1.tid, &attr,
			       (void *(*)(void *))edcu1.edcu_static,
			       (void *) &edcu1)) {
    strerror_r(err_no, errmsgbuf, sizeof(errmsgbuf));
    pthread_attr_destroy (&attr);
    system_log(1, "EDCU pthread_create() err=%s", errmsgbuf);
    return 1;
  }
  pthread_attr_destroy (&attr);
  DEBUG(2, cerr << "EDCU thread created; tid=" <<  edcu1.tid << endl);
  edcu1.running = 1;
  return 0;
}

/// Start Epics IOC server. This one serves the daqd status Epics channels.
int
daqd_c::start_epics_server (ostream *yyout, char *prefix, char *prefix1, char *prefix2)
{
  // error message buffer
  char errmsgbuf[80];
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
    strerror_r(err_no, errmsgbuf, sizeof(errmsgbuf));
    pthread_attr_destroy (&attr);
    system_log(1, "Epics pthread_create() err=%s", errmsgbuf);
    return 1;
  } 
  pthread_attr_destroy (&attr);
  DEBUG(2, cerr << "Epics server thread created; tid=" <<  epics1.tid << endl);
  epics1.running = 1;
  return 0;
}

#endif

/// Start the producer thread. Its purpose is to extract the data from the 
/// receiver buffers and put it into the main circular buffer.
int
daqd_c::start_producer (ostream *yyout)
{
  // error message buffer
  char errmsgbuf[80];
  pthread_attr_t attr;
  pthread_attr_init (&attr);
  pthread_attr_setstacksize (&attr, daqd.thread_stack_size);
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
  //  pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);

  int err_no;
  if (err_no = pthread_create (&producer1.tid, &attr,
			       (void *(*)(void *))producer1.frame_writer_static,
			       (void *) &producer1)) {
    strerror_r(err_no, errmsgbuf, sizeof(errmsgbuf));
    pthread_attr_destroy (&attr);
    system_log(1, "producer pthread_create() err=%s", errmsgbuf);
    return 1;
  }
  pthread_attr_destroy (&attr);
  DEBUG(2, cerr << "producer created; tid=" << producer1.tid << endl);
  return 0;
}

/// Start full resolution frame saving thread.
int
daqd_c::start_frame_saver (ostream *yyout, int science)
{
  assert (b1);
// error message buffer
  char errmsgbuf[80];
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
        strerror_r(err_no, errmsgbuf, sizeof(errmsgbuf));
	pthread_attr_destroy (&attr);
	system_log(1, "pthread_create() err=%s", errmsgbuf);
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

void daqd_c::initialize_vmpic(unsigned char **_move_buf, int *_vmic_pv_len, put_dpvec *vmic_pv)
{
    int vmic_pv_len = 0;
    unsigned char *move_buf = 0;

    locker mon(this);

    int s = daqd.block_size / DAQ_NUM_DATA_BLOCKS_PER_SECOND;
    if (s < 128*1024) s = 128*1024;
  #ifdef USE_BROADCAST
    // Broadcast has 4096 byte header, so we allocate space for it here
    s += BROADCAST_HEADER_SIZE;
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
    move_buf += BROADCAST_HEADER_SIZE; // Keep broadcast header space in front of data space
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
          DEBUG(10, cerr << "Myrinet testpoint " << j << endl);
          DEBUG(10, cerr << "vmic_pv: " << hex << (unsigned long) vmic_pv [vmic_pv_len].src_pvec_addr << dec << "\t" << vmic_pv [vmic_pv_len].dest_vec_idx << "\t" << vmic_pv [vmic_pv_len].vec_len << endl);
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
  *_move_buf = move_buf;
  *_vmic_pv_len = vmic_pv_len;
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


void daqd_c::set_thread_priority (char *thread_name, char *thread_abbrev, int rt_priority, int cpu_affinity) {
    // get thread ID, thread label (limit to 16 characters)
    pid_t my_tid;
    char my_thr_label[16];
    my_tid = (pid_t) syscall(SYS_gettid);
    strncpy(my_thr_label,thread_abbrev,16);
    // Name the thread
    prctl(PR_SET_NAME,my_thr_label,0, 0, 0);

    std::string affinity_key = std::string(thread_abbrev) + std::string("_cpu");
    std::string priority_key = std::string(thread_abbrev) + std::string("_priority");

    cpu_affinity = daqd.parameters().get<int>(affinity_key, cpu_affinity);
    rt_priority = daqd.parameters().get<int>(priority_key, rt_priority);

    system_log(1, "%s thread - label %s pid=%d", thread_name, my_thr_label, (int) my_tid);
    // If priority is non-zero, add to the real-time scheduler at that priority

    {
        int policy = SCHED_FIFO;
        if (rt_priority <= 0) {
            rt_priority = 0;
            policy = SCHED_OTHER;
        }

        struct sched_param my_sched_param = { rt_priority };
        int set_stat;
        set_stat = pthread_setschedparam(pthread_self(), policy, &my_sched_param);
        if(set_stat != 0){
            system_log(1, "%s thread priority error %s",thread_name, strerror(set_stat));
        } else {
            system_log(1, "%s thread set to priority %d",thread_name, rt_priority);
        }
    }

    // set the affinity (if enough CPUs)
    // If set to 0, set allow thread on all CPUs
    //  If positive, count from 0, if negative, count from max
    // count CPUs
    {
        int numCPU, cpuId;
        numCPU = sysconf(_SC_NPROCESSORS_ONLN);
        if (cpu_affinity < 0) {
            cpuId = numCPU + cpu_affinity;
        } else {
            cpuId = cpu_affinity;
        }
        if( numCPU > 1 && (cpuId >= 0  && cpuId < numCPU)){
            cpu_set_t my_cpu_set;
            CPU_ZERO(&my_cpu_set);
            if (cpuId > 0) {
                CPU_SET(cpuId,&my_cpu_set);
            } else {
                for (int cur_cpu = 0; cur_cpu < numCPU; ++cur_cpu)
                    CPU_SET(cur_cpu, &my_cpu_set);
            }
            int set_stat;
            set_stat = pthread_setaffinity_np(pthread_self(),sizeof(cpu_set_t),&my_cpu_set);
            if(set_stat != 0){
               system_log(1, "%s thread setaffinity error %s",thread_name, strerror(set_stat));
            } else {
           system_log(1, "%s thread put on CPU %d",thread_name, cpuId);
            }
        }
     }
}

/// Print out usage message and exit
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

/// Parse command line arguments.
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

/// Use gcore command to produce core on fatal signals.
/// This is done, so that the program can run suid on execution.
void
shandler (int a) {
        char p[25];
	system_log(1,"going down on signal %d", a);
	int ignored_value = seteuid (0); // Try to switch to superuser effective uid
	sprintf (p,"/bin/gcore %d", getpid());
	// Works on Gentoo this way:
        //sprintf (p,"gdb --pid=%d --batch -ex gcore", getpid());
        int error = system (p);
}

#ifdef USE_SYMMETRICOM

#ifndef USE_IOP

int symmetricom_fd = -1;

/// Get current GPS time from the symmetricom IRIG-B card
unsigned long
daqd_c::symm_gps(unsigned long *frac, int *stt) {
    unsigned long t[3];
    ioctl (symmetricom_fd, IOCTL_SYMMETRICOM_TIME, &t);
    t[1] *= 1000;
    t[1] += t[2];
    if (frac) *frac = t[1];
    if (stt) *stt = 0;
    return  t[0] + daqd.symm_gps_offset;
}

/// See if the GPS card is locked.
bool
daqd_c::symm_ok() {
  unsigned long req = 0;
  ioctl (symmetricom_fd, IOCTL_SYMMETRICOM_STATUS, &req);
  printf("Symmetricom status: %s\n", req? "LOCKED": "UNCLOCKED");
  return req;
}

#else // ifdef USE_IOP

/// Get current GPS time from the IOP
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
/// Server main function.
main (int argc, char *argv [])
{
  int farg;
  int i;
  int stf;
  pthread_t startup_iprt;
  char startup_fname [filesys_c::filename_max];
  // error message buffer
  char errmsgbuf[80];

#ifdef USE_SYMMETRICOM
#ifndef USE_IOP
  symmetricom_fd =  open ("/dev/gpstime", O_RDWR | O_SYNC);
  if (symmetricom_fd < 0) {
	perror("/dev/gpstime");
	exit(1);
  }

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

  int set_nice = nice(-20);
  if(set_nice != 0){
        system_log(1, "Unable to set to nice = -20 -error %s\n",strerror(set_nice));
   } else {
        system_log(1, "Set daqd to nice = -20\n");
   }
  // Switch effective to real user ID -- can always switch back to saved effective
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
    const struct rlimit ulmt = {RLIM_INFINITY, RLIM_INFINITY};
    const struct rlimit small = {536870912, 536870912};
#ifndef NDEBUG
    // Want to dump unlimited core for debugging
    setrlimit (RLIMIT_CORE, &ulmt);
#endif
  }
#ifdef DAEMONIC
  {


    openlog (programname, LOG_PID, LOG_USER);
  }
#endif

  signal (SIGPIPE, SIG_IGN);

  fflush (stderr);
  fflush (stdout);

  pthread_mutex_init (&framelib_lock, NULL);

#ifdef USE_MX
  unsigned int max_mx_end;
  max_mx_end = open_mx();
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
#ifdef USE_GM
      cerr << "daqd built with USE_GM" << endl;
#endif
#ifdef USE_MX
      cerr << "daqd built with USE_MX" << endl;
#endif
#ifdef USE_UDP
      cerr << "daqd built with USE_UDP" << endl;
#endif
#ifdef USE_BROADCAST
      cerr << "daqd built with USE_BROADCAST" << endl;
#endif

      pthread_attr_t attr;
      pthread_attr_init (&attr);
      pthread_attr_setstacksize (&attr, daqd.thread_stack_size);
      pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
      /* change to long for 64-bit - KAT 2015-05-06 */
      long stderr_dup = dup (2);
      if (stderr_dup < 0)
	stderr_dup = 2;

      int err_no;
      if (err_no = pthread_create (&startup_iprt, &attr, (void *(*)(void *))interpreter_no_prompt, (void *) (stderr_dup << 16 | stf))) {
        strerror_r(err_no, errmsgbuf, sizeof(errmsgbuf));
	pthread_attr_destroy (&attr);
	system_log(1, "unable to spawn startup file interpreter: pthread_create() err=%s", errmsgbuf);
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

/// Shutdown the daqd server, exit.
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

// error message buffer  
  char errmsgbuf[80];

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
          strerror_r(errno, errmsgbuf, sizeof(errmsgbuf));
	  system_log(1, "shutdown: socket(); %s", errmsgbuf);
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
/*
 * This is a funny way of doing this, as the target platforms for daqd currently
 * support regexp.h, but it is also being developed on systems that do not.
 * So for now the goal is to allow it to work with the old depricated (but tested in
 * production) code unless it cannot.
 */
#ifdef HAVE_REGEXP_H

#define ESIZE 1024
char ipexpbuf [ESIZE];
char dec_num_expbuf [ESIZE];

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

/// See if the IP address is valid
int
daqd_c::is_valid_ip_address (char *str)
{
  return step (str, ipexpbuf);
}

#else
#ifdef HAVE_REGEX_H

/* this code replaces the depricated regexp.h code, but still needs to be tested */

regex_t ip_regex;
//regex_t dec_num_regex;

char *ipregexp ="^([0-9]\\{1,3\\}\\.)\\{3\\}[0-9]\\{1,3\\}$";
//char *dec_num_regexp = "^[0-9]\\{1,\\}$";


void
daqd_c::compile_regexp ()
{
  (void)regcomp(&ip_regex, ipregexp, REG_NOSUB);
  //(void)regcomp(&dec_num_regex, dec_num_regexp, REG_NOSUB);
}

/// See if the IP address is valid
int
daqd_c::is_valid_ip_address (char *str)
{
  return regexec(&ip_regex, str, 0, NULL, 0) == 0;
}



#else
#error Some form of regexp is still required, either <regexp.h> (depricated) or <regex.h>
#endif
#endif

int
daqd_c::is_valid_dec_number (char *str)
{
  // Somehow the regular expression stuff is broken on amd64 linux...
  char *endptr;
  long int res = strtol(str, &endptr, 10);
  return endptr == (str + strlen(str));
  //return step (str, dec_num_expbuf);
}
