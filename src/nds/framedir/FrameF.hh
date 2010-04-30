#ifndef FRAMEF_HH
#define FRAMEF_HH

#include "fferrors.hh"
#include <string>
#include <iostream>

/**  Class FrameF is a fast frame file reader. It understands the structure
  *  headers in a frame file and keeps track of the current structure and
  *  where it ends.
  *  @memo Fast frame file reader.
  *  @author John Zweizig
  *  @version 1.2; Modified July 27, 2000 
  */
class FrameF {
public:
  /**  Construct a Frame file reader and attach it to an existing stream. 
    *  The stream is specified.
    *  @memo File constructor.
    */
  FrameF(std::istream& in);

  /**  Destructor.
   */
  ~FrameF(void);

  //------------------------------------  Accessors
  /**  Test whether the input data sream is ready for reading.
    *  @memo Test input stream.
    *  @return true if input datastream is ready to read from.
    */
  bool   isOK(void) const;

  /**  Get the current structure ID.
    *  @memo Get the current structure ID.
    *  @return the current structure ID.
    */
  short  getID(void) const;

  /**  Get the current structure instance number.
    *  @memo Get the current structure instance number.
    *  @return the current structure instance number.
    */
  short  getInstance(void) const;

  /**  Get the current structure length. The length returned includes the
    *  structure header length.
    *  @memo Get the current structure length.
    *  @return Current structure length.
    */
  int    getLength(void) const;

  /**  Get the frame patch number.
    *  @memo Get the frame patch number.
    *  @return the frame patch number.
    */
  int    getPatch(void) const;

  /**  Get the version of the frame standard to which this file conforms.
    *  @memo Get the frame version.
    *  @return the frame version.
    */
  int    getVersion(void) const;

  //------------------------------------  Mutators
  void   ReadHeader(void) throw (BadFile);
  bool   NxStruct(void) throw (BadFile);
  short  getShort(void) throw (BadFile);
  float  getFloat(void) throw (BadFile);
  int    getInt(void) throw (BadFile);
  std::string getString(void) throw (BadFile);
  double getDouble(void) throw (BadFile);
  void   resetHeader(void);
  void   Seek(int off, std::ios::seekdir mode);
  void   Skip(int off);

public:
struct StrHdr {
    int   length;
    short ID;
    short Instance;
};

private:
  std::istream& mIn;
  bool     mSwap;
  bool     mHeaderOK;
  char     mData[40];
  StrHdr   mSHdr;
  int      mOffset;
};

inline int 
FrameF::getVersion() const {
    return mData[5];
}
inline int 
FrameF::getPatch() const {
    return mData[5];
}

inline bool
FrameF::isOK(void) const {
    return !mIn.bad();
}

inline short
FrameF::getID(void) const {
    return mSHdr.ID;
}

inline short 
FrameF::getInstance(void) const {
    return mSHdr.Instance;
}

inline int
FrameF::getLength(void) const {
    return mSHdr.length;
}
inline void
FrameF::resetHeader(void) {
    mHeaderOK = false;
}
#endif
