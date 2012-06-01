package org.csstudio.utility.pv.nds;

import java.awt.*;
import java.io.*;

/**
 * Data acquisition control class
 * Connects network, channel set and file save classes
 */
public class DataAcquisition implements Runnable, Debug {
  protected Label progressLabel;
  Label fileLabel;
  protected ChannelSet channelSet;
  protected Thread thread;
  protected int dataBlocks;
  protected CommonPanel commonPanel;
  protected Preferences preferences;
  boolean textFile = true;
  protected FileSaver fsv  = null;
  boolean running;
  OutputStream outputStream;
  Heartbeat heartbeat = null;

  public DataAcquisition (Preferences preferences) {
    this (null, preferences);
  }

  public DataAcquisition (Preferences preferences, OutputStream outputStream) {
    this (null, preferences);
    this.outputStream = outputStream;
  }


  public DataAcquisition (CommonPanel commonPanel, Preferences preferences) {
    progressLabel = null;
    fileLabel = null;
    thread = null;
    dataBlocks = 0;
    this.commonPanel = commonPanel;
    this.preferences = preferences;
    textFile = preferences.csvFormatSelected () || preferences.tabsFormatSelected ();
    outputStream = null;
  }

  public void join () {
    if (thread != null) {
      try {
	thread.join ();
      } catch (InterruptedException e) {
	System.err.println ("data acquisition thread was interrupted");
      }
    }
  }

  /**
   * Begin data acquisition
   */
  public synchronized boolean start () {
    try {
      if (thread == null) {
	thread  = new Thread (this);
	thread.start ();
      } else 
	thread.resume ();
    } catch (Exception e) {
      System.err.println ("While starting Daq thread:");
      System.err.println (e);
      thread = null;
      return false;
    }
    return true;
  }

  protected void updateProgressLabel () {
    if (progressLabel != null) {
      if (dataBlocks == 0)
	progressLabel.setText ("no data blocks were processed");
      else if (dataBlocks == 1)
	progressLabel.setText ("single data block processed");
      else
	progressLabel.setText (dataBlocks + " data blocks processed");
    }
  }


  /**
   * Stop data acquisition and disconnect from the server
   */
  public synchronized void stop () {
    running = false;

    if (_debug > 8)
      System.err.println ("DataAcquisition.stop () called");

    /*
    if (thread != null) {
      preferences.getNet ().stopNetWriter ();
      fsv.endSavingSession ();
      updateProgressLabel ();
      thread.stop ();
      if (commonPanel != null)
        commonPanel.enableMenu ();
      thread = null;
    }
    */
  }

  public void kill () {
    if (thread != null) {
      thread.stop ();
      thread = null;
    }
  }

  /**
   * Data acquisition thread
   */
  public void run () {
    if (commonPanel != null)
      commonPanel.disableMenu ();

    if (progressLabel != null)
      progressLabel.setText ("no data blocks processed yet");

    try {
      if (preferences.doConnectToFrameApplicationServer ()) {
	fsv = new FileSaveFormatBlock (preferences, fileLabel, outputStream);
      } else if (! preferences.getFilePerBlockFlag () && ! preferences.getFilePerChannelFlag ()) {
	/*
	  One file created
	*/
	fsv = new FileSaveFormatSingle (preferences, fileLabel, outputStream);
      } else if (preferences.getFilePerBlockFlag () && ! preferences.getFilePerChannelFlag ()) {
	/*
	  New file is created for every `preferences.getTimePerFiles ()' seconds
	*/
	fsv = new FileSaveFormatPeriod (preferences, fileLabel);
      } else if (! preferences.getFilePerBlockFlag () && preferences.getFilePerChannelFlag ()) {
	/*
	  New file is created for each data channel acquired
	*/
	fsv = new FileSaveFormatChannel (preferences, fileLabel);
      } else if (preferences.getFilePerBlockFlag () && preferences.getFilePerChannelFlag ()) {
	/*
	  New file is created for each data channel acquired
	  for every `preferences.getTimePerFiles ()' seconds
	*/
	fsv = new FileSaveFormatChannelPeriod (preferences, fileLabel);
      }

      fsv.startSavingSession ();

      if (preferences.getOnline () && !preferences.getContinuous ())
	runClientControlled (fsv);   // Run for a specified period of time
      else
	runServerControlled (fsv);  // get all the data from the server until the server ends the transmission
      
      preferences.getNet ().stopNetWriter ();
      updateProgressLabel ();

      //      if (outputStream == null)
      fsv.endSavingSession ();
    } catch (Exception e) {
      e.printStackTrace ();
    }

    thread = null;
    if (commonPanel != null)
      commonPanel.enableMenu ();
  }


