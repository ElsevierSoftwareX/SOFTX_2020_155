//
// Created by jonathan.hanks on 8/13/20.
//

#ifndef DAQD_TRUNK_CHECK_SIZE_HH
#define DAQD_TRUNK_CHECK_SIZE_HH

#include <cstddef>

#include "mbuf_probe.hh"

namespace check_mbuf_sizes
{
    int check_size_rmipc( volatile void*    buffer,
                          std::size_t       buffer_Size,
                          const ConfigOpts& options );
    int check_size_multi( volatile void*    buffer,
                          std::size_t       buffer_Size,
                          const ConfigOpts& options );
} // namespace check_mbuf_sizes

#endif // DAQD_TRUNK_CHECK_SIZE_HH
