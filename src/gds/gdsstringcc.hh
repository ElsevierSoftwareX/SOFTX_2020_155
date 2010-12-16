/* Version: $Id$ */
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: gdsstringcc						*/
/*                                                         		*/
/* Module Description: adds additional string handling routines		*/
/*									*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 24Apr98  MRP/DS    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: gdsstring.html					*/
/*	References: none						*/
/*                                                         		*/
/* Author Information:							*/
/* Name          Telephone       Fax             e-mail 		*/
/* Daniel Sigg   (509) 372-8336  (509) 372-2178  sigg_d@ligo.mit.edu	*/
/* Mark Pratt    (617) 253-6410			 mrp@mit.edu		*/
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
#ifndef _GDS_GDSSTRING_CC_H
#define _GDS_GDSSTRING_CC_H

#include <time.h>
#include <string>
#include <functional>
#include <set>
#include "PConfig.h"
#ifdef __GNU_STDC_OLD
#include <gnusstream.h>
#else
#include <sstream>
#endif
#include "dtt/gdsstring.h"


/** @name Additional C++ classes
   Implements string classes which are not case sensitive and for
   temporary file names.

   @memo String handling routines for C++
   @author MRP/DS, Apr. 1998  */

/*@{*/ 


namespace diag {
   using std::string;


/** This string class describes a name for temporary file.
    @memo Temporary file name.
    @author DS, November 98
    @see Diagnostics storage API
 ************************************************************************/
   class tempFilename : public std::string {
   public:
      /** Constructs a string holding a name of a temoparary file.
          @memo Default constructor.
          @return void
       ******************************************************************/
      tempFilename ();
   };


/** This string compare class template is a binary function obejct 
    comparing two strings ignoring their case. The template parameter 
    must be an integer compare function object. It will be applied on
    (gds_strcasecmp (s1, s2), 0).
   
    @param Compare integer comapare function
    @memo Template for comparing string.
    @author DS, November 98
    @see Diagnostics storage API
 ************************************************************************/
template <class Compare = std::less<int> >
   struct stringcase_comp : 
   std::binary_function <string, string, bool> {
   public:
      stringcase_comp() {
      }
      /** Compares two strings ignroing their case.
          @param s1 first string
   	  @param s2 second string
          @memo Function operator.
          @return result of compare
       ******************************************************************/
      bool operator() (const string& s1, const string& s2) const {
         return Compare() (gds_strcasecmp (s1.c_str(), s2.c_str()), 0);
      }
   };


#ifndef __GNU_STDC_OLD
/** This traits class is not case sensitive.
    @memo case insensitive traits.
    @author DS, November 98
 ************************************************************************/
   struct case_char_traits : public std::char_traits<char> {
      static bool eq (const char_type& c1, const char_type& c2) { 
         return tolower (c1) == tolower (c2); 
      }
      static bool ne (const char_type& c1, const char_type& c2) { 
         return !(tolower (c1) == tolower (c2));
      }
      static bool lt (const char_type& c1, const char_type& c2) { 
         return tolower (c1) < tolower (c2);
      }
      static int compare (const char_type* s1, const char_type* s2, size_t n) { 
         return gds_strncasecmp (s1, s2, n); 
      }
   };


/** This string class is not case sensitive.
    @memo case insensitive string.
    @author DS, November 98
 ************************************************************************/
   typedef std::basic_string <char, case_char_traits> stringcase;
#else


   class stringcase : public std::string {
   public:
      stringcase (const char* s) : string (s) {
      }
      stringcase (const string& s) : string (s) {
      }
      bool operator== (const string& s) {
         return gds_strcasecmp (c_str(), s.c_str()) == 0; }
      bool operator!= (const string& s) {
         return gds_strcasecmp (c_str(), s.c_str()) != 0; }
      bool operator< (const string& s) {
         return gds_strcasecmp (c_str(), s.c_str()) < 0; }
      bool operator<= (const string& s) {
         return gds_strcasecmp (c_str(), s.c_str()) <= 0; }
      bool operator> (const string& s) {
         return gds_strcasecmp (c_str(), s.c_str()) > 0; }
      bool operator>= (const string& s) {
         return gds_strcasecmp (c_str(), s.c_str()) >= 0; }
   };

#endif

}

/*@}*/

#endif /* _GDS_GDSSTRING_CC_H */
