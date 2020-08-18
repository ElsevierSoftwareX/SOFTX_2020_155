//
// Created by jonathan.hanks on 8/13/20.
//

#include "check_size.hh"

#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>

#include "daq_core.h"
#include "daq_core_defs.h"
#include "daq_data_types.h"

#include "daqmap.h"
extern "C" {
#include "param.h"
}

namespace check_mbuf_sizes
{
    typedef struct ini_file_info
    {
        ini_file_info( )
            : dcu_sizes( ), dcu_config_crc( ), last_dcuid( 0 ), valid( true )
        {
            std::fill( dcu_sizes.begin( ), dcu_sizes.end( ), 0 );
            std::fill( dcu_config_crc.begin( ), dcu_config_crc.end( ), 0 );
        }
        ini_file_info( const ini_file_info& ) noexcept = default;
        ini_file_info& operator=( const ini_file_info& ) noexcept = default;

        std::array< unsigned int, DAQ_TRANSIT_MAX_DCU >  dcu_sizes;
        std::array< unsigned long, DAQ_TRANSIT_MAX_DCU > dcu_config_crc;
        int                                              last_dcuid;
        bool                                             valid;
    } size_info;

    int
    channel_callback( char*              channel_name,
                      struct CHAN_PARAM* params,
                      void*              user )
    {
        auto* info = reinterpret_cast< ini_file_info* >( user );
        int   chan_size = data_type_size( params->datatype ) *
            ( params->datarate / DAQ_NUM_DATA_BLOCKS_PER_SECOND );
        if ( chan_size == 0 )
        {
            info->valid = false;
        }
        else
        {
            info->dcu_sizes[ params->dcuid ] += chan_size;
        }
        info->last_dcuid = params->dcuid;
        return 1;
    }

    ini_file_info
    load_master( const std::string& fname )
    {
        ini_file_info sysinfo;
        // File names are specified in `master_config' file
        std::ifstream mcf( fname );

        for ( std::string line; std::getline( mcf, line ); )
        {
            std::string trimmed_line;
            trimmed_line.reserve( line.size( ) );
            std::copy_if( line.begin( ),
                          line.end( ),
                          std::back_inserter( trimmed_line ),
                          []( const char ch ) -> bool { return ch != ' '; } );
            if ( line.front( ) == '#' )
            {
                continue;
            }
            if ( trimmed_line.empty( ) || trimmed_line.front( ) == '#' )
            {
                continue;
            }

            std::string ext =
                ( trimmed_line.size( ) > 4
                      ? trimmed_line.substr( trimmed_line.size( ) - 4 )
                      : "" );
            auto testpoint = ( ext == ".par" );

            unsigned long file_crc = 0;
            sysinfo.valid = true;
            sysinfo.last_dcuid = -1;
            int parse_status =
                ::parseConfigFile( const_cast< char* >( trimmed_line.c_str( ) ),
                                   &file_crc,
                                   channel_callback,
                                   testpoint,
                                   0,
                                   reinterpret_cast< void* >( &sysinfo ) );
            if ( parse_status == 0 || !sysinfo.valid || sysinfo.last_dcuid < 0 )
            {
                std::cerr << "Unable to process " << trimmed_line
                          << " either the file is invalid, bad data types, or "
                             "other error ("
                          << sysinfo.valid << ", " << sysinfo.last_dcuid
                          << ")\n";
                if ( sysinfo.last_dcuid >= 0 )
                {
                    sysinfo.dcu_sizes[ sysinfo.last_dcuid ] = 0;
                }
            }
            else
            {
                sysinfo.dcu_config_crc[ sysinfo.last_dcuid ] = file_crc;
            }
        }
        return sysinfo;
    }

    int
    check_size_rmipc( volatile void*      buffer,
                      std::size_t         buffer_size,
                      const ::ConfigOpts& options )
    {
        if ( options.analysis_type != MBUF_RMIPC ||
             buffer_size != 64 * 1024 * 1024 )
        {
            return 0;
        }

        load_master( options.ini_file_fname );

        unsigned int prev_config_crc{ 0 };
        unsigned int prev_data_size{ 0 };

        auto header = reinterpret_cast< const volatile rmIpcStr* >( buffer );

        ini_file_info dcu_reference;
        bool          noted_crc_change{ false };

        bool have_inis = false;
        if ( !options.ini_file_fname.empty( ) )
        {
            dcu_reference = load_master( options.ini_file_fname );
            have_inis = true;
        }

        bool first_iteration = true;
        while ( true )
        {
            wait_until_changed< const unsigned int, 5000 >( &header->cycle,
                                                            header->cycle );

            auto cur_cycle = header->cycle;
            auto dcuid = header->dcuId;

            if ( !first_iteration )
            {
                if ( have_inis )
                {
                    if ( header->crc != dcu_reference.dcu_config_crc[ dcuid ] &&
                         !noted_crc_change )
                    {
                        std::cout
                            << "config change on dcu " << dcuid << " from "
                            << dcu_reference.dcu_config_crc[ dcuid ] << " to "
                            << header->crc << "\n";
                        noted_crc_change = true;
                    }
                    if ( dcu_reference.dcu_sizes[ dcuid ] !=
                         header->dataBlockSize )
                    {
                        std::cout << "wrong data size on " << dcuid
                                  << " expected "
                                  << dcu_reference.dcu_sizes[ dcuid ] << " got "
                                  << header->dataBlockSize << "\n";
                    }
                }
                else
                {
                    if ( prev_config_crc != header->crc && !noted_crc_change )
                    {
                        std::cout << "config change on dcu " << dcuid
                                  << " from " << prev_config_crc << " to "
                                  << header->crc << "\n";
                        noted_crc_change;
                    }
                    if ( prev_data_size != header->dataBlockSize )
                    {
                        std::cout << "data size change on dcu " << dcuid
                                  << " from " << prev_data_size << " to "
                                  << header->dataBlockSize << "\n";
                    }
                }
            }
            prev_config_crc = header->crc;
            prev_data_size = header->dataBlockSize;

            if ( first_iteration )
            {
                std::cout
                    << "Completed the first iteration of the check_size test"
                    << std::endl;
                first_iteration = false;
            }
        }
        return 0;
    }

