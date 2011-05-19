static const char *versionId = "Version $Id$" ;
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
#include "DAQSocket.hh"
#include "sockutil.h"
#include "Time.hh"
#include "Interval.hh"
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/file.h>
#include <iostream>
#ifndef __GNU_STDC_OLD
#include <sstream>
#else
#include "gnusstream.h"
#endif

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

//--------------------------------------  Hex conversion
   static int 
   CVHex(const char *text, int N) {
      int v = 0;
      for (int i=0 ; i<N ; i++) {
         v<<=4;
         if      ((text[i] >= '0') && (text[i] <= '9')) v += text[i] - '0';
         else if ((text[i] >= 'a') && (text[i] <= 'f')) v += text[i] - 'a' + 10;
         else if ((text[i] >= 'A') && (text[i] <= 'F')) v += text[i] - 'A' + 10;
         else 
            return -1;
      }
      return v;
   }

//--------------------------------------  Swap ints and shorts in place
   static void
   SwapI(int *array, int N) {
      char temp;
      for (int i=0 ; i<N ; i++) {
         char* ptr = (char*) (array + i);
         temp   = ptr[0];
         ptr[0] = ptr[3];
         ptr[3] = temp;
         temp   = ptr[1];
         ptr[1] = ptr[2];
         ptr[2] = temp;
      }
      return;
   }
/*
   static void
   SwapS(short *array, int N) {
      char temp;
      for (int i=0 ; i<N ; i++) {
         char* ptr = (char*) (array + i);
         temp   = ptr[0];
         ptr[0] = ptr[1];
         ptr[1] = temp;
      }
      return;
   }
*/
//--------------------------------------  Constructors
   DAQSocket::DAQSocket() 
   : mOpened(false), mDebug(false), mRcvBuffer (16384), mGetAll(false), 
   mWriterType(NoWriter), mVersion (0), mRevision (0), mAbort (0)
   {
      int test = 0;
      *(char*) &test = 1;
      mReorder = (test == 1);
   }

   DAQSocket::DAQSocket(const char* ipaddr, int ipport, int RcvBufferLen) 
   : mOpened(false), mDebug(false), mRcvBuffer (RcvBufferLen), mGetAll(false), 
   mWriterType(NoWriter), mAbort (0)
   {
      int   test=0;
      *(char*) &test = 1;
      mReorder = (test == 1);
   
    //----------------------------------  Open the socket
      open(ipaddr, ipport, mRcvBuffer);
   }

//--------------------------------------  Destructor
   DAQSocket::~DAQSocket() {
      if (mOpened) close();
   }

//--------------------------------------  Open a socket
   int 
   DAQSocket::open(const char* ipaddr, int ipport, int RcvBufferLen) 
   {
   
      semlock lockit (mux);
      struct sockaddr_in socknam;
      char version[4];
      int size;
      mRcvBuffer = RcvBufferLen;
   
    //----------------------------------  Make sure the socket isn't open
      if (mOpened) 
         return -1;
   
    //----------------------------------  Get a socket
      mSocket = socket(PF_INET,SOCK_STREAM,0);
      if (mSocket < 0) 
         return -1;
   
    //----------------------------------- Set the buffer size
      if (setsockopt (mSocket, SOL_SOCKET, SO_RCVBUF, 
                     (char*) &mRcvBuffer, sizeof (int)) != 0) {
         if (mDebug) {
            cerr << "set socket buffer failed for length " << mRcvBuffer << endl;
         }
      }
   
    //----------------------------------  Bind the socket to a port.
      socknam.sin_family      = AF_INET;
      socknam.sin_port        = 0;
      socknam.sin_addr.s_addr = 0;
      int len                 = sizeof(socknam);
      if (bind(mSocket, (struct sockaddr *)&socknam, len) < 0) 
         return -1;
   
    //----------------------------------  Connect to the server.
      socknam.sin_family      = AF_INET;
      socknam.sin_port        = htons (ipport);
      if (nslookup (ipaddr, &socknam.sin_addr) < 0) {
         return -1;
      }
      struct timeval	timeout = 	/* timeout */
         {_TIMEOUT / 1000000, _TIMEOUT % 1000000};
      if (connectWithTimeout (mSocket, (struct sockaddr *)&socknam, 
                           sizeof (socknam), &timeout) < 0)
         return -1;
      mOpened = true;
   
    //----------------------------------  Get the server version number
      mVersion = 0;
      mRevision = 0;
      int rc = SendRequest("version;", version, 4, &size, &timeout);
      if (rc || (size != 4)) {
         ::close(mSocket);
         mOpened = false;
         return rc ? rc : -1;
      }
      mVersion = CVHex(version, 4);
      rc = SendRequest("revision;", version, 4, &size, &timeout);
      if (rc || (size != 4)) {
         ::close(mSocket); 
         mOpened = false;
         return rc ? rc : -1;
      }
      mRevision = CVHex(version, 4);
      if (mDebug) cerr << "Connected to server version " << Version() << endl;
   
    //----------------------------------  Done, Return.
      return rc;
   }

