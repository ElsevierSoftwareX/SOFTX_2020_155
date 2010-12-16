/* Version: $Id$ */
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: databroker.h						*/
/*                                                         		*/
/* Module Description: abstract class for accessing data		*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 21Nov98  D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: databroker.html					*/
/*	References: none						*/
/*                                                         		*/
/* Author Information:							*/
/* Name          Telephone       Fax             e-mail 		*/
/* Daniel Sigg   (509) 372-8132  (509) 372-2178  sigg_d@ligo.mit.edu	*/
/*                                                         		*/
/* Code Compilation and Runtime Specifications:				*/
/*	Code Compiled on: Ultra-Enterprise, Solaris 5.6			*/
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

#ifndef _GDS_DATABROKER_H
#define _GDS_DATABROKER_H

/* Header File List: */
#include <string>
#include <vector>
#include "dtt/channelinput.hh"
#include "dtt/gdsdatum.hh"
#include "dtt/testchn.hh"
#include "gmutex.hh"
#include "tconv.h"
#include "dtt/testpointmgr.hh"

namespace diag {


/** @name Data Broker
    Data is read through a data broker. This is an abstract class
    which is implemented as a real-time data distribution (rtddinput)
    or as an offline reader using lidax.
   
    @memo Data Broker.
    @author Written November 1998 by Daniel Sigg
    @version 0.1
************************************************************************/

//@{

/** Class for reading a set of channels from a data 
    distribution system. This object manages a list of dataChannel
    objects.
    Usage: In general, a diagnostics test should only use add and del
    methods at the beginning and end of the test, respectively. On the 
    other hand a diagnostics supervisory task should use set and clear
    to start and stop the data flow.

    @memo Class for channel input from the rtdd.
    @author DS, November 98
    @see Real-time data distribution input API
 ************************************************************************/
   class dataBroker : public channelHandler {
   public:
      /** Constructs a data distribution management object.
   	  @memo Default constructor
          @param dat storage object
          @param TPmgr test point manager
          @param Lazytime Time to wait for cleanup after a lazy clear
       ******************************************************************/
      explicit dataBroker (gdsStorage* dat = 0, 
                        testpointMgr* TPMgr = 0, double Lazytime = 0);
   
      /** Destructs the data distribution management object.
          @memo Destructor.
       ******************************************************************/
      virtual ~dataBroker ();
   
      /** Establishes connection to the network data server. If the
          server is a 0 pointer (default), the server name is read from
          the parameter file. If the port number is zero (default),
          the port number is read from parameter file.
          @memo Connect method.
          @param server name of NDS
          @param port port number of NDS
          @return true if successful
       ******************************************************************/
      virtual bool connect () = 0;
   
      /** Initializes the storage object pointer.
          @memo Init method.
          @param dat storage object
          @return true if successful
       ******************************************************************/
      virtual bool init (gdsStorage* dat = 0);
   
      /** Initializes the test point manager pointer.
          @memo Init method.
          @param TPMgr test point manager
          @return true if successful
       ******************************************************************/
      virtual bool init (testpointMgr* TPMgr);
   
      /** Returns true if all channels are set active.
          @memo Are channels set method.
          @return true if set
       ******************************************************************/
      virtual bool areSet () const;
   
      /** Returns true if all channels are in use.
          @memo Are channels used method.
          @return true if set
       ******************************************************************/
      virtual bool areUsed () const;
   
      /** Requests channel data by sending a request to the RTDD API.
          (This will not set test points!) This method will request
          on-line data from the NDS.
          @memo Set method.
          @param start time when channels are needed
          @param active time when channels become available
          @return true if successful
       ******************************************************************/
      virtual  bool set (tainsec_t start = 0, tainsec_t* active = 0) = 0;
   
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
      virtual bool set (taisec_t start, taisec_t duration) = 0;
   
      /** Returns true if the data broker is currently busy 
          reading data from the data distribution system.
          @memo Busy method.
          @return true if busy
       ******************************************************************/
      virtual bool busy () const;
   
      /** Clears channels by sending a request to the RTDD API.
          (This will not clear test points!)
          @memo Clear method.
          @param lazy lazy clear if true
          @return true if successful
       ******************************************************************/
      virtual bool clear (bool lazy = false);
   
      /** Adds a channel to the internal list. (Channel is added to
          the test point manager if it is a test point.) Optionally,
   	  the inUse count (after adding it) of the channel is returned.
          @memo Add method.
          @param name channel name
          @param inUseCount number of uses (return)
          @return true if successful
       ******************************************************************/
      virtual bool add (const string& name, int* inUseCount = 0);
   
      /** Adds a channel to the internal list. (Channel is added to
          the test point manager if it is a test point.) Optionally,
   	  the inUse count (after adding it) of the channel is returned.
          @memo Add method.
          @param chn channel information
          @param inUseCount number of uses (return)
          @return true if successful
       ******************************************************************/
      virtual bool add (const dataChannel& chn, int* inUseCount = 0);
   
