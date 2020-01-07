#ifndef _LIGO_MMAP_HH
#define _LIGO_MMAP_HH
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: mmap							*/
/*                                                         		*/
/* Module Description: Class for supporting memory mapped files		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 27Nov01  D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: mmap.html						*/
/*	References: none						*/
/*                                                         		*/
/* Author Information:							*/
/* Name          Telephone       Fax             e-mail 		*/
/* Daniel Sigg   (509) 372-8132  (509) 372-8137  sigg_d@ligo.mit.edu	*/
/*                                                         		*/
/*                                                         		*/
/*                      -------------------                             */
/*                                                         		*/
/*                             LIGO					*/
/*                                                         		*/
/*        THE LASER INTERFEROMETER GRAVITATIONAL WAVE OBSERVATORY.	*/
/*                                                         		*/
/*                     (C) The LIGO Project, 1999.			*/
/*                                                         		*/
/*                                                         		*/
/* Caltech				MIT		   		*/
/* LIGO Project MS 51-33		LIGO Project NW-17 161		*/
/* Pasadena CA 91125			Cambridge MA 01239 		*/
/*                                                         		*/
/* LIGO Hanford Observatory		LIGO Livingston Observatory	*/
/* P.O. Box 1970 S9-02			19100 LIGO Lane Rd.		*/
/* Richland WA 99352			Livingston, LA 70754		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

#ifndef __CINT__
#include "PConfig.h"
#include <sys/types.h>
#ifndef __GNU_STDC_OLD
#include <ios>
#else
#include <streambuf.h>
#ifndef ios_base
#define ios_base ios
#endif
#endif

namespace gdsbase
{

    /** @name Memory mapped file class. For convenience this class also
        supports mapping arbitrary memory regions. This class cannot be
        copied. If copying is needed, use mmap_ptr.

        @memo Memory mapped file class
        @author Written November 2001 by Daniel Sigg
        @version 1.0
     ************************************************************************/
    class mmap
    {
        mmap( const mmap& );
        mmap& operator=( const mmap& );

    public:
        /// Pointer type
        typedef void* pointer_type;
        /// Const pointer type
        typedef const void* const_pointer_type;
        /// Size type
        typedef size_t size_type;

        /** Creates a NULL pointer.
            @memo Default constructor
         ******************************************************************/
        mmap( );
        /** Creates an pointer and mappes it to a file.
            @memo Constructor
         ******************************************************************/
        mmap( const char*             filename,
              std::ios_base::openmode which = std::ios_base::in |
                  std::ios_base::out );
        /** Creates an pointer and mappes it to a memory region.
            Does not own the data!
            @memo Constructor
         ******************************************************************/
        mmap( pointer_type p, size_type len );
        /** Destroys the pointer.
            @memo Destructor
         ******************************************************************/
        ~mmap( );

        /** Conversion to void*
            @memo Conversion to void*
         ******************************************************************/
        operator void*( )
        {
            return (void*)fData;
        }
        /** Conversion to bool. True if mapped.
            @memo Conversion to bool
         ******************************************************************/
        bool operator!( ) const
        {
            return ( fData == 0 );
        }
        /** True if mapping is a file.
            @memo File mapping?
         ******************************************************************/
        bool
        filemap( )
        {
            return fFileMap;
        }

        /** Get a pointer.
            @memo Get
         ******************************************************************/
        pointer_type
        get( )
        {
            return fData;
        }
        /** Get a pointer.
            @memo Get
         ******************************************************************/
        const_pointer_type
        get( ) const
        {
            return fData;
        }
        /** Get length.
            @memo Get length
         ******************************************************************/
        size_type
        size( ) const
        {
            return fLength;
        }

    private:
        /// Pointer mapped to a file?
        bool fFileMap;
        /// Pointer to data
        pointer_type fData;
        /// Data length
        size_type fLength;

    public:
        /// map a file
        static bool
        map_file( const char*             filename,
                  pointer_type&           addr,
                  size_type&              len,
                  std::ios_base::openmode which = std::ios_base::in |
                      std::ios_base::out );
        /// unmap a file
        static bool unmap_file( pointer_type addr, size_type len );
    };

} // namespace gdsbase

#endif // __CINT__
#endif // _LIGO_MMAP_HH