//--------------------------------------  Close a socket
   void DAQSocket::close() 
   {
      semlock lockit (mux);
      if (mOpened) {
         StopWriter();
         SendRequest("quit;");
         ::close(mSocket);
         mOpened = false;
      }
      mChannel.clear();
      mWriterType = NoWriter;
   }

//--------------------------------------  flush data from socket
   void DAQSocket::flush () {
      semlock lockit (mux);
      const int buflen = 16 * 1024;
      char text[buflen];
   
      int i = 0;
      int rc;
      do {
         rc = recv (mSocket, text, buflen, MSG_DONTWAIT);
      } while ((rc >= buflen) && (++i < 100));
   }

//--------------------------------------  Stop data transfer
   int DAQSocket::StopWriter() {
      semlock lockit (mux);
   
    //----------------------------------  Make sure a writer is running.
      if (mWriterType == NoWriter) 
         return -1;
   
    //----------------------------------  Build the request
      ostringstream request;
      request << "kill net-writer " << CVHex (mWriter, 8) << ";";
      request.put(0);
   
    //----------------------------------  Send the request
      int rc = SendRequest(request.str().c_str(), mWriter, 0);
      mWriterType = NoWriter;
   
      return rc;
   }

//--------------------------------------  Request a stream of frame data
   int DAQSocket::RequestFrames() {
      semlock lockit (mux);
   
    //----------------------------------  Build the request
      ostringstream request;
      request << "start frame-writer ";
      if (mGetAll) {
         request << "all;";
      } 
      else {
         request << "{";
         for (Channel_iter i = mChannel.begin() ; i != mChannel.end() ; i++) {
            request << "\"" << i->first << "\"";
         }
         request << "};";
      }
      request.put(0);
   
    //----------------------------------  Send the request
      int rc = SendRequest(request.str().c_str(), mWriter, sizeof(mWriter));
      if (rc) 
         return rc;
      mWriterType = FrameWriter;
   
    //----------------------------------  Read in the offline flag
      int ldata = RecvRec((char*) &mOffline, sizeof(mOffline), false);
      if (ldata != sizeof(mOffline)) 
         return ldata;
   
    //----------------------------------  Done, return
      return rc;
   }

//--------------------------------------  Request file names
   int DAQSocket::RequestNames (long timeout) {
      semlock lockit (mux);
      timeval	maxwait;
      maxwait.tv_sec = timeout < 0 ? -1 : timeout;
      maxwait.tv_usec = 0;
      int rc = SendRequest("start name-writer all;", mWriter, sizeof(mWriter),
                          0, &maxwait);
      if (rc) 
         return rc;
      mWriterType = NameWriter;
   
    //----------------------------------  Read in the offlne flag
      rc = RecvRec((char*) &mOffline, sizeof(mOffline), false, &maxwait);
      if (rc != sizeof(mOffline)) 
         return -1;
      return 0;
   }

