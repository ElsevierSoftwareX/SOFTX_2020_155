#ifndef _LIGO_MMAP_PTR_H
#define _LIGO_MMAP_PTR_H
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: mmap_ptr						*/
/*                                                         		*/
/* Module Description: Smart pointer supporting memory mapped files	*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 27Nov01  D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: mmap_ptr.html					*/
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
#include "mmap.hh"
#include "refcount.hh"

namespace gdsbase
{

    /** @name Smart pointer for memory mapped files.
        This pointer can be copied. For convenience this pointer also
        supports mapping arbitrary memory regions.

        @memo Smart pointer for memory mapped files
        @author Written November 2001 by Daniel Sigg
        @version 1.0
     ************************************************************************/
    template < class T >
    class mmap_ptr
    {
    public:
        /// Value type
        typedef T value_type;
        /// Pointer type
        typedef T* pointer_type;
        /// Const pointer type
        typedef const T* const_pointer_type;
        /// Reference type
        typedef T& reference_type;
        /// Const reference type
        typedef const T& const_reference_type;
        /// Size type
        typedef mmap::size_type size_type;
        /// Reference counter
        typedef dynamic_ref_counter< mmap > ref_counter;

        /** Creates a NULL pointer.
            @memo Default constructor
         ******************************************************************/
        mmap_ptr( ) : fMmap( 0 )
        {
        }
        /** Creates an pointer and mappes it to a file.
            @memo Constructor
         ******************************************************************/
        mmap_ptr( const char*             filename,
                  std::ios_base::openmode which = std::ios_base::in |
                      std::ios_base::out )
            : fMmap( 0 )
        {
            set( filename, which );
        }
        /** Creates an pointer and mappes it to a memory region.
            Does not own the data!
            @memo Constructor
         ******************************************************************/
        mmap_ptr( pointer_type p, size_type len ) : fMmap( 0 )
        {
            set( p, len );
        }

        /** Dereference operator.
            @memo Dereference oprator
         ******************************************************************/
        reference_type operator*( )
        {
            return *get( );
        }
        /** Dereference operator.
            @memo Dereference oprator
         ******************************************************************/
        const_reference_type operator*( ) const
        {
            return *get( );
        }
        /** Member access oprator.
            @memo Member access oprator
         ******************************************************************/
        pointer_type operator->( )
        {
            return get( );
        }
        /** Member access oprator.
            @memo Member access oprator
         ******************************************************************/
        const_pointer_type operator->( ) const
        {
            return get( );
        }
        /** Get a pointer.
            @memo Get
         ******************************************************************/
        pointer_type
        get( )
        {
            return fMmap ? (pointer_type)fMmap->get( ) : 0;
        }
        /** Get a pointer.
            @memo Get
         ******************************************************************/
        const_pointer_type
        get( ) const
        {
            return fMmap ? (const_pointer_type)fMmap->get( ) : 0;
        }
        /** Get size.
            @memo Get size
         ******************************************************************/
        size_type
        size( ) const
        {
            return fMmap ? fMmap->size( ) / element_size( ) : 0;
        }
        /** Get element size.
            @memo Get element size
         ******************************************************************/
        size_type
        element_size( ) const
        {
            return sizeof( value_type );
        }
        /** Is mapped?
            @memo Is mapped?
         ******************************************************************/
        bool
        is_mapped( ) const
        {
            return ( get( ) != 0 );
        }
        /** Set pointer to a mapped file.
            @memo Set
         ******************************************************************/
        bool set( const char*             filename,
                  std::ios_base::openmode which = std::ios_base::in |
                      std::ios_base::out );
        /** Creates an pointer and mappes it to a memory region.
            Does not own the data!
            @memo Constructor
         ******************************************************************/
        bool set( pointer_type p, size_type len );
        /** Set this pointer back to NULL.
            @memo Reset
         ******************************************************************/
        void reset( );

    private:
        mmap* fMmap;
        /// Reference count
        ref_counter fCount;
    };

    //______________________________________________________________________________
    template < class T >
    inline bool
    mmap_ptr< T >::set( const char* filename, std::ios_base::openmode which )
    {
        fMmap = new mmap( filename, which );
        fCount = ref_counter( fMmap );
        return get( ) != 0;
    }

    //______________________________________________________________________________
    template < class T >
    inline bool
    mmap_ptr< T >::set( pointer_type p, size_type len )
    {
        fMmap =
            new mmap( (gdsbase::mmap::pointer_type)p, len * element_size( ) );
        fCount = ref_counter( fMmap );
        return get( ) != 0;
    }

    //______________________________________________________________________________
    template < class T >
    inline void
    mmap_ptr< T >::reset( )
    {
        if ( fMmap )
        {
            fMmap = 0;
            fCount = ref_counter( fMmap );
        }
    }

} // namespace gdsbase

#endif // __CINT__
#endif // _LIGO_MMAP_PTR_H
