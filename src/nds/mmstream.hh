#ifndef _LIGO_MMSTREAMBUF_H
#define _LIGO_MMSTREAMBUF_H
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: mmstreambuf						*/
/*                                                         		*/
/* Module Description: Stream buffer with memory mapped files for IO	*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 17Apr01  D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: pipe_exec.html					*/
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
#include <iosfwd>
#ifndef __GNU_STDC_OLD
#include <ios>
#include <streambuf>
#else
#include <streambuf.h>
#include <string>
#ifndef ios_base
#define ios_base ios
#endif
#define basic_streambuf streambuf
#endif
#include "mmap_ptr.hh"

namespace std
{

    /** @name Streams that work with memory mapped files.
        This header defines streams that work with memory mapped files.
        It utilizes a stream buffer which reads and writes from memory.
        The current implementation does not expand the file when writing
        beyond the end of file. For read operations this can be used as
        a replacement for ifstream. For write oprtaions the file must
        exists; the length of the file is never changed. For both input
        and output operations, the read and write features are combined.
        Independent put and get pointers are used (as opposite to fstream).

        @memo Memory mapped file streams
        @author Written November 2001 by Daniel Sigg
        @version 1.0
     ************************************************************************/

    //@{

#ifndef __GNU_STDC_OLD
    /** Basic stream buffer for memory mapped files.

        @memo Basic memory mapped file stream buffer
     ************************************************************************/
    template < class charT, class traits = char_traits< charT > >
    class basic_mmbuf : public basic_streambuf< charT, traits >
    {
    public:
        typedef typename basic_streambuf< charT, traits >::int_type  int_type;
        typedef typename basic_streambuf< charT, traits >::off_type  off_type;
        typedef typename basic_streambuf< charT, traits >::pos_type  pos_type;
        typedef typename basic_streambuf< charT, traits >::char_type char_type;
        typedef traits traits_type;

        static int_type
        eof( )
        {
            return traits::eof( );
        }
#else
    class basic_mmbuf : public basic_streambuf
    {
    public:
        typedef char                       char_type;
        typedef int                        int_type;
        typedef streampos                  pos_type;
        typedef streamoff                  off_type;
        typedef string_char_traits< char > traits;
        static int_type
        eof( )
        {
            return EOF;
        }
#endif
        typedef gdsbase::mmap_ptr< char_type > map_pointer;

        /** Constructor.
            Create a default streambuf.
          */
        basic_mmbuf( ios_base::openmode which = ios_base::in | ios_base::out );
        /** Constructor.
            Create a streambuf from a file name. The file must be
            mappable.
          */
        basic_mmbuf( const char*        filename,
                     ios_base::openmode which = ios_base::in | ios_base::out );
        /** Constructor.
            Create a streambuf from a memory region.
          */
        basic_mmbuf( char_type*         data,
                     streamsize         len,
                     ios_base::openmode which = ios_base::in | ios_base::out );
        /** Destructor.
            Destroyes a streambuf.
          */
        virtual ~basic_mmbuf( );

        /// Is open
        bool
        is_open( ) const
        {
            return get_address( ) != 0;
        }
        /// Set the mapping
        bool set_map( const char* filename );
        /// Set the memory region
        bool set_region( char_type* data, streamsize len );
        /// close
        bool close( );

        /// Return the memory address
        char_type*
        get_address( )
        {
            return ( fMode & ios_base::in ) ? this->eback( ) : this->pbase( );
        }
        /// Return the memory address
        const char_type*
        get_address( ) const
        {
            return ( fMode & ios_base::in ) ? this->eback( ) : this->pbase( );
        }
        /// Return the data length
        streamsize
        get_length( ) const
        {
            return ( fMode & ios_base::in ) ? this->egptr( ) - this->eback( )
                                            : this->pptr( ) - this->pbase( );
        }

    private:
        map_pointer fMap;
        /// Buffer mode
        ios_base::openmode fMode;

