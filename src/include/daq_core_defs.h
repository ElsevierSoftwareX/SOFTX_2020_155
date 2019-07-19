#ifndef DAQ_CORE_DEFS_H
#define DAQ_CORE_DEFS_H

/**
 * This file holds a few defines that are common between the daqd, FE data layout, and the zmq/mx/ix streams.
 * It is broken out to keep the definitions in one file, not in two places that may conflict if changes are made.
 */

#define DAQ_NUM_DATA_BLOCKS     16                  ///< Number of DAQ data blocks
#define DAQ_NUM_DATA_BLOCKS_PER_SECOND  16          ///< Number of DAQ data blocks to xfer each second

#define DCU_COUNT 256		///< MAX number of real-time DAQ processes in single control system

#define DAQ_DCU_SIZE            0x400000            ///< MAX data in bytes/sec allowed per process
#define DAQ_DCU_BLOCK_SIZE      (DAQ_DCU_SIZE/DAQ_NUM_DATA_BLOCKS)   ///< Size of one DAQ data block


#endif /*  DAQ_CORE_DEFS_H */