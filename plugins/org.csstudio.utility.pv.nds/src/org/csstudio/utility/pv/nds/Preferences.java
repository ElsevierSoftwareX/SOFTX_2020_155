package org.csstudio.utility.pv.nds;

import java.io.*;
import java.util.*;
import java.net.*;

public class Preferences implements Serializable, Debug, Defaults {
  int selectedFormat = 0;
  String fileDirectory = defaultFileDirectory;
  ConnectionParams connectionParams = null; // TCP port and IP address of the server
  ChannelSet channelSet = null; // selected channels
  GroupSet groupSet = null; // server channel groups
  boolean online;
  boolean continuous;
  int period;
  long gpsTime;
  boolean filePerBlockFlag = false;
  boolean filePerChannelFlag = false;
  int timePerFile = 1;
  URL imageMapURL;
  String framerHost = defaultFramerHost;
  int framerPort = defaultFramerPort;
  transient boolean isServlet = false;
  int acquisitionMode = 0;

  public transient static final String knownExportFormats [] = {"csv", "tabs", "raw", "frame", "bin"};
  public transient static final String knownExportFormatLabels []
    = {"Comma Separated Values", "Values Delimited With Tabs", "Raw Binary", "Frame", "Binary w/Header"};

  public transient static final String knownAcquisitionModes [] = {"full", "trend", "minute_trend"};
  public transient static final String knownAcquisitionModeLabels []
    = {"Full", "Trend", "Minute Trend"};

  //  public transient static final String knownAcquisitionModes [] = {"full", "trend"};
  //  public transient static final String knownAcquisitionModeLabels []
  //    = {"Full", "Trend"};


  transient Net net;

  public Preferences () {
    online = true;
    continuous = true;
    period = 0;
    gpsTime = 0;
    try {
      imageMapURL = new URL (defaultImageMapUrl);
    } catch (MalformedURLException e) {
      ;
    }
  }

  public Preferences (Preferences p) {
    setValues (p);
  }

  public void setValues (Preferences p) {
    online = p.getOnline ();
    continuous = p.getContinuous ();
    period = p.getPeriod ();
    gpsTime = p.getGpsTime ();
    selectedFormat = p.getExportFormatIndex ();
    fileDirectory = p.getFileDirectory ();
    filePerBlockFlag = p.getFilePerBlockFlag ();
    filePerChannelFlag = p.getFilePerChannelFlag ();
    timePerFile = p.getTimePerFile ();
    connectionParams = p.getConnectionParams ();
    imageMapURL = p.getImageMapURL ();
    framerHost = p.getFramerHost ();
    framerPort = p.getFramerPort ();
    isServlet = p.isServlet ();
    acquisitionMode = p.getAcquisitionModeIndex ();
    net = p.getNet ();
  }

  public void setNet (Net net) {
    this.net = net;
  }

  public Net getNet () {
    return net;
  }

  public URL getImageMapURL () {
    return imageMapURL;
  }

  public void setImageMapURL (URL url) {
    imageMapURL = url;
  }

  public String getFramerHost () {
    return framerHost;
  }

  public int getFramerPort () {
    return framerPort;
  }

  public void setFramerHost (String framerHost) {
    this.framerHost = framerHost;
  }

  public void setFramerPort (int port) {
    this.framerPort = port;
  }

  public String getFramerPortString () {
    return Integer.toString (framerPort);
  }

  public String getDaqdHost () {
    return connectionParams.getHost ();
  }

  public String getPortString () {
    return connectionParams.getHost () + ":" + Integer.toString (connectionParams.getPort ());
  }

  public String getTimePerFileString () {
    return Integer.toString (timePerFile);
  }

  public int getTimePerFile () {
    return timePerFile;
  }

  public void setTimePerFile (int timePerFile) {
    this.timePerFile = timePerFile;
  }

  public String getOnlineString () {
    return online? "true": "false";
  }

  public boolean getOnline () {
    return online;
  }

  public void  setOnline (boolean online) {
    this.online = online;
  }

  public String getContinuousString () {
    return continuous? "true": "false";
  }

  public boolean getContinuous () {
    return continuous;
  }

  public void setContinuous (boolean continuous) {
    this.continuous = continuous;
  }

  public String getPeriodString () {
    return Integer.toString (period);
  }

  public int getPeriod () {
    return period;
  }

  public void setPeriod (int period) {
    this.period = period;
  }

  public String getGpsTimeString () {
    return Long.toString (gpsTime);
  }

  public long getGpsTime () {
    return gpsTime;
  }

  public void setGpsTime (long gpsTime) {
    this.gpsTime = gpsTime;
  }
      
  public void savePreferences () {
    try {
      FileOutputStream ostream = new FileOutputStream (fileName ());
      ObjectOutputStream p = new ObjectOutputStream (ostream);
      p.writeObject (this);
      p.flush ();
      ostream.close ();
    } catch (Exception e) {
      if (_debug > 5)
	e.printStackTrace ();
    }
  }

  public static String fileName () {
    return System.getProperty ("user.home") + File.separator + ".jdclientrc";
  }

