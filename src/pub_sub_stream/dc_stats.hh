#ifndef CPS_PUB_SUB_DC_STATS_HH
#define CPS_PUB_SUB_DC_STATS_HH

#include <array>
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
    DCStats( const DCStats& ) = delete;
    DCStats( DCStats&& ) = delete;
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

private:
    dc_queue::value_type get_message( simple_pv_handle epics_server );

    static int process_channel( std::vector< channel_t >& channels,
                                int&                      ini_file_dcu_id,
                                const char*               channel_name,
                                const CHAN_PARAM*         params );

    dc_queue                          queue_;
    std::array< DCUStats, DCU_COUNT > dcu_status_{};

    int          not_stalled_{ 0 };
    int          uptime_{ 0 };
    unsigned int gpstime_{ 0 };
    int          channel_config_hash_{ 0 };
    int          unique_dcus_per_sec_{ 0 };
    int          data_rate_{ 0 };
    int          total_chans_{ 0 };
    int          open_tp_count_{ 0 };
    unsigned int tp_data_kb_per_s_{ 0 };
    unsigned int model_data_kb_per_s_{ 0 };
    unsigned int total_data_kb_per_s_{ 0 };

    bool valid_{ false };
};

#endif // CPS_PUB_SUB_DC_STATS_HH