//--------------------------------------  Request a stream of channel data
   int DAQSocket::RequestOnlineData (bool fast, long timeout) 
   {
      semlock lockit (mux);
      timeval	maxwait;
      maxwait.tv_sec = timeout < 0 ? -1 : timeout;
      maxwait.tv_usec = 0;
    //----------------------------------  Build the request
      ostringstream request;
      if (fast) {
         request << "start fast-writer ";
      }
      else {
         request << "start net-writer ";
      }
      if (mGetAll) {
         request << "all;";
      } 
      else {
         request << "{";
         for (Channel_iter i = mChannel.begin() ; i != mChannel.end() ; i++) {
            request << "\"" << i->first << "\"";
         }
         request << "};";
      }
      request.put(0);
      if (mDebug)
         cerr << "NDS request = " << request.str() << endl;
   
    //----------------------------------  Send the request
      int rc = SendRequest(request.str().c_str(), mWriter, sizeof(mWriter),
                          0, &maxwait);
      if (mDebug) cerr << mWriter << " = " << CVHex (mWriter, 8) << endl;
      if (rc) {
         return rc;
      }
      mWriterType = DataWriter;
   
    //----------------------------------  Read in the header
      int ldata = RecvRec((char*) &mOffline, sizeof(mOffline), false, &maxwait);
      if (ldata != sizeof(mOffline)) 
         return ldata;
   
    //----------------------------------  Done, return
      return rc;
   }

//--------------------------------------  Request a stream of channel data
   int DAQSocket::RequestData (unsigned long start, unsigned long duration,
                     long timeout)
   {
      semlock lockit (mux);
      timeval	maxwait;
      maxwait.tv_sec = timeout < 0 ? -1 : timeout;
      maxwait.tv_usec = 0;
    //----------------------------------  Build the request
      ostringstream request;
      request << "start net-writer " << start << " " << duration << " ";
      if (mGetAll) {
         request << "all;";
      } 
      else {
         request << "{";
         for (Channel_iter i = mChannel.begin() ; i != mChannel.end() ; i++) {
            request << "\"" << i->first << "\"";
         }
         request << "};";
      }
      request.put(0);
      if (mDebug)
         cerr << "NDS past data request = " << request.str() << endl;
   
    //----------------------------------  Send the request
      int rc = SendRequest(request.str().c_str(), mWriter, sizeof(mWriter),
                          0, &maxwait);
      if (mDebug) cerr << mWriter << " = " << CVHex (mWriter, 8) << endl;
      if (rc) {
         return rc;
      }
      mWriterType = DataWriter;
   
    //----------------------------------  Read in the header
      int ldata = RecvRec((char*) &mOffline, sizeof(mOffline), false, &maxwait);
      if (mDebug) cerr << mOffline << endl;
      if (ldata != sizeof(mOffline)) 
         return ldata;
   
    //----------------------------------  Done, return
      return rc;
   }

//--------------------------------------  Request trend data
   int DAQSocket::RequestTrend (unsigned long start, unsigned long duration,
                     bool mintrend, long timeout)
   {
      semlock lockit (mux);
      timeval	maxwait;
      maxwait.tv_sec = timeout < 0 ? -1 : timeout;
      maxwait.tv_usec = 0;
    //----------------------------------  Build the request
      ostringstream request;
      request << "start trend " << (mintrend ? "60 " : "") << 
         "net-writer " << start << " " << duration << " ";
      if (mGetAll) {
         request << "all;";
      } 
      else {
         request << "{";
         for (Channel_iter i = mChannel.begin() ; i != mChannel.end() ; i++) {
            request << "\"" << i->first << "\"";
         }
         request << "};";
      }
      request.put(0);
      if (mDebug) 
         cerr << "NDS trend data request = " << request.str() << endl;
   
    //----------------------------------  Send the request
      int rc = SendRequest(request.str().c_str(), mWriter, sizeof(mWriter),
                          0, &maxwait);
      if (mDebug) cerr << mWriter << " = " << CVHex (mWriter, 8) << endl;
      if (rc) {
         return rc;
      }
      mWriterType = DataWriter;
   
    //----------------------------------  Read in the header
      int ldata = RecvRec((char*) &mOffline, sizeof(mOffline), false, &maxwait);
      if (ldata != sizeof(mOffline)) 
         return ldata;
   
    //----------------------------------  Done, return
      return rc;
   }

