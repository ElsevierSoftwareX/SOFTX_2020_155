package org.csstudio.utility.pv.nds;

import java.io.ByteArrayInputStream;
import java.io.DataInputStream;
import java.io.IOException;
import java.util.Date;
import java.util.HashMap;
import java.util.Iterator;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Queue;
import java.util.StringTokenizer;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.logging.Level;
import java.util.logging.Logger;

//import org.csstudio.data.values.IMetaData;
//import org.csstudio.utility.pv.nds.Activator;
import org.csstudio.data.values.IDoubleValue;
import org.csstudio.data.values.INumericMetaData;
import org.csstudio.data.values.ISeverity;
import org.csstudio.data.values.ITimestamp;
import org.csstudio.data.values.IValue;
import org.csstudio.data.values.TimestampFactory;
import org.csstudio.data.values.ValueFactory;
import org.csstudio.data.values.ValueUtil;
import org.csstudio.data.values.IValue.Quality;
import org.csstudio.utility.pv.PV;
import org.csstudio.utility.pv.PVListener;
import org.csstudio.utility.pv.nds.NDS_PV.ThreadRef;
//import org.eclipse.core.runtime.PlatformObject;
import org.eclipse.core.runtime.Platform;
import org.eclipse.core.runtime.preferences.IPreferencesService;

/**
 * LIGO NDS (V1) implementation of the PV interface.
 * 
 * <p>
 * When started, it connects to the configured NDS server, gets the meta data for
 * the channel and then starts new network writer to receive the data online at
 * the native update rate in 16Hz chunks (start fast-writer NDS command).
 * 
 * 
 * @see PV
 * @author Alex Ivanov
 * 
 */
public class NDS_PV  implements PV, Runnable, Debug, Defaults, DataTypeConstants {

	private String name;
	
	/** Current PV value */
    private IValue value;

    /** PVListeners of this PV */
    private final CopyOnWriteArrayList<PVListener> listeners =
        new CopyOnWriteArrayList<PVListener>();

    private boolean running = false;

    public static BlockingQueue<IValue> qe;
    static {
    	qe = new ArrayBlockingQueue<IValue>(32);
    }
    
    /**
     * Thread and reference count private class.
     *
     */
	class ThreadRef {
		public ThreadRef(int references, Thread update_thread, NDS_PV nds_pv, int cIdx) {
			super();
			this.references = references;
			this.update_thread = update_thread;
			this.nds_pv = nds_pv;
			this.time = TimestampFactory.createTimestamp(0,0);
			this.connectIndex = cIdx;
		}
		int references;
		
	    /** Thread that updates the named PV.
	     *  <p>
	     */
		volatile Thread update_thread;
		
		/** Timestamp of the last data block that has arrived 
		 * 
		 */
		ITimestamp time;
		
		int connectIndex;
		
		/**
		 * The NDS_PV object the thread is running
		 */
		NDS_PV nds_pv;
		
		public void up() {
			references++;
		}
		public void down() {
			references--;
		}
		public int count() {
			return references;
		}
		public Thread thread() {
			return	update_thread;
		}
		public void reset() {
			references = 0;
		}
	};
	
	/**
	 * Class field map that maps NDS PV name into theThread and reference count.
	 * Setting the value object to null will stop the running DAQ thread.
	 */
	
	private static Map<String,ThreadRef> m;
	static {
		m = new HashMap<String, ThreadRef>();
	}
	
	private static int connectIndexCounter;
	static {
		connectIndexCounter = 0;
	}

	private Preferences preferences;


	public Preferences getPreferences() {
		return preferences;
	}

	private Net net;

	public Net getNet() {
		return net;
	}

	static Logger LOG;

    static {
    	LOG = Logger.getAnonymousLogger();
    	LOG.setLevel(Level.INFO);
    }
    

	/**
	 * Generate an NDS PV.
	 * 
	 * @param name
	 *            The PV name.
	 */
	public NDS_PV(final String name) {
		this.setName(name);
		value = TextUtil.parseValueFromString("0.0", null);
	}

	@Override
	public String getName() {
		return NDSPVFactory.PREFIX + "://" + name;
	}

		@Override
	public IValue getValue(double timeout_seconds) throws Exception {
		return value;
	}

	@Override
	public void addListener(PVListener listener) {
        listeners.add(listener);
        // When running, send the one and only initial update
        if (running && isConnected()) {
        	listener.pvValueUpdate(this);

        }
	}
	
	@Override
	public void removeListener(PVListener listener) {
        listeners.remove(listener);
//        if (listeners.isEmpty()) {
//            synchronized (this)
//            {
//            	
//        		m.get(name).reset();
//                notifyAll();
//            }
//        }
	}

