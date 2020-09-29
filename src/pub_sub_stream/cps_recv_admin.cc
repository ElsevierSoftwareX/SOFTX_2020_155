//
// Created by jonathan.hanks on 9/28/20.
//

#include "cps_recv_admin.hh"
#include <sstream>
#include <thread>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/optional.hpp>

#include <rapidjson/document.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>

namespace ip = boost::asio::ip;
namespace http = boost::beast::http;
using tcp = boost::asio::ip::tcp;

namespace cps_admin
{
    namespace detail
    {
        tcp::endpoint
        parse_address( const std::string& interface )
        {
            auto pos = interface.find( ':' );
            if ( pos == std::string::npos || pos == 0 )
            {
                throw std::runtime_error(
                    "The interface definition must contain address:port" );
            }
            auto addr_string = interface.substr( 0, pos );
            auto port_str = interface.substr( pos + 1 );
            auto port =
                static_cast< unsigned short >( std::atoi( port_str.c_str( ) ) );

            ip::address address = ip::make_address( addr_string );
            return tcp::endpoint{ address, port };
        }

        struct Connection : std::enable_shared_from_this< Connection >
        {
            using response_type = http::response< http::string_body >;

            Connection( boost::asio::io_context& context,
                        std::vector< SubEntry >& subscriptions )
                : context_{ context }, s_{ context_ }, subscriptions_{
                      subscriptions
                  }
            {
            }

            void
            run( )
            {
                read_request( );
            }

            void
            read_request( )
            {
                req_ = {};
                http::async_read( s_,
                                  buffer_,
                                  req_,
                                  [self = shared_from_this( )](
                                      const boost::system::error_code& ec,
                                      std::size_t bytes_transferred ) {
                                      if ( ec )
                                      {
                                          return;
                                      }
                                      self->send_response(
                                          self->handle_request( ) );
                                  } );
            }

            void
            send_response( response_type&& resp )
            {
                auto resp_ptr =
                    std::make_shared< response_type >( std::move( resp ) );
                resp_ptr->keep_alive( false );
                http::async_write( s_,
                                   *resp_ptr,
                                   [self = shared_from_this( ), resp_ptr](
                                       const boost::system::error_code& ec,
                                       std::size_t bytes_transferred ) {
                                       // let it close.
                                   } );
            }

            response_type
            handle_request( )
            {
                if ( req_.target( ) == "/" || req_.target( ).empty( ) )
                {
                    return handle_index_view( );
                }
                auto opt_id = parse_sub_id_target( req_.target( ) );
                if ( opt_id )
                {
                    return handle_sub_id_view( opt_id.get( ) );
                }
                return not_found( );
            }

            static boost::optional< pub_sub::SubId >
            parse_sub_id_target( const boost::beast::string_view target )
            {
                boost::optional< pub_sub::SubId > sub_id{ boost::none };
                if ( target.size( ) >= 3 && target.starts_with( '/' ) &&
                     target.ends_with( '/' ) )
                {
                    auto middle = target.substr( 1, target.size( ) - 2 );
                    auto non_digit = middle.find_first_not_of( "0123456789" );
                    if ( non_digit == boost::beast::string_view::npos )
                    {
                        std::string digits( middle.begin( ), middle.end( ) );
                        sub_id = pub_sub::SubId( std::atoi( digits.c_str( ) ) );
                    }
                }
                return sub_id;
            }

            response_type
            handle_index_view( )
            {
                if ( req_.method( ) != http::verb::get )
                {
                    return bad_req_type( "Only GET supported" );
                }

                std::ostringstream                             os;
                rapidjson::OStreamWrapper                      json_out( os );
                rapidjson::Writer< rapidjson::OStreamWrapper > writer(
                    json_out );

                writer.StartArray( );

                std::for_each( subscriptions_.begin( ),
                               subscriptions_.end( ),
                               [&writer]( const SubEntry& entry ) {
                                   static const char* conn_str = "conn_str";
                                   writer.StartObject( );
                                   writer.Key( "id", 2, false );
                                   writer.Int64( entry.sub_id );
                                   writer.Key(
                                       conn_str, strlen( conn_str ), false );
                                   writer.String( entry.conn_str.c_str( ),
                                                  entry.conn_str.size( ),
                                                  true );
                                   writer.EndObject( 2 );
                               } );
                writer.EndArray( subscriptions_.size( ) );

                auto resp = response_type{ http::status::ok, req_.version( ) };
                resp.set( http::field::content_type, "application/json" );
                resp.keep_alive( false );
                resp.body( ) = os.str( );
                resp.prepare_payload( );
                return resp;
            }

