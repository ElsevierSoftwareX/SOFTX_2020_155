package org.csstudio.utility.pv.nds;

import java.awt.*;
import java.net.*;
import java.io.*;
import java.util.*;

/**
 * DAQD network connection and communication protocol
 */
public class DaqdNet12_0 extends DaqdNet9_0 {
  static final int version = 12;
  static final int revision = 0;

  public DaqdNet12_0 () {
      super ();
  }

  public DaqdNet12_0 (Label infoLabel, Preferences pref) {
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
      BufferedReader d = new BufferedReader(new InputStreamReader(clientSocket.getInputStream ()));
            
      // Request channel names from the server
      String command = "status channels 3;";
      if (_debug > 6)
	System.err.println("DaqdNet 12.0: about to send `" + command + "'");

      sout.println (command);
      if (_debug > 6)
	System.err.println("DaqdNet 12.0: command was sent");
      
      // Read response
      String s = d.readLine();

      if (_debug > 6)
      	System.err.println( "Received response " + s);
      int chnum =  Integer.parseInt (s, 10);

      if (chnum == 0) {
	String msg = "Got response `" + chnum + "' from the server on `" + command + "'";
	if (infoLabel != null)
	  infoLabel.setText(msg);
	throw new IOException (msg);
      }

      if (_debug > 6)
	System.err.println ("Receiving " + chnum + " channels");

      for (int i = 0; i < chnum; i++) {
/*
X1:ATS-MASTER_TX1_ADC_FILTER_1_IN2
65536
4
10191
0
none
1
1
0
*/
/*
                    if (schan) {
                      *yyout << ((my_lexer *) lexer) -> channels [i].name << endl;
                    } else {
                      *yyout << c [i].name << endl;
                    }
                    *yyout << c [i].sample_rate << endl;
                    *yyout << c [i].data_type << endl;
#ifdef GDS_TESTPOINTS
                    if (IS_GDS_ALIAS(c [i])) {
                        *yyout << c [i].chNum << endl;
                    } else
#endif
                        *yyout << 0 << endl;
                    *yyout << c [i].group_num << endl;
                    *yyout << c [i].signal_units << endl;
                    *yyout << c [i].signal_gain << endl;
                    *yyout << c [i].signal_slope << endl;
                    *yyout << c [i].signal_offset << endl;
*/
	String chName = d.readLine();
	int chRate = Integer.parseInt (d.readLine(), 10);
	int dataType = Integer.parseInt (d.readLine(), 10);
	int testPointNum = Integer.parseInt (d.readLine(), 10);
	boolean chTrend = false; // ???
	int chGrp = Integer.parseInt (d.readLine(), 10);
	String units = d.readLine();
	String gain = d.readLine();
	String slope = d.readLine();
	String offset = d.readLine();
	int bps = 4; // ???

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
