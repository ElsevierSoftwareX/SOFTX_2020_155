package org.csstudio.utility.pv.nds;

/**
 * DAQD data acquisition client main function and GUI
 *
 * @author 	Alex Ivanov
 * @version 	1.0, 12/22/98
 */

import java.io.*;
import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.net.*;

import org.csstudio.utility.pv.nds.*;


/**
 * Application's main class
 */
public class JdClient extends CommonPanel implements Debug, Defaults, ActionListener, ItemListener {
  MenuItem exitMenuItem;
  MenuItem filePreferencesMenuItem;
  MenuItem aboutMenuItem;

  public boolean init () {
    super.init ();

    daq = new DataAcquisition (this, preferences);

    setLayout (new GridBagLayout ());
    GridBagConstraints c = new GridBagConstraints ();
    c.fill = GridBagConstraints.BOTH;
    c.gridwidth = 10;
    c.gridheight = 3;
    c.insets = new Insets (2,2,0,2);
    c.gridx = c.gridy = 0;
    c.weightx = 1.0; c.weighty = 0.0;
    Controls controlsPanel = new Controls ();
    add (controlsPanel, c);

    c.gridy = 3;
    c.gridheight = 6;
    c.weightx = c.weighty = 1.0;
    add (cl, c);

    c.gridy = 9;
    c.gridheight = 1;
    c.weightx = 1.0; c.weighty = 0.0;
    c.insets = new Insets (0,2,0,2);
    add (netLabel, c);

    daq.setProgressLabel (controlsPanel.getProgressLabel ());
    daq.setFileLabel (controlsPanel.getFileLabel ());
    controlsPanel.setDaq (daq);
    controlsPanel.setPreferences (preferences);
    return true;
  }

  /**
   * Create read config class and actually read channel configuration from a file
   */
  void readConfig () {
    String fileName;

    if ((fileName = System.getProperty ("JDCONFIG")) != null) {
      try {
	ReadConfig rc =
	  new ReadConfig (new BufferedReader (new FileReader (fileName)), preferences);
	rc.readFile();
      } catch (FileNotFoundException ee) {
	System.err.println ("While trying to open config file:");
	System.err.println (ee);	
	System.exit (1);
      } catch (IOException e) {
	System.err.println ("While reading config file:");
	System.err.println (e);
	System.exit (1);
      }
    }
  }

  /**
   * Create top-level component frame and show it
   */
  public void showFrame (Frame frame, MenuBar menuBar) {
    super.showFrame (frame, menuBar);

    exitMenuItem = new MenuItem ("Exit", new MenuShortcut (KeyEvent.VK_Q));
    exitMenuItem.addActionListener (this);
    fileMenu.add (exitMenuItem);

    filePreferencesMenuItem = new MenuItem ("File...");
    filePreferencesMenuItem.addActionListener (this);
    preferencesMenu.add (filePreferencesMenuItem);

    Menu helpMenu = new Menu ("Help");
    menuBar.add (helpMenu);
    menuBar.setHelpMenu (helpMenu);

    aboutMenuItem = new MenuItem ("About");
    aboutMenuItem.addActionListener (this);
    helpMenu.add (aboutMenuItem);
    frame.pack ();
    Display.centralise (frame);
    //frame.show ();
    frame.setVisible(true);
  }

  public void actionPerformed (ActionEvent e) {
    Object src = e.getSource ();
    if (src == (Object) exitMenuItem) {
      preferences.savePreferences ();
      System.exit (0);
    } else if (src == (Object) filePreferencesMenuItem) {
      setCursor (new Cursor (Cursor.WAIT_CURSOR));
      new FilePreferencesDialog (frame, "File Preferences", DIALOGS_ARE_MODAL, preferences, this);
    } else if (src == (Object) aboutMenuItem) {
      new ErrorDialog (frame, "About", 
		       this.getClass ().getName () + " "
		       + Integer.toString (VERSION) + "." + Integer.toString (REVISION) + "\n"
		       + "http://www.ligo.caltech.edu/~aivanov/jdclient/index.html" + "\n"
		       + "---\n"
		       + "JVM: " + System.getProperty ("java.version")
		       + " by " + System.getProperty ("java.vendor") + "\n"
		       + "Operating System: " + System.getProperty ("os.name")
		       + " version " + System.getProperty ("os.version") + "\n"
		       + "Architecture: " + System.getProperty ("os.arch")
		       );
    } else 
      super.actionPerformed (e);
  }


