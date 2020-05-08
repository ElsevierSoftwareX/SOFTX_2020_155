//
// Created by jonathan.hanks on 4/13/20.
//

#ifndef DAQD_TRUNK_SUB_PLUGIN_DAQ_M_HH
#define DAQD_TRUNK_SUB_PLUGIN_DAQ_M_HH

#include <vector>
#include <cds-pubsub/sub_plugin.hh>

namespace cps_plugins
{
    class SubPluginDaqMApi : public pub_sub::plugins::SubscriptionPluginApi
    {
    public:
        SubPluginDaqMApi( );
        ~SubPluginDaqMApi( ) override;

        const std::string& prefix( ) const override;

        const std::string& version( ) const override;

        const std::string& name( ) const override;

        pub_sub::SubId subscribe( const std::string&        address,
                                  pub_sub::SubDebugNotices& debug_hooks,
                                  pub_sub::SubHandler       handler ) override;

    private:
        std::vector< std::unique_ptr< pub_sub::plugins::Subscription > >
            subscriptions_;
    };
} // namespace cps_plugins

#endif // DAQD_TRUNK_SUB_PLUGIN_DAQ_M_HH
