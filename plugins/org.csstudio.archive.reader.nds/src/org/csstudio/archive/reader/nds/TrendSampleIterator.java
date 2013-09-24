package org.csstudio.archive.reader.nds;

import java.io.ByteArrayInputStream;
import java.io.DataInputStream;
import java.io.IOException;
import java.util.logging.Logger;

import org.csstudio.archive.reader.ValueIterator;
import org.csstudio.data.values.IMinMaxDoubleValue;
import org.csstudio.data.values.INumericMetaData;
import org.csstudio.data.values.ISeverity;
import org.csstudio.data.values.ITimestamp;
import org.csstudio.data.values.IValue;
import org.csstudio.data.values.TimestampFactory;
import org.csstudio.data.values.ValueFactory;
import org.csstudio.data.values.IValue.Quality;
import org.csstudio.utility.pv.nds.DataBlock;
import org.csstudio.utility.pv.nds.NDS_PV;
import org.csstudio.utility.pv.nds.Net;

public class TrendSampleIterator extends AbstractNDSValueIterator implements ValueIterator {
	DataBlock block = null;
	Net net = null;
	int samples_left = 0;
	static Logger LOG;
	DataInputStream block_in_mean;
	DataInputStream block_in_min;
	DataInputStream block_in_max;

	//int rate_hz = 1; // Second trend comes in always at 1 Hz
	int total_samples = 0;
	
    static {
    	LOG = Logger.getAnonymousLogger();
    }

	// This expect the request already sent and will be reading the blocks from the socket
	public TrendSampleIterator(NDS_PV nds) {
			net = nds.getNet();
	}

	@Override
	public boolean hasNext() {
		if (block == null || samples_left < 1) {
			// Try get next
			if (nextBlock()) {
				net.disconnect();
				return false;
			}
		}
		return samples_left > 0;
	}

	@Override
	public IValue next() throws Exception {
		// See if we need to read the next block from the socket
		if (block == null || samples_left < 1) {
			// Try get next
			nextBlock();
		}
		if (samples_left < 1) {
			net.disconnect();
			throw new Exception();
		}
		

		int samples_sent = total_samples - samples_left;
		//LOG.finest("NDS Raw Iterator =" + block.getTimestamp () + " dataLen=" + block.getWave().length);

        final ITimestamp now = TimestampFactory.createTimestamp(TConvert.unix(block.getTimestamp () + samples_sent), 0);
        final ISeverity OK = ValueFactory.createOKSeverity();
        final INumericMetaData meta = ValueFactory.createNumericMetaData(0, 0, 0, 0, 0, 0, 1, "counts");

		double mean[] = {0.};
		double min = 0, max = 0;
    	try {
    		 mean[0] = block_in_mean.readDouble ();
    		 min = block_in_min.readFloat ();
    		 max = block_in_max.readFloat ();
    	} catch (IOException e) {
    		e.printStackTrace();
    	}
    	
    	IValue value = ValueFactory.createMinMaxDoubleValue(now, OK, OK.toString(), meta, Quality.Original, mean, min, max);

		//LOG.info("TrendSampleIterator::next called " + now.seconds() + "." + now.nanoseconds() + " val=" + a [0]);
		//LOG.info("samples_sent=" + samples_sent);
    	
		samples_left--;
		return value;
	}

	// Read the next block from the socket
	// Return true if no blocks were read (end of transmission)
	private boolean nextBlock() {
		block = net.getDataBlock ();
		if (block != null && !block.getEot()) {
			try {
				// See how many samples we got on this one and populate our
				// counter
				samples_left = block.getPeriod();
				total_samples = block.getPeriod();
				block_in_mean = new DataInputStream(new ByteArrayInputStream(
						block.getWave()));
				
				block_in_min = new DataInputStream(new ByteArrayInputStream(
						block.getWave()));
				block_in_min.skip(8 * total_samples);
				block_in_max = new DataInputStream(new ByteArrayInputStream(
						block.getWave()));
				block_in_max.skip(12 * total_samples);
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
		return block == null;
	}

	@Override
	public void close() {
		// TODO Auto-generated method stub

	}

}
