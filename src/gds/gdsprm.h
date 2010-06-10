/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: GDS Parameter File API 					*/
/*                                                         		*/
/* Module Description:  GDS Parameter File API  prototypes		*/
/*									*/
/*                                                         		*/
/* Module Arguments:					   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date    Engineer   Comments			   		*/
/* 0.1   4/1/98  MRP/PG	    Initial release              		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: HTML documentation  					*/
/*	References:							*/
/*                                                         		*/
/* Author Information:							*/
/*	Name          Telephone    Fax          e-mail 			*/
/*	Mark Pratt    617-253-6410		mrp@mit.edu		*/
/*                                                         		*/
/* Code Compilation and Runtime Specifications:				*/
/*	Code Compiled on: Sun Ultra					*/
/*	Compiler Used: CC & gcc						*/
/*	Runtime environment: Solaris, vxWorks				*/
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

#ifndef _GDS_PARAM_H
#define _GDS_PARAM_H
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

 /**
    @name Parameter File API 

    * GDS ASCII parameter files comprise '\n' terminated lines of less
    than PARAM_ENTRY_LEN chars.  Files are organized by sections,
    delimited by lines of the form "[any_section_name]".  Within a
    section, parameters are assigned one per line eg.  "param_name =
    value". The value field is returned as a string beginning with the
    first non space character following the '='.  Comment lines,
    whitespace and empty lines are ignored.  Comments are preceeded by
    ";" or "\#" as the first non whitespace character.  Leading
    whitespace is ignored for comments and parameter assignments
    however there can be no leading whitespace on a section heading
    line.  Both section and parameter names are case-insensitve.

    The API provides for sequential reading to eliminate N^2 behaviour
    in reading large param files (providing the user scans in file
    order).  This is the default behaviour.  Setting rwd or passing in
    a zero cursor will cause search functions to start at the
    beginning of a file or block respectively.

    NOTE: Because of the capability for sequential reading, multiple
    occurances of section names within a file and parameter names
    within a section will cause ambiguous results that depend on the
    access history of the file or parameter block.  It's best to just
    not let this happen in a parameter file.

    @memo Reads and writes parameters from parameter files
    @author Written Mar. 1998 by Mark Pratt & Paul Govereau
    @version 0.1
*********************************************************************/

