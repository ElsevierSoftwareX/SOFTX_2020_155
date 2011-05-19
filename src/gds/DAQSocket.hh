/* Version: $Id$ */
#ifndef DAQSOCKET_HH
#define DAQSOCKET_HH
//
//    This is a C++ interface for frame data.
//
#include <sys/time.h>
#include <time.h>
#include <vector>
#include <string>
#include <map>
#include "gmutex.hh"


#define DAQD_PORT 8088


/** @name Network Data Access API (C++)

    This API provides routines to get channel data over the network.

    @memo Access channel data through the network data server
    @author Written by John Zweizig
    @version 0.1
************************************************************************/

/*@{*/

// This is a nice idea that didn't pan out. doc++ lists all classes and 
// structures in a name space with only the namespace name.
//
// namespace DAQD {
//     class  DAQSocket;
//     struct Channel;
//     struct RecHdr;
// };

/**  Channel data-base entry.
  *  The channel database contains a description of all the available 
  *  channels including their names, sampling rates and group numbers.
  */
   struct DAQDChannel {
   ///                  The channel name
      char mName[MAX_CHNNAME_SIZE];
   ///                  The channel group number
      int  mGroup;
   ///                  The channel sample rate
      int  mRate;
   ///                  The channel or testpoint number
      int  mNum;
   ///                  The channel bytes-per-sample value
      int  mBPS;
   ///                  The channel data type
      int  mDatatype;
   ///                  The channel front-end gain
      float mGain;
   ///                  The channel slope
      float mSlope;
   ///                  The channel offset
      float mOffset;
   ///                  The channel unit
      char mUnit[40];
   };

/**  DAQD header record.
  *  The DAQD header record is sent before each block of data.
  */
   struct DAQDRecHdr {
   ///            Data block length (in bytes) excluding this word.
      int Blen;
   ///            Data length in seconds.
      int Secs;
   ///            GPS time (in seconds) of the start of this data.
      int GPS;
   ///            Time offset of the data from the GPS second (in ns).
      int NSec;
   ///            Data block sequence number (first reply to a request is 0).
      int SeqNum;
   };