  /**
   * Create the JdClient and show it in a frame
   */
  public static void main (String args []) {
    JdClient jdClient = new JdClient ();

    if (! jdClient.init ())
      System.exit (1);

    jdClient.readConfig ();
    Frame frame = new ApplicationClosableFrame ("JdClient", jdClient.getPreferences ());
    MenuBar menuBar = new MenuBar ();
    jdClient.showFrame (frame, menuBar);
    frame.setMenuBar (menuBar);
    frame.setVisible(true);
  }
}

/**
 * Packed in are some field assignments
 */
class GridBagConstraints1 extends GridBagConstraints {
  public GridBagConstraints1 () {
  }
}

/**
 * GUI control and display area 
 */
class Controls extends Panel implements /* ItemListener, */ ActionListener {
  Preferences preferences;
  DataAcquisition daq;
  //  Choice formatChoice;
  Label fileLabel;
  Label progressLabel;
  //  Label netLabel;
  Button startButton;
  Button stopButton;
  /*
  Button suspendButton;
  Button exitButton;
  Button decimateButton;
  Button channelsButton;
  */

  public Controls () {
    setLayout (new GridBagLayout ());
    GridBagConstraints c = new GridBagConstraints ();      
    c.insets = new Insets (0,2,0,2);
    c.gridx = c.gridy = 0;
    c.gridwidth = GridBagConstraints.REMAINDER;
    c.gridheight = 1;
    c.weightx = c.weighty = .0;
    c.fill = GridBagConstraints.HORIZONTAL;

    Panel p = new Panel ();
    p.setLayout (new FlowLayout (FlowLayout.LEFT, 0, 0));
    startButton = new Button ("Start");
    add (startButton, c);
    startButton.addActionListener (this);
    p.add (startButton);
    stopButton = new Button ("Stop");
    add (stopButton, c);
    stopButton.addActionListener (this);
    p.add (stopButton);
    add (p, c);


    /*
    c.gridx = 2;
    suspendButton = new Button ("Pause");
    add (suspendButton, c);
    suspendButton.addActionListener (this);

    c.gridx = 3;
    exitButton = new Button ("Exit");
    add (exitButton, c);
    exitButton.addActionListener (this);
    */

    /*
    c.gridy = 1;
    c.gridx = 0;
    decimateButton = new Button ("Decimate");
    add (decimateButton, c);
    decimateButton.addActionListener (this);

    c.gridx = 1;
    channelsButton = new Button ("Channels");
    add (channelsButton, c);
    channelsButton.addActionListener (this);

    add (formatChoice = new Choice (), c);
    formatChoice.addItemListener (this);
    c.gridy = 3;
    add (progressLabel = new Label (), c);
    c.gridy = 4;
T    add (fileLabel = new Label (), c);
    */
    c.gridx = 0;
    c.gridwidth = GridBagConstraints.REMAINDER;
    c.weightx = 1.0;
    c.fill = GridBagConstraints.HORIZONTAL;
    c.gridy = 1;
    add (progressLabel = new Label (), c);
    c.gridy = 2;
    add (fileLabel = new Label (), c);

    //    c.gridy = 5;
    //    add (netLabel = new Label (), c);
  }

  public void setDaq (DataAcquisition daq) {
    this.daq = daq;
  }

  public void setPreferences (Preferences preferences) {
    this.preferences = preferences;
  }

  /*  
  public void itemStateChanged (ItemEvent e) {
    preferences.setExportFormatIndex (formatChoice.getSelectedIndex ());
  }
  */

  public void actionPerformed (ActionEvent e) {
    Object src = e.getSource ();
    if (src == (Object) startButton)
      daq.start ();
    else if (src == (Object) stopButton)
      daq.stop ();
  }

  /*
  Frame getParentFrame (Component parent) {
    for (;;parent = parent.getParent ())
      if (parent instanceof Frame)
	return (Frame) parent;
  }
  */

  //  public Choice getFormatChoice () { return formatChoice; }
  public Label getFileLabel () { return fileLabel; }
  public Label getProgressLabel () { return progressLabel; }
  //  public Label getNetLabel () { return netLabel; }
}

/**
 * Top-level frame component that can be closed with the window manager; application exits
 */
class ApplicationClosableFrame extends Frame implements WindowListener {
  Preferences preferences;

  public ApplicationClosableFrame (String title, Preferences preferences) {
    super (title);
    this.addWindowListener (this);
    this.preferences = preferences;
  }

  public void windowClosing (WindowEvent e) {
    preferences.savePreferences ();
    System.exit (0);
  };

  public void windowOpened  (WindowEvent e) {}
  public void windowClosed  (WindowEvent e) {}
  public void windowIconified  (WindowEvent e) {}
  public void windowDeiconified  (WindowEvent e) {}
  public void windowActivated  (WindowEvent e) {}
  public void windowDeactivated  (WindowEvent e) {}
}