    protected:
        /// Read one character
        virtual int_type underflow( );
        /// Write one character
        virtual int_type overflow( int_type c = eof( ) );
        /// Read multiple characters
        virtual streamsize xsgetn( char_type* s, streamsize n );
        /// Write multiple characters
        virtual streamsize xsputn( const char_type* s, streamsize num );
        /// Seek with offset
        virtual pos_type seekoff( off_type           off,
                                  ios_base::seekdir  way,
                                  ios_base::openmode which = ios_base::in |
                                      ios_base::out );
        /// Seek with position
        virtual pos_type seekpos( pos_type           sp,
                                  ios_base::openmode which = ios_base::in |
                                      ios_base::out );
    };

#ifndef __GNU_STDC_OLD
    /** Stream buffer with memory mapped file (char).

        @memo Stream buffer with memory mapped file (char)
     ************************************************************************/
    typedef basic_mmbuf< char > mm_streambuf;
#else
    typedef basic_mmbuf mm_streambuf;
#endif

#ifndef __GNU_STDC_OLD
/** Stream buffer with memory mapped file (wchar_t).

    @memo Stream buffer with memory mapped file (wchar_t)
 ************************************************************************/
#ifndef __CINT__
    typedef basic_mmbuf< wchar_t > mm_wstreambuf;
#endif
#endif

#ifndef __GNU_STDC_OLD
    /** Basic input stream for memory mapped files.

        @memo Basic input stream buffer for memory mapped files
     ************************************************************************/
    template < class charT, class traits = char_traits< charT > >
    class basic_mmistream : public basic_istream< charT, traits >
    {
    public:
        typedef charT                            char_type;
        typedef basic_istream< charT, traits >   istream_type;
        typedef basic_mmbuf< charT, traits >     buffer_type;
        typedef basic_mmistream< charT, traits > mmstream_type;
#else
    class basic_mmistream : public istream
    {
    public:
        typedef char            char_type;
        typedef istream         istream_type;
        typedef basic_mmbuf     buffer_type;
        typedef basic_mmistream mmstream_type;
#endif
        /** Default constructor.
         */
        basic_mmistream( ) : istream_type( &fBuf ), fBuf( ios_base::in )
        {
            init( &fBuf );
        }
        /** Constructor.
            Create a streambuf from a file name. The file must be
            mappable.
          */
        explicit basic_mmistream( const char* filename )
            : istream_type( &fBuf ), fBuf( filename, ios_base::in )
        {
            init( &fBuf );
            if ( !fBuf.is_open( ) )
                this->setstate( ios_base::failbit );
        }
        /** Constructor.
            Create a streambuf from a memory region.
          */
        explicit basic_mmistream( char_type* data, streamsize len )
            : istream_type( &fBuf ), fBuf( data, len, ios_base::in )
        {
            init( &fBuf );
            if ( !fBuf.is_open( ) )
                this->setstate( ios_base::failbit );
        }

        /// Buffer
        buffer_type*
        rdbuf( )
        {
            return &fBuf;
        }
        /// Buffer
        const buffer_type*
        rdbuf( ) const
        {
            return &fBuf;
        }
        /// Is open?
        bool
        is_open( ) const
        {
            return fBuf.is_open( );
        }
        /// Open
        void
        open( const char* filename )
        {
            this->clear( );
            fBuf.set_map( filename );
            if ( !fBuf.is_open( ) )
                this->setstate( ios_base::failbit );
        }
        /// Open
        void
        open( char_type* data, streamsize len )
        {
            this->clear( );
            fBuf.set_region( data, len );
            if ( !fBuf.is_open( ) )
                this->setstate( ios_base::failbit );
        }
        /// Close
        void
        close( )
        {
            if ( !fBuf.close( ) )
                this->setstate( ios_base::failbit );
            ;
        }
        /// Block read
        mmstream_type&
        read( char_type* s, streamsize n )
        {
            if ( !s || fBuf.sgetn( s, n ) != n )
                this->setstate( ios_base::failbit | ios_base::badbit );
            return *this;
        }

