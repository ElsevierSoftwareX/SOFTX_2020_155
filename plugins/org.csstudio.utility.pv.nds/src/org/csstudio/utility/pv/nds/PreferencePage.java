package org.csstudio.utility.pv.nds;

import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IWorkbench;
import org.eclipse.ui.IWorkbenchPreferencePage;
import org.eclipse.ui.preferences.ScopedPreferenceStore;
import org.eclipse.jface.preference.FieldEditorPreferencePage;
import org.eclipse.jface.preference.IntegerFieldEditor;
import org.eclipse.jface.preference.StringFieldEditor;
import org.eclipse.core.runtime.preferences.IScopeContext;
import org.eclipse.core.runtime.preferences.InstanceScope;


public class PreferencePage extends FieldEditorPreferencePage
		implements IWorkbenchPreferencePage {
	
    public PreferencePage() {
    	super(GRID);
        final IScopeContext scope = InstanceScope.INSTANCE;
		setPreferenceStore(new ScopedPreferenceStore(scope,
                org.csstudio.utility.pv.nds.Activator.ID));

    }
    
	@Override
	public void init(IWorkbench workbench) {
		
	}

	@Override
	protected void createFieldEditors() {
		setMessage("NDS Server Settings");
        final Composite parent = getFieldEditorParent();

        addField(new StringFieldEditor("host", "Host", parent));
        addField(new IntegerFieldEditor("port", "Port", parent));
	}


}
