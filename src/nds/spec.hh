#ifndef CDS_NDS_SPEC_HH
#define CDS_NDS_SPEC_HH

#include <string>
#include <iostream>
#include <vector>
#include "config.h"
#include "debug.h"
#include <assert.h>

namespace CDS_NDS
{
  class Spec;
}

namespace CDS_NDS {

class Spec
{
public:
  typedef enum {FullData, TrendData, MinuteTrendData, RawMinuteTrendData} DataClassType;

  typedef enum {
    _undefined = 0,
    _16bit_integer = 1,
    _32bit_integer = 2,
    _64bit_integer = 3,
    _32bit_float = 4,
    _64bit_double = 5,
    _32bit_complex = 6
  } DataTypeType;

  inline static const std::string dataTypeString( DataTypeType d ) {
    switch (d) {
    case  _undefined: return "unknown";
    case  _16bit_integer: return "_16bit_integer";
    case _32bit_integer: return "_32bit_integer";
    case _64bit_integer: return "_64bit_integer";
    case _32bit_float: return "_32bit_float";
    case _64bit_double: return "_64bit_double";
    case _32bit_complex: return "_32bit_complex";
    }
  }

  Spec();


#if 0
  Spec(DataClassType dataType, unsigned long startGpsTime, unsigned long endGpsTime,
       const std::vector<std::string> &signalNames, const std::vector<DataTypeType> &signalTypes,
       const std::string &archiveDir, const std::string &prefix, const std::string &suffix,
       const std::vector<std::pair<unsigned long, unsigned long> > &gps);
#endif

  bool parse(std::string);

  class AddedArchive {
  public:
    AddedArchive() : prefix(), suffix(), gps() {}
    AddedArchive(std::string &d, std::string &p,
		 std::string &s, std::vector<std::pair<unsigned long, unsigned long> > &g)
      : dir(d), prefix(p), suffix(s), gps(g) {}
    // see whether all fields are set
    inline bool complete() {
      return (dir.length() > 0
	      && prefix.length() > 0
	      && suffix.length() > 0
	      && gps.size() > 0);
    }
    std::string dir;
    std::string prefix;
    std::string suffix;
    std::vector<std::pair<unsigned long, unsigned long> >gps;
  };

  DataClassType getDataType() const throw() { return mDataType; };
  unsigned long getStartGpsTime() const throw() { return mStartGpsTime; };
  unsigned long getEndGpsTime() const throw() { return mEndGpsTime; };
  std::string getFilter() const throw() { return mFilter; };
  const std::vector<std::string>& getSignalNames() const throw() { return mSignalNames; };
  const std::vector<float>& getSignalOffsets() const throw() { return mSignalOffsets; };
  const std::vector<float>& getSignalSlopes() const throw() { return mSignalSlopes; };
  const std::vector<unsigned int>& getSignalRates() const throw() { return mSignalRates; };
  const std::vector<DataTypeType>& getSignalTypes() const throw() { return mSignalTypes; };
  const std::vector<unsigned int>& getSignalBps() const throw() { return mSignalBps; };
  const std::string getDaqdResultFile() const throw() { return mDaqdResultFile; };
  const std::string getArchiveDir() const throw() { return mArchiveDir; };
  const std::string getArchivePrefix() const throw() { return mArchivePrefix; };
  const std::string getArchiveSuffix() const throw() { return mArchiveSuffix; };
  const std::vector<AddedArchive>& getAddedArchives() const throw() { return mAddedArchives; };
  const std::vector<std::string>& getAddedFlags() const throw() { return mAddedFlags; };
  static const std::vector<std::string> split(std::string) throw();
  const std::vector<std::pair<unsigned long, unsigned long> > getArchiveGps() const throw() { return mArchiveGps; };

  void setMainArchive(const AddedArchive &a) {
    mArchiveDir = a.dir;
    mArchivePrefix = a.prefix;
    mArchiveSuffix = a.suffix;
    mArchiveGps = a.gps;
  }
  void setDataType(DataClassType t) { mDataType = t; }
  void setSignalRates(const std::vector<unsigned int> &v) { mSignalRates = v; }
  void setSignalSlopes(double val) {
       for (int i = 0; i < mSignalSlopes.size(); i++) mSignalSlopes[i] = val;
  }

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

private:
  DataClassType mDataType; // type of the data
  unsigned long mStartGpsTime; 
  unsigned long mEndGpsTime;
  std::string mFilter; // how to decimate data
  std::vector<std::string> mSignalNames; // list of signals
  std::vector<unsigned int> mSignalRates; // list of requested signal rates
  std::vector<unsigned int> mSignalBps; // list of signals' bytes per sample
  std::vector<DataTypeType> mSignalTypes; // list of signal data types
  std::vector<float> mSignalOffsets; // list of signal offset conversion values
  std::vector<float> mSignalSlopes; // list of signal slope conversion values
  std::string mDaqdResultFile; // results produced by the DAQD
  std::string mArchiveDir; // location of main data archive
  std::string mArchivePrefix;
  std::string mArchiveSuffix;
  std::vector<AddedArchive> mAddedArchives; // list of added archives (identified by dir name)
  std::vector<std::string> mAddedFlags; // establishes signal membership in archives, "0" means main archive, dir name for added archive
  // Gps times for the data archive "Data*" directories
  std::vector<std::pair<unsigned long, unsigned long> > mArchiveGps;
};

} // namespace

#endif
