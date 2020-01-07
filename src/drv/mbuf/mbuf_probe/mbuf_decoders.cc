//
// Created by jonathan.hanks on 10/10/19.
//

#include "mbuf_decoders.hh"

#include <algorithm>
#include <numeric>
#include <iostream>
#include <sstream>

namespace mbuf_decoders
{
    template < typename T >
    class BasicDecoder : public Decoder
    {
    public:
        explicit BasicDecoder( std::size_t count ) : count_( count )
        {
        }

        virtual ~BasicDecoder( )
        {
        }

        virtual volatile void*
        decode( volatile void* data, std::ostream& os ) const
        {
            volatile T* cur = reinterpret_cast< volatile T* >( data );
            for ( std::size_t i = 0; i < count_; ++i )
            {
                os << " " << *cur;
                ++cur;
            }
            return reinterpret_cast< volatile void* >( cur );
        }

        virtual std::size_t
        required_size( ) const
        {
            return sizeof( T ) * count_;
        }

        virtual std::string
        describe( ) const
        {
            std::ostringstream os;
            os << "BasicDecoder data_size = " << sizeof( T )
               << " count = " << count_ << " req_size = " << required_size( );
            return os.str( );
        }

    private:
        std::size_t count_;
    };

    class PadDecoder : public Decoder
    {
    public:
        explicit PadDecoder( std::size_t count ) : count_( count )
        {
        }

        virtual ~PadDecoder( )
        {
        }

        virtual volatile void*
        decode( volatile void* data, std::ostream& os ) const
        {
            volatile char* cur = reinterpret_cast< volatile char* >( data );
            cur += count_;
            return reinterpret_cast< volatile void* >( cur );
        }

        virtual std::size_t
        required_size( ) const
        {
            return count_;
        }

        virtual std::string
        describe( ) const
        {
            return "pad";
        }

    private:
        std::size_t count_;
    };

    std::tr1::shared_ptr< Decoder >
    create_decoder( char format, std::size_t count = 1 )
    {
        std::tr1::shared_ptr< Decoder > ptr;

        switch ( format )
        {
        case 'x':
            ptr = std::tr1::shared_ptr< Decoder >( new PadDecoder( count ) );
            break;
        case 'c':
            ptr = std::tr1::shared_ptr< Decoder >(
                new BasicDecoder< char >( count ) );
            break;
        case 'b':
            ptr = std::tr1::shared_ptr< Decoder >(
                new BasicDecoder< signed char >( count ) );
            break;
        case 'B':
            ptr = std::tr1::shared_ptr< Decoder >(
                new BasicDecoder< unsigned char >( count ) );
            break;
        case 'h':
            ptr = std::tr1::shared_ptr< Decoder >(
                new BasicDecoder< short >( count ) );
            break;
        case 'H':
            ptr = std::tr1::shared_ptr< Decoder >(
                new BasicDecoder< unsigned short >( count ) );
            break;
        case 'i':
            ptr = std::tr1::shared_ptr< Decoder >(
                new BasicDecoder< int >( count ) );
            break;
        case 'I':
            ptr = std::tr1::shared_ptr< Decoder >(
                new BasicDecoder< unsigned int >( count ) );
            break;
        case 'f':
            ptr = std::tr1::shared_ptr< Decoder >(
                new BasicDecoder< float >( count ) );
            break;
        case 'd':
            ptr = std::tr1::shared_ptr< Decoder >(
                new BasicDecoder< double >( count ) );
            break;
        default:
            throw std::runtime_error(
                "Unknown format specifier for a decoder" );
        }
        return ptr;
    }
} // namespace mbuf_decoders

class not_equal
{
public:
    not_equal( char c ) : val( c )
    {
    }
    not_equal( const not_equal& other ) : val( other.val )
    {
    }
    not_equal&
    operator=( const not_equal& other )
    {
        val = other.val;
        return *this;
    }

    bool
    operator( )( char cur ) const
    {
        return cur != val;
    }

private:
    char val;
};

DataDecoder::DataDecoder( std::size_t offset, const std::string& format )
    : offset_( offset ), decoders_( )
{
    decoders_.reserve( format.size( ) );

    std::string::const_iterator cur = format.begin( );
    std::string::const_iterator end = format.end( );

    while ( cur != end )
    {
        std::string::const_iterator next =
            find_if( cur, end, not_equal( *cur ) );
        decoders_.push_back(
            mbuf_decoders::create_decoder( *cur, next - cur ) );
        cur = next;
    }

    std::cout << "Decoder:\n";
    for ( int i = 0; i < decoders_.size( ); ++i )
    {
        std::cout << "\t" << decoders_[ i ]->describe( ) << "\n";
    }
}

DataDecoder::DataDecoder( const DataDecoder& other )
    : offset_( other.offset_ ), decoders_( other.decoders_ )
{
}

DataDecoder&
DataDecoder::operator=( const DataDecoder& other )
{
    if ( &other != this )
    {
        decoders_ = other.decoders_;
        offset_ = other.offset_;
    }
    return *this;
}

static std::size_t
required_size_accumulator( std::size_t                     cur,
                           std::tr1::shared_ptr< Decoder > decoder )
{
    return cur + decoder->required_size( );
}

std::size_t
DataDecoder::required_size( ) const
{
    return std::accumulate( decoders_.begin( ),
                            decoders_.end( ),
                            std::size_t( 0 ),
                            required_size_accumulator );
}

void
DataDecoder::decode( volatile void* data, std::ostream& os ) const
{
    std::vector< std::size_t > steps( decoders_.size( ), 0 );
    os << "offset: " << offset_;
    volatile void* cur = reinterpret_cast< volatile void* >(
        reinterpret_cast< volatile char* >( data ) + offset_ );
    for ( std::size_t i = 0; i < decoders_.size( ); ++i )
    {
        volatile void* tmp = decoders_[ i ]->decode( cur, os );
        steps[ i ] = ( (char*)tmp ) - ( (char*)cur );
        cur = tmp;
    }

    os << " steps -";
    for ( std::size_t i = 0; i < steps.size( ); ++i )
    {
        os << " " << steps[ i ];
    }
}