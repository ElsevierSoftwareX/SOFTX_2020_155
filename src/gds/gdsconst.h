/* Version: $Id$ */
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: gdsconst			        		*/
/*                                                         		*/
/* Module Description: numerical and physical constants.		*/
/*									*/
/* Module Arguments: none                				*/
/*									*/
/* Revision History:					   		*/
/* Rel   Date     Programmer   		Comments       	       		*/
/* 0.1   1/1/99   Edward Daw            				*/
/*                                                                      */
/* Documentation References: 						*/
/*	Man Pages: doc++ generated html					*/
/*	References: Mathematica						*/
/*                                                         		*/
/* Author Information:							*/
/*	Name	Telephone    Fax          e-mail 			*/
/*	E. Daw  617-258-7697 617-253-7014 edaw@ligo.mit.edu		*/
/*                                                         		*/
/* Code Compilation and Runtime Specifications:				*/
/*	Code Compiled on: Unix						*/
/*	Compiler Used: Sun Sparcworks C compiler         		*/
/*	Runtime environment: UNIX    					*/
/*                                                         		*/
/* Code Standards Conformance:						*/
/*	Code Conforms to: LIGO standards.	TBD			*/
/*			  Lint.			TBD			*/
/*			  ANSI			TBD			*/
/*			  POSIX			TBD			*/
/*									*/
/* Known Bugs, Limitations, Caveats:                       		*/
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


#ifndef _GDS_CONST_H
#define _GDS_CONST_H

#ifdef __cplusplus
extern "C" {
#endif

/** @name Numerical and physical constants

    @memo Constants and Flags.
    @author DS
   **********************************************************************/

/*@{*/

/** @name Numerical Constants.

    @memo Numerical Constants.
    @author DS
   **********************************************************************/

/*@{*/

#ifdef PI
#undef PI
#endif
   /** Pi */
#define PI		3.1415926535897932384626433832795029

   /** Pi: alternative spelling */
#define ONE_PI		PI

   /**  two times pi */
#define TWO_PI		(2*ONE_PI)

   /** two over pi */
#define TWO_OVER_PI	(2 / ONE_PI)

   /** radians per degree */
#define RAD_PER_DEG	(ONE_PI/180.0)

   /** Euler e */
#define EULER_E		2.7182818284590452353602874713526625

   /** root two */
#define ROOT_TWO	1.4142135623730950488016887242096981

/*@}*/


/** @name Flags to select input data type.

    @memo Input data type.
    @author DS
   **********************************************************************/

/*@{*/

  /**Flag to specify real input data */
#define DATA_REAL                0
  /**Flag to specify complex input data */
#define DATA_COMPLEX             1

/*@}*/

/*@}*/

#ifdef __cplusplus
     }
#endif

#endif /* _GDS_CONST_H */
