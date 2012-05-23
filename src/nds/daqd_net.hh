#ifndef DAQD_NET_HH
#define DAQD_NET_HH

#include <limits.h>
#include <vector>
#include <list>
#include <map>
#include "ldas/ldasconfig.hh"

#if FRAMECPP_DATAFORMAT_VERSION >= 6

#include "ldas/ldasconfig.hh"
#include "framecpp/Common/FrameSpec.hh"
#include "framecpp/Common/CheckSum.hh"
#include "framecpp/Common/IOStream.hh"
#include "framecpp/Version8/FrameStream.hh"
#include "framecpp/Common/FrameBuffer.hh"


#include "framecpp/FrameCPP.hh"

#include "framecpp/FrameH.hh"
#include "framecpp/FrAdcData.hh"
#include "framecpp/FrRawData.hh"
#include "framecpp/FrVect.hh"

#include "framecpp/Dimension.hh"

#else

#ifdef LDAS_VERSION_NUMBER
#include "framecpp/Version6/FrameH.hh"
#else
#include "framecpp/frame.hh"
#if FRAMECPP_DATAFORMAT_VERSION > 4
#include "framecpp/Version6/FrameH.hh"
#endif
#endif
#endif

#include "spec.hh"


namespace CDS_NDS {

// DAQD network protocol reconfig data
typedef struct reconfig_data_t {
  float signal_offset;
  float signal_slope;
  unsigned int signal_status;
} reconfig_data_t;

class daqd_net {
public:

  // Subjob state
  class SubjobReadState {
  public:
    SubjobReadState(const std::vector<unsigned int> &v)
      : finished(false), seq_num(-1),
	block_list(), signalIndices(v)
      {}
    // Set if subjob finished
    bool finished;
    // Sequence number send by the sub-NDS when subjob finished
    int seq_num;
    // Subjob data block (input block)
    class BT {
    public:
      BT() : length(0), dt(0), data(0) {}
      BT(unsigned long l, char *p) : length(l), dt(0), data(p) {}
      unsigned long length; // 'data' length
      unsigned long dt;     // used for block splitting in combine_send_data()
      char * data;          // malloced block's data
      void print_debug_info() {
	std::cerr << "BT: length=" << length << "; dt=" << dt << std::endl;
      }
    };
    typedef std::list<BT> BLT;
    BLT block_list;
    typedef std::list<BT>::const_iterator BLI;
    typedef std::list<BT>::iterator BLINC;
    // Add new block to the list
    void add_block(unsigned long size, char *block) {
      block_list.insert (block_list.end(), BT(size, block));
    }
    // Signal indices in 'spec.getSignalNames()' vector
    std::vector<unsigned int> signalIndices;
  };

  // See if the data block 'd' is a reconfiguration block
  inline bool is_reconfig_block(const char * d) const {
    unsigned long l;
    memcpy(&l,d,sizeof(l));
    return ntohl (l) == 0xffffffff;
  }

  // Map FIFO read file descriptor onto the vector of signal indices
  typedef std::map<int, std::vector<unsigned int> > CPT;
  typedef std::map<int, std::vector<unsigned int> >::const_iterator CPMI;
  // Map FIFO read file descriptor into the read state object
  typedef std::map<int, SubjobReadState> CRST;
  typedef std::map<int, SubjobReadState>::const_iterator CRMI;
  typedef std::map<int, SubjobReadState>::iterator CRMINC;

  daqd_net (int fd, Spec &spec);
  daqd_net (int fd, Spec &spec, CPT &v); // Combination processing
  ~daqd_net ();

#ifdef LDAS_VERSION_NUMBER
  bool send_data (FrameCPP::Version::FrameH &frame, const char *file_name, unsigned frame_number, int *seq_num);
#else
#if FRAMECPP_DATAFORMAT_VERSION > 4
  bool send_data (FrameCPP::Version::FrameH &frame, const char *file_name, unsigned frame_number, int *seq_num);
#else
  bool send_data (FrameCPP::Frame &frame, const char *file_name, unsigned frame_number, int *seq_num);
#endif
#endif
  bool finish ();
  bool comb_read(int fildes);
  bool comb_subjob_done(int fildes, int seq_num);
  bool send_reconfig_data(Spec &spec);
  bool send_reconfig_block();

  static int read_bytes (int fd, char *cptr, int numb);
  static unsigned long read_long (int fd);

  static inline short averaging (short *v, int num) {
    assert (num > 0 && num < SHRT_MAX);
    long res = 0;
    for (int i = 0; i < num; i++)
      res += v [i];

    return (short) (res / num);
  }

  static inline int averaging (int *v, int num) {
    assert (num > 0 && num < SHRT_MAX);
    long long res = 0;
    for (int i = 0; i < num; i++)
      res += v [i];

    return (int) (res / num);
  }

  static inline float averaging (float *v, int num) {
    assert (num > 0 && num < SHRT_MAX);
    double res = 0;
    for (int i = 0; i < num; i++)
      res += v [i];

    return (float) (res / num);
  }

  static inline double averaging (double *v, int num) {
    assert (num > 0 && num < SHRT_MAX);
    long double res = 0;
    for (int i = 0; i < num; i++)
      res += v [i];

    return (double) (res / num);
  }

  // Merge received data and send
  bool combine_send_data();

private:
  void *buf;
  unsigned long ndata;
  unsigned num_signals;
  reconfig_data_t *reconfig_data;
  bool first_time;
  int mDataFd; // client data socket file descriptor
  const Spec &mSpec;  // job specification
  const static unsigned long buf_size = 1024*1024;
  unsigned long transmission_block_size; // merger output block size for one second of time
  CRST subjobReadStateMap; // merger input state
  unsigned long seq_num; // merged output block's sequence number
  std::vector<unsigned long> mSignalBlockOffsets; // merged data block's signal offsets (for one unit of data)
};

} // namespace
#endif
