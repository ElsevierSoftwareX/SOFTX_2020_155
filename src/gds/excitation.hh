/* Version: $Id$ */
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: excitation.h						*/
/*                                                         		*/
/* Module Description: Excitation management for  Diagnostics Tests	*/
/*									*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 4Nov98   D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: excitation.html					*/
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

#ifndef _GDS_EXCITATION_H
#define _GDS_EXCITATION_H

/* Header File List: */

#include <string>
#include <vector>
#include "tconv.h"
#include "gmutex.hh"
#include "dtt/gdsstring.h"
#include "dtt/testchn.hh"
#include "dtt/awgtype.h"

namespace diag {

   class testpointMgr;

/** @name Excitation Management API
    

   
    @memo Objects for manageing the excitation channels
    @author Written November 1998 by Daniel Sigg
    @version 0.1
 ************************************************************************/

//@{

/** @name Constants
    Constants of the excitation management API

    @memo Constants of the excitation management API
 ************************************************************************/

//@{

//@}

/** @name Data types
    * Data types of the excitation management API

    @memo Data types of the excitation management API
 ************************************************************************/

//@{

/** Represents the data type of a storage object.
 ************************************************************************/

//@}



/** @name Functions
    * Functions of the excitation management API

    @memo Functions of the excitation management API
 ************************************************************************/

//@{

//@}

/** Excitation object to manage a single excitation channel.
    This object is multi-thread safe.

    @memo Class to manage an excitation channel. 
    @author DS, November 98
    @see Excitation management API
 ************************************************************************/
   class excitation {
      friend class excitationManager;
   public:
      /// synchronization delay for excitation signals, ~200 ms
      static const double syncDelay;
      /// synchronization uncertainty for non GPS signals, ~250 ms
      static const double syncUncertainty;
      /// link speed in char/sec; first ethernet, second rs232 (cobox)
      static const double linkSpeed[2];
   
      /// Excitation channel type
      enum excitationchannel {
      /// not a channel
      invalid = 0,
      /// represents an EPICS channel
      EPICS = 1,
      /// represents a test point channel
      testpoint = 2,
      /// represents a digital-to-analog channel
      DAC = 3,
      /// represents a digital signal generator
      DSG = 4
      };
      /// Flag representing a capability of an excitation channel
      enum capabilityflag {
      /// output channel?
      output = 0,
      /// syncronized with GPS?
      GPSsync = 1,
      /// periodic signals?
      periodicsignal = 2,
      /// random noise signals?
      randomsignal = 3,
      /// arbitrary waveform?
      waveform = 4,
      /// multiple arbitrary waveforms?
      multiplewaveforms = 5
      };
      /// signal list
      typedef std::vector<AWG_Component> signallist;
      /// signal list iterator
      typedef std::vector<AWG_Component>::const_iterator const_sigiterator;
      /// point list
      typedef std::vector<float> pointlist;
   
      /// channel name
      string 		chnname;
      /// Type of excitation channel
      excitationchannel	channeltype;
      /// filter string
      string		filtercmd;
   
      /** Constructs an excitation object.
          @memo Constructor.
          @param Chnname name of channel
          @param Wait settling time
          @return void
       ******************************************************************/
      explicit excitation (const string& Chnname, double Wait = 0);
   
      /** Constructs an excitation object. When constructing a new
          excitation object from another one, the channel ownership 
          is transfered to the new excitation object, i.e. the
          old object should not be used any further.
          @memo Copy constructor.
          @param exc excitation object
          @return void
       ******************************************************************/
      excitation (const excitation& exc);
   
      /** Copies an excitation object. When copying an
          excitation object from another one, the channel ownership 
          is transfered to the new excitation object, i.e. the
          old object should not be used any further.
          @memo Copy operator.
          @param exc excitation object
          @return void
       ******************************************************************/
      excitation& operator= (const excitation& exc);
   
      /** Destructs the excitation managment object.
          @memo Destructor.
          @return void
       ******************************************************************/
      virtual ~excitation ();
   
      /** Checks validity of excitation channel.
          @memo Not operator.
          @return true if excitation channel is invalid
       ******************************************************************/
      bool operator! () const {
         return (channeltype == invalid);
      }
   
      /** Compares two channel object by their name.
          @memo Equality operator.
   	  @param exc excitation channel
          @return true if equal
       ******************************************************************/
      bool operator== (const excitation& exc) const {
         return gds_strcasecmp (chnname.c_str(), 
                              exc.chnname.c_str()) == 0;
      }
   
      /** Compares two channel object by their name.
          @memo Equality operator.
   	  @param name channel name
          @return true if equal
       ******************************************************************/
      bool operator== (const string& name) const {
         return gds_strcasecmp (chnname.c_str(), 
                              name.c_str()) == 0;
      }
   
