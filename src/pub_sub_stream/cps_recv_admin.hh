//
// Created by jonathan.hanks on 9/28/20.
//

#ifndef DAQD_TRUNK_CPS_RECV_ADMIN_HH
#define DAQD_TRUNK_CPS_RECV_ADMIN_HH

#include <cds-pubsub/sub.hh>

#include <memory>
#include <vector>

namespace cps_admin
{
    namespace detail
    {
        struct AdminInterfaceIntl;
    }

    struct SubEntry
    {
        std::string          conn_str;
        pub_sub::SubId       sub_id;
        pub_sub::Subscriber* subscriber;
    };

    class AdminInterface
    {
    public:
        AdminInterface( const std::string&       interface,
                        std::vector< SubEntry >& subscriptions );
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