//--------------------------------------  Wait for a message to arrive
//                                        WaitforData() <= 0 means no data.
   int DAQSocket::WaitforData (bool poll) 
   {
      int nset;
      fd_set readfds;
      FD_ZERO(&readfds);
      FD_SET(mSocket, &readfds); 
      if (poll) {
         struct timeval timeout = {0, 0};
         nset = select(FD_SETSIZE, &readfds, 0, 0, &timeout);
      }
      else {
         nset = select(FD_SETSIZE, &readfds, 0, 0, 0);
      }
      if (nset < 0) perror("DAQSocket: Error in select()");
      return nset;
   }

//--------------------------------------  Add a channel to the channel list
   int 
   DAQSocket::AddChannel(const DAQDChannel& chn) {
      semlock lockit (mux);
      mGetAll = false;
   
    //----------------------------------  Add the channel.
      mChannel.insert (channellist::value_type (chn.mName, chn));
      return 1;
   }

//--------------------------------------  Add a channel to the channel list
   int 
   DAQSocket::AddChannel(const char* chan, rate_bps_pair rb) {
      semlock lockit (mux);
    //----------------------------------  Set the all flag if all specified.
      if (string(chan) == "all") {
         mGetAll = true;
         mChannel.clear();
      }
      if (mGetAll) 
         return 1;
      DAQDChannel chn;
      strncpy (chn.mName, chan, sizeof (chn.mName));
      chn.mName[sizeof (chn.mName)-1] = 0;
      chn.mRate = rb.first;
      chn.mBPS = rb.second;
    //----------------------------------  Add the channel.
      return AddChannel (chn);
   }

//--------------------------------------  Remove a channel from the list
   void 
   DAQSocket::RmChannel(const char* chan) 
   {
      semlock lockit (mux);
      string Channel(chan);
      if (Channel == "all") {
         mGetAll = false;
         mChannel.clear();
      }
      else {
         Channel_iter iter = mChannel.find (Channel);
         if (iter != mChannel.end()) {
            mChannel.erase (iter);
         }
      }
   }

//--------------------------------------  Receive frame data into a buffer
   int 
   DAQSocket::GetFrame(char *buffer, int length) {
      semlock lockit (mux);
      if (mWriterType != FrameWriter) 
         return -1;
      return RecvData(buffer, length);
   }

//--------------------------------------  Receive frame data into a buffer
   int 
   DAQSocket::GetName(char *buffer, int length) {
      semlock lockit (mux);
      if (mWriterType != NameWriter) 
         return -1;
      return RecvData(buffer, length);
   }

//--------------------------------------  Receive proprietary data into buffer
   int 
   DAQSocket::GetData(char *buffer, int length, long timeout) {
      semlock lockit (mux);
      if (mWriterType != DataWriter) 
         return -1;
      char* bufpt = buffer + sizeof(DAQDRecHdr);
      int nbyt = RecvData(bufpt, length-sizeof(DAQDRecHdr), 
                         (DAQDRecHdr*)buffer, timeout);
      if (nbyt <= 0) 
         return nbyt;
      return nbyt;
   }

   int DAQSocket::GetData (char** buffer, long timeout) {
      semlock lockit (mux);
      *buffer = 0;
      if (mWriterType != DataWriter) 
         return -10;
      int nbyt = RecvData (buffer, timeout);
      if (nbyt <= 0) {
         return nbyt;
      }
      return nbyt;
   }

