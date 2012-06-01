package org.csstudio.archive.reader.nds;

import org.csstudio.archive.reader.ArchiveReader;
import org.csstudio.archive.reader.ArchiveReaderFactory;

public class NDSArchiveReaderFactory implements ArchiveReaderFactory {

	@Override
	public ArchiveReader getArchiveReader(String url) throws Exception {
		// TODO Auto-generated method stub
		return new NDSArchiveReader(url);
	}

}