/*@{*/		/* subset of GDS Parameter File API documentation */

 /** Moves file pointer to the next parameter file section.
     Starts seek at current fp.  On success, returns cbuf filled with
     section name, null terminated, w/o brackets, and leaves fp
     pointing at first line of section.  On failure, returns 0 and
     rewinds fp.  Both fp and cbuf must be valid pointers, the
     latter at least char[PARAM_ENTRY_LEN] in size.

     WARNING: doesn't check for line too long.

     @param fp open parameter file pointer
     @param cbuf char buffer in which output is written
     @return section name string, cbuf, on success or 0 on failure
     @author PG & MRP, March 1998 
     @see GDS Parameter File API
*********************************************************************/
   char *
   nextParamFileSection (FILE* fp, char* cbuf);

 /** Looks for matching section heading in parameter file.
     Match is performed with strncmp(), where n is set by
     the length of sec_name.  If(rwd), seeks from start, otherwise
     seeks from current position and allows a single rewind.  On
     success, returns 1 and leaves fp pointing at first line of
     section.  On failure, returns 0 and leaves fp "near" entry point.
     Both fp and sec_name must be valid pointers.

     WARNING:  the match is performed only up to the length of the
     input sec_name so matches are possible on substrings of actual
     parameter names.

     @param fp open parameter file pointer
     @param sec_name requested section name
     @param rwd switch to begin search at top of file
     @return Places fp at section head and returns true if found
     @author PG & MRP, March 1998 
     @see GDS ParamFile API
*********************************************************************/
   int
   findParamFileSection (FILE *fp, const char *sec_name, int rwd);


 /** Returns a character array of the requested section.
     On success, returns named section, writes number of lines to
     nentry and leaves fp pointing above next section name.  On
     failure, returns 0 and leaves fp pointing near the entry point or
     at EOF.  Returns section as a zero padded block of
     char[nentry][PARAM_ENTRY_LEN].  There should be no comment, empty
     or whitespace only lines left in output.  Newline characters
     are removed from the section block;
      
     If sec_name && rwd, searches from beginning of file.
     If sec_name && !rwd, searches from current location & may wrap once.
     If sec_name = NULL, gets current section (no rwd option; should
     be proceeded by a call to nextParamFileSection or 
     findParamFileSection).

     WARNING: doesn't check for oversize entries.

     KLUDGE: to minimize allocated block, this function makes 2 reads.  

     @param fp open parameter file pointer
     @param sec_name requested section name
     @param rwd switch to begin search at top of file
     @param nentry overwritten with number of param entries in section
     @return Section block char[*nentry][PARAM_ENTRY_LEN]

     @author PG & MRP, March 1998 
     @see GDS Parameter File API
*********************************************************************/	
   char *
   getParamFileSection (FILE *fp, const char *sec_name,
                     int *nentry, int rwd );

 /** Returns pointer to next parameter entry in section.
     On success, returns pointer to null terminated param entry str
     and updates *cursor.  Seek starts at *cursor entry if it's valid,
     otherwise, starts at the beginning of section.  On failure,
     returns zero.

     NOTE: The cursor is implemented to reduce seek times for ordered
     reads.  This parameter is mangaged by the xxxParamSectionEntry()
     functions.  The intial call should be made with *cursor < 0 or >=
     nentry-1.

     @param section Section block as formatted by getParamFileSection
     @param nentry Number of param entries in section block
     @param cursor Optional ounter holding to current entry number
     @return entire param assignment entry as char[PARAM_ENTRY_LEN]

     @author MRP, March 1998 
     @see GDS Parameter File API and getParamFileSection()
*********************************************************************/
   char *
   nextParamSectionEntry (char *section, int nentry, int *cursor);

 /** Finds requested entry in parameter section.
     On success returns *cursor, the value (entry number) of the
     matching param entry.  On failure, returns a negative number.

     NOTE: The cursor is implemented to reduce seek times for ordered
     reads.  This parameter is mangaged by the xxxParamSectionEntry()
     functions.  The intial call should be made with *cursor < 0 or >=
     nentry-1.

     @param section Section block as formatted by getParamFileSection
     @param nentry Number of param entries in section block
     @param cursor Optional ounter holding to current entry number
     @return true if requested entry is found, false otherwise 

     @author MRP, March 1998 
     @see GDS Parameter File API and getParamFileSection()
*********************************************************************/	
   int
   findParamSectionEntry (const char *par_name, char *section,
                     int nentry, int *cursor);

 /** Returns value string of requested parameter declaration.
     On success returns the value field for the requested parameter,
     null terminated and stripped of leading fluff and '='.  On
     failure, returns 0.

     WARNING: Because partial names can be matched, there is increased
     danger of non-unique matches.

     NOTE: The cursor is implemented to reduce seek times for ordered
     reads.  This parameter is mangaged by the xxxParamSectionEntry()
     functions.  The intial call should be made with *cursor < 0 or >=
     nentry-1.

     @param par_name Requested parameter name (may be truncated)
     @param section Section block as formatted by getParamFileSection
     @param nentry Number of param entries in section block
     @param cursor Optional ounter holding to current entry number
     @return Value field of parameter assignment as char[].

     @author MRP, March 1998 
     @see GDS Parameter File API and getParamFileSection()
*********************************************************************/	
   char *
   getParamSectionEntry (const char *par_name, char *section,
                     int nentry, int *cursor);

 /** Loads value of requested parameter declaration.
     On success loads the value of the requested parameter into *def.
     The data type of the parameter is indicated by type.  If no value
     is found, *def is unaltered.  For boolean requests, a value field
     beginning with '0', 'F' or 'N' is taken as false, all other
     values yield true.

     WARNING: This function is easily abused - def must match the
     requested data type.

     NOTE: The cursor is implemented to reduce seek times for ordered
     reads.  This parameter is mangaged by the xxxParamSectionEntry()
     functions.  The intial call should be made with *cursor < 0 or >=
     nentry-1.

     @param par_name Requested parameter name (may be truncated)
     @param section Section block as formatted by getParamFileSection
     @param nentry Number of param entries in section block
     @param cursor Optional ounter holding to current entry number
     @param type 0 = boolean, 1 = int, 2 = double, 3 = string,
            4 = unsigned long (can be hex), 5 = short, 6 = float
     @param def pointer to default parameter value variable 
     @return error condition and writes the value to *def if found

     @author MRP, April 1998 
     @see GDS Parameter File API and getParamFileSection()
*********************************************************************/	
   int
   loadParamSectionEntry (const char *par_name, char *section,
                     int nentry, int *cursor, int type, void *def );

 /** Loads boolean parameter value.
     If file, section and param name are found, the value field is
     parsed for a boolean value which is written to *def.  If no value
     is found, *def retains its original value.  This is convenience
     function derived from lower level library functions.  A value
     field beginning with '0', 'F' or 'N' is taken as false, all other
     values yield true.

     NOTE: Because each call opens and closes the parameter file, this
     function should not be used for many succesive calls to the same
     file.

     @param filename pathname of file
     @param sec_name requested section name
     @param par_name requested parameter name (may be truncated)
     @param def pointer to default parameter value
     @return error condition and writes value to *def if found

     @author MRP, April 1998 
     @see GDS Parameter File API and loadParamSectionEntry
*********************************************************************/	
   int
   loadBoolParam (const char *filename, const char *sec_name,
                 const char *par_name, int *def); 

 /** Loads integer parameter value.
     If file, section and param name are found, the value field is
     parsed for an integer value which is written to *def.  If no value
     is found, *def retains its original value.  This is convenience
     function derived from lower level library functions.

     NOTE: Because each call opens and closes the parameter file, this
     function should not be used for many succesive calls to the same
     file.

     @param filename pathname of file
     @param sec_name requested section name
     @param par_name requested parameter name (may be truncated)
     @param def pointer to default parameter value
     @return error condition and writes value to *def if found

     @author MRP, April 1998 
     @see GDS Parameter File API and loadParamSectionEntry
*********************************************************************/	
   int
   loadIntParam (const char *filename, const char *sec_name,
                const char *par_name, int *def);

 /** Loads numerical parameter value.
     If file, section and param name are found, the value field is
     parsed for an numerical value which is written to *def.  If no value
     is found, *def retains its original value.  This is convenience
     function derived from lower level library functions. This function
     is almost identical to loadIntParam, but returns a long and defaults
     to hexadecimal (octal) representation, if the value is preceeded by
     '0x' ('0'). The value has to be positiv.

     NOTE: Because each call opens and closes the parameter file, this
     function should not be used for many succesive calls to the same
     file.

     @param filename pathname of file
     @param sec_name requested section name
     @param par_name requested parameter name (may be truncated)
     @param def pointer to default parameter value
     @return error condition and writes value to *def if found

     @author MRP, April 1998 
     @see GDS Parameter File API and loadParamSectionEntry
*********************************************************************/	
   int
   loadNumParam (const char *filename, const char *sec_name,
                const char *par_name, unsigned long *def);

 /** Loads Floating point parameter value (double precision).
     If file, section and param name are found, the value field is
     parsed for a numeric value which is written to *def.  If no value
     is found, *def retains its original value.  This is convenience
     function derived from lower level library functions.

     NOTE: Because each call opens and closes the parameter file, this
     function should not be used for many succesive calls to the same
     file.

     @param filename pathname of file
     @param sec_name requested section name
     @param par_name requested parameter name (may be truncated)
     @param def pointer to default parameter value
     @return error condition and writes value to *def if found

     @author MRP, April 1998 
     @see GDS Parameter File API and loadParamSectionEntry
*********************************************************************/	
   int
   loadFloatParam (const char *filename, const char *sec_name,
                  const char *par_name, double *def);

/** Loads string parameter value.
    If file, section and param name are found, the value field is
    parsed for a string in the value field which is copied to *def.
    If no value is found, *def retains its original value.  This is
    convenience function derived from lower level library functions.
    def should be allocated to at least char[PARAM_ENTRY_LEN].

     NOTE: Because each call opens and closes the parameter file, this
     function should not be used for many succesive calls to the same
     file.

     @param filename pathname of file
     @param sec_name requested section name
     @param par_name requested parameter name (may be truncated)
     @param def pointer to default parameter value
     @return error condition and copies value field to *def if found

     @author MRP, April 1998 
     @see GDS Parameter File API and loadParamSectionEntry
*********************************************************************/	
   int
   loadStringParam (const char *filename, const char *sec_name,
                   const char *par_name, char *def);

/** Maximum length of parameter file lines.
    
    @author MRP, March 1998 
    @see GDS Parameter File API
*********************************************************************/
#define PARAM_ENTRY_LEN	128

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* _GDS_PARAM_H	*/