	@Override
	public synchronized void start() throws Exception {
        running = true;
        for (PVListener listener : listeners)
            listener.pvValueUpdate(this);
        LOG.entering(name, "start");
        try {
			//if (NDS_PV.references == 1) {
        	if (m.get(name) != null) {
        		m.get(name).up();
        	} else {
        		
        		if (name.equals("gps")) {
        			// GPS NDS PV is a special PV used to receive the GPS
        			// time stamp on the latest available data
        			// So no thread is started on that one
        			m.put(name, new ThreadRef(1, null, this, 0));

        		} else {
        			preferences = null;

        			connect();

        			// Load channel from the server
        			preferences.updateChannelSet(new ChannelSet(null, net));

        			if (!preferences.getChannelSet().addChannel(name, 16))
        				throw new Exception();

        			//if (!net.startNetWriter(preferences.getChannelSet(), 0, 0))
        				//throw new Exception();

        			//final Thread update_thread = new Thread(this, getName());
        			//update_thread.start();
        			
        			// See how many elements there are in the map already, calculate the index
    				int ccount = 0;
    				
    				// Update all NDS PV objects.
    				Iterator<Entry<String, ThreadRef>> it1 = m.entrySet().iterator();
    				while (it1.hasNext()) {
    					Entry<String, ThreadRef> p = it1.next();
    					if (p.getKey().equals("gps")) continue;
    					ccount++;
    				}

        			//m.put(name, new ThreadRef(1, update_thread, this, ccount));
        		}
			}
			LOG.log(Level.FINE,"name=" + getName() + " this=" + this + " NDS PV references =" + m.get(name).count());
		} catch (Exception e) {
			if (net != null) {
				net.disconnect();
				net = null;
			}
			if (m.get("name") != null) {
				m.get(name).down();
				if (m.get(name).count() == 0) {
					m.put(name, null);
				}
			}
	        running = false;
		}
	}

	public void connect() throws Exception,
			ClassNotFoundException {
		String daqdNetVersionString = null;
		final IPreferencesService prefs = Platform.getPreferencesService();
		String hostName = "<empty>";
		if (prefs != null) {
			hostName = prefs.getString(org.csstudio.utility.pv.nds.Activator.ID, "host", null, null);
			final int port = prefs.getInt(org.csstudio.utility.pv.nds.Activator.ID, "port", 0, null);

			hostName += ":" + port;
			LOG.log(Level.FINEST, "NDS host from preferences=" + hostName);
			daqdNetVersionString = getVersionString(hostName);
		} else {
			// Connect to the NDS server
			final String ndsServerStr = System.getenv("NDSSERVER");
			if (ndsServerStr != null) {
				//verbose("NDSSERVER="+ndsServerStr);
				StringTokenizer st = new StringTokenizer(ndsServerStr, ",");
				while (st.hasMoreTokens()) {
					hostName = st.nextToken();
					daqdNetVersionString = getVersionString(hostName);
				}
			}
		}
		if (daqdNetVersionString == null) {
			LOG.log(Level.FINEST, "Unable to connect to " + hostName);
			throw new Exception();
		} else {
			net = null;

			LOG.log(Level.FINER, "DaqdNet protocol version " + daqdNetVersionString);

			try {
				net = (Net) Class.forName(
						"org.csstudio.utility.pv.nds.DaqdNet"
								+ daqdNetVersionString).newInstance();
			} catch (ClassNotFoundException e) {
				LOG.log(Level.SEVERE, "Communication protocol version or revision is not supported by this program");
				throw e;
			} catch (Exception e) {
				LOG.log(Level.SEVERE, "Unable to load DaqdNet" + daqdNetVersionString + e);
				throw e;
			}
		}

		net.setPreferences(preferences);
		preferences.setNet(net);
	}

	private String getVersionString(final String host) {
		String daqdNetVersionString;
		preferences = new Preferences();	
		preferences.setConnectionParams(new ConnectionParams(host), true);

		DaqdNetVersion daqdNetVersion = new DaqdNetVersion(preferences);
		daqdNetVersionString = daqdNetVersion.getVersionString();
		if (daqdNetVersionString != null) {
			LOG.log(Level.FINER, "Connected to the NDS host=" + host);
		}
		return daqdNetVersionString;
	}

	@Override
	public boolean isRunning() {
		return running;
	}

	@Override
	public boolean isConnected() {
		return running;
	}

	@Override
	public boolean isWriteAllowed() {
		return false;
	}

	@Override
	public String getStateInfo() {
		return "nds";
	}

	protected void finalize() throws Throwable {
		LOG.info("Finalize method called");
		this.stop();
	}

	@Override
	public void stop() {
        running = false;
        final Thread running_thread;
        synchronized (this)
        {
        	LOG.log(Level.FINER, "stop() called on " + name);
        	if (m.get(name) == null) return;
    		LOG.log(Level.FINER,"stop() called; references=" + m.get(name).count());
    		if (m.get(name).count() == 0) return;
    		else {
    			m.get(name).down();
    			if (m.get(name).count() > 0) return;
    			running_thread = m.get(name).thread();
    			synchronized (m) {
    				m.remove(name);    		
    			}
    		}
    		LOG.log(Level.FINER,"stop() is requesting DAQ to stop");

            notifyAll();
        }
        // Wait for update_thread to finish.
        // Without waiting, a quick follow-up call to start() could
        // set update_thread != null and then the 'old' thread
        // would continue to run.
        try
        {	if(running_thread != null)
            	running_thread.join();
        }
        catch (InterruptedException ex)
        {
            // Ignore
        }
		LOG.log(Level.FINEST,"stop() has joined the running thread");
	}

