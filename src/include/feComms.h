///	@file feComms.h
///	@brief Defines structure for data passing between EPICS and real-time
///process.

#include FE_HEADER

/// Structure for passing data between EPICS and Real-time Code. \n
/// Original use was via RFM network, but now used in shared memory.
typedef struct RFM_FE_COMMS
{
    union
    {
        char pad[ 0x1000 ]; ///< Reserved for 5579 cntrl reg
        struct
        {
            unsigned int awgtpman_gps; ///< awgtpman passes its current GPS time
                                       ///< seconds to the FE for checking
            unsigned int feDaqBlockSize; ///< Front-end passes its current DAQ
                                         ///< block size so awgtpman can figure
                                         ///< out the maximum number of TPs
        };
    } padSpace;
    union
    { /* Starts at 	0x0000 0040 */
        char      sysepics[ 0x100000 ];
        CDS_EPICS epicsShm; ///< EPICS shared memory space
    } epicsSpace;
    union
    { /*		0x0000 1000 */
        char     sysdsp[ 0x200000 ];
        FILT_MOD epicsDsp; ///< Filter module input/output data space.
    } dspSpace
#ifdef PNM
        [ NUM_SYSTEMS ]
#endif
        ;
    union
    { /*		0x0000 2000 */
        char     syscoeff[ 0x400000 ];
        VME_COEF epicsCoeff; ///< Filter module coefficient info space.
    } coeffSpace
#ifdef PNM
        [ NUM_SYSTEMS ]
#endif
        ;
} RFM_FE_COMMS;