      /** Adds a partitions to a channel.
          @memo Add method.
          @param chnname name of channel
          @param partitions partition list
      	  @param useActiveTime if true active time will be used 
          @return true if successful
       ******************************************************************/
      virtual bool add (const string& chnname, 
                       const dataChannel::partitionlist& partitions, 
                       bool useActiveTime = false);
   
      /** Adds a preprocessing stage to a channel.
          @memo Add method.
          @param chnname name of channel
          @param Decimate1 first decimation rate (power of 2)
          @param Decimate2 second decimation rate (power of 2)
          @param Zoomstart time zero for down-conversion
          @param Zoomfreq down-conversion frequency
      	  @param rmvDelay remove decimation filter delay
          @return true if successful
       ******************************************************************/
      virtual bool add (const string& name, 
                       int Decimate1, int Decimate2 = 1, 
                       tainsec_t Zoomstart = 0, double Zoomfreq = 0,
                       bool rmvDelay = true);
   
      /** Deletes a channel from the internal list. If a lazy clear
          was done, the the list entry is only marked for deletion.
          (Channel is deleted from the test point manager if it is a 
          test point.)
          @memo Delete method.
          @param chnname name of channel
          @return true if successful
       ******************************************************************/
      virtual bool del (const string& chnname);
   
      /** Deletes all channels from the internal list. Ignores lazy
          clear and always deletes all channels. (Channel are
          deleted from the test point manager if they are test points.)
          @memo Delete method.
          @return true if successful
       ******************************************************************/
      virtual bool del ();
   
      /** Resets a channel from the internal list. 
          @memo Reset method.
          @param chnname name of channel
          @return true if successful
       ******************************************************************/
      virtual bool reset (const string& chnname);
   
      /** Resets all channels from the internal list.
          @memo Reset method.
          @return true if successful
       ******************************************************************/
      virtual bool reset ();
   
      /** Returns the time of the received data blocks. This function
          returns the minimum of all channel time stamps. The function
          guarantees that all data samples from earlier time were 
          received.
          @memo Time stamp method.
          @return time of received data blocks
       ******************************************************************/
      virtual tainsec_t timeStamp () const;
   
      /** Returns the maximum time delay introduced by the preprocessing
          (mainly FIR filter delays).
          @memo Time delay method.
          @return maximum time delay through preprocessing
       ******************************************************************/
      virtual tainsec_t maxDelay () const;
   
      /** Returns the maximum time client should wait for data to
          return; <=0 means wait forever.
          @memo Timeout value.
          @return maximum time for data to become available
       ******************************************************************/
      virtual tainsec_t timeoutValue (bool online = false) const {
         return 0; }
   
       /** Returns the time when the last data receiving was completed.
          This function returns the actual time the transfer was 
          completed rather than the time stamp of the transfer.
          @memo Receive time method.
          @return last time data was received
       ******************************************************************/
      virtual tainsec_t receivedTime () const {
         return lasttime; }
   
   protected:
      /// channel list
      typedef std::vector <dataChannel> channellist;
   
      /// mutex to protect object
      mutable thread::recursivemutex	mux;
      /// Pointer to storage object
      gdsStorage*	storage;
      /// Pointer to test point object
      testpointMgr*	tpMgr;
      /// list of RTDD channels
      channellist	channels;
      /// time to keep test points around after clear/delete
      double		lazytime;
      /// time when a lazy clear occured (0 indicates no clear)
      double		cleartime;
      /// timestamp of last data transfer
      tainsec_t		nexttimestamp;
      /// start time of transfer (when reading old data)
      tainsec_t		starttime;
      /// stop time of transfer (when reading old data)
      tainsec_t		stoptime;
      /// actual time when last data transfer occured
      tainsec_t		lasttime;
      /// ID of nds task
      taskID_t		TID;
   
      /** Finds a channel. Returns insert position if not found.
          @memo Find method.
          @param name channel name
          @return iterator to channel object
       ******************************************************************/
      virtual channellist::const_iterator find (const string& name) const;
      virtual channellist::iterator find (const string& name);
   
      /** Cleanup task. Clears channels if lazy delay has expired.
          @memo Cleanup method.
       ******************************************************************/
      virtual void cleanup();
   
      /// stop broker
      virtual bool dataStop () = 0;
      /// check stop time of data
      virtual bool dataCheckEnd();
   
   private:
      /// prevent copy
      dataBroker (const dataBroker&);
      dataBroker& operator= (const dataBroker&);
   
      /// ID of cleanup task
      taskID_t		cleanTID;
      /// cleanup task
      static int cleanuptask (dataBroker& RTDDMgr);
   };

//@}
}

#endif /* _GDS_DATABROKER_H */
