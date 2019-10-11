//
// Created by jonathan.hanks on 8/22/19.
//
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <numeric>
#include <map>
#include <string>
#include <sstream>

#include "fe_stream_generator.hh"
#include "fe_generator_support.hh"
#include "../include/daqmap.h"
#include "../include/daq_core.h"

#include "gps.hh"

extern "C" {

#include "../drv/rfm.c"
}

enum MODEL_STATUS
{
    RUNNING,
    STOPPED,
};

typedef std::vector< int > failure_list;
typedef std::vector< int > testpoint_list;

struct ModelParams
{
    ModelParams( )
        : name( ), model_rate( 2048 ), dcuid( -1 ), data_rate( 10000 ),
          tp_list( ), status( MODEL_STATUS::RUNNING )
    {
    }
    std::string    name;
    int            model_rate;
    int            dcuid;
    int            data_rate;
    testpoint_list tp_list;
    MODEL_STATUS   status;
};

struct TestPointRequest
{
    TestPointRequest( ) : dcuid( 0 ), testpoints( )
    {
    }
    TestPointRequest( const TestPointRequest& other )
        : dcuid( other.dcuid ), testpoints( other.testpoints )
    {
    }

    TestPointRequest&
    operator=( const TestPointRequest& other )
    {
        dcuid = other.dcuid;
        testpoints = other.testpoints;
        return *this;
    }
    int            dcuid;
    testpoint_list testpoints;
};

struct Options
{
    Options( )
        : models( ), ini_root( ), master_path( "master" ),
          mbuf_name( "local_dc" ), mbuf_size_mb( 100 ), concentrate( true ),
          show_help( false )
    {
    }
    std::vector< ModelParams > models;
    std::string                ini_root;
    std::string                master_path;
    std::string                mbuf_name;
    size_t                     mbuf_size_mb;
    bool                       concentrate;
    bool                       show_help;
};

class GeneratorStore
{
public:
    GeneratorStore( ) : store_( )
    {
    }

    GeneratorPtr
    get( const std::string& name )
    {
        std::map< std::string, GeneratorPtr >::iterator it =
            store_.find( name );
        return ( it == store_.end( ) ? GeneratorPtr( (Generator*)0 )
                                     : it->second );
    }

    void
    set( const std::string& name, GeneratorPtr& g )
    {
        std::map< std::string, GeneratorPtr >::iterator it =
            store_.find( name );
        if ( it == store_.end( ) )
        {
            store_.insert( std::make_pair( name, g ) );
        }
    }

private:
    std::map< std::string, GeneratorPtr > store_;
};

class IniManager
{
public:
    IniManager( const std::string& ini_root, const std::string& master_path )
        : ini_root_( ini_root ), master_path_( master_path )
    {
    }

    int
    add( const std::string&          model_name,
         int                         dcu_id,
         int                         model_rate,
         std::vector< GeneratorPtr > chans,
         std::vector< GeneratorPtr > test_points )
    {
        output_ini_files(
            ini_root_, model_name, chans, test_points, dcu_id, model_rate );
        add_to_master( model_name );
        return calculate_ini_crc( ini_root_, model_name );
    }

private:
    void
    add_to_master( const std::string& model_name )
    {
        std::string name = cleaned_system_name( model_name );
        master_contents_.push_back(
            generate_ini_filename( ini_root_, model_name ) );
        master_contents_.push_back(
            generate_par_filename( ini_root_, model_name ) );

        rewrite_master( );
    }

    void
    rewrite_master( )
    {
        std::string tmp_name = master_path_;
        tmp_name += ".tmp";

        {
            std::ofstream                        out( tmp_name.c_str( ) );
            std::ostream_iterator< std::string > it( out, "\n" );
            std::copy( master_contents_.begin( ), master_contents_.end( ), it );
        }

        if ( std::rename( tmp_name.c_str( ), master_path_.c_str( ) ) == -1 )
        {
            throw std::runtime_error( "Unable to replace the master file" );
        }
    }

    std::vector< std::string > master_contents_;

    std::string ini_root_;
    std::string master_path_;
};

