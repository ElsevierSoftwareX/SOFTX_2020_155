#include "pub_plugin_daq_m.hh"

#include "daq_core.h"
#include "drv/shmem.h"

#include "make_unique.hh"

namespace cds_plugins
{
    namespace detail
    {
        unsigned int
        calculate_cycle_data_size( std::size_t size_in_mb )
        {
            std::size_t  size = size_in_mb * 1024 * 1024;
            unsigned int cycle_data_size =
                ( size - sizeof( daq_multi_cycle_header_t ) ) /
                DAQ_NUM_DATA_BLOCKS;
            cycle_data_size -= ( cycle_data_size % 8 );
            return cycle_data_size;
        }

        class DaqMPublisherInstance : public pub_sub::plugins::PublisherInstance
        {
        public:
            DaqMPublisherInstance( const std::string& name,
                                   std::size_t        size_in_mb )
                : dest_{ (daq_multi_cycle_data_t*)shmem_open_segment(
                      name.c_str( ), size_in_mb * 1024 * 1024 ) },
                  size_in_mb_{ size_in_mb }, cycle_data_size_{
                      calculate_cycle_data_size( size_in_mb )
                  }
            {
                dest_->header.maxCycle = DAQ_NUM_DATA_BLOCKS;
                dest_->header.cycleDataSize = cycle_data_size_;
            }

            ~DaqMPublisherInstance( ) override = default;
            /*!
             * @brief The message publishing interface
             * @param key Message key
             * @param msg Message data
             */
            void
            publish( pub_sub::KeyType key, pub_sub::Message msg ) override
            {
                if ( !msg.data( ) )
                {
                    return;
                }
                if ( msg.length > cycle_data_size_ )
                {
                    throw std::runtime_error(
                        "Overflow of the daqm memory buffer" );
                }
                auto data = reinterpret_cast< const daq_multi_dcu_data_t* >(
                    msg.data( ) );
                if ( data->header.dcuTotalModels < 1 )
                {
                    return;
                }
                auto cycle = data->header.dcuheader[ 0 ].cycle;
                if ( cycle >= DAQ_NUM_DATA_BLOCKS )
                {
                    throw std::runtime_error( "Invalid cycle number" );
                }
                auto data_block = (char*)( &dest_->dataBlock[ 0 ] ) +
                    cycle * cycle_data_size_;
                auto start = reinterpret_cast< const char* >( data );
                auto stop = start + msg.length;
                std::copy( start, stop, data_block );
                dest_->header.curCycle = cycle;
            }

        private:
            daq_multi_cycle_data_t* dest_;
            std::size_t             size_in_mb_;
            unsigned int            cycle_data_size_;
        };
    } // namespace detail

    const std::string&
    PubPluginDaqMApi::prefix( ) const
    {
        const static std::string my_prefix = "daqm://";
        return my_prefix;
    }
    const std::string&
    PubPluginDaqMApi::version( ) const
    {
        const static std::string my_version = "0";
        return my_version;
    }
    const std::string&
    PubPluginDaqMApi::name( ) const
    {
        const static std::string my_name = "daq multi dcu memory buffer";
        return my_name;
    }
    pub_sub::plugins::UniquePublisherInstance
    PubPluginDaqMApi::publish( const std::string&        address,
                               pub_sub::PubDebugNotices& debug_hooks )
    {
        if ( address.find( prefix( ) ) != 0 )
        {
            throw std::runtime_error(
                "Invalid publisher type passed to the daqm publisher" );
        }
        auto        conn_str = address.substr( prefix( ).size( ) );
        auto        sep_index = conn_str.find( ':' );
        auto        name = conn_str;
        std::size_t buffer_size_mb = 100;
        if ( sep_index != std::string::npos )
        {
            name = conn_str.substr( 0, sep_index );
            auto size_str = conn_str.substr( sep_index + 1 );
            buffer_size_mb = std::stoi( size_str );
        }
        return make_unique_ptr< detail::DaqMPublisherInstance >(
            name, buffer_size_mb );
    }
} // namespace cds_plugins