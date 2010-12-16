/* Version: $Id$ */
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: channelinput.h						*/
/*                                                         		*/
/* Module Description: Manages input into a data channel		*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 21Nov98  D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: channelinput.html					*/
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

#ifndef _GDS_CHANNELINPUT_H
#define _GDS_CHANNELINPUT_H

/* Header File List: */
#include <vector>
#include "dtt/gdsstring.h"
#include "dtt/gdsdatum.hh"
#include "gmutex.hh"
#include "tconv.h"

namespace diag {


/** @name Channel input API
    A channel input supports data which is down-converted and 
    decimated if necessary, partitioned and stored in the diagnostics 
    storage object.
   
    @memo Channel Input API.
    @author Written November 1998 by Daniel Sigg
    @version 0.1
************************************************************************/

//@{


/** Class implementing a callback method for the data 
    distribution system.
    @memo Class for channel callback.
    @author DS, November 98
************************************************************************/
   class chnCallback {
   public:
      /** Constructs an channel callback object.
          @memo Constructor.
          @param chnname channel name
          @return void
       ******************************************************************/
      explicit chnCallback (const string& Chnname);
   
      /** Destructs the channel callback object.
          @memo Destructor.
          @return void
       ******************************************************************/
      virtual ~chnCallback ();
   
      /** Constructs an channel callback object.
          @memo Copy constructor.
          @param chncb channel callback object
          @return void
       ******************************************************************/
      chnCallback (const chnCallback& chncb);
   
      /** Constructs an channel callback object.
          @memo Copy constructor.
          @param chncb channel callback object
          @return void
       ******************************************************************/
      chnCallback& operator= (const chnCallback& chncb);
   
      /** Compares two channel object by their name.
          @memo Equality operator.
          @return true if equal
       ******************************************************************/
      bool operator== (const chnCallback& chnchn) const {
         return gds_strcasecmp (chnname.c_str(), 
                              chnchn.chnname.c_str()) == 0;
      }
   
      /** Compares two channel object by their name.
          @memo Equality operator.
          @return true if equal
       ******************************************************************/
      bool operator== (const string& name) const {
         return gds_strcasecmp (chnname.c_str(), 
                              name.c_str()) == 0;
      }
   
      /** Compares two channel object by their name.
          @memo Inequality operator.
          @return true if not equal
       ******************************************************************/
      bool operator!= (const chnCallback& chnchn) const {
         return gds_strcasecmp (chnname.c_str(), 
                              chnchn.chnname.c_str()) != 0;
      }
   
      /** Compares two channel object by their name.
          @memo Inequality operator.
          @return true if not equal
       ******************************************************************/
      bool operator!= (const string& name) const {
         return gds_strcasecmp (chnname.c_str(), 
                              name.c_str()) != 0;
      }
   
      /** Compares two channel object by their name.
          @memo Equality operator.
          @return true if equal
       ******************************************************************/
      bool operator< (const chnCallback& chnchn) const {
         return gds_strcasecmp (chnname.c_str(), 
                              chnchn.chnname.c_str()) < 0;
      }
   
      /** Subsrcibe to real-time data distribution.
          @memo Subscribe method.
          @param start time when channels are needed
          @param active time when channels become available
          @return true if successful
       ******************************************************************/
      bool subscribe (tainsec_t start = 0, tainsec_t* active = 0);
   
      /** Subsrcibe to real-time data distribution.
          @memo Subscribe method.
          @param start time when channels are needed
          @param active time when channels become available
          @return true if successful
       ******************************************************************/
      bool unsubscribe ();
   
      /** Checks if channel callback was successfully setup.
          @memo isSet operator.
          @return true if channel subscription failed
       ******************************************************************/
      bool isSet () const {
         return (idnum >= 0);
      }
   
