#ifndef CPS_PUB_SUB_DC_STATS_HH
#define CPS_PUB_SUB_DC_STATS_HH

#include <array>
#include <atomic>
#include <vector>

#include <daq_core.h>
#include "simple_pv.h"
#include "message_queue.hh"
#include "channel.hh"

using dc_queue = Message_queue< std::unique_ptr< daq_dc_data_t >, 4 >;

struct CHAN_PARAM;

struct DCUStats
{
    int          expected_config_crc{ 0 };
    unsigned int status{ 0 };
    int          crc_per_sec{ 0 };
    int          crc_sum{ 0 };

    std::string full_model_name{};
    // use vectors instead of strings as we need raw c strings
    // and need to reference them in a stable way, not through'
    // a temp c_str()
    std::vector< char > expected_config_crc_name{};
    std::vector< char > status_name{};
    std::vector< char > crc_per_sec_name{};
    std::vector< char > crc_sum_name{};

    bool seen_last_cycle{ false };
    bool processed{ false };
    bool processed_this_cycle{ false };

    void setup_pv_names( );
};

class DCStats
{
public:
    DCStats( std::vector< SimplePV >& pvs,
             const std::string&       channel_list_path );

    /*!
     * @brief This is a debug interface to allow setting up the state
     * of the dcus.  It does not handle epics information.
     * @tparam C
     * @param dcus
     */
    template < typename C >
    explicit DCStats( const C& dcus )
    {
        valid_ = true;
        if ( dcus.size( ) > dcu_status_.size( ) )
        {
            throw std::runtime_error( "The list of dcus provided was too big" );
        }
        std::copy( dcus.begin( ), dcus.end( ), dcu_status_.begin( ) );
    }

    DCStats( const DCStats& ) = delete;

    DCStats( DCStats&& ) = delete;

    ~DCStats( );

    DCStats& operator=( const DCStats& ) = delete;

    DCStats& operator=( DCStats&& ) = delete;

    dc_queue*
    get_queue( )
    {
        return valid_ ? &queue_ : nullptr;
    }

    void run( simple_pv_handle epics_server );

    bool
    is_valid( ) const
    {
        return valid_;
    }

    void
    request_clear_crc( )
    {
        request_clear_crc_ = true;
    }

    void
    stop( )
    {
        request_stop_ = true;
        queue_.emplace_with_timeout( std::chrono::milliseconds( 0 ), nullptr );
    }

    /*!
     * @brief this is a debug interface
     * @param dcuid
     * @return
     */
    const DCUStats&
    peek_stats( unsigned int dcuid ) const
    {
        return dcu_status_[ dcuid ];
    }

    /*!
     * @brief Return the total crc count for the system, a debugging/testing
     * aid.
     * @return the sum of the crc count for the lifetime of this object.
     */
    int
    get_total_crcs( ) const
    {
        return total_crc_count_;
    }

    /*!
     * @brief get a readonly reference to the channel list
     * @return the channel list
     */
    const std::vector< channel_t >&
    channels( ) const
    {
        return channels_;
    }

private:
    /*!
     * @brief sort the channels in a way that is compatible with daqd
     */
    void sort_channels( );

    dc_queue::value_type get_message( simple_pv_handle epics_server );

    int process_channel( int&              ini_file_dcu_id,
                         const char*       channel_name,
                         const CHAN_PARAM* params );

    dc_queue                          queue_;
    std::array< DCUStats, DCU_COUNT > dcu_status_{};

    int          not_stalled_{ 0 };
    int          uptime_{ 0 };
    unsigned int gpstime_{ 0 };
    double       data_crc_{ 0 };
    double       channel_config_hash_{ 0 };
    int          unique_dcus_per_sec_{ 0 };
    int          data_rate_{ 0 };
    int          total_chans_{ 0 };
    int          open_tp_count_{ 0 };
    unsigned int tp_data_kb_per_s_{ 0 };
    unsigned int model_data_kb_per_s_{ 0 };
    unsigned int total_data_kb_per_s_{ 0 };
    int          total_crc_count_{ 0 };

    std::atomic< bool > request_clear_crc_{ false };
    std::atomic< bool > request_stop_{ false };
    bool                valid_{ false };

    std::vector< channel_t > channels_{};
};

#endif // CPS_PUB_SUB_DC_STATS_HH