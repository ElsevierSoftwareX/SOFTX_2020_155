//
// Created by jonathan.hanks on 10/10/19.
//

#ifndef DAQD_MBUF_DECODERS_HH
#define DAQD_MBUF_DECODERS_HH

#include <cstddef>
#include <ostream>
#include <string>
#include <vector>

#include <tr1/memory>

class Decoder
{
public:
    Decoder( )
    {
    }
    virtual ~Decoder( ){};

    virtual volatile void* decode( volatile void* data,
                                   std::ostream&  os ) const = 0;
    virtual std::size_t    required_size( ) const = 0;

    virtual std::string describe( ) const = 0;
};

class DataDecoder
{
public:
    DataDecoder( ) : offset_( 0 ), decoders_( )
    {
    }
    DataDecoder( std::size_t offset, const std::string& format );
    DataDecoder( const DataDecoder& other );
    DataDecoder& operator=( const DataDecoder& other );

    std::size_t
    offset( ) const
    {
        return offset_;
    }
    std::size_t required_size( ) const;

    void decode( volatile void* data, std::ostream& os ) const;

private:
    std::size_t                                    offset_;
    std::vector< std::tr1::shared_ptr< Decoder > > decoders_;
};

#endif // DAQD_TRUNK_MBUF_DECODERS_HH
