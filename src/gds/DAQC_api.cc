static char *versionId = "Version $Id$" ;
/* -*- mode: c++; c-basic-offset: 4; -*- */
//
//    Implementation of the C++ DAQ client interface (DAQSocket) class
//
//    Revision History
//
//    23-Feb-1999  J.Zweizig
//        First really working version.
//
//    26-Feb-1999  J.Zweizig
//        Better comments, lots of little fixes, byte-swapping support.
//
//    Notes:
//      1) The channel list data format is incorrectly documented. There 
//         is an extra 4-byte field at the start of the reply string, just 
//         after the channel count field. Also, starting with version 9,
//         there are two additional (undocumentd) 4-byte hex fields. For
//         now these are copied to fields more1 and more2 in the channel 
//         structure.
//      2) I have introduced code to swap bytes on little-endian machines.
//         This was tested on sadan (Linux alpha) on 26/02/1999.
//
#ifndef __EXTENSIONS__
#define __EXTENSIONS__
#endif
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
#include "PConfig.h"
#include "DAQC_api.hh"
#include "sockutil.h"
#include "Time.hh"
#include "Interval.hh"
//#include "test_endian.hh"
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/file.h>
#include <iostream>

#define _TIMEOUT 10000000	/* 10 sec */
#ifdef P__WIN32
#define MSG_DONTWAIT 0
#endif

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Forwards							        //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

using namespace std;
using namespace thread;

// TODO: ?

//static test_endian daqs_tend;

template<class T> 
void 
SwapN(T* d, long N) {
	return;
#if 0
    if (daqs_tend.big_end() || sizeof(T) < 2) return;
    for (long i=0;  i<N; ++i) {
	char* pf = reinterpret_cast<char*>(d + i);
	char* pe = pf + sizeof(T);
	while (pf < pe) {
	    char t = *pf;
	    *pf++ = *--pe;
	    *pe   = t;
	}
    }
#endif
}

const char*
DAQDChannel::cvt_chantype_str(enum chantype t) {
    switch (t) {
    case cOnline:
	return "online";
    case cRaw:
	return "raw";
    case cRDS:
	return "reduced";
    case cSTrend:
	return "s-trend";
    case cMTrend:
	return "m-trend";
    case cTestPoint:
	return "test-pt";
    default:
	return "unknown";
    }
    return "unknown";
}

chantype 
DAQDChannel::cvt_str_chantype(const string& name) {
#define TEST_NAME(x) if (name == cvt_chantype_str(x)) return x
    TEST_NAME(cOnline);
    TEST_NAME(cRaw);
    TEST_NAME(cRDS);
    TEST_NAME(cSTrend);
    TEST_NAME(cMTrend);
    TEST_NAME(cTestPoint);
#undef TEST_NAME
    return cUnknown;
}

const char*
DAQDChannel::cvt_datatype_str(enum datatype t) {
    switch (t) {
    case _undefined:
	return "byte_1";
    case _16bit_integer:
	return "int_2";
    case _32bit_integer:
	return "int_4";
    case _64bit_integer:
	return "int_8";
    case _32bit_float:
	return "real_4";
    case _64bit_double:
	return "real_8";
    case _32bit_complex:
	return "complex_8";
	//case _64bit_complex:
	//return "complex_16";
    default:
	return "byte_1";
    }
    return "byte_1";
}

datatype 
DAQDChannel::cvt_str_datatype(const string& name) {
#define TEST_NAME(x) if (name == cvt_datatype_str(x)) return x
    TEST_NAME(_16bit_integer);
    TEST_NAME(_32bit_integer);
    TEST_NAME(_64bit_integer);
    TEST_NAME(_32bit_float);
    TEST_NAME(_64bit_double);
    TEST_NAME(_32bit_complex);
    //TEST_NAME(_64bit_complex);
#undef TEST_NAME
    return _undefined;
}

int
DAQDChannel::datatype_size(datatype dtype) {
    int nwd = 0;
    switch (dtype) {
    case _16bit_integer:
	nwd = sizeof(short);
	break;
    case _32bit_integer:
	nwd = sizeof(int);
	break;
    case _32bit_float:
    case _32bit_complex:
	nwd = sizeof(float);
	break;
    case _64bit_double:
	nwd = sizeof(double);
	break;
    default:
	nwd = 0;
    }
    return nwd;
}

