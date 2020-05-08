#ifndef DAQD_PUB_SUB_ARENA_HH
#define DAQD_PUB_SUB_ARENA_HH

#include <memory>
#include <boost/lockfree/spsc_queue.hpp>
/*!
 * @brief a simple memory arena/pool interface
 */
class Arena
{
public:
    explicit Arena( int prealloc_count ) : arena_{}
    {
        int count = std::min( prealloc_count, 10 );
        for ( int i = 0; i < count; ++i )
        {
            put( new unsigned char[ sizeof( daq_dc_data_t ) ] );
        }
    }
    Arena( const Arena& ) = delete;
    Arena( Arena&& ) = delete;
    ~Arena( )
    {
        while ( arena_.read_available( ) )
        {
            unsigned char* tmp = nullptr;
            arena_.pop( tmp );
            if ( tmp )
            {
                delete[] tmp;
            }
        }
    }
    Arena& operator=( const Arena& ) = delete;
    Arena& operator=( Arena&& ) = delete;

    pub_sub::DataPtr
    get( )
    {
        unsigned char* tmp = nullptr;
        if ( !arena_.pop( tmp ) )
        {
            tmp = new unsigned char[ sizeof( daq_dc_data_t ) ];
        }
        return std::shared_ptr< unsigned char[] >( tmp,
                                                   [this]( unsigned char* p ) {
                                                       if ( p )
                                                       {
                                                           this->put( p );
                                                       }
                                                   } );
    }

    void
    put( unsigned char* p )
    {
        if ( !p )
        {
            return;
        }
        if ( !arena_.push( p ) )
        {
            delete[] p;
        }
    }

private:
    using queue_t =
    boost::lockfree::spsc_queue< unsigned char*,
        boost::lockfree::capacity< 10 > >;
    queue_t arena_;
};

#endif