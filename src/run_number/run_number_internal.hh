//
// Created by jonathan.hanks on 3/26/20.
//

#ifndef DAQD_TRUNK_RUN_NUMBER_INTERNAL_HH
#define DAQD_TRUNK_RUN_NUMBER_INTERNAL_HH

#include <boost/asio.hpp>
#include <boost/utility/string_view.hpp>
#include "run_number_structs.h"

#if BOOST_ASIO_VERSION < 101200
using io_context_t = boost::asio::io_service;
inline io_context_t&
get_context( boost::asio::ip::tcp::acceptor& acceptor )
{
    return acceptor.get_io_service( );
}

inline boost::asio::ip::address
make_address( const char* str )
{
    return boost::asio::ip::address::from_string( str );
}
#else
using io_context_t = boost::asio::io_context;

inline io_context_t&
get_context( boost::asio::ip::tcp::acceptor& acceptor )
{
    return acceptor.get_executor( ).context( );
}

inline boost::asio::ip::address
make_address( const char* str )
{
    return boost::asio::ip::make_address( str );
}
#endif

namespace daqd_run_number
{

    using boost::asio::ip::tcp;

    struct endpoint_address
    {
        std::string address{ "0.0.0.0" };
        std::string port{ "5556" };
    };

    /*!
     * @brief parse a connection string into an address/port pair.
     * @param conn_str The address
     * @return The parsed address
     * @details The full form is address:port, the port is defaulted if not
     * specified.  It can also accept zmq style tcp://address:port.
     * If "*" is passed as the address it is turned to "0.0.0.0".
     * @note the default port is 5556
     */
    inline endpoint_address
    parse_connection_string( const std::string& conn_str )
    {
        static const std::string tcp_prefix = "tcp://";
        endpoint_address         results;

        boost::string_view conn = conn_str;
        if ( conn.starts_with( tcp_prefix ) )
        {
            conn = conn.substr( tcp_prefix.size( ) );
        }

        auto index = conn.find( ':' );
        if ( index != boost::string_view::npos )
        {
            auto port = conn.substr( index + 1 );
            if ( !port.empty( ) )
            {
                results.port = std::string( port.begin( ), port.end( ) );
            }
        }
        auto address = conn.substr( 0, index );
        results.address = std::string( address.begin( ), address.end( ) );
        if ( results.address == "*" )
        {
            results.address = "0.0.0.0";
        }

        return results;
    }

    inline boost::asio::mutable_buffer
    to_buffer( daqd_run_number_req_v1_t& req )
    {
        return boost::asio::buffer( &req, sizeof( req ) );
    }

    inline boost::asio::mutable_buffer
    to_buffer( daqd_run_number_resp_v1_t& resp )
    {
        return boost::asio::buffer( &resp, sizeof( resp ) );
    }

    inline boost::asio::const_buffer
    to_const_buffer( daqd_run_number_req_v1_t& req )
    {
        return boost::asio::buffer( &req, sizeof( req ) );
    }

    inline boost::asio::const_buffer
    to_const_buffer( daqd_run_number_resp_v1_t& resp )
    {
        return boost::asio::buffer( &resp, sizeof( resp ) );
    }
} // namespace daqd_run_number

#endif // DAQD_TRUNK_RUN_NUMBER_INTERNAL_HH
