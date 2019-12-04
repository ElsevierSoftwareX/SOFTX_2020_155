/* Version: $Id$ */
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: rmorg							*/
/*                                                         		*/
/* Module Description: 	Reflective memory organization			*/
/*									*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 9Sep98   D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: gds html pages					*/
/*	References: none						*/
/*                                                         		*/
/* Author Information:							*/
/* Name          Telephone       Fax             e-mail 		*/
/* Daniel Sigg   (509) 372-8132  (509) 372-2178  sigg_d@ligo.mit.edu	*/
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

#ifndef _GDS_RMORG_H
#define _GDS_RMORG_H

#ifdef __cplusplus
extern "C" {
#endif


/* Header File List: */
#include "dtt/hardware.h"
#include "dtt/map.h"

/** @name Reflective Memory Organization
    This module defines the organizational structure of the reflective
    memory. This API is a wrapper around the "map.h" file of the DAQ 
    system.

    @memo Defines the memory layout
    @author Written September 1998 by Daniel Sigg
    @see DAQ reflective memory organization, Testpoint Definition
    @version 0.1
************************************************************************/

/*@{*/

/** @name Constants and flags.
    Constants and flags of the reflective memory layout.

    @memo Constants and flags
    @author DS, September 98
    @see DAQ reflective memory organization
************************************************************************/

/*@{*/

/** @name Limits

    @author DS, September 98
    @see DAQ reflective memory organization
************************************************************************/

/*@{*/

/** Size of test point channel data element. This number is in bytes.

    @author DS, September 98
    @see Testpoint Definition
************************************************************************/
#define TP_DATUM_LEN		sizeof (float)

/** Size of reserved space for the test point index. This number is in
    bytes.

    @author DS, September 98
    @see Testpoint Definition
************************************************************************/
#define TP_INDEX_SIZE 		0x100

/** Maximum number of test point nodes. This is identical to the number
    of reflective memory rings. UNIX target has command line argument
    to set the node id.

    @author DS, September 98
    @see Testpoint Definition
************************************************************************/
#define TP_MAX_NODE        256

/** Maximum number of test point interfaces. This number is currently 4
    (LSC/ASC excitation and LSC/ASC test point readout).

    @author DS, September 98
    @see Testpoint Definition
************************************************************************/
#define TP_MAX_INTERFACE	4

/** Maximum number of entries in the test point index. This number must 
    be greater or equal than any of the individual test point indexes.

    @author DS, September 98
    @see Testpoint Definition
************************************************************************/
#define TP_MAX_INDEX		64

/** Highest test point index number.

    @author DS, September 98
    @see Testpoint Definition
************************************************************************/
#define TP_HIGHEST_INDEX	60000

/** Maximum number of preselected testpoints.

    @author DS, September 98
    @see Testpoint Definition
************************************************************************/
#define TP_MAX_PRESELECT	20

/*@}*/

/** @name Identification numbers

    @author DS, September 98
    @see DAQ reflective memory organization
************************************************************************/

/*@{*/

/** Test point node id of node 0.

    @author DS, September 98
    @see Testpoint Definition
************************************************************************/
#define TP_NODE_0		0

/** Test point node id of node 1.

    @author DS, September 98
    @see Testpoint Definition
************************************************************************/
#define TP_NODE_1		1

/** Test point interface id of LSC excitation.

    @author DS, September 98
    @see Testpoint Definition
************************************************************************/
#define TP_LSC_EX_INTERFACE	0

/** Test point interface id of ASC excitation.

    @author DS, September 98
    @see Testpoint Definition
************************************************************************/
#define TP_ASC_EX_INTERFACE	1

/** Test point interface id of LSC readout.

    @author DS, September 98
    @see Testpoint Definition
************************************************************************/
#define TP_LSC_TP_INTERFACE	2

/** Test point interface id of ASC readout.

    @author DS, September 98
    @see Testpoint Definition
************************************************************************/
#define TP_ASC_TP_INTERFACE	3

/** Test point interface id of the diagital-to-analog converter. (Does 
    not have a corresponding reflective memory region; this is a logical
    id only.)

    @author DS, September 98
    @see Testpoint Definition
************************************************************************/
#define TP_DAC_INTERFACE	100

/** Test point interface id of DS340 signal generator. (Does not have
    a corresponding reflective memory region; this is a logical id only.)

    @author DS, September 98
    @see Testpoint Definition
************************************************************************/
#define TP_DS340_INTERFACE	101

/*@}*/

/** @name Index lengths

    @author DS, September 98
    @see DAQ reflective memory organization
************************************************************************/

/*@{*/

/** Defines the number of LSC excitation engine test points.

    @author DS, June 98
    @see Test point API
************************************************************************/
#define TP_LSC_EX_NUM		64

/** Defines the number of LSC test point outputs.

    @author DS, June 98
    @see Test point API
************************************************************************/
#define TP_LSC_TP_NUM		64

/** Defines the number of ASC excitation engine test points.

    @author DS, June 98
    @see Test point API
************************************************************************/
#define TP_ASC_EX_NUM		64

/** Defines the number of ASC test point outputs.

    @author DS, June 98
    @see Test point API
************************************************************************/
#define TP_ASC_TP_NUM		64

/*@}*/

/** @name Channel lengths

    @author DS, September 98
    @see DAQ reflective memory organization
************************************************************************/

/*@{*/

/** Channel length of LSC test points. This is 1/16th of the LSC sampling
    rate of 16386Hz.

    @author DS, June 98
    @see Test point API
************************************************************************/
#ifdef GDS_UNIX_TARGET
extern int sys_freq_mult; /* how many times faster than 16 kHz is the system */
#define TP_LSC_CHN_LEN		(1024 * sys_freq_mult)
#else
#define TP_LSC_CHN_LEN		1024
#endif

/** Channel length of LSC test points. This is 1/16th of the ASC sampling
    rate of 2048Hz.

    @author DS, June 98
    @see Test point API
************************************************************************/
#ifdef GDS_UNIX_TARGET
extern int sys_freq_mult; /* how many times faster than 16 kHz is the system */
#define TP_ASC_CHN_LEN		(128 * sys_freq_mult)
#else
#define TP_ASC_CHN_LEN		128
#endif

/*@}*/

/** @name Test point numbers

    @author DS, September 98
    @see DAQ reflective memory organization
************************************************************************/

/*@{*/

/** Defines the test point ID offset for the LSC excitastion engine.

    @author DS, June 98
    @see Test point API
************************************************************************/
#define TP_ID_LSC_EX_OFS	1

/** Defines the test point ID offset for the LSC system.

    @author DS, June 98
    @see Test point API
************************************************************************/
#define TP_ID_LSC_TP_OFS	10000

/** Defines the test point ID offset for the ASC excitation engine.

    @author DS, June 98
    @see Test point API
************************************************************************/
#define TP_ID_ASC_EX_OFS	20000

/** Defines the test point ID offset for the ASC system.

    @author DS, June 98
    @see Test point API
************************************************************************/
#define TP_ID_ASC_TP_OFS	30000

/** Defines the test point ID offset for the DAC channels.

    @author DS, June 98
    @see Test point API
************************************************************************/
#define TP_ID_DAC_OFS		40000

/** Defines the test point ID offset for the DS340 channels.

    @author DS, June 98
    @see Test point API
************************************************************************/
#define TP_ID_DS340_OFS		50000

/** Defines the test point ID offset for the last TP.

    @author DS, June 98
    @see Test point API
************************************************************************/
#define TP_ID_END_OFS		60000

/** Defines the test point ID for clearing all selected TPs.

    @author DS, June 98
    @see Test point API
************************************************************************/
#define _TP_CLEAR_ALL		((unsigned short)(-1))

/*@}*/
/*@}*/

/** @name Functions and Macros.
    * Functions of the reflective memory layout.

    @memo Functions and Macros
    @author DS, September 98
    @see DAQ reflective memory organization
************************************************************************/

/*@{*/

#if 0
/** Returns the DCU node ID as function of the unit ID.

    @param A unit ID
    @return DCU node ID
    @author DB, June 98
************************************************************************/
#define UNIT_ID_TO_RFM_NODE_ID(A)

/** Returns the DCU offset as function of the unit ID.

    @param A unit ID
    @return DCU offset
    @author DB, June 98
************************************************************************/
#define UNIT_ID_TO_RFM_OFFSET(A)

/** Returns the DCU size as function of the unit ID.

    @param A unit ID
    @return DCU size
    @author DB, June 98
************************************************************************/
#define UNIT_ID_TO_RFM_SIZE(A)

/** Returns the DCU name as function of the unit ID.

    @param A unit ID
    @return DCU name
    @author DB, June 98
************************************************************************/
#define UNIT_ID_TO_NAME(A)

/** Returns TRUE if DCU is a test point DCU, FALSE otherwise.

    @param A unit ID
    @return 1 if test point DCU, 0 otherwise
    @author DS, June 98
************************************************************************/
#define IS_TP(A)

/** Returns the test point node ID as function of the unit ID.

    @param A unit ID
    @return test point node ID
    @author DS, June 98
************************************************************************/
#define UNIT_ID_TO_TP_NODE(A)

/** Returns the test point interface ID as function of the unit ID.

    @param A unit ID
    @return test point interface ID
    @author DS, June 98
************************************************************************/
#define UNIT_ID_TO_TP_INTERFACE(A)

/** Returns the unit ID as function of the node ID and the test point
    interface ID.

    @param B node ID
    @param A test point interface ID
    @return unit ID
    @author DS, June 98
************************************************************************/
#define TP_NODE_INTERFACE_TO_UNIT_ID(B,A)

/** Returns the test point interface ID as function of the test point
    number. Zero is an invalid test point number and the returned
    interface id will also be invalid (-1).

    @param A test point number
    @return test point interface ID
    @author DS, June 98
************************************************************************/
#define TP_ID_TO_INTERFACE(A) 

/** Returns the test point channel length as function of the test point
    interface ID.

    @param A test point interface ID
    @return test point channel length
    @author DS, June 98
************************************************************************/
#define TP_INTERFACE_TO_CHN_LEN(A)

/** Returns the number of test point indexes as function of the test
    point interface ID.

    @param A test point interface ID
    @return number of test point indexes
    @author DS, June 98
************************************************************************/
#define TP_INTERFACE_TO_INDEX_LEN(A)

/** Returns the address of the DCU as function of the node ID and the 
    test point interface ID.

    @param B node ID
    @param A test point interface ID
    @return address of test point indexes
    @author DS, June 98
************************************************************************/
#define TP_NODE_INTERFACE_TO_RFM_OFFSET(B,A)

/** Returns the size of the DCU as function of the node ID and the 
    test point interface ID.

    @param B node ID
    @param A test point interface ID
    @return address of test point indexes
    @author DS, June 98
************************************************************************/
#define TP_NODE_INTERFACE_TO_RFM_SIZE(B,A)

/** Returns the address of the test point indexes as function of the 
    node ID and the test point interface ID.

    @param B node ID
    @param A test point interface ID
    @return address of test point indexes
    @author DS, June 98
************************************************************************/
#define TP_NODE_INTERFACE_TO_INDEX_OFFSET(B,A)

/** Returns the address of the test point data as function of the 
    node ID and the test point interface ID.

    @param B node ID
    @param A test point interface ID
    @return address of test point indexes
    @author DS, June 98
************************************************************************/
#define TP_NODE_INTERFACE_TO_DATA_OFFSET(B,A)

/** Returns the block size of the test point data as function of the 
    node ID and the test point interface ID.

    @param B node ID
    @param A test point interface ID
    @return address of test point indexes
    @author DS, June 98
************************************************************************/
#define TP_NODE_INTERFACE_TO_DATA_BLOCKSIZE(B,A)

/** Returns the reflective memory ID as function of the node ID.

    @param B node ID
    @return reflective memory ID
    @author DS, June 98
************************************************************************/
#define TP_NODE_ID_TO_RFM_ID(A)

/** Returns the size of the DCU header information.

    @param A unit ID
    @return DCU header size
    @author DS, June 98
************************************************************************/
#define UNIT_ID_TO_RFM_INTRO_SIZE(A)

/** Returns the offset to the data area as function of the unit ID.

    @param A unit ID
    @return offset to the data area
    @author DS, June 98
************************************************************************/
#define UNIT_ID_TO_DATA_OFFSET(A)

/** Returns the data block size as function of the unit ID.

    @param A unit ID
    @return data block size
    @author DS, June 98
************************************************************************/
#define UNIT_ID_TO_DATA_BLOCKSIZE(A)

/** Returns the channel address for a given epoch.

    @param rmOffset channel offset to first data block
    @param rmBlockSize data block size
    @param epoch epoch of data
    @return channel address
    @author DS, June 98
************************************************************************/
#define CHN_ADDR(rmOffset, rmBlockSize, epoch)
#endif

#define UNIT_ID_TO_TP_NODE(A) ( \
        ((A) == GDS_2k_LSC_EX_ID ? TP_NODE_1 :     \
        ((A) == GDS_2k_ASC_EX_ID ? TP_NODE_1 :     \
        ((A) == GDS_2k_LSC_TP_ID ? TP_NODE_1 :     \
        ((A) == GDS_2k_ASC_TP_ID ? TP_NODE_1 :     \
        ((A) == GDS_4k_LSC_EX_ID ? TP_NODE_0 :     \
        ((A) == GDS_4k_ASC_EX_ID ? TP_NODE_0 :     \
        ((A) == GDS_4k_LSC_TP_ID ? TP_NODE_0 :     \
        ((A) == GDS_4k_ASC_TP_ID ? TP_NODE_0 :     \
         -1)))))))))