//======================================  Hex conversion
int 
DAQC_api::CVHex(const char* text, int N) {
    int v = 0;
    for (int i=0 ; i<N ; i++) {
	v <<= 4;
	if      ((text[i] >= '0') && (text[i] <= '9')) v += text[i] - '0';
	else if ((text[i] >= 'a') && (text[i] <= 'f')) v += text[i] - 'a' + 10;
	else if ((text[i] >= 'A') && (text[i] <= 'F')) v += text[i] - 'A' + 10;
	else                                           return -1;
    }
    return v;
}

//======================================  Constructors
DAQC_api::DAQC_api(void)
  : mOpened(false), mDebug(0), mWriterType(NoWriter),
    mVersion (0), mRevision (0), mAbort (0)
{
}

//======================================  Destructor
DAQC_api::~DAQC_api(void) {
}

//======================================  Add a channel to the channel list
int 
DAQC_api::AddChannel(const DAQDChannel& chn) {
    semlock lockit(mux);

    //----------------------------------  Make sure there's enough room
    if (mRequest_List.capacity() < 4096) {
	mRequest_List.reserve(4096);
    }

    //----------------------------------  Add the channel.
    mRequest_List.push_back(chn);
    return 1;
}

//======================================  Add a channel to the channel list
int 
DAQC_api::AddChannel(const std::string& chan, chantype ty, double rate) {
    semlock lockit (mux);
    DAQDChannel chn;
    chn.mName     = chan;
    chn.mRate     = rate;
    chn.mChanType = ty;

    //----------------------------------  Add the channel.
    return AddChannel (chn);
}

//======================================  Remove a channel from the list
void 
DAQC_api::RmChannel(const std::string& chan) 
{
    semlock lockit (mux);
    if (chan == "all") {
	mRequest_List.clear();
    } else {
	channel_iter iter = FindChannel(chan);
	if (iter != mRequest_List.end()) mRequest_List.erase (iter);
    }
}

//======================================  Remove a channel from the list
DAQC_api::channel_iter
DAQC_api::FindChannel(const string& chan) 
{
    semlock lockit (mux);
    for (channel_iter iter=mRequest_List.begin(); 
	 iter != mRequest_List.end(); ++iter) {
	if (iter->mName == chan) return iter;
    }
    return mRequest_List.end();
}

DAQC_api::const_channel_iter
DAQC_api::FindChannel(const string& chan) const {
    semlock lockit(mux);
    for (const_channel_iter iter=chan_begin(); iter!=chan_end(); ++iter) {
	if (iter->mName == chan) return iter;
    }
    return mRequest_List.end();
}

//======================================  copy data to float template
template<class T>
int
copyData(const DAQC_api::recvBuf& b, const DAQC_api::const_channel_iter& i, 
	 float* data, long maxlen) 
{
    const T* p = reinterpret_cast<const T*>(b.ref_data() + i->mBOffset);
    long nwd = i->mStatus;
    if (nwd > maxlen) nwd = maxlen;
    nwd /= sizeof(T);
    for (int j=0; j<nwd; ++j) {
	data[j] = *p++;
    }
    return nwd;
}

//======================================  Copy data for one channel
int
DAQC_api::GetChannelData(const string& chan, float* data, long maxlen) const {
    semlock lockit(mux);
    const_channel_iter i = FindChannel(chan);
    if (i == chan_end()) return -1;
    int nwd = i->mStatus;
    if (nwd <= 0) return nwd;
    switch (i->mDatatype) {
    case _16bit_integer:
	nwd = copyData<short>(mRecvBuf, i, data, maxlen);
	break;
    case _32bit_integer:
	nwd = copyData<int>(mRecvBuf, i, data, maxlen);
	break;
    case _32bit_float:
    case _32bit_complex:
	nwd = copyData<float>(mRecvBuf, i, data, maxlen);
	break;
    case _64bit_double:
	nwd = copyData<double>(mRecvBuf, i, data, maxlen);
	break;
    default:
	nwd = 0;
    }
    return nwd;
}

//======================================  Get a channel data
int
DAQC_api::GetData(wait_time timeout) {
    semlock lockit(mux);
    int rc = -2;
    while (rc == -2) {
	rc = RecvData(timeout);
	if (rc == -2) {
	    long block_len = mRecvBuf.ref_header().Blen 
		           - (sizeof(DAQDRecHdr) -sizeof(int));
	    rc = RecvReconfig(block_len, timeout);
	}
    }
    return rc;
}

