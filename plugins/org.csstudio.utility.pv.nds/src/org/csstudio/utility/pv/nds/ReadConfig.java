package org.csstudio.utility.pv.nds;

import java.io.*;
import java.util.*;
import java.awt.*;

/**
 * Read and parse config file
 */
public class ReadConfig  {
  BufferedReader in;
  Preferences preferences;
  public ReadConfig (BufferedReader rd, Preferences preferences) {
    in = rd;
    this.preferences = preferences;
  }

  /**
   * Read and parse config file
   */
  public boolean readFile () throws IOException {
    int numChannels = 0;

    for(;;) {  // infinite loop
      String line = in.readLine ();      // Get next config file line
      if (line == null)
	break;                           // Quit if we get EOF.
      try {
	// Use a StringTokenizer to parse the user's command
	StringTokenizer t = new StringTokenizer (line);
	if (!t.hasMoreTokens ())
	  continue;                      // if input was blank line

	// Get the first word of the input and convert to lower case
	String command = t.nextToken ().toLowerCase();
	// Now compare it to each of the possible commands, doing the
	// appropriate thing for each command
	//	System.err.println ("Command: " + command);
	if (command.equals ("channel")) {       // Channel command
	  String p = t.nextToken ();            // Get the next word of input
	  try {
	    if (preferences.getChannelSet ().addChannel (p, Integer.parseInt (t.nextToken ())))  // Add to the seleted channel set and display list
	      numChannels++;    
	  } catch (Exception e) { // no or invalid rate token
	    if (preferences.getChannelSet ().addChannelDefaultRate (p))  // Add to the seleted channel set and display list
	      numChannels++;    
	  }
	} else if (command.equals ("format")) { // Format command
	  String p = t.nextToken ();            // Get the next word of input

	  if (!preferences.setExportFormat (p)) {
	    System.err.println ("Unknown export format: " + p);
	  }
	}
      } catch (Exception e) {
	// If an exception occurred during the command, print an error
	// message, then output details of the exception.
	System.err.println ("While reading and processing config file:");
	System.err.println (e);
	return false;    
      }
    }
    System.err.println ("Configured " + numChannels + " channel"
			+ (Math.abs (numChannels) != 1 ? "s": ""));
    return true;
  }
}
