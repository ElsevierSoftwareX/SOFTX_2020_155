package org.csstudio.utility.pv.nds;

import java.io.*;
import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.net.*;

import org.csstudio.utility.pv.nds.*;

class ImageMapDialog extends AppFrame {
  CommonPanel commonPanel;
  Preferences preferences;

  public ImageMapDialog (Frame frame, String title, boolean modal, Preferences preferences, CommonPanel commonPanel)
    throws Exception {
    super (title, frame, 1030, 710, preferences.getImageMapURL ()); //Display.screenX, Display.screenY
    this.commonPanel = commonPanel;
    this.preferences = preferences;
    for (Enumeration e = preferences.getChannelSet ().elements (); e.hasMoreElements ();)
      list_selected.addItem (((Channel) e.nextElement ()).getName ());
    commonPanel.setCursor (new Cursor (Cursor.DEFAULT_CURSOR));
  }


  public void actionPerformed(ActionEvent event) {
    String command = event.getActionCommand();

    if (command.equals("Save")) {
      ChannelSet channelSet = preferences.getChannelSet ();
      ChannelSet newChannelSet = new ChannelSet (channelSet, channelSet.getChannelList ()); // copy the set
      newChannelSet.removeAllChannels (); // remove channel selection

      commonPanel.enableAllComponents ();

      // Add new channels
      for (int i = 0; i < list_selected.getItemCount (); i++) {
	Channel channel = null;
	if ((channel = (Channel) channelSet.channelObject (list_selected.getItem (i))) == null)
	  newChannelSet.addChannelDefaultRate (list_selected.getItem (i));
	else
	  newChannelSet.addChannel (channel.getName (), channel.getRate ());
      }
      channelSet.replaceChannels (newChannelSet);
      close ();
    } else if (command.equals("KillGraphics")) {
      close ();
    } else 
      super.actionPerformed (event);
  }


  public void windowClosing (WindowEvent e) {
    close ();
  };

  void close () {
    if (commonPanel != null)
      commonPanel.enableAllComponents ();

    try {
      if (applet != null)
	applet.destroy ();
    } catch (Exception e) {
      ;
    }
    dispose ();
    System.gc ();
  }
}