/**  The DAQD socket class.
  *  DAQSocket provides a client interface to the Network Data Server.
  *  The server provides data in the CDS proprietary format or as standard
  *  frames. The interface may also be used to list channel names, or to
  *  specify the channels to be read in.
  */
   class DAQSocket {
   public:
      /// pair representing data rate (in Hz) & byte per sample
      typedef std::pair<int, int> rate_bps_pair;
      /// list of channels: map between channel name and channel info
      typedef std::map<std::string, DAQDChannel> channellist;
      /// channel list iterator
      typedef channellist::iterator Channel_iter;
   
   /**  List of channel names to be read.
    */
      channellist mChannel;
   
   /**  Construct an unopened socket.
    */
      DAQSocket();
   
   /**  Create a socket and connect it to a server.
    *  The network address argument has the same syntax as that passed to
    *  DAQSocket::open().
    */
      explicit DAQSocket(const char* ipaddr, int ipport = DAQD_PORT,
                        int RcvBufferLen = 16384);
   
   /**  Disconnect and close a socket.
    */
      ~DAQSocket();
   
   /**  Open an existing socket and connect it to a server.
    *  The argument, ipaddr, specifies the IP address of the node on which 
    *  the network data server is running. It may be specified either as 
    *  a symbolic name or as four numeric fields separated by dots. open()
    *  returns zero if successful, a positive non-zero error code if one was 
    *  returned by DAQD or -1.
    */
      int  open (const char* ipaddr, int ipport = DAQD_PORT, 
                int RcvBufferLen = 16384);
   
   /**  Disconnect and close a socket.
    */
      void close();
   
   /**  flushes the input data from the socket.
    */
      void flush();
   
   /**  true if socket is open and connected
    */
      bool isOpen() const {
         return mOpened;
      }
   
   /**  true if request was alreday sent
    */
      bool isOn() const {
         return mWriterType != NoWriter;
      }
   
   /**  Stop a data writer.
    *  This function effectively countermands the RequestXXX() functions.
    *  StopWriter returns either the server response code or -1 if no 
    *  writer has been requested.
    */
      int  StopWriter();
   
   /**  Start reading frame data.
    *  The network data server is requested to start a frame writer task.
    *  Only channels explicitly specified by AddChannel() will be written.
    */
      int  RequestFrames();
   
   /**  Start reading CDS data.
    *  The network data server is requested to start a net-writer task.
    *  Only channels explicitly specified by AddChannel() will be written.
    */
      int  RequestOnlineData (bool fast = false, long timeout = -1);
   
   /**  Start reading CDS data.
    *  The network data server is requested to start a net-writer task
    *  for past data. Start time and duration are given in GPS seconds. 
    *  Only channels explicitly specified by AddChannel() will be written.
    */
      int  RequestData (unsigned long start, unsigned long duration, 
                       long timeout = -1);
   
   /**  Start reading CDS trend data.
    *  The network data server is requested to start a trend net-writer 
    *  task. Start time and duration are given in GPS seconds. 
    *  Only channels explicitly specified by AddChannel() will be written.
    */
      int  RequestTrend (unsigned long start, unsigned long duration,
                        bool mintrend = false, long timeout = -1);
   
   /**  Start reading file names.
    *  The network data server is requested to start a name writer task.
    */
      int  RequestNames(long timeout = -1);
   
   /**  Wait for data to arrive.
    *  Execution is blocked until data are available to the socket. This
    *  can be used to wait for data after a request has been made. The
    *  calling function can then e.g. allocate a buffer before calling 
    *  GetData(), GetName() or GetFrame(). If poll is true the function
    *  returns immediately with 1 if data is ready, or 0 if not.
    */
      int  WaitforData (bool poll = false);
   
   /**  Add a channel to the request list.
    *  All channels may be added by specifying "all" instead of a channel
    *  name.
    */
      int AddChannel (const DAQDChannel& chns);
   
   /**  Add a channel to the request list.
    *  All channels may be added by specifying "all" instead of a channel
    *  name.
    */
      int  AddChannel(const char* chan, 
                     rate_bps_pair rb = rate_bps_pair (0, 0));
   
   /**  Remove a channel from the request list.
    */
      void RmChannel(const char* chan);
   
   /**  Receive block of data in the CDS proprietary format.
    *  A single block of data (including the header) is received and copied
    *  into the specified buffer. The data length (excluding the header) is 
    *  returned. GetData() returns -1 if an error is detected, or 0 if an
    *  end-of file record is found.
    */
      int GetData(char* buf, int len, long timeout = -1);
   
   /**  Receive block of data in the CDS proprietary format.
    *  A single block of data (including the header) is received and copied
    *  into the specified buffer. The data length (excluding the header) is 
    *  returned. GetData() returns -1 if an error is detected, or 0 if an
    *  end-of file record is found. A buffer of the correct length will be 
    *  allocated and returned automatically. The caller is reponsible
    *  for deallocation using delete[].
    */
      int GetData (char** buf, long timeout = -1);
   
   /**  Receive a file name.
    *  The next frame file name written by the NDS is copied to buf and the 
    *  data length is returned. The GetName returns -1 if a name-writer 
    *  hasn't been started, if the data buffer is too short, or if an error
    *  occurs in reading the data. GetData waits for a new message if one is 
    *  not available from the socket.
    */
      int  GetName(char *buf, int len);
   
   /**  Receive a data frame.
    *  A single data frame is received and copied to the specified buffer.
    *  The length of the Frame data is returned. GetFrame() returns -1 
    *  in case of an error or 0 if a trailer (end of file) block is received.
    */
      int  GetFrame(char *buf, int len);
   
   /**  List all known channels.
    *  The names, sample rates, etc. of all channels known by the server are 
    *  copied into the channel list. The list is preallocated by the caller 
    *  with N entries. Available() returns the number of entries found or -1
    *  if an error occurred. If the number of entries is greater than N, 
    *  only the first N are copied to list;
    */
      int  Available (DAQDChannel list[], int N);
      int  Available (std::vector<DAQDChannel>& list);
   
   /**  Known time intervals.
    *  The network data server is requested to return start time and 
       duration of the data stored on disk.
    */
      int  Times (unsigned long& start, unsigned long& duration, 
                 long timeout = -1);
   /**  Known time intervals of trend data.
    *  The network data server is requested to return start time and 
       duration of the trend data stored on disk.
    */
      int  TimesTrend (unsigned long& start, unsigned long& duration, 
                      bool mintrend = false, long timeout = -1);
   
   /**  Set debug mode.
    *  Setting debug mode to true causes the following to be printed to cout: 
    *  all request text, the status code and reply text for each request, 
    *  the header of each data block received and the length of each data/text
    *  block received and its buffer size.
    */
      void setDebug(bool debug=true) {
         mDebug = debug;}
   
   /**  Set abort button.
    *  If the abort "button" is used, recv/send will periodically check if
    *  it has become true and if yes, abort the transaction. Make sure
    *  to set abort to false before calling a method that receives data
    *  or sends a request.
    */
      void setAbort (bool* abort) {
         mAbort = abort;}
   
   /**  Test that connection is open.
    *  This is the only means of testing whether the creator was able to
    *  connect to a socket.
    */
      bool TestOpen(void) {
         return mOpened;}
   
   /**  Get the server version ID.
    *  The version and revision numbers of the server software are returned 
    *  in a single float as (Version + 0.01*Revision).
    */
      float Version(void) {
         return mVersion + 0.01*mRevision;}
   
   private:
   /**  Send a request.
    *  SendRequest sends a request specified by 'text' to the server and
    *  reads the reply status and optionally the reply text. 'reply' is 
    *  a preallocated buffer into which the reply is copied and 'length' 
    *  is the size of the reply buffer. If 'reply' is omitted or coded 
    *  as 0, the status will not be read. The reply status is returned by 
    *  SendRequest and the length of the reply message is returned in 
    *  *Size if Size is non-zero. Only the first message is read into 
    *  reply, so in general *Size != length.
    */
      int SendRequest(const char* text, char *reply=0, int length=0, int *Size=0,
                     timeval* maxwait=0);
   
   /**  Receive data from the socket.
    *  Up to 'len' bytes are read from the socket into '*buf'. If readall
    *  is true, RecvRec will perform as many reads as is necessary to 
    *  fill 'buf'.
    */
      int RecvRec(char *buf, int len, bool readall=false, 
                 timeval* maxwait=0);
   
   /**  Send a record data to the socket.
    *  Up to 'len' bytes are written to the socket from '*buf'.
    */
      int SendRec(const char *buffer, int length, timeval* maxwait = 0);
   
   /**  Receive a data header and data block.
    *  Up to 'len' bytes are read from the socket into '*buf'. If keep
    *  is true, RecvData will put the header record into the buffer, if it 
    *  is false, the header is discarded.
    */
      int RecvData(char *buf, int len, DAQDRecHdr* hdr=0, long timeout = -1);
   
   /**  Receive a data header and data block.
    *  A memory buffer of correct length is allocated automatically. The header 
    *  record will be put at the beginning of the buffer. Returns the number
    *  of data bytes, or <0 on error.
    */
      int RecvData(char **buf, long timeout = -1);
   
   private:
   /**  mutex to protect object.
    */   
      mutable thread::recursivemutex	mux;
   
   /**  Socket is open and connected.
    */
      bool  mOpened;
   
   /**  Debug mode was specified.
    */
      bool  mDebug;
   
   /**  Socket number.
    */
      int   mSocket;
   
   /**  Socket receiving buffer length.
    */
      int   mRcvBuffer;
   
   /**  Set if 'all' channels was specified.
    *  The channel list is ignored if this flag is set.
    */
      bool  mGetAll;
   
   /**  Set if bytes must be reordered.
    */
      bool  mReorder;
   
   /**  ID number of the data writer currently in use.
    */
      char  mWriter[8];
   
   /**  Writer type enumeration.
    *  This enumerator specifies the type of data requested through the 
    *  socket. It is set when a data writer is started and tested by the 
    *  data read methods when they are called.
    */
      enum {NoWriter, NameWriter, DataWriter, FrameWriter, FastWriter} 
      mWriterType;
   
   /**  Offline flag sent when the writer is started.
    */
      int   mOffline;
   
   /**  Server version number.
    */
      int   mVersion;
   
   /**  Server revision number.
    */
      int   mRevision;
   
   /**  Abort "button".
    */
      bool*   mAbort;
   
   };

/*@}*/

#endif  //  DAQSOCKET_HH