/**
 * @brief Represent a model (essentially a list of channels + testpoints)
 */
class Model
{
    static testpoint_list
    clean_tp_list( const testpoint_list& tp_list )
    {
        std::size_t    MAX_TP = 32;
        testpoint_list result( MAX_TP, 0 );
        std::size_t    max_index = std::min( MAX_TP, tp_list.size( ) );
        for ( std::size_t i = 0; i < max_index; ++i )
        {
            result[ i ] =
                ( tp_list[ i ] <= MAX_TP && tp_list[ i ] >= 0 ? tp_list[ i ]
                                                              : 0 );
        }
        return result;
    }

public:
    /**
     * @brief Initialize a model, generating a set of channels
     * @param name model name
     * @param dcu_id dcu
     * @param model_rate rate
     * @param data_rate_bytes The max number of bytes to use.  Should be a
     * multiple of 4 (sizeof float)
     * @param tp_list The list of active test points
     */
    Model( IniManager&           ini_manager,
           const std::string&    name,
           int                   dcu_id,
           int                   model_rate,
           int                   data_rate_bytes,
           const testpoint_list& tp_list,
           MODEL_STATUS          status )
        : name_( name ), dcu_id_( dcu_id ), model_rate_( model_rate ),
          data_rate_bytes_( data_rate_bytes ), config_crc_( 0 ),
          status_( status ), tp_table_( clean_tp_list( tp_list ) ),
          generators_( ), tp_generators_( ), null_tp_( (Generator*)0 ),
          rmipc_( (volatile char*)0 )
    {
        size_t fast_data_bytes = data_rate_bytes_ / 2;
        size_t mid_data_bytes = data_rate_bytes / 4;
        size_t slow_data_bytes = data_rate_bytes / 4;

        if ( fast_data_bytes + mid_data_bytes + slow_data_bytes !=
             data_rate_bytes_ )
        {
            std::cerr << fast_data_bytes << "\n";
            std::cerr << mid_data_bytes << "\n";
            std::cerr << slow_data_bytes << "\n";
            std::cerr << data_rate_bytes_ << "\n";
            throw std::runtime_error(
                "Fast/mid/slow mix is bad, rates do not add up" );
        }

        int mid_rate = model_rate_ / 2;

        size_t fast_channel_num =
            fast_data_bytes / ( sizeof( float ) * model_rate_ );
        size_t mid_channel_num =
            mid_data_bytes / ( sizeof( float ) * mid_rate );
        size_t slow_channel_num = slow_data_bytes / ( sizeof( float ) * 16 );
        size_t slow_16i_channel_num = 0;
        if ( slow_channel_num > 2 )
        {
            slow_channel_num -= 1;
            slow_16i_channel_num = 2;
        }
        size_t channel_num = fast_channel_num + mid_channel_num +
            slow_channel_num + slow_16i_channel_num;

        generators_.reserve( channel_num );
        tp_generators_.reserve( tp_table_.size( ) );

        size_t mid_channel_boundary = fast_channel_num + mid_channel_num;
        size_t slow_channel_boundary = mid_channel_boundary + slow_channel_num;

        ChNumDb chDb;
        ChNumDb tpDb;

        for ( size_t i = 0; i < fast_channel_num; ++i )
        {
            int chnum = chDb.next( 4 );

            std::ostringstream ss;
            ss << name_ << "-" << i;
            generators_.push_back(
                GeneratorPtr( new Generators::GPSSecondWithOffset< int >(
                    SimChannel( ss.str( ), 2, model_rate_, chnum ),
                    ( i + dcu_id_ ) % 21 ) ) );
        }
        for ( size_t i = fast_channel_num; i < mid_channel_boundary; ++i )
        {
            int chnum = chDb.next( 4 );

            std::ostringstream ss;
            ss << name_ << "-" << i;
            generators_.push_back(
                GeneratorPtr( new Generators::GPSSecondWithOffset< int >(
                    SimChannel( ss.str( ), 2, mid_rate, chnum ),
                    ( i + dcu_id_ ) % 21 ) ) );
        }
        for ( size_t i = mid_channel_boundary; i < slow_channel_boundary; ++i )
        {
            int chnum = chDb.next( 4 );

            std::ostringstream ss;
            ss << name_ << "-" << i;
            generators_.push_back( GeneratorPtr(
                new Generators::GPSMod100kSecWithOffsetAndCycle< int >(
                    SimChannel( ss.str( ), 2, 16, chnum ),
                    ( i + dcu_id_ ) % 21 ) ) );
        }
        for ( size_t i = slow_channel_boundary; i < channel_num; ++i )
        {
            int chnum = chDb.next( 4 );

            std::ostringstream ss;
            ss << name_ << "-" << i;
            generators_.push_back( GeneratorPtr(
                new Generators::GPSMod100SecWithOffsetAndCycle< short >(
                    SimChannel( ss.str( ), 1, 16, chnum ),
                    ( i + dcu_id_ ) % 21 ) ) );
        }

        for ( size_t i = 0; i < tp_table_.size( ); ++i )
        {
            int chnum = tpDb.next( 4 );

            std::ostringstream ss;
            ss << name_ << "-TP" << i;
            // TP need truncated
            tp_generators_.push_back( GeneratorPtr(
                new Generators::GPSMod100kSecWithOffsetAndCycle< float >(
                    SimChannel( ss.str( ), 4, model_rate_, chnum, dcu_id_ ),
                    ( i + dcu_id_ ) % 21 ) ) );
        }
        null_tp_ = GeneratorPtr( new Generators::StaticValue< float >(
            SimChannel( "null_tp_value", 4, model_rate_, 0x7fffffff ), 0.0 ) );
        config_crc_ = ini_manager.add(
            name, dcu_id, model_rate, generators_, tp_generators_ );
    }

