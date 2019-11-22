#ifndef _GDS_FRAMEXMITTYPE_H
#define _GDS_FRAMEXMITTYPE_H
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
#include <sys/types.h>
#include <netinet/in.h>
#include <inttypes.h>

namespace diag
{

    /** @name Flow control
        Constants and classes used for controling the data flow.
        @memo Flow control classes and constants
    ************************************************************************/

    /*@{*/

    /// default IP port [7096]
    const int frameXmitPort = 7096;
    /// multicast time-to-live (i.e., number of hopes)
    const unsigned char mcast_TTL = 1;
    /// priority of daemon (lower values = higher priority) [1]
    const int daemonPriority = -19;
    /// default number of buffers for the sender [7]
    const int sndDefaultBuffers = 30;
    /// packet size (in bytes) [64000]
    const int packetSize =
        64000; // Limit imposed by UDP protocol 16-bit length field
    /// continues packet burst (in # of packets) [4]
    extern int packetBurst;
    /// minimum time interval between bursts (in usec) [1000]
    const int packetBurstInterval = 1;
    /// sender delay tick if there are no packets to send (in usec) [1000]
    const int sndDelayTick = 10;
    /// receiver delay tick if there were no packets received (in usec) [1000]
    const int rcvDelayTick = 1;
    /// maximum number of packets allowed for retransmit [200]
    const int maximumRetransmit = 200;
    /// sender socket input buffer length (in bytes) [65536]
    const int sndInBuffersize = 65536;
    /// sender socket output buffer length (in bytes) [262144]
    const int sndOutBuffersize = 1 * 1024 * 1024;
    /// receiver socket input buffer length (in bytes) [262144]
    const int rcvInBuffersize = 20 * 1024 * 1024;
    /// receiver socket input buffer maximum length (in bytes) [1048576]
    const int rcvInBufferMax = 10 * 1024 * 1024;
    /// receiver socket output buffer length (in bytes) [65536]
    const int rcvOutBuffersize = 1 * 1024 * 1024;
    /// receiver packet buffer length (in number of packets) [1024]
    const int rcvpacketbuffersize = 1024 * 10;
    /// maximum number of retries to rebroadcast [5]
    const int maxRetry = 50;
    /// timeout for each retry (in usec) [250000]
    const int retryTimeout = 250000;
    /// maximum value a sequnce can be out of sync [5]
    const int maxSequenceOutOfSync = 500;
    /// number of data segments used for determining the average retransmission
    /// rate [10.1]
    const double retransmissionAverage = 10.1;
    /// artifically introduced receiver error rate [0], only valid if compiled
    /// with DEBUG
    const double rcvErrorRate = 0.1;

    /// broadcast packet type [123]
    const int PKT_BROADCAST = 123;
    /// rebroadcast packet type [124]
    const int PKT_REBROADCAST = 124;
    /// retransmit request packet [125]
    const int PKT_REQUEST_RETRANSMIT = 125;

    /// Packet header (information is stored in network byteorder)
    struct packetHeader
    {
        /// packet type
        int32_t pktType;
        /// length of payload in bytes
        int32_t pktLen;
        /// sequence number
        uint32_t seq;
        /// packet number
        int32_t pktNum;
        /// total number of packets in sequence
        int32_t pktTotal;
        /// checksum of packet (currently not used)
        uint32_t checksum;
        /// time stamp of data
        uint32_t timestamp;
        /// time length of data
        uint32_t duration;
        /// swap byte-order in header from host to network
        inline void hton( );
        /// swap byte-order in header from network to host
        inline void ntoh( );
    };

    /// Standard data packet
    struct packet
    {
        /// packet header
        packetHeader header;
        /// packet payload
        char payload[ packetSize ];
        /// swap byte-order in header from host to network
        inline void hton( );
        /// swap byte-order in header from network to host
        inline void ntoh( );
    };

    /// Retransmit request packet
    struct retransmitpacket
    {
        /// packet header
        packetHeader header;
        /// packet payload
        int32_t pktResend[ packetSize / sizeof( int32_t ) ];
        /// swap byte-order in header from host to network
        inline void hton( );
        /// swap byte-order in header from network to host
        inline void ntoh( );
    };

    /** Auto pointer for a packet.
        Packets are ordered by sequence number and packet number.
     ************************************************************************/
    class auto_pkt_ptr
    {
    private:
        packet*      ptr;
        mutable bool owns;

    public:
        explicit auto_pkt_ptr( packet* p = 0 ) : ptr( p ), owns( p )
        {
        }
        auto_pkt_ptr( const auto_pkt_ptr& a ) : ptr( a.ptr ), owns( a.owns )
        {
            a.owns = 0;
        }
        auto_pkt_ptr&
        operator=( const auto_pkt_ptr& a )
        {
            if ( &a != this )
            {
                if ( owns )
                    delete ptr;
                owns = a.owns;
                ptr = a.ptr;
                a.owns = 0;
            }
            return *this;
        }
        ~auto_pkt_ptr( )
        {
            if ( owns )
                delete ptr;
        }
        packet& operator*( ) const
        {
            return *ptr;
        }
        packet* operator->( ) const
        {
            return ptr;
        }
        packet*
        get( ) const
        {
            return ptr;
        }
        packet*
        release( ) const
        {
            owns = false;
            return ptr;
        }

        bool
        operator==( const auto_pkt_ptr& a ) const
        {
            if ( !owns || !a.owns )
            {
                return false;
            }
            return ( ( ptr->header.seq == a.ptr->header.seq ) &&
                     ( ptr->header.pktNum == a.ptr->header.pktNum ) );
        }
        bool
        operator<( const auto_pkt_ptr& a ) const
        {
            if ( !owns || !a.owns )
            {
                return owns;
            }
            return ( ( ptr->header.seq < a.ptr->header.seq ) ||
                     ( ( ptr->header.seq == a.ptr->header.seq ) &&
                       ( ptr->header.pktNum < a.ptr->header.pktNum ) ) );
        }
    };

    inline void
    packetHeader::hton( )
    {
        pktType = htonl( pktType );
        pktLen = htonl( pktLen );
        seq = htonl( seq );
        pktNum = htonl( pktNum );
        pktTotal = htonl( pktTotal );
        checksum = htonl( checksum );
        timestamp = htonl( timestamp );
        duration = htonl( duration );
    }

    inline void
    packetHeader::ntoh( )
    {
        pktType = ntohl( pktType );
        pktLen = ntohl( pktLen );
        seq = ntohl( seq );
        pktNum = ntohl( pktNum );
        pktTotal = ntohl( pktTotal );
        checksum = ntohl( checksum );
        timestamp = ntohl( timestamp );
        duration = ntohl( duration );
    }

    inline void
    packet::hton( )
    {
        // payload not swapped
        header.hton( );
    }

    inline void
    packet::ntoh( )
    {
        header.ntoh( );
        // payload not swapped
    }

    inline void
    retransmitpacket::hton( )
    {
        for ( int i = 0; i < header.pktLen / (int)sizeof( int32_t ); ++i )
            pktResend[ i ] = htonl( pktResend[ i ] );
        header.hton( );
    }

    inline void
    retransmitpacket::ntoh( )
    {
        header.ntoh( );
        for ( int i = 0; i < header.pktLen / (int)sizeof( int32_t ); ++i )
            pktResend[ i ] = ntohl( pktResend[ i ] );
    }

    /*@}*/

} // namespace diag

#endif // _GDS_FRAMEXMITTYPE_H
