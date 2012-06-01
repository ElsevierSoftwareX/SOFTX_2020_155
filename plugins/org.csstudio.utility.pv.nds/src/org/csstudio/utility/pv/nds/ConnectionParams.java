package org.csstudio.utility.pv.nds;

import java.io.*;

/**
 * Parameters of the TCP/IP connection to the DAQD server
 */
public class ConnectionParams implements DataTypeConstants, Debug, Serializable{
  String host;
  int port;
  transient boolean defaultConfig;

  public ConnectionParams (String port){
//	  if (System.getProperty ("JDPORT") != null) {
//		  port = System.getProperty ("JDPORT");
//		  defaultConfig = false;
//	  } else
//		  defaultConfig = true;

	  String portString = port;

	  if (port.indexOf (":") > -1) {
		  this.host = port.substring (0, port.indexOf (":"));
		  try {
			  this.port = Integer.parseInt (port.substring (port.indexOf (":") + 1));
		  } catch (NumberFormatException e) {
			  System.err.println ("Using DAQD server default TCP port " + DEFAULT_DAQD_PORT);
			  this.port = DEFAULT_DAQD_PORT;
		  }
		  //System.out.println("Setting preferences host="+this.host+" port="+this.port);
	  } else {
		  this.host = port;
		  this.port = DEFAULT_DAQD_PORT;
	  }
  }

  public void setHost (String host) { this.host = host; }
  public String getHost () { return host; }
  public void setPort (int port) { this.port = port; }
  public int getPort () { return port; }
  public String getPortString () {
    return host + ":" + Integer.toString (port);
  }

  /**
   * See if it was not initialized from the system property JDPORT
   */
  public boolean defaultConfig () {
    return defaultConfig;
  }

}