    ~Model( )
    {
        close_rmpic( );
    }

    void
    open_rmpic( )
    {
        if ( rmipc_ != 0 || status_ == MODEL_STATUS::STOPPED )
        {
            return;
        }
        std::string mbuf_name = name_ + "_daq";
        rmipc_ = reinterpret_cast< volatile char* >( findSharedMemorySize(
            const_cast< char* >( mbuf_name.c_str( ) ), 64 ) );
        if ( rmipc_ == 0 )
        {
            std::cerr << "Unable to open shmem buffer\n";
            exit( 1 );
        }
        volatile struct rmIpcStr* ipc =
            reinterpret_cast< volatile struct rmIpcStr* >(
                rmipc_ + CDS_DAQ_NET_IPC_OFFSET );
        volatile struct cdsDaqNetGdsTpNum* tpData =
            reinterpret_cast< volatile struct cdsDaqNetGdsTpNum* >(
                rmipc_ + CDS_DAQ_NET_GDS_TP_TABLE_OFFSET );
        volatile char* data = rmipc_ + CDS_DAQ_NET_DATA_OFFSET;

        ipc->cycle = -1;
        ipc->dcuId = dcu_id( );
        ipc->crc = config_crc( );
        ipc->command = 0;
        ipc->cmdAck = 0;
        ipc->request = 0;
        ipc->reqAck = 0;
        ipc->status = 0xbad;
        ipc->channelCount = generators( ).size( );
        ipc->dataBlockSize =
            data_bytes_per_sec( ) / DAQ_NUM_DATA_BLOCKS_PER_SECOND;
        for ( int i = 0; i < DAQ_NUM_DATA_BLOCKS; ++i )
        {
            ipc->bp[ i ].status = 0xbad;
            ipc->bp[ i ].timeSec = 0;
            ipc->bp[ i ].timeNSec = 0;
            ipc->bp[ i ].run = 0;
            ipc->bp[ i ].cycle = 0;
            ipc->bp[ i ].crc = ipc->dataBlockSize;
        }
        tpData->count = 0;
        for ( int i = 0; i < DAQ_GDS_MAX_TP_NUM; ++i )
        {
            tpData->tpNum[ i ] = 0;
        }
    }

    void
    close_rmpic( )
    {
        if ( rmipc_ == 0 )
        {
            return;
        }
    }

    const std::string&
    name( ) const
    {
        return name_;
    }

