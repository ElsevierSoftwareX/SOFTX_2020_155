//
// Created by jonathan.hanks on 9/28/20.
//

#ifndef DAQD_TRUNK_CPS_RECV_ADMIN_HH
#define DAQD_TRUNK_CPS_RECV_ADMIN_HH

#include <memory>
#include <vector>

#include <cds-pubsub/sub.hh>
#include "dc_stats.hh"

namespace cps_admin
{
    namespace detail
    {
        struct AdminInterfaceIntl;
    }

    /*!
     * @brief A SubEntry is the record of a subscription that the admin
     * interface should report on and manage.
     */
    struct SubEntry
    {
        std::string          conn_str;
        pub_sub::SubId       sub_id;
        pub_sub::Subscriber* subscriber;
    };

    /*!
     * @brief The AdminInterface represents an administrative interface into the
     * cps_recv service that runs as small web server.
     */
    class AdminInterface
    {
    public:
        /*!
         * @brief create the admin interface and start its http server.
         * @param interface The address and port to listen on
         * @param subscriptions The list of subscriptions to manage, these must
         * be valid for the lifetime of the AdminInterface
         */
        AdminInterface( const std::string&       interface,
                        std::vector< SubEntry >& subscriptions,
                        DCStats&                 dc_stats );
        /*!
         * Close down the AdminInterface and stop its web server.
         */
        ~AdminInterface( );

        AdminInterface( const AdminInterface& ) = delete;
        AdminInterface( AdminInterface&& ) noexcept;

        AdminInterface& operator=( const AdminInterface& ) = delete;
        AdminInterface& operator=( AdminInterface&& ) noexcept;

    private:
        std::unique_ptr< detail::AdminInterfaceIntl > p_;
    };
} // namespace cps_admin

#endif // DAQD_TRUNK_CPS_RECV_ADMIN_HH
