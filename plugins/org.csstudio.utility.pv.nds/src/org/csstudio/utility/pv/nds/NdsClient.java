package org.csstudio.utility.pv.nds;

//import org.csstudio.opibuilder.scriptUtil.ConsoleUtil;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.widgets.Display;

public class NdsClient {
	public static void test() {
		//ConsoleUtil.writeInfo("Test");
		MessageDialog.openInformation(Display.getCurrent().getActiveShell(), "Test", "Ligo NDS Client Fragment called from a JavaScript script.");
	}
}
