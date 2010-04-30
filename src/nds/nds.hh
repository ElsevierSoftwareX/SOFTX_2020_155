#ifndef CDS_NDS_NDS_HH
#define CDS_NDS_NDS_HH

#include <string>
#include "debug.h"
#include "spec.hh"

namespace CDS_NDS {


typedef std::pair<unsigned long, unsigned long > ulong_pair;
typedef std::pair<unsigned long, ulong_pair > ulong_triple;

// order predicate
class cmp2 { public: int operator()(ulong_pair p1, ulong_pair p2) { return p1.first < p2.first; }};
class cmp3 { public: int operator()(ulong_triple p1, ulong_triple p2) { return p1.first < p2.first; }};

class Nds
{
public:
  Nds(std::string);
  bool run();
  bool rawMinuteTrend(); // striped raw minute trend data reader
  bool readFrameFileArchive(); // older code to read a frame files archive
  bool readTocFrameFileArchive(); // newer code to read a frame files archive
  bool combineMinuteTrend(); // minute trend combiner which does its processing if data is requested from more than one archive
  bool scanArchiveDir(std::vector<ulong_pair> *tstamps);

  // Determine program name (strip filesystem path)
  //
  static std::string basename( std::string s ) throw(){
    try {
      std::string s1(s.substr(s.find_last_of ("/")));
      return s1.replace(s1.find("/"),1,"");
    } catch (...) {
      return s;
    }
  }

  // Get the filesystem path without the name
  //
  static std::string dirname( std::string s ) throw(){
    try {
      return s.substr(0, s.find_last_of ("/"));
    } catch (...) {
      return s;
    }
  }


private:
  std::string mPipeFileName; // unix domain socket binding file name
  std::string mSpecFileName; // job specification file name
  std::string mResultFileName; // job result transmission image
  int mDataFd; // client data socket file descriptor
  Spec mSpec;  // job specification
  int seq_num;
  enum { filename_max = FILENAME_MAX };
};

typedef struct {
  union {int I; double D; float F;} min;
  union {int I; double D; float F;} max;
  int n; // the number of valid points used in calculating min, max, rms and mean
  double rms;
  double mean;
} trend_block_t;

typedef struct raw_trend_record_struct {
  unsigned int gps;
  trend_block_t tb;
} raw_trend_record_struct;


class data_span {
public:
  const static int point_period = 60;
  data_span():offs(0),gps(0),length(0){}
  unsigned long offs; // in the file
  unsigned long gps; // time stamp of the first data point
  unsigned long length; // number of data points available in this span
  inline bool operator<(data_span &y) { return gps < y.gps; } // compare spans
  inline void operator()(data_span &y) { length += y.length; } // accumulate
  inline unsigned long end_gps () { return gps+length*point_period; }
  inline unsigned long end_gps () const { return gps+length*point_period; }
  inline void extend (const data_span &s) { 
    unsigned long new_length = (s.end_gps () - gps)/point_period;
    if (new_length > length) length = new_length;
  }
};


class mapping_data_span : public data_span {
public:
  mapping_data_span () : data_span (), image_offs (0) {};
  unsigned long image_offs;
  inline mapping_data_span &operator=(const data_span &ds) {*(data_span *)this = ds; image_offs = 0;};
};


} // namespace

#endif