    int
    dcu_id( ) const
    {
        return dcu_id_;
    }

    size_t
    model_rate( ) const
    {
        return model_rate_;
    }

    size_t
    chan_count( ) const
    {
        return generators_.size( );
    }

    MODEL_STATUS
    status( ) const
    {
        return status_;
    }

    std::vector< GeneratorPtr >&
    generators( )
    {
        return generators_;
    }

    std::vector< GeneratorPtr >&
    tp_generators( )
    {
        return tp_generators_;
    }

    std::size_t
    data_bytes_per_sec( )
    {
        std::size_t                           sum = 0;
        std::vector< GeneratorPtr >::iterator it;
        for ( it = generators_.begin( ); it != generators_.end( ); ++it )
        {
            Generator* p = it->get( );
            if ( !p )
            {
                continue;
            }
            sum += p->bytes_per_sec( );
        }
        return sum;
    }

    std::size_t
    active_tp( ) const
    {
        int last = -1;
        for ( int i = 0; i < tp_table_.size( ); ++i )
        {
            if ( tp_table_[ i ] > 0 )
            {
                last = i;
            }
        }
        ++last;
        return static_cast< std::size_t >( last );
    }

    std::size_t
    tp_data_bytes_per_sec( )
    {
        return model_rate( ) * sizeof( float ) * active_tp( );
    }

    int
    config_crc( ) const
    {
        return config_crc_;
    }

    volatile char*
    rmicp( ) const
    {
        return rmipc_;
    }

    const testpoint_list&
    tp_table( ) const
    {
        return tp_table_;
    }

    Generator&
    null_generator( ) const
    {
        return *null_tp_;
    }

private:
    std::string                 name_;
    int                         dcu_id_;
    size_t                      model_rate_;
    size_t                      data_rate_bytes_;
    int                         config_crc_;
    MODEL_STATUS                status_;
    testpoint_list              tp_table_;
    std::vector< GeneratorPtr > generators_;
    std::vector< GeneratorPtr > tp_generators_;
    GeneratorPtr                null_tp_;
    volatile char*              rmipc_;
};

typedef std::tr1::shared_ptr< Model > ModelPtr;

/**
 * @brief Given a set of models, and a data block, fill the data block with the
 * models data.
 * @param models Set of models
 * @param dest Where to put the data
 * @param max_data_size The data block size
 * @param cycle cycle number
 * @param cur_time gps time to generate data for
 */
void
generate_models( std::vector< ModelPtr >& models,
                 daq_dc_data_t&           dest,
                 size_t                   max_data_size,
                 int                      cycle,
                 const GPS::gps_time&     cur_time )
{
    dest.header.dcuTotalModels = models.size( );
    dest.header.fullDataBlockSize = 0;
    char* data = dest.dataBlock;
    char* data_end = data + max_data_size;
    for ( int i = 0; i < models.size( ); ++i )
    {
        char*     start = data;
        ModelPtr& mp = models[ i ];
        if ( mp->status( ) == MODEL_STATUS::STOPPED )
        {
            continue;
        }
        daq_msg_header_t& dcu_header = dest.header.dcuheader[ i ];
        dcu_header.dcuId = mp->dcu_id( );
        dcu_header.fileCrc = mp->config_crc( );
        dcu_header.status = 2;
        dcu_header.cycle = cycle;
        dcu_header.timeSec = cur_time.sec;
        dcu_header.timeNSec = cur_time.nanosec;
        dcu_header.dataCrc = 0;
        dcu_header.dataBlockSize = 0;
        dcu_header.tpBlockSize = mp->tp_data_bytes_per_sec( );
        dcu_header.tpCount = mp->active_tp( );
        std::fill( &dcu_header.tpNum[ 0 ],
                   &dcu_header.tpNum[ DAQ_GDS_MAX_TP_NUM ],
                   0 );

        std::vector< GeneratorPtr >&          gen = mp->generators( );
        std::vector< GeneratorPtr >::iterator it = gen.begin( );
        for ( ; it != gen.end( ); ++it )
        {
            char* next_data =
                ( *it )->generate( static_cast< int >( cur_time.sec ),
                                   static_cast< int >( cur_time.nanosec ),
                                   data );
            dest.header.fullDataBlockSize +=
                static_cast< int >( next_data - data );
            data = next_data;
        }
        dcu_header.dataBlockSize = static_cast< unsigned int >( data - start );
        dcu_header.dataCrc = calculate_crc( reinterpret_cast< void* >( start ),
                                            dcu_header.dataBlockSize );
        for ( int tp = 0; tp < dcu_header.tpCount; ++tp )
        {
            char* next_data = 0;
            int   tp_index = mp->tp_table( )[ tp ];
            if ( tp_index == 0 )
            {
                next_data = mp->null_generator( ).generate(
                    static_cast< int >( cur_time.sec ),
                    static_cast< int >( cur_time.nanosec ),
                    data );
            }
            else
            {
                next_data = mp->tp_generators( )[ tp_index - 1 ]->generate(
                    static_cast< int >( cur_time.sec ),
                    static_cast< int >( cur_time.nanosec ),
                    data );
            }
            data = next_data;
        }
    }
}

