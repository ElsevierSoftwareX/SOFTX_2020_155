/* Version: $Id$ */
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: testpoint						*/
/*                                                         		*/
/* Module Description: API for handling testpoints			*/
/*									*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 25June98 D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: testpoint.html					*/
/*	References: none						*/
/*                                                         		*/
/* Author Information:							*/
/* Name          Telephone       Fax             e-mail 		*/
/* Daniel Sigg   (509) 372-8336  (509) 372-2178  sigg_d@ligo.mit.edu	*/
/*                                                         		*/
/* Code Compilation and Runtime Specifications:				*/
/*	Code Compiled on: Ultra-Enterprise, Solaris 5.6			*/
/*	Compiler Used: sun workshop C 4.2				*/
/*	Runtime environment: sparc/solaris				*/
/*                                                         		*/
/* Code Standards Conformance:						*/
/*	Code Conforms to: LIGO standards.	OK			*/
/*			  Lint.			TBD			*/
/*			  ANSI			TBD			*/
/*			  POSIX			TBD			*/
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

#ifndef _GDS_TESTPOINT_H
#define _GDS_TESTPOINT_H

#ifdef __cplusplus
extern "C" {
#endif


/* Header File List: */
#ifndef _TAINSEC_T
#define _TAINSEC_T
#include <inttypes.h>
   typedef int64_t tainsec_t;
   typedef unsigned long taisec_t;
#endif


/**
   @name Test Point API
   A set of routines to handle digital test points in reflective
   memory. These routines can be called from any machine and will
   use rpc to commmunicate with the reflective memory system 
   controller when necessary. Test points are always valid for full
   one second intervals and can not be changed in between. Test 
   points are used to read data from LSC and ASC, as well as writing
   waveforms to them through the excitation engine.

   @memo API to digital test points in the reflective memory
   @author Written June 1998 by Daniel Sigg
   @version 0.1
************************************************************************/

/*@{*/

/** @name Constants and flags.
    * Constants and flags of the test point API.

    @memo Constants and flags
    @author DS, June 98
    @see Test point API
************************************************************************/

/*@{*/

#if 0

/** Compiler flag for the DAQD.

    @author DS, June 98
    @see Test point API
************************************************************************/
#define _TP_DAQD

/** Compiler flag for a enabling dynamic configuration. When enabled
    the host address and interface information of the test point 
    manager is queried from the network rather than read in through a 
    file.

    @author DS, June 98
    @see Test point API
************************************************************************/
#undef _CONFIG_DYNAMIC

/** Compiler flag for disabling test points. If defined the test point
    interface (both client and server) are disabled and all test point
    access routines will return with an error.

    @author DS, June 98
    @see Test point API
************************************************************************/
#define _NO_TESTPOINTS

/** Compiler flag for disabling keep alive messages. Keep alive 
    messages are sent to the test point manager once every 15 sec. The
    test point manager will automatiocally free all test points
    associated with the current client if no keep alive message is
    received for more than 60 sec.

    @author DS, June 98
    @see Test point API
************************************************************************/
#define _NO_KEEP_ALIVE

/** Compiler flag for test point access. This flag defines if test 
    points are read directly from reflective memory (non-zero), or 
    through an rpc call to the test point server (0). If direct
    access is specified the flag must bit encode the available 
    test point nodes, e.g., 1 is mode zero, 2 is mode one and 3 is
    both modes. If this flag is undefined at compile time it will 
    default to 1 (direct access, node zero) for interferometer 1 under 
    VxWorks, to 2 (direct access, node one) for interferometer 2 under 
    VxWorks, to 3 (direct access, both nodes) for PEM under VxWorks,
    and to 0 (server access) under Unix, respectively.

    @author DS, June 98
    @see Test point API
************************************************************************/
#define _TESTPOINT_DIRECT
#endif

/** Defines the epoch when test points are getting updated.

    @author DS, June 98
    @see Test point API
************************************************************************/
#define TESTPOINT_UPDATE_EPOCH	8

/** Defines the first epoch when testpoint information is valid.

    @author DS, June 98
    @see Test point API
************************************************************************/
#define TESTPOINT_VALID1	10

/** Defines the last epoch when testpoint information is valid..

    @author DS, June 98
    @see Test point API
************************************************************************/
#define TESTPOINT_VALID2	4

/*@}*/


/** @name Data types.
    * Data types of the test point API.

    @memo Data types
    @author DS, June 98
    @see Test point API
************************************************************************/

/*@{*/

#ifndef __TESTPOINT_T
#define __TESTPOINT_T
/** Defines the type of a test point.

    @author DS, June 98
    @see Test point API
************************************************************************/
   typedef unsigned int testpoint_t;
#endif

/*@}*/

/** @name Functions.
    * Functions of the test point API.

    @memo Functions
    @author DS, June 98
    @see Test point API
************************************************************************/

/*@{*/

/** Installs a test point client interface. This function should be
    called prior of using the test point interface. If not, the first
    call to any of the functions in this API will call it. However,
    it may take up to two seconds, before the index cache is filled
    and the interface is working properly.

    @param void
    @return 0 if successful, <0 otherwise
    @author DS, June 98
************************************************************************/
   int testpoint_client (void);

/** Terminates a test point client interface.

    @param void
    @return void
    @author DS, June 98
************************************************************************/
   void testpoint_cleanup (void);

#ifdef _TP_DAQD
/** Sets the test point manager address. This function is only 
    available when compiling with the _TP_DAQD flag. It will disable
    the default parameter file and set the host address and rpc program
    and version number directly. If a program number of zero is
    specified, the default will be used. If a program version of 
    zero is specified, the default will be used. This function must
    be called before any other function (including testpoint_client).

    @param node test point node
    @param hostname host name of test point manager
    @param prognum rpc program number
    @param progver rpc program version
    @return 0 if successful, <0 otherwise
    @author DS, June 98
************************************************************************/
   int tpSetHostAddress (int node, const char* hostname, 
                     unsigned long prognum, unsigned long progver);
#endif


/** Request test points. The requested test points are made available
    in the reflective memory if enough test point slots are free.
    Each test point has a unique ID which must be defined in the 
    channel information block of the reflective memory. An optional
    timeout can be specified to automatically clear a test point after
    a given time (-1 means forever). Test points are set in the 8th 
    epoch only. If successful, this routine returns the number of newly 
    set test points and a negative error number otherwise. Optionally, 
    the time and epoch when the test points become active can be returned.
    If more test points are requested than there are free slots, the
    routine will allocate as many test points as possible. Test points
    which are allocated with a timeout must not be cleared.

    @param node test point node
    @param tp list of test point ID numbers
    @param tplen length of test point list
    @param timeout maximum time (in nsec) the test point is needed
    @param time time (in sec) when the test point becomes active
    @param epoch epoch when the test point becomes active
    @return 0 if successful, <0 otherwise
    @author DS, June 98
************************************************************************/
   int tpRequest (int node, const testpoint_t tp[], int tplen,
   tainsec_t timeout, taisec_t* time, int* epoch);


/** Request test points by name. This routine is similar to 
    tpRequest but accepts channel names of test points. The tpNames
    argument can contain a list of space separated channel names.
    Limitation: Sends the test point names to the test point
    manager of node 0 only.

    @param node test point node
    @param tpNames space seperated list of channel names
    @param timeout maximum time (in nsec) the test point is needed
    @param time time (in sec) when the test point becomes active
    @param epoch epoch when the test point becomes active
    @return 0 if successful, <0 otherwise
    @author DS, June 98
************************************************************************/
   int tpRequestName (const char* tpNames,
   tainsec_t timeout, taisec_t* time, int* epoch);


/** Clear test points. This routine clears test points from the
    reflective memory. Test points are cleared in the 6th epoch only.
    If the pointer to the test point list is NULL, all test points
    of this node are cleared. This routine returns immediately.

    @param node test point node
    @param tp list of test point ID numbers
    @param tplen length of test point list
    @return 0 if successful, <0 otherwise
    @author DS, June 98
************************************************************************/
   int tpClear (int node, const testpoint_t tp[], int tplen);


/** Clear test points by names. This routine is similar to tpClear but
    accepts channel names of test points. The tpNames
    argument can contain a list of space separated channel names.
    Limitation: Sends the test point names to the test point
    manager of node 0 only.

    @param node test point node
    @param tpNames space seperated list of channel names
    @return 0 if successful, <0 otherwise
    @author DS, June 98
************************************************************************/
   int tpClearName (const char* tpNames);


/** Query the test point interface. This routine returns a list of
    all assigned test points in the reflective memory belonging to the
    specified node/interface combination. The position within the list
    corresponds to the slot number. A zero indicates an unused slot.
    To avoid ambiguities around the time when the test point list
    changes, the routine takes the requested time and epoch as
    additional arguments. If the time is too far in the future, the
    routine will return the most recent test point information, rather
    than wait. If the time is too far in the past, it will return an 
    error. If both time and epoch are zero, the current test point
    information is returned. However, this may already be out-dated
    when the function returns.

    If running on a front-end CPU this routine will retrieve the 
    information directly from the reflective memory (assuming
    the node is accessible and the _TESTPOINT_DIRECT flag was
    specified during compilation). The number of read test point is 
    returned if successful. If the test point list is NULL, nothing
    is copied, but the routine will still return the number of test
    points in the index. This routine will try to query a remote 
    test point server if the test points aren't accessible locally.

    @param node test point node
    @param tpinterface testpoint interface id
    @param tp list of test point ID numbers
    @param tplen length of test point list
    @param time time of query request
    @param epoch epoch of query request
    @return Number of read test points if successful, <0 otherwise
    @author DS, June 98
************************************************************************/
   int tpQuery (int node, int tpinterface, testpoint_t tp[], int tplen, 
   taisec_t time, int epoch);

/** Gets the test point index directly. This routine is similar to
    tpQuery, however, it will only try to read the test point index 
    locally; and return with an error if the index is not accessible.

    @param node test point node
    @param tpinterface testpoint interface id
    @param tp list of test point ID numbers
    @param tplen length of test point list
    @param time time of query request
    @param epoch epoch of query request
    @return Number of read test points if successful, <0 otherwise
    @author DS, June 98
************************************************************************/
   int tpGetIndexDirect (int node, int tpinterface, testpoint_t tp[], 
   int tplen, taisec_t time, int epoch);

/** Returns the address of a test point in reflective memory. If
    the test point couldn't be found or if the specified time and
    epoch are too far away from the present the routine returns 
    with an error. This routine only returns an address if the
    corresponding reflective memory module exists and if the 
    test point interface direct access is enable for this node.
    However, this routine does not guarantee that the data at the
    address is valid at the time it returns.

    @param node return argument for node id
    @param tp test point
    @param time time of request
    @param epoch epoch of request
    @return address of test point if successful, <0 otherwise
    @author DS, June 98
************************************************************************/
   int tpAddr (int node, testpoint_t tp, taisec_t time, int epoch);

/** ASCII interface to a test point interface.

    The function returns a string which was allocated with malloc. The
    caller is reposnible to free this string if no longer needed.

    @param cmd command string
    @return reply string
    @author DS, June 98
************************************************************************/
   char* tpCommand (const char* cmd);

/** ASCII interface to a test point interface.

    The function returns calls tpCommand and writes the result to
    stdout.

    @param cmd command string
    @return 0 if successful, <0 otherwise
    @author DS, June 98
************************************************************************/
   int tpcmdline (const char* cmd);


/*@}*/

/*@}*/


#ifdef __cplusplus
}
#endif

#endif /*_GDS_TESTPOINT_H */
