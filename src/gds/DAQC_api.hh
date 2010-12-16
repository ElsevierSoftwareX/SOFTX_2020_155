/* Version: $Id$ */
/* -*- mode: c++; c-basic-offset: 4; -*- */
#ifndef DAQC_API_HH
#define DAQC_API_HH
//
//    This is a C++ interface for frame data.
//
#include <sys/time.h>
#include <time.h>
#include <vector>
#include <string>
#include <map>
#include "gmutex.hh"

/** @page Network Data Access API (C++)
    A typical online NDS client would do the following:
    \verbatim
    DAQC_api* nds(0);

    //---------  Construct a concrete DAQC_api socket with
    nds = new NDS1Socket;

    //---------  or with
    nds = new NDS2Socket;

    //---------  Open a socket to the specified server port.
    const char* servername = "nds-server:port";
    nds.open(servername);
    if (!nds.TestOpen()) fail("Open failed!");

    //---------  Specify the channel to be read.
    const char* chan = "channel name";
    if (!nds.AddChannel(chan)) fail("Add channel failed!");
    if (nds.RequestOnlineData()) fail("Data Request failed");

    //---------  Specify the channel to be read.
    float* Samples = new float[data_len];
    while (1) {
        int nData = nds.GetData((char*) Samples, data_len);
	if (nData <= 0) break;
        ...  Process data ...
    }
    \endverbatim
    @memo Access channel data through the network data server
    @author John Zweizig
    @version 0.1; Last modified March 5, 2008
    @ingroup IO_daqs
************************************************************************/

//--------------------------------------  Default protocol ports
#ifndef DAQD_PORT
#define DAQD_PORT 8088
#endif
#define NDS2_PORT 31200

/*@{*/

/**  Define channel types. The channel types are used to distinguish 
  *  the requested source of the data. 
  *  \remarks
  *  This expands on and replaces the channel group code in the NDS1 API
  *  which seems to have a very few values (0=normal fast channel, 1000=dmt 
  *  trend channel, 1001=obsolete dmt channel).
  *  \brief Channel type code enumerator.
  */
enum chantype {
    /**  Unknown or unspecified default type. */
    cUnknown,

    /**  Online channel */
    cOnline,

    /**  Archived raw data channel */
    cRaw,

    /** Processed/RDS data channel */
    cRDS,

    /**  Second trend data */
    cSTrend,

    /**  Minute trend data */
    cMTrend,

    /**  Minute trend data */
    cTestPoint

};

/**  Data type enumerator.
  *  \remarks numbering must be contiguous 
  */
enum datatype {
    _undefined = 0,
    _16bit_integer = 1,
    _32bit_integer = 2,
    _64bit_integer = 3,
    _32bit_float = 4,
    _64bit_double = 5,
    _32bit_complex = 6
};

/**  The channel database contains a description of all the available 
  *  channels including their names, sampling rates and group numbers.
  *  @brief Channel data-base entry.
  *  @author John Zweizig
  *  @version 1.0; last modified March 5, 2008
  *  @ingroup IO_daqs
  */
struct DAQDChannel {
    ///                  The channel name
    std::string mName;
    ///                  The channel type
    chantype mChanType;
    ///                  The channel sample rate
    double mRate;
    ///                  Data type
    datatype mDatatype;
    ///                  Channel byte offset in record
    int mBOffset;
    ///                  Data length or error code
    int mStatus;
    ///                  Front-end gain
    float mGain;
    ///                  Unit conversion slope
    float mSlope;
    ///                  Unit conversion offset
    float mOffset;
    ///                  Unit name.
    std::string mUnit;

    DAQDChannel(void) 
	: mChanType(cUnknown), mRate(0.0), mDatatype(_undefined), mBOffset(0), 
	  mStatus(0), mGain(1.0), mSlope(1.0), mOffset(0.0)
    {}

    /// Number of data words
    long nwords(double dt) const {
	return long(dt*mRate + 0.5);
    }

    ///  Convert type name string to channel type
    static chantype cvt_str_chantype(const std::string& str);