/*!
 * @brief generate all the models and write their data to the
 * rmipc structure (not concentrated)
 * @param models modules
 * @param cycle cycle number
 * @param transmit_time time
 */
void
generate_models_rmpic( std::vector< ModelPtr >& models,
                       int                      cycle,
                       const GPS::gps_time&     transmit_time )
{
    for ( std::vector< ModelPtr >::iterator it = models.begin( );
          it != models.end( );
          ++it )
    {
        if ( !( *it ).get( ) )
        {
            continue;
        }
        Model& cur_model = *( *it ).get( );
        if ( cur_model.status( ) == MODEL_STATUS::STOPPED )
        {
            continue;
        }
        volatile char* shmem = cur_model.rmicp( );
        if ( !shmem )
        {
            continue;
        }
        volatile struct rmIpcStr* ipc =
            reinterpret_cast< volatile struct rmIpcStr* >(
                shmem + CDS_DAQ_NET_IPC_OFFSET );
        volatile struct cdsDaqNetGdsTpNum* tpData =
            reinterpret_cast< volatile struct cdsDaqNetGdsTpNum* >(
                shmem + CDS_DAQ_NET_GDS_TP_TABLE_OFFSET );
        volatile char* data = shmem + CDS_DAQ_NET_DATA_OFFSET;

        // The block size is the maximal block size * 2 (to allow for TP data)
        volatile char* data_cur = data + cycle * ( DAQ_DCU_BLOCK_SIZE * 2 );
        volatile char* data_end = data_cur + ( DAQ_DCU_BLOCK_SIZE * 2 );

        for ( int i = 0; i < cur_model.generators( ).size( ); ++i )
        {
            data_cur = cur_model.generators( )[ i ]->generate(
                transmit_time.sec, transmit_time.nanosec, (char*)data_cur );
            if ( data_cur > data_end )
                throw std::range_error(
                    "Generator exceeded its output boundary" );
        }
        int tp_count = cur_model.active_tp( );
        for ( int i = 0; i < tp_count; ++i )
        {
            int index = cur_model.tp_table( )[ i ];
            if ( index == 0 )
            {
                data_cur = cur_model.null_generator( ).generate(
                    transmit_time.sec, transmit_time.nanosec, (char*)data_cur );
            }
            else
            {
                data_cur = cur_model.tp_generators( )[ index - 1 ]->generate(
                    transmit_time.sec, transmit_time.nanosec, (char*)data_cur );
            }
            if ( data_cur > data_end )
                throw std::range_error(
                    "Generator exceeded its output boundary" );
        }
        ipc->status = 0;
        ipc->bp[ cycle ].status = 2;
        ipc->bp[ cycle ].timeSec = transmit_time.sec;
        ipc->bp[ cycle ].timeNSec = transmit_time.nanosec;
        ipc->bp[ cycle ].cycle = cycle;

        ipc->cycle = cycle;
    }
}

/*!
 * @brief get the first count valid dcuids for models.
 * @param count Number of dcuids to return
 * @return a list of dcuids
 */
