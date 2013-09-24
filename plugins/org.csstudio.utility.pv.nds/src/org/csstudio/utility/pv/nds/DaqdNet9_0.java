package org.csstudio.utility.pv.nds;

import java.awt.*;
import java.net.*;
import java.io.*;
import java.util.*;

/**
 * DAQD network connection and communication protocol
 */
public class DaqdNet9_0 extends Net implements DataTypeConstants, Debug {
  static int version = 9;
  static int revision = 0;
  Label infoLabel;
  int blockCount;

  boolean startNetWriterDisco;

  public DaqdNet9_0 () {
    startNetWriterDisco = false;    
  }

  public DaqdNet9_0 (Label infoLabel, Preferences pref) {
    super (pref);
    this.infoLabel = infoLabel;
    startNetWriterDisco = false;
  }

  public void setLabel (Label infoLabel) {
    this.infoLabel = infoLabel;
  }

  public Label getLabel () {
    return infoLabel;
  }

  public boolean connect () {
    return connect (false);
  }

  /**
   * Open network socket and crate input/output data streams
   */
  public boolean connect (boolean considerApplicationServer) {
    if (super.connect (considerApplicationServer)) {
      if (infoLabel != null)
	infoLabel.setText ("connected to " + getSocketString ());
      return true;
    } 

    if (infoLabel != null)
      infoLabel.setText ("Could not connect to " + getSocketString ());
    return false;
  }

  /**
   * Close communication socket
   */
  public boolean disconnect () {
    if (super.disconnect ()) {
      if (infoLabel != null)
	infoLabel.setText ("disconnected from " + getSocketString ());
      return true;
    } 
    if (infoLabel != null)
      infoLabel.setText ("Could not disconnect from " + getSocketString ());
    return false;
  }

  /**
   * Contact the DAQD server and get channel group set
   */
  public GroupSet getGroups () {
    GroupSet grSet;
    boolean disco = false;

    if (clientSocket == null) {
      if (!connect ()) {
	return null;
      }
      disco = true;
    }
    grSet = new GroupSet ();

    try {
            
      // Request channel group names from the server
      String command = "status channel-groups;";
      if (_debug > 6)
	System.err.println("DaqdNet 9.0: about to send `" + command + "'");

      sout.println (command);
      if (_debug > 6)
	System.err.println("DaqdNet 9.0: command was sent");
      
      // Read response, which is four bytes -- four hex digits
      int resp = readResp (is);

      if (resp != 0) {
	String msg = "Got response `" + resp + "' from the server on `" + command + "'";
	if (infoLabel != null)
	  infoLabel.setText(msg);
	throw new IOException (msg);
      }

      if (_debug > 6)
	System.err.println ("got ACK from the server");

      // Get the number of channel groups
      int grnum = readResp (is);

      for (int i = 0; i < grnum; i++) {
	String grName = readString (is, MAX_CHANNEL_NAME_LENGTH).trim (); 
	int grNumber = readResp (is); // Get rate
	grSet.addGroup (grName, grNumber);
	if (_debug > 8)
	  System.err.println (grName + "\t" + grNumber);
      }
    } catch (IOException e) {
      grSet = null;
    }

    if (disco) {
      disconnect ();
    }
    return grSet;
  }

  /**
   * Contact the DAQD server and get a set of data channels
   */
  public ChannelSet getChannels () {
    ChannelSet chSet;
    boolean disco = false;

    if (clientSocket == null) {
      if (!connect ()) {
	return null;
      }
      disco = true;
    }
    chSet = new ChannelSet ();

    try {
            
      // Request channel names from the server
      String command = "status channels;";
      if (_debug > 6)
	System.err.println("DaqdNet 9.0: about to send `" + command + "'");

      sout.println (command);
      if (_debug > 6)
	System.err.println("DaqdNet 9.0: command was sent");
      
      // Read response, which is four bytes -- four hex digits
      int resp = readResp (is);

      if (resp != 0) {
	String msg = "Got response `" + resp + "' from the server on `" + command + "'";
	if (infoLabel != null)
	  infoLabel.setText(msg);
	throw new IOException (msg);
      }

      if (_debug > 6)
	System.err.println ("got ACK from the server");

      // Get the number of channels
      int chnum = readResp (is);
      int daqd_clock = readResp (is);

      for (int i = 0; i < chnum; i++) {
	String chName = readString (is, MAX_CHANNEL_NAME_LENGTH).trim (); // Get channel name
	int chRate = readResp (is); // Get rate
	boolean chTrend = readResp (is) != 0; // Get trend flag
	int chGrp = readResp (is);
	int bps = readResp (is);
	int dataType = readResp (is);
	chSet.addChannel (chName, chRate, chGrp,  bps, dataType, chTrend, i);
	if (_debug > 8)
	  System.err.println (chName + "\t" + chRate);
      }
      if (_debug > 8)
	System.err.println (chnum + " channel on server");

    } catch (IOException e) {
      chSet = null;
    }

    if (disco) {
      disconnect ();
    }
    return chSet;
  }