  public GroupSet getGroupSet () {
    return groupSet;
  }

  public void setGroupSet (GroupSet groupSet) {
    this.groupSet = groupSet;
  }

  public ChannelSet getChannelSet () {
    return channelSet;
  }

  public void setChannelSet (ChannelSet channelSet) {
    this.channelSet = channelSet;
  }

  public void updateChannelSet (ChannelSet channelSet) {
    // Update passed channel set with the this.channelSet channels
    if (this.channelSet != null) {
      for (Enumeration e = this.channelSet.elements (); e.hasMoreElements ();) {
	Channel ch = (Channel) e.nextElement ();
	channelSet.addChannel (ch.getName (), ch.getRate ());
      }
    }

    // Set this channel set to the passed one, so if this object is
    // serialized, the passed channel set will be serialized too
    setChannelSet (channelSet);
  }

  public void setConnectionParams (ConnectionParams connectionParams, boolean setValue) {
    if (this.connectionParams != null && setValue) {
      connectionParams.setHost (this.connectionParams.getHost ());
      connectionParams.setPort (this.connectionParams.getPort ());
    }
    this.connectionParams = connectionParams;
  }

  public ConnectionParams getConnectionParams () {
    return connectionParams;
  }

  public String getExportFormat () {
    return knownExportFormats [selectedFormat];
  }

  public int getExportFormatIndex () {
    return selectedFormat;
  }

  public boolean setExportFormatIndex (int idx) {
    if (idx >= 0 && idx < knownExportFormats.length) {
      selectedFormat = idx;
      if (_debug > 5)
	System.err.println ("Export format set to `" + knownExportFormats [idx] + "'");
      return true;
    }
    return false;
  }

  public boolean setExportFormat (String format) {
    for (int i = 0; i < knownExportFormats.length; i++)
      if (format.equalsIgnoreCase (knownExportFormats [i])) {
	if (_debug > 5)
	  System.err.println ("Export format set to `" + knownExportFormats [i] + "'");
	selectedFormat = i;
	return true;
      }
    return false;
  }

  public boolean csvFormatSelected () {
    return selectedFormat == 0;
  }

  public boolean tabsFormatSelected () {
    return selectedFormat == 1;
  }
  
  public boolean rawFormatSelected () {
    return selectedFormat == 2;
  }

  public boolean frameFormatSelected () {
    return selectedFormat == 3;
  }

  public boolean binaryWithHeaderFormatSelected () {
    return selectedFormat == 4;
  }

  public void setFileDirectory (String fileDirectory) {
    this.fileDirectory = fileDirectory;
  }

  public String getFileDirectory () {
    return fileDirectory;
  }

  // Substitute emty directory for the current one
  public String getFileDirectoryCurrentPlus () {
    return fileDirectory.length () == 0 ?
      System.getProperty("user.dir") : fileDirectory;
  }

  public boolean getFilePerChannelFlag () {
    return filePerChannelFlag;
  }

  public void setFilePerChannelFlag (boolean flag) {
    filePerChannelFlag = flag;
  }

  public boolean getFilePerBlockFlag () {
    return filePerBlockFlag;
  }

  public void setFilePerBlockFlag (boolean flag) {
    filePerBlockFlag = flag;
  }

  public boolean isServlet () {
    return isServlet;
  }

  public void setIsServlet (boolean isServlet) {
    this.isServlet = isServlet;
  }

  public String getAcquisitionMode () {
    return knownAcquisitionModes [acquisitionMode];
  }

  public int getAcquisitionModeIndex () {
    return acquisitionMode;
  }

  public boolean setAcquisitionModeIndex (int idx) {
    if (idx >= 0 && idx < knownAcquisitionModes.length) {
      acquisitionMode = idx;
      if (_debug > 5)
	System.err.println ("Acquisition mode set to `" + knownAcquisitionModes [idx] + "'");
      return true;
    }
    return false;
  }

  public boolean setAcquisitionMode (String mode) {
    for (int i = 0; i < knownAcquisitionModes.length; i++)
      if (mode.equalsIgnoreCase (knownAcquisitionModes [i])) {
	if (_debug > 5)
	  System.err.println ("Acquisition mode set to `" + knownAcquisitionModes [i] + "'");
	acquisitionMode = i;
	return true;
      }

    if (_debug > 5)
      System.err.println ("Acquisition unchanged, unknown mode `" + mode + "' specified");
    return false;
  }

  public boolean fullAcquisitionModeSelected () {
    return acquisitionMode == 0;
  }

  public boolean trendAcquisitionModeSelected () {
    return acquisitionMode == 1;
  }

  public boolean minuteTrendAcquisitionModeSelected () {
    return acquisitionMode == 2;
  }

  public String getModeFileNameString () {
    if (fullAcquisitionModeSelected ())
      return "";
    else
      return "_" + getAcquisitionMode ();
  }

  // See if the current preferences require a connection to
  // nds_framer -- frames application server
  public boolean doConnectToFrameApplicationServer () {
    if (frameFormatSelected ()) {
      return true;
    }
    return false;
  }
}