std::vector< int >
get_n_dcus( int count )
{
    std::vector< int > dcus;
    dcus.reserve( 256 );

    for ( int i = 5; i <= 12; ++i )
    {
        dcus.push_back( i );
    }
    for ( int i = 17; i < 256; ++i )
    {
        dcus.push_back( i );
    }
    if ( count > dcus.size( ) )
    {
        throw std::runtime_error( "Requested too many dcus" );
    }
    dcus.resize( count );
    return dcus;
}

int
from_string( const std::string& s )
{
    int                val = 0;
    std::istringstream os( s );
    os >> val;
    return val;
}

/*!
 * @brief parse a comma seperated list of ints
 * @param input
 * @return a list of integers
 */
std::vector< int >
parse_int_list( const std::string& input )
{
    std::string            tmp( input );
    std::string::size_type comma = tmp.find( ',' );
    while ( comma != std::string::npos )
    {
        tmp[ comma ] = ' ';
        comma = tmp.find( ',' );
    }
    std::vector< int >           results;
    std::istringstream           is( tmp );
    std::istream_iterator< int > it( is );
    std::copy(
        it, std::istream_iterator< int >( ), std::back_inserter( results ) );
    return results;
}

/*!
 * @brief parse a testpoint option requestion
 * @details Given a string of the form dcuid:num,num,num,... parse it
 * into a TestpointRequest
 * @param s The input string
 * @return A TestPointRequest, throws on error
 */
TestPointRequest
parse_testpoint_option( const char* s )
{
    TestPointRequest req;
    if ( !s )
    {
        throw std::runtime_error( "Invalid input to parse_testpoint_option" );
    }
    std::string            input( s );
    std::string::size_type split = input.find( ':' );
    if ( split < 1 || split == std::string::npos )
    {
        throw std::runtime_error( "Invalid input to parse_testpoint_option" );
    }
    std::string dcu_str = input.substr( 0, split );
    std::string tp_list = input.substr( split + 1 );
    req.dcuid = from_string( dcu_str );
    std::string::size_type prev = 0;
    std::string::size_type pos = tp_list.find( ',', prev );
    while ( pos != std::string::npos )
    {
        if ( pos == prev )
        {
            throw std::runtime_error(
                "Invalid input to parse_testpoint_option" );
        }
        req.testpoints.push_back(
            from_string( tp_list.substr( prev, pos - 1 ) ) );
        prev = pos + 1;
        pos = tp_list.find( ',', prev );
    }
    req.testpoints.push_back( from_string( tp_list.substr( prev ) ) );
    return req;
}

void
usage( const char* progname )
{
    std::cout << progname << " simulation for multiple LIGO FE computers\n";
    std::cout << "\nUsage: " << progname << " options\n";
    std::cout << "Where options are:\n";
    std::cout << "\t-i path - Path to the ini/par file folder to use (should "
                 "be a full path)\n";
    std::cout << "\t-M path - Path to the master file (will be overwritten) "
                 "[master]\n";
    std::cout << "\t-b name - Name of the output mbuf [local_dc]\n";
    std::cout << "\t-m size_mb - Size in MB of the output mbuf [100]\n";
    std::cout << "\t-k size_kb - Default data rate of each model in kB\n";
    std::cout << "\t-R num - number of models to simulate [1-120]\n";
    std::cout << "\t-t dcu:tp#,tp#,tp#,... - enable the given testpoints on "
                 "the given dcu\n";
    std::cout << "\t-f dcu,dcu,dcu,... - fail the given dcu's (write configs, "
                 "but do not run)\n";
    std::cout << "\t-S - Set if each model should write to its own mbuf rmIpc "
                 "structs\n";
    std::cout << "\t-h - this help\n";
    std::cout
        << "If -S is not specified (default) the data is concentrated into"
           "one buffer\n";
    std::cout << "-t/-f options must come after the -R call\n";
}

