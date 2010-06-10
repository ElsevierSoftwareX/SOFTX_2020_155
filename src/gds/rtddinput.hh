/* -*- mode: c++; c-basic-offset: 3; -*- */
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: rtddinput.h						*/
/*                                                         		*/
/* Module Description: gets data through the rtdd and stores it		*/
/*		       a storage object					*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 21Nov98  D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: gdsdatum.html					*/
/*	References: none						*/
/*                                                         		*/
/* Author Information:							*/
/* Name          Telephone       Fax             e-mail 		*/
/* Daniel Sigg   (509) 372-8132  (509) 372-2178  sigg_d@ligo.mit.edu	*/
/*                                                         		*/
/* Code Compilation and Runtime Specifications:				*/
/*	Code Compiled on: Ultra-Enterprise, Solaris 5.6-10	       	*/
/*	Compiler Used: sun workshop C++ 4.2				*/
/*	Runtime environment: sparc/solaris				*/
/*                                                         		*/
/* Code Standards Conformance:						*/
/*	Code Conforms to: LIGO standards.	OK			*/
/*			  Lint.			TBD			*/
/*			  ANSI			OK			*/
/*			  POSIX			OK			*/
/*									*/
/* Known Bugs, Limitations, Caveats:					*/
/*								 	*/
/*									*/
/*                                                         		*/
/*                      -------------------                             */
/*                                                         		*/
/*                             LIGO					*/
/*                                                         		*/
/*        THE LASER INTERFEROMETER GRAVITATIONAL WAVE OBSERVATORY.	*/
/*                                                         		*/
/*                     (C) The LIGO Project, 1996.			*/
/*                                                         		*/
/*                                                         		*/
/* California Institute of Technology			   		*/
/* LIGO Project MS 51-33				   		*/
/* Pasadena CA 91125					   		*/
/*                                                         		*/
/* Massachusetts Institute of Technology		   		*/
/* LIGO Project MS 20B-145				   		*/
/* Cambridge MA 01239					   		*/
/*                                                         		*/
/* LIGO Hanford Observatory				   		*/
/* P.O. Box 1970 S9-02					   		*/
/* Richland WA 99352					   		*/
/*                                                         		*/
/* LIGO Livingston Observatory		   				*/
/* 19100 LIGO Lane Rd.					   		*/
/* Livingston, LA 70754					   		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

#ifndef _GDS_RTDDINPUT_H
#define _GDS_RTDDINPUT_H

/* Header File List: */
#include "gmutex.hh"
#include "dtt/databroker.hh"
#define NDS2_API_VERSION 1

#ifdef NDS2_API_VERSION
#include "DAQC_api.hh"
#else
#include "DAQSocket.hh"
#endif

namespace diag {


/** @name Real-time data distribution input API
    Data is read through the real-time data distribution system,
    down-converted and decimated if necessary, partitioned and
    stored in the diagnostics storage object.

   
    @memo Reads data through the real-time data distribution.
    @author Written November 1998 by Daniel Sigg
    @version 0.1
************************************************************************/

//@{

/** @name Data types and constants
    Data types of the real-time data distribution input API

    @memo Data types of the real-time data distribution input API
************************************************************************/

//@{
#if 0
/** Compiler flag for a enabling dynamic configuration. When enabled
    the host address and interface information of network data server 
    are queried from the network rather than read in through a file.

    @author DS, June 98
    @see Test point API
************************************************************************/
#define _CONFIG_DYNAMIC
#endif

//@}


/** Class for reading a set of channels from the real-time data 
    distribution system. This object manages a list of rtddChannel
    objects.
    Usage: In general, a diagnostics test should only use add and del
    methods at the beginning and end of the test, respectively. On the 
    other hand a diagnostics supervisory task should use set and clear
    to start and stop the data flow.

    @memo Class for channel input from the rtdd.
    @author DS, November 98
    @see Real-time data distribution input API
 ************************************************************************/
   class rtddManager : public dataBroker {
   public:
      /** Constructs a real-time data distribution management object.
   	  @memo Default constructor
          @param dat storage object
          @param TPmgr test point manager
          @param Lazytime Time to wait for cleanup after a lazy clear
          @param usernds True if a user specified NDS
       ******************************************************************/
      explicit rtddManager (gdsStorage* dat = 0, 
			    testpointMgr* TPMgr = 0, 
			    double Lazytime = 0);
   
