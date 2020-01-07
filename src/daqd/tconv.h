/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: tconv							*/
/*                                                         		*/
/* Module Description: Time Conversion API	 			*/
/*									*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.5	 10Apr98  D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: tconv.html						*/
/*	References: none						*/
/*                                                         		*/
/* Author Information:							*/
/* Name          Telephone       Fax             e-mail 		*/
/* Daniel Sigg   (509) 372-8336  (509) 372-2178  sigg_d@ligo.mit.edu	*/
/*                                                         		*/
/* Code Compilation and Runtime Specifications:				*/
/*	Code Compiled on: Ultra-Enterprise, Solaris 5.5.1		*/
/*	Compiler Used: sun workshop C 4.2				*/
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

#ifndef _UTC_TAI_H
#define _UTC_TAI_H

#ifdef __cplusplus
extern "C" {
#endif

/* Header File List: */
#ifdef OS_VXWORKS
#include <vxWorks.h>
#endif

#include <time.h>

/** @name Time Conversion API
    * The time conversion API provides functions to convert between
    TAI (international atomic time) and UTC (coordinated universal time).
    It uses an internal table to account for leap seconds. When additional
    leap seconds are announced, the tabel has to be update and the
    module recompiled.
    This module also provides utility routines to break down a TAI
    format, to convert to and from a network portable representation
    and to access the leap second information.

    Knowm limitations: The conversion only works for dates after
    Jan. 1, 1972.

    @memo Converts between UTC and TAI
    @author Written April 1998 by Daniel Sigg
    @version 0.5
************************************************************************/

/*@{*/

/** @name Constants and flags.
    * Constants and flags of the time conversion API.

    @memo Constants and flags
    @author DS, April 98
    @see Time Conversion
************************************************************************/

/*@{*/

#if 0

/** Compiler flag specifying to use a POSIX timer. If this flag is 
    defined during comilation of a VxWorks object, the timer module will
    use a POSIX timer derived from the on-board real-time clock to 
    generate the heartbear (rather than an external GPS VME board).

    @author DS, April 98
    @see Time Conversion
************************************************************************/
#define _USE_POSIX_TIMER
#endif

/** Defines the TAI offset relative to GPS time. GPS time is expressed in
    weeks and seconds starting from zero at Sun, Jan. 6, 1980, 00:00 UTC.

    @author DS, April 98
    @see Time Conversion
************************************************************************/
#define TAIatGPSzero 694656019UL

/** One second expressed in ns.

    @author DS, April 98
    @see Time Conversion
************************************************************************/
#define _ONESEC 1000000000LL

/** One hour expressed in ns.

    @author DS, April 98
    @see Time Conversion
************************************************************************/
#define _ONEHOUR ( 3600 * _ONESEC )

/** One day expressed in ns.

    @author DS, April 98
    @see Time Conversion
************************************************************************/
#define _ONEDAY ( 24 * _ONEHOUR )

/** Number of epochs within a second. Should reflect the system
    heartbeat, e.g. 16 for LIGO.

    @author DS, April 98
    @see Heartbeat API
************************************************************************/
#define NUMBER_OF_EPOCHS 16

/** One epoch expressed in ns.

    @author DS, April 98
    @see Heartbeat API
************************************************************************/
#define _EPOCH ( _ONESEC / NUMBER_OF_EPOCHS )

/*@}*/

/** @name Data types.
    * Data types of the time conversion API.

    @memo Data types
    @author DS, April 98
    @see Time Conversion
************************************************************************/

/*@{*/

#ifndef _TAINSEC_T
#define _TAINSEC_T
/** Denotes a type representing TAI in nsec. This is an unsigned integer
    of 64 bits.

    @author DS, April 98
    @see Time Conversion
************************************************************************/
typedef long long tainsec_t;

/** Denotes a type representing TAI in sec. This is an unsigned integer
    of 32 bits.

    @author DS, April 98
    @see Time Conversion
************************************************************************/
typedef unsigned long taisec_t;
#endif

/** Denotes a type representing the nsec part of TAI. This is an integer
    of 32 bits.

    @author DS, April 98
    @see Time Conversion
************************************************************************/
typedef long nsec_t;

/** Denotes a struct representing TAI broken down in sec and nsec.

    @author DS, April 98
    @see Time Conversion
************************************************************************/
struct tai_struct
{
    /** sec part of TAI. */
    taisec_t tai;
    /** nsec part of TAI. */
    nsec_t nsec;
};

/** Denotes a type representing TAI broken down in sec and nsec.

    @author DS, April 98
    @see Time Conversion
************************************************************************/
typedef struct tai_struct tai_t;

/** Denotes a type representing UTC. It is identical to the struct tm
    defined in <time.h>.

    @author DS, April 98
    @see Time Conversion
************************************************************************/
typedef struct tm utc_t;

/** Denotes a struct representing UTC leap seconds.

    @author DS, April 98
    @see Time Conversion
************************************************************************/
struct leap_struct
{
    /** TAI when the leap seconds takes effect. */
    taisec_t transition;
    /** Seconds of correction to apply. The correction is given as
     the new total difference between TAI and UTC. */
    int change;
};

/** Denotes a type representing UTC leap seconds.

    @author DS, April 98
    @see Time Conversion
************************************************************************/
typedef struct leap_struct leap_t;

/*@}*/

/** @name Functions.
    * Functions of the time conversion API.

    @memo Functions
    @author DS, April 98
    @see Time Conversion
************************************************************************/

/*@{*/

/** Converts TAI (international atomic time) to UTC (coordinated universal
    time). TAI time is defined as Jan. 1, 1958, 00:00. UTC is defined
    to coincide with TAI on Jan. 1, 1972, 00:00. To keep the earth
    period synchronized with UTC, leap seconds are added periodically.
    This function corrects for leap seconds as long as the internal table
    is up to date. Known limitations: only works for dates after Jan. 1,
    1972.

    @param t atomic time in sec
    @param utc_ptr pointer to a time/data structure which will return UTC
    @return pointer to UTC structure, NULL if failed
    @author DS, April 98
    @see Time Conversion
************************************************************************/
utc_t* TAItoUTC( taisec_t t, utc_t* utc_ptr );

/** Converts TAI (international atomic time) to UTC (coordinated universal
    time). TAI zero is defined as Jan. 1, 1958, 00:00. UTC is defined
    to coincide with TAI on Jan. 1, 1972, 00:00. To keep the earth
    period synchronized with UTC, leap seconds are added periodically.
    This function corrects for leap seconds as long as the internal table
    is up to date. Known limitations: only works for dates after Jan. 1,
    1972.

    @param t atomic time in nsec
    @param utc_ptr pointer to a time/data structure which will return UTC
    @return pointer to UTC structure, NULL if failed
    @author DS, April 98
    @see Time Conversion
************************************************************************/
utc_t* TAIntoUTC( tainsec_t t, utc_t* utc_ptr );

/** Converts UTC (coordinated universal time) to TAI (international atomic
    time). TAI zero is defined as Jan. 1, 1958, 00:00. UTC is defined
    to coincide with TAI on Jan. 1, 1972, 00:00. To keep the earth
    period synchronized with UTC, leap seconds are added periodically.
    This function corrects for leap seconds as long as the internal table
    is up to date. Known limitations: only works for dates after Jan. 1,
    1972.

    @param utc_ptr pointer to a time/data structure containing UTC
    @return atomic time in sec, 0 if failed
    @author DS, April 98
    @see Time Conversion
************************************************************************/
taisec_t UTCtoTAI( const utc_t* utc_ptr );

/** Converts UTC (coordinated universal time) to TAI (international atomic
    time). TAI zero is defined as Jan. 1, 1958, 00:00. UTC is defined
    to coincide with TAI on Jan. 1, 1972, 00:00. To keep the earth
    period synchronized with UTC, leap seconds are added periodically.
    This function corrects for leap seconds as long as the internal table
    is up to date. Known limitations: only works for dates after Jan. 1,
    1972.

    @param utc_ptr pointer to a time/data structure containing UTC
    @return atomic time in nsec, 0 if failed
    @author DS, April 98
    @see Time Conversion
************************************************************************/
tainsec_t UTCtoTAIn( const utc_t* utc_ptr );

/** Returns the current atomci time in nsec. Implementation dependent;
    uses <time.h> on some systems.

    @return current atomic time in nsec
    @author DS, April 98
    @see Time Conversion
************************************************************************/
tainsec_t TAInow( void );

/** Converts TAI from a broken down format into one with nsec units.

    @param t broken down TAI
    @return atomic time in nsec
    @author DS, April 98
    @see Time Conversion
************************************************************************/
tainsec_t TAInsec( const tai_t* t );

/** Converts TAI from a format with nsec units into one with sec units.
    Also calculates the broken down format if a pointer to a data
    structure is provided.

    @param t atomic time in nsec
    @param tai pointer to a broken down TAI data structure
    @return atomic time in sec
    @author DS, April 98
    @see Time Conversion
************************************************************************/
taisec_t TAIsec( tainsec_t t, tai_t* tai );

/** Converts a TAI variable in nsec units into a network portable form.
    Uses TAIsec to break down TAI into two 32 bit numbers which are then
    put into network byte order (seconds are first). The provided buffer
    must be at least 8 bytes in size.

    @param t atomic time in nsec
    @param buf pointer to a buffer used for stroring the network
               portable TAI
    @return pointer to the buffer
    @author DS, April 98
    @see Time Conversion
************************************************************************/
char* htonTAI( tainsec_t t, char* buf );

/** Converts a TAI variable from network portable form into one with nsec
    units. First, reads 2 4 byte integers, convertes them into host format
    and then uses TAInsec to obtain TAI in nsec. The provided buffer
    must be at least 8 bytes in size.

    @param buf pointer to a buffer containing TAI in network portabel form
    @return atomic time in nsec
    @author DS, April 98
    @see Time Conversion
************************************************************************/
tainsec_t ntohTAI( const char* buf );

/** Returns the next leap second from the internal table. Neglects any
    leap seconds prior to the provided TAI. The information is provided
    as the second (TAI) when the leap occures (added leap second) or the
    second prior to the leap (subtracted leap second). The second returned
    value describes the new total difference between TAI and UTC.

    @param t atomic time in sec
    @param  nextleap pointer to data structure used for storing the next
                     leap second information
    @return pointer the next leap second information, NULL if no more
            leap seconds.
    @author DS, April 98
    @see Time Conversion
************************************************************************/
leap_t* getNextLeap( taisec_t t, leap_t* nextleap );

/*@}*/

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /*_UTC_TAI_H */