      /** Callback method for channel data. This method is called 
          by the RTDD every time new channel data is avaialble. This
          is a pure virtual function and has to be overwritten by a
          descendent class. This function should return 0, in order 
          to continue to receive data.
          @memo Callback method.
          @param time time of first data point (sec)
          @param epoch epoch of first data point
          @param data array of data points
          @param ndata number of data points
          @param err error code indicating invalid or missing data
          @return 0 to continue
       ******************************************************************/
      virtual int callback (taisec_t time, int epoch,
                        float data[], int ndata, int err) = 0;
   
      /** Get the channel name
          @memo Get channel name.
          @return channel name
       ******************************************************************/
      virtual const char* getChnName () const {
         return chnname.c_str(); }
      /** Set the channel name
          @memo Set channel name.
          @param name channel name
       ******************************************************************/
      virtual void setChnName (const char* name) {
         chnname = name ? name : ""; }
      /** Set the channel name
          @memo Set channel name.
          @param name channel name
       ******************************************************************/
      virtual void setChnName (const std::string& name) {
         chnname = name; }
   
   protected:
      /// mutex to protect object
      mutable thread::recursivemutex	mux;
      /// channel name
      string		chnname;
   
   private:
      // chn identifier
      int		idnum;
   
      static int callbackC (chnCallback* usrdata, 
                        taisec_t time, int epoch,
                        float data[], int ndata, int err)
      {
         return usrdata->callback (time, epoch, data, ndata, err);
      }
   };


/** Class for reading channel data from the a data 
    distribution system. This class will receive data from a data
    system through the callback mechanism. It implements the following
    prepropressing options: a first decimation stage, a down-conversion,
    a second decimation stage and automatic partition of data. Incoming
    data is buffered in a partition if of the right time frame. If a
    partition is complete it is automatically handed over to the
    diagnostics storage object. Partitions can have unique names,
    meaning a new data object is added for each partition, or they
    can have a name which was previously used, meaning the data is 
    added to the data object of same name.
    @memo Class for channel input from a data distribution system.
    @author DS, November 98
************************************************************************/
   class dataChannel : public chnCallback {
   public:
      class partition;
      class preprocessing;
   
      /// list of partitions
      typedef std::vector <partition> partitionlist;
      /// list of preprocessing objects
      typedef std::vector <preprocessing> preprocessinglist;
   
   
      /** This class describes a channel preprocessing stage. A
          preprocessing stage contains an optional first decimation,
   	  an optional down-conversion and and optional second decimation.
   	  @memo Data preprocessing.
       ******************************************************************/
      class preprocessing {
         /// dataChannel is a friend
         friend class dataChannel;
      public:
      
      /** Constructs a preprocessing object.
          If Decimate1 is -1, it is assumed that the incoming time
          series is already heterodyned, ie., it is complex. Neither
          the first nor the heterodyne step are the applied. Only the
          second heterodyning is computed.
          @memo Constructor.
      	  @param dataRate channel data rate
          @param Decimate1 first decimation rate (power of 2)
          @param Decimate2 second decimation rate (power of 2)
          @param Zoomstart time zero for down-conversion
          @param Zoomfreq down-conversion frequency
          @param rmvDelay remove decimation delay
          @return void
       ******************************************************************/
         explicit preprocessing (int dataRate, 
                           int Decimate1 = 1, int Decimate2 = 1, 
                           tainsec_t Zoomstart = 0, double Zoomfreq = 0,
                           bool rmvDelay = true);
      
      /** Constructs a preprocessing object.
          @memo Copy constructor.
      	  @param p preprocessing object
          @return void
       ******************************************************************/
         preprocessing (const preprocessing& p);
      
      /** Copies a preprocessing object.
          @memo Copy operator.
      	  @param p preprocessing object
          @return void
       ******************************************************************/
         preprocessing& operator= (const preprocessing& p);
      
      /** Destructs a preprocessing object.
          @memo Destructor.
          @return void
       ******************************************************************/
         ~preprocessing ();
      
