package org.csstudio.utility.pv.nds;

import java.io.*;
import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.net.*;

import org.csstudio.utility.pv.nds.*;

class EmptyChannelSet extends ChannelSet { 
  EmptyChannelSet (ChannelList list, Net net) {
    super (list, net);
  }

  /*
   * Accepts all channels
   */
  public boolean addChannel (String name, int rate) {
    Channel ch = new Channel (name, rate, 0, 0, 0, false, 0);
    addElement (ch);
    if (list != null) {
      list.addChannel (ch);
    }
    return true;
  }
}


public class CommonPanel extends Panel implements Debug, Defaults, ActionListener, ItemListener {
  protected Preferences preferences;
  protected DataAcquisition daq;
  Frame frame;
  MenuBar menuBar;
  protected Menu fileMenu;
  protected Menu editMenu;
  Menu preferencesMenu;
  MenuItem openMenuItem;
  MenuItem saveMenuItem;
  MenuItem decimateMenuItem;
  MenuItem deleteMenuItem;
  MenuItem channelsMenuItem;
  MenuItem graphicsMenuItem;
  MenuItem daqPreferencesMenuItem;
  MenuItem networkPreferencesMenuItem;
  CheckboxMenuItem fullCheckboxMenuItem;
  CheckboxMenuItem trendCheckboxMenuItem;
  CheckboxMenuItem minuteTrendCheckboxMenuItem;
  String openDialogFileName = null;
  String openDialogDirectory = null;
  FilenameFilter openDialogFilter = null;
  protected ChannelList cl;
  protected Label netLabel;

