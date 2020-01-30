#ifndef EPICS_PVS_HH
#define EPICS_PVS_HH

#include "atomic.hh"
#include "daqmap.h"

namespace PV
{

    enum PV_NAME
    {
        PV_CYCLE = 0,
        PV_TOTAL_CHANS = 1,
        PV_DATA_RATE = 2,
        PV_EDCU_CHANS = 3,
        PV_EDCU_CONN_CHANS = 4,
        PV_UPTIME_SECONDS = 5,
        PV_LOOKBACK_RAM = 6,
        PV_LOOKBACK_FULL = 7,
        PV_LOOKBACK_DIR = 8,
        PV_LOOKBACK_STREND = 9,
        PV_LOOKBACK_STREND_DIR = 10,
        PV_LOOKBACK_MTREND = 11,
        PV_LOOKBACK_MTREND_DIR = 12,
        PV_FAST_DATA_CRC = 13,
        PV_FAULT = 14,
        PV_BCAST_RETR = 15,
        PV_BCAST_FAILED_RETR = 16,
        PV_GPS = 17,
        PV_CHANS_SAVED = 18,
        PV_FRAME_SIZE = 19,
        PV_SCIENCE_FRAME_SIZE = 20,
        PV_SCIENCE_TOTAL_CHANS = 21,
        PV_SCIENCE_CHANS_SAVED = 22,
        PV_FRAME_WRITE_SEC = 23,
        PV_SCIENCE_FRAME_WRITE_SEC = 24,
        PV_SECOND_FRAME_WRITE_SEC = 25,
        PV_MINUTE_FRAME_WRITE_SEC = 26,
        PV_SECOND_FRAME_SIZE,
        PV_MINUTE_FRAME_SIZE,
        PV_RETRANSMIT_TOTAL,
        // Main producer thread timings
        PV_PRDCR_TIME_FULL_MEAN_MS,
        PV_PRDCR_TIME_FULL_MIN_MS,
        PV_PRDCR_TIME_FULL_MAX_MS,
        PV_PRDCR_TIME_RECV_MEAN_MS,
        PV_PRDCR_TIME_RECV_MIN_MS,
        PV_PRDCR_TIME_RECV_MAX_MS,
        // Producer/CRC timings
        PV_PRDCR_CRC_TIME_FULL_MEAN_MS,
        PV_PRDCR_CRC_TIME_FULL_MIN_MS,
        PV_PRDCR_CRC_TIME_FULL_MAX_MS,
        PV_PRDCR_CRC_TIME_CRC_MEAN_MS,
        PV_PRDCR_CRC_TIME_CRC_MIN_MS,
        PV_PRDCR_CRC_TIME_CRC_MAX_MS,
        PV_PRDCR_CRC_TIME_XFER_MEAN_MS,
        PV_PRDCR_CRC_TIME_XFER_MIN_MS,
        PV_PRDCR_CRC_TIME_XFER_MAX_MS,
        // Profiler buffer values
        PV_PROFILER_FREE_SEGMENTS_MAIN_BUF,
        // Sate informatino for frame writers
        PV_RAW_FW_STATE, // frame writer IO thread state
        PV_RAW_FW_DATA_STATE, // frame writer data thread state
        PV_RAW_FW_DATA_SEC, // processing time for fw data thread
        PV_SCIENCE_FW_STATE,
        PV_SCIENCE_FW_DATA_STATE,
        PV_SCIENCE_FW_DATA_SEC,
        PV_STREND_FW_STATE,
        PV_MTREND_FW_STATE,
        // checksum information on frames
        PV_FRAME_CHECK_SUM_TRUNC,
        PV_SCIENCE_FRAME_CHECK_SUM_TRUNC,
        PV_SECOND_FRAME_CHECK_SUM_TRUNC,
        PV_MINUTE_FRAME_CHECK_SUM_TRUNC,
        // run/configuration numbers
        PV_CONFIGURATION_NUMBER,
        // raw trend writer state
        PV_RAW_MTREND_TW_STATE,   // is the raw trend writer currently writing
        PV_RAW_MTREND_TW_WRITE_SEC,     // processing time for raw trend data
        MAX_PV
    };

}

// FIXME: how can we expose this nicely.
extern daqd_atomic_uint epicsDcuStatus[ 3 ][ 2 ][ DCU_COUNT ];

extern daqd_atomic_uint pvValue[ PV::MAX_PV ];

namespace PV
{

    static inline void
    set_pv( PV_NAME pv, unsigned int value )
    {
        daqd_store_val( ::pvValue[ pv ], value );
    }

    static inline daqd_atomic_uint&
    pv( PV_NAME pv )
    {
        return ::pvValue[ pv ];
    }

    static inline void
    incr_pv( PV_NAME pv )
    {
        ::pvValue[ pv ]++;
    }

    static inline void
    incr_pv( PV_NAME pv, unsigned int n )
    {
        daqd_fetch_add( ::pvValue[ pv ], n );
    }

} // namespace PV

#endif // EPICS_PVS_HH
