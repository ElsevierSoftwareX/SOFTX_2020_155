package org.csstudio.utility.pv.nds;

import java.io.*;
import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.net.*;

import org.csstudio.utility.pv.nds.*;


/**
 * Set data saving format, directory, modes
 */
class FilePreferencesDialog
  extends JdClosableDialog
  implements ItemListener, ActionListener, TextListener, Debug, DataTypeConstants {

  Button okButton;
  Button cancelButton;
  Button clearButton;
  Preferences preferences;
  Preferences newPreferences;
  TextField directoryTextField;
  TextField timePerFileTextField;
  Choice formatChoice;
  Checkbox filePerBlockCheckbox;
  Checkbox filePerChannelCheckbox;

  public FilePreferencesDialog (Frame frame, String title, boolean modal, Preferences preferences, CommonPanel commonPanel) {
    super (frame, title, commonPanel);
    this.preferences = preferences;
    newPreferences = new Preferences (preferences);

    setLayout (new GridBagLayout ());
    GridBagConstraints c = new GridBagConstraints ();
    c.fill = GridBagConstraints.BOTH;
    c.insets = new Insets (5,5,5,5);
    c.gridx = c.gridy = 0;

    c.gridwidth = 10;
    c.gridheight = 1;
    c.weightx = 1.0; c.weighty = .0;
    Panel p = new Panel ();
    p.setLayout (new GridBagLayout ());
    GridBagConstraints pc = new GridBagConstraints ();
    pc.insets = new Insets (5,5,5,5);
    pc.gridheight = 1;
    pc.gridwidth = 1;
    pc.weightx = pc.weighty = .0;
    pc.fill = GridBagConstraints.NONE;
    pc.gridx = 0;
    pc.gridy = 0;
    p.add (new Label ("Directory"), pc);
    pc.gridx = 1;
    pc.weightx = pc.weighty = 1.0;
    pc.fill = GridBagConstraints.HORIZONTAL;
    pc.gridwidth = 3;
    p.add (directoryTextField = new TextField (6), pc);
    directoryTextField.addTextListener (this);
    directoryTextField.setText (preferences.getFileDirectory ());
    pc.weightx = pc.weighty = .0;
    c.fill = GridBagConstraints.HORIZONTAL;
    add (p, c);

    p = new Panel ();
    p.setLayout (new GridBagLayout ());
    p.add (new Label ("Format"));
    p.add (formatChoice = new Choice ());
    formatChoice.addItemListener (this);
    for (int i = 0; i< preferences.knownExportFormatLabels.length; i++)
      formatChoice.addItem (preferences.knownExportFormatLabels [i]);
    formatChoice.select (preferences.getExportFormatIndex ());
    if (_debug > 5)
      System.out.println (preferences.getExportFormatIndex ());
    c.gridy = 1;
    add (p, c);

    c.gridy = 2;
    add (new Label ("New File Creation Rules:"), c);

    p = new Panel ();
    p.setLayout (new BorderLayout ());
    filePerBlockCheckbox = new Checkbox ("File per ", preferences.getFilePerBlockFlag ());
    filePerBlockCheckbox.addItemListener (this);

    p.add (filePerBlockCheckbox, "West");
    p.add (timePerFileTextField = new TextField (6));
    timePerFileTextField.addTextListener (this);
    timePerFileTextField.setText (preferences.getTimePerFileString ());
    p.add (new Label (" second(s)"), "East");

    //    c.fill = GridBagConstraints.NONE;
    c.gridy = 3;
    c.gridx = GridBagConstraints.REMAINDER;
    c.anchor = GridBagConstraints.WEST;
    c.insets = new Insets (5,25,5,5); // left indent
    c.weightx = c.weighty = 1.0;
    add (p, c);

    filePerChannelCheckbox =
      new Checkbox ("File per data channel", preferences.getFilePerChannelFlag ());
    filePerChannelCheckbox.addItemListener (this);
    c.gridy = 4;
    c.fill = GridBagConstraints.NONE;
    c.weightx = c.weighty = .0;
    add (filePerChannelCheckbox, c);

    if (preferences.frameFormatSelected ()) {
      // need to disable file format selections, there are rules here
      // 'file per data channels' toggle is always unset and disabled
      filePerChannelCheckbox.setState (false);
      filePerChannelCheckbox.setEnabled (false);
      if (preferences.getOnline () && preferences.getContinuous ()) {
	// 'file per N seconds' toggle is always set and disabled for inline DAQ
	filePerBlockCheckbox.setState (true);
	filePerBlockCheckbox.setEnabled (false);
      }
    }

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
    c.weightx = c.weighty = .0;
    add (p, c);
    pack ();
    Display.centralise (this);
    //    setResizable (false);
    show ();
    commonPanel.setCursor (new Cursor (Cursor.DEFAULT_CURSOR));
  }

  public void itemStateChanged (ItemEvent e) {
    Object src = e.getSource ();
    if (src == (Object) formatChoice) {
      if (_debug > 5)
	System.out.println (formatChoice.getSelectedIndex ());
      newPreferences.setExportFormatIndex (formatChoice.getSelectedIndex ());
      if (newPreferences.frameFormatSelected ()) {
	// need to disable file format selections, there are rules here
	// 'file per data channels' toggle is always unset and disabled
	filePerChannelCheckbox.setState (false);
	filePerChannelCheckbox.setEnabled (false);
	newPreferences.setFilePerChannelFlag (false);
	if (preferences.getOnline () && preferences.getContinuous ()) {
	  // 'file per N seconds' toggle is always set and disabled for inline DAQ
	  filePerBlockCheckbox.setState (true);
	  filePerBlockCheckbox.setEnabled (false);
	  newPreferences.setFilePerBlockFlag (true);
	}
      } else {
	filePerChannelCheckbox.setEnabled (true);
	filePerBlockCheckbox.setEnabled (true);
      }
      
    } else if (src == (Object) filePerBlockCheckbox) {
      newPreferences.setFilePerBlockFlag (filePerBlockCheckbox.getState ());
    } else if (src == (Object) filePerChannelCheckbox) {
      newPreferences.setFilePerChannelFlag (filePerChannelCheckbox.getState ());
    }
  }

  public void performEnterAction () {
    preferences.setValues (newPreferences);
    close ();
  }
  
  public void actionPerformed (ActionEvent e) {
    Object src = e.getSource ();
    if (src == (Object) okButton || e.getSource () instanceof TextField) {
      performEnterAction ();
    } else if (src == (Object) clearButton) {
      CommonPanel.clearFields (this);
      timePerFileTextField.setText ("1");
      formatChoice.select (0);
      clearButton.requestFocus ();
    } else if (src == (Object) cancelButton)
      close ();
  }

  public void textValueChanged (TextEvent e) {
    Object src = e.getSource ();
    if (src == (Object) directoryTextField) {
      newPreferences.setFileDirectory (directoryTextField.getText ());
    } else if (src == (Object) timePerFileTextField) {
      if (timePerFileTextField.getText ().length () == 0)
	newPreferences.setTimePerFile (1);
      else {
	try {
	  int period = Integer.parseInt (timePerFileTextField.getText ());
	  if (period <= 0)
	    throw new NumberFormatException ("invalid time");
	  
	  newPreferences.setTimePerFile (period);
	} catch (NumberFormatException ee) {
	  timePerFileTextField.setText (newPreferences.getTimePerFileString ());
	}
      }
    }
  }
}
