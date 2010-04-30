#ifndef FRAMEDIR_HH
#define FRAMEDIR_HH

//
// Frame finder class.
//
#include <map>
#include <string>
#include "Time.hh"
#include "Interval.hh"
#include "fferrors.hh"

/** @name Frame Directory
  * @memo Build and manipulate a frame file directory.
  */
//@{

/**  The frame file data structure contains information about the frames 
  *  in a specified frame file. A flag in the ffData object specifies 
  *  whether the information has been gathered by reading the file or by
  *  inference.
  *  @memo Frame file data.
  *  @author J. Zweizig
  *  @version V1.0; Modified February 3, 2000
  */
   struct ffData {
   public:
      typedef Time::ulong_t gps_t;
      typedef unsigned long count_t;
   
    /**  Default constructor.
      *  @memo Default constructor.
      */
      ffData(void);
   
    /**  Construct a table entry for a specified file. 
      *  @memo Data constructor.
      *  @param File    Name of frame file
      *  @param time    Start time of first frame
      *  @param nFrames Number of frames in the file.
      *  @param Dt      Length of time spanned by each frame.
      *  @param DataOK  True if measured data are supplied.
      */
      ffData(const char* File, const Time& time, count_t nFrames=1, 
            Interval Dt=Interval(1.0), bool DataOK=false);
   
    /** Copy an ffData object.
      * @memo Copy constructor.
      */
      ffData(const ffData& x);
   
    /**  Copy an ffData object.
      *  @memo Assignment operator.
      *  @param x ffData object to be copied.
      *  @return reference to the modified ffData.
      */
      ffData& operator=(const ffData& x);
   
    /**  Get the length of time covered by a single frame in the file.
      *  @memo Length of time covered by a frame.
      *  @return Interval covered by a frame.
      */
      Interval getDt(void) const;
   
    /**  Get the name of the filed described by this object.
      *  @memo File name.
      *  @return Pointer to the file name.
      */
      const char* getFile(void) const;
   
    /**  Get the number of frames in the file.
      *  @memo Number of frames.
      *  @return Number of frames in the file.
      */
      count_t getNFrames(void) const;
   
    /**  Get the absolute starting time of the data in the file.
      *  @memo Start time.
      *  @return Start time for the first frame in the file.
      */
      Time getStartTime(void) const;
   
    /**  Get the absolute end time of the data in the file.
      *  @memo End time.
      *  @return End time for the last frame in the file.
      */
      Time getEndTime(void) const;
   
    /**  Test whether the information has been gathered by reading
      *  the file or inferred from default values or other files in 
      *  the directory.
      *  @memo Test data quality.
      *  @return true if data have been gathered from the file.
      */
      bool isValid(void) const;
   
    /**  Get the starting GPS second.
      *  @memo GPS start time.
      *  @return Start time in GPS seconds of the first frame.
      */
      gps_t getStartGPS(void) const;
   
    /**  Get the end GPS second.
      *  @memo GPS end time.
      *  @return End time of the first frame in GPS seconds.
      */
      gps_t getEndGPS(void) const;
   
   private:
    /** File name including directory name.
     */
      std::string mFile;
   
    /** Starting GPS time of first frame in file.
      */
      Time mStartGPS;
   
    /**  Number of frames in the file.
      */
      count_t mNFrames;
   
    /**  Length of each frame in the file.
      */
      Interval mDt;
   
    /**  Data comes from reading frame
     */
      bool mDataOK;
   };

   inline 
   ffData::ffData(const char* File, const Time& time, count_t nFrames, 
                 Interval Dt, bool DataOK) 
   : mFile(File), mStartGPS(time), mNFrames(nFrames), mDt(Dt), mDataOK(DataOK)
   {}

   inline  
   ffData::ffData(void) 
   : mDataOK(false)
   {}

   inline  
   ffData::ffData(const ffData& x) 
   : mFile(x.mFile), mStartGPS(x.mStartGPS), mNFrames(x.mNFrames), 
   mDt(x.mDt), mDataOK(x.mDataOK)
   {}

   inline ffData&
   ffData::operator=(const ffData& x) {
      mFile     = x.mFile;
      mStartGPS = x.mStartGPS;
      mNFrames  = x.mNFrames; 
      mDt       = x.mDt; 
      mDataOK   = x.mDataOK;
      return *this;
   }

   inline Interval 
   ffData::getDt(void) const {
      return mDt;
   }

   inline const char* 
   ffData::getFile(void) const {
      return mFile.c_str();
   }

   inline ffData::count_t 
   ffData::getNFrames(void) const {
      return mNFrames;
   }

   inline bool 
   ffData::isValid(void) const {
      return mDataOK;
   }

   inline Time 
   ffData::getStartTime(void) const {
      return mStartGPS;
   }

   inline Time 
   ffData::getEndTime(void) const {
      return mStartGPS + mDt*mNFrames;
   }

   inline ffData::gps_t
   ffData::getStartGPS(void) const {
      return mStartGPS.getS();
   }

   inline ffData::gps_t
   ffData::getEndGPS(void) const {
      return getEndTime().getS();
   }

