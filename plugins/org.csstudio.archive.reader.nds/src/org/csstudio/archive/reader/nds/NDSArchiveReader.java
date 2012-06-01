package org.csstudio.archive.reader.nds;

import java.util.ArrayList;
import java.util.Enumeration;
import java.util.logging.Logger;

import org.csstudio.archive.reader.ArchiveInfo;
import org.csstudio.archive.reader.ArchiveReader;
import org.csstudio.archive.reader.UnknownChannelException;
import org.csstudio.archive.reader.ValueIterator;
import org.csstudio.data.values.ITimestamp;
import org.csstudio.utility.pv.nds.*;

public class NDSArchiveReader implements ArchiveReader {

	static Logger LOG;
    final private String url;

    static {
    	LOG = Logger.getAnonymousLogger();
    }
    
	public NDSArchiveReader(String url) {
        this.url = url;
	}

	@Override
	public String getServerName() {
		return "NDS";
	}

	@Override
	public String getURL() {
		return url;	
	}

	@Override
	public String getDescription() {
        return "NDS Archive V" + getVersion() ;
	}

	@Override
	public int getVersion() {
		return 1;
	}

	@Override
	public ArchiveInfo[] getArchiveInfos() {
        return new ArchiveInfo[]
        {
            new ArchiveInfo("nds", "NDS Server", 1)
        };
	}

	@Override
	public String[] getNamesByPattern(int key, String glob_pattern)
			throws Exception {
		return searchNDSChannelsGlobOrRegex(glob_pattern, null);
	}
	
	@Override
	public String[] getNamesByRegExp(int key, String reg_exp) throws Exception {
		return searchNDSChannelsGlobOrRegex(null, reg_exp);
	}


	private String[] searchNDSChannelsGlobOrRegex(String glob_pattern, String regex_pattern)
			throws Exception, ClassNotFoundException {
		ChannelSet serverSet = getChannelList();
		ArrayList<String> sl = new ArrayList<String>();
		//LOG.info("Regex pattern is " + regex_pattern);

	    for (@SuppressWarnings("unchecked")
		Enumeration<Channel> e = serverSet.elements (); e.hasMoreElements ();) {
	        Channel ch = e.nextElement ();
	        if (matches(ch.getName(), glob_pattern, regex_pattern)) {
	        		sl.add("nds://" + ch.getName());
	        }
	      }

		return sl.toArray(new String[sl.size()]);
	}

	private ChannelSet getChannelList() throws Exception,
			ClassNotFoundException {
		final NDS_PV nds = new NDS_PV("");
		nds.connect();
		final Net net = nds.getNet();
		final Preferences preferences = nds.getPreferences();
		final ChannelSet cs = new ChannelSet(null, net);
		preferences.updateChannelSet(cs);
		final ChannelSet serverSet = cs.getServerSet();
		net.disconnect();

		return serverSet;
	}


	@Override
	public ValueIterator getRawValues(int key, String name, ITimestamp start,
			ITimestamp end) throws UnknownChannelException, Exception {
		LOG.info("NDsArchiveReader::getRawValues called on name=" + name + " start=" + start.seconds()
				+ " end=" + end.seconds());
		return new RawSampleIterator(this, name, start, end);
	}

	@Override
	public ValueIterator getOptimizedValues(int key, String name,
			ITimestamp start, ITimestamp end, int count)
					throws UnknownChannelException, Exception {
		LOG.info("NDsArchiveReader::getOptimizedValues called on name=" + name + " start=" + start.seconds()
				+ " end=" + end.seconds() + " count=" + count);
		return getRawValues(key, name, start, end) ;
	}

	@Override
	public void cancel() {
		// TODO Auto-generated method stub

	}

	@Override
	public void close() {
		// TODO Auto-generated method stub

	}
	
	/**
	 * Match text either on regex pattern on on glob pattern. Either glob or
	 * regex_pattern needs to be not null.
	 * 
	 * @param text Text to match
	 * @param glob Glob pattern to match or null
	 * @param regex_pattern Regex pattern to match (takes precedence) or null
	 * @return
	 */
	public static boolean matches(String text, String glob, String regex_pattern) {
		if (regex_pattern != null) {
			return text.matches(regex_pattern);
		}
		String rest = null;
		int pos = glob.indexOf('*');
		if (pos != -1) {
			rest = glob.substring(pos + 1);
			glob = glob.substring(0, pos);
		}

		if (glob.length() > text.length())
			return false;

		// handle the part up to the first *
		for (int i = 0; i < glob.length(); i++)
			if (glob.charAt(i) != '?' 
			&& !glob.substring(i, i + 1).equalsIgnoreCase(text.substring(i, i + 1)))
				return false;

		// recurse for the part after the first *, if any
		if (rest == null) {
			return glob.length() == text.length();
		} else {
			for (int i = glob.length(); i <= text.length(); i++) {
				if (matches(text.substring(i), rest, null))
					return true;
			}
			return false;
		}
	}

}