      /** Delete the rtddManager. Close and delete the nds.
        */
      ~rtddManager(void);


      /** Establishes connection to the network data server. If the
          server is a 0 pointer (default), the server name is read from
          the parameter file. If the port number is zero (default),
          the port number is read from parameter file.
          @memo Connect method.
          @param server name of NDS
          @param port port number of NDS
          @return true if successful
       ******************************************************************/
      virtual bool connect (const char* server, int port = 0, 
			    bool usernds = false);
      virtual bool connect() { 
         return connect (0, 0); }
   
      /** Requests channel data by sending a request to the RTDD API.
          (This will not set test points!) This method will request
          on-line data from the NDS.
          @memo Set method.
          @param start time when channels are needed
          @param active time when channels become available
          @return true if successful
       ******************************************************************/
      virtual bool set (tainsec_t start = 0, tainsec_t* active = 0);
   
      /** Requests channel data by sending a request to the RTDD API.
          (This will not set test points!) This method works with 
          a start time and duration and therefore will lookup data
          from the NDS archive. Start time and duration are given
          in multiples of GPS seconds.
          @memo Set method.
          @param start start time of request
          @param duration requested time interval
          @return true if successful
       ******************************************************************/
      virtual bool set (taisec_t start, taisec_t duration);
   
      /** Request all channels in the broker channel list.
          @memo Set cahnnel list method.
          @param start start time of request
          @param duration requested time interval
          @return true if successful
       ******************************************************************/
      bool set_channel_list(tainsec_t start, tainsec_t *active);

      /** Obtaines channel information. Takes user nds into account. 
          makes sure channel names are expanded correctly.
          @memo info method.
          @param name channel name
   	  @param info channel info
          @return true if successful
       ******************************************************************/
      virtual bool channelInfo (const string& name, 
                        gdsChnInfo_t& info) const;
   
      /** Requests times of data availibility from NDS.
          @memo Get times method.
          @param start start time of data (return)
          @param duration time interval (return)
          @return true if successful
       ******************************************************************/
      virtual bool getTimes (taisec_t& start, taisec_t& duration);
   
      /** Returns the maximum time client should wait for data to
          return; <=0 means wait forever. (offline access only)
          @memo Timeout value.
          @return maximum time for data to become available
       ******************************************************************/
      virtual tainsec_t timeoutValue (bool online = false) const;
   
      /** Shut down the NDS connection.
          @memo Shut down.
       ******************************************************************/
      virtual void shut(void);
   
   protected:
      /// User NDS?
      bool		userNDS;
      /// User NDS channel list
      std::vector<DAQDChannel> userChnList;
      /// Real-time mode?
      bool		RTmode;
      /// fast/slow NDS writer?
      bool		fastUpdate;
      /// abort
      bool		abort;
      /// NDS interface object
#ifdef NDS2_API_VERSION
     DAQC_api*          nds;
#else
     DAQSocket*         nds;
#endif
   private:      /// prevent copy
      rtddManager (const rtddManager&);
      rtddManager& operator= (const rtddManager&);
   
      /// mutex to protect task from being canceled
      thread::mutex		ndsmux;
      /// nds server name
      char		daqServer[256];
      /// nds server port
      int		daqPort;
      /// nds task for receiving data
      static int ndstask (rtddManager& RTDDMgr);
      /// nds callback method
      virtual bool ndsdata (const char* buf, int err = 0);
      /// nds start
      virtual bool ndsStart ();
      /// nds start with old data
      virtual bool ndsStart (taisec_t start, taisec_t duration);
      /// nds stop
      virtual bool dataStop ();
   };

//@}
}

#endif /* RTDDINPUT */
