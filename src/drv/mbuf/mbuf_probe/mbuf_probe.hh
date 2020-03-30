//
// Created by jonathan.hanks on 10/10/19.
//

#ifndef DAQD_MBUF_PROBE_HH
#define DAQD_MBUF_PROBE_HH

#include <cstddef>
#include <string>
#include <vector>

#include <unistd.h>

#include "mbuf_decoders.hh"

enum MBufCommands
{
    INVALID,
    CREATE,
    LIST,
    COPY,
    DELETE,
    ANALYZE,
    GAP_CHECK,
};

enum MBufStructures
{
    MBUF_INVALID,
    MBUF_RMIPC,
    MBUF_DAQ_MULTI_DC,
};

struct ConfigOpts
{
    ConfigOpts( )
        : action( INVALID ), buffer_size( 0 ), buffer_name( "" ),
          output_fname( "probe_out.bin" ), error_msg( "" ), dcu_id( -1 ),
          decoder( ), analysis_type( MBUF_INVALID )
    {
    }
    MBufCommands   action;
    std::size_t    buffer_size;
    std::string    buffer_name;
    std::string    output_fname;
    std::string    error_msg;
    int            dcu_id;
    DataDecoder    decoder;
    MBufStructures analysis_type;

    bool
    select_action( MBufCommands selected_action )
    {
        if ( action != INVALID )
        {
            set_error( "Please only select one action" );
            return false;
        }
        action = selected_action;
        return true;
    }

    void
    set_error( const std::string& msg )
    {
        action = INVALID;
        error_msg = msg;
    }

    void
    validate_options( )
    {
        if ( !error_msg.empty( ) )
        {
            return;
        }
        switch ( action )
        {
        case CREATE:
            if ( buffer_name.empty( ) || buffer_size == 0 )
            {
                set_error( "Both a buffer name and buffer size are required to "
                           "create a buffer" );
            }
            break;
        case LIST:
            break;
        case COPY:
            if ( buffer_name.empty( ) || buffer_size == 0 ||
                 output_fname.empty( ) )
            {
                set_error( "To copy a buffer a buffer name, size, and output "
                           "filename must be provided" );
            }
            break;
        case DELETE:
            if ( buffer_name.empty( ) )
            {
                set_error( "To delete a buffer you must specify its name" );
            }
            break;
        case ANALYZE:
            if ( buffer_name.empty( ) || buffer_size == 0 ||
                 analysis_type == MBUF_INVALID )
            {
                set_error( "To analyze a buffer a buffer, size, and structure "
                           "type must be provided" );
            }
            break;
        case GAP_CHECK:
            if ( buffer_name.empty( ) || buffer_size == 0 ||
                 analysis_type != MBUF_DAQ_MULTI_DC )
            {
                set_error( "To do gap checking a buffer and size must be set "
                           "as well as daq_multi_cycle" );
            }
            break;
        case INVALID:
        default:
            set_error( "Please select a valid action" );
        }
    }

    bool
    should_show_help( )
    {
        return action == INVALID;
    }

    bool
    is_in_error( )
    {
        return ( should_show_help( ) ? !error_msg.empty( ) : false );
    }
};

template < typename T >
unsigned int
wait_until_changed( volatile T* counter, T old_counter )
{
    T cur_cycle = *counter;
    do
    {
        usleep( 250 );
        cur_cycle = *counter;
    } while ( cur_cycle == old_counter );
    return cur_cycle;
}

#endif // DAQD_MBUF_PROBE_HH
