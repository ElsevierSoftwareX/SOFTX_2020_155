#ifndef PRODUCER_HH
#define PRODUCER_HH

#include <stats/stats.hh>
#include <iterator>
#include <memory>
#include "work_queue.hh"

#include "daq_core.h"

/// dcu_move_address is a list of start points in a move buffer for each
/// possible dcu it is provided as a helper for producer type systems which
/// need to deal with an internal move buffer.
struct dcu_move_address
{
    dcu_move_address( )
    {
        memset( &start[ 0 ], 0, sizeof( char* ) * DCU_COUNT );
    }
    unsigned char* start[ DCU_COUNT ];
};

/// Data producer thread
class producer : public stats
{
private:
    const int pnum;
    int       bytes;
    time_t    period;

    int             pvec_len;
    struct put_pvec pvec[ MAX_CHANNELS ];

    const static int PRODUCER_WORK_QUEUES = 2;
    const static int PRODUCER_WORK_QUEUE_BUF_COUNT = 5;
    const static int PRODUCER_WORK_QUEUE_START = 0;
    const static int RECV_THREAD_INPUT = 0;
    const static int RECV_THREAD_OUTPUT = 1;
    const static int CRC_THREAD_INPUT = 1;
    const static int CRC_THREAD_OUTPUT = 0;

    struct producer_buf
    {
        struct put_dpvec         vmic_pv[ MAX_CHANNELS ]{};
        int                      vmic_pv_len{ 0 };
        unsigned char*           move_buf{ nullptr };
        circ_buffer_block_prop_t prop{};
        // unsigned int gps{0};
        // unsigned int gps_n{0};
        // unsigned int seq{0};
        // int length{0};
        dcu_move_address dcu_move_addresses{};
    };

    using work_queue_t =
        work_queue::work_queue< producer_buf, PRODUCER_WORK_QUEUES >;
    using shared_work_queue_ptr = std::shared_ptr< work_queue_t >;

public:
    producer( int a = 0 )
        : pnum( 0 ), pvec_len( 0 ), cycle_input( 3 ), parallel( 1 )
    {
        for ( int i = 0; i < 2; i++ )
            for ( int j = 0; j < DCU_COUNT; j++ )
            {
                dcuStatus[ i ][ j ] = DAQ_STATE_FAULT;
                dcuStatCycle[ i ][ j ] = dcuLastCycle[ i ][ j ] == 0;
                dcuCycleStatus[ i ][ j ] = 0;
            }
        pthread_mutex_init( &prod_mutex, NULL );
        pthread_mutex_init( &prod_crc_mutex, NULL );
        pthread_cond_init( &prod_cond_go, NULL );
        pthread_cond_init( &prod_cond_done, NULL );
        pthread_cond_init( &prod_crc_cond, NULL );
        prod_go = 0;
        prod_done = 0;
    };
    void* frame_writer( );
    static void*
    frame_writer_static( void* a )
    {
        return ( (producer*)a )->frame_writer( );
    }

    void frame_writer_crc( shared_work_queue_ptr work_queue );

    void* grabIfoData( int, int, unsigned char* );
    void  grabIfoDataThread( void );
    static void*
    grabIfoData_static( void* a )
    {
        ( (producer*)a )->grabIfoDataThread( );
        return (void*)0;
    };

    pthread_t       tid;
    pthread_t       tid1; ///< Parallel producer thread
    pthread_t       debug_crc_tid; ///< debugging thread id
    pthread_t       crc_tid; ///< crc calculating thread
    pthread_mutex_t prod_mutex;
    pthread_mutex_t prod_crc_mutex;
    pthread_cond_t  prod_cond_go;
    pthread_cond_t  prod_cond_done;
    pthread_cond_t  prod_crc_cond;
    int             prod_go;
    int             prod_done;

    int          dcuStatus[ 2 ][ DCU_COUNT ]; ///< Internal rep of DCU status
    unsigned int dcuStatCycle[ 2 ][ DCU_COUNT ]; ///< Cycle count for DCU OK
    unsigned int dcuLastCycle[ 2 ][ DCU_COUNT ];
    int dcuCycleStatus[ 2 ][ DCU_COUNT ]; ///< Status check each 1/16 sec
    int cycle_input; ///< Where to input controller cycle from: 1-5565rfm;
                     ///< 2-5579rfm
    int parallel; ///< Can we do parallel data input ?

    /// Override class stats print call
    virtual void
    print( std::ostream& os )
    {
        os << "Producer: ";
        stats::print( os );
        os << endl;
    }

    /// Override the clear from class stats
    virtual void
    clearStats( )
    {
        stats::clearStats( );
        for ( vector< class stats >::iterator i = rcvr_stats.begin( );
              i != rcvr_stats.end( );
              i++ )
            i->clearStats( );
    }

    /// Statistics for the receiver threads
    std::vector< class stats > rcvr_stats;
};

#endif
