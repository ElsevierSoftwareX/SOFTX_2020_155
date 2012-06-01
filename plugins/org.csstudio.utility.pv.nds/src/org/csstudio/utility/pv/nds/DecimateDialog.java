package org.csstudio.utility.pv.nds;

import java.io.*;
import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.net.*;

import org.csstudio.utility.pv.*;

/**
 * Decimate selected channels
 */
class DecimateDialog
  extends JdClosableDialog
  implements ActionListener, Debug, DataTypeConstants {

  Button okButton;
  Button cancelButton;
  ChannelSet channelSet;
  Choice rateChoice;
  int [] selectedIndexes;

  public DecimateDialog (Frame frame, String title, boolean modal, ChannelSet channelSet, CommonPanel commonPanel) {
    super (frame, title, commonPanel);
    this.channelSet = channelSet;

    selectedIndexes = channelSet.getChannelList ().getSelectedIndexes ();
    setLayout (new GridBagLayout ());
    GridBagConstraints c = new GridBagConstraints ();
    c.fill = GridBagConstraints.BOTH;
    c.insets = new Insets (5,5,5,5);
    c.gridx = c.gridy = 0;


    MultiLineLabel label =
      new MultiLineLabel ("Decimate "
			  + (selectedIndexes.length > 0 ? "selected": "all")
			  + " channels to the following rate:");

    c.gridwidth = c.gridheight = 1;
    c.weightx = c.weighty = 1.0;
    add (label, c);

    rateChoice = new Choice ();
    for (int i = 0; i < validRates.length; i++)
      rateChoice.add (String.valueOf (validRates [i]));
    c.gridy = 1;
    c.weightx = c.weighty = .0;
    c.fill = GridBagConstraints.NONE;
    add (rateChoice, c);
    
    Panel p = new Panel ();
    p.setLayout (new FlowLayout (FlowLayout.CENTER, 15, 15));
    okButton = new Button ("OK");
    okButton.addActionListener (this);
    p.add (okButton);
    cancelButton = new Button ("Cancel");
    cancelButton.addActionListener (this);
    p.add (cancelButton);
    c.gridy = 2;
    add (p, c);
    pack ();
    Display.centralise (this);
    // setResizable (false);
    show ();
    commonPanel.setCursor (new Cursor (Cursor.DEFAULT_CURSOR));
  }

  public void performEnterAction () {
    //      System.out.println (validRates [rateChoice.getSelectedIndex ()]);

    if (selectedIndexes.length > 0)
      channelSet.changeRate (selectedIndexes, validRates [rateChoice.getSelectedIndex ()]);
    else
      channelSet.changeRate (validRates [rateChoice.getSelectedIndex ()]);
    close ();
  }

  public void actionPerformed (ActionEvent e) {
    Object src = e.getSource ();
    if (src == (Object) okButton) {
      performEnterAction ();
    } else if (src == (Object) cancelButton)
      close ();
  }
}