/**  The frame directory class is used to keep a directory of frame files
  *  in memory. Each frame is represented by an #ffData# object indexed 
  *  by the starting GPS time. The data in the #ffData# structure are set 
  *  as follows:
  *  \begin{itemize}
  *  \item The start time is inferred from the last numeric string in the 
  *        file name.
  *  \item The frame length and number of frames are assumed to be the 
  *        same as the preceding file if this is consistent with the time
  *        to the following file.
  *  \item If the inferred time indicates a gap between a file and the
  *        following file, the starting time and frame length are read from 
  *        the first frame header, and the number of frames in the file is
  *        read from the #FrEndOfFile# structure.
  *  \end{itemize}
  *  The frame file descriptors may be accessed sequentially using a 
  *  #file_iterator# type variable.
  *  @memo Frame Finder Class
  *  @author J. Zweizig
  *  @version V1.0; Modified February 3, 2000
  */
   class FrameDir {
   
   public:
      typedef Time::ulong_t gps_t;
      struct cmpr {
         bool operator()(gps_t a, gps_t b) const {
            return a<b;}
      };
      typedef std::map<gps_t, ffData, cmpr> dmap_t;
   
   /**  Iterator provides sequential access to the data on the files in the
    *  directory.
    *  @memo File data iterator.
    */
      typedef dmap_t::const_iterator file_iterator;
   
   public:
   /**  Erase all FrameDir entries.
    *  @memo FrameDir destructor.
    */
      virtual ~FrameDir(void);
   
   /**  Construct an empty frame directory.
    *  @memo Empty directory constructor.
    */
      FrameDir();
   
   /**  Construct a Frame directory and fill it with data from files 
    *  matching the specified specification. The frame file specification
    *  may contain wild-cards in the file name but not in the directory
    *  name.
    *  @memo Construct and fill a directory.
    *  @param dir Frame file specification.
    *  @param delayed Check files when needed, otherwise do it after add.
    */
      explicit FrameDir(const char* dir, bool delayed = false);
   
   //------------------------------------  Accessors
   /**  Get the data for the file that covers time t.
    *  @memo Get file data by time.
    */
      const ffData& find(const Time& t) const throw(NoData);
   
   /**  Get the start GPS of the first period on or after the specified 
    *  time.
    *  @memo find the next data segment.
    *  @return Start time of the first data segment after the specified time.
    *  @param time Begin time for search.
    */
      gps_t getStart(gps_t time) const;
   
   /**  Get the end GPS of the first period on or after the specified time.
    *  @memo Find the end of the next data segment.
    *  @return End time of the first data segment after the specified time.
    *  @param time Begin time for search.
    */
      gps_t getLast(gps_t time) const;
   
   /**  Get the specified debug level.
    *  @memo Find the end of the n.
    *  @return End time of the first data segment after the specified time.
    */
      int   getDebug(void) const;
   
   /**  Get a file_iterator pointing the first (earliest) frame file entry 
    *  in the directory.
    *  @memo Get a iterator pointing to the sequential beginning.
    *  @return A file_iterator pointing to the beginning of the directory.
    */
      file_iterator begin(void) const;
   
   /**  Get a file_iterator pointing the end of the frame file directory.
    *  @memo Get a iterator pointing to the directory end.
    *  @return A #file_iterator# pointing to the end of the directory.
    */
      file_iterator end(void) const;
   
   /**  Returns the number of entries in the FrameDir list.
    *  @memo Number of entries.
    */
      int size() const {
         return mList.size(); }
   /**  Returns true if there are no entries in the FrameDir list.
    *  @memo Empty list?.
    */
      bool empty() const {
         return mList.empty(); }
   
   //------------------------------------  Mutators
   
   /**  Add all the specified frame files to the Frame Directory. The 
    *  File specification can contain one or more wild cards in the 
    *  File name but not in the directory name.
    *  @memo Add one or more frame files.
    *  @param dir File name(s) to be added to the FrameDir.
    *  @param delayed Check files when needed, otherwise do it after add.
    */
      void add(const char* dir, bool delayed = false);
   
   /**  Add a single file to the frame directory. The file name must not
     *  contain wild-cards and must conform to the LIGO frame file naming
     *  standard i.e. the name must have the following form:
     *  #<directory>/<ifo-name>-<start-gps>[-<nsec>][.<extension>]#. To
     *  allow backward compatibility, the data length and extension may 
     *  be omitted.
     *  @memo Add a file to the FrameDir.
     *  @param File Name of file to be added to the list.
     */
      void addFile(const char* File) throw(BadFile);
   
   /**  Delete all file descriptors from the FrameDir list.
     *  @memo Delete all entries.
     */
      void clear(void);
   
   /**  Delete the frame file descriptor for the specified time.
    *  @memo Delete a frame file entry.
    *  @param time GPS time of start of 
    */
      void erase(gps_t time);
   
   /**  Remove all specified files from a frame directory.
    *  @memo Remove specified file descriptors.
    *  @param dir Specification for files to be removed.
    */
      void remove(const char* dir);
   
   /**  Set the debug printout threshold to a specified level.
    *  @memo Set debug level.
    *  @param Level New debug printout level.
    */
      void setDebug(int Level);
   
   private:
   /**  Checkup mode specifier.
    */
      enum CheckLevel {none, gapsOnly, allData};
   
   /**  Check all entries in the frame directory list for contiguity.
    *  Depending on the level of checking demanded, files are inspected 
    *  in all cases or only to resolve data gaps.
    *  @memo Check frame coverage.
    *  @param Level of checking to be performed.
    */
      void checkData(CheckLevel lvl=gapsOnly);
      void checkData(CheckLevel lvl=gapsOnly) const;
   
   /**  Get a copy of the table entry for a specified file.
    *  @memo Get specified file data.
    */
      ffData getFileData(const char* File) throw(BadFile);
   
   private:
   /**  Debug printout level.
    */
      int    mDebug;
   /**  Frame added since last check.
    */
      mutable bool   mDirty;
   
   /**  Frame file directory, keyed by GPS start times.
    */
      dmap_t mList;
   };

   inline FrameDir::file_iterator
   FrameDir::begin(void) const {
      if (mDirty) checkData();
      return mList.begin();
   }

   inline FrameDir::file_iterator
   FrameDir::end(void) const {
      if (mDirty) checkData();
      return mList.end();
   }

   inline int 
   FrameDir::getDebug(void) const {
      return mDebug;
   }

   inline void
   FrameDir::setDebug(int Level) {
      mDebug = Level;
   }

//@}

#endif   // FRAMEDIR_HH
