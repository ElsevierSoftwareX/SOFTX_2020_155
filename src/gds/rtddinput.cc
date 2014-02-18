static const char *versionId = "Version $Id$" ;
/* -*- mode: c++; c-basic-offset: 3; -*- */
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: rttdinput						*/
/*                                                         		*/
/* Module Description: reads in channel data through the RTDD interface	*/
/* implements decimation and zoom functions, partitions the data and	*/
/* stores it in a storage object					*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

//#define DEBUG

// Header File List:
#include "dtt/rtddinput.hh"
#include <strings.h>
#include <signal.h>
#include <pthread.h>
#include <math.h>
#include <errno.h>
#include <algorithm>
#include <iostream>
#include "tconv.h"
#include "dtt/gdsprm.h"
#include "dtt/gdstask.h"
#include "dtt/map.h"
#include "framefast/fftype.hh"
#if defined (_CONFIG_DYNAMIC)
#include "dtt/confinfo.h" 
#endif

// TODO: will nto compile, can't find NDS1Socket.hh nor NDS2Socket.hh

#ifdef NDS2_API_VERSION
#include "NDS1Socket.hh"
#include "NDS2Socket.hh"
#endif



namespace diag {
   using namespace std;
   using namespace thread;
   using namespace framefast;


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Constants: PRM_FILE		  parameter file name			*/
/*            PRM_SECTION	  section heading is channel name!	*/
/*            PRM_SERVERNAME	  entry for server name			*/
/*            PRM_SERVERPORT	  entry for server port			*/
/*            DAQD_SERVER	  default server name for channel info	*/
/*            DAQD_PORT		  default server port for channel info	*/
/*            __ONESEC		  one second (in nsec)			*/
/*            _MIN_NDS_DELAY	  minimum delay allowed for NDS (sec)	*/
/*            _MAX_NDS_DELAY	  maximum delay allowed for NDS (sec)	*/
/*            _NDS_DELAY	  NDS delay for slow data (sec)		*/
/*            taskNdsName	  nds task priority			*/
/*            taskNdsPriority	  nds task name				*/
/*            taskCleanupName	  nds clenaup task priority		*/
/*            taskCleanupPriority nds cleanup task name			*/
/*            daqBufLen		  length of receiving socket buffer	*/
/*            								*/
/*----------------------------------------------------------------------*/
#define _CHNLIST_SIZE		200
#if !defined (_CONFIG_DYNAMIC)
#define PRM_FILE		gdsPathFile ("/param", "nds.par")
#define PRM_SECTION		gdsSectionSite ("nds")
#define PRM_SERVERNAME		"hostname"
#define PRM_SERVERPORT		"port"
#define DAQD_SERVER		"fb0"
#define DAQD_PORT		8088
#endif
#define _MIN_NDS_DELAY		0.0
#define _MAX_NDS_DELAY		5.0 // 5 seconds
#define _NDS_DELAY		1

   const char	taskNdsName[] = "tNDS";
   const int	taskNdsPriority = 0;
   const char	taskCleanupName[] = "tNDScleanup";
   const int	taskCleanupPriority = 20;
   const int 	daqBufLen = 1024*1024;
   const long	taskNdsOnlineTimeout = 5;
   const long	taskNdsOfflineTimeout = 24 * 3600;  // 1 day!
   const bool 	kNdsDebug = false;

   const double __ONESEC = (double) _ONESEC;
   // timeout for receiving data (real-time interface)
   const tainsec_t timeoutWaitRT = 
   (tainsec_t)taskNdsOnlineTimeout * _ONESEC;
   // timeout for receiving data (off-line interface)
   const tainsec_t timeoutWaitOL = 
   (tainsec_t)taskNdsOfflineTimeout * _ONESEC;


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Class Name: rtddManager						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int rtddManager::ndstask (rtddManager& RTDDMgr) 
   {
#ifdef  NDS2_API_VERSION
      DAQC_api*	        lclnds = RTDDMgr.nds;
#else
      DAQSocket*	lclnds = RTDDMgr.nds;
#endif
      char*		buf = 0;// data buffer
      int		len;	// length of read buffer
      int		seqNum = -1;
      int		err;
      int		reconf;
      const timespec tick = {0, 1000000}; // 1ms
   
      // wait for data
      pthread_setcancelstate (PTHREAD_CANCEL_DISABLE, 0);
      while (1) {
         // get the mutex
         while (!RTDDMgr.ndsmux.trylock()) {
            pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, 0);
            nanosleep (&tick, 0);
            pthread_testcancel();
            pthread_setcancelstate (PTHREAD_CANCEL_DISABLE, 0);
         }
         // check if data is ready
         // tainsec_t	t1 = TAInow();
         err = lclnds->WaitforData (true);
         if (err < 0) {
            cerr << "NDS socket ERROR" << endl;
	    RTDDMgr.shut();
	    RTDDMgr.ndsmux.unlock();
            return -1;
         }
         else if (err == 0) {
            RTDDMgr.ndsmux.unlock();
            pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, 0);
            nanosleep (&tick, 0);
            pthread_testcancel();
            pthread_setcancelstate (PTHREAD_CANCEL_DISABLE, 0);
            continue;
         }
         // get data
         err = 0;
         cerr << "get data from nds" << endl;
         len = lclnds->GetData (&buf); //, 6 * taskNdsGetDataTimeout);
         cerr << "got data from nds " << len << " (>0 length, " <<
            "<0 error, -13 timeout)" << endl;
         if (len == 0) {
            cerr << "Data block with length 0 encountered " << 
               "****************************" << endl;
         }
         // reconfig block?
         reconf = 0;
         if ((len > 0) && 
            (((DAQDRecHdr*) buf)->GPS == (int)0x0FFFFFFFF)) {
            reconf = 1;
         }
         // check sequence number
         else if (len > 0) {
            err = (seqNum >= 0) && 
               (((DAQDRecHdr*) buf)->SeqNum != seqNum + 1) ? 1 : 0;
            seqNum = ((DAQDRecHdr*) buf)->SeqNum; 
            cerr << "seq # = " << seqNum << endl;
         }
         if (err || (len < 0)) {
            cerr << "DATA RECEIVING ERROR " << len << " errno " << errno << endl;
            // exit (1);
         }
      
         // process reconfigure information
         if (reconf) {
            // just skip for now
         }
         // process received data
         else if (len > 0) {
            if (!RTDDMgr.ndsdata (buf, err)) {
               len = -1;
            }
         }
         // end of data transmission encountered
         else if ((len <= 0) && !RTDDMgr.fastUpdate) {
            if (buf) {
               cerr << "TRAILER TIME = " << ((DAQDRecHdr*) buf)->GPS << endl;
            }
         }
         delete [] buf;
         buf = 0;
         // quit if end of transfer is reached or on fatal error
         if ((len < 0) || ((len == 0) && !RTDDMgr.fastUpdate)) {
            if ((len <= 0) && !RTDDMgr.fastUpdate) {
               RTDDMgr.dataCheckEnd();
            }
	    RTDDMgr.shut();
	    RTDDMgr.ndsmux.unlock();
            return -1;
         }
         RTDDMgr.ndsmux.unlock();
         // tainsec_t	t2 = TAInow();
         // cerr << "TIME ndstask = " << (double)(t2-t1)/1E9 << endl;
         pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, 0);
         pthread_testcancel();
         pthread_setcancelstate (PTHREAD_CANCEL_DISABLE, 0);
      }
   
      return 0;
   }


   rtddManager::rtddManager (gdsStorage* dat, testpointMgr* TPMgr, 
                     double Lazytime) 
   : dataBroker (dat, TPMgr, Lazytime), userNDS (false), 
     RTmode (false), fastUpdate (false), abort(0), nds(0)
   {
      strcpy (daqServer, "");
      daqPort = 0;
   }

   rtddManager::~rtddManager (void) {
      if (nds) {
	 //if (nds->isOpen()) nds->close();
	 delete nds;
      }
   }

   class chnorder {
   public:
      chnorder() {}
      bool operator() (const DAQDChannel& c1, const DAQDChannel& c2) const {
#ifdef NDS2_API_VERSION
         return strcasecmp (c1.mName.c_str(), c2.mName.c_str() ) < 0;
#else
         return strcasecmp (c1.mName, c2.mName) < 0;
#endif
      }
   };

   bool rtddManager::connect (const char* server, int port, bool usernds)
   {
      int	 status;
   
      // get NDS parameters
      if (server == 0) {
         // dynamic configuration
      #if defined (_CONFIG_DYNAMIC)
         const char* const* cinfo;	// configuration info
         confinfo_t	crec;		// conf. info record
         for (cinfo = getConfInfo (0, 0); cinfo && *cinfo; cinfo++) {
            if ((parseConfInfo (*cinfo, &crec) == 0) &&
               (gds_strcasecmp (crec.interface, 
                                CONFIG_SERVICE_NDS) == 0) &&
               (crec.ifo == -1) && (crec.progver == -1)) {
               strcpy (daqServer, crec.host);
               daqPort = crec.port_prognum;
            }
         }
      #else
         // from parameter file
         strcpy (daqServer, DAQD_SERVER);
         loadStringParam (PRM_FILE, PRM_SECTION, PRM_SERVERNAME, daqServer);
         daqPort = DAQD_PORT;
         loadIntParam (PRM_FILE, PRM_SECTION, PRM_SERVERPORT, &daqPort);
      #endif
         if (daqPort <= 0) {
            daqPort = DAQD_PORT;
         }
      }
      else {
         // user specified
         strncpy (daqServer, server, sizeof (daqServer) - 1);
         daqServer[sizeof(daqServer)-1] = 0;
         daqPort = (port <= 0) ? DAQD_PORT : port;
      }
   
      // connect to NDS
#ifdef NDS2_API_VERSION
      cerr << "Open NDS, port=" << daqPort << " nds=" << long(nds) << endl;
      if (nds) delete nds;
      nds = 0;
      if (daqPort == 8088) nds = new NDS1Socket();
      else                 nds = new NDS2Socket();
      nds->setDebug (kNdsDebug);
      status = nds->open (daqServer, daqPort, daqBufLen);
#else
      if (!nds) nds = new DAQSocket();
      nds->setDebug (kNdsDebug);
      status = nds->open (daqServer, daqPort, daqBufLen);
#endif
      cerr << "NDS version = " << nds->Version() << endl;
      if (status != 0) {
         return false;
      }
      // get channel list if user NDS
      userNDS = usernds;
      if (usernds) {
#ifdef NDS2_API_VERSION
         nds->Available (cUnknown, 0, userChnList);
#else
         nds->Available (userChnList);
#endif
         sort (userChnList.begin(), userChnList.end(), chnorder());
      }
   
      return true;
   }


   bool rtddManager::set (tainsec_t start, tainsec_t* active)
   {
      semlock		lockit (mux);	// lock mutex */
   
      // check if lazy clears have to be committed
      if ((cleartime > 0) && !areUsed()) {
         mux.unlock();
         if (!dataStop ()) {
            return false;
         }
         mux.lock();
         channellist::iterator iter = channels.begin();
         while (iter != channels.end()) {
            // if not used delete
            if (iter->inUseCount() <= 0) {
               nds->RmChannel (iter->getChnName());
               iter = channels.erase (iter);
            }
            else {
               iter++;
            }
         }
      }
   
      // set minimum active time
      if (active != 0) {
         *active = start;
      }
      cleartime = 0;
      // check if already set
   
      if (!areSet()) {
         // make sure nds is stopped
         mux.unlock();
         if (!dataStop ()) {
            return false;
         }
         mux.lock();
	 bool setok = set_channel_list(start, active);
	 if (!setok) return false;
      }
   
      // all set: start nds
      if (!ndsStart ()) {
         for (channellist::iterator iter = channels.begin();
             iter != channels.end(); iter++) {
            iter->unsubscribe();
         }     
         return false;
      }
      // round active time to next second after adding max. filter delays
      if (active != 0) {
         tainsec_t 	now = TAInow();
         now = _ONESEC * ((now + _ONESEC - 1) / _ONESEC);
         *active = max (now, *active);
      }
   
      return true;
   }


   bool rtddManager::set (taisec_t start, taisec_t duration)
   {
      semlock		lockit (mux);	// lock mutex */
      cerr << "TIME STAMP BEFORE START = " << timeStamp() << endl;
   
      // make sure nds is stopped
      mux.unlock();
      if (!dataStop ()) {
         return false;
      }
      mux.lock();
      cleartime = 0;
   
      // setup channels
      bool setok = set_channel_list(start*_ONESEC, 0);
      if (!setok) return false;
   
      // all set: start nds
      cerr << "start NDS @ " << start << ":" << duration << endl;
      if (!ndsStart(start, duration)) {
         for (channellist::iterator iter = channels.begin();
             iter != channels.end(); iter++) {
            iter->unsubscribe();
         }     
         return false;
      }
   
      cerr << "start NDS @ " << start << ":" << duration << " done" << endl;
      return true;
   }

   bool rtddManager::set_channel_list(tainsec_t start, tainsec_t *active) {
      tainsec_t	chnactive = 0;	// time when channel active

      nds->RmChannel ("all");
      cerr << "setup channels for NDS" << endl;
      // setup channels
      for (channellist::iterator iter = channels.begin();
	   iter != channels.end(); iter++) {
	 // add channel to list
#ifdef NDS2_API_VERSION
	 nds->AddChannel (iter->getChnName(), iter->getDatarate(), 
			  static_cast<datatype>(iter->getDatatype()));
#else
	 nds->AddChannel (iter->getChnName(), 
			  DAQSocket::rate_bps_pair 
			  (iter->getDatarate(), iter->getBps()));
#endif
	 if (!iter->isSet()) {
	    // activate channel
	    if (!iter->subscribe (start, &chnactive)) {
	       // error
	       for (channellist::reverse_iterator iter2 (iter);
		    iter2 != channels.rend(); iter2++) {
		  iter2->unsubscribe();
	       }
	       nds->RmChannel ("all");
	       return false;
	    }
	    if (active != 0) *active = max (chnactive, *active);
	 }
      }
      return true;
   }

   bool rtddManager::channelInfo (const string& name, 
                     gdsChnInfo_t& info) const
   {
      if (!userNDS) {
         return channelHandler::channelInfo (name, info);
      }
      else {
         // find channel
         DAQDChannel item;
#ifdef NDS2_API_VERSION
	 item.mName = name;
#else
         strncpy (item.mName, name.c_str(), sizeof (item.mName) - 1);
         item.mName[sizeof(item.mName)-1] = 0;
#endif
         vector<DAQDChannel>::const_iterator chn =
            lower_bound (userChnList.begin(), userChnList.end(),
                        item, chnorder());
#ifdef NDS2_API_VERSION
         if ((chn == userChnList.end()) ||
	     (strcasecmp (item.mName.c_str(), chn->mName.c_str()) != 0)) {
            return false;
         }
         else {
            strncpy (info.chName, chn->mName.c_str(), sizeof (info.chName)-1);
            info.chName[sizeof(info.chName)-1] = 0;
            info.chGroup = chn->mChanType;
            info.dataRate = chn->mRate;
            info.dataType = chn->mDatatype;
            info.gain = chn->mGain;
            info.slope = chn->mSlope;
            info.offset = chn->mOffset;
            strncpy (info.unit, chn->mUnit.c_str(), sizeof (info.unit)-1);
            info.unit[sizeof(info.unit)-1] = 0;
            return true;
         }
#else
         memset (&info, 0, sizeof (gdsChnInfo_t));
         if ((chn == userChnList.end()) ||
	     (strcasecmp (item.mName, chn->mName) != 0)) {
            return false;
         }
         else {
            strncpy (info.chName, chn->mName.c_str(), sizeof (info.chName)-1);
            info.chName[sizeof(info.chName)-1] = 0;
            info.chGroup = chn->mGroup;
            info.dataRate = chn->mRate;
            info.bps = chn->mBPS;
            info.dataType = chn->mDatatype;
            info.gain = chn->mGain;
            info.slope = chn->mSlope;
            info.offset = chn->mOffset;
            strncpy (info.unit, chn->mUnit, sizeof (info.unit)-1);
            info.unit[sizeof(info.unit)-1] = 0;
            return true;
         }
#endif
      }
   }

   void rtddManager::shut(void) {
      TID = 0;
      if (nds) {
	 nds->StopWriter ();
	 nds->RmChannel ("all");
	 nds->close();
	 delete nds;
	 nds = 0;
      }
   }

   bool rtddManager::getTimes (taisec_t& start, taisec_t& duration)
   {
      start = 0;
      duration = 0;
      // get time segments
#ifdef NDS2_API_VERSION
      return (nds->Times (cRaw, start, duration) == 0);
#else
      return (nds->Times (start, duration) == 0);
#endif
   }


   tainsec_t rtddManager::timeoutValue (bool online) const
   {
      return online ? timeoutWaitRT : timeoutWaitOL;
   }


