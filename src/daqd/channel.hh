#ifndef CHANNEL_HH
#define CHANNEL_HH

#include "channel.h"
#include <string.h>

typedef struct {
  enum { channel_group_name_max_len = MAX_CHANNEL_NAME_LENGTH };
  int num; // Group number
  char name [channel_group_name_max_len]; // Group name
} channel_group_t;

#define GDS_CHANNEL 1
#define GDS_ALIAS_CHANNEL 2
#define IS_GDS_ALIAS(a) ((a).gds & 2)
#define IS_GDS_SIGNAL(a) ((a).gds & 1)

/// Channel name structure
struct channel_t {
  /// archive channels group number is the same for all archived channels (except "obsolete" archive)
  const static int arc_groupn = 1000;

  /// Obsolete channel archive group number
  const static int obsolete_arc_groupn = 1001;

  /// archive channels are all assumed to be of the same data rate
  /// (archive have trend and minute-trend data only)
  /// This is phony number really, 16 Hz is used to put archive channels
  /// into the slow signal list
  const static int arc_rate = 16;

  enum { channel_name_max_len = MAX_CHANNEL_NAME_LENGTH , engr_unit_max_len = MAX_ENGR_UNIT_LENGTH };
  int chNum; ///< channel hardware address
  int seq_num;
  void *id;
#ifndef NO_SLOW_CHANNELS
  int slow;
#endif
  char name [channel_name_max_len];
  int sample_rate;
  int active;
  int trend; ///< Set if trend is calculated and saved for this channel
  int group_num; ///< Channel group number

  unsigned int bps; ///< Bytes per sample 
  long offset; ///< In the data block for this channel
  int bytes; ///< Size of the channel data in the block 
  int req_rate; ///< Sampling rate requested for this channel (used by net_writer_c)

#if defined(NETWORK_PRODUCER)
  int rcvbuf_offset;
#endif

  int dcu_id;
  int ifoid;
  int tp_node; ///< Testpoint node id
  long rm_offset; ///< Reflected memory offset for the channel data
#ifdef GDS_TESTPOINTS
  int gds; ///< Set to 1 if gds channel, set to 3 if gds alias channel
#endif
  
  daq_data_t data_type;

  float signal_gain;
  float signal_slope;
  float signal_offset;
  char signal_units [engr_unit_max_len]; ///< Engineering units

};

/// Long channel name structure
class long_channel_t : public channel_t {
public:
  enum { channel_name_max_len = MAX_LONG_CHANNEL_NAME_LENGTH };
  char name [channel_name_max_len];
  long_channel_t & operator =(const channel_t &a) {
     chNum = a.chNum;
     seq_num = a.seq_num;
     id = a.id;
#ifndef NO_SLOW_CHANNELS
     slow = a.slow;
#endif
     strcpy(name, a.name);
     sample_rate = a.sample_rate;
     active = a.active;
     trend = a.trend;
     group_num = a.group_num;

     bps = a.bps;
     offset = a.offset;
     bytes = a.bytes;
     req_rate = a.req_rate;

#if defined(NETWORK_PRODUCER)
     rcvbuf_offset = a.rcvbuf_offset;
#endif

     tp_node = a.tp_node; ///< Testpoint node id
     dcu_id = a.dcu_id;
     ifoid =  a.ifoid;
     rm_offset = a.rm_offset;
#ifdef GDS_TESTPOINTS
     gds = a.gds;
#endif
  
     data_type = a.data_type;

     signal_gain = a.signal_gain;
     signal_slope = a.signal_slope;
     signal_offset = a.signal_offset;
     strcpy(signal_units, a.signal_units);
     return *this;
  }
};

#endif
