#ifndef _LIGO_REFCOUNT_H
#define _LIGO_REFCOUNT_H
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: refcount						*/
/*                                                         		*/
/* Module Description: Helper class for reference counting		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 3Mar01   D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: refcount.html					*/
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

#include "gmutex.hh"

namespace gdsbase
{

    /** @name Helper class for reference counting.

        @memo Helper class for reference counting
        @author Written April 2001 by Daniel Sigg
        @version 1.0
     ************************************************************************/

    //@{

    /** A templated reference counter. The counter maintains a count value
        and a pointer to a parent class. The counter value is initialized
        with zero and can be increased and decreased with the corresponding
        operators. If the reference counter reaches zero, the parant class
        will be deleted. A static reference counter can not be copied.
        A static refernce counter is MT safe in the sense that it protects
        its refernce counter. However, it will not protect the parent
        class. Since the static_ref_counter is derived from the mutex
        object, it can be used to protect the parent class. To avoid
        deadlock it must not be locked during increasing or decreasing its
        count value.

        Example:
        \begin{verbatim}
        // one_resource is a sharable resource which implements
        // reference counting.
        class one_resource {
           static_ref_count<one_resource> fRef;
        public:
           one_resource() : fRef (this) {++fRef; }
           bool refInc() {retrun fRef.Increase(); }
           bool refDec() {retrun fRef.Decrease(); }
           void modify() {
              thread::semlock (fRef);  // use it as mutex
              ... }
           ...
        };
        // Since one-resource deleted itself when the refercnce count reaches
        // zero, it has to be created with the new operator. In most cases it
        // is probably better to use a dynamic refernce counter.
        \end{verbatim}

        @memo static reference counter.
     ************************************************************************/
    template < class T >
    class static_ref_counter : public thread::mutex
    {
    public:
        /// Parent class type
        typedef T class_type;

        /// Make a reference counter
        explicit static_ref_counter( T* parent = 0, bool array = false )
            : fParent( parent ), fArray( array ), fCount( 0 )
        {
        }
        /// Get the reference count
        int
        getCount( ) const
        {
            return fCount;
        }
        /// Set the reference count
        void
        setCount( int count )
        {
            fCount = count;
        }
        /// Get the parent class
        T*
        getParent( ) const
        {
            return fParent;
        }
        /// Set the parent class
        void
        setParent( T* parent, bool array = false )
        {
            fParent = parent;
            fArray = array;
        }

        /// increase the reference count (prefix)
        static_ref_counter&
        operator++( )
        {
            increase( );
            return *this;
        }
        /// increase the reference count (postfix)
        static_ref_counter&
        operator++( int )
        {
            increase( );
            return *this;
        }
        /// decrease the reference count (prefix); deletes parant if reaches
        /// zero
        static_ref_counter&
        operator--( )
        {
            decrease( );
            return *this;
        }
        /// decrease the reference count (postfix); deletes parant if reaches
        /// zero
        static_ref_counter&
        operator--( int )
        {
            decrease( );
            return *this;
        }

        /// increase (returns true if counter value is positive)
        bool
        increase( )
        {
            thread::semlock( *this );
            return ( ++fCount > 0 );
        }
        /// decrease (returns true if deleted)
        bool
        decrease( )
        {
            thread::semlock( *this );
            if ( ( fCount > 0 ) && ( --fCount == 0 ) && fParent )
            {
                reset( );
                return true;
            }
            else
                return false;
        }

        /// dereference operator (returns parent)
        T& operator*( ) const
        {
            return *fParent;
        }
        /// member access operator (returns parent)
        T* operator->( ) const
        {
            return fParent;
        }

    private:
        /// Parent class
        T* fParent;
        /// Parent is array?
        bool fArray;
        /// Counter
        int fCount;

        /// deletes the parent object
        void
        reset( )
        {
            if ( fArray )
                delete[] fParent;
            else
                delete fParent;
            fParent = 0;
        }

        /// No copy constructor
        static_ref_counter( const static_ref_counter& );
        /// No assignment operator
        static_ref_counter& operator=( const static_ref_counter& );
    };

    /** A templated reference counter. This counter maintains a pointer to
        a static reference counter. The counter value is initialized
        with one and is increased everytime the counter is copied and
        decreased everytime the counter is destroyed.

        Example:
        \begin{verbatim}
        // class which manages a shared floating point array
        class container {
           float* fData; // must be before reference counter!
           dynamic_ref_counter<float> fRef;
        public:
           container (int size)
            : fData (new float[size]), fRef (fData, true) {}
           ...
        };
        // This is all what is needed to implement reference counting on
        // the data array. The user does not have to implement a
        // destructor, a copy costructor nor assignment operator.
        // Because of the reference counter, the above example class
        // implements a "shared" copy paradigm and destroyes the
        // data automatically when no longer needed.
        \end{verbatim}

        @memo dynamic reference counter.
     ************************************************************************/
    template < class T >
    class dynamic_ref_counter
    {
    public:
        /// Parent class type
        typedef T class_type;
        /// Static reference counter
        typedef static_ref_counter< T > counter_type;

        /// Get the reference count
        int
        getCount( ) const
        {
            return ( fCounter ) ? fCounter->getCount( ) : -1;
        }
        // Set the reference count
        // void setCount (int count) {
        // if (fCounter) fCounter->setCount (count); }
        /// Get the parent class
        T*
        getParent( ) const
        {
            return ( fCounter ) ? fCounter->getParent( ) : 0;
        }
        /// Set the parent class
        void
        setParent( T* parent, bool array = false )
        {
            if ( fCounter )
                fCounter->setParent( parent, array );
        }

        /// Constructor
        explicit dynamic_ref_counter( T* parent = 0, bool array = false )
            : fCounter( new counter_type( parent, array ) )
        {
            fCounter->increase( );
        }

        /// Destructor
        ~dynamic_ref_counter( )
        {
            if ( fCounter && fCounter->decrease( ) )
                delete fCounter;
        }
        /// Copy constructor
        dynamic_ref_counter( const dynamic_ref_counter& dref ) : fCounter( 0 )
        {
            *this = dref;
        }
        /// Copy constructor (from a static reference counter)
        dynamic_ref_counter( counter_type* sref ) : fCounter( sref )
        {
            if ( fCounter )
                fCounter->increase( );
        }
        /// Assignment operator
        dynamic_ref_counter&
        operator=( const dynamic_ref_counter& dref )
        {
            if ( this != &dref )
            {
                if ( fCounter && fCounter->decrease( ) )
                    delete fCounter;
                fCounter = dref.fCounter;
                if ( fCounter )
                    fCounter->increase( );
            }
            return *this;
        }

        /// Assignment operator (from a static reference counter)
        dynamic_ref_counter&
        operator=( counter_type* sref )
        {
            if ( fCounter && fCounter->decrease( ) )
                delete fCounter;
            fCounter = sref;
            if ( fCounter )
                fCounter->increase( );
            return *this;
        }
        /// dereference operator (returns parent)
        T& operator*( ) const
        {
            return *fCounter->getParent( );
        }
        /// member access operator (returns parent)
        T* operator->( ) const
        {
            return fCounter->getParent( );
        }

    private:
        /// Pointer to static reference counter
        counter_type* fCounter;
    };

    //@}
} // namespace gdsbase

#endif // _LIGO_REFCOUNT_H