//--------------------------------------  Receive proprietary data into buffer
   int 
   DAQSocket::RecvData(char* buffer, int length, DAQDRecHdr* hdr,
                     long timeout) {
      DAQDRecHdr header, *h;
      timeval	maxwait;
      maxwait.tv_sec = timeout < 0 ? -1 : timeout;
      maxwait.tv_usec = 0;
   
    //----------------------------------  Set header pointer.
      if (hdr) {
         h = hdr;
      } 
      else {
         h = &header;
      }
   
    //----------------------------------  Read in the data header. Calculate
    //                                    the record length.
      int rc = RecvRec((char*) h, sizeof(DAQDRecHdr), true, &maxwait);
      if (rc != sizeof(DAQDRecHdr)) 
         return -1;
      if (mReorder) SwapI((int*) h, sizeof(DAQDRecHdr)/sizeof(int));
      if (mDebug) {
         cerr << "Record Header: BLen=" << h->Blen << " Secs=" << h->Secs 
            << " GPS=" << h->GPS << " NSec=" << h->NSec << " SeqNum=" 
            << h->SeqNum << endl;
      }
      int ndata = h->Blen - (sizeof(DAQDRecHdr) - sizeof(int));
      if (ndata == 0) 
         return 0;
      if (ndata  < 0) 
         return -1;
      if (ndata > length) {
         cerr << "DAQSocket::RecvData - Buffer length (" << length 
            << " bytes) is too small for record (" << ndata 
            << " bytes)." << endl;
         return -1;
      }
   
    //----------------------------------  Read in the data body
      rc = RecvRec(buffer, ndata, true, &maxwait);
      return rc;
   }

   int 
   DAQSocket::RecvData (char** buffer, long timeout) 
   {
      DAQDRecHdr h;
      memset (&h, 0, sizeof(DAQDRecHdr));
      *buffer = 0;
      timeval	maxwait;
      maxwait.tv_sec = timeout < 0 ? -1 : timeout;
      maxwait.tv_usec = 0;
   
    //----------------------------------  Read in the data header. Calculate
    //                                    the record length.
      int rc = RecvRec((char*) &h, sizeof(int), true, &maxwait);
      if (rc != sizeof(int)) { 
         return -2;
      }
      if (mReorder) SwapI((int*) &h, sizeof(int));
      int min = h.Blen < (int) (sizeof(DAQDRecHdr) - sizeof(int)) ? 
         h.Blen : sizeof(DAQDRecHdr) - sizeof(int);
      if (min > 0) {
         rc = RecvRec((char*) &h.Secs, min, true, &maxwait);
         if (rc != min) { 
            return -3;
         }
      }
      if (mReorder) SwapI((int*) &h.Secs, sizeof(DAQDRecHdr)/sizeof(int)-1);
      if (mDebug) {
         cerr << "Record Header: BLen=" << h.Blen << " Secs=" << h.Secs 
            << " GPS=" << h.GPS << " NSec=" << h.NSec << " SeqNum=" 
            << h.SeqNum << endl;
      }
      int ndata = h.Blen - (int)(sizeof(DAQDRecHdr) - sizeof(int));
      if (ndata <= 0) {
         ndata = 0;
      }
      *buffer = new (nothrow) char [ndata + sizeof(DAQDRecHdr)];
      if (*buffer == 0) {
         return -4;
      }
      memcpy (*buffer, &h, sizeof(DAQDRecHdr));
      if (ndata == 0) 
         return 0;
   
    //----------------------------------  Read in the data body
      rc = RecvRec (*buffer + sizeof(DAQDRecHdr), ndata, true, &maxwait);
      return rc;
   }