    ///  Convert channel type to name string
    static const char* cvt_chantype_str(chantype typ);

    ///  Convert type name string to channel type
    static datatype cvt_str_datatype(const std::string& str);

    ///  Convert channel type to name string
    static const char* cvt_datatype_str(datatype typ);

    ///  Convert channel type to name string
    static int datatype_size(datatype typ);
};

/**  The DAQD header record is sent before each block of data.
  *  @brief DAQD header record.
  *  @ingroup IO_daqs
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

/**  DAQC_api provides a client interface to the Network Data Server.
  *  The server provides data in the CDS proprietary format or as standard
  *  frames. The interface may also be used to list channel names, or to
  *  specify the channels to be read in.
  *  @brief The DAQD socket class.
  *  @author John Zweizig and Daniel Sigg
  *  @version 1.2; last modified March 5, 2008
  *  @ingroup IO_daqs
  */
class DAQC_api {
public:
    /// list of channels: map between channel name and channel info
    typedef std::vector<DAQDChannel> chan_list;

    /// channel list iterator
    typedef chan_list::iterator channel_iter;

    /// channel list iterator
    typedef chan_list::const_iterator const_channel_iter;

    typedef double wait_time;

    typedef unsigned long count_type;

public:
    /**  Construct an unopened socket.
      */
    DAQC_api(void);

    /**  Disconnect and close a socket.
     */
    virtual ~DAQC_api(void);

    /**  Open an existing socket and connect it to a server.
      *  The argument, ipaddr, specifies the IP address of the node on which 
      *  the network data server is running. It may be specified either as 
      *  a symbolic name or as four numeric fields separated by dots.
      *  \brief Connect to a server.
      *  \return zero if successful, a positive non-zero error code if one was 
      *  returned by DAQD or -1.
      */
    virtual int open (const std::string& ipaddr, int ipport, 
		      long buflen = 16384) = 0;

    /**  Disconnect and close a socket.
      */
    virtual void close() = 0;

    /**  flushes the input data from the socket.
      */
    virtual void flush() = 0;

    /**  Test whether the socket is open.
      *  \brief Test open.
      *  \return true if socket is open and connected
      */
    virtual bool isOpen(void) const;

    /**  Test whether the server is processing a request from this client.
      *  \brief Test if transaction in progress.
      *  \return true if request was sent
      */
    virtual bool isOn(void) const;

    /**  Add a channel to the request list.
      *  All channels may be added by specifying "all" instead of a channel
      *  name.
      */
    virtual int AddChannel(const DAQDChannel& chns);

    /**  Add a channel to the request list.
      *  All channels may be added by specifying "all" instead of a channel
      *  name.
      */
    virtual int AddChannel(const std::string& chan, chantype ty, double rate);

    /**  The names, sample rates, etc. of all channels known by the server are 
      *  copied into the channel vector. Available() returns the number of 
      *  entries found or -1 if an error occurred.
      *  @brief List all known channels.
      *  @return Number of channels or -1 if an error occurred.
      */
    virtual int Available(chantype typ, long gps, chan_list& list, 
			  wait_time timeout=-1) = 0;

    /**  Request list iterator start.
      */
    const_channel_iter chan_begin(void) const;

    /**  Request list iterator end.
      */
    const_channel_iter chan_end(void) const;

    /**  Find a channel in the request list.
      */
    const_channel_iter FindChannel(const std::string& chan) const;

    /**  Find a channel in the request list.
      */
    channel_iter FindChannel(const std::string& chan);

    /**  Copy data for a specified channel to a user buffer.
      *  @brief Get channel data
      *  @param chan Channel name
      *  @param data User provided data buffer
      *  @param maxlen Length of user buffer in bytes.
      *  @return Number of bytes or -1 on error.
      */
    virtual int GetChannelData(const std::string& chan, float* data,
			       long maxlen) const;
   
    /**  Receive block of data in the CDS proprietary format.
      *  A single block of data (including the header) is received and stored
      *  in an internal buffer. 
      *  @brief Get a data block from the server.
      *  @param
      *  @return data length, or failure.
      */
     virtual int GetData(wait_time timeout = -1);