template <class T>
   inline void convertRTDDData (float x[], T y[], int len)
   {
      if (littleendian()) {
         T tmp;
         for (int i = 0; i < len; i++) {
            tmp = y[i];
            swap (&tmp);
            x[i] = static_cast<float>(tmp);
         }
      }
      else {
         for (int i = 0; i < len; i++) {
            x[i] = static_cast<float>(y[i]);
         }
      }
   }


   bool rtddManager::ndsdata (const char* buf, int err)
   {

      if (buf == 0) {
         return true;
      }
      semlock		lockit (mux);	// lock mutex 
      DAQDRecHdr*	head = (DAQDRecHdr*) buf;
      taisec_t		time = 		// time (sec) of data
         (taisec_t) head->GPS;
      int		epoch =		// epoch of data
         (head->NSec + _EPOCH / 10) / _EPOCH;
      tainsec_t		duration = 	// time duration (sec) of data
         fastUpdate ? _EPOCH : (taisec_t) head->Secs * _ONESEC;
      tainsec_t		timestamp = 	// time stamp
         (tainsec_t)time * _ONESEC + (tainsec_t)epoch * _EPOCH;

      //cerr << "time GPS = " << 	time << " nsec = " << epoch << endl;
      if (!fastUpdate) { // NDS bug ???
         epoch = 0;
      }
      cerr << "time GPS = " << 	time << " nsec = " << epoch << 
         " duration sec = " << (double)duration / __ONESEC << endl;
   
      // check if we lost data
      if ((nexttimestamp != 0) && 
         (timestamp > nexttimestamp + 1000)) {
         cerr << "NDS RECEIVING ERROR: # of epochs lost = " <<
            (timestamp - (nexttimestamp - 1000)) / _EPOCH << endl;
      }
   
      // check NDS time
   #ifdef GDS_ONLINE
      if (RTmode) {
         double delay = (double) (TAInow() - timestamp) / __ONESEC;
         double maxdelay = _MAX_NDS_DELAY + duration / __ONESEC;
         if ((delay < _MIN_NDS_DELAY) || (delay > maxdelay)) {
            cerr << "TIMEOUT ERROR: NDS delay = " << delay << endl;
            //return false;
         }
      }
   #endif
   
      // go through channel list
#ifdef NDS2_API_VERSION
      for (DAQC_api::const_channel_iter iter = nds->chan_begin(); 
          iter != nds->chan_end(); iter++) {

         // calculate # of data points
         int ndata = iter->nwords(nds->mRecvBuf.ref_header().Secs);
      
         // find daq channel and invoke callback
         channellist::iterator chn = find (iter->mName);
         if ((chn == channels.end()) || (*chn != iter->mName)) {
            continue;
         }

	 if (iter->mDatatype != DAQ_DATATYPE_FLOAT &&
	     iter->mDatatype != DAQ_DATATYPE_COMPLEX) {
	    float* fptr = new float[ndata];
	    nds->GetChannelData(iter->mName, fptr, ndata);
	    chn->callback (time, epoch, fptr, ndata, err);
	    delete[] fptr;
	 } else {
	    float* fptr = 
	       reinterpret_cast<float*>(nds->mRecvBuf.ref_data() 
					      + iter->mBOffset);
	    chn->callback (time, epoch, fptr, ndata, err);
	 }
      }
#else
      const int   size = 32*1024;
      const char* dptr = buf + sizeof (DAQDRecHdr);
      float       fdat[size];	// data buffer
      int         datasize = head->Blen - (sizeof (DAQDRecHdr) - sizeof (int));
      int         idata = 0;	// data index
      for (DAQSocket::Channel_iter iter = nds->mChannel.begin(); 
	   iter != nds->mChannel.end(); iter++) {

         // calculate # of data points
         int ndata = (int) ((double)iter->second.mRate * 
			    ((double)duration / __ONESEC) + 0.5);

         cerr << "rate = " << iter->second.mRate << " length: ndata = " 
	      << ndata << " data size = " << datasize << " bps = " 
	      << iter->second.mBPS << " idata = " << idata << endl;

         // check buffer length
         if (idata + ((ndata == 0) ? 1 : ndata) * iter->second.mBPS > 
            datasize) {
            return false;
         }
      
         // find daq channel and invoke callback
         channellist::iterator chn = find (iter->first);
         if ((chn == channels.end()) || (*chn != iter->first)) {
            // not found; go to next data record
            idata += ((ndata == 0) ? 1 : ndata) * iter->second.mBPS;
            dptr += ((ndata == 0) ? 1 : ndata) * iter->second.mBPS;
            continue;
         }
         // check if data has to be converted into floats
         int cmplxmul = (chn->getDatatype() == DAQ_DATATYPE_COMPLEX) ? 2 : 1;
         if (((chn->getDatatype() == DAQ_DATATYPE_FLOAT) ||
             (chn->getDatatype() == DAQ_DATATYPE_COMPLEX)) && 
            !littleendian()) {
            fptr = (float*) dptr;
         }
         else {
            cmplxmul = (chn->getDatatype() == DAQ_DATATYPE_COMPLEX) ? 2 : 1;
            // allocate buffer
	    float* fptr;
            if (cmplxmul * ndata <= size) {
               fptr = fdat;
            }
            else {
               fptr = new (nothrow) float [cmplxmul * ndata];
               if (fptr == 0) {
                  idata += ((ndata == 0) ? 1 : ndata) * iter->second.mBPS;
                  dptr += ((ndata == 0) ? 1 : ndata) * iter->second.mBPS;
                  continue;
               }
            }
	    int ncvt = ndata;
            switch (chn->getDatatype()) {
	    case DAQ_DATATYPE_16BIT_INT:
	       convertRTDDData (fptr, (int_2s_t*) dptr, ndata);
	       break;
	    case DAQ_DATATYPE_32BIT_INT:
	       convertRTDDData (fptr, (int_4s_t*) dptr, ndata);
	       break;
	    case DAQ_DATATYPE_64BIT_INT:
	       convertRTDDData (fptr, (int_8s_t*) dptr, ndata);
	       break;
	    case DAQ_DATATYPE_COMPLEX:
	       ncvt *= 2;
	    case DAQ_DATATYPE_FLOAT:
	       convertRTDDData (fptr, (real_4_t*) dptr, ncvt);
	       break;
	    case DAQ_DATATYPE_DOUBLE:
	       convertRTDDData (fptr, (real_8_t*) dptr, ndata);
	       break;
	    case DAQ_DATATYPE_32BIT_UINT:
	       convertRTDDData (fptr, (int_4u_t*) dptr, ndata);
	       break;
	    default:
	       memset (fptr, 0, cmplxmul*ndata * sizeof (float));
	       break;
            }
         }
         // invoke callback
         chn->callback (time, epoch, fptr, ndata, err);
      
         // free data buffer if necessary
         if ((cmplxmul * ndata > size) &&
            !(((chn->getDatatype() == DAQ_DATATYPE_FLOAT) ||
              (chn->getDatatype() == DAQ_DATATYPE_COMPLEX)) && 
             !littleendian())) {
            delete [] fptr; 
         }
      
         // advance to next channel
         idata += ((ndata == 0) ? 1 : ndata) * iter->second.mBPS;
         dptr += ((ndata == 0) ? 1 : ndata) * iter->second.mBPS;
      }
#endif
   
   #ifndef DEBUG 
      cerr << "nds callback done " << 
         (double) ((time*_ONESEC+epoch*_EPOCH) % (1000 * _ONESEC)) / 1E9 <<
         " at " << (double) (TAInow() % (1000 * _ONESEC)) / 1E9 << endl;
      cerr << "time stamp = " << timeStamp() << endl;
   #endif
   
      // set time of last successful NDS data transfer
      nexttimestamp = timestamp + duration;
      lasttime = TAInow();
   
      return true;
   }


   bool rtddManager::ndsStart ()
   {
      // check if already running 
      if (TID != 0) {
         return true;
      }
      // check if any channels are selected
#ifdef NDS2_API_VERSION
      if (!nds->nRequest()) {
         return true;
      }
#else
      if (nds->mChannel.empty()) {
         return true;
      }
#endif

      // start net writer
      cerr << "nds start" << endl;
      abort = false;
      nds->setAbort (&abort);
      RTmode = true;
#ifdef NDS2_API_VERSION
      fastUpdate = true;
#else
      fastUpdate = true;
      for (DAQSocket::Channel_iter iter = nds->mChannel.begin(); 
          iter != nds->mChannel.end(); iter++) {
         // check data rate
         if (iter->second.mRate < NUMBER_OF_EPOCHS) {
            fastUpdate = false;
            break;
         }
      }
#endif   
      // set last time
      nexttimestamp = 0;
      starttime = 0;
      stoptime = 0; // no end
      lasttime = TAInow();
   
      // establish connection
#ifdef NDS2_API_VERSION
      if (!nds || !nds->isOpen()) {
	 if (!nds) {
	    if (daqPort == 8088) nds = new NDS1Socket();
	    else                 nds = new NDS2Socket();
	 }
	 nds->setDebug(true);
	 nds->open (daqServer, daqPort);
      }
      if (nds && !nds->isOpen()) {
	 nds->close();    
	 delete nds;
	 nds = 0;
         return false;
      }
#else
      if (!nds || !nds->isOpen()) {
	 if (!nds) nds = new DAQSocket();
	 nds->setDebug(true);
	 if (nds->open (daqServer, daqPort) != 0) {
	    nds->RmChannel ("all");    
	    return false;
	 }
      }
#endif
      if (nds->RequestOnlineData (fastUpdate, taskNdsOnlineTimeout) != 0) {     
         nds->RmChannel ("all");    
         return false;
      }
   
      // create nds task
      int		attr;	// task create attribute
   #ifdef OS_VXWORKS
      attr = VX_FP_TASK;
   #else
      attr = PTHREAD_CREATE_DETACHED;
   #endif
      if (taskCreate (attr, taskNdsPriority, &TID, 
                     taskNdsName, (taskfunc_t) ndstask, 
                     (taskarg_t) this) != 0) {
         nds->StopWriter();     
         nds->RmChannel ("all");    
         return false;
      }
      cerr << "nds started" << endl;
   
      return true;
   }

   bool rtddManager::ndsStart (taisec_t start, taisec_t duration)
   {
      // check if already running 
      if (TID != 0) {
         return true;
      }
      // check if any channels are selected
#ifdef NDS2_API_VERSION
      if (!nds->nRequest()) {
         return true;
      }
#else
      if (nds->mChannel.empty()) {
         return true;
      }
#endif   
      // wait for data to become available
      while (TAInow() < (start + duration + _NDS_DELAY) * _ONESEC) {
         timespec wait = {0, 250000000};
         nanosleep (&wait, 0);
      }
   
      // set last time
      nexttimestamp = start * _ONESEC;
      starttime = start * _ONESEC;
      stoptime = (start + duration) * _ONESEC;
      lasttime = TAInow();
   
      // start net writer
      RTmode = false;
      fastUpdate = false;
      cerr << "nds start old data" << endl;
      abort = false;
      nds->setAbort (&abort);
      if (!nds->isOpen() && (nds->open (daqServer, daqPort) != 0)) {
         nds->RmChannel ("all");
         cerr << "nds error during open" << endl;
         return false;
      }
      if (nds->RequestData (start, duration, taskNdsOfflineTimeout) != 0) {     
         nds->RmChannel ("all");    
         cerr << "nds error during data request" << endl;
         return false;
      }
   
      // create nds task
      int		attr;	// task create attribute
   #ifdef OS_VXWORKS
      attr = VX_FP_TASK;
   #else
      attr = PTHREAD_CREATE_DETACHED;
   #endif
      if (taskCreate (attr, taskNdsPriority, &TID, 
                     taskNdsName, (taskfunc_t) ndstask, 
                     (taskarg_t) this) != 0) {
         nds->StopWriter();     
         nds->RmChannel ("all");    
         cerr << "nds error during task spawn" << endl;
         return false;
      }
      cerr << "nds started" << endl;
   
      return true;
   }


   bool rtddManager::dataStop ()
   {
      cerr << "kill nds task: get mutex" << endl;
      // get the mutex
      int n = 30;
      const timespec tick = {0, 100000000}; // 100ms
      abort = true;
      while ((n >= 0) && !ndsmux.trylock()) {
         nanosleep (&tick, 0);
         n--;
      	 // send a signal to unblock select in daqsocket
         if (n % 10 == 2) {
            taskID_t tid = TID;
            if (tid) pthread_kill (tid, SIGCONT);
         }
      }
      if (n < 0) {
         return false;
      }
      //ndsmux.lock();
      if (TID != 0) {
         cerr << "kill nds task" << endl;
         taskCancel (&TID);
         cerr << "killed nds task" << endl;
	 shut();
         cerr << "killed nds" << endl;
      }
      ndsmux.unlock();
      return true;
   }

}