//--------------------------------------  Get a list of available channels
   int 
   DAQSocket::Available (std::vector<DAQDChannel>& list) 
   {
      semlock lockit (mux);
      char buf[1024];
      bool extendedList = ((mVersion > 11) || 
                          ((mVersion == 11) && (mRevision >= 3)));
      bool longNames = (mVersion >= 12) ;	// longer names introduced in version 12.
      int  nameOffset ;				// Either 40 or MAX_CHNNAME_SIZE
   
    //----------------------------------  Request a list of channels.
      int rc = 0;
      if (extendedList) {
         rc = SendRequest("status channels 2;", buf, 8);
      }
      else {
         rc = SendRequest("status channels;", buf, 4);
      }
      if (rc) 
         return -1;
   
    //----------------------------------  Get the number of channels.
      int nw= extendedList ? CVHex(buf, 8) : CVHex(buf, 4);
      int recsz = 52;
      if ((mVersion == 9) || (mVersion == 10)) recsz = 60;
      if (mVersion >= 11) recsz = 124;
      if (extendedList) recsz = 128;
      if (longNames) recsz = 128 + (MAX_CHNNAME_SIZE - 40) ;

    //----------------------------------  Skip DAQ data rate.
      if (!extendedList) rc = RecvRec(buf, 4, true);
   
    //----------------------------------  Fill in the list (test)
      list.clear();
      DAQDChannel chn;
      for (int i = 0 ; i < nw ; i++) {
         rc = RecvRec(buf, recsz, true);
         if (rc <recsz) 
            return -1;
         memcpy(chn.mName, buf, MAX_CHNNAME_SIZE);
         for (int j=(MAX_CHNNAME_SIZE-1) ; j>=0 && isspace(chn.mName[j]) ; j--) {
            chn.mName[j] = 0;
         }
	 nameOffset = MAX_CHNNAME_SIZE ;
         if (extendedList) {
            chn.mRate  = CVHex (buf + nameOffset, 8);
            chn.mNum = CVHex (buf + nameOffset + 8, 8);
            chn.mGroup = CVHex (buf + nameOffset + 16, 4);
            /*chn.mBPS = CVHex (buf + 56, 4);*/
            chn.mDatatype = CVHex (buf + nameOffset + 20, 4);
            *((int*)(&chn.mGain)) = CVHex(buf + nameOffset + 24, 8);
            *((int*)(&chn.mSlope)) = CVHex(buf + nameOffset + 32, 8);
            *((int*)(&chn.mOffset)) = CVHex(buf + nameOffset + 40, 8);
            memcpy(chn.mUnit, buf + nameOffset + 48, 40);     
	    for (int j=39; j>=0 && isspace(chn.mUnit[j]) ; j--) {
               chn.mUnit[j] = 0;
            }
	 }
	 else {
            chn.mRate  = CVHex(buf+40, 4);
            chn.mNum = CVHex(buf+44, 4);
            chn.mGroup = CVHex(buf+48, 4);
            if (recsz > 52) {
               chn.mBPS = CVHex(buf+52, 4);
               chn.mDatatype = CVHex(buf+56, 4);
            } 
            else {
               chn.mBPS = 0;
               chn.mDatatype = 0;
            }
            if (recsz > 60) {
               *((int*)(&chn.mGain)) = CVHex(buf+60, 8);
               *((int*)(&chn.mSlope)) = CVHex(buf+68, 8);
               *((int*)(&chn.mOffset)) = CVHex(buf+76, 8);
               memcpy(chn.mUnit, buf+84, 40);
               for (int j=39; j>=0 && isspace(chn.mUnit[j]) ; j--) {
        	  chn.mUnit[j] = 0;
               }
            }
            else {
               chn.mGain = 0;
               chn.mSlope = 0;
               chn.mOffset = 0;
               strcpy (chn.mUnit, "");
            }
	 }
         list.push_back (chn);
      }
      return nw;
   }

//--------------------------------------  Get a list of available channels
   int 
   DAQSocket::Available(DAQDChannel list[], int N) {
      std::vector<DAQDChannel> l;
      int rc = Available (l);
      if (rc < 0) {
         return rc;
      }
      for (int i = 0; (i < N) && (i < rc); ++i) {
         list[i] = l[i];
      }
      return rc;
   }

//--------------------------------------  Get a list of available channels
   int 
   DAQSocket::Times (unsigned long& start, unsigned long& duration, 
                    long timeout) 
   {
      semlock lockit (mux);
      timeval	maxwait;
      maxwait.tv_sec = timeout < 0 ? -1 : timeout;
      maxwait.tv_usec = 0;
    //----------------------------------  Build the request
      string request = "status main filesys;";
   
    //----------------------------------  Send the request
      int rc = SendRequest(request.c_str(), mWriter, sizeof(mWriter),
                          0, &maxwait);
      if (mDebug) cerr << mWriter << " = " << CVHex (mWriter, 8) << endl;
      if (rc) {
         return rc;
      }
      mWriterType = DataWriter;
   
    //----------------------------------  Read in the header
      int ldata = RecvRec((char*) &mOffline, sizeof(mOffline), false, &maxwait);
      if (mDebug) cerr << mOffline << endl;
      if (ldata != sizeof(mOffline)) 
         return ldata;
   
    //----------------------------------  Get an empty data block
      DAQDRecHdr hdr;
      rc = RecvData(0, 0, &hdr, timeout);
      if (rc) {
         if (mDebug) cerr << "times failed" << rc << endl;
         return rc;
      }
      start = hdr.GPS;
      duration = hdr.Secs;
    //----------------------------------  Done, return
      return rc;
   
   }