    private:
        /// Stream buffer
        buffer_type fBuf;
    };

#ifndef __GNU_STDC_OLD
    /** Basic output stream for memory mapped files.

        @memo Basic output stream buffer for memory mapped files
     ************************************************************************/
    template < class charT, class traits = char_traits< charT > >
    class basic_mmostream : public basic_ostream< charT, traits >
    {
    public:
        typedef charT                            char_type;
        typedef basic_ostream< charT, traits >   ostream_type;
        typedef basic_mmbuf< charT, traits >     buffer_type;
        typedef basic_mmostream< charT, traits > mmstream_type;
#else
    class basic_mmostream : public ostream
    {
    public:
        typedef char            char_type;
        typedef ostream         ostream_type;
        typedef basic_mmbuf     buffer_type;
        typedef basic_mmostream mmstream_type;
#endif
        /** Default constructor.
         */
        basic_mmostream( ) : ostream_type( &fBuf ), fBuf( ios_base::out )
        {
            init( &fBuf );
        }
        /** Constructor.
            Create a streambuf from a file name. The file must be
            mappable.
          */
        explicit basic_mmostream( const char* filename )
            : ostream_type( &fBuf ), fBuf( filename, ios_base::out )
        {
            init( &fBuf );
            if ( !fBuf.is_open( ) )
                this->setstate( ios_base::failbit );
        }
        /** Constructor.
            Create a streambuf from a memory region.
          */
        explicit basic_mmostream( char_type* data, streamsize len )
            : ostream_type( &fBuf ), fBuf( data, len, ios_base::out )
        {
            init( &fBuf );
            if ( !fBuf.is_open( ) )
                this->setstate( ios_base::failbit );
        }

        /// Buffer
        buffer_type*
        rdbuf( )
        {
            return &fBuf;
        }
        /// Buffer
        const buffer_type*
        rdbuf( ) const
        {
            return &fBuf;
        }
        /// Is open?
        bool
        is_open( ) const
        {
            return fBuf.is_open( );
        }
        /// Open
        void
        open( const char* filename )
        {
            this->clear( );
            fBuf.set_map( filename );
            if ( !fBuf.is_open( ) )
                this->setstate( ios_base::failbit );
        }
        /// Open
        void
        open( char_type* data, streamsize len )
        {
            this->clear( );
            fBuf.set_region( data, len );
            if ( !fBuf.is_open( ) )
                this->setstate( ios_base::failbit );
        }
        /// Close
        void
        close( )
        {
            if ( !fBuf.close( ) )
                this->setstate( ios_base::failbit );
            ;
        }
        /// Block write
        mmstream_type&
        write( const char_type* s, streamsize n )
        {
            if ( !s || fBuf.sputn( s, n ) != n )
                this->setstate( ios_base::failbit | ios_base::badbit );
            return *this;
        }

    private:
        /// Stream buffer
        buffer_type fBuf;
    };

#ifndef __GNU_STDC_OLD
    /** Basic IO stream for memory mapped files.

        @memo Basic IO stream buffer for memory mapped files
     ************************************************************************/
    template < class charT, class traits = char_traits< charT > >
    class basic_mmstream : public basic_iostream< charT, traits >
    {
    public:
        typedef charT                           char_type;
        typedef basic_iostream< charT, traits > stream_type;
        typedef basic_mmbuf< charT, traits >    buffer_type;
        typedef basic_mmstream< charT, traits > mmstream_type;
#else
    class basic_mmstream : public iostream
    {
    public:
        typedef char           char_type;
        typedef iostream       stream_type;
        typedef basic_mmbuf    buffer_type;
        typedef basic_mmstream mmstream_type;
#endif
        /** Default constructor.
         */
        basic_mmstream( )
            : stream_type( &fBuf ), fBuf( ios_base::in | ios_base::out )
        {
            init( &fBuf );
        }
        /** Constructor.
            Create a streambuf from a file name. The file must be
            mappable.
          */
        explicit basic_mmstream( const char* filename )
            : stream_type( &fBuf ),
              fBuf( filename, ios_base::in | ios_base::out )
        {
            init( &fBuf );
            if ( !fBuf.is_open( ) )
                this->setstate( ios_base::failbit );
        }
        /** Constructor.
            Create a streambuf from a memory region.
          */
        explicit basic_mmstream( char_type* data, streamsize len )
            : stream_type( &fBuf ),
              fBuf( data, len, ios_base::in | ios_base::out )
        {
            init( &fBuf );
            if ( !fBuf.is_open( ) )
                this->setstate( ios_base::failbit );
        }