      /** Compares two preprocessing object.
          @memo Equality operator.
      	  @param pre preprocessing object
          @return true if equal decimation and zoom parameters
       ******************************************************************/
         bool operator== (const preprocessing& pre) const;
      
      /** Sets the active time of a preprocessing object. When set,
          data is only sent through the decimation filters for the
          specified interval. To account for filter settling times,
          data pass through actually start earlier by 10 times the
          filter delay and stops later by twice the filter delay.
          A value of -1 indicates that the corresponding limit
          should be ignored, i.e. data will be processed irregardly.
          If this function is called multiple times, the active time
          will be the minimum/maximum of all supplied values. Setting
          newValues to true will override this behaviour and take them
          as is. Only when obey is set true, the active time will 
          actually be used to determine if preprocessing should take
          place.
      
          @memo set active time method.
      	  @param Start time when preprocessing must deliver first data
      	  @param Stop time when preprocessing can stop delivering data
      	  @param obey if true active time will actually be used 
      	  @param newValues when false the min/max is calculated
          @return void
       ******************************************************************/
         void setActiveTime (tainsec_t Start, tainsec_t Stop,
                           bool obey = false, bool newValues = false);
      
      /** Data feeding function. If the number of data points fed into
          this routine is smaller than the decimation rate, data will
          be held back until enough data points have been received. Only
          when enough data points are ready will the process function be
          called. The number of data points fed into the routine has 
          to be either a multiple of the total decimation rate or an
          integer fraction thereof. If an integer fraction, subsequent 
          calls must be of equal length.
          @memo Data feeding method.
          @param time time of first data point (sec)
          @param epoch epoch of first data point
          @param data array of data points
          @param ndata number of data points
          @param err error code indicating invalid or missing data
          @param partitions list of partitions
          @param mux channel mutex
          @param update indicates if partition list needs to be updated
          @return true if successful
       ******************************************************************/
         bool operator() (taisec_t time, int epoch,
                         float data[], int ndata, int err,
                         partitionlist& partitions, 
                         thread::mutex& mux, bool& update);
      
      /** Channel preprocessing function.
          @memo Preprocessing method.
          @param time time of first data point (sec)
          @param epoch epoch of first data point
          @param data array of data points
          @param ndata number of data points
          @param err error code indicating invalid or missing data
          @param partitions list of partitions
          @param mux channel mutex
          @param update indicates if partition list needs to be updated
          @return true if successful
       ******************************************************************/
         bool process (taisec_t time, int epoch,
                      float data[], int ndata, int err,
                      partitionlist& partitions, 
                      thread::mutex& mux, bool& update);
      
      protected:
         /// channel data rate
         int		datarate;
      	 /// complex input series (pre heterodyned)
      	 bool		cmplx;
         /// first decimation rate (must be a power of 2)
         int		decimate1;
         /// second decimation rate (must be a power of 2)
         int		decimate2;
         /// decimation flag
         int		decimationflag;
         /// time zero for down-conversion (nsec)
         tainsec_t	zoomstart;
         /// frequency of down-conversion (Hz)
         double		zoomfreq;
         /** spacing of data points (Hz); original time series, and after 
             1st and 2nd decimation */
         double		dt[3];
      	 /// remove decimation delay
         bool		removeDelay;
      	 /// decimation delay (in sec)
         double		decdelay;
      	 /// number of taps in the delay filter
         int		delaytaps;
      	 /// delay shift (in nsec)
         tainsec_t	delayshift;
      	 /// accumulated delay after first decimation stage (in nsec)
         tainsec_t	delay1;
      
         /// if true do preprocessing only during active time
         bool		useActiveTime;
      	 /// preprocessing start time; -1 ignore, 0 uninitialized
         tainsec_t	start;
      	 /// preprocessing stop time; -1 ignore, 0 uninitialized
         tainsec_t	stop;
      
      private:
         /// buffer time
         tainsec_t	bufTime;
         /// size of data in buffer
         int		bufSize;
         /// t = 0 of grid
         tainsec_t 	t0Grid;
         /// buffer for holding data segments which are too small
         float*		buf;
      