  String getBlockString () {
    return
      " bs="
      + (preferences.getContinuous () && preferences.getOnline ()
	 ? preferences.getTimePerFileString ()
	 : (preferences.getFilePerBlockFlag ()
	    ? preferences.getTimePerFileString ()
	    : preferences.getPeriodString ()))
      + (preferences.isServlet ()? " heartbeat": " ");
  }

  /**
   * Send `start net-writer' command to start the data acquisition for `chSet' channels
   */
  public boolean startNetWriter (ChannelSet chSet, long gps, int period) {
    blockCount = 0;
    if (clientSocket == null) {
      if (!connect (true))
	return false;
      startNetWriterDisco = true;
    }
    //  if (infoLabel != null)
    //    infoLabel.setText ("net-writer started");

    // Initiate transmission
    String command = null;
    if (preferences.frameFormatSelected ()) {
      command = "start frame-writer"
	+ (gps > 0 ? (" " + Long.toString (gps)): "")
	+ (period > 0 ? (" " + Integer.toString (period)): "")
	+ (preferences.doConnectToFrameApplicationServer () ? getBlockString (): "")
	+ " { " + getChannelRateString (chSet, false) + " };";
    } else {
      command = "start ";
      if (preferences.trendAcquisitionModeSelected ())
	command += "trend net-writer";
      else if (preferences.minuteTrendAcquisitionModeSelected ())
	command += "trend 60 net-writer";
      else
	command += "net-writer";

      command += (gps > 0 ? (" " + Long.toString (gps)): "")
	+ (period > 0 ? (" " + Integer.toString (period)): "")
	+ (preferences.doConnectToFrameApplicationServer () ? getBlockString (): "")
	+ " { " + getChannelRateString (chSet, !preferences.fullAcquisitionModeSelected ()) + " };";
    }

    if (_debug > 6)
      System.err.println ("DaqdNet 9.0: about to send `" + command + "'");

    sout.println (command);

    if (_debug > 6)
      System.err.println ("DaqdNet 9.0: command sent");

    try {
      // Read response, which is four bytes -- four hex digits
      int resp = readResp (is);

      if (resp == 0) {
	if (_debug > 6)
	  System.err.println ("reader is active");
      } else {
	if (infoLabel != null)
	  infoLabel.setText ("Got response `" + resp + "' from the server to `" + command + "'");
	if (startNetWriterDisco) {
	  startNetWriterDisco = false;
	  disconnect ();
	}
	return false;
      }

      // Read net-writer ID
      int net_writer_id = readResp (is) << 16 | readResp (is);
      if (_debug > 6)
	System.err.println ("net-writer ID is `" + net_writer_id + "'");

      int offline = is.readInt ();
      if (_debug > 6) {
	if (offline == 0)
	  System.err.println ("receiving data blocks online");
	else
	  System.err.println ("receiving data blocks off-line");
      }
    } catch (IOException e) {
      System.err.println ("While getting server response:");
      System.err.println (e);
      if (startNetWriterDisco) {
	startNetWriterDisco = false;
	disconnect ();
      }
      return false;
    }

    return true;
  }

  /**
   * Kill net-writer
   */
  public boolean stopNetWriter () {
    //        if (infoLabel != null)
    //    infoLabel.setText ("net-writer stopped");
    if (startNetWriterDisco && clientSocket != null) {
      startNetWriterDisco = false;
      if (!disconnect ())
	return false;
    }
    return true;
  }

  /*
  public DataBlock getDataBlock () {
    double [] wave = {1, 1.5, 2, 3, 4, 5, 6, 7, 8, 9};
    sleep (500);
    return new DataBlock (wave, blockCount++);
  }
  */

  /**
   * Input next communication data block from the server connection
   */
  public DataBlock getDataBlock () {
    return new DataBlock8_1 (is);
  }

  void sleep (int millis) {
    try {
      Thread.sleep (500);
    } catch (Exception e) {
      ;
    }
  }

  /**
   * Read a byte string of a certain length 
   */
  protected String readString (DataInputStream is, int len) throws IOException {
    byte[] respb = new byte [len];
    is.readFully (respb);
    return new String (respb);
  }

  /**
   * Read server response to a command
   */
  protected int readResp (DataInputStream is) throws IOException {
    byte[] respb = new byte [4];
    is.readFully (respb);
    String num = new String (respb);

    try {
      return Integer.parseInt (num, 16);
    } catch (NumberFormatException e) {
      System.err.println("DaqdNet9_0: can't parse `" + num + "' as integer");
      throw e;
    }
  }

  /**
   * Construct a string of channel-rate pairs for the channel set
   */
  private String getChannelRateString (ChannelSet chSet, boolean trend) {
    String rs = "";
    for (Enumeration e = chSet.elements (); e.hasMoreElements ();) {
      Channel ch = (Channel) e.nextElement ();
      rs += " \"" + ch.getName () + (trend? ".mean": "") + "\" " + 
	  (trend ? (" \"" + ch.getName () + ".min" + "\" " + 
		    " \"" + ch.getName () + ".max" + "\" " 
		    ): "") +
	(!trend && ch.getRate () != ((Channel) chSet.getServerSet ().channelObject (ch.getName ())).getRate ()?
	 Integer.toString (ch.getRate ()): "");

    }
    return rs;
  }

  public int getVersion () { return version; }
  public int getRevision () { return revision; }

} 
