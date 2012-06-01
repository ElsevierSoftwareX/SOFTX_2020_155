package org.csstudio.utility.pv.nds;

import java.io.*;
import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.net.*;

import org.csstudio.utility.pv.nds.*;

/**
 * Set daq preferences: timing mostly
 */
class DaqPreferencesDialog
  extends JdClosableDialog
  implements ItemListener, ActionListener, FocusListener, TextListener, Debug, DataTypeConstants {

  Button okButton;
  Button cancelButton;
  Button clearButton;
  Preferences preferences;
  Preferences newPreferences;
  CheckboxGroup checkboxGroup;
  Checkbox continuousCheckbox;
  Checkbox periodCheckbox;
  Checkbox offlineCheckbox;
  TextField periodTextField;
  TextField offlinePeriodTextField;
  TextField periodDayTextField;
  TextField periodHourTextField;
  TextField periodMinuteTextField;
  TextField periodSecondTextField;
  TextField gpsTextField;
  TextField gpsYearTextField; 
  TextField gpsMonthTextField;
  TextField gpsDayTextField;
  TextField gpsHourTextField;
  TextField gpsMinuteTextField;
  TextField gpsSecondTextField;

  public DaqPreferencesDialog (Frame frame, String title, boolean modal, Preferences preferences, CommonPanel commonPanel) {
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
    //    MultiLineLabel label = new MultiLineLabel ("Data Acquisition Control");
    //    add (label, c);

    checkboxGroup = new CheckboxGroup ();
    continuousCheckbox = new Checkbox ("Online Continuous", checkboxGroup,
				       preferences.getOnline () && preferences.getContinuous ());
    continuousCheckbox.addItemListener (this);
    c.fill = GridBagConstraints.NONE;
    c.anchor = GridBagConstraints.WEST;
    c.weightx = c.weighty = 0.0;
    c.gridy = 1;
    add (continuousCheckbox, c);

    periodCheckbox = new Checkbox ("Online Period", checkboxGroup,
				   preferences.getOnline () && !preferences.getContinuous ());
    periodCheckbox.addItemListener (this);
    c.gridy = 2;
    add (periodCheckbox, c);

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
    p.add (new Label ("For"), pc);
    pc.gridx = 1;
    pc.weightx = pc.weighty = 1.0;
    pc.fill = GridBagConstraints.HORIZONTAL;
    pc.gridwidth = 3;
    p.add (periodTextField = new TextField (6), pc);
    periodTextField.addFocusListener (this);
    periodTextField.addTextListener (this);
    periodTextField.setText (preferences.getPeriodString ());
    pc.weightx = pc.weighty = .0;
    pc.fill = GridBagConstraints.NONE;
    pc.gridwidth = 1;
    pc.gridx = 4;
    p.add (new Label ("second(s)"), pc);
    c.fill = GridBagConstraints.HORIZONTAL;
    c.gridy = 3;
    c.weightx = 1.0; c.weighty = .0;
    c.gridwidth = 5;
    add (p, c);

    offlineCheckbox = new Checkbox ("Offline", checkboxGroup, !preferences.getOnline ());
    offlineCheckbox.addItemListener (this);
    c.fill = GridBagConstraints.NONE;
    c.weightx = c.weighty = 0.0;
    c.gridy = 4;
    add (offlineCheckbox, c);

    p = new Panel ();
    p.setLayout (new GridBagLayout ());
    pc = new GridBagConstraints ();
    pc.fill = GridBagConstraints.BOTH;
    pc.insets = new Insets (5,5,5,5);

    pc.gridheight = 1;
    pc.gridwidth = 6;
    pc.weightx = pc.weighty = 0.0;
    pc.gridx = 0;
    pc.gridy = 0;
    p.add (new Label ("UTC Time"), pc);

    pc.gridy = 1;
    pc.gridwidth = 1;
    p.add (new Label ("YR"), pc);
    pc.gridx = 1;
    p.add (new Label ("MO"), pc);
    pc.gridx = 2;
    p.add (new Label ("DAY"), pc);
    pc.gridx = 3;
    p.add (new Label ("HR"), pc);
    pc.gridx = 4;
    p.add (new Label ("MIN"), pc);
    pc.gridx = 5;
    p.add (new Label ("SEC"), pc);

    pc.gridx = 0;
    pc.gridy = 3;
    pc.gridwidth = 2;
    p.add (new Label ("Period:"), pc);

    pc.gridy = 0;
    pc.gridwidth = 2;
    pc.gridheight = 2;
    pc.gridx = 6;
    p.add (new Label ("GPS Time"), pc);
    pc.gridx = 8;
    p.add (new Label ("Period"), pc);

    pc.gridy = 2;
    pc.gridwidth = pc.gridheight = 1;
    pc.weightx = pc.weighty = 1.0;
    pc.gridx = 0;

    p.add (gpsYearTextField = new TextField (4), pc);
    gpsYearTextField.addFocusListener (this);
    gpsYearTextField.addTextListener (this);
    pc.gridx++;

    p.add (gpsMonthTextField = new TextField (2), pc);
    gpsMonthTextField.addFocusListener (this);
    gpsMonthTextField.addTextListener (this);
    pc.gridx++;

    p.add (gpsDayTextField = new TextField (2), pc);
    gpsDayTextField.addFocusListener (this);
    gpsDayTextField.addTextListener (this);
    pc.gridx++;

    p.add (gpsHourTextField = new TextField (2), pc);
    gpsHourTextField.addFocusListener (this);
    gpsHourTextField.addTextListener (this);
    pc.gridx++;

    p.add (gpsMinuteTextField = new TextField (2), pc);
    gpsMinuteTextField.addFocusListener (this);
    gpsMinuteTextField.addTextListener (this);
    pc.gridx++;

    p.add (gpsSecondTextField = new TextField (2), pc);
    gpsSecondTextField.addFocusListener (this);
    gpsSecondTextField.addTextListener (this);
    pc.gridx++;

    gpsYearTextField.setEnabled (false);
    gpsMonthTextField.setEnabled (false);
    gpsDayTextField.setEnabled (false);
    gpsHourTextField.setEnabled (false);
    gpsMinuteTextField.setEnabled (false);
    gpsSecondTextField.setEnabled (false);

    pc.gridwidth = 2;
    p.add (gpsTextField = new TextField (12), pc);
    gpsTextField.addFocusListener (this);
    gpsTextField.addTextListener (this);
    gpsTextField.setText (preferences.getGpsTimeString ());

    pc.gridy = 3;
    pc.gridwidth = pc.gridheight = 1;
    pc.weightx = pc.weighty = 1.0;    

    pc.gridx = 2;
    p.add (periodDayTextField = new TextField (2), pc);
    periodDayTextField.addFocusListener (this);
    periodDayTextField.addTextListener (this);
    pc.gridx++;
    p.add (periodHourTextField = new TextField (2), pc);
    periodHourTextField.addFocusListener (this);
    periodHourTextField.addTextListener (this);
    pc.gridx++;
    p.add (periodMinuteTextField = new TextField (2), pc);
    periodMinuteTextField.addFocusListener (this);
    periodMinuteTextField.addTextListener (this);
    pc.gridx++;
    p.add (periodSecondTextField = new TextField (2), pc);
    periodSecondTextField.addFocusListener (this);
    periodSecondTextField.addTextListener (this);
    pc.gridx++;

    pc.gridx += 2;
    pc.gridwidth = 2;
    p.add (offlinePeriodTextField = new TextField (12), pc);
    offlinePeriodTextField.addFocusListener (this);
    offlinePeriodTextField.addTextListener (this);
    offlinePeriodTextField.setText (preferences.getPeriodString ());

    c.fill = GridBagConstraints.HORIZONTAL;
    c.weightx = 1.0;
    c.gridy = 5;
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
    c.weightx = c.weighty = .0;
    add (p, c);
    pack ();
    Display.centralise (this);
    //    setResizable (false);
    show ();
    commonPanel.setCursor (new Cursor (Cursor.DEFAULT_CURSOR));
  }

  /**
   * Return "0" for the empty string or the string passed otherwise
   */
  String nvl (String instr) {
    if (instr.length () == 0)
      return "0";
    else
      return instr;
  }

  public void textValueChanged (TextEvent e) {
    //    System.out.println (e.paramString ());
    Object src = e.getSource ();

    try {
      if ((src == (Object) periodTextField
	   || src == (Object) offlinePeriodTextField)
	  && (newPreferences.getPeriod ()
	      == Integer.parseInt (nvl (((TextField) src).getText ())))) {
	setPeriodFields (newPreferences.getPeriod ());
	return;
      }
    } catch (NumberFormatException ee) {
      ;
    }

    /*
    try {
      if (src == (Object) gpsTextField
	  && (newPreferences.getGpsTime ()
	      == Long.parseLong (nvl (((TextField) src).getText ())))) {
	setGpsFields (newPreferences.getGpsTime ());
	return;
      }
    } catch (NumberFormatException ee) {
      ;
    }
    */

    if (src == (Object) periodTextField) {
      if (setPreferencesPeriod (periodTextField))
	offlinePeriodTextField.setText (periodTextField.getText ());
      else
	periodTextField.setText (newPreferences.getPeriodString ());
    } else if (src == (Object) offlinePeriodTextField) {
      if (setPreferencesPeriod (offlinePeriodTextField)) {
	periodTextField.setText (offlinePeriodTextField.getText ());
	//	setPeriodFields (newPreferences.getPeriod ());
      } else
	offlinePeriodTextField.setText (newPreferences.getPeriodString ());
    } else if (src == (Object) periodDayTextField
	       || src == (Object) periodHourTextField
	       || src == (Object) periodMinuteTextField
	       || src == (Object) periodSecondTextField) {
      try {
	int newDay = Integer.parseInt (nvl (periodDayTextField.getText ()));
	int newHour = Integer.parseInt (nvl (periodHourTextField.getText ()));
	int newMinute = Integer.parseInt (nvl (periodMinuteTextField.getText ()));
	int newSecond = Integer.parseInt (nvl (periodSecondTextField.getText ()));

	int newPeriod = newDay * 3600 * 24 + newHour * 3600 + newMinute * 60 + newSecond;

	if (newPeriod != newPreferences.getPeriod ())
	  periodTextField.setText (Integer.toString (newPeriod));
      } catch (NumberFormatException ee) { 
	//	System.out.println ("can't parse one of the period fields");
	periodTextField.setText (newPreferences.getPeriodString ());
      }
    } else if (src == (Object) gpsTextField) {
      if (! setGpsTime (gpsTextField))
	gpsTextField.setText (newPreferences.getGpsTimeString ());

	//	long newGps = Long.parseLong (gpsTextField.getText ());
	//	if (newGps != newPreferences.getGpsTime ()) {
	//	  newPreferences.setGpsTime (newGps);
	//	  setGpsFields (newGps);

      /*
    } else if (src == (Object) gpsYearTextField
	       || src == (Object) gpsMonthTextField
	       || src == (Object) gpsDayTextField
	       || src == (Object) gpsHourTextField
	       || src == (Object) gpsMinuteTextField
	       || src == (Object) gpsSecondTextField) {
      try {
	int newYear = Integer.parseInt (nvl (gpsYearTextField.getText ()));
	int newMonth = Integer.parseInt (nvl (gpsMonthTextField.getText ()));
	int newDay = Integer.parseInt (nvl (gpsDayTextField.getText ()));
	int newHour = Integer.parseInt (nvl (gpsHourTextField.getText ()));
	int newMinute = Integer.parseInt (nvl (gpsMinuteTextField.getText ()));
	int newSecond = Integer.parseInt (nvl (gpsSecondTextField.getText ()));
	GPS gps = new GPS (newYear, newMonth, newDay, newHour, newMinute, newSecond);

	if (gps.getTimeInSeconds () != newPreferences.getGpsTime ())
	  gpsTextField.setText (Long.toString (gps.getTimeInSeconds ()));
      } catch (NumberFormatException ee) { 
	gpsTextField.setText (newPreferences.getGpsTimeString ());
	;
	//	System.out.println ("can't parse one of GPS fields");
	//	setGpsFields (newPreferences.getGpsTime ());
      }
      */
    }
  }

  public void focusLost (FocusEvent e) {
    ;
  }

  public void focusGained (FocusEvent e) {
    Object src = e.getSource ();

    if (src == (Object) periodTextField) {
      periodCheckbox.setState (true);
      itemStateChanged (new ItemEvent (periodCheckbox,0,periodCheckbox,0));
    } else {
      offlineCheckbox.setState (true);
      itemStateChanged (new ItemEvent (offlineCheckbox,0,offlineCheckbox,0));
    }
  }

  public void itemStateChanged (ItemEvent e) {
    Object src = e.getSource ();
    if (src == (Object) continuousCheckbox) {
      newPreferences.setOnline (true);
      newPreferences.setContinuous (true);
    } else if (src == (Object) periodCheckbox) {
      newPreferences.setOnline (true);
      newPreferences.setContinuous (false);
      setPreferencesPeriod (periodTextField);
    } else if (src == (Object) offlineCheckbox) {
      newPreferences.setOnline (false);
    }
  }
  
  public void performEnterAction () {
    preferences.setValues (newPreferences);
    if (_debug > 10) {
      System.out.print ("online=" + preferences.getOnlineString ());
      System.out.print (" continuous=" + preferences.getContinuous ());
      System.out.print (" gpsTime=" + preferences.getGpsTimeString ());
      System.out.println (" period=" + preferences.getPeriodString ());
    }
    close ();
  }


  public void actionPerformed (ActionEvent e) {
    Object src = e.getSource ();
    if (src == (Object) okButton) {
      performEnterAction ();
    } else if (src == (Object) clearButton) {
      CommonPanel.clearFields (this);
      checkboxGroup.setSelectedCheckbox (continuousCheckbox);
      clearButton.requestFocus ();
    } else if (src == (Object) cancelButton)
      close ();
  }

  boolean setPreferencesPeriod (TextField t) {
    try {
      if (t.getText ().length () == 0)
	newPreferences.setPeriod (0);
      else
	newPreferences.setPeriod (Integer.parseInt (t.getText ()));
      return true;
    } catch (NumberFormatException ee) {
      //      t.setText ("1");
      //      newPreferences.setPeriod (1);
       return false;
    }
  }


  void setPeriodFields (int period) {
    int day = period / 3600 / 24;
    int hour = period / 3600 % 24;
    int minute = period % 3600 / 60;
    int second = period % 60;
    boolean printZero;

    periodDayTextField.setText ((printZero = day > 0)? Integer.toString (day): "");
    periodHourTextField.setText (hour > 0? Integer.toString (hour): printZero? "0": "");
    printZero |= hour > 0;
    periodMinuteTextField.setText (minute > 0? Integer.toString (minute): printZero? "0": "");
    printZero |= minute > 0;
    periodSecondTextField.setText (second > 0? Integer.toString (second): printZero? "0": "");
  }

  boolean setGpsTime (TextField t) {
    try {
      if (t.getText ().length () == 0)
	newPreferences.setGpsTime (0);
      else
	newPreferences.setGpsTime (Long.parseLong (t.getText ()));
      return true;
    } catch (NumberFormatException ee) {
      //      t.setText ("???");
      //      newPreferences.setGpsTime (1);
       return false;
    }
  }

  void setGpsFields (long gpsTime) {
    GPS gps = new GPS (gpsTime);

    gpsYearTextField.setText (Integer.toString (gps.get (Calendar.YEAR)));
    gpsMonthTextField.setText (Integer.toString (gps.get (Calendar.MONTH)));
    gpsDayTextField.setText (Integer.toString (gps.get (Calendar.DATE)));
    gpsHourTextField.setText (Integer.toString (gps.get (Calendar.HOUR)));
    gpsMinuteTextField.setText (Integer.toString (gps.get (Calendar.MINUTE)));
    gpsSecondTextField.setText (Integer.toString (gps.get (Calendar.SECOND)));
  }
}
