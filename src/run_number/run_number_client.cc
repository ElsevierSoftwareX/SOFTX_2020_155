#include "run_number_client.hh"

#include <chrono>

#include "run_number_internal.hh"

namespace daqd_run_number
{
    void
    run_socket_timeout( tcp::socket& s, const std::chrono::seconds& timeout )
    {
        io_context_t& context = s.get_executor( ).context( );

        context.restart( );
        context.run_for( timeout );
        if ( !context.stopped( ) )
        {
            s.close( );
            context.run( );
        }
        else
        {
            context.restart( );
        }
    }

    void
    read_with_timeout( tcp::socket&                s,
                       boost::asio::mutable_buffer dest,
                       std::chrono::seconds        timeout )
    {
        bool read_done{ false };

        io_context_t& context = s.get_executor( ).context( );

        boost::asio::async_read(
            s,
            dest,
            [&read_done, &context]( const boost::system::error_code& ec,
                                    size_t bytes_read ) {
                context.stop( );
                if ( !ec )
                {
                    read_done = true;
                }
            } );
        run_socket_timeout( s, timeout );
        if ( !read_done )
        {
            throw std::runtime_error( "Timeout or network error" );
        }
    }

    void
    write_with_timeout( tcp::socket&              s,
                        boost::asio::const_buffer src,
                        std::chrono::seconds      timeout )
    {
        bool write_done{ false };

        io_context_t& context = s.get_executor( ).context( );

        boost::asio::async_write(
            s,
            src,
            [&write_done, &context]( const boost::system::error_code& ec,
                                     size_t bytes_written ) {
                context.stop( );
                if ( !ec )
                {
                    write_done = true;
                }
            } );
        run_socket_timeout( s, timeout );
        if ( !write_done )
        {
            throw std::runtime_error( "Timeout or network error" );
        }
    }

    int
    get_run_number( const std::string& target, const std::string& hash )
    {
        const std::chrono::seconds timeout( 10 );

        try
        {
            daqd_run_number_req_v1_t req;
            if ( hash.size( ) > sizeof( req.hash ) )
                return 0;

            auto target_address =
                daqd_run_number::parse_connection_string( target );

            io_context_t                context;
            tcp::resolver               resolver( context );
            tcp::resolver::results_type endpoints =
                resolver.resolve( target_address.address, target_address.port );
            tcp::socket socket( context );
            boost::asio::connect( socket, endpoints );

            req.version = 1;
            req.hash_size = static_cast< short >( hash.size( ) );
            memset( &req.hash[ 0 ], 0, sizeof( req.hash ) );
            memcpy( &req.hash[ 0 ], hash.data( ), hash.size( ) );

            write_with_timeout( socket, to_const_buffer( req ), timeout );

            daqd_run_number_resp_v1_t resp;
            read_with_timeout( socket, to_buffer( resp ), timeout );

            if ( resp.version != 1 )
                return 0;
            return resp.number;
        }
        catch ( ... )
        {
            return 0;
        }
    }
} // namespace daqd_run_number