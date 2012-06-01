package org.csstudio.utility.pv.nds;

import java.io.*;
import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.net.*;

import org.csstudio.utility.pv.nds.*;


/**
 * Select channels
 */
class ChannelsDialog
  extends JdClosableDialog
  implements ActionListener, Debug, DataTypeConstants {

  Button okButton;
  Button cancelButton;
  ChannelSet channelSet;
  ChannelList serverList, selectionList;
  Button selectButton, removeButton;
  ChannelSet serverSet, selectionSet;

  public ChannelsDialog (Frame frame, String title, boolean modal, ChannelSet channelSet, CommonPanel commonPanel) {
    super (frame, title, commonPanel);
    this.channelSet = channelSet;

    setLayout (new GridBagLayout ());
    GridBagConstraints c = new GridBagConstraints ();

    c.insets = new Insets (0,2,0,2);
    c.gridwidth = 4;
    c.gridheight = 1;
    c.gridx = 0;
    c.gridy = 0;
    c.weightx = c.weighty = 0.0;
    add (new Label ("Server Set"), c);

    c.gridx = 6;
    add (new Label ("Selection"), c);

    serverList = new ChannelList (7);
    c.fill = GridBagConstraints.BOTH;
    c.gridy = 1;
    c.gridx = 0;
    c.weightx = c.weighty = 0.5;
    c.gridwidth = 4;
    c.gridheight = 10;
    add (serverList, c);

    //    c.fill = GridBagConstraints.NONE;
    c.gridy = 6;
    c.gridx = 4;
    c.weightx = c.weighty = .0;
    c.gridwidth = 2;
    c.gridheight = 1;

    selectButton = new Button (">>>");
    selectButton.addActionListener (this);
    add (selectButton, c);
    removeButton = new Button ("<<<");
    c.gridy = 8;
    removeButton.addActionListener (this);
    add (removeButton, c);

    selectionList = new ChannelList (7);
    c.fill = GridBagConstraints.BOTH;
    c.gridy = 1;
    c.gridx = 6;
    c.weightx = c.weighty = 0.5;
    c.gridwidth = 4;
    c.gridheight = 10;
    add (selectionList, c);

    selectionSet = new ChannelSet (channelSet, selectionList);
    serverSet = new ChannelSet (channelSet.getServerSet (), serverList);

    Panel p = new Panel ();
    p.setLayout (new FlowLayout (FlowLayout.CENTER, 15, 15));
    okButton = new Button ("OK");
    okButton.addActionListener (this);
    p.add (okButton);
    cancelButton = new Button ("Cancel");
    cancelButton.addActionListener (this);
    p.add (cancelButton);

    c.fill = GridBagConstraints.NONE;
    c.gridy = 16;
    c.gridx = 0;
    c.weightx = c.weighty = .0;
    c.gridwidth = 10;
    c.gridheight = 1;
    add (p, c);
    pack ();
    Display.centralise (this);
    //    setResizable (false);
    show ();
    commonPanel.setCursor (new Cursor (Cursor.DEFAULT_CURSOR));
  }
  

  public void performEnterAction () {
    channelSet.replaceChannels (selectionSet);
    close ();
  }

  public void actionPerformed (ActionEvent e) {
    Object src = e.getSource ();
    if (src == (Object) okButton) {
      performEnterAction ();
    } else if (src == (Object) cancelButton)
      close ();
    else if (src == (Object) selectButton) {
      selectionSet.addSelectedChannels (serverSet);
    } else if (src == (Object) removeButton) {
      selectionSet.removeSelectedChannels ();
    }
  }
}


