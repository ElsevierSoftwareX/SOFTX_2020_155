#ifndef _GDS_FRAMESEND_H
#define _GDS_FRAMESEND_H
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: framexmit						*/
/*                                                         		*/
/* Module Description: API for broadcasting frames			*/
/*		       implements a reliable UDP/IP broadcast for	*/
/*                     large data sets over high speed links		*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 1.0	 10Aug99  D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: doc/index.html (use doc++)				*/
/*	References: none						*/
/*                                                         		*/
/* Author Information:							*/
/* Name          Telephone       Fax             e-mail 		*/
/* Daniel Sigg   (509) 372-8132  (509) 372-8137  sigg_d@ligo.mit.edu	*/
/*                                                         		*/
/* Code Compilation and Runtime Specifications:				*/
/*	Code Compiled on: Ultra-Enterprise, Solaris 5.7			*/
/*	Compiler Used: egcs-1.1.2					*/
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
/* LIGO Project MS NW17-161				   		*/
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

// include files
#include <netinet/in.h>
#include <deque>
#include "gdstask.h"
#include "gdsmutex.hh"
#include "framexmittypes.hh"

namespace diag
{

    /** Class for broadcasting frame data.
        This class implements the broadcast transmitter. A code example
        can be found in 'sndtest.cc'.

        @memo Class for broadcasting frame data
        @author Written August 1999 by Daniel Sigg
        @version 2.0
     ************************************************************************/
    class frameSend
    {
        /// task which runs the transmit daemon is a friend
        friend void xmitDaemonCallback( frameSend& );

    public:
        /** Constructs a default broadcast transmitter.
            @param maxBuffers maximum number of used buffers
            @memo Default constructor.
            @return void
         ******************************************************************/
        explicit frameSend( int maxBuffers = sndDefaultBuffers )
            : sock( -1 ), seq( 0 ), maxbuffers( maxBuffers )
        {
        }

        /** Constructs a broadcast/multicast transmitter and connects it.
            @param addr broadcast address/multicast group
            @param interface interface or subnet used by multicast
            @param port port number
            @param maxBuffers maximum number of used buffers
            @memo Constructor.
            @return void
         ******************************************************************/
        explicit frameSend( const char* addr,
                            const char* interface = 0,
                            int         port = frameXmitPort,
                            int         maxBuffers = sndDefaultBuffers )
            : sock( -1 ), seq( 0 ), maxbuffers( maxBuffers )
        {
            open( addr, interface, port );
        }

        /** Denstructs the broadcast transmitter.
            @memo Destructor.
            @return void
         ******************************************************************/
        ~frameSend( )
        {
            close( );
        }

        /** Opens the conenction. If the specified addr is a multicast
            address, the transmitter will use UDP/IP multicast rather
            than UDP/IP broadcast. If multicast is used, an additional
            parameter specifes the interface which will be used.
            If the interface is obmitted, the default interface will be
            used. In general, one can use the subnet address as the
            interface address argument. The function will then go through
            the list of all local interfaces and determine the closest
            match.
            @memo Open function.
            @param addr broadcast address
            @param interface interface or subnet used by multicast
            @param port port number
            @return true if successful
         ******************************************************************/
        bool open( const char* addr,
                   const char* interface = 0,
                   int         port = frameXmitPort );

        /** Opens the conenction. Uses UDP/IP broadcast.
            @memo Open function.
            @param port port number
            @return true if successful
         ******************************************************************/
        bool
        open( int port = frameXmitPort )
        {
            return open( 0, 0, port );
        }

        /** Closes the conenction.
            @memo Close function.
            @return void
         ******************************************************************/
        void close( );

