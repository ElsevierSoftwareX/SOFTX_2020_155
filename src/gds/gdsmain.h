/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: gdsmain							*/
/*                                                         		*/
/* Module Description: Defines GDS target parameters	 		*/
/*									*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 03Apr98  D. Sigg    	First release		   		*/
/* 0.2	 08Apr98  M. Pratt    	removed enumeration of GDS_TARGET	*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: gdsmain.html						*/
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

#ifndef _GDS_MAIN_H
#define _GDS_MAIN_H

#include "PConfig.h"
#ifdef HAVE_CONFIG
#include "config.h"
#endif
#include <stdio.h>

/* possible site flag values */
#define GDS_SITE_NONE	0
#define GDS_SITE_LHO 	1
#define GDS_SITE_LLO 	2
#define GDS_SITE_MIT 	3
#define GDS_SITE_CIT 	4
#define GDS_SITE_GEO 	5

#ifndef TARGET
#define TARGET		0
#endif

#ifndef SITE
#define SITE GDS_SITE_NONE
#endif

#if 0
#if SITE == GDS_SITE_LLO
#define SITE_PREFIX 	"L"
#elif SITE == GDS_SITE_LHO
#define SITE_PREFIX 	"H"
#elif SITE == GDS_SITE_CIT
#define SITE_PREFIX 	"C"
#elif SITE == GDS_SITE_MIT
#define SITE_PREFIX 	"M"
#elif SITE == GDS_SITE_GEO
#define SITE_PREFIX 	"G"
#else
#define SITE_PREFIX "."
#endif
#endif


/* possible interferometer flag values */
#define GDS_IFO_NONE 0
#define GDS_IFO1 1
#define GDS_IFO2 2
#define GDS_IFO_PEM 3

#ifndef IFO
#define IFO GDS_IFO_NONE
#endif

#if 0
#if IFO == GDS_IFO1
#define IFO_PREFIX "1"
#elif IFO == GDS_IFO2
#define IFO_PREFIX "2"
#elif IFO == GDS_IFO_PEM
#define IFO_PREFIX "0"
#else
#define IFO_PREFIX ""
#endif
#endif

/* define gds archive path: */
#ifndef ARCHIVE
#ifdef OS_VXWORKS
#define ARCHIVE "/gds"
#elif defined (HAVE_CONFIG)
#define ARCHIVE ""
#else
#define ARCHIVE ""
#endif
#endif


/* doc++ stuff for above defines */

#if 0

/**
   @name Compiling Environment
   * This module defines target specific parameters and global constants.

   @memo Specfies target dependent constants
   @author Written Mar. 1998 by Daniel Sigg & Mark Pratt
   @version 0.5
************************************************************************/

/*@{*/		/* GDS Macros and Compiler Flags */

/** Environment variable defining the target. This flag is set at 
    compile time via the Makefile as -DTARGET=${GDS_TARGET}. Recognized 
    values are numbers only.

    @memo Target identification number
    @author MRP, Apr. 1998
    @see Main
************************************************************************/
setenv  GDS_TARGET

/** Environment variable defining the target operating system.  This
    flag is set at compile time via the Makefile as -D${GDS_OS_TYPE}.
    Recognized values are:

    OS_VXWORKS   used to indicate compilation for VxWorks

    These flags are mutually exculsive and one must be set. Furthermore
    this flag must match GDS_PROCESSOR.

    @memo Operating system of target
    @author MRP, Apr. 1998
    @see Main and GDS_PROCESSOR
************************************************************************/
setenv  GDS_OS_TYPE

/** Environment variable defining the target processor. This flag is
    is set at compile time via the Makefile as -D${GDS_PROCESSOR}.
    Recognized flags are:

    PROCESSOR_BAJA47  used to denote a Baja MIPS front-end processor

    PROCESSOR_MV162   used to deonte a Motorola 680X0 front-end processor

    PROCESSOR_I486    used to deonte a Pentium front-end processor

    These flags are mutually exculsive and one must be set. Furthermore
    this flag must match GDS_OS_TYPE

    @memo Target processor
    @author MRP, Apr. 1998
    @see Main and GDS_OS_TYPE
************************************************************************/
setenv  GDS_PROCESSOR

/** Environment variable specifying the gds archive directory.
    This is a site specific valued flag normally set by the Makefile
    at compile time as -DARCHIVE=${GDS_ARCHIVE}. If no value is
    provided, a default is determined using the value of SITE.

    @memo Archive path of target system
    @author MRP, Apr. 1998
    @see Main
************************************************************************/
setenv GDS_ARCHIVE

/** Environment variable definining the target site.  This is a site
    specific valued flag, normally set by the Makefile at compile
    time with -DSITE=${GDS_SITE}. Recognized values are:
    
    GDS_SITE_NONE | GDS_SITE_LHO | GDS_SITE_LLO | GDS_SITE_MIT | GDS_SITE_CIT

    If SITE is undefined, SITE takes on GDS_SITE_NONE as the default.

    @memo Site where target system resides
    @author DS, March 98
    @see Main
************************************************************************/
setenv GDS_SITE

/** Defines the site prefix. This macro is defined within gdsmain.h
    depending on the value of SITE. Possible values are

    "" | "H" | "L" | "M" | "C"

    for Undefined, Hanford, Livingston, MIT and Caltech respectively .

    @author DS, March 98
    @see Main
************************************************************************/
#define SITE_PREFIX

