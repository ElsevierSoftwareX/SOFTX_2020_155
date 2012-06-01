package org.csstudio.utility.pv.nds;

import java.io.*;
import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.net.*;

import org.csstudio.utility.pv.nds.*;


/**
 * Disable and enable visual components -- make the dialog modal
 */
public class JdClosableDialog extends ClosableDialog {
  protected CommonPanel commonPanel;
  public JdClosableDialog (Frame frame, String title, CommonPanel commonPanel) {
    super (frame, title, false);
    this.commonPanel = commonPanel;
    commonPanel.disableAllComponents ();
  }

  public void close (){
    super.close ();
    commonPanel.enableAllComponents ();
  }
}
