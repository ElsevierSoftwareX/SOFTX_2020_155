//
// Created by jonathan.hanks on 8/22/19.
//

#ifndef DAQD_TRUNK_FE_GENERATOR_SUPPORT_HH
#define DAQD_TRUNK_FE_GENERATOR_SUPPORT_HH

#include <string>
#include <vector>

#include "fe_stream_generator.hh"

class ChNumDb
{
private:
    int max_;

public:
    ChNumDb( ) : max_( 40000 )
    {
    }
    explicit ChNumDb( int start ) : max_( start )
    {
    }

    int
    next( int channel_type )
    {
        max_++;
        return max_;
    }
};

std::string cleaned_system_name( const std::string& system_name );

std::string generate_ini_filename( const std::string& ini_dir,
                                   const std::string& system_name );

std::string generate_par_filename( const std::string& ini_dir,
                                   const std::string& system_name );

void output_ini_files( const std::string&          ini_dir,
                       const std::string&          system_name,
                       std::vector< GeneratorPtr > channels,
                       std::vector< GeneratorPtr > tp_channels,
                       int                         dcuid,
                       int                         model_rate=2048);

unsigned int calculate_ini_crc( const std::string& ini_dir,
                                const std::string& system_name );

unsigned int
calculate_crc( const void* buffer, size_t len );

#endif // DAQD_TRUNK_FE_GENERATOR_SUPPORT_HH