         /// primitive high pass for zomm analysis
         bool		hpinit;
         float		hpval;
         /// temporary storage for delay filter
         float*		tmpdelay1;
         /// temporary storage for 1st decimation filter
         float*		tmpdec1;
         /// temporary storage for 2nd decimation filter, I phase
         float*		tmpdec2I;
         /// temporary storage for 2nd decimation filter, Q phase
         float*		tmpdec2Q;
      };
   
   
      /** This class describes a channel data partition. A partition
          describes a finite time series. It contains a start time, a 
          duration and the spacing of the data points. A partition
          can describe a single time series or the both in-phase and
          the quad-phase of a down-converted time series.
   	  @memo Data partition.
       ******************************************************************/
      class partition {
         /// dataChannel is a friend
         friend class dataChannel;
         /// rttChannel::partition is a friend, too
         friend class dataChannel::preprocessing;
      public:
      
      /** Constructs a partition.
          @memo Constructor.
      	  @param Name name of partition
      	  @param Start beginning of partition (nsec)
      	  @param Duration length of partition (nsec)
      	  @param Dt spacing of data points
      	  @param tp precursor time
          @return void
       ******************************************************************/
         partition (string Name, tainsec_t Start, tainsec_t Duration,
                   double Dt = 1.0, tainsec_t tp = 0);
      
      /** Constructs a partition.
          @memo Copy constructor.
      	  @param p partition
          @return void
       ******************************************************************/
         partition (const partition& p);
      
      /** Sets a new data point spacing. If the original data is
          complex (heterodyned), Decimate1 must be one.
          @memo data point spacing function.
          @param Dt spacing of data points
          @param Decimate1 first decimation rate (power of 2)
          @param Decimate2 second decimation rate (power of 2)
          @param Zoomstart time zero for down-conversion
          @param Zoomfreq down-conversion frequency
      	  @param rmvDelay remove decimation filter delay
          @return void
       ******************************************************************/
         void setup (double Dt, 
                    int Decimate1 = 1, int Decimate2 = 1, 
                    tainsec_t Zoomstart = 0, double Zoomfreq = 0,
                    bool rmvDelay = true);
      
      /** Copies new data points into the partition.
          @memo Fill function.
          @param data data array
          @param len number of data points
          @param bufnum buffer ID (0 = in-phase; 1 = quad-phase)
          @return void
       ******************************************************************/
         void fill (const float data[], int len, int bufnum = 0);
      
      /** Copies data points from the partition into a data array.
          @memo Copy function.
          @param data data array
          @param len maximum number of data points
          @param cmplx true if down-converted time series
          @return void
       ******************************************************************/
         void copy (float data[], int max, bool cmplx = false);
      
      /** Returns the index of data array to be copied given the
          start time of the time series and its length. Returns 
          -1 if the time series is ahead of the partion interval, and
          -2 if the time series is past the partition interval.
          @memo Index function.
          @param Start beginning of time series (nsec)
          @param  Length number of data points in time series
          @return time series index to fill partition
       ******************************************************************/
         int index (tainsec_t Start, int Length) const;
      
      /** Returns the number of points to be copied from the time 
          series into the partition.
          @memo Range function.
          @param Start beginning of time series (nsec)
          @param Length number of data points in time series
          @return void
       ******************************************************************/
         int range (tainsec_t Start, int Length) const;
      
      /** Indicates to the partition that it can not expect any more
          data. Sets the error flag if partition is not full.
          @memo Stop waiting.
          @return void
       ******************************************************************/
         void nomore ();
      
      /** Checks if partition is full, meaning all data points have 
          been filled into the internal buffer.
          @memo Is full function.
          @return true if partition is full
       ******************************************************************/
         bool isFull () const;
      
      /** Checks if partition is done, meaning it is either full or
          no more data can be expected.
          @memo Is done function.
          @return true if partition is done
       ******************************************************************/
         bool isDone () const;
      