Options
parse_arguments( int argc, char* argv[] )
{
    Options opts;
    int     c;
    int     model_data_size = 700 * 1024;

    while ( ( c = getopt( argc, argv, "i:M:b:m:R:k:h:St:f:" ) ) != -1 )
    {
        switch ( c )
        {
        case 'i':
            opts.ini_root = optarg;
            break;
        case 'M':
            opts.master_path = optarg;
            break;
        case 'b':
            opts.mbuf_name = optarg;
            break;
        case 'm':
            opts.mbuf_size_mb = std::atoi( optarg );
            break;
        case 'k':
            model_data_size = std::atoi( optarg );
            if ( model_data_size < 10 || model_data_size > 3900 )
            {
                throw std::runtime_error(
                    "Invalid model_data_size [10-3900]k" );
            }
            model_data_size *= 1024;
            break;
        case 'R':
        {
            int count = std::atoi( optarg );
            if ( count > 120 || count < 1 )
            {
                throw std::runtime_error( "Must specify [1-128] models" );
            }
            if ( !opts.models.empty( ) )
            {
                throw std::runtime_error( "Models already specified" );
            }
            if ( model_data_size * count >=
                 ( opts.mbuf_size_mb * 1024 * 1024 ) )
            {
                throw std::runtime_error(
                    "Too much data, increase the buffer size or reduce models "
                    "or data rate" );
            }
            opts.models.reserve( count );
            std::vector< int > model_dcus = get_n_dcus( count );
            for ( int i = 0; i < model_dcus.size( ); ++i )
            {
                int         rate = 2048;
                int         data_rate = model_data_size;
                int         dcuid = model_dcus[ i ];
                ModelParams params;
                params.dcuid = dcuid;
                params.model_rate = rate;
                params.data_rate = data_rate;
                std::ostringstream os;
                os << "mod" << dcuid;
                params.name = os.str( );
                opts.models.push_back( params );
            }
        }
        break;
        case 'S':
            opts.concentrate = false;
            break;
        case 't':
            if ( opts.models.empty( ) )
            {
                throw std::runtime_error( "Please specify -R before -t" );
            }
            {
                TestPointRequest tp_req = parse_testpoint_option( optarg );
                for ( std::vector< ModelParams >::iterator cur =
                          opts.models.begin( );
                      cur != opts.models.end( );
                      ++cur )
                {
                    if ( cur->dcuid == tp_req.dcuid )
                    {
                        cur->tp_list = tp_req.testpoints;
                        break;
                    }
                }
            }
            break;
        case 'f':
            if ( opts.models.empty( ) )
            {
                throw std::runtime_error( "Please specify -R before -f" );
            }
            {
                failure_list failures = parse_int_list( optarg );
                for ( std::vector< ModelParams >::iterator cur =
                          opts.models.begin( );
                      cur != opts.models.end( );
                      ++cur )
                {
                    if ( std::find( failures.begin( ),
                                    failures.end( ),
                                    cur->dcuid ) != failures.end( ) )
                    {
                        cur->status = MODEL_STATUS::STOPPED;
                        std::cerr << "Failing " << cur->dcuid << "\n";
                    }
                }
            }
            break;
        case 'h':
        default:
            opts.show_help = true;
            break;
        }
    }
    return opts;
}

/**
 * @brief helper function to sum models channel count via std::accumulate
 * @param current current value
 * @param m model ptr
 * @return current + m->chan_count
 */
size_t
sum_channels( const size_t current, const ModelPtr& m )
{
    return current + m->chan_count( );
}

/**
 * @brief helper function to open up all the mbufs for models
 * @param m model to open the mbuf for
 */
void
open_rmipc( const ModelPtr& m )
{
    m->open_rmpic( );
}