        /// Buffer
        buffer_type*
        rdbuf( )
        {
            return &fBuf;
        }
        /// Buffer
        const buffer_type*
        rdbuf( ) const
        {
            return &fBuf;
        }
        /// Is open?
        bool
        is_open( ) const
        {
            return fBuf.is_open( );
        }
        /// Open
        void
        open( const char* filename )
        {
            this->clear( );
            fBuf.set_map( filename );
            if ( !fBuf.is_open( ) )
                this->setstate( ios_base::failbit );
        }
        /// Open
        void
        open( char_type* data, streamsize len )
        {
            this->clear( );
            fBuf.set_region( data, len );
            if ( !fBuf.is_open( ) )
                this->setstate( ios_base::failbit );
        }
        /// Close
        void
        close( )
        {
            if ( !fBuf.close( ) )
                this->setstate( ios_base::failbit );
            ;
        }
        /// Block read
        mmstream_type&
        read( char_type* s, streamsize n )
        {
            if ( !s || ( fBuf.sgetn( s, n ) != n ) )
                this->setstate( ios_base::failbit | ios_base::badbit );
            return *this;
        }
        /// Block write
        mmstream_type&
        write( const char_type* s, streamsize n )
        {
            if ( !s || ( fBuf.sputn( s, n ) != n ) )
                this->setstate( ios_base::failbit | ios_base::badbit );
            return *this;
        }

    private:
        /// Stream buffer
        buffer_type fBuf;
    };

#ifndef __GNU_STDC_OLD
    /** Input stream with file descriptors (char).

        @memo Input stream buffer with file descriptors (char)
     ************************************************************************/
    typedef basic_mmistream< char > mm_istream;
#else
    typedef basic_mmistream mm_istream;
#endif

#ifndef __GNU_STDC_OLD
/** Input stream with file descriptors (wchar_t).

    @memo Input stream buffer with file descriptors (wchar_t)
 ************************************************************************/
#ifndef __CINT__
    typedef basic_mmistream< wchar_t > mm_wistream;
#endif
#endif

#ifndef __GNU_STDC_OLD
    /** Output stream with file descriptors (char).

        @memo Output stream buffer with file descriptors (char)
     ************************************************************************/
    typedef basic_mmostream< char > mm_ostream;
#else
    typedef basic_mmostream mm_ostream;
#endif

#ifndef __GNU_STDC_OLD
/** Output stream with file descriptors (wchar_t).

    @memo Output stream buffer with file descriptors (wchar_t)
 ************************************************************************/
#ifndef __CINT__
    typedef basic_mmostream< wchar_t > mm_wostream;
#endif
#endif

#ifndef __GNU_STDC_OLD
    /** IO stream with file descriptors (char).

        @memo IO stream buffer with file descriptors (char)
     ************************************************************************/
    typedef basic_mmstream< char > mm_stream;
#else
    typedef basic_mmstream  mm_stream;
#endif

#ifndef __GNU_STDC_OLD
/** IO stream with file descriptors (wchar_t).

    @memo IO stream buffer with file descriptors (wchar_t)
 ************************************************************************/
#ifndef __CINT__
    typedef basic_mmstream< wchar_t > mm_wstream;
#endif
#endif
    //@}

    //______________________________________________________________________________
#ifndef __GNU_STDC_OLD
    template < class charT, class traits >
    basic_mmbuf< charT, traits >::basic_mmbuf( ios_base::openmode which )
#else
    inline basic_mmbuf::basic_mmbuf( ios_base::openmode which )
#endif
        : fMode( which )
    {
        this->setp( 0, 0 );
        this->setg( 0, 0, 0 );
    }

    //______________________________________________________________________________
#ifndef __GNU_STDC_OLD
    template < class charT, class traits >
    basic_mmbuf< charT, traits >::basic_mmbuf( const char*        filename,
                                               ios_base::openmode which )
#else
    inline basic_mmbuf::basic_mmbuf( const char*        filename,
                                     ios_base::openmode which )
#endif
        : fMode( which )
    {
        this->setp( 0, 0 );
        this->setg( 0, 0, 0 );
        set_map( filename );
    }

