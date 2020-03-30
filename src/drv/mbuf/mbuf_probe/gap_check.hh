//
// Created by jonathan.hanks on 3/13/20.
//

#ifndef DAQD_TRUNK_GAP_CHECK_HH
#define DAQD_TRUNK_GAP_CHECK_HH

#include <cstdint>

namespace check_gap
{
    extern int check_gaps( volatile void* buffer, std::size_t buffer_size );
}

#endif // DAQD_TRUNK_GAP_CHECK_HH
