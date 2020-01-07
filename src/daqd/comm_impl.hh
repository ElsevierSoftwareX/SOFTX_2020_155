//
// Created by jonathan.hanks on 2/7/18.
// This file is pieces of logic pulled out of the bison/yacc file comm.y.
// They have been pulled out to make it easier to consume with external
// tools and debuggers.  Also the hope is that removing all the ${1}...
// makes the code easier to reason about.
//

#ifndef DAQD_TRUNK_COMM_IMPL_HH

#include <ostream>

namespace comm_impl
{

    extern void configure_channels_body_begin_end( );

}

#define DAQD_TRUNK_COMM_IMPL_HH

#endif // DAQD_TRUNK_COMM_IMPL_HH