        /** Broadcast a data buffer. This function will not block and
            return immediately. To assure that the data array stays
            valid until after it is transmitted, the caller can either
            request that the data array is copied, or supply a pointer
            to a boolean 'inUse' variable which will be set false by
            the transmitter after the buffer has been sent. The inUse
            variable should never be accessed directly, but rather through
            'isUsed' function only.

            The send fucntion works as follows:

            1. The send function will NOT transmit the buffer by itself
            but rather put the buffer into a queue which is then managed
            by the xmitdaemon.

            2. The send function will always return immediately and will
            NOT block until the buffer is sent.

            3. If the buffer queue is full, the send function will remove
            all unsent buffers from it.

            4. After a buffer is sent, it will stay in the queue for an
            additional time period (~3 sec) to allow retransmit. Then it
            will automatically be released.

            5. There are basically two ways to deal with the problem of
            how long data in a buffer must be kept:

            a. The send function copies the buffer (avoids buffer validity
            problems all together), or

            b. The send function does not copy the buffer and the caller
            must keep the buffer valid until the xmitdaemon is done with it.

            In the second case the caller MUST NOT delete or change the
            data in the buffer until it is no longer needed. To verify if
            the buffer is still in use, the caller can check the 'inUse'
            variable with th ehelp of the isUsed method.

            @memo Send function.
            @param data data array
            @param len length of data array (in bytes)
            @param inUse pointer to in use variable (ignored if 0)
            @param copy request a copy of the data to be used
            @param timestamp time stamp of data array
            @param duration time length of data array
            @return true if successful
         ******************************************************************/
        bool send( char*        data,
                   int          len,
                   bool*        inUse = 0,
                   bool         copy = false,
                   unsigned int timestamp = 0,
                   unsigned int duration = 0 );

        /** Returns the total number of skipped output buffers.
            @memo Skip function.
            @return skipped buffers
         ******************************************************************/
        int
        skipped( ) const
        {
            return skippedDataBuffers;
        }

        /** Returns true if variable is in use (MT safe).
            @memo isUsed function.
            @param inUse in use variable to be read
            @return in use value
         ******************************************************************/
        bool
        isUsed( bool& inUse ) const
        {
            extern mutex cmnInUseMux;
            cmnInUseMux.lock( );
            bool u = inUse;
            cmnInUseMux.unlock( );
            return u;
        }

    private:
        /// socket
        int sock;
        /// multicast transmitter?
        bool multicast;
        /// outgoing address
        struct sockaddr_in name;
        /// buffer sequence number
        unsigned int seq;
        /// maximum number of buffers
        int maxbuffers;
        /// number of skipped data buffers
        int skippedDataBuffers;
        /// in use mutex (class variable)
        // mutex		inUseMux;

        /** Buffer class.
            @memo Data buffer.
         ******************************************************************/
        class buffer
        {
        public:
            /// construct default buffer
            buffer( );
            /// copy constructor
            buffer( const buffer& buf );
            /// constructs a data buffer
            buffer( char*        Data,
                    int          Len,
                    unsigned int Seq,
                    bool         Own = false,
                    bool*        Used = 0,
                    mutex*       InUseMux = 0,
                    unsigned int Timestamp = 0,
                    unsigned int Duration = 0 );
            /// destructs the buffer
            ~buffer( );
            /// asignment operator
            buffer& operator=( const buffer& buf );

            /// buffer sequence number
            unsigned int seq;
            /// buffer owns the data
            mutable bool own;
            /// data pointer
            char* data;
            /// length of data array
            int len;
            /// pointer to in use variable
            mutable bool* used;
            /// in use mutex
            mutex* inUseMux;
            /// time stamp
            unsigned int timestamp;
            /// duration
            unsigned int duration;
            /// transferred so far
            int sofar;
        };
        /// Compare sequence
        static bool compSeqeuence( const buffer&, const retransmitpacket& );

        /// list of buffers
        std::deque< buffer > buffers;
        /// currently transmitted buffer
        int curbuf;
        /// mutex to protect buffers
        diag::mutex mux;
        /// send a buffer
        bool send( buffer& data );
        /// frined: compare a buffer to a sequence # of a retransmit packet
        friend bool compSeqeuence( const buffer& b, const retransmitpacket& p );
        /// task ID of transmit daemon
        taskID_t daemon;

        /// send packets away
        bool putPackets( packet pkts[], int n );
        /// receive waiting retransmit packets
        bool getRetransmitPacket( retransmitpacket& pkt );
        /// transmit daemon
        void xmitDaemon( );

        /// disable copy constructor
        frameSend( const frameSend& );
        /// disable assignement operator
        frameSend& operator=( const frameSend& );
    };

} // namespace diag

#endif // _GDS_FRAMESEND_H