  /**
   * Create GUI elements.
   * Create network, file save, data acquisition objects.
   * First, it connects to the server and gets communication
   * protocol version and revision, say 8 and 1.
   * Then it tries to load netowork class DaqdNet from directory v8/1.
   * Create preferences object.
   */
  public boolean init () {

    // Try to get preferences from the resource file
    try {
      FileInputStream istream = new FileInputStream (Preferences.fileName ());
      ObjectInputStream p = new ObjectInputStream (istream);
      preferences = (Preferences) p.readObject ();
      istream.close ();
    } catch (Exception e) {
      if (_debug > 5)
	e.printStackTrace ();
      System.err.println ("-------------------------------------------------------------");
      System.err.println ("Couldn't load preferences from " + Preferences.fileName ());
      System.err.println ("Saved configuration lost. You'll need to reconfigure JdClient");
      System.err.println ("Configuration loss normally happens after JdClient upgrade");
      System.err.println ("This is version "
			  + Integer.toString (VERSION) + "." + Integer.toString (REVISION)
			  + " of JdClient");
      System.err.println ("-------------------------------------------------------------");

      preferences = new Preferences ();
    }

    // Create common widgets
    cl = new ChannelList (7);
    cl.addItemListener (this);
    netLabel = new Label ();

    ConnectionParams connectionParams = new ConnectionParams (defaultPort);
    // If connection params are default after the init, set params from preferences
    preferences.setConnectionParams (connectionParams, connectionParams.defaultConfig ());

    DaqdNetVersion daqdNetVersion = new DaqdNetVersion (preferences);

    String daqdNetVersionString = daqdNetVersion.getVersionString ();
    Net net = null;

    if (daqdNetVersionString == null) {
      System.err.println ("Unable to determine DaqdNet protocol version");
      System.err.println ("To correct: enter a valid DAQD address in the Network Parameters dialog");
      System.err.println ("Alternative: start with `-DJDPORT=target_daqd_host_name:8088' parameter");
      daqdNetVersionString = "";
      //      return false;

      net = new DaqdNetUndefined (); // use the `version' class as an undefined `Net' class
      netLabel.setText ("can't connect to " + preferences.getPortString ());
    } else {
      System.err.println ("DaqdNet protocol version " + daqdNetVersionString);

      try {
	net = (Net) Class.forName ("jdclient.DaqdNet" + daqdNetVersionString).newInstance ();
      } catch (ClassNotFoundException  e) {
	System.err.println ("Communication protocol version or revision is not supported by this program");
	return false;
      } catch (Exception e) {
	System.err.println ("Unable to load DaqdNet" + daqdNetVersionString);
	System.err.println (e);
	return false;
      }
    }

    net.setPreferences (preferences);
    preferences.setNet (net);
    net.setLabel (netLabel);

    ChannelSet cs;
    if (net instanceof DaqdNetUndefined) 
      cs = new EmptyChannelSet (cl, net);
    else
      cs = new ChannelSet (cl, net);

    //cs.addChannel ("test channel1");
    //cs.addChannel ("test fast channel2", 16*1024);
    preferences.updateChannelSet (cs); // Put restored preferences into the channel list
    //    daq = new DataAcquisition (this, preferences);
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

  public void showFrame (Frame frame, MenuBar menuBar) {
    this.frame = frame;
    this.menuBar = menuBar;

    frame.add ("Center", this);

    fileMenu = new Menu ("File");
    menuBar.add (fileMenu);

    openMenuItem = new MenuItem ("Open...", new MenuShortcut (KeyEvent.VK_O));
    openMenuItem.addActionListener (this);
    fileMenu.add (openMenuItem);

    saveMenuItem = new MenuItem ("Save...", new MenuShortcut (KeyEvent.VK_S));
    saveMenuItem.addActionListener (this);
    fileMenu.add (saveMenuItem);


    editMenu = new Menu ("Edit");
    menuBar.add (editMenu);

    decimateMenuItem = new MenuItem ("Decimate...", new MenuShortcut (KeyEvent.VK_E));
    decimateMenuItem.addActionListener (this);
    editMenu.add (decimateMenuItem);

    deleteMenuItem = new MenuItem ("Delete", new MenuShortcut (KeyEvent.VK_D));
    deleteMenuItem.addActionListener (this);
    deleteMenuItem.setEnabled (false);
    editMenu.add (deleteMenuItem);

    Menu channelMenu = new Menu ("Channels");
    editMenu.add (channelMenu);

    channelsMenuItem = new MenuItem ("List...", new MenuShortcut (KeyEvent.VK_L));
    channelsMenuItem.addActionListener (this);
    channelMenu.add (channelsMenuItem);

    graphicsMenuItem = new MenuItem ("Image Map...", new MenuShortcut (KeyEvent.VK_M));
    graphicsMenuItem.addActionListener (this);
    channelMenu.add (graphicsMenuItem);

    preferencesMenu = new Menu ("Preferences");
    editMenu.add (preferencesMenu);

    Menu modeMenu = new Menu ("Mode");
    preferencesMenu.add (modeMenu);

    fullCheckboxMenuItem =  new CheckboxMenuItem ("Full");
    fullCheckboxMenuItem.addItemListener (this);
    modeMenu.add (fullCheckboxMenuItem);
    trendCheckboxMenuItem =  new CheckboxMenuItem ("Trend");
    trendCheckboxMenuItem.addItemListener (this);
    modeMenu.add (trendCheckboxMenuItem);
    minuteTrendCheckboxMenuItem =  new CheckboxMenuItem ("Minute Trend");
    minuteTrendCheckboxMenuItem.addItemListener (this);
    modeMenu.add (minuteTrendCheckboxMenuItem);

    if (preferences.fullAcquisitionModeSelected ()) {
      fullCheckboxMenuItem.setState (true);
      minuteTrendCheckboxMenuItem.setState (false);
      trendCheckboxMenuItem.setState (false);
    } else if (preferences.trendAcquisitionModeSelected ()) {
      fullCheckboxMenuItem.setState (false);
      minuteTrendCheckboxMenuItem.setState (false);
      trendCheckboxMenuItem.setState (true);
    } else if (preferences.minuteTrendAcquisitionModeSelected ()) {
      fullCheckboxMenuItem.setState (false);
      trendCheckboxMenuItem.setState (false);
      minuteTrendCheckboxMenuItem.setState (true);
    }


    daqPreferencesMenuItem = new MenuItem ("Acquisition...");
    daqPreferencesMenuItem.addActionListener (this);
    preferencesMenu.add (daqPreferencesMenuItem);

    networkPreferencesMenuItem = new MenuItem ("Network...");
    networkPreferencesMenuItem.addActionListener (this);
    preferencesMenu.add (networkPreferencesMenuItem);

    //    frame.pack ();
    //    Display.centralise (frame);
    //    frame.show ();

    /*
    if (preferences.getNet() instanceof DaqdNetUndefined) {
      setCursor (new Cursor (Cursor.WAIT_CURSOR));
      new NetPreferencesDialog (frame, "Network Parameters", DIALOGS_ARE_MODAL, preferences, this);
    }
    */
  }

  public Preferences getPreferences () { return preferences; }

  /**
   * Grey out all visible components in this window
   */
  public void disableAllComponents () {
    disableMenu ();
    frame.setEnabled (false);
  }

  /**
   * Enable all visible components
   */
  public void enableAllComponents () {
    frame.setEnabled (true);
    enableMenu ();
  }

  /**
   * Grey out menu items
   */
  public void disableMenu () {
    for (int i = 0; i < menuBar.getMenuCount (); i++)
      menuBar.getMenu (i).setEnabled (false);
    cl.setEnabled (false);
  }

  /**
   * Enable all menu items
   */
  public void enableMenu () {
    cl.setEnabled (true);;
    for (int i = 0; i < menuBar.getMenuCount (); i++)
      menuBar.getMenu (i).setEnabled (true);
  }

  void updateMenuItems () {
    // see if there are selections in the list, enable `Delete' menu item
    if (cl.getSelectedObjects ().length > 0)
      deleteMenuItem.setEnabled (true);
    else
      deleteMenuItem.setEnabled (false);
  }

  public void updateMenu () {
    updateMenuItems ();
  }

  public void itemStateChanged (ItemEvent e) {
    Object src = e.getSource ();

    if (src == (Object) fullCheckboxMenuItem) {
      fullCheckboxMenuItem.setState (true);
      trendCheckboxMenuItem.setState (false);
      minuteTrendCheckboxMenuItem.setState (false);
      preferences.setAcquisitionMode ("full");
      if (_debug > 5)
	System.err.println ("full mode selected");
    } else if (src == (Object) trendCheckboxMenuItem) {
      fullCheckboxMenuItem.setState (false);
      trendCheckboxMenuItem.setState (true);
      minuteTrendCheckboxMenuItem.setState (false);
      preferences.setAcquisitionMode ("trend");
      if (_debug > 5)
	System.err.println ("trend mode selected");
    } else if (src == (Object) minuteTrendCheckboxMenuItem) {
      fullCheckboxMenuItem.setState (false);
      trendCheckboxMenuItem.setState (false);
      minuteTrendCheckboxMenuItem.setState (true);
      preferences.setAcquisitionMode ("minute_trend");
      if (_debug > 5)
	System.err.println ("minute trend mode selected");
    } else 
      updateMenu ();
  }

  public void actionPerformed (ActionEvent e) {
    Object src = e.getSource ();
    if (src == (Object) openMenuItem) {
      FileDialog d = new FileDialog (frame, "Open Config File", FileDialog.LOAD);
      if (openDialogFileName != null)
	d.setFile (openDialogFileName);
      if (openDialogDirectory != null)
	d.setDirectory (openDialogDirectory);
      if (openDialogFilter != null) 
	d.setFilenameFilter (openDialogFilter);
      Display.centralise (d);
      d.show ();
      if (d.getFile () != null) {
	try {
	  ReadConfig rc =
	    new ReadConfig (new BufferedReader 
			    (new FileReader (d.getDirectory ()
					     + File.separator
					     + d.getFile ())), preferences);
	  preferences.getChannelSet ().removeAllChannels ();
	  rc.readFile();
	} catch (FileNotFoundException ee) {
	  System.err.println ("While trying to open config file:");
	  System.err.println (ee);	
	} catch (Exception ee) {
	  System.err.println ("While reading config file:");
	  System.err.println (ee);
	  //	System.exit (1);
	}
	openDialogFileName = d.getFile ();
	openDialogDirectory = d.getDirectory ();
	openDialogFilter = d.getFilenameFilter ();
      }
      d.dispose ();
    } else if (src == (Object) saveMenuItem) {
      FileDialog d = new FileDialog (frame, "Save Config File", FileDialog.SAVE);
      if (openDialogFileName != null)
	d.setFile (openDialogFileName);
      if (openDialogDirectory != null)
	d.setDirectory (openDialogDirectory);
      if (openDialogFilter != null) 
	d.setFilenameFilter (openDialogFilter);
      Display.centralise (d);
      d.show ();
      if (d.getFile () != null) {
	try {
	  FileWriter writer;
	  SaveConfig rc =
	    new SaveConfig (new PrintWriter
			    (writer = new FileWriter (d.getDirectory ()
						      + File.separator
						      + d.getFile ())), preferences);
	  rc.saveFile ();
	  writer.close ();
	} catch (FileNotFoundException ee) {
	  System.err.println ("While trying to open config file for writing:");
	  System.err.println (ee);	
	} catch (Exception ee) {
	  System.err.println ("While writing config file:");
	  System.err.println (ee);
	}
	openDialogFileName = d.getFile ();
	openDialogDirectory = d.getDirectory ();
	openDialogFilter = d.getFilenameFilter ();
      }
      d.dispose ();
    } else if (src == (Object) decimateMenuItem) {
      setCursor (new Cursor (Cursor.WAIT_CURSOR));
      new DecimateDialog (frame, "Decimate", true, preferences.getChannelSet (), this);
    } else if (src == (Object) deleteMenuItem) {
      preferences.getChannelSet ().removeSelectedChannels ();
      updateMenuItems ();
    } else if (src == (Object) channelsMenuItem) {
      setCursor (new Cursor (Cursor.WAIT_CURSOR));
      new ChannelsDialog (frame, "Channels", true, preferences.getChannelSet (), this);
    } else if (src == (Object) graphicsMenuItem) {
      setCursor (new Cursor (Cursor.WAIT_CURSOR));
      disableAllComponents ();
      try {
	new ImageMapDialog (frame, "Image Map", DIALOGS_ARE_MODAL, preferences, this);
      } catch (Exception ee) {
	enableAllComponents ();
	System.err.println ("Cannot create image map from " + preferences.getImageMapURL ().toString ().trim ());
	new Alert (frame, "Cannot create image map.\nCheck the URL: " + preferences.getImageMapURL ().toString ().trim ());
	setCursor (new Cursor (Cursor.DEFAULT_CURSOR));
      }
    } else if (src == (Object) daqPreferencesMenuItem) {
      setCursor (new Cursor (Cursor.WAIT_CURSOR));
      new DaqPreferencesDialog (frame, "Acquisition Control", DIALOGS_ARE_MODAL, preferences, this);
    } else if (src == (Object) networkPreferencesMenuItem) {
      setCursor (new Cursor (Cursor.WAIT_CURSOR));
      new NetPreferencesDialog (frame, "Network Parameters", DIALOGS_ARE_MODAL, preferences, this);
    }
  }

  /**
   * Clear all TextField objects in the container recursively
   */
  public final static void clearFields (Container container) {
    Component [] components = container.getComponents ();
    for (int i = 0; i < components.length; i++)
      if (components [i] instanceof Container)
	clearFields ((Container) components [i]);
      else if (components [i] instanceof TextComponent)
	((TextComponent) components [i]).setText ("");
      else if (components [i] instanceof Checkbox)
	((Checkbox) components [i]).setState (false);
  }
}
