package org.csstudio.utility.pv.nds;

import java.awt.*;
import java.awt.event.*;

class ClosableFrame extends Frame implements WindowListener {

  public ClosableFrame (String title) {
    super (title);
    this.addWindowListener (this);
  }

  public void windowClosing (WindowEvent e) {
    dispose ();
  };

  public void windowOpened  (WindowEvent e) {}
  public void windowClosed  (WindowEvent e) {}
  public void windowIconified  (WindowEvent e) {}
  public void windowDeiconified  (WindowEvent e) {}
  public void windowActivated  (WindowEvent e) {}
  public void windowDeactivated  (WindowEvent e) {}
}
