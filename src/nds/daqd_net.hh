#ifndef DAQD_NET_HH
#define DAQD_NET_HH

#include <limits.h>
#include <vector>
#include <list>
#include <map>
#ifdef USE_LDAS_VERSION
#include "ldas/ldasconfig.hh"
#endif
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
#include "spec.hh"


namespace CDS_NDS {

/// DAQD network protocol reconfig data.
typedef struct reconfig_data_t {
  float signal_offset;
  float signal_slope;
  unsigned int signal_status;
} reconfig_data_t;

/// DAQD protocol communication back to client.
class daqd_net {
public:

  /// Subjob state.
  class SubjobReadState {
  public:
    /// Constructor using a set of signal indices.
    SubjobReadState(const std::vector<unsigned int> &v)
      : finished(false), seq_num(-1),
	block_list(), signalIndices(v)
      {}
    /// Set if subjob finished.
    bool finished;
    /// Sequence number send by the sub-NDS when subjob finished.
    int seq_num;
    //// Subjob data block (input block).
    class BT {
    public:
      /// Default constructor.
      BT() : length(0), dt(0), data(0) {}
      /// Constructor with length 'l' and data pointser 'p'.
      BT(unsigned long l, char *p) : length(l), dt(0), data(p) {}
      /// 'data' length.
      unsigned long length; 
      /// Used for block splitting in combine_send_data().
      unsigned long dt;
      /// malloced block's data.
      char * data;
      /// Output some debugging information.
      void print_debug_info() {
	std::cerr << "BT: length=" << length << "; dt=" << dt << std::endl;
      }
    };
    /// A linked list of subjob data blocks.
    typedef std::list<BT> BLT;
    /// A linked list of subjob data blocks.
    BLT block_list;
    /// A const iterator on a linked list of subjob data blocks.
    typedef std::list<BT>::const_iterator BLI;
    /// An iterator on a linked list of subjob data blocks.
    typedef std::list<BT>::iterator BLINC;
    /// Add new block to the list.
    void add_block(unsigned long size, char *block) {
      block_list.insert (block_list.end(), BT(size, block));
    }
    /// Signal indices in 'spec.getSignalNames()' vector.
    std::vector<unsigned int> signalIndices;
  };

  /// See if the data block 'd' is a reconfiguration block.
  inline bool is_reconfig_block(const char * d) const {
    unsigned long l;
    memcpy(&l,d,sizeof(l));
    return ntohl (l) == 0xffffffff;
  }

  /// Map subjob FIFO read file descriptor onto the vector of signal indices.
  typedef std::map<int, std::vector<unsigned int> > CPT;
  typedef std::map<int, std::vector<unsigned int> >::const_iterator CPMI;
  /// Map subjob FIFO read file descriptor into the read state object
  typedef std::map<int, SubjobReadState> CRST;
  typedef std::map<int, SubjobReadState>::const_iterator CRMI;
  typedef std::map<int, SubjobReadState>::iterator CRMINC;

  /// Constructor.
  daqd_net (int fd, Spec &spec);
  /// Combination processing.
  daqd_net (int fd, Spec &spec, CPT &v);
  /// Default destructor.
  ~daqd_net ();

  /// \brief Send frame data to the client.
  ///     @param[in] &frame Frame object to send.
  ///     @param[in] *file_name Name of the file where the frame was read from.
  ///	  @param[in] frame_number Frame number of the frame within the frame file.
  ///	  @param[in, out] *seq_num Current data block sequencer number, gets incremented by one for each block transmitted.
  ///     @return True if data was sent successfully.
  bool send_data (FrameCPP::Version::FrameH &frame, const char *file_name, unsigned frame_number, int *seq_num);
  /// Finalize the send.
  bool finish ();
  /// \brief Read a block from fildes and do combination processing.
  ///	@param[in] fildes File descriptor to read from.
  ///	@return True if read, processed and transmitted the data successfully.
  bool comb_read(int fildes);
  /// Finish one subjob processing.
  ///	@param[in] fildes File descriptor to read from.
  ///	@param[in] seq_num Current data block sequence number.
  ///	@return True if read, processed and transmitted the data successfully.
  bool comb_subjob_done(int fildes, int seq_num);
  /// Assign the data an send one metadata reconfiguration block.
  ///	@param[in] &spec Job specification.
  ///	@return	True if sent the block successfully.
  bool send_reconfig_data(Spec &spec);
  /// Send one metadata reconfiguration block.
  ///	@return	True if sent the block successfully.
  bool send_reconfig_block();
  /// Read the number of bytes from the file descriptor.
  ///	@param[in] fd	File descriptor to read from.
  ///	@param[in] *cptr  Pointer where to write the bytes.
  ///	@param[in] numb	The number of bytes to read.
  ///	@return The number of bytes read.
  static int read_bytes (int fd, char *cptr, int numb);
  /// Read one 4 byte long integer from the file descriptor.
  /// The four bytes are converted to long with ntohl() call.
  ///	@param[in] fd File descirptor to read from.
  ///	@return The integer rad from the file.
  static unsigned long read_long (int fd);

  /// Average an array of shorts.
  /// 	@param[in] *v	A pointer to the array.
  ///	@param[in] num  The number of elements in the array.
  ///	@return The average of the elements in the array.
  static inline short averaging (short *v, int num) {
    assert (num > 0 && num < SHRT_MAX);
    long res = 0;
    for (int i = 0; i < num; i++)
      res += v [i];

    return (short) (res / num);
  }

  /// Average an array of ints.
  /// 	@param[in] *v	A pointer to the array.
  ///	@param[in] num  The number of elements in the array.
  ///	@return The average of the elements in the array.
  static inline int averaging (int *v, int num) {
    assert (num > 0 && num < SHRT_MAX);
    long long res = 0;
    for (int i = 0; i < num; i++)
      res += v [i];

    return (int) (res / num);
  }

  /// Average an array of floats.
  ///   @param[in] *v   A pointer to the array.
  ///   @param[in] num  The number of elements in the array.
  ///   @return The average of the elements in the array.
  static inline float averaging (float *v, int num) {
    assert (num > 0 && num < SHRT_MAX);
    double res = 0;
    for (int i = 0; i < num; i++)
      res += v [i];

    return (float) (res / num);
  }

  /// Average an array of doubles.
  ///   @param[in] *v   A pointer to the array.
  ///   @param[in] num  The number of elements in the array.
  ///   @return The average of the elements in the array.
  static inline double averaging (double *v, int num) {
    assert (num > 0 && num < SHRT_MAX);
    long double res = 0;
    for (int i = 0; i < num; i++)
      res += v [i];

    return (double) (res / num);
  }

  /// Merge received data and send
  /// @return	True if transmitted all the data successfully.
  bool combine_send_data();

private:
  void *buf;
  unsigned long ndata;
  unsigned num_signals;
  reconfig_data_t *reconfig_data;
  bool first_time;
  int mDataFd; ///< Client data socket file descriptor.
  const Spec &mSpec;  ///< Job specification.
  const static unsigned long buf_size = 1024*1024;
  unsigned long transmission_block_size; ///< Merger output block size for one second of time.
  CRST subjobReadStateMap; ///< Merger input state.
  unsigned long seq_num; ///< Merged output block's sequence number.
  std::vector<unsigned long> mSignalBlockOffsets; ///< Merged data block's signal offsets (for one unit of data).
};

} // namespace
#endif
