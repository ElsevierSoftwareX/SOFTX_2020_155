//
// Created by jonathan.hanks on 10/10/19.
//

#ifndef DAQD_MBUF_ANALYSE_RMIPC_HH
#define DAQD_MBUF_ANALYSE_RMIPC_HH

#include <cstddef>

#include "mbuf_probe.hh"

namespace analyze
{
    void analyze_rmipc( volatile void*    buffer,
                        std::size_t       size,
                        const ConfigOpts& options );
}

#endif // DAQD_MBUF_ANALYSE_RMIPC_HH
