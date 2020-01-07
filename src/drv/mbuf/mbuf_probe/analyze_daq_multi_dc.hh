//
// Created by jonathan.hanks on 10/11/19.
//

#ifndef DAQD_MBUF_ANALYZE_DAQ_MULTI_DC_HH
#define DAQD_MBUF_ANALYZE_DAQ_MULTI_DC_HH

#include <cstddef>

#include "mbuf_probe.hh"

namespace analyze
{
    void analyze_multi_dc( volatile void*    buffer,
                           std::size_t       size,
                           const ConfigOpts& options );
}

#endif // DAQD_MBUF_ANALYZE_DAQ_MULTI_DC_HH