  /**
   * Period is controlled by the server as specified in the request
   */
  protected void runServerControlled (FileSaver fsv) {
    dataBlocks = 0;
    Net net = preferences.getNet ();

    if (! net.startNetWriter (preferences.getChannelSet (),
			      preferences.getGpsTime (),
			      preferences.getOnline ()? 0: preferences.getPeriod ()))
      return;
    running = true;
    for (;;) {
      DataBlock block = net.getDataBlock ();
      if (block.getEot ())
	break;
      if (heartbeat != null && block.getHeartbeat ()) {
	if (heartbeat.isDead (block.getTimestamp ())) {
	  break;
	}
	continue;
      }
      
      if (!fsv.saveFile (block)) {
	new Alert (commonPanel.frame, "Couldn't save file in \n"
		   + preferences.getFileDirectoryCurrentPlus () + " directory.\nPlease make sure it's writable.");
	return;
      } else {
	if (progressLabel != null)
	  progressLabel.setText ("block " + dataBlocks);
      }

      synchronized (this) {
	if (_debug > 8)
	  System.err.println ("DataAcquisition.runServerControlled(); running=" + running);
	if (! running)
	  return;
      }
      dataBlocks++;
    }
  }


  /**
   * Time period on-line data acquisition, controlled locally
   */
  protected void runClientControlled (FileSaver fsv) {
    dataBlocks = 0;
    int period = preferences.getPeriod ();
    
    Net net = preferences.getNet ();
    if (! net.startNetWriter (preferences.getChannelSet (), 0, 0))
      return;
    
    int finalTime = 0;

    running = true;
    for (int i = 0;;) {
      DataBlock block = net.getDataBlock ();
      if (block.getEot ())
	break;
      if (heartbeat != null && block.getHeartbeat ()) {
	if (heartbeat.isDead (block.getTimestamp ())) {
	  break;
	}
	continue;
      }

      if (i == 0)
	finalTime = block.getTimestamp () + period;
      else
	if (block.getTimestamp () >= finalTime)
	  break;

      if (! fsv.saveFile (block)) {
	new Alert (commonPanel.frame, "Couldn't save file in \n"
		   + preferences.getFileDirectoryCurrentPlus () + " directory.\nPlease make sure it's writable.");
	return;
      } else if (progressLabel != null)
	progressLabel.setText ("block " + i);

      synchronized (this) {
	if (_debug > 8)
	  System.err.println ("DataAcquisition.runClientControlled(); running=" + running);
	if (! running)
	  return;
      }
      dataBlocks++;
      i++;

      if (block.getTimestamp () + block.getPeriod () >= finalTime)
	break;
    }
  }


  public void setProgressLabel (Label progressLabel) {
    this.progressLabel = progressLabel;
  }

  public void setFileLabel (Label fileLabel) {
    this.fileLabel = fileLabel;
  }

  public int getNumBlocksProcessed () {
    return dataBlocks;
  }

  public void setHeartbeat (Heartbeat heartbeat) {
    this.heartbeat = heartbeat;
  }

  public Heartbeat getHeartbeat () {
    return heartbeat;
  }
}