            response_type
            handle_sub_id_view( pub_sub::SubId sub_id )
            {
                const static char* restart_requested = "Restart requested";

                if ( req_.method( ) != http::verb::post )
                {
                    return bad_req_type( "Only POST supported" );
                }
                auto it =
                    std::find_if( subscriptions_.begin( ),
                                  subscriptions_.end( ),
                                  [sub_id]( const SubEntry& entry ) -> bool {
                                      return entry.sub_id == sub_id;
                                  } );
                if ( it == subscriptions_.end( ) )
                {
                    return not_found( );
                }
                it->subscriber->reset_subscription( it->sub_id );

                std::ostringstream                             os;
                rapidjson::OStreamWrapper                      json_out( os );
                rapidjson::Writer< rapidjson::OStreamWrapper > writer(
                    json_out );
                writer.String( restart_requested );

                auto resp = response_type{ http::status::ok, req_.version( ) };
                resp.set( http::field::content_type, "application/json" );
                resp.keep_alive( false );
                resp.body( ) = os.str( );
                resp.prepare_payload( );
                return resp;
            }

            response_type
            bad_req_type( const std::string& msg )
            {
                std::string msg_prefix{ "Method not supported. " };
                auto resp = response_type{ http::status::method_not_allowed,
                                           req_.version( ) };
                resp.set( http::field::content_type, "text/plain" );
                resp.keep_alive( false );
                resp.body( ) = msg_prefix + msg;
                resp.prepare_payload( );
                return resp;
            }

            response_type
            not_found( )
            {
                auto resp =
                    response_type{ http::status::not_found, req_.version( ) };
                resp.set( http::field::content_type, "text/plain" );
                resp.keep_alive( false );
                resp.body( ) = "Not found";
                resp.prepare_payload( );
                return resp;
            }

            boost::asio::io_context& context_;
            tcp::socket              s_;

            http::request< http::string_body > req_{};
            boost::beast::flat_buffer          buffer_{};
            std::vector< SubEntry >&           subscriptions_;
        };

        struct AdminInterfaceIntl
        {
            using work_guard = boost::asio::executor_work_guard<
                typename boost::asio::io_context::executor_type >;
            AdminInterfaceIntl( std::string              interface,
                                std::vector< SubEntry >& subscriptions )
                : context_( ), work_guard_{ boost::asio::make_work_guard(
                                   context_ ) },
                  acceptor_( context_, parse_address( interface ) ), thread_{},
                  subscriptions_{ subscriptions }
            {
                context_.post( [this]( ) { start_accept( ); } );
                thread_ = std::thread( [this]( ) { context_.run( ); } );
            }

            void
            start_accept( )
            {
                auto conn =
                    std::make_shared< Connection >( context_, subscriptions_ );
                acceptor_.async_accept(
                    conn->s_,
                    [this, conn]( const boost::system::error_code& ec ) {
                        if ( ec == boost::system::errc::operation_canceled )
                        {
                            return;
                        }
                        if ( !ec )
                        {
                            conn->run( );
                        }
                        start_accept( );
                    } );
            }

            std::thread              thread_;
            boost::asio::io_context  context_;
            work_guard               work_guard_;
            tcp::acceptor            acceptor_;
            std::vector< SubEntry >& subscriptions_;
        };

    } // namespace detail

    AdminInterface::AdminInterface( const std::string&       interface,
                                    std::vector< SubEntry >& subscriptions )
        : p_( new detail::AdminInterfaceIntl( interface, subscriptions ) )
    {
    }

    AdminInterface::AdminInterface( AdminInterface&& ) noexcept = default;

    AdminInterface::~AdminInterface( ) = default;

    AdminInterface& AdminInterface::
                    operator=( AdminInterface&& ) noexcept = default;
} // namespace cps_admin