    //______________________________________________________________________________
#ifndef __GNU_STDC_OLD
    template < class charT, class traits >
    basic_mmbuf< charT, traits >::basic_mmbuf( char_type*         data,
                                               streamsize         len,
                                               ios_base::openmode which )
#else
    inline basic_mmbuf::basic_mmbuf( char_type*         data,
                                     streamsize         len,
                                     ios_base::openmode which )
#endif
        : fMode( which )
    {
        this->setp( 0, 0 );
        this->setg( 0, 0, 0 );
        set_region( data, len );
    }

    //______________________________________________________________________________
#ifndef __GNU_STDC_OLD
    template < class charT, class traits >
    basic_mmbuf< charT, traits >::~basic_mmbuf( )
#else
    inline basic_mmbuf::~basic_mmbuf( )
#endif
    {
        close( );
    }

    //______________________________________________________________________________
#ifndef __GNU_STDC_OLD
    template < class charT, class traits >
    bool
    basic_mmbuf< charT, traits >::set_map( const char* filename )
#else
    inline bool
    basic_mmbuf::set_map( const char* filename )
#endif
    {
        // cerr << "set map " << filename << endl;
        if ( !filename )
        {
            return close( );
        }
        fMap = map_pointer( filename, fMode );
        if ( fMap.is_mapped( ) )
        {
            char_type* data = fMap.get( );
            streamsize len = fMap.size( );
            if ( fMode & ios_base::in )
                this->setg( data, data, data + len );
            if ( fMode & ios_base::out )
                this->setp( data, data + len );
            return true;
        }
        else
        {
            return false;
        }
    }

    //______________________________________________________________________________
#ifndef __GNU_STDC_OLD
    template < class charT, class traits >
    bool
    basic_mmbuf< charT, traits >::set_region( char_type* data, streamsize len )
#else
    inline bool
    basic_mmbuf::set_region( char_type* data, streamsize len )
#endif
    {
        // cerr << "set region " << (void*)data << " " << len << endl;
        if ( !data && !len && is_open( ) )
        {
            return close( );
        }
        else if ( data && len )
        {
            fMap = map_pointer( data, len );
            if ( fMap.is_mapped( ) )
            {
                if ( fMode & ios_base::in )
                    this->setg( data, data, data + len );
                if ( fMode & ios_base::out )
                    this->setp( data, data + len );
            }
#ifdef __SUNPRO_CC
            mode_ = fMode;
#endif
            return true;
        }
        else
        {
            return false;
        }
    }

    //______________________________________________________________________________
#ifndef __GNU_STDC_OLD
    template < class charT, class traits >
    bool
    basic_mmbuf< charT, traits >::close( )
#else
    inline bool
    basic_mmbuf::close( )
#endif
    {
        // cerr << "called close" << endl;
        if ( is_open( ) )
        {
            fMap.reset( );
            this->setp( 0, 0 );
            this->setg( 0, 0, 0 );
            return true;
        }
        else
        {
            return false;
        }
    }

    //______________________________________________________________________________
#ifndef __GNU_STDC_OLD
    template < class charT, class traits >
    typename basic_mmbuf< charT, traits >::int_type
    basic_mmbuf< charT, traits >::underflow( )
#else
    inline basic_mmbuf::int_type
    basic_mmbuf::underflow( )
#endif
    {
        // cerr << "called underflow" << endl;
        return eof( ); // cannot read beyond end!
    }

    //______________________________________________________________________________
#ifndef __GNU_STDC_OLD
    template < class charT, class traits >
    typename basic_mmbuf< charT, traits >::int_type
    basic_mmbuf< charT, traits >::overflow( int_type c )
#else
    inline basic_mmbuf::int_type
    basic_mmbuf::overflow( int_type c )
#endif
    {
        // cerr << "called overflow" << endl;
        return eof( ); // cannot write beyond end!
    }

    //______________________________________________________________________________
#ifndef __GNU_STDC_OLD
    template < class charT, class traits >
    streamsize
    basic_mmbuf< charT, traits >::xsgetn( char_type* s, streamsize num )
#else
    inline streamsize
    basic_mmbuf::xsgetn( char_type* s, streamsize num )
#endif
    {
        streamsize left = this->egptr( ) - this->gptr( );
        if ( left < num )
            num = left;
        if ( num )
        {
            traits::copy( s, this->gptr( ), num );
            // cerr << "xsgetn::gptr() = " << int(gptr()-eback()) << endl;
            // cerr << "xsgetn::num = " << num << endl;
            this->gbump( num );
            // cerr << "xsgetn::gptr() = " << int(gptr()-eback()) << endl;
        }
        return num;
    }