#define UNIT_ID_TO_TP_INTERFACE(A) ( \
        ((A) == GDS_2k_LSC_EX_ID ? TP_LSC_EX_INTERFACE :     \
        ((A) == GDS_2k_ASC_EX_ID ? TP_ASC_EX_INTERFACE :     \
        ((A) == GDS_2k_LSC_TP_ID ? TP_LSC_TP_INTERFACE :     \
        ((A) == GDS_2k_ASC_TP_ID ? TP_ASC_TP_INTERFACE :     \
        ((A) == GDS_4k_LSC_EX_ID ? TP_LSC_EX_INTERFACE :     \
        ((A) == GDS_4k_ASC_EX_ID ? TP_ASC_EX_INTERFACE :     \
        ((A) == GDS_4k_LSC_TP_ID ? TP_LSC_TP_INTERFACE :     \
        ((A) == GDS_4k_ASC_TP_ID ? TP_ASC_TP_INTERFACE :     \
         -1)))))))))

#define TP_INTERFACE_TO_CHN_LEN(A) ( \
        ((A) == TP_LSC_EX_INTERFACE ? TP_LSC_CHN_LEN :     \
        ((A) == TP_ASC_EX_INTERFACE ? TP_ASC_CHN_LEN :     \
        ((A) == TP_LSC_TP_INTERFACE ? TP_LSC_CHN_LEN :     \
        ((A) == TP_ASC_TP_INTERFACE ? TP_ASC_CHN_LEN :     \
         0)))))