      /** Compares two partitions using their start time.
          @memo Less than operator.
          @param p another partition
          @return true if smaller
       ******************************************************************/
         bool operator< (const partition& p) const;
      
         /// name of time series; will be used to store data
         string		name;
      
      protected:
      	 /// beginning of time series (GPS nsec)
         tainsec_t	start;
      	 /// length of time series (GPS nsec)
         tainsec_t	duration;
         /// precursor time
         tainsec_t	precursor;
      	 /// spacing of data points (sec)
         double		dt;
      	 /// number of data points
         int		length;
      	 /// data buffers
         std::vector<float>	buf[2];
         /// first decimation rate (power fo 2)
         int		decimate1;
         /// second decimation rate (power of 2)
         int		decimate2;
         /// time zero for down-conversion (nsec)
         tainsec_t	zoomstart;
         /// frequency of down-conversion (Hz)
         double		zoomfreq;
      	 /// remove decimation filter delay
         bool 		removeDelay;
      	 /// decimation delay (in sec)
         double		decdelay;
      	 /// taps in time delay filter (in number of original samples)
         int		delaytaps;
         /// remaining time delay of data
         double 	timedelay;
      
      private:
         bool		done;
      };
   
   
      /** Constructs a channel object.
          @memo Constructor.
          @param Chnname channel name
          @param dat storage object for storing partition data
          @param dataRate data rate in Hz
          @param dataType data type as defined by the nds
          @return void
       ******************************************************************/
      dataChannel (string Chnname, gdsStorage& dat, int dataRate, 
                  int dataType);
   
      /** Constructs a channel object.
          @memo Copy constructor.
      	  @param chn channel object
          @return void
       ******************************************************************/
      dataChannel (const dataChannel& chn);
   
      /** Destructs the channel object.
          @memo Destructor.
          @return void
       ******************************************************************/				
      virtual ~dataChannel ();
   
      /** Copies a channel object.
          @memo Copy operator.
      	  @param chn channel object
          @return channel object
       ******************************************************************/
      dataChannel& operator= (const dataChannel& chn);
   
      /** Adds a new preprocessing stage. Usually, this is done 
          automatically by addPartitions.
          @memo Add preprocessing stage.
          @param Decimate1 first decimation rate (power of 2)
          @param Decimate2 second decimation rate (power of 2)
          @param Zoomstart time zero for down-conversion
          @param Zoomfreq down-conversion frequency
      	  @param rmvDelay remove decimation filter delay
      	  @param useActiveTime if true active time will be used 
      	  @param Start time when preprocessing must deliver first data
      	  @param Stop time when preprocessing can stop delivering data
          @return true if successful
       ******************************************************************/
      bool addPreprocessing (int Decimate1 = 1, int Decimate2 = 1, 
                        tainsec_t Zoomstart = 0, double Zoomfreq = 0,
                        bool rmvDelay = true, bool useActiveTime = false,
                        tainsec_t Start = -1, tainsec_t Stop = -1);
   
      /** Adds another list of partitions to the channel object. This
          function also adds all necessary preprocessing objects.
          @memo Add partitions.
          @param newPartitions list of new partitions
      	  @param useActiveTime if true active time will be used 
          @return true if successful
       ******************************************************************/
      bool addPartitions (const partitionlist& newPartitions, 
                        bool useActiveTime = false);
   
      /** Resets the partition list.
          @memo Reset function.
       ******************************************************************/
      void reset ();
   
      /** Skips partitions which require data from the time before
          stop.
          @memo Skip function.
          @param stop skip partitions before this time
       ******************************************************************/
      void skip (tainsec_t stop);
   
      /** Returns the time of the last received data block (or zero
          if none was received yet). The returned time represents the
          time of the (last + 1) data sample in GPS nsec, i.e. all data
          sample with a time earlier than returned were received.
          @memo Time stamp method.
          @return time of last received data block
       ******************************************************************/
      tainsec_t timeStamp () const {
         thread::semlock	lockit (mux);
         return timestamp;
      }
   