int
main( int argc, char* argv[] )
{
    Options opts = parse_arguments( argc, argv );
    if ( opts.show_help )
    {
        usage( argv[ 0 ] );
        exit( 1 );
    }
    bool                    concentrate = opts.concentrate;
    std::vector< ModelPtr > models;
    IniManager              ini_manager( opts.ini_root, opts.master_path );

    models.reserve( opts.models.size( ) );
    for ( int i = 0; i < opts.models.size( ); ++i )
    {
        ModelParams& param = opts.models[ i ];
        models.push_back( ModelPtr( new Model( ini_manager,
                                               param.name,
                                               param.dcuid,
                                               param.model_rate,
                                               param.data_rate,
                                               param.tp_list,
                                               param.status ) ) );
    }

    size_t total_chans = std::accumulate(
        models.begin( ), models.end( ), (size_t)0, sum_channels );
    std::cout << "Model count: " << models.size( ) << "\n";
    std::cout << "Channel count: " << total_chans << "\n";

    std::cout << "Sizeof daq_multi_cycle_data_t: "
              << sizeof( daq_multi_cycle_data_t ) << "\n";
    std::cout << "Sizeof daq_multi_cycle_data_t: "
              << sizeof( daq_multi_cycle_data_t ) / ( 1024 * 1024 ) << "MB\n";
    std::cout << "Sizeof daq_multi_cycle_data_t data: "
              << DAQ_TRANSIT_DC_DATA_BLOCK_SIZE * DAQ_NUM_DATA_BLOCKS_PER_SECOND
              << "\n";
    std::cout << "Sizeof daq_multi_cycle_data_t data: "
              << ( DAQ_TRANSIT_DC_DATA_BLOCK_SIZE *
                   DAQ_NUM_DATA_BLOCKS_PER_SECOND ) /
            ( 1024 * 1024 )
              << "MB\n";

    std::vector< daq_dc_data_t > buffer( 1 );

    volatile char*                     shmem = 0;
    volatile daq_multi_cycle_header_t* ifo_header = 0;
    volatile char*                     ifo_data = 0;
    size_t                             data_size = 0;

    if ( concentrate )
    {
        std::string shmem_sysname = opts.mbuf_name;

        size_t buffer_size_mb = opts.mbuf_size_mb;
        size_t buffer_size = buffer_size_mb * 1024 * 1024;
        shmem = reinterpret_cast< volatile char* >( findSharedMemorySize(
            const_cast< char* >( shmem_sysname.c_str( ) ), buffer_size_mb ) );
        if ( shmem == 0 )
        {
            std::cerr << "Unable to open shmem buffer\n";
            exit( 1 );
        }
        ifo_header =
            reinterpret_cast< volatile daq_multi_cycle_header_t* >( shmem );
        ifo_header->maxCycle = 16;
        data_size = buffer_size - sizeof( *ifo_header );
        data_size /= 16;
        data_size -= ( data_size % 8 );
        ifo_header->cycleDataSize = static_cast< unsigned int >( data_size );
        ifo_data = shmem + sizeof( *ifo_header );
    }
    else
    {
        std::for_each( models.begin( ), models.end( ), open_rmipc );
    }

    GPS::gps_clock clock( 0 );

    GPS::gps_time time_step = GPS::gps_time( 0, 1000000000 / 16 );
    GPS::gps_time transmit_time = clock.now( );
    ++transmit_time.sec;
    transmit_time.nanosec = 0;

    int delay_multiplier = 0;
    int cycle = 0;

    while ( true )
    {
        // The simulation writes 1/16s behind real time
        // So we wait until the cycle start, then compute
        // then write and wait for the next cycle.
        GPS::gps_time now = clock.now( );
        while ( now < transmit_time )
        {
            usleep( 1 );
            now = clock.now( );
        }
        usleep( delay_multiplier * 1000 );

        if ( concentrate )
        {
            generate_models( models,
                             buffer.front( ),
                             sizeof( buffer.front( ).dataBlock ),
                             cycle,
                             transmit_time );

            volatile char* dest = ifo_data + ( cycle * data_size );
            char* start = reinterpret_cast< char* >( &buffer.front( ) );
            std::copy( start, start + sizeof( daq_dc_data_t ), dest );

            ifo_header->curCycle = cycle;
        }
        else
        {
            generate_models_rmpic( models, cycle, transmit_time );
        }

        if ( cycle == 0 )
        {
            GPS::gps_time end = clock.now( );
            GPS::gps_time delta = end - now;
            std::cout << "Cycle took "
                      << static_cast< double >( delta.nanosec ) / 1000000000.0
                      << "s\n";
        }

        cycle = ( cycle + 1 ) % 16;
        transmit_time = transmit_time + time_step;
    }
    return 0;
}