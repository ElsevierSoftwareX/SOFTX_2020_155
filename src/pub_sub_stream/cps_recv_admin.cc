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

        /*!
         * @brief The Connection represents a client connection into the admin
         * interface.  It holds the buisiness logic of the interface, manages
         * request parsing, routing, and responses.  The http message support is
         * provided by boost::beast.
         *
         * @note At this time no matter what the keep alive policy of the
         * request the connection closes after the response is sent.
         */
        struct Connection : std::enable_shared_from_this< Connection >
        {
            using response_type = http::response< http::string_body >;

            Connection( boost::asio::io_context& context,
                        std::vector< SubEntry >& subscriptions,
                        DCStats&                 dc_stats )
                : context_{ context }, s_{ context_ },
                  subscriptions_{ subscriptions }, dc_stats_{ dc_stats }
            {
            }

            /*!
             * @brief Start running the client.  First read data, parse the
             * request, and then respond.
             */
            void
            run( )
            {
                read_request( );
            }

            /*!
             * @brief Perform a async read of the request, then trigger the
             * response to be created
             */
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

            /*!
             * @brief Transmit a response
             * @param resp the response message (headers + body) to send
             * @note Closes the connection
             */
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

            /*!
             * @brief do basic routing of the request and return a response
             * @return the reponse for the request
             */
            response_type
            handle_request( )
            {
                if ( req_.target( ) == "/" || req_.target( ).empty( ) )
                {
                    return handle_index_view( );
                }
                if ( req_.target( ) == "/clear_crc/" )
                {
                    return handle_clear_crc( );
                }
                auto opt_id = parse_sub_id_target( req_.target( ) );
                if ( opt_id )
                {
                    return handle_sub_id_view( opt_id.get( ) );
                }
                return not_found( );
            }

            /*!
             * @brief Helper to parse urls of the form "/<id>/" where id is an
             * integer value.
             * @param target The url to parse
             * @return An optional<SubId>, with a value if one could be parsed,
             * else empty
             */
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

            /*!
             * @brief the index view, list the subscriptions and ids in json
             * @return the response
             */
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

            /*!
             * @brief handle a request to clear the crcs
             * @return the response
             */
            response_type
            handle_clear_crc( )
            {
                if ( req_.method( ) != http::verb::post )
                {
                    return bad_req_type( "Only POST supported" );
                }
                dc_stats_.request_clear_crc( );

                auto resp = response_type{ http::status::ok, req_.version( ) };
                resp.set( http::field::content_type, "application/json" );
                resp.keep_alive( false );
                resp.body( ) = "clear crc requested";
                resp.prepare_payload( );
                return resp;
            }

            /*!
             * @brief handle a request to restart a subscription
             * @param sub_id the id of the subscription to restart
             * @return the appropriate response to send to the user.
             */
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

            /*!
             * @brief helper to return a method not allowed error message
             * @param msg optional message to add to the response
             * @return a http response object
             */
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

            /*!
             * @brief helper to return a not found error message
             * @return a http response object
             */
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
            DCStats&                           dc_stats_;
        };

        /*!
         * @brief The AdminInterfaceIntl manages the server socket and accept
         * loop for the AdminInterface.  It owns the thread and io_context that
         * client sockets are handled via.  Client connections are managed via
         * the Connection objects.
         */
        struct AdminInterfaceIntl
        {
            using work_guard = boost::asio::executor_work_guard<
                typename boost::asio::io_context::executor_type >;
            AdminInterfaceIntl( std::string              interface,
                                std::vector< SubEntry >& subscriptions,
                                DCStats&                 dc_stats )
                : context_( ), work_guard_{ boost::asio::make_work_guard(
                                   context_ ) },
                  acceptor_( context_, parse_address( interface ) ), thread_{},
                  subscriptions_{ subscriptions }, dc_stats_{ dc_stats }
            {
                context_.post( [this]( ) { start_accept( ); } );
                thread_ = std::thread( [this]( ) { context_.run( ); } );
            }

            void
            start_accept( )
            {
                auto conn = std::make_shared< Connection >(
                    context_, subscriptions_, dc_stats_ );
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
            DCStats&                 dc_stats_;
        };

    } // namespace detail

    AdminInterface::AdminInterface( const std::string&       interface,
                                    std::vector< SubEntry >& subscriptions,
                                    DCStats&                 dc_stats )
        : p_( new detail::AdminInterfaceIntl(
              interface, subscriptions, dc_stats ) )
    {
    }

    AdminInterface::AdminInterface( AdminInterface&& ) noexcept = default;

    AdminInterface::~AdminInterface( ) = default;

    AdminInterface& AdminInterface::
                    operator=( AdminInterface&& ) noexcept = default;
} // namespace cps_admin