      /** Returns the maximum time delay introduced by the preprocessing
          (mainly FIR filter delays).
          @memo Time delay method.
          @return maximum time delay through preprocessing
       ******************************************************************/
      tainsec_t maxDelay () const;
   
      /** Callback method for channel data (see description of parent).
          @memo Callback method.
          @param time time of first data point (sec)
          @param epoch epoch of first data point
          @param data array of data points
          @param ndata number of data points
          @param err error code indicating invalid or missing data
          @return 0 to continue
       ******************************************************************/
      virtual int callback (taisec_t time, int epoch,
                        float data[], int ndata, int err);
   
      /** Looks for finished partitions and stores them in the
          diagnostics storage object.
          @memo Update storage object with finished partitions.
          @param async runs asynchronous if true
          @return void
       ******************************************************************/
      virtual void updateStorage (bool async = false);
   
      /** Is a testpoint?
          @memo Is tespoint.
          @return true if TP
       ******************************************************************/
      virtual bool isTestpoint() const {
         return isTP; }
      /** Set as a testpoint?
          @memo set tespoint.
          @param tp testpoint condition
       ******************************************************************/
      virtual void setTestpoint (bool tp) {
         isTP = tp; }
      /** In use count?
          @memo In use count.
          @return in use count
       ******************************************************************/
      virtual int inUseCount() const {
         return inUse; }
      /** Increase/decrease in use count
          @memo Increase/decrease in use count.
          @param up increase (true) or decrease (false)
       ******************************************************************/
      virtual void useCount (bool up = true) {
         if (up) ++inUse; 
         else --inUse; }
      /** Set in use count.
          @memo Set in use count.
          @param n in use count
       ******************************************************************/
      virtual void inUseSet (int n) {
         inUse = n; }
   
      /** Get the data rate
          @memo Get data rate.
          @return data rate
       ******************************************************************/
      virtual int getDatarate () const {
         return datarate; }
      /** Set the data rate
          @memo Set data rate.
          @param rate data rate
       ******************************************************************/
      virtual void setDatarate (int rate) {
         datarate = rate; }
      /** Get the data type
          @memo Get data type.
          @return data type
       ******************************************************************/
      virtual int getDatatype () const {
         return datatype; }
      /** Set the data type
          @memo Set data type.
          @param rate data type
       ******************************************************************/
      virtual void SetDatatype (int typ) {
         datatype = typ; }
      /** Get the byte per second
          @memo Get byte per second.
          @return byte per second
       ******************************************************************/
      virtual int getBps () const {
         return databps; }
      /** Set the byte per second
          @memo Set byte per second.
          @param bps byte per second
       ******************************************************************/
      virtual void setBps (int bps) {
         databps = bps; }
   protected:
      /// channel data rate
      int		datarate;
      /// channel data type
      int		datatype;
      /// channel data type: bytes per sample
      int		databps;
      /// pointer to storage object
      gdsStorage*	storage;
      /// list of partitions
      partitionlist	partitions;
      /// list of preprocessing stages
      preprocessinglist	preprocessors;
      /// time stamp of most recent data
      tainsec_t		timestamp;
      /// counts how many times channel is in use
      int		inUse;
      /// true if channel is a test point
      bool		isTP;
   
   private:
      /// asynchronous task for updating storage object
      static void updateStorageTask (dataChannel* chn);
      /**  mutex for updating tasks, every tasks gets a read lock until
           it is finshed, upon destruction of a channel the destructor will
           get a write lock to make sure all update tasks have terminated.
           Never try to get this lock after mux is already locked!! */
      mutable thread::readwritelock	updatelock;
      /// true if asynchronoud update task is running
      bool		asyncUpdate;
   };

//@}
}

#endif /* _GDS_CHANNELINPUT_H */
