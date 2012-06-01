package org.csstudio.utility.pv.nds;

import java.awt.*;
import java.net.*;
import java.io.*;
import java.util.*;

/**
 * DAQD network connection and communication protocol
 */
public class DaqdNet11_0 extends DaqdNet9_0 {
  static final int version = 11;
  static final int revision = 0;

  public DaqdNet11_0 () {
      super ();
  }

  public DaqdNet11_0 (Label infoLabel, Preferences pref) {
    super (infoLabel, pref);
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

	// calibration values
	readResp(is);readResp(is);
	readResp(is);readResp(is);
	readResp(is);readResp(is);
	readString (is, MAX_CHANNEL_NAME_LENGTH).trim (); // Get units

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
} 
