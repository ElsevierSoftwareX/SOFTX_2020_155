/* Version: $Id$ */
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: targets							*/
/*                                                         		*/
/* Module Description: 	Defines target numbers				*/
/*									*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 16Jul98  D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: targets.html						*/
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

#ifndef _GDS_TARGETS_H
#define _GDS_TARGETS_H

#ifdef __cplusplus
extern "C" {
#endif

/** @name Target Definitions
    This header files provides the target definitions and the macros 
    which use them to obtain the hardware configuration.

    @memo Defines the compiling environment for a sepcific target
    @author Written July 1998 by Daniel Sigg
    @see hardware.h
    @version 0.1
************************************************************************/

/*@{*/

/* excitation engine in LVEA, 4K */
#define TARGET_H1_GDS_AWG1	100
/* excitation engine in EX */
#define TARGET_H1_GDS_AWG2	101
/* excitation engine in EY */
#define TARGET_H1_GDS_AWG3	102
/* excitation engine in LVEA, 2K */
#define TARGET_H2_GDS_AWG1	120
/* excitation engine in MX */
#define TARGET_H2_GDS_AWG2	121
/* excitation engine in MY */
#define TARGET_H2_GDS_AWG3	122
/* test point manager */
#define TARGET_H_RM_MANAGER	150

/* excitation engine in LVEA, 4K */
#define TARGET_L1_GDS_AWG1	300
/* excitation engine in EX */
#define TARGET_L1_GDS_AWG2	301
/* excitation engine in EY */
#define TARGET_L1_GDS_AWG3	302
/* test point manager */
#define TARGET_L_RM_MANAGER	350

/* AWG/tpman at MIT */
#define TARGET_M_GDS_UNIX	500
#define TARGET_L_GDS_UNIX	501
#define TARGET_H_GDS_UNIX	502
#define TARGET_G_GDS_UNIX	503
#define TARGET_C_GDS_UNIX	504


/** @name Target Names

    The following target names are defined:

    \begin{verbatim}

	TARGET_H1_GDS_AWG1	100	excitation engine, 4K LVEA
	TARGET_H1_GDS_AWG2	101	excitation engine, 4K EX
	TARGET_H1_GDS_AWG3	102	excitation engine, 4K EX
	TARGET_H2_GDS_AWG1	120	excitation engine, 2K LVEA
	TARGET_H2_GDS_AWG2	121	excitation engine, 2K MX
	TARGET_H2_GDS_AWG3	122	excitation engine, 2K MY
	TARGET_H_RM_MANAGER	150	test point manager, LHO
	TARGET_H_GDS_UNIX	200
	TARGET_H_GDS_UNIX	250	test point library (stand-alone)
	TARGET_H_GDS_UNIX	260	awg library (stand-alone)
	TARGET_L_GDS_CONF	300
	TARGET_L1_GDS_AWG1	300
	TARGET_L_GDS_UNIX	400

    \end{verbatim}

    @memo List of target names
    @author DS, July 98
    @see hardware.h
    @version 0.1
************************************************************************/

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /*_GDS_TARGETS_H */
