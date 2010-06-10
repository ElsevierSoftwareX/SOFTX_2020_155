/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: lidaxinput.h						*/
/*                                                         		*/
/* Module Description: gets data through lidax and stores it in		*/
/*		       a storage object					*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 21Nov98  D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: lidaxinput.html					*/
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

#ifndef _GDS_LIDAXINPUT_H
#define _GDS_LIDAXINPUT_H

/* Header File List: */
#include "gmutex.hh"
#include "dtt/databroker.hh"
#include "dfm/dataacc.hh"

namespace diag {


/** @name LIGO Data Access (LiDaX) input API
    Data is read through LiDaX. LiDaX supports access to the LIGO
    archive, to online data (NDS), to shared memory partitions (DMT),
    to tape (TAR archives), and to local files (frame files).
   
    @memo Reads data LiDaX.
    @author Written November 2001 by Daniel Sigg
    @version 0.1
************************************************************************/

//@{


/** Class for reading a set of channels from the offline. 
    Usage: In general, a diagnostics test should only use add and del
    methods at the beginning and end of the test, respectively. On the 
    other hand a diagnostics supervisory task should use set and clear
    to start and stop the data flow.

    @memo Class for channel input from offline.
    @author DS, November 98
    @see Offline data distribution input API
 ************************************************************************/
   class lidaxManager : public dataBroker {
   public:
      /** Constructs an offline lidax data distribution management object.
   	  @memo Default constructor
          @param dat storage object
          @param TPmgr test point manager
          @param Lazytime Time to wait for cleanup after a lazy clear
       ******************************************************************/
      explicit lidaxManager (gdsStorage* dat = 0, double Lazytime = 0);
      /** Destructs the data distribution management object.
          @memo Destructor.
       ******************************************************************/
      virtual ~lidaxManager ();
   
      /** Establishes connection to the data server. Does nothing.
          @memo Connect method.
          @param server name of NDS
          @param port port number of NDS
          @return true if successful
       ******************************************************************/
      virtual bool connect (const char* server, int port = 0) {
         return setup(); }
      virtual bool connect () { 
         return connect (0, 0); }
   
      /** Gets Lidax input paramters from storage object.
          @memo Setup method.
          @return true if successful
       ******************************************************************/
      virtual bool setup();
   
      /** Requests channel data by sending a request to LiDaX.
          Online requests not supported for LiDaX.
          @memo Set method.
          @param start time when channels are needed
          @param active time when channels become available
          @return true if successful
       ******************************************************************/
      virtual bool set (tainsec_t start = 0, tainsec_t* active = 0) {
         return false; }
   
      /** Requests channel data by sending a request to LiDaX.
          This method works with a start time and duration. Start time 
          and duration are given in multiples of GPS seconds.
          @memo Set method.
          @param start start time of request
          @param duration requested time interval
          @return true if successful
       ******************************************************************/
      virtual bool set (taisec_t start, taisec_t duration);
   
      /** Obtaines channel information. Takes user nds into account. 
          makes sure channel names are expanded correctly.
          @memo info method.
          @param name channel name
   	  @param info channel info
          @return true if successful
       ******************************************************************/
      virtual bool channelInfo (const string& name, 
                        gdsChnInfo_t& info) const;
   
      /** Gets an ide string describing server and UDN list.
          @memo Id method.
          @return Id string
       ******************************************************************/
      static string id (gdsStorage& storage);
   
      /** Returns the maximum time client should wait for data to
          return; <=0 means wait forever. (offline access only)
          @memo Timeout value.
          @return maximum time for data to become available
       ******************************************************************/
      virtual tainsec_t timeoutValue (bool online = false) const;
   
      /// Clear IO and UDN caches
      static void clearCache();
   	
   protected:
      /// Abort
      bool 			fAbort;
      /// Lidax interface object
      dfm::dataaccess 		lidax;
      /// Channel list
      fantom::channellist	chnList;
   
   private:
      /// prevent copy
      lidaxManager (const lidaxManager&);
      lidaxManager& operator= (const lidaxManager&);
      /// Callback for frame output
      static bool dfm_callback (const char* frame, int len,
                        lidaxManager* ldx);
      /// Callback for frame output
      bool callback (const char* frame, int len);
   
      /// mutex to protect task from being canceled
      thread::mutex		ldxmux;
      /// lidax task for receiving data
      static int ldxtask (lidaxManager& ldxMgr);
      /// lidax start
      virtual bool dataStart (taisec_t start, taisec_t duration);
      /// lidax stop
      virtual bool dataStop ();
   };

//@}
}

#endif /* _GDS_LIDAXINPUT_H */