#define TP_INTERFACE_TO_INDEX_LEN(A) ( \
        ((A) == TP_LSC_EX_INTERFACE ? TP_LSC_EX_NUM :     \
        ((A) == TP_ASC_EX_INTERFACE ? TP_ASC_EX_NUM :     \
        ((A) == TP_LSC_TP_INTERFACE ? TP_LSC_TP_NUM :     \
        ((A) == TP_ASC_TP_INTERFACE ? TP_ASC_TP_NUM :     \
         0)))))

#define TP_NODE_INTERFACE_TO_UNIT_ID(B,A) ( \
        ((A) == TP_LSC_EX_INTERFACE ? \
                ((B) == TP_NODE_0 ? GDS_4k_LSC_EX_ID : \
                ((B) == TP_NODE_1 ? GDS_2k_LSC_EX_ID : \
                 -1)) :     \
        ((A) == TP_ASC_EX_INTERFACE ? \
                ((B) == TP_NODE_0 ? GDS_4k_ASC_EX_ID : \
                ((B) == TP_NODE_1 ? GDS_2k_ASC_EX_ID : \
                 -1)) :     \
        ((A) == TP_LSC_TP_INTERFACE ? \
                ((B) == TP_NODE_0 ? GDS_4k_LSC_TP_ID : \
                ((B) == TP_NODE_1 ? GDS_2k_LSC_TP_ID : \
                 -1)) :     \
        ((A) == TP_ASC_TP_INTERFACE ? \
                ((B) == TP_NODE_0 ? GDS_4k_ASC_TP_ID : \
                ((B) == TP_NODE_1 ? GDS_2k_ASC_TP_ID : \
                 -1)) :     \
        -1)))))

