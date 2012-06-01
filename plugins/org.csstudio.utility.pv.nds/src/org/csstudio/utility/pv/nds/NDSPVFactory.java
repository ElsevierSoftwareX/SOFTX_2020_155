package org.csstudio.utility.pv.nds;

import org.csstudio.utility.pv.IPVFactory;
import org.csstudio.utility.pv.PV;
import org.csstudio.utility.pv.nds.NDS_PV;

public class NDSPVFactory implements IPVFactory {
	/** PV type prefix */
    public static final String PREFIX = "nds"; //$NON-NLS-1$
    
	@Override
	public PV createPV(String name) throws Exception {
		return new NDS_PV(name);
	}

}
