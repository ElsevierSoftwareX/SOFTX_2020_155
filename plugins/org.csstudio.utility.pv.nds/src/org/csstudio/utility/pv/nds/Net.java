package org.csstudio.utility.pv.nds;

import java.awt.*;
import java.net.*;
import java.io.*;
import java.util.*;


/**
 * Generic TCP/IP connection functionality and abstract DaqdNet access functions
 */
public abstract class Net implements Debug {
  Preferences preferences;
  Socket clientSocket;
  String host;
  int port;
  DataInputStream is;
  PrintWriter sout;

  public Net () {
    clientSocket = null;
  }

  public Net (Preferences preferences) {
    this.preferences = preferences;
    clientSocket = null;
    host = null;
    port = 0;
  }

  public String getSocketString () {
    if (host != null)
      return host + ":" + Integer.toString (port);
    else
      return "unknown:" + Integer.toString (port);
  }

  public void setPreferences (Preferences preferences) {
    this.preferences = preferences;
  }

  public boolean connect () {
    return connect (false);
  }

  /**
   * Open network socket and crate input/output data streams
   */
  public boolean connect (boolean considerApplicationServer) {
	  try {
		  if (considerApplicationServer && preferences.doConnectToFrameApplicationServer ()) {
			  host = preferences.getFramerHost ();
			  port = preferences.getFramerPort ();

		  } else {
			  host = preferences.getConnectionParams ().getHost ();
			  port = preferences.getConnectionParams ().getPort ();
		  }
		  clientSocket = new Socket (host, port);

		  is = new DataInputStream (clientSocket.getInputStream ());
		  sout = new PrintWriter (clientSocket.getOutputStream (), true);
	  } catch (Exception e) {
		  clientSocket = null;
		  is = null; sout = null;
		  System.err.println ("Could not connect to " + getSocketString ());
		  return false;
	  }
	  return true;
  }

  /**
   * Close communication socket
   */
  public boolean disconnect () {
    try {
    	if (clientSocket != null) {
    		clientSocket.close ();
    	}
    } catch (IOException e) {
      clientSocket = null;
      is = null; sout = null;
      System.err.println ("Could not disconnect from " + getSocketString ());
      return false;
    }
    clientSocket = null;
    is = null; sout = null;
    return true;
  }

  public boolean is_connected() {
	  	return clientSocket != null;
  }
  /**
   * Get DAQD net protocol version and revision string in a format like "8.1"
   */
  public boolean checkDaqdNetVersion () {
    if (clientSocket == null) {
      System.err.println ("Unable to verify DaqdNet protocol version");
      return false;
    }
    
    // FIXME: verify version and revision for this class and DataBlock class
    
    return true;
  }

  public abstract void setLabel (Label l);
  public abstract Label getLabel ();
  public abstract ChannelSet getChannels ();
  public abstract GroupSet getGroups ();
  public abstract boolean startNetWriter (ChannelSet c, long gps, int period);
  public abstract boolean stopNetWriter ();
  public abstract DataBlock getDataBlock ();
  public abstract int getVersion ();
  public abstract int getRevision ();
}