    /**  Receive a block of data. Transfer the header and data into an 
      *  Allocated buffer.
      */
    virtual int GetData(char** buf, wait_time timeout = -1);

    /**  Get the number of requested channels.
      *  \brief Number of requested channels.
      *  \return Number of requested channels.
      */
    virtual unsigned long nRequest(void) const;

    /**  The network data server is requested to start a net-writer task.
      *  Only channels explicitly specified by AddChannel() will be written.
      *  \brief Start reading CDS data.
      *  \param fast Reat 1/16th sec at a time.
      *  \param timeout Maximum time to wait for a ewsponse (-1 = infinite)
      *  \return Zero if successful, NDS response or -1.
      */
    virtual int RequestOnlineData (bool fast, wait_time timeout) = 0;
   
    /**  The network data server is requested to start a net-writer task
      *  for past data. Start time and duration are given in GPS seconds. 
      *  Only channels explicitly specified by AddChannel() will be written.
      *  \brief Start reading archived data.
      *  \param start GPS time of start of data to be read.
      *  \param duration Number of seconds of data to read.
      *  \param timeout Maximum time to wait for a response (-1 = infinite)
      *  \return Zero if successful, NDS response or -1.
      */
    virtual int RequestData (unsigned long start, unsigned long duration, 
			     wait_time timeout = -1) = 0;

    /**  Remove the specified channel from the request list.
      *  \brief Remove channel from the request list
      *  \param chan Channel name.
      */
    virtual void RmChannel(const std::string& chan);
   
    /**  Set the abort flag. If the abort "button" is used, recv/send will 
      *  periodically check its state and if it has become true, will abort 
      *  the transaction. Make sure to set abort to false before calling a 
      *  method that receives data or sends a request.
      *  \brief Set abort button.
      */
    virtual void setAbort (bool* abort);

    /**  Set debug mode.
      *  Setting debug mode to true causes the following to be printed to cout: 
      *  all request text, the status code and reply text for each request, 
      *  the header of each data block received and the length of each data/text
      *  block received and its buffer size.
      */
    virtual void setDebug(int debug=1);
    virtual void setDebug(bool debug=true);
   
    /**  StopWriter effectively countermands the RequestXXX() functions.
      *  \brief Stop a data writer.
      *  \return the server response code or -1 if no writer is active.
      */
    virtual int StopWriter() = 0;

    /**  Known time intervals.
      *  The network data server is requested to return start time and 
      *  duration of the data stored on disk.
      */
    virtual int Times(chantype type, unsigned long& start, 
		      unsigned long& duration, wait_time timeout = -1);

    /**  Get the server version ID.
      *  The version and revision numbers of the server software are returned 
      *  in a single float as (Version + 0.01*Revision).
      */
     virtual float Version(void) const;

    /**  Wait for data to arrive.
      *  Execution is blocked until data are available to the socket. This
      *  can be used to wait for data after a request has been made. The
      *  calling function can then e.g. allocate a buffer before calling 
      *  GetData(), GetName() or GetFrame(). If poll is true the function
      *  returns immediately with the return value indicating whether data 
      *  are present.
      *  \brief Wait for data.
      *  \param poll If true WaitforData returns immediately. The return 
      *              codes indicates whether data are available.
      *  \return 0: no data available, &gt;0: data are available, &lt; error.
      */
    virtual int  WaitforData (bool poll = false) = 0;

protected:
    /**  Swap all the data.
     */
    void SwapHeader(void);

    /**  Swap all the data.
     */
    int SwapData(void);

    int CVHex(const char* text, int N);
   
    /**  Receive a data record consisting of a header record followed by 
      *  a data block. The header is checked for an out-of-band message 
      *  type (e.g. reconfigure block) and the return code is set accordingly.
      *  A memory buffer of correct length is allocated automatically.
      *  \brief Receive a data header and data block.
      *  \param timeou Maximum wait time
      *  \return Data length, -1 on error, -2 on reconfigure block.
      */
    virtual int RecvData(wait_time timeout=-1) = 0;
   