      /** Compares two channel object by their name.
          @memo Inequality operator.
   	  @param exc excitation channel
          @return true if not equal
       ******************************************************************/
      bool operator!= (const excitation& exc) const {
         return !(*this == exc);
      }
   
      /** Compares two channel object by their name.
          @memo Inequality operator.
   	  @param name channel name
          @return true if not equal
       ******************************************************************/
      bool operator!= (const string& name) const {
         return !(*this == name);
      }
   
      /** Describes the capabilities of an excitation channel. This 
          function returns non-zero if the queried capability is 
          available for this excitation channel. Supported capability
          flags are:
          \begin{tabular}{ll}
          flag & Description  \\
       output & the channel can be used to apply an excitation \\
       GPSsync & the output signal can be syncronized with GPS \\
          periodicsignal & periodic signals are supported \\
          randomsignal & random noise signals are supported \\
          waveform & arbitary waveforms are supported \\
       multiplewaveforms & multiple arbitrary waveforms are supported
          \end{tabular}
   
          @memo Excitation channel capability.
          @param cap capability flag
          @return non-zero if capability available
       ******************************************************************/
      virtual int capability (capabilityflag cap) const;
   
      /** Sets up an excitation channel given the channel name.
          Returns false if the channel name doesn't correspond to an 
          excitation channel.
          @memo Setup of excitation channel.
          @param Chnname name of channel
          @return true if successful
       ******************************************************************/
      virtual bool setup (const string& Chnname);
   
      /** Adds a signal to an excitation channel.
          @memo Add excitation signal.
          @param comp waveform component
          @return true if successful
       ******************************************************************/
      virtual bool add (const AWG_Component& comp);
   
      /** Adds a signal list to an excitation channel. If begin
          and end describe a valid iterator pair, the return value
          points to the element behind the last added one. Thus, if 
          all components were added, it points to end.
          @memo Add excitation signals.
          @param begin iterator to first wavform component
          @param end iterator to one behind last waveform component
          @return true if successful
       ******************************************************************/
      virtual bool add (const_sigiterator begin, const_sigiterator end);
   
      /** Adds a list of waveform points to an excitation channel. This
          is only useful for arbitary waveforms.
          @memo Add waveform points.
          @param Points list of waveform points
          @return true if successful
       ******************************************************************/
      virtual bool add (const pointlist& Points);
   
      /** Returns the dwell time of the excitation.
          @memo dwell time function.
          @return dwell time
       ******************************************************************/
      virtual double dwellTime () const;
   
      /** Switches the excitation signal on. If the excitation signal
          can be synchronized with GPS an optional start time (in GPS 
          nsec) can be specified; otherwise this argument is ignored.
          @memo Start excitation signals.
          @param start start time of waveform (in GPS nsec)
          @param timeout max. time for start to succeed (<0 wait forever)
          @return true if successful
       ******************************************************************/
      virtual bool start (tainsec_t starttime = -1, 
                        tainsec_t timeout = -1);
   
      /** Switches the excitation signal off.
          @memo Stop excitation signals.
          @param timeout max. time for stop to succeed (<0 wait forever)
          @param ramptime time to ramp down the signal
          @return true if successful
       ******************************************************************/
      virtual bool stop (tainsec_t timeout = -1, tainsec_t ramptime = 0);
   
      /** Freezes the excitation signal.
          @memo Freeze excitation signals.
          @return true if successful
       ******************************************************************/
      virtual bool freeze ();
   
      /** Reset an excitation channel. A hard reset will switch off
          the excitation signal and will disconnect the channel from the
          excitation engine/signal generator. A soft reset switches
          the excitation signal off (and resets the signal list).
          @memo Reset the excitation signal.
          @param hard hard (true) or soft reset
          @param timeout max. time for reset to succeed (<0 wait forever)
          @return true if successful
       ******************************************************************/
      virtual bool reset (bool hard = false,
                        tainsec_t timeout = -1);
   
   protected:
      /// mutex to protect excitation object
      mutable thread::recursivemutex	mux;
      /// true if channel can be set
      bool		writeaccess;
      /// channel info structure
      gdsChnInfo_t	chninfo;
      /// settlign time
      double 		wait;
      /// signal list
      signallist	signals;
      /// list of waveform points
      pointlist		points;
      /// awg slot (ownership is transfered on copy)
      mutable int	slot;
      /// counts how many times channel is in use  (ownership is transfered on copy)
      mutable int	inUse;
      /// true if channel is a test point
      bool		isTP;
      /// value of epics channel before excitation was switched on
      double 		epicsvalue;
   };


/** Excitation object to manage a list of excitation channels.
    This object is multi-thread safe.

    Usage: In general, it is responsibility of a diagnostics test 
    or a diagnostics test iterator to control the excitation signals.
    The supervisory control should just ensure at the end of the test
    that all excitations are off.

    @memo Class to manage a list of excitation channels. 
    @author DS, November 98
    @see Excitation management API
 ************************************************************************/
   class excitationManager : public channelHandler {
   public:
      /// type representing a list of excitation channels
      typedef std::vector <excitation> excitationlist;
   
