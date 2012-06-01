package org.csstudio.opibuilder.customScriptUtil;

import org.csstudio.data.values.ValueUtil;
import org.csstudio.opibuilder.scriptUtil.ConsoleUtil;
import org.csstudio.utility.pv.PV;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.widgets.Display;
import org.csstudio.utility.pv.nds.NDS_PV;

public class LigoScriptUtil {
	
//	public static void helloWorld(){
//		ConsoleUtil.writeInfo("Hello World!");
//		MessageDialog.openInformation(Display.getCurrent().getActiveShell(), "Hello", 
//				"LigoScriptUtil says: Hello World!");
//	}

	public final static double[] getDoubleArray(){
		try {
			return ValueUtil.getDoubleArray(NDS_PV.qe.take());
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
			return null;
		}
	}

}