    /**  Receive a float data word, swap the byte ordering if necessary
      *  and store thedata in the argument location.
      *  \brief Receive a float data word.
      */
    virtual int RecvFloat(float& data, wait_time timeout=-1);

    /**  Receive an integer data word, swap the byte ordering if necessary
      *  and store thedata in the argument location.
      *  \brief Receive an integer data word.
      */
    virtual int RecvInt(int& data, wait_time timeout=-1);

    /**  Receive data from the socket.
     *  Up to 'len' bytes are read from the socket into '*buf'. If readall
     *  is true, RecvRec will perform as many reads as is necessary to 
     *  fill 'buf'.
     */
    virtual int RecvRec(char *buf, long len, bool readall=false, 
			wait_time maxwait=-1) = 0;
    
    /**  Receive a reconfigure block.
      *  A reconfigure structure of channel metadata.
      */
    virtual int RecvReconfig(count_type len, wait_time maxwait=-1) = 0;
   
    
    /**  Receive a string consisting of a 4-byte length followed by a
      *   text field.
      *  \brief Receive a string.
      *  \param str     String into which input data will be stored.
      *  \param maxwait Maximum wait time.
      *  \return length of string or -1.
      */
    virtual int RecvStr(std::string& str, wait_time maxwait=-1);

public:
    class recvBuf {
    public:
	typedef unsigned long size_type;
    public:
	recvBuf(size_type length=0);
	~recvBuf(void);
	size_type capacity(void) const;
	void clear(void);
	char* ref_data(void);
	const char* ref_data() const;
	DAQDRecHdr& ref_header();
	const DAQDRecHdr& ref_header() const;
	void reserve(size_type length);
    private:
	size_type  mLength;
	DAQDRecHdr mHeader;
	char*      mData;
    };

    /**  Receive buffer
     */
    recvBuf mRecvBuf;

protected:
    /**  mutex to protect object.
      */   
    mutable thread::recursivemutex	mux;
   
    /**  Socket is open and connected.
      */
    bool  mOpened;

    /**  Debug mode was specified.
      */
    int mDebug;

    /**  Writer type enumeration.
      *  This enumerator specifies the type of data requested through the 
      *  socket. It is set when a data writer is started and tested by the 
      *  data read methods when they are called.
      */
    enum writer_type {
	NoWriter, 
	NameWriter, 
	DataWriter, 
	FrameWriter, 
	FastWriter,
	NDS2Writer
    } mWriterType;
   
    /**  Offline flag sent when the writer is started.
      */
    int   mOffline;

    /**  Server version number.
      */
    int   mVersion;

    /**  Server revision number.
      */
    int   mRevision;

    /**  Abort flag
     */
    bool* mAbort;

    /**  Channel request list.
      */
    chan_list mRequest_List;

};

//======================================  Inline methods
inline DAQC_api::const_channel_iter 
DAQC_api::chan_begin(void) const {
    return mRequest_List.begin();
}

inline DAQC_api::const_channel_iter 
DAQC_api::chan_end(void) const {
    return mRequest_List.end();
}

inline bool 
DAQC_api::isOpen(void) const {
    return mOpened;
}

inline float 
DAQC_api::Version(void) const {
    return mVersion + 0.01*mRevision;
}

inline bool 
DAQC_api::isOn() const {
    return mWriterType != NoWriter;
}

inline unsigned long 
DAQC_api::nRequest(void) const {
    return mRequest_List.size();
}

inline DAQC_api::recvBuf::size_type 
DAQC_api::recvBuf::capacity(void) const {
    return mLength;
}

inline char* 
DAQC_api::recvBuf::ref_data(void) {
    return mData;
}

inline const char* 
DAQC_api::recvBuf::ref_data() const {
    return mData;
}

inline DAQDRecHdr& 
DAQC_api::recvBuf::ref_header() {
    return mHeader;
}

inline const DAQDRecHdr&
DAQC_api::recvBuf:: ref_header() const {
    return mHeader;
}

/*@}*/

#endif  //  DAQSOCKET_HH

