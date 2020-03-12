//
// Created by jonathan.hanks on 12/20/19.
//
#include "simple_pv.h"
#include "simple_epics.hh"
#include "simple_epics_internal.hh"

#include <algorithm>
#include <memory>
#include <vector>

#include "fdManager.h"

extern "C" {

simple_pv_handle
simple_pv_server_create( const char* prefix, SimplePV* pvs, int pv_count )
{
    if ( !pvs || pv_count <= 0 )
    {
        return nullptr;
    }
    std::unique_ptr< simple_epics::Server > server =
        simple_epics::detail::make_unique_ptr< simple_epics::Server >( );

    const std::string prefix_ = ( prefix ? prefix : "" );

    auto pv_server = server.get();
    std::for_each(
        pvs,
        pvs + pv_count,
        [&prefix_, pv_server]( const SimplePV& pv ) -> void {
            if ( !pv.name || !pv.data )
            {
                return;
            }
            switch ( pv.pv_type )
            {
            case SIMPLE_PV_INT:
            {
                pv_server->addPV( simple_epics::pvIntAttributes(
                    prefix_ + pv.name,
                    reinterpret_cast< int* >( pv.data ),
                    std::make_pair( pv.alarm_low, pv.alarm_high ),
                    std::make_pair( pv.warn_low, pv.warn_high ) ) );
            }
            break;
            case SIMPLE_PV_STRING:
            {
                pv_server->addPV( simple_epics::pvStringAttributes(
                    prefix_ + pv.name, reinterpret_cast< char* >( pv.data ) ) );
            }
            break;
            }
        } );

    return server.release( );
}

void
simple_pv_server_update( simple_pv_handle server )
{
    auto server_ = reinterpret_cast< simple_epics::Server* >( server );
    if ( server_ == nullptr )
    {
        return;
    }
    server_->update( );
    fileDescriptorManager.process( 0. );
}

void
simple_pv_server_destroy( simple_pv_handle* server )
{
    if ( server == nullptr || *server == nullptr )
    {
        return;
    }
    std::unique_ptr< simple_epics::Server > server_(
        reinterpret_cast< simple_epics::Server* >( *server ) );
}
}
