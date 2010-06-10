/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: timingCard.h					   	*/
/*                                                         		*/
/* Module Description: Hanford Data Acquisition System. 		*/
/*		       Include Header file for LIGO Timing Card.        */
/*                                                         		*/
/* Module Arguments:					   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date    Engineer   Comments			   		*/
/* 00    17Oct98 D.Barker   First Release.		   		*/
/* 01    21jun99 D.Barker   Add AS command.                             */
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages:							*/
/*	References:							*/
/*                                                         		*/
/* Author Information:							*/
/*	Name          Telephone    Fax          e-mail 			*/
/*	David Barker (509)3736203 (509)3722178 barker@ligo.caltech.edu  */
/*                                                         		*/
/* Code Compilation and Runtime Specifications:				*/
/*	Code Compiled on:						*/
/*	Compiler Used:							*/
/*	Runtime environment:						*/
/*                                                         		*/
/* Code Standards Conformance:						*/
/*	Code Conforms to: ANSI, LINT, POSIX, etc.			*/
/*									*/
/* Known Bugs, Limitations, Caveats:					*/
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
/*----------------------------------------------------------------------*/

#ifndef _GDS_TC_H
#define _GDS_TC_H
#ifdef __cplusplus
extern "C" {
#endif

/** @name Timing board
    This routines are used by ICS115 DAC driver to set and arm the
    timing board.

    @memo Interface to the GPS Clock Driver
    @author Written June 1999 by Christine Patton
    @version 0.1
    @see Arbitrary Waveform Generator, D980369-11-C.
************************************************************************/

/*@{*/

/** Initializes the timing board.
    @return 0 if successful, <0 otherwise
    @author CP, June 99
   *********************************************************************/
   int initTimingCard (void); 

/** Resets the timing board.
    @return 0 if successful, <0 otherwise
    @author CP, June 99
   *********************************************************************/
   int resetTimingCard (void);

/** Arms the timing board.
    @return 0 if successful, <0 otherwise
    @author CP, June 99
   *********************************************************************/
   int armingTimingCard (void);

/** Returns the status of the timing board.
    \begin{verbatim}
    BIT 0 - enable 1PPS syncronization
    BIT 1 - Clock enable (aux. connector)
    BIT 3 - Syncronized with 1 PPS
    BIT 4 - Syncronized with 4MHz clock
    BIT 5 - Aquire/Trigger enabled
    BIT 6 - Synchronization error
    \end{verbatim}
    @return 0 if successful, <0 otherwise
    @author CP, June 99
   *********************************************************************/
   int statusTimingCard (void);

/*@}*/

#ifdef __cplusplus
}
#endif
#endif /* _GDS_TC_H */
