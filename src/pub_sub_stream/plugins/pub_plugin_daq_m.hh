//
// Created by jonathan.hanks on 4/28/20.
//
#ifndef DAQD_TRUNK_PUB_PLUGIN_DAQ_M_HH
#define DAQD_TRUNK_PUB_PLUGIN_DAQ_M_HH

#include <cds-pubsub/pub_plugin.hh>

namespace cps_plugins
{
    class PubPluginDaqMApi : public pub_sub::plugins::PublisherPluginApi
    {
    public:
        ~PubPluginDaqMApi( ) override = default;

        const std::string& prefix( ) const override;
        const std::string& version( ) const override;
        const std::string& name( ) const override;
        pub_sub::plugins::UniquePublisherInstance
        publish( const std::string&        address,
                 pub_sub::PubDebugNotices& debug_hooks ) override;
    };
} // namespace cps_plugins

#endif /* DAQD_TRUNK_PUB_PLUGIN_DAQ_M_HH */