package org.csstudio.utility.pv.nds;

import java.io.*;
import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.net.*;

import org.csstudio.utility.pv.nds.*;

/**
 * Set network connection params
 */
class NetPreferencesDialog
  extends JdClosableDialog
  implements  ActionListener, TextListener, Debug, DataTypeConstants {

  class UnableToConnect extends Exception {
    UnableToConnect () {}
    UnableToConnect (String s) {super (s);}
  }

  Button okButton;
  Button cancelButton;
  Button clearButton;
  Preferences preferences;
  Preferences newPreferences;
  TextField hostTextField;
  TextField portTextField;
  TextField urlTextField;
  TextField framerHostTextField;
  TextField framerPortTextField;
  Frame frame;

  public NetPreferencesDialog (Frame frame, String title, boolean modal, Preferences preferences, CommonPanel commonPanel) {
    super (frame, title, commonPanel);
    this.frame = frame;
    this.preferences = preferences;
    newPreferences = new Preferences (preferences);
    newPreferences.setConnectionParams (new ConnectionParams (preferences.getPortString ()), false); // copy the connection params too

    setLayout (new GridBagLayout ());
    GridBagConstraints c = new GridBagConstraints ();
    c.fill = GridBagConstraints.BOTH;
    c.insets = new Insets (5,5,5,5);
    c.gridx = c.gridy = 0;
    c.gridwidth = 10;
    c.gridheight = 1;
    c.weightx = 1.0; c.weighty = .0;

    Panel p = new Panel ();
    p.setLayout (new BorderLayout ());
    p.add (new Label ("Image Map URL"), "West");
    String urlString = preferences.getImageMapURL () == null ? "": preferences.getImageMapURL ().toString ();
    p.add (urlTextField = new TextField (urlString.length ()));
    urlTextField.addTextListener (this);
    urlTextField.setText (urlString);
    add (p, c);

    p = new Panel ();
    p.setLayout (new BorderLayout ());
    p.add (new Label ("DAQD host and port"), "West");
    p.add (hostTextField = new TextField (12));
    hostTextField.addTextListener (this);
    hostTextField.setText (preferences.getConnectionParams ().getHost ());
    p.add (portTextField = new TextField (6), "East");
    portTextField.addTextListener (this);
    portTextField.setText (Integer.toString (preferences.getConnectionParams ().getPort ()));
    //    c.fill = GridBagConstraints.NONE;
    c.gridy = 3;
    c.gridx = GridBagConstraints.REMAINDER;
    c.anchor = GridBagConstraints.WEST;
    add (p, c);

    p = new Panel ();
    p.setLayout (new BorderLayout ());
    p.add (new Label ("NDS Framer host and port"), "West");
    p.add (framerHostTextField = new TextField (12));
    framerHostTextField.addTextListener (this);
    framerHostTextField.setText (preferences.getFramerHost ());
    p.add (framerPortTextField = new TextField (6), "East");
    framerPortTextField.addTextListener (this);
    framerPortTextField.setText (Integer.toString (preferences.getFramerPort ()));
    c.gridy = 4;
    c.gridx = GridBagConstraints.REMAINDER;
    c.anchor = GridBagConstraints.WEST;
    add (p, c);

    p = new Panel ();
    p.setLayout (new FlowLayout (FlowLayout.CENTER, 15, 15));
    okButton = new Button ("OK");
    okButton.addActionListener (this);
    p.add (okButton);
    clearButton = new Button ("Clear");
    clearButton.addActionListener (this);
    p.add (clearButton);
    cancelButton = new Button ("Cancel");
    cancelButton.addActionListener (this);
    p.add (cancelButton);

    c.anchor = GridBagConstraints.CENTER;
    c.gridy = 16;
    c.gridx = 0;
    add (p, c);
    pack ();
    Display.centralise (this);
    //    setResizable (false);
    show ();
    commonPanel.setCursor (new Cursor (Cursor.DEFAULT_CURSOR));
  }

  public void performEnterAction() {
    setCursor (new Cursor (Cursor.WAIT_CURSOR));
    try {
      newPreferences.setImageMapURL (new URL (urlTextField.getText ()));

      // Reconfigure network connection and channels
      // recreate Net class; this may involve the loading of different Net subclass

      DaqdNetVersion daqdNetVersion = new DaqdNetVersion (newPreferences);

      String daqdNetVersionString = daqdNetVersion.getVersionString ();

      if (daqdNetVersionString == null)
	throw new UnableToConnect ("Unable to connect to " + newPreferences.getPortString ());

      System.err.println ("DaqdNet protocol version " + daqdNetVersionString);
      Net net = null;
      try {
	net = (Net) Class.forName ("jdclient.DaqdNet" + daqdNetVersionString).newInstance ();
      } catch (ClassNotFoundException  ee) {
	throw new UnableToConnect ("Communication protocol version or revision is not supported by this program");
      } catch (Exception ee) {
	throw new UnableToConnect ("Unable to load DaqdNet" + daqdNetVersionString);
      }

      net.setPreferences (newPreferences); // use new preferences so that it connects to the right place
      net.setLabel (preferences.getNet ().getLabel ());
      preferences.setNet (net);
      newPreferences.setNet (net);

      // Reload server channel set and check that all selected channels are on the server

      // clear display list
      ChannelList list = preferences.getChannelSet ().getChannelList ();
      list.removeAll ();
      ChannelSet newSet = new ChannelSet (list, net); // `net' is used to get the list of channels
      // Try to add all selected channels to the new set and to the display list
      for (Enumeration em = preferences.getChannelSet ().elements (); em.hasMoreElements ();) {
	Channel ch = (Channel) em.nextElement ();
	newSet.addChannel (ch.getName (), ch.getRate ());
      }
      preferences.setChannelSet (newSet);
      net.setPreferences (preferences);
      preferences.setValues (newPreferences);
      close ();
    } catch (UnableToConnect ee) {
      new Alert (frame, ee.getMessage());
    } catch (MalformedURLException ee) {
      new Alert (frame, "Invalid URL Entered");
    }
    setCursor (new Cursor (Cursor.DEFAULT_CURSOR));
  }
  
  public void actionPerformed (ActionEvent e) {
    Object src = e.getSource ();
    if (src == (Object) okButton) {
      performEnterAction ();
    } else if (src == (Object) clearButton) {
      CommonPanel.clearFields (this);
      clearButton.requestFocus ();
    } else if (src == (Object) cancelButton)
      close ();
  }


  public void textValueChanged (TextEvent e) {
    Object src = e.getSource ();
    if (src == (Object) hostTextField) {
      newPreferences.getConnectionParams ().setHost (hostTextField.getText ());
    } else if (src == (Object) portTextField) {
      if (portTextField.getText ().length () == 0)
	newPreferences.getConnectionParams ().setPort (0);
      else {
	try {
	  newPreferences.getConnectionParams ().setPort (Integer.parseInt (portTextField.getText ()));
	} catch (NumberFormatException ee) {
	  portTextField.setText (Integer.toString (newPreferences.getConnectionParams ().getPort ()));
	}
      }
    } else if (src == (Object) framerHostTextField) {
      newPreferences.setFramerHost (framerHostTextField.getText ());
    } else if (src == (Object) framerPortTextField) {
      if (framerPortTextField.getText ().length () == 0)
	newPreferences.setFramerPort (0);
      else {
	try {
	  newPreferences.setFramerPort (Integer.parseInt (framerPortTextField.getText ()));
	  } catch (NumberFormatException ee) {
	    framerPortTextField.setText (Integer.toString (newPreferences.getFramerPort ()));
	  }
      }
    }

 /*
    } else if (src == (Object) urlTextField) {
      if (urlTextField.getText ().equals (""))
	newPreferences.setImageMapURL (null);
      else {
	try {
	  newPreferences.setImageMapURL (new URL (urlTextField.getText ()));
	} catch (MalformedURLException ee) {
	  urlTextField.setText (newPreferences.getImageMapURL ().toString ());
	}
      }
 */
  }
}