/** Environment variable declaring the target IFO. This is a site
    specific valued flag normally this by the Makefile at compile time
    with -DIFO=${GDS_IFO}. Recognized values are:
    
    GDS_IFO_NONE | GDS_IFO1 | GDS_IFO2 | GDS_IFO_PEM

    If IFO is undefined, IFO takes on GDS_IFO_NONE as the default.

    @memo Target system interferometer
    @author DS, March 98
    @see Main
************************************************************************/
setenv GDS_IFO

/** Defines the interferometer prefix. Possible values are
    This macro is defined within gdsmain.h depending on the value of IFO.
    Possible values are:
    
    "" | "1" | "2" | "0"

    for IFO1, IFO2 and PEM respectively.

    @author DS, March 98
    @see Main
************************************************************************/
#define IFO_PREFIX

#endif

/** Macro which returns a subdirectory under the GDS archive directory.
    The subdirectory path must include a preceeding '/', but not a 
    trailing one. The returned directory is 
    
    <ARCHIVE><subdir>
    
    @param subdir string describing the subdirectory path 
    @return subdirectory in the GDS archive directory
    @author DS, March 98
    @see Main and GDS_ARCHIVE
************************************************************************/
#define gdsPath(subdir) \
   ARCHIVE subdir

/** Macro which returns a parameter file section extended by the
    site and interferometer qualifier.
    The returned sections is 
    
    SITE_PREFIX IFO_PREFIX "-" section
    
    @param section parameter file section 
    @return qualified parameter file section
    @author DS, March 98
    @see Main and GDS_ARCHIVE
************************************************************************/
#define gdsSectionSiteIfo(section) \
   SITE_PREFIX IFO_PREFIX "-" section

/** Macro which returns a parameter file section extended by the
    site qualifier. The returned sections is 
    
    SITE_PREFIX "-" section
    
    @param section parameter file section 
    @return qualified parameter file section
    @author DS, March 98
    @see Main and GDS_ARCHIVE
************************************************************************/
#define gdsSectionSite(section) \
   SITE_PREFIX "-" section

/** Macro which returns a filename under the GDS archive directory.
    The subdirectory path must include a preceeding '/', but not a 
    trailing one. The returned filename is 
    
    <ARCHIVE><subdir>/<filename>
    
    @param subdir string describing the subdirectory path 
    @param filename string describing the short name of the file
    @return name of file in the GDS archive directory
    @author DS, March 98
    @see Main and GDS_ARCHIVE
************************************************************************/
static inline char* gdsPathFile(char *subdir, char *filename)  {
	static char s[1024];
	sprintf(s, "%s%s/%s", archive, subdir, filename);
	return s;
}

static inline char* gdsPathFile2(char *subdir, char *filename, char *c1, char *c2)  {
	static char s[1024];
	sprintf(s, "%s%s/%s%s%s", archive, subdir, filename, c1, c2);
	return s;
}

#ifndef __GNUC__
/* Solaris/Sparc works: use pargams for init/cleanup functions */

/** Macro to define a function prototype for a module initialialization
    function. The function prototype is of the form "void (*)(void)".
    Use as follows:
    \begin{verbatim}
    __init__(initXXX);
    #pragma init(initXXX)
    static void initXXX(void) { ... }
    \end{verbatim}
    This works with the SUN workshop compilers and with the GNU compilers.
    For C/C++ modules running under VxWorks one has to do a 'munching'
    using the GDS "munch" program (see the Tornado User Guide /
    Cross-Development for more details).
    
    @param name of initialization function
    @return void
    @author DS, July 98
    @see Module initialization and cleanup
************************************************************************/
#define __init__(name) \
	static void name(void)

/** Macro to define a function prototype for a module cleanup
    function. The function prototype is of the form "void (*)(void)".
    Use as follows:
    \begin{verbatim}
    __fini__(finiXXX);
    #pragma init(finiXXX)
    static void finiXXX(void) { ... }
    \end{verbatim}
    This works with the SUN workshop compiler and with the GNU compiler.
    For C/C++ modules running under VxWorks one has to do a 'munching'
    using the GDS "munch" program (see the Tornado User Guide /
    Cross-Development for more details).

    @param name of initialization function
    @return void
    @author DS, July 98
    @see Module initialization and cleanup
************************************************************************/
#define __fini__(name) \
	static void name(void)

/* GNU C here */
#elif !defined (__cplusplus) && defined(OS_VXWORKS)

/* VX WORKS */
#ifdef PROCESSOR_BAJA47
/* for ELF link format: use an alias to mimic a con/de-structor */
#define __init__(name) \
	static void name(void) __attribute__((constructor)); \
	void _GLOBAL__I_##name(void) __attribute__((weak, alias(#name)))

#define __fini__(name) \
	static void name(void) __attribute__((destructor)); \
	void _GLOBAL__D_##name(void) __attribute__((weak, alias(#name)))

#else
/* for other link formats: just call the function from a 
   con/de-structor look-alike */
#define __init__(name) \
	static void name(void); \
	void _GLOBAL__I_##name(void) {name();}

#define __fini__(name) \
	static void name(void); \
	void _GLOBAL__D_##name(void) {name();}

#endif /* PROCESSOR_BAJA47 */

/* OTHER operating systems */
#else /* !defined (__cplusplus) && defined(OS_VXWORKS) */

/* just use the con/de-structor attributes */
#define __init__(name) \
	static void name(void) __attribute__((constructor))

#define __fini__(name) \
	static void name(void) __attribute__((destructor))
#endif


/*@}*/

#endif /*_GDS_ERR_H */
