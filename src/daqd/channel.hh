#ifndef CHANNEL_HH
#define CHANNEL_HH

#include "channel.h"
#include <string.h>
#include <cstdlib>

typedef struct
{
    enum
    {
        channel_group_name_max_len = MAX_CHANNEL_NAME_LENGTH
    };
    int  num; // Group number
    char name[ channel_group_name_max_len ]; // Group name
} channel_group_t;

#define GDS_CHANNEL 1
#define GDS_ALIAS_CHANNEL 2
#define IS_GDS_ALIAS( a ) ( ( a ).gds & 2 )
#define IS_GDS_SIGNAL( a ) ( ( a ).gds & 1 )

/// Channel name structure
struct channel_t
{
    /// archive channels group number is the same for all archived channels
    /// (except "obsolete" archive)
    const static int arc_groupn = 1000;

    /// Obsolete channel archive group number
    const static int obsolete_arc_groupn = 1001;

    /// archive channels are all assumed to be of the same data rate
    /// (archive have trend and minute-trend data only)
    /// This is phony number really, 16 Hz is used to put archive channels
    /// into the slow signal list
    const static int arc_rate = 16;

    enum
    {
        channel_name_max_len = MAX_CHANNEL_NAME_LENGTH,
        engr_unit_max_len = MAX_ENGR_UNIT_LENGTH
    };
    int   chNum; ///< channel hardware address
    int   seq_num;
    void* id;
#ifndef NO_SLOW_CHANNELS
    int slow;
#endif
    char name[ channel_name_max_len ];
    int  sample_rate;
    int  active;
    int  trend; ///< Set if trend is calculated and saved for this channel
    int  group_num; ///< Channel group number

    unsigned int bps; ///< Bytes per sample
    long         offset; ///< In the data block for this channel
    int          bytes; ///< Size of the channel data in the block
    int req_rate; ///< Sampling rate requested for this channel (used by
                  ///< net_writer_c)

#if defined( NETWORK_PRODUCER )
    int rcvbuf_offset;
#endif

    int  dcu_id;
    int  ifoid;
    int  tp_node; ///< Testpoint node id
    long rm_offset; ///< Reflected memory offset for the channel data
#ifdef GDS_TESTPOINTS
    int gds; ///< Set to 1 if gds channel, set to 3 if gds alias channel
#endif

    daq_data_t data_type;

    float signal_gain;
    float signal_slope;
    float signal_offset;
    char  signal_units[ engr_unit_max_len ]; ///< Engineering units
};

/// Long channel name structure
class long_channel_t : public channel_t
{
public:
    enum
    {
        channel_name_max_len = MAX_LONG_CHANNEL_NAME_LENGTH
    };
    char name[ channel_name_max_len ];
    long_channel_t&
    operator=( const channel_t& a )
    {
        chNum = a.chNum;
        seq_num = a.seq_num;
        id = a.id;
#ifndef NO_SLOW_CHANNELS
        slow = a.slow;
#endif
        strcpy( name, a.name );
        sample_rate = a.sample_rate;
        active = a.active;
        trend = a.trend;
        group_num = a.group_num;

        bps = a.bps;
        offset = a.offset;
        bytes = a.bytes;
        req_rate = a.req_rate;

#if defined( NETWORK_PRODUCER )
        rcvbuf_offset = a.rcvbuf_offset;
#endif

        tp_node = a.tp_node; ///< Testpoint node id
        dcu_id = a.dcu_id;
        ifoid = a.ifoid;
        rm_offset = a.rm_offset;
#ifdef GDS_TESTPOINTS
        gds = a.gds;
#endif

        data_type = a.data_type;

        signal_gain = a.signal_gain;
        signal_slope = a.signal_slope;
        signal_offset = a.signal_offset;
        strcpy( signal_units, a.signal_units );
        return *this;
    }
};

template < typename Hash >
class FrameCpp_hash_adapter
{
public:
    explicit FrameCpp_hash_adapter( Hash& hash ) : hash_{ hash }
    {
    }
    FrameCpp_hash_adapter( const FrameCpp_hash_adapter& ) = default;

    void
    add( const void* data, std::size_t length )
    {
        // why is this not const void* ?
        hash_.Update( const_cast< void* >( data ), length );
    }

private:
    Hash& hash_;
};

template < typename HashObject >
void
hash_channel_v0_broken( HashObject& hash, const channel_t& channel )
{
    // this is broken, but is kept (for now) for compatibility
    // the name and units sum the first sizeof(size_t) bytes of their values
    hash.add( &( channel.chNum ), sizeof( channel.chNum ) );
    hash.add( &( channel.seq_num ), sizeof( channel.seq_num ) );
    size_t name_len = strnlen( channel.name, channel_t::channel_name_max_len );
    hash.add( channel.name, sizeof( name_len ) );
    hash.add( &( channel.sample_rate ), sizeof( channel.sample_rate ) );
    hash.add( &( channel.active ), sizeof( channel.active ) );
    hash.add( &( channel.trend ), sizeof( channel.trend ) );
    hash.add( &( channel.group_num ), sizeof( channel.group_num ) );
    hash.add( &( channel.bps ), sizeof( channel.bps ) );
    hash.add( &( channel.dcu_id ), sizeof( channel.dcu_id ) );
    hash.add( &( channel.data_type ), sizeof( channel.data_type ) );
    hash.add( &( channel.signal_gain ), sizeof( channel.signal_gain ) );
    hash.add( &( channel.signal_slope ), sizeof( channel.signal_slope ) );
    hash.add( &( channel.signal_offset ), sizeof( channel.signal_offset ) );
    size_t unit_len =
        strnlen( channel.signal_units, channel_t::engr_unit_max_len );
    hash.add( channel.signal_units, sizeof( unit_len ) );
}

template < typename HashObject >
void
hash_channel( HashObject& hash, const channel_t& channel )
{
    hash.add( &( channel.chNum ), sizeof( channel.chNum ) );
    hash.add( &( channel.seq_num ), sizeof( channel.seq_num ) );
    size_t name_len = strnlen( channel.name, channel_t::channel_name_max_len );
    hash.add( channel.name, name_len );
    hash.add( &( channel.sample_rate ), sizeof( channel.sample_rate ) );
    hash.add( &( channel.active ), sizeof( channel.active ) );
    hash.add( &( channel.trend ), sizeof( channel.trend ) );
    hash.add( &( channel.group_num ), sizeof( channel.group_num ) );
    hash.add( &( channel.bps ), sizeof( channel.bps ) );
    hash.add( &( channel.dcu_id ), sizeof( channel.dcu_id ) );
    hash.add( &( channel.data_type ), sizeof( channel.data_type ) );
    hash.add( &( channel.signal_gain ), sizeof( channel.signal_gain ) );
    hash.add( &( channel.signal_slope ), sizeof( channel.signal_slope ) );
    hash.add( &( channel.signal_offset ), sizeof( channel.signal_offset ) );
    size_t unit_len =
        strnlen( channel.signal_units, channel_t::engr_unit_max_len );
    hash.add( channel.signal_units, unit_len );
}

#endif
