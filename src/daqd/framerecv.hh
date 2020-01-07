#ifndef _GDS_FRAMERECV_H
#define _GDS_FRAMERECV_H
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
#include <string>
#include "framexmittypes.hh"

namespace diag
{

    /** Class for receiving frame broadcast data.
        This class implements the broadcast receiver. A code example
        can be found in 'rcvtest.cc'.

        @memo Class for receiving frame broadcast data
        @author Written August 1999 by Daniel Sigg
        @version 2.0
     ************************************************************************/
    class frameRecv
    {
    public:
        /** Constructs a broadcast receiver.
            An optional parameter specifies the quality of service:
            0 - up to 30% retransmission; 1 - 10% retransmission;
            2 - 3% retransmission. The QoS limit is determined by
            averaging over the retransmission of the last few data
            buffers. This allows for higher burst retransmission, but
            enforces a limit of the averaged retransmission.

            @param qos quality of service
            @memo Default constructor.
            @return void
         ******************************************************************/
        explicit frameRecv( int QoS = 1 )
            : sock( -1 ), qos( QoS ), logison( true ), maxlog( 25 )
        {
        }

        /** Destructs the broadcast receiver.
            @memo Destructor.
            @return void
         ******************************************************************/
        ~frameRecv( )
        {
            close( );
        }

        /** Opens the conenction to the transmitter. If no multicast
            address is supplied, the function will listen to UDP/IP
            broadcast transmission at the specified port. Otherwise, it
            tries to join the specified multicast group at the given
            interface. If the interface is obmitted, the default interface
            will be used. In general, one can use the subnet address as the
            interface address argument. The function will then go through
            the list of all local interfaces and determine the closest
            match.

            @memo Open function.
            @param mcast_addr multicast address
            @param interface interface or subnet used by multicast
            @param port port number to listen
            @return true if successful
         ******************************************************************/
        bool open( const char* mcast_addr,
                   const char* interface = 0,
                   int         port = frameXmitPort );

        /** Opens the conenction to the transmitter. Uses UDP/IP broadcast.
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

        /** Purges the receiver. This function will empty the
            receiver socket buffer and put the data into an internal
            buffer. Returns true if there is data in the internal
            buffer. This function only checks if at least one packet
            was received. There is no guarantee that a full data buffer
            was received.
            @memo Purge function.
            @return true if data pending
         ******************************************************************/
        bool purge( );

        /** Gets a data buffer. If the supplied data buffer is 0,
            the function allocates a new memory buffer upon return.
            Upon success the function returns the length of the received
            data. A value between -10 and -1 indicates a connection
            or memory allocation error, whereas a value below <10
            indicates that the preallocated memory buffer is too small.
            In this case the negative of the needed length is returned.

            @memo receive function.
            @param data data array
            @param maxlen maximum length of data array (in bytes)
            @param sequence sender sequence number (return)
            @param timestamp time stamp of data array (return)
            @param duration time length of data array (return)
            @return length of received data, if successful; <0 if failed
         ******************************************************************/
        int receive( char*&        data,
                     int           maxlen,
                     unsigned int* sequence = 0,
                     unsigned int* timestamp = 0,
                     unsigned int* duration = 0 );

        /** Last few log messages.
            @memo log message function.
            @return log messages
         ******************************************************************/
        const char* logmsg( );

        /** Clear old log messages.
            @memo clear log function.
            @return
         ******************************************************************/

        void clearlog( );

        /** Turn logging on/off.
            @memo turn log on function.
            @param set true = on
         ******************************************************************/
        void
        logging( bool set = true )
        {
            logison = set;
        }

        /** Set maximum number of log messages.
            @memo Maximum log messages function.
            @param max Maximum messages
         ******************************************************************/
        void
        setmaxlog( int max = 25 )
        {
            maxlog = max;
        }

    private:
        /// socket
        int sock;
        /// multicast receiver?
        bool multicast;
        /// multicast parameters
        ip_mreq group;
        /// old seqeunce value
        unsigned int seq;
        /// quality of service:
        int qos;
        /// retransmission rate
        double retransmissionRate;
        /// first packet?
        bool first;
        /// old sequence number
        unsigned int oldseq;
        /// send back address
        struct sockaddr_in name;
        int                port;
        /// packet list type
        typedef std::deque< auto_pkt_ptr > packetlist;
        /// packet buffers
        packetlist pkts;
        /// Do logging?
        bool logison;
        /// Maximum logging?
        int maxlog;
        /// Last few log messages
        std::deque< std::string > logs;
        /// Output log buffer
        std::string logbuf;

        /// receive a data packet
        bool getPacket( bool block = true );
        /// send a retransmit packet
        bool putPacket( retransmitpacket& pkt );
        /// Compare sequence numbers
        static long long calcDiff( const packetlist& pkts,
                                   unsigned int      newseq );

        /// disable copy constructor
        frameRecv( const frameRecv& );
        /// disable assignment operator
        frameRecv& operator=( const frameRecv& );

        /// Add a log message
        void addLog( const std::string& s );
        /// Add a log message
        void addLog( const char* s );
        /// Copy the log messages into the output buffer
        const char* copyLog( );
    };

} // namespace diag

#endif // _GDS_FRAMERECV_H