//--------------------------------------  Get a list of available channels
   int 
   DAQSocket::TimesTrend (unsigned long& start, unsigned long& duration, 
                     bool mintrend, long timeout) 
   {
      semlock lockit (mux);
      timeval	maxwait;
      maxwait.tv_sec = timeout < 0 ? -1 : timeout;
      maxwait.tv_usec = 0;
    //----------------------------------  Build the request
      string request = mintrend ? 
         "status minute-trend filesys;" : "status trend filesys;";
   
    //----------------------------------  Send the request
      int rc = SendRequest(request.c_str(), mWriter, sizeof(mWriter),
                          0, &maxwait);
      if (mDebug) cerr << mWriter << " = " << CVHex (mWriter, 8) << endl;
      if (rc) {
         return rc;
      }
      mWriterType = DataWriter;
   
    //----------------------------------  Read in the header
      int ldata = RecvRec((char*) &mOffline, sizeof(mOffline), false, &maxwait);
      if (mDebug) cerr << mOffline << endl;
      if (ldata != sizeof(mOffline)) 
         return ldata;
   
    //----------------------------------  Get an empty data block
      DAQDRecHdr hdr;
      rc = RecvData(0, 0, &hdr, timeout);
      if (rc) {
         return rc;
      }
      start = hdr.GPS;
      duration = hdr.Secs;
    //----------------------------------  Done, return
      return rc;
   
   }

//--------------------------------------  Send a request, wait for response.
   int
   DAQSocket::SendRequest(const char* text, char *reply, int length, 
                     int *Size, timeval* maxwait) {
      char status[4];
   
    //----------------------------------  Send the request
      if (mDebug) cerr << "Request: " << text << endl;
      int rc = SendRec (text, strlen(text), maxwait);
      if (rc <= 0) {
         if (mDebug) cerr << "send ret1 = " << rc << endl; 
         return rc;
      }
   
    //----------------------------------  Return if no reply expected.
      if (reply == 0) 
         return 0;
   
    //----------------------------------  Read the reply status.
      rc = RecvRec (status, 4, true, maxwait);
      if (rc != 4) {
         if (mDebug) cerr << "send ret2 = " << rc << endl; 
         return -1;
      }
      if (mDebug) cerr << "Status: " << status << endl;
      rc = CVHex(status, 4);
      if (rc) 
         return rc;
   
    //----------------------------------  Read the reply text.
      if (length != 0) {
         rc = RecvRec(reply, length, true, maxwait);
         if (rc < 0) {
            if (mDebug) cerr << "send ret3 = " << rc << endl; 
            return rc;
         }
         if (rc < length) reply[rc] = 0;
         if (mDebug) cerr << "reply: " << reply << endl;
         if (Size)       *Size = rc;
      }
      return 0;
   }