    //______________________________________________________________________________
#ifndef __GNU_STDC_OLD
    template < class charT, class traits >
    streamsize
    basic_mmbuf< charT, traits >::xsputn( const char_type* s, streamsize num )
#else
    inline streamsize
    basic_mmbuf::xsputn( const char_type* s, streamsize num )
#endif
    {
        streamsize left = this->epptr( ) - this->pptr( );
        if ( left < num )
            num = left;
        if ( num )
        {
            traits::copy( this->pptr( ), s, num );
            // cerr << "xsputn::pptr() = " << int(pptr()-pbase()) << endl;
            // cerr << "xsputn::num = " << num << endl;
            this->pbump( num );
            // cerr << "xsuttn::pptr() = " << int(pptr()-pbase()) << endl;
        }
        return num;
    }

    //______________________________________________________________________________
#ifndef __GNU_STDC_OLD
    template < class charT, class traits >
    typename basic_mmbuf< charT, traits >::pos_type
    basic_mmbuf< charT, traits >::seekoff( off_type           off,
                                           ios_base::seekdir  way,
                                           ios_base::openmode which )
#else
    inline basic_mmbuf::pos_type
    basic_mmbuf::seekoff( off_type           off,
                          ios_base::seekdir  way,
                          ios_base::openmode which )
#endif
    {
        // std::cerr << "off=" << off << " way=" << (int)way << " which=" <<
        // (int)which << endl;
        streamsize newoff = 0;
        if ( which & ios_base::in )
        {
            if ( way == ios_base::beg )
                newoff = 0;
            if ( way == ios_base::cur )
                newoff = this->gptr( ) - this->eback( );
            if ( way == ios_base::end )
                newoff = this->egptr( ) - this->eback( );
        }
        else if ( which & ios_base::out )
        {
            if ( way == ios_base::beg )
                newoff = 0;
            if ( way == ios_base::cur )
                newoff = this->pptr( ) - this->pbase( );
            if ( way == ios_base::end )
                newoff = this->epptr( ) - this->pbase( );
        }
        else
        {
            return pos_type( off_type( -1 ) );
        }
        return seekpos( newoff + off, which );
    }

    //______________________________________________________________________________
#ifndef __GNU_STDC_OLD
    template < class charT, class traits >
    typename basic_mmbuf< charT, traits >::pos_type
    basic_mmbuf< charT, traits >::seekpos( pos_type           sp,
                                           ios_base::openmode which )
#else
    inline basic_mmbuf::pos_type
    basic_mmbuf::seekpos( pos_type sp, ios_base::openmode which )
#endif
    {
        // std::cerr << "sp=" << sp << " which=" << (int)which << endl;
        if ( ( sp < 0 ) )
        {
            return pos_type( off_type( -1 ) );
        }
        if ( which & ios_base::in )
        {
            if ( sp > ( this->egptr( ) - this->eback( ) ) )
            {
                return pos_type( off_type( -1 ) );
            }
            // cerr << "gbump=" << ((int)sp - int(gptr() - eback())) <<
            // " " << int(gptr() - eback()) << endl;
            this->gbump( (int)sp - int( this->gptr( ) - this->eback( ) ) );
            // cerr << "gbump2=" << int(gptr() - eback()) << endl;
        }
        if ( which & ios_base::out )
        {
            if ( sp > ( this->epptr( ) - this->pbase( ) ) )
            {
                return pos_type( off_type( -1 ) );
            }
            // cerr << "pbump=" << ((int)sp - int(pptr() - pbase())) <<
            // " " << int(pptr() - pbase()) << " " << mode_ << endl;
            this->pbump( (int)sp - int( this->pptr( ) - this->pbase( ) ) );
        }
        return sp;
    }

#ifdef __GNU_STDC_OLD
#ifndef _LIGO_MMSTREAMBUF_CC
#undef basic_streambuf
#endif

#endif // __GNU_STDC_OLD

} // namespace std

#endif // __CINT__
#endif // _LIGO_MMSTREAMBUF_H
