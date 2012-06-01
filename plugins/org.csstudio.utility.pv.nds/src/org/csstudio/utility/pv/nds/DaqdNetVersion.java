package org.csstudio.utility.pv.nds;

import java.awt.*;
import java.net.*;
import java.io.*;

/**
 * Get DaqdNet protocol version and revision
 */
public class DaqdNetVersion extends Net implements Debug {
  public DaqdNetVersion (Preferences pref) {
    super (pref);
  }

  /**
   * Get DAQD net protocol version and revision string in a format like "8.1"
   */
  public String getVersionString () {
    String versionString = null;
    boolean disco = false;

    if (clientSocket == null) {
      if (!connect ()) {
	return null;
      }
      disco = true;
    }

    try {
      // Request protocol version from the server
      String command = "version;";
      if (_debug > 6)
	System.err.println("DaqdNet: about to send `" + command + "'");

      sout.println (command);
      if (_debug > 6)
	System.err.println("DaqdNet: command was sent");
      
      // Read response, which is eight bytes -- eight hex digits
      long version = readResp (is);

      // Request protocol revision from the server
      command = "revision;";
      if (_debug > 6)
	System.err.println("DaqdNet: about to send `" + command + "'");

      sout.println (command);
      if (_debug > 6)
	System.err.println("DaqdNet: command was sent");
      
      // Read response, which is eight bytes -- eight hex digits
      long revision = readResp (is);

      versionString = version + "_" + revision;

    } catch (IOException e) {
      versionString = null;
    }


    if (disco) {
      disconnect ();
    }
    
    return versionString;
  }

  /**
   * Read eight bytes from the server, representing eight hexadecimal digits
   */
  private int readResp (DataInputStream is) throws IOException {
    byte[] respb = new byte [8];
    is.readFully (respb);
    String num = new String (respb);

    try {
      return Integer.parseInt (num, 16);
    } catch (NumberFormatException e) {
      System.err.println("DaqdNetVersion: can't parse " + num + " as integer");
      throw e;
    }
  }

  public void setLabel (Label l) {}
  public Label getLabel () { return null; }
  public ChannelSet getChannels () { return null; }
  public GroupSet getGroups () { return null; }
  public boolean startNetWriter (ChannelSet a, long b, int c) { return false; }
  public boolean stopNetWriter () { return false; }
  public DataBlock getDataBlock () { return null; }
  public int getVersion () { return 0; }
  public int getRevision () { return 0; }
}