//--------------------------------------  Receive data into a buffer
   int 
   DAQSocket::RecvRec(char *buffer, int length, bool readall, timeval* maxwait) {
      Time stop;
      int 	fileflags;
      char* point = buffer;
      int   nRead = 0;
      int poll = ((maxwait != 0) && (maxwait->tv_sec >= 0));
      if (poll) {
         stop = Now() + Interval (maxwait->tv_sec, 1000*maxwait->tv_usec);
      }
      bool timedout = false;
      do {
         // use select to test for timeout
         if (poll || mAbort) {
            int nset;
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(mSocket, &readfds); 
            timeval timeout;
            if (mAbort) {
               timeout.tv_sec = 0;
               timeout.tv_usec = 100000; // 100ms
            } 
            else {
               timeout.tv_sec = maxwait->tv_sec;
               timeout.tv_usec = maxwait->tv_usec;
            }
            // wait
            nset = select (FD_SETSIZE, &readfds, 0, 0, &timeout);
            //cerr << "select returns with " << nset << endl;
            if (nset < 0) {
               perror("DAQSocket: Error in select()");
               return -12;
            }
            if (nset == 0) {
               // timeout?
               if (errno == 0) {
                  if (!mAbort || *mAbort) {
                     return -13;
                  }
               }
               // signal
               else {
                  cerr << "Signal received in select ++++++++++++++++++++++++++++++++++++++++++++" << endl;
                  continue;
               }
            }
            // compute how much time is left in timeout
            if (poll) {
               Interval diff = stop - Now();
               if (diff <= Interval (0.)) {
                  maxwait->tv_sec = 0;
                  maxwait->tv_usec = 0;
                  timedout = true;
               }
               else {
                  maxwait->tv_sec = diff.GetS();
                  maxwait->tv_usec = diff.GetN() / 1000;
               }
            }
            // continue if no data and abort not set
            if ((nset == 0) && mAbort && !timedout) {
               continue;
            }
            /* set socket to non blocking */
            if ((fileflags = fcntl (mSocket, F_GETFL, 0)) == -1) {
               return -1;
            }
            if (fcntl (mSocket, F_SETFL, fileflags | O_NONBLOCK) == -1) {
               return -1;
            }
         }
         int nB = recv(mSocket, point, length - nRead, 0);
         //cerr << "recv returns with " << nB << endl;
         if (poll || mAbort) {
            if (mDebug && (nB == 0)) { 
               cerr << "RecvRec with zero length" << endl;
            }
            fcntl (mSocket, F_SETFL, fileflags & !O_NONBLOCK);
         }
         if (nB == -1) {
            if (mDebug)
               cerr << "RecvRec failed with errno " << errno << endl;
            return -10;
         }
         point += nB;
         nRead += nB;
         if (timedout || (mAbort && *mAbort)) {
            return -13;
         }
      } while (readall && (nRead < length));
      if (mDebug) cerr << "RecvRec read " << nRead << "/" << length << endl;
      return nRead;
   }


//--------------------------------------  Send data from a buffer
   int 
   DAQSocket::SendRec(const char *buffer, int length, timeval* maxwait) {
      int 	fileflags;
      const char* point = buffer;
      int   nWrite = 0;
      int poll = (maxwait != 0) && (maxwait->tv_sec >= 0);
   
      bool timedout = false;
      do {
         // use select to test for timeout
         if (poll || mAbort) {
            int nset;
            fd_set writefds;
            FD_ZERO(&writefds);
            FD_SET(mSocket, &writefds); 
            timeval timeout;
            if (mAbort) {
               timeout.tv_sec = 0;
               timeout.tv_usec = 100000; // 100ms
            } 
            else {
               timeout.tv_sec = maxwait->tv_sec;
               timeout.tv_usec = maxwait->tv_usec;
            }
            Time now = Now();
            // wait
            nset = select (FD_SETSIZE, 0, &writefds, 0, &timeout);
            if (nset < 0) {
               perror("DAQSocket: Error in select()");
               return -12;
            }
            else if (nset == 0) {
               // timeout error
               if (!mAbort || *mAbort) {
                  return -13;
               }
            }
            // compute how much time is left in timeout
            if (poll) {
               Interval diff = Now() - now;
               Interval wait (maxwait->tv_sec, 1000*maxwait->tv_usec);
               diff = wait - diff;
               if (diff < Interval (0.)) {
                  maxwait->tv_sec = 0;
                  maxwait->tv_usec = 0;
                  timedout = true;
               }
               else {
                  maxwait->tv_sec = diff.GetS();
                  maxwait->tv_usec = diff.GetN() / 1000;
               }
            }
            // continue if no data sent and abort not set
            if ((nset == 0) && mAbort && !timedout) {
               continue;
            }
            /* set socket to non blocking */
            if ((fileflags = fcntl (mSocket, F_GETFL, 0)) == -1) {
               return -1;
            }
            if (fcntl (mSocket, F_SETFL, fileflags | O_NONBLOCK) == -1) {
               return -1;
            }
         }
         int nB = send (mSocket, point, length - nWrite, 0);
         if (poll || mAbort) {
            fcntl (mSocket, F_SETFL, fileflags & !O_NONBLOCK);
         }
         if (nB == -1) {
            if (mDebug)
               cerr << "SendRec failed with errno " << errno << endl;
            return -10;
         }
         point += nB;
         nWrite += nB;
         if (timedout || (mAbort && *mAbort)) {
            return -13;
         }
      } while (nWrite < length);
      if (mDebug) cerr << "SendRec write " << nWrite << "/" << length << endl;
      return nWrite;
   }


