//
// Created by jonathan.hanks on 12/20/19.
//
#include "simple_pv.h"
#include "simple_epics.hh"

namespace simple_epics
{

}

extern "C" {

simple_pv_handle
simple_pv_server_create( SimplePV* pvs, int pv_count )
{
    if ( !pvs || pv_count <= 0 )
    {
        return nullptr;
    }
    return nullptr;
}

void
simple_pv_server_update( simple_pv_handle server )
{
    if ( server == nullptr )
    {
        return;
    }
}

void
simple_pv_server_destroy( simple_pv_handle* server )
{
    if ( server == nullptr || *server == nullptr )
    {
        return;
    }
}
}