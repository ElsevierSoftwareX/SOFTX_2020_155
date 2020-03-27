#include "run_number_client.hh"

#include "run_number_internal.hh"

namespace daqd_run_number
{

    int
    get_run_number( const std::string& target, const std::string& hash )
    {
        const int timeout = 10 * 1000;

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

            boost::asio::write( socket, to_const_buffer( req ) );

            daqd_run_number_resp_v1_t resp;
            boost::asio::read( socket, to_buffer( resp ) );

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