//======================================  Get a channel data
int 
DAQC_api::GetData(char** buf, wait_time timeout) {
    semlock lockit(mux);
    int len = GetData(timeout);
    if (len > 0) {
	long lh = sizeof(DAQDRecHdr); 
	*buf = new char[lh + len];
	if (!*buf) {
	    len = -1;
	} else {
	    memcpy(*buf, reinterpret_cast<char*>(&mRecvBuf.ref_header()), lh);
	    memcpy(*buf+lh, mRecvBuf.ref_data(), len);
	    len += lh;
	}
    }
    return len;
}

//======================================  Get an integer
int 
DAQC_api::RecvInt(int& data, wait_time timeout) {
    semlock lockit(mux);
    int rc = RecvRec((char*)&data, sizeof(int), true, timeout);
    if (rc != sizeof(int)) return -1;
    SwapN(&data, 1);
    return rc;
}

//======================================  Get an float
int 
DAQC_api::RecvFloat(float& data, wait_time timeout) {
    semlock lockit(mux);
    int rc = RecvRec((char*)&data, sizeof(int), true, timeout);
    if (rc != sizeof(int)) return -1;
    SwapN(&data, 1);
    return rc;
}

//======================================  Get a string
int 
DAQC_api::RecvStr(string& str, wait_time timeout) {
    semlock lockit(mux);
    int len;
    int rc = RecvInt(len, timeout);
    if (rc != sizeof(int)) return -1;
    str.resize(len);
    rc = RecvRec(&str[0], len, true, timeout);
    if (rc < len) rc = -1;
    return rc;
}


//======================================  Swap the header in the receive buffer
void
DAQC_api::SwapHeader(void) {
    SwapN(reinterpret_cast<int*>(&mRecvBuf.ref_header()), 5);
}

//======================================  Swap all the data bytes.
int
DAQC_api::SwapData(void) {
    int rc = 0;
    int N  = mRequest_List.size();
    for (int i=0; i<N; ++i) {
	datatype datyp = mRequest_List[i].mDatatype;
	long nWord  = mRequest_List[i].mStatus;
	char* p = mRecvBuf.ref_data() + mRequest_List[i].mBOffset;

	switch (datyp) {
	case _16bit_integer:
	    nWord /= sizeof(short);
	    SwapN(reinterpret_cast<short*>(p), nWord);
	    break;
	case _32bit_integer:
	    nWord /= sizeof(int);
	    SwapN(reinterpret_cast<int*>(p), nWord);
	    break;
	case _32bit_float:
	case _32bit_complex:
	    nWord /= sizeof(float);
	    SwapN(reinterpret_cast<float*>(p), nWord);
	    break;
	case _64bit_integer:
	case _64bit_double:
	    nWord /= sizeof(double);
	    SwapN(reinterpret_cast<double*>(p), nWord);
	    break;
	default:
	    cerr << "DAQC_api: Unidentified data type code: " << datyp << endl;
	    rc = 1;
	}
    }
    return rc;
}


//======================================  Set the abort button
void 
DAQC_api::setAbort (bool* abort) {
    mAbort = abort;
}

//======================================  Set the debug print level
void 
DAQC_api::setDebug(int debug) {
    mDebug = debug;
}

void
DAQC_api::setDebug(bool debug) {
    setDebug(debug?1:0);
}

//======================================  Provide data times - bogus by default
int 
DAQC_api::Times(chantype type, unsigned long& start, unsigned long& duration, 
	       wait_time timeout) {
    start    = 815000000;
    duration = 200000000;
    return 0;
}

//======================================  
DAQC_api::recvBuf::recvBuf(size_type length) 
    : mLength(0), mData(0)
{
    reserve(length);
}

//======================================  
DAQC_api::recvBuf::~recvBuf(void) {
    clear();
}

//======================================  Clear the receive buffer
void 
DAQC_api::recvBuf::clear(void) {
    if (mData) delete[] mData;
    mData   = 0;
    mLength = 0;
}

//======================================  Set the receive buffer size
void 
DAQC_api::recvBuf::reserve(size_type length) {
    if (mData && mLength < length) clear();
    if (!mData & length > 0) {
	mLength = length;
	mData = new char[length];
    }
}