    int
    check_size_multi( volatile void*      buffer,
                      std::size_t         buffer_size,
                      const ::ConfigOpts& options )
    {
        if ( !buffer || buffer_size < DAQD_MIN_SHMEM_BUFFER_SIZE ||
             buffer_size > DAQD_MAX_SHMEM_BUFFER_SIZE ||
             options.analysis_type != MBUF_DAQ_MULTI_DC )
        {
            return 0;
        }

        load_master( options.ini_file_fname );

        std::array< unsigned int, DAQ_TRANSIT_MAX_DCU > prev_config_crc{};
        std::array< unsigned int, DAQ_TRANSIT_MAX_DCU > prev_data_size{};

        std::fill( prev_config_crc.begin( ), prev_config_crc.end( ), 0 );
        std::fill( prev_data_size.begin( ), prev_data_size.end( ), 0 );

        auto multi_header =
            reinterpret_cast< const volatile daq_multi_cycle_data_t* >(
                buffer );

        ini_file_info                           dcu_reference;
        std::array< bool, DAQ_TRANSIT_MAX_DCU > noted_crc_change{};
        std::fill( noted_crc_change.begin( ), noted_crc_change.end( ), false );
        bool have_inis = false;
        if ( !options.ini_file_fname.empty( ) )
        {
            dcu_reference = load_master( options.ini_file_fname );
            have_inis = true;
        }

        bool first_iteration = true;
        while ( true )
        {
            wait_for_time_change< 5000 >( multi_header->header );

            auto cur_cycle = multi_header->header.curCycle;
            auto cycle_data_size = multi_header->header.cycleDataSize;

            auto cur_cylce_block =
                reinterpret_cast< const volatile daq_multi_dcu_data_t* >(
                    &multi_header->dataBlock[ 0 ] +
                    (cycle_data_size)*cur_cycle );
            auto total_models = cur_cylce_block->header.dcuTotalModels;

            for ( auto i = 0; i < total_models; ++i )
            {
                auto& cur_header = cur_cylce_block->header.dcuheader[ i ];
                auto  dcuid = cur_header.dcuId;

                if ( !first_iteration )
                {
                    if ( have_inis )
                    {
                        if ( cur_header.fileCrc !=
                                 dcu_reference.dcu_config_crc[ dcuid ] &&
                             !noted_crc_change[ dcuid ] )
                        {
                            std::cout << "config change on dcu " << dcuid
                                      << " from "
                                      << dcu_reference.dcu_config_crc[ dcuid ]
                                      << " to " << cur_header.fileCrc << "\n";
                            noted_crc_change[ dcuid ] = true;
                        }
                        if ( dcu_reference.dcu_sizes[ dcuid ] !=
                             cur_header.dataBlockSize )
                        {
                            std::cout << "wrong data size on " << dcuid
                                      << " expected "
                                      << dcu_reference.dcu_sizes[ dcuid ]
                                      << " got " << cur_header.dataBlockSize
                                      << "\n";
                        }
                    }
                    else
                    {
                        if ( prev_config_crc[ dcuid ] != cur_header.fileCrc &&
                             !noted_crc_change[ dcuid ] )
                        {
                            std::cout << "config change on dcu " << dcuid
                                      << " from " << prev_config_crc[ dcuid ]
                                      << " to " << cur_header.fileCrc << "\n";
                            noted_crc_change[ dcuid ];
                        }
                        if ( prev_data_size[ dcuid ] !=
                             cur_header.dataBlockSize )
                        {
                            std::cout << "data size change on dcu " << dcuid
                                      << " from " << prev_data_size[ dcuid ]
                                      << " to " << cur_header.dataBlockSize
                                      << "\n";
                        }
                    }
                }
                prev_config_crc[ dcuid ] = cur_header.fileCrc;
                prev_data_size[ dcuid ] = cur_header.dataBlockSize;
            }
            if ( first_iteration )
            {
                std::cout
                    << "Completed the first iteration of the check_size test"
                    << std::endl;
                first_iteration = false;
            }
        }
        return 0;
    }
} // namespace check_mbuf_sizes