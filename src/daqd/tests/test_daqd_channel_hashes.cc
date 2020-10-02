//
// Created by jonathan.hanks on 10/2/20.
//

#include "channel.hh"
#include <cstring>
#include <sstream>
#include <string>

#include "framecpp/Common/MD5SumFilter.hh"
#include "catch.hpp"

namespace
{
    channel_t
    channel1( )
    {
        channel_t ch{};

        ch.chNum = 1000;
        ch.seq_num = 1;
        ch.id = nullptr;
        std::strcpy( ch.name, "CHANNEL1_THE_REAL_DEAL" );
        ch.sample_rate = 16;
        ch.active = 1;
        ch.group_num = 1;
        ch.bps = 16 * 4;
        ch.offset = 0;
        ch.bytes = 4;
        ch.req_rate = 16;
        ch.dcu_id = 1;
        ch.ifoid = 0;
        ch.tp_node = 1;
        ch.rm_offset = 0;
        ch.data_type = daq_data_t::_32bit_float;
        ch.signal_gain = 1.0;
        ch.signal_slope = 1.0;
        ch.signal_offset = 0.0;
        std::strcpy( ch.signal_units, "SOMETHING" );
        return ch;
    }

    channel_t
    channel2( )
    {
        channel_t ch{};

        ch.chNum = 1000;
        ch.seq_num = 1;
        ch.id = nullptr;
        std::strcpy( ch.name, "CHANNEL1_IMPOSTER" );
        ch.sample_rate = 16;
        ch.active = 1;
        ch.group_num = 1;
        ch.bps = 16 * 4;
        ch.offset = 0;
        ch.bytes = 4;
        ch.req_rate = 16;
        ch.dcu_id = 1;
        ch.ifoid = 0;
        ch.tp_node = 1;
        ch.rm_offset = 0;
        ch.data_type = daq_data_t::_32bit_float;
        ch.signal_gain = 1.0;
        ch.signal_slope = 1.0;
        ch.signal_offset = 0.0;
        std::strcpy( ch.signal_units, "SOMETHING_ELSE" );
        return ch;
    }

    channel_t
    channel3( )
    {
        channel_t ch{};

        ch.chNum = 1001;
        ch.seq_num = 1;
        ch.id = nullptr;
        std::strcpy( ch.name, "NOT_LIKE_CHANNEL1_IMPOSTER" );
        ch.sample_rate = 16;
        ch.active = 1;
        ch.group_num = 1;
        ch.bps = 16 * 4;
        ch.offset = 0;
        ch.bytes = 4;
        ch.req_rate = 16;
        ch.dcu_id = 1;
        ch.ifoid = 0;
        ch.tp_node = 1;
        ch.rm_offset = 0;
        ch.data_type = daq_data_t::_32bit_float;
        ch.signal_gain = 1.0;
        ch.signal_slope = 1.0;
        ch.signal_offset = 0.0;
        std::strcpy( ch.signal_units, "SOMETHING_ELSE_ENTIRELY" );
        return ch;
    }

    std::string
    test_hash_this_channel_v0( const channel_t& ch )
    {
        FrameCPP::Common::MD5Sum                       check_sum;
        FrameCpp_hash_adapter< decltype( check_sum ) > hash_wrapper(
            check_sum );

        hash_channel_v0_broken( hash_wrapper, ch );

        check_sum.Finalize( );
        std::ostringstream os;
        os << check_sum;
        return os.str( );
    }

    std::string
    test_hash_this_channel_v1( const channel_t& ch )
    {
        FrameCPP::Common::MD5Sum                       check_sum;
        FrameCpp_hash_adapter< decltype( check_sum ) > hash_wrapper(
            check_sum );

        hash_channel( hash_wrapper, ch );

        check_sum.Finalize( );
        std::ostringstream os;
        os << check_sum;
        return os.str( );
    }
} // namespace

TEST_CASE(
    "hash_channel_v0_broken has easy collisions, reproduce buggy behavior" )
{

    auto hash1 = test_hash_this_channel_v0( channel1( ) );
    auto hash2 = test_hash_this_channel_v0( channel2( ) );

    REQUIRE( hash1 == hash2 );

    REQUIRE( hash1 == "1353dc82bb61dddaf97ffdf2400206d0" );
}

TEST_CASE( "hash_channel_v0_broken doesn't collide on everything" )
{
    auto hash1 = test_hash_this_channel_v0( channel1( ) );
    auto hash3 = test_hash_this_channel_v0( channel3( ) );

    REQUIRE( hash1 != hash3 );

    REQUIRE( hash3 == "c45a8cc2d02de142d87c89d260e7f6d5" );
}

TEST_CASE( "hash_channel does not have the same collision issue as "
           "hash_channel_v0_broken" )
{
    auto hash1 = test_hash_this_channel_v1( channel1( ) );
    auto hash2 = test_hash_this_channel_v1( channel2( ) );
    auto hash3 = test_hash_this_channel_v1( channel3( ) );

    REQUIRE( hash1 != hash2 );
    REQUIRE( hash1 != hash3 );
    REQUIRE( hash2 != hash3 );

    REQUIRE( hash1 == "5bd1825ca5aaf1f1354ac3d39fe9a5e3" );
    REQUIRE( hash2 == "7aa4d0506697a4d7ad7d5268a8a38389" );
    REQUIRE( hash3 == "6b194dd0ec444ab919072d94ed95c130" );
}
