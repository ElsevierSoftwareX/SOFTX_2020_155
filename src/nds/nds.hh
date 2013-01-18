#ifndef CDS_NDS_NDS_HH
#define CDS_NDS_NDS_HH

#include <string>
#include "debug.h"
#include "spec.hh"

namespace CDS_NDS {

/// A pair of unsigned longs
typedef std::pair<unsigned long, unsigned long > ulong_pair;
typedef std::pair<unsigned long, ulong_pair > ulong_triple;

/// Order predicate
class cmp2 { public: int operator()(ulong_pair p1, ulong_pair p2) { return p1.first < p2.first; }};
/// Order predicate
class cmp3 { public: int operator()(ulong_triple p1, ulong_triple p2) { return p1.first < p2.first; }};

/// Network Data Server to process requests to read data from the data files.
class Nds {
public:
  /// Creates an Nds object.
  Nds(std::string);
  /// Main processing loop.
  bool run();
  /// Striped raw minute trend data reader.
  /// @param[in] path raw minute trend archive directory
  bool rawMinuteTrend(std::string path);
  /// Older code to read a frame file archive.
  bool readFrameFileArchive();
  /// Newer code to read a frame file archive.
  bool readTocFrameFileArchive();
  /// Minute trend combiner code, used when data is requested from multiple archives.
  bool combineMinuteTrend();
  /// Discover archive data subdirectories.
  bool scanArchiveDir(std::vector<ulong_pair> *tstamps);
  /// Determine program name (strip filesystem path).
  static std::string basename( std::string s ) throw(){
    try {
      std::string s1(s.substr(s.find_last_of ("/")));
      return s1.replace(s1.find("/"),1,"");
    } catch (...) {
      return s;
    }
  }

  /// Get the filesystem path without the name.
  static std::string dirname( std::string s ) throw(){
    try {
      return s.substr(0, s.find_last_of ("/"));
    } catch (...) {
      return s;
    }
  }

private:
  /// UNIX domain socket binding file name.
  std::string mPipeFileName;
  /// Job specification file name.
  std::string mSpecFileName;
  /// Job result transmission image.
  std::string mResultFileName;
  /// Client data socket file descriptor.
  int mDataFd;
  /// Job specification.
  Spec mSpec;
  /// Data block sequence number.
  int seq_num;
  /// Maximum file name length.
  enum { filename_max = FILENAME_MAX };
};

/// Raw minute trend data substructure.
typedef struct {
  union {int I; double D; float F;} min;
  union {int I; double D; float F;} max;
  int n; // the number of valid points used in calculating min, max, rms and mean
  double rms;
  double mean;
} trend_block_t;

/// Raw minute trend data file structure.
typedef struct raw_trend_record_struct {
  unsigned int gps;
  trend_block_t tb;
} raw_trend_record_struct;

/// Data span using GPS time stamp and length.
/// Used for processing raw minute trend data.
class data_span {
public:
  /// Class is only used for the minute trend data.
  const static int point_period = 60;
  /// Default constructor.
  data_span():offs(0),gps(0),length(0){}
  /// Offset in the file.
  unsigned long offs;
  /// Time stamp of the first data point.
  unsigned long gps;
  /// The number of data points available in this span.
  unsigned long length;
  /// Compare two data spans by their GPS start times.
  inline bool operator<(data_span &y) { return gps < y.gps; }
  /// Accumulate the span length.
  inline void operator()(data_span &y) { length += y.length; }
  /// Calculate and returns this span's ending GPS time.
  inline unsigned long end_gps () { return gps+length*point_period; }
  /// Calculate and returns this span's ending GPS time.
  inline unsigned long end_gps () const { return gps+length*point_period; }
  /// Extend the data span.
  inline void extend (const data_span &s) { 
    unsigned long new_length = (s.end_gps () - gps)/point_period;
    if (new_length > length) length = new_length;
  }
};

/// Raw minute trend data span with added mapping data.
class mapping_data_span : public data_span {
public:
  /// Default constructor.
  mapping_data_span () : data_span (), image_offs (0) {};
  /// Offset into the image.
  unsigned long image_offs;
  /// Copy the data span and zero out the offset.
  inline mapping_data_span &operator=(const data_span &ds) {*(data_span *)this = ds; image_offs = 0;};
};

} // namespace

#endif