#if RMEM_LAYOUT  > 0
#include "gdsLib2.h"

/* Macro assumes ETMX SUS controllers, VEA ICS115s and HEPI controllers are on the second RFM net */
#define TP_ID_TO_RMEM(A) ( \
	(((A) >= GDS_ETMX_VALID_EX_MIN) && ((A) < GDS_ETMY_VALID_EX_MAX)) \
	|| (((A) >= GDS_HEPI1_VALID_EX_MIN) && ((A) < GDS_HEPIY_VALID_EX_MAX)) \
	|| (((A) >= GDS_ADCU3_VALID_XEX_LOW) && ((A) < GDS_ADCU3_VALID_XEX_HI)) \
	|| (((A) >= GDS_ADCU4_VALID_XEX_LOW) && ((A) < GDS_ADCU4_VALID_XEX_HI)) \
	)
#define TP_NODE_INTERFACE_TO_INDEX_OFFSET(B,A) ( \
        IS_UNIT(TP_NODE_INTERFACE_TO_UNIT_ID(B,A)) ? \
	   DAQ_GDS_BLOCK_ADD \
		+ ((A) * (sizeof(GDS_CNTRL_BLOCK) / TP_MAX_INTERFACE)) : \
	   -1)
#else
#define TP_NODE_INTERFACE_TO_INDEX_OFFSET(B,A) ( \
        IS_UNIT(TP_NODE_INTERFACE_TO_UNIT_ID(B,A)) ? \
           TP_NODE_INTERFACE_TO_DATA_OFFSET(B,A) - TP_INDEX_SIZE : \
           -1)