      /// list of excitation channels
      excitationlist		excitations;
   
      /** Constructs an excitation manager object.
          @memo Default constructor.
       ******************************************************************/
      excitationManager ();
      /** Destructs an excitation manager object.
          @memo Destructor.
       ******************************************************************/
      virtual ~excitationManager () {
      }
   
      /** Initializes an excitation object.
          @memo Initialization function.
          @param Silent if true prohibits applying excitations
          @param rdown ramp down time
          @return true if successful
       ******************************************************************/
      virtual bool init (testpointMgr& TPMgr, bool Silent,
                        tainsec_t rdown = 0);
   
      /** Starts the excitation signals. An optional start time can be 
          specified; if zero the current time will be used; if negative
          the start time argument is ignored. The timeout specifies how 
          long to wait.
          @memo Start function.
          @param start start time (0 now)
          @param timeout time out (<0 wait forever)
          @return true if successful
       ******************************************************************/
      virtual bool start (tainsec_t start = 0, 
                        tainsec_t timeout = -1);
   
      /** Stops the excitation signals.The timeout specifies how long to 
          wait.
          @memo Stop function.
          @param timeout time out (<0 wait forever)
          @param ramptime time to ramp down the signal
          @return true if successful
       ******************************************************************/
      virtual bool stop (tainsec_t timeout = -1, tainsec_t ramptime = 0);
   
      /** Freezes the excitation signal.
          @memo Freeze excitation signals.
          @return true if successful
       ******************************************************************/
      virtual bool freeze ();
   
      /** Adds an excitation object. Returns false, if the channel is 
          invalid, or the waveform description can not be recognized.
          @memo Add excitation function.
          @param channel channel name
          @return true if successful
       ******************************************************************/
      virtual bool add (const string& channel);
   
      /** Adds an excitation object. Returns false, if the channel is 
          invalid, or the waveform description can not be recognized.
          @memo Add excitation function.
          @param channel channel name
   	  @param waveform description of waveform
   	  @param settlingtime settling time
          @return true if successful
       ******************************************************************/
      virtual bool add (const string& channel,
                       const string& waveform,
                       double settlingtime = 0);
   
      /** Adds excitation waveforms. Returns false, if the channel is 
          invalid, or the waveform description can not be recognized.
          @memo Add excitation function.
          @param channel channel name
   	  @param awglist list of awg waveforms
          @return true if successful
       ******************************************************************/
      virtual bool add (const string& channel,
                       const std::vector<AWG_Component>& awglist);
   
      /** Adds a filter to an excitation.
          @memo Add excitation filter function.
          @param channel channel name
   	  @param filtercmd Filter command string
          @return true if successful
       ******************************************************************/
      virtual bool addFilter (const string& channel,
                        const std::string& filtercmd);
   
      /** Removes an excitations from the excitation manager.
          @memo Clear function.
   	  @param channel channel name
          @return true if successful
       ******************************************************************/
      virtual bool del (const string& channel);
   
      /** Removes all excitations from the excitation manager.
          @memo Clear function.
   	  @param timeout timeout
          @return true if successful
       ******************************************************************/
      virtual bool del (tainsec_t timeout = -1);
   
      /** Returns the longest dwell time of all excitations.
          @memo dwell time function.
          @return dwell time
       ******************************************************************/
      virtual double dwellTime () const;
      /** Sets the ramp down time.
          @memo Set ramp down time.
          @param ramp down time
       ******************************************************************/
      virtual void setRampDown (tainsec_t rdown) {
         rampdown = rdown; }
      /** Returns the ramp down time.
          @memo Get ramp down time.
          @return ramp down time
       ******************************************************************/
      virtual tainsec_t rampDown () const {
         return rampdown; }
   
   
   protected:
      /// mutex to protect excitation manager
      mutable thread::recursivemutex	mux;
      /// Pointer to test point management object
      testpointMgr*		tpMgr;
      /// if true excitations are not applied
      bool			silent;
      /// global ramp down time
      tainsec_t			rampdown;
   
   };

//@}
}

#endif /* _GDS_EXCITATION_H */
