#ifndef _LIGO_REF_PTR_H
#define _LIGO_REF_PTR_H
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: ref_ptr							*/
/*                                                         		*/
/* Module Description: Smart pointer supporting reference counting	*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 27Nov01  D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: ref_ptr.html						*/
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

#include "refcount/refcount.hh"

#ifndef __CINT__

namespace gdsbase
{

    /** @name Smart pointer which implements reference counting.
        This pointer can be copied and used as an object for standard
        containers. This smart pointer adopts the object and will free
        it when the last pointer to it gets destroyed.

        @memo Smart pointer with reference counting
        @author Written November 2001 by Daniel Sigg
        @version 1.0
     ************************************************************************/
    template < class T >
    class ref_ptr
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
        /// Reference counter
        typedef dynamic_ref_counter< T > ref_counter;

        /** Creates a NULL pointer.
            @memo Default constructor
         ******************************************************************/
        ref_ptr( ) : fPtr( 0 )
        {
        }
        /** Creates an pointer and adopts the object.
            @memo Constructor
            @param obj Object to be adopted
            @param array Set true if array delete has to be used
         ******************************************************************/
        explicit ref_ptr( T* obj, bool array = false ) : fPtr( 0 )
        {
            reset( obj, array );
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
            return fPtr;
        }
        /** Get a pointer.
            @memo Get
         ******************************************************************/
        const_pointer_type
        get( ) const
        {
            return fPtr;
        }
        /** Set pointer.
            @memo Reset
         ******************************************************************/
        void
        reset( T* obj = 0, bool array = false )
        {
            fPtr = obj;
            fCount = ref_counter( obj, array );
        }

    private:
        /// Pointer to object
        T* fPtr;
        /// Reference count
        ref_counter fCount;
    };

} // namespace gdsbase

#endif // __CINT__
#endif // _LIGO_REF_PTR_H
