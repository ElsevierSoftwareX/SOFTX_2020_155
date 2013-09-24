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

public class RawSampleIterator extends AbstractNDSValueIterator implements ValueIterator {
	DataBlock block = null;
	Net net = null;
	int samples_left = 0;
	static Logger LOG;
	DataInputStream block_in;
	int rate_hz = 0;
	int total_samples = 0;
	long block_time = 0;
	
    static {
    	LOG = Logger.getAnonymousLogger();
    }

	// This expect the request already sent and will be reading the blocks from the socket
	public RawSampleIterator(NDS_PV nds, int rateHz) {
			net = nds.getNet();
			rate_hz = rateHz;
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
		long period_nsec = (long)1e9 / rate_hz;
		//LOG.finest("NDS Raw Iterator =" + block.getTimestamp () + " dataLen=" + block.getWave().length);

        final ITimestamp now = TimestampFactory.createTimestamp(block_time + samples_sent/rate_hz, block.getNano() + samples_sent % rate_hz * period_nsec);
        final ISeverity OK = ValueFactory.createOKSeverity();
        final INumericMetaData meta = ValueFactory.createNumericMetaData(0, 0, 0, 0, 0, 0, 1, "counts");

		double a[] = {0.};
    	try {
    		double f=block_in.readFloat ();
    		//System.out.println (f);
    		a[0]=f;
    	} catch (IOException e) {
    		e.printStackTrace();
    	}
    	
    	IValue value = ValueFactory.createDoubleValue(now, OK, OK.toString(), meta, Quality.Original, a);

		//LOG.info("RawSampleIterator::next called " + now.seconds() + "." + now.nanoseconds() + " val=" + a [0]);
		//LOG.info("samples_sent=" + samples_sent);

		samples_left--;
		return value;
	}

	// Read the next block from the socket
	// Return true if no blocks were read (end of transmission)
	private boolean nextBlock() {
		block = net.getDataBlock ();
		if (block != null && !block.getEot()) {
			// See how many samples we got on this one and populate our counter
			samples_left = block.getPeriod() * rate_hz; // :TODO: Of course this only works with one sample per second request (full res)
			total_samples = block.getPeriod() * rate_hz;
			block_in = new DataInputStream (new ByteArrayInputStream (block.getWave()));
			block_time = TConvert.unix(block.getTimestamp ());
		}
		return block == null;
	}

	@Override
	public void close() {
		// TODO Auto-generated method stub

	}

}
