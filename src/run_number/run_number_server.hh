//
// Created by jonathan.hanks on 3/26/20.
//

#ifndef DAQD_TRUNK_RUN_NUMBER_SERVER_HH
#define DAQD_TRUNK_RUN_NUMBER_SERVER_HH

#include <cstdlib>
#include <memory>
#include <ostream>

#include "run_number_internal.hh"

namespace daqd_run_number
{
    template < typename RN >
    class connection : public std::enable_shared_from_this< connection< RN > >
    {
    public:
        explicit connection( tcp::socket s, RN& run_number, std::ostream& log )
            : s_{ std::move( s ) },
              run_number_{ run_number }, log_{ log }, req_{}, resp_{}
        {
        }
        void
        run( )
        {
            auto self = this->shared_from_this( );
            boost::asio::async_read(
                s_,
                to_buffer( req_ ),
                [self]( const boost::system::error_code& ec,
                        std::size_t                      bytes_read ) {
                    if ( !ec )
                    {
                        self->process_request( );
                    }
                } );
        }

    private:
        void
        process_request( )
        {
            int number = 0;
            if ( req_.version == 1 && req_.hash_size > 0 &&
                 req_.hash_size <= sizeof( req_.hash ) )
            {
                auto hash = std::string( req_.hash, req_.hash_size );
                number = run_number_.get_number( hash );
                log_ << "Received a v" << req_.version << " request with hash "
                     << hash << " returning run number = " << number
                     << std::endl;
            }
            else
            {
                log_ << "Invalid request received, returning run_number = 0"
                     << std::endl;
            }
            send_response( number );
        }

        void
        send_response( int number )
        {
            resp_.version = 1;
            resp_.padding = 0;
            resp_.number = number;

            auto self = this->shared_from_this( );
            boost::asio::async_write(
                s_,
                to_const_buffer( resp_ ),
                [self]( const boost::system::error_code&, std::size_t ) {} );
        }

        tcp::socket   s_;
        RN&           run_number_;
        std::ostream& log_;

        daqd_run_number_req_v1_t  req_;
        daqd_run_number_resp_v1_t resp_;
    };

    template < typename RN >
    class server
    {
    public:
        server( RN&                     run_number,
                const endpoint_address& address,
                std::ostream&           log )
            : context_{},
              acceptor_(
                  context_,
                  tcp::endpoint( make_address( address.address.c_str( ) ),
                                 std::atoi( address.port.c_str( ) ) ) ),
              run_number_{ run_number }, log_{ log }
        {
        }

        server( const server& ) = delete;
        server( server&& ) = delete;
        server& operator=( const server& ) = delete;
        server& operator=( server&& ) = delete;

        void
        run( )
        {
            start_accept( );
            context_.run( );
        }

        void
        stop( )
        {
            context_.stop( );
        }

    private:
        void
        start_accept( )
        {
            acceptor_.async_accept(
                [this]( const boost::system::error_code& ec, tcp::socket s ) {
                    if ( !ec )
                    {
                        handle_accept( std::move( s ) );
                    }
                    start_accept( );
                } );
        }

        void
        handle_accept( tcp::socket s )
        {
            auto conn = std::make_shared< connection< RN > >(
                std::move( s ), run_number_, log_ );
            conn->run( );
        }

        io_context_t  context_;
        tcp::acceptor acceptor_;
        RN&           run_number_;
        std::ostream& log_;
    };

} // namespace daqd_run_number

#endif // DAQD_TRUNK_RUN_NUMBER_SERVER_HH