	@Override
	public IValue getValue() {
		LOG.log(Level.FINER,"getValue(" + name + ")");

		return value;
	}

	@Override
	public void setValue(Object new_value) throws Exception {
        throw new Exception(getName() + " is read-only");
	}

	public void setName(String name) {
		this.name = name;
	}

	@Override
	public void run() {
        final ISeverity OK = ValueFactory.createOKSeverity();
        final INumericMetaData meta = ValueFactory.createNumericMetaData(-5000, +5000, 0, 0, 0, 0, 1, "counts");

	    Channel ch = (Channel)preferences.getChannelSet ().firstElement();
	    final int chrate = ch.getRate()/16;
	    double a[] = new double [chrate];
	    
        while (true)
        {
        	DataBlock block = net.getDataBlock ();		
        	byte [] wave = block.getWave();

        	LOG.finest("On " + name + " Received DAQ block time=" + block.getTimestamp () + " dataLen=" + wave.length);
        	final ITimestamp now = TimestampFactory.createTimestamp(block.getTimestamp (), block.getNano());

        	DataInputStream in = new DataInputStream (new ByteArrayInputStream (wave));
        	for (int i = 0; i < chrate; i++){
        		try {
        			double f=in.readFloat ();
        			//System.out.println (f);
        			a[i]=f;
        		} catch (IOException e) {
        			e.printStackTrace();
        		}
        	}
        	value = ValueFactory.createDoubleValue(now,
        			OK, OK.toString(), meta, Quality.Original,
        			a);

        	// Synchronized over multiple threads on the class object "m"
        	// Still keep multiple connections, this needs to be changed into a single one.
        	if (m.get(name) != null) {
        		synchronized (m) {
        			m.get(name).time = now;

        			Iterator<Entry<String, ThreadRef>> it = m.entrySet().iterator();
        			boolean doit = true;
        			while (it.hasNext()) {
        				Entry<String, ThreadRef> p = it.next();
        				if (p.getKey().equals("gps")) continue;
        				if (p.getValue() == null) {
        					doit = false;
        					continue;
        				}
        				LOG.finer("Thread " + name + " key = " + p.getKey()
        						+ " = " + p.getValue().time.seconds() + "." + p.getValue().time.nanoseconds() 
        						+ " now = " + now.seconds() + "." + now.nanoseconds());
        				//it.remove(); // avoids a ConcurrentModificationException
        				// Check the time stamps on all receive threads, if one of them is different, do not update
        				if (p.getValue().time.toDouble() != now.toDouble()) {
        					doit = false;
        					break;
        				}
        			}
        			if (doit) {
        				int ccount = 0;
        				
        				// Update all NDS PV objects.
        				Iterator<Entry<String, ThreadRef>> it1 = m.entrySet().iterator();
        				while (it1.hasNext()) {
        					Entry<String, ThreadRef> p = it1.next();
        					p.getValue().nds_pv.update();
        					LOG.finer("update called by " + name + " on " + p.getKey() + " index=" + p.getValue().connectIndex);
        					if (p.getValue().connectIndex > ccount) ccount = p.getValue().connectIndex;
        				}
        				ccount++;
        				
        				final int r = 1;
        				// Assign data into the queue
        				double cval[] = new double[1 + r * ccount]; // TODO remove hard coded rate
        				cval[0] = now.toDouble(); // First element will contain the time stamp
        				it1 = m.entrySet().iterator();
        				while (it1.hasNext()) {
        					Entry<String, ThreadRef> p = it1.next();
        					if (p.getKey().equals("gps")) continue;
        					double [] val = ValueUtil.getDoubleArray(p.getValue().nds_pv.value);
        					int idx = p.getValue().connectIndex;
        					System.arraycopy(val, 0, cval, 1 + idx * r, r);
        				}
        				IValue ival = ValueFactory.createDoubleValue(now,
    							OK, OK.toString(), meta, Quality.Original,
    							cval);
        				qe.offer(ival);
        				
        				// Update gps trigger PV
        				if (m.get("gps") != null) {
        					double b[] = new double[1];
        					b[0] = now.toDouble();
        					m.get("gps").nds_pv.value = ValueFactory.createDoubleValue(now,
        							OK, OK.toString(), meta, Quality.Original,
        							b);

        					//m.get("gps").nds_pv.update();
        				}

        			}
        		}
    		}
      //		Thread.yield();

        	// stop() requested?
            synchronized (this)
            {
                if (m.get(name) == null) {
    			    LOG.finer("run() is returning");
    				net.disconnect();

                    return;
                }
            }
        }

	}

	private void update() {
		//value = TextUtil.parseValueFromString("1.234", null);

		// Commented on Aug 28, 2014
       // for (PVListener listener : listeners)
            //listener.pvValueUpdate(this);
	}

}