#endif

#define TP_NODE_INTERFACE_TO_RFM_OFFSET(B,A) ( \
        IS_UNIT(TP_NODE_INTERFACE_TO_UNIT_ID(B,A)) ? \
           UNIT_ID_TO_RFM_OFFSET(TP_NODE_INTERFACE_TO_UNIT_ID(B,A)) : \
           -1)

#define TP_NODE_INTERFACE_TO_RFM_SIZE(B,A) ( \
        IS_UNIT(TP_NODE_INTERFACE_TO_UNIT_ID(B,A)) ? \
           UNIT_ID_TO_RFM_SIZE(TP_NODE_INTERFACE_TO_UNIT_ID(B,A)) : \
           0)

#define TP_NODE_INTERFACE_TO_DATA_OFFSET(B,A) ( \
        UNIT_ID_TO_DATA_OFFSET(TP_NODE_INTERFACE_TO_UNIT_ID(B,A)))

#define TP_NODE_INTERFACE_TO_DATA_BLOCKSIZE(B,A) ( \
        UNIT_ID_TO_DATA_BLOCKSIZE(TP_NODE_INTERFACE_TO_UNIT_ID(B,A)))

#define TP_ID_TO_INTERFACE(A) ( \
	((A) == 0 ? -1 :	    \
	((A) < TP_ID_LSC_TP_OFS ? TP_LSC_EX_INTERFACE :	    \
	((A) < TP_ID_ASC_EX_OFS ? TP_LSC_TP_INTERFACE :	    \
	((A) < TP_ID_ASC_TP_OFS ? TP_ASC_EX_INTERFACE :	    \
	((A) < TP_ID_DAC_OFS ? TP_ASC_TP_INTERFACE :	    \
	((A) < TP_ID_DS340_OFS ? TP_DAC_INTERFACE :	    \
        ((A) < TP_ID_END_OFS ? TP_DS340_INTERFACE :         \
         -1))))))))

#ifdef GDS_UNIX_TARGET
#define TP_NODE_ID_TO_RFM_ID(A) 0
#else
#define TP_NODE_ID_TO_RFM_ID(A) ( \
	((A) == 0 ? 0 : \
	((A) == 1 ? (VMIVME5588_1_BASE_ADDRESS == 0 ? 0 : 1) : \
	 -1)))
#endif

/*#define IS_TP(A) \
        (UNIT_ID_TO_TP_INTERFACE(A) == -1 ? 0 : 1)*/
#define IS_TP(A) \
        (A->tpNum <= 0 ? 0 : 1)

#define IS_UNIT(A) \
        (UNIT_ID_TO_RFM_NODE_ID(A) == -1 ? 0 : 1)

#if RMEM_LAYOUT > 0

#define UNIT_ID_TO_RFM_INTRO_SIZE(A) 0
#define UNIT_ID_TO_DATA_OFFSET(A) (IS_UNIT(A) ? DATA_OFFSET_DCU(A): -1)
#define UNIT_ID_TO_DATA_BLOCKSIZE(A) \
	(IS_UNIT(A) ? (UNIT_ID_TO_RFM_SIZE(A) / DATA_BLOCKS): 0)
#else 

#define UNIT_ID_TO_RFM_INTRO_SIZE(A) \
        (IS_UNIT(A) ? IPC2DATA_OFFSET : 0)

#define UNIT_ID_TO_DATA_OFFSET(A) \
	(IS_UNIT(A) ? UNIT_ID_TO_RFM_OFFSET(A) + IPC2DATA_OFFSET : -1)

#define UNIT_ID_TO_DATA_BLOCKSIZE(A) \
	(IS_UNIT(A) ? \
	  (UNIT_ID_TO_RFM_SIZE(A) - IPC2DATA_OFFSET) / DATA_BLOCKS : \
	 0)

#endif /* RMEM_LAYOUT == 0 */

#define CHN_ADDR(rmOffset, rmBlockSize, epoch) \
		(rmOffset + (epoch % DATA_BLOCKS) * rmBlockSize)

/*@}*/

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /*_GDS_RMORG_H */
