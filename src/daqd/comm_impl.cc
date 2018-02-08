//
// Created by jonathan.hanks on 2/7/18.
// This file is pieces of logic pulled out of the bison/yacc file comm.y.
// They have been pulled out to make it easier to consume with external
// tools and debuggers.  Also the hope is that removing all the ${1}...
// makes the code easier to reason about.
//
#include "comm_impl.hh"

#include "config.h"

#include "daqd.hh"

extern daqd_c daqd;

namespace comm_impl {

    void configure_channels_body_begin_end()
    {
        int i, j, k, offs;
        int rm_offs = 0;

        // Configure channels from files
        if (daqd.configure_channels_files ()) exit(1);
        int cur_dcu = -1;

        // Save channel offsets and assign trend channel data struct
        for (i = 0, offs = 0; i < daqd.num_channels + 1; i++) {

            int t = 0;
            if (i == daqd.num_channels) t = 1;
            else t = cur_dcu != -1 && cur_dcu != daqd.channels[i].dcu_id;

            if (IS_TP_DCU(daqd.channels[i].dcu_id) && t) {
                // Remember the offset to start of TP/EXC DCU data
                // in main buffer
                daqd.dcuTPOffset[daqd.channels[i].ifoid][daqd.channels[i].dcu_id] = offs;
            }

            // Finished with cur_dcu: channels sorted by dcu_id
            if (IS_MYRINET_DCU(cur_dcu) && t) {
                int dcu_size = daqd.dcuSize[daqd.channels[i-1].ifoid][cur_dcu];
                daqd.dcuDAQsize[daqd.channels[i-1].ifoid][cur_dcu] = dcu_size;

#ifndef USE_BROADCAST
                // When compiled with USE_BROADCAST we do not allocate contigious memory
                // in the move buffer for the test points. We allocate this memory later
                // at the end of the buffer.

                // Save testpoint data offset
                daqd.dcuTPOffset[daqd.channels[i-1].ifoid][cur_dcu] = offs;
                daqd.dcuTPOffsetRmem[daqd.channels[i-1].ifoid][cur_dcu] = rm_offs;

                // Allocate testpoint data buffer for this DCU
                int tp_buf_size = 2*DAQ_DCU_BLOCK_SIZE*DAQ_NUM_DATA_BLOCKS_PER_SECOND - dcu_size*DAQ_NUM_DATA_BLOCKS_PER_SECOND;
                offs += tp_buf_size;
                daqd.block_size += tp_buf_size;
                rm_offs += 2*DAQ_DCU_BLOCK_SIZE - dcu_size;
                DEBUG1(cerr << "Configured MYRINET DCU, block size=" << tp_buf_size << endl);
                DEBUG1(cerr << "Myrinet DCU size " << dcu_size << endl);
                daqd.dcuSize[daqd.channels[i-1].ifoid][cur_dcu] = 2*DAQ_DCU_BLOCK_SIZE;
#endif
            }
            if (!IS_MYRINET_DCU(cur_dcu) && t) {
                daqd.dcuDAQsize[daqd.channels[i-1].ifoid][cur_dcu] = daqd.dcuSize[daqd.channels[i-1].ifoid][cur_dcu];
            }
            if (i == daqd.num_channels) continue;

            cur_dcu = daqd.channels[i].dcu_id;

            daqd.channels [i].bytes = daqd.channels [i].sample_rate * daqd.channels [i].bps;

#ifdef USE_BROADCAST

            #ifndef GDS_TESTPOINTS
		// Skip testpoints
		if (IS_TP_DCU(cur_dcu)) continue;
#else
		// We want to keep TP/EXC DCU data at the very end of the buffer
		// so we an receive the broadcast into the move buffer directly
		if (IS_TP_DCU(cur_dcu) && !IS_GDS_ALIAS (daqd.channels [i])) {
		  daqd.dcuSize[daqd.channels[i].ifoid][cur_dcu]
			      += daqd.channels [i].bytes / DAQ_NUM_DATA_BLOCKS_PER_SECOND;
		  printf("ifo %d dcu %d channel %d size %d\n",
			daqd.channels[i].ifoid, cur_dcu, i, daqd.channels [i].bytes);
		  if (IS_EXC_DCU(cur_dcu))
			  daqd.dcuSize[daqd.channels[i].ifoid][cur_dcu] += 4;
		  continue;
		}
#endif
#endif


#ifdef GDS_TESTPOINTS
            if (IS_GDS_ALIAS (daqd.channels [i])) {
			daqd.channels [i].offset = 0;
		} else {
#endif
            daqd.channels [i].offset = offs;

            offs += daqd.channels [i].bytes;
            daqd.block_size += daqd.channels [i].bytes;

            daqd.dcuSize[daqd.channels[i].ifoid][daqd.channels[i].dcu_id]
                    += daqd.channels [i].bytes / DAQ_NUM_DATA_BLOCKS_PER_SECOND;
            daqd.channels [i].rm_offset = rm_offs;
            rm_offs += daqd.channels [i].bytes / DAQ_NUM_DATA_BLOCKS_PER_SECOND;
            /* For the EXC DCUs: add the status word length */
            if (IS_EXC_DCU(daqd.channels[i].dcu_id)) {
                daqd.dcuSize[daqd.channels[i].ifoid][daqd.channels[i].dcu_id] += 4;
                rm_offs += 4;
                daqd.channels [i].rm_offset += 4;
            }

#ifdef GDS_TESTPOINTS
            }
#endif

            if (!daqd.broadcast_set.empty()) {
                // Activate configured DMT broadcast channels
                daqd.channels [i].active = daqd.broadcast_set.count(daqd.channels [i].name);
            }

            if (daqd.channels [i].trend) {
                if (daqd.trender.num_channels >= trender_c::max_trend_channels) {
                    system_log(1, "too many trend channels. No trend on `%s' channel", daqd.channels [i].name);
                } else {
                    daqd.trender.channels [daqd.trender.num_channels] = daqd.channels [i];
                    daqd.trender.block_size += sizeof (trend_block_t);
                    if (daqd.trender.channels[daqd.trender.num_channels].data_type == _32bit_complex) {
                        /* Change data type from complex to float but leave bps field at 8 */
                        /* Trend loop function checks bps variable to detect complex data */
                        daqd.trender.channels[daqd.trender.num_channels].data_type = _32bit_float;
#if 0
                        daqd.trender.channels[daqd.trender.num_channels].bps /= 2;
				  daqd.trender.channels[daqd.trender.num_channels].bytes /= 2;
#endif
                    }

                    daqd.trender.num_channels++;
                }
            }
        }


#ifdef USE_BROADCAST
        for (int ifo = 0; ifo < daqd.data_feeds; ifo++) {
	 for (int mdcu = 0; mdcu < DCU_COUNT; mdcu++) {
	  printf("ifo %d dcu %d size %d\n", ifo, mdcu, daqd.dcuSize[ifo][mdcu]);
	  if (0 == daqd.dcuSize[ifo][mdcu]) continue;
	  if (IS_TP_DCU(mdcu) || IS_MYRINET_DCU(mdcu)) {

	    // Save testpoint data offset, at the end of the buffer
	    daqd.dcuTPOffset[ifo][mdcu] = daqd.block_size;

	    // Allocate memory for the test points' buffers at the end of a move buffer
	    daqd.dcuTPOffsetRmem[ifo][mdcu] = rm_offs;

	    // One test point data size
	    int tp_size = daqd.dcuRagmDaqIpcte[ifo][mdcu] * 4 / DAQ_NUM_DATA_BLOCKS_PER_SECOND;
	    int tp_buf_size = 0;
	    if (IS_TP_DCU(mdcu)) {
#if 0
	      int tp_index = mdcu - DCU_ID_FIRST_GDS;
              int tp_table_len = daqGdsTpNum[tp_index];
              if (tp_table_len > DAQ_GDS_MAX_TP_ALLOWED)  {
                	tp_table_len = DAQ_GDS_MAX_TP_ALLOWED;
              }
	      tp_buf_size = tp_size * tp_table_len;
#endif
	      tp_buf_size = daqd.dcuSize[ifo][mdcu];
	      cerr << "Legacy TP DCU " << mdcu <<" tp data size " << tp_buf_size << endl;

	      // Now set the correct offsets for each channel within this DCU
	      int dcu_chnum = 0;
	      for (int chnum = 0; chnum < daqd.num_channels; chnum++) {
		if (daqd.channels[chnum].dcu_id == mdcu && daqd.channels[chnum].ifoid == ifo) {
		   daqd.channels [chnum].offset = daqd.block_size
			+ dcu_chnum * tp_size * DAQ_NUM_DATA_BLOCKS_PER_SECOND;
		   daqd.channels [chnum].rm_offset = rm_offs
			+ dcu_chnum * tp_size;
		   dcu_chnum++;
		}
	      }
	    } else {
	      // Allocate testpoint data buffer for this DCU
	      tp_buf_size = DAQ_GDS_MAX_TP_ALLOWED * tp_size;
	      if (tp_buf_size > 2*DAQ_DCU_BLOCK_SIZE) {
		// Limit the number of testpoints for this DCU
		tp_buf_size = 2*DAQ_DCU_BLOCK_SIZE;
	      }
	      cerr << "Myrinet DCU " << mdcu <<" tp data size " << tp_buf_size << endl;
	    }
	    rm_offs += tp_buf_size;
	    daqd.block_size += tp_buf_size * DAQ_NUM_DATA_BLOCKS_PER_SECOND;
	  }
	 }
	}
#endif


        // Calculate memory size needed for channels status storage in the main buffer
        // and increment main block size
        daqd.block_size += 17 * sizeof(int) * daqd.num_channels;

        daqd.trender.num_trend_channels = 0;
        // Assign trend output channels
        for (i = 0, offs = 0; i < daqd.trender.num_channels; i++) {
            int l = strlen(daqd.trender.channels[i].name);
            if (l > (MAX_CHANNEL_NAME_LENGTH - 6)) {
                printf("Channel %s length %d over the limit of %d\n",
                       daqd.trender.channels[i].name,
                       (int)strlen(daqd.trender.channels[i].name),
                       MAX_CHANNEL_NAME_LENGTH - 6);
                exit(1);
            }
            for (int j = 0; j < trender_c::num_trend_suffixes; j++) {
                daqd.trender.trend_channels [daqd.trender.num_trend_channels]
                        = daqd.trender.channels [i];

                // Add a suffix to the channel name
                strcat (daqd.trender.trend_channels [daqd.trender.num_trend_channels].name,
                        daqd.trender.sufxs [j]);

                // Set the sample rate always to one, since
                // we have fixed trend calculation period equal to one second
                daqd.trender.trend_channels [daqd.trender.num_trend_channels].sample_rate = 1;

                switch (j) {
                    case 0:
                    case 1:
                        if (daqd.trender.channels [i].data_type != _64bit_double && daqd.trender.channels [i].data_type != _32bit_float)
                            daqd.trender.trend_channels [daqd.trender.num_trend_channels].data_type = _32bit_integer;
                        else
                            daqd.trender.trend_channels [daqd.trender.num_trend_channels].data_type = daqd.trender.channels [i].data_type;
                        break;
                    case 2: // `n'
                        daqd.trender.trend_channels [daqd.trender.num_trend_channels].data_type = _32bit_integer;
                        break;
                    case 3: // `mean'
                    case 4: // `rms'
                        daqd.trender.trend_channels [daqd.trender.num_trend_channels].data_type = _64bit_double;
                        break;
                }

                daqd.trender.trend_channels [daqd.trender.num_trend_channels].bps
                        = daqd.trender.bps [daqd.trender.channels [i].data_type] [j];
                daqd.trender.trend_channels [daqd.trender.num_trend_channels].bytes
                        = daqd.trender.trend_channels [daqd.trender.num_trend_channels].bps
                          * daqd.trender.trend_channels [daqd.trender.num_trend_channels].sample_rate;
                daqd.trender.trend_channels [daqd.trender.num_trend_channels].offset = daqd.trender.toffs [j] + offs;
                daqd.trender.num_trend_channels++;
            }
            offs += sizeof (trend_block_t);
        }

        DEBUG1(cerr << "Configured " << daqd.num_channels << " channels" << endl);
        DEBUG1(cerr << "Configured " << daqd.trender.num_channels << " trend channels" << endl);
        DEBUG1(cerr << "Configured " << daqd.trender.num_trend_channels << " output trend channels" << endl);
        DEBUG1(cerr << "comm.y: daqd block_size=" << daqd.block_size << endl);

    }

}