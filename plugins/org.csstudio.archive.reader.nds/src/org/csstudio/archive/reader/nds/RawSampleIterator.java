package org.csstudio.archive.reader.nds;

import org.csstudio.archive.reader.ValueIterator;
import org.csstudio.data.values.INumericMetaData;
import org.csstudio.data.values.ISeverity;
import org.csstudio.data.values.ITimestamp;
import org.csstudio.data.values.IValue;
import org.csstudio.data.values.TimestampFactory;
import org.csstudio.data.values.ValueFactory;

public class RawSampleIterator extends AbstractNDSValueIterator implements ValueIterator {

	long count;
	ITimestamp s, e;
	
	public RawSampleIterator(NDSArchiveReader ndsArchiveReader, String name,
			ITimestamp start, ITimestamp end) {
		this.s = start; this.e = end;
		count = e.seconds() - s.seconds();
	}

	@Override
	public boolean hasNext() {
		if (count > 0) {
			return true;
		} else {
			return false;
		}
	}

	@Override
	public IValue next() throws Exception {
        //final ITimestamp now = TimestampFactory.createTimestamp(block.getTimestamp (), block.getNano());
        final ITimestamp now = TimestampFactory.createTimestamp(e.seconds()-count,0);
        final ISeverity OK = ValueFactory.createOKSeverity();
        final INumericMetaData meta = ValueFactory.createNumericMetaData(-5000, +5000, 0, 0, 0, 0, 1, "counts");


        double []data = {123.446 * count};
		IValue value = ValueFactory.createDoubleValue(now, OK, OK.toString(),
					meta, IValue.Quality.Original, data);
		NDSArchiveReader.LOG.info("RawSampleIterator::next called " + now.seconds() + " val=" + data[0]);

		count--;
		return value;
	}

	@Override
	public void close() {
		// TODO Auto-generated method stub

	}

}
