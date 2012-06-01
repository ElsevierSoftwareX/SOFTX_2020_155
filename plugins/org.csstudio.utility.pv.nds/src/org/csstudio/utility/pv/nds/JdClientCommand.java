package org.csstudio.utility.pv.nds;

import java.io.*;
import java.util.*;

/**
 * Command line interface to JdClient
 */

public class JdClientCommand implements Debug, Defaults {
  private boolean verbose = true;
  private Preferences preferences;
  private boolean stopRecursion = false;

  public final int beef  = 0xbeef;

  public JdClientCommand () {
	  this(new String[] {"-verbose", "-channels", "X1:ATS-MASTER_TX1_ADC_FILTER_10_OUT_DQ", "16384"});	  
  }
  
  public JdClientCommand (String args[]) {
    // Try to get preferences from the resource file
//    try {
//      FileInputStream istream = new FileInputStream (Preferences.fileName ());
//      ObjectInputStream p = new ObjectInputStream (istream);
//      preferences = (org.csstudio.opibuilder.alex.Preferences) p.readObject ();
//      istream.close ();
//    } catch (Exception e) {
//      if (_debug > 8)
//	e.printStackTrace ();
//      preferences = new Preferences ();
//      verbose ("Unable to open resource file");
//    }

	preferences = new Preferences ();
    ConnectionParams connectionParams = new ConnectionParams (defaultPort);
    // If connection params are default after the init, set params from preferences
    preferences.setConnectionParams (connectionParams, connectionParams.defaultConfig ());

    DaqdNetVersion daqdNetVersion = new DaqdNetVersion (preferences);

    String daqdNetVersionString = daqdNetVersion.getVersionString ();
    Net net = null;

    if (daqdNetVersionString == null) {
      fatal ("Unable to determine DaqdNet protocol version");
    } else {
      verbose ("DaqdNet protocol version " + daqdNetVersionString);

      try {
	net = (Net) Class.forName ("org.csstudio.opibuilder.ligoNdsClient.DaqdNet" + daqdNetVersionString).newInstance ();
      } catch (ClassNotFoundException  e) {
	fatal ("Communication protocol version or revision is not supported by this program");
      } catch (Exception e) {
	fatal ("Unable to load DaqdNet" + daqdNetVersionString + e);
      }
    }

    net.setPreferences (preferences);
    preferences.setNet (net);
    // Load channel from the server
    preferences.updateChannelSet (new ChannelSet (null, net));

    parseCommandLine (args);
  }

  public int process () {
    // Create and start data acquisition
    DataAcquisition daq = new DataAcquisition (preferences);
    try {
      daq.start ();
      daq.join ();
    } catch (Exception e) {
      verbose ("In process():" + e);
      return 1;
    }
    return 0;
  }

  private static String iterateKnownExportFormats () {
    String str = "(";
    for (int i = 0; i < Preferences.knownExportFormats.length; i++)
      str += (i > 0? " | ": "") + Preferences.knownExportFormats [i];
    return str + ")";
  }

  private static void usage () {
    System.out.println ("JdClientCommand "
			+ Integer.toString (VERSION) + "." + Integer.toString (REVISION) + "\n"
			+ "http://www.ligo.caltech.edu/~aivanov/jdclient/index.html" + "\n"
			+ "---\n"
			+ "JVM: " + System.getProperty ("java.version")
			+ " by " + System.getProperty ("java.vendor") + "\n"
			+ "Operating System: " + System.getProperty ("os.name")
			+ " version " + System.getProperty ("os.version") + "\n"
			+ "Architecture: " + System.getProperty ("os.arch"));

    System.out.println ("Parameters supported:");
    System.out.println ("\t-verbose");
    System.out.println ("\t-online [Period] -offline [[Start] Period]");
    System.out.println ("\t-fileDir FilesystemPath");
    System.out.println ("\t-fileMode (Single | (fileForPeriod [Period])\n\t\t| filePerChannel | fileForPeriodPerChannel [Period])");
    System.out.println ("\t-format " + iterateKnownExportFormats ());
    System.out.println ("\t-channels (Name [Rate])+  // This must be the last switch");
    System.out.println ("\t-parameters FileName");
    //    System.out.println ("\t-server HostName[:PortName]");
  }
  

  private void fatal (String message) {
    System.err.println ((new Date()).toString () + ": " +  message);
    System.exit (1);
  }

  private void verbose (String message) {
    if (verbose)
      System.out.println ((new Date()).toString () + ": " +  message);      
  }

  private void parseCommandLine (String args []) {
    try {
      for (int i = 0; i < args.length; i++) {
	if (args [i].length () < 1)
	  continue;
	else if (args [i].equalsIgnoreCase ("-verbose"))
	  verbose = true;
	else if (args [i].equalsIgnoreCase ("-filedir"))
	  preferences.setFileDirectory (args [++i]);
	else if (args [i].equalsIgnoreCase ("-online")) {
	  int period = 0;
	  preferences.setOnline (true);
	  try {
	    if (i < args.length-1) {
	      if ((period = Integer.parseInt (args [i + 1])) < 0)
		throw new Exception ();
	      ++i;
	    }
	  } catch (NumberFormatException e) {
	    ;
	  }
	  if (period == 0)
	    preferences.setContinuous (true);
	  else
	    preferences.setPeriod (period);
	} else if (args [i].equalsIgnoreCase ("-offline")) {
	  preferences.setOnline (false);
	  int firstNumber = 0;
	  int secondNumber = 0;
	  try {
	    if (i < args.length-1) {
	      if ((firstNumber = Integer.parseInt (args [i + 1])) < 0)
		throw new Exception ();
	      ++i;
	    }
	  } catch (NumberFormatException e) {
	    ;
	  }

	  try {
	    if (i < args.length-1) {
	      if ((secondNumber = Integer.parseInt (args [i + 1])) < 0)
		throw new Exception ();
	      ++i;
	    }
	  } catch (NumberFormatException e) {
	    ;
	  }

	  if (secondNumber == 0) {
	    preferences.setPeriod (firstNumber);
	    preferences.setGpsTime (0);
	  } else {
	    preferences.setGpsTime (firstNumber);
	    preferences.setPeriod (secondNumber);
	  }
	} else if (args [i].equalsIgnoreCase ("-filemode")) {
	  ++i;
	  if (args [i].equalsIgnoreCase ("single")) {
	    preferences.setFilePerChannelFlag (false);
	    preferences.setFilePerBlockFlag (false);
	  } else if (args [i].equalsIgnoreCase ("fileperchannel")) {
	    preferences.setFilePerChannelFlag (true);
	    preferences.setFilePerBlockFlag (false);
	  } else if (args [i].equalsIgnoreCase ("fileforperiod")) {
	    int period = 0;
	    // next parameter, if present, should be numerical
	    try {
	      if (i < args.length-1) {
		if ((period = Integer.parseInt (args [i + 1])) < 0)
		  throw new Exception ();
		++i;
	      }
	    } catch (NumberFormatException e) {
	      ;
	    }
	    if (period != 0)
	      preferences.setTimePerFile (period);
	    preferences.setFilePerChannelFlag (false);
	    preferences.setFilePerBlockFlag (true);
	  } else if (args [i].equalsIgnoreCase ("fileforperiodperchannel")) {
	    int period = 0;
	    // next parameter, if there, should be numerical
	    try {
	      if (i < args.length-1) {
		if ((period = Integer.parseInt (args [i + 1])) < 0)
		  throw new Exception ();
		++i;
	      }
	    } catch (NumberFormatException e) {
	      ;
	    }
	    if (period != 0)
	      preferences.setTimePerFile (period);
	    preferences.setFilePerChannelFlag (true);
	    preferences.setFilePerBlockFlag (true);
	  } else
	    throw new Exception ();
	} else if (args [i].equalsIgnoreCase ("-format")) {
	  if (! preferences.setExportFormat (args [++i]))
	    throw new Exception ();
	} else if (args [i].equalsIgnoreCase ("-channels")) {
	  /* This one should be the last switch on the command line */

	  // First, clear the channel list
	  preferences.getChannelSet ().removeAllChannels ();

	  for (i++; i < args.length; i++) {
	    String channel = args [i];
	    if (i < args.length-1) {
	      // See if the next argument is numerical
	      try {
		int rate = 0;
		if ((rate = Integer.parseInt (args [++i])) < 0)
		  throw new Exception ();
		preferences.getChannelSet ().addChannel (channel, rate);
		continue;
	      } catch (NumberFormatException e) {
		// Couldn't parse the number -- must be the name of next channel -- continue
		;
	      }
	    }
	    preferences.getChannelSet ().addChannelDefaultRate (channel);
	  }
	} else if (args [i].equalsIgnoreCase ("-parameters")) {
	  if (stopRecursion) {
	    verbose ("recursive `-parameters' is not allowed");
	    throw new Exception ();
	  }
	  String params = "";
	  // open a file
	  FileReader istream = new FileReader (args [++i]);
	  BufferedReader breader = new BufferedReader (istream);
	  for (String tmp; (tmp = breader.readLine ()) != null; params += " " + tmp);
	  istream.close ();
	  StringSplitter stringSplitter = new StringSplitter (params, "whitespace");
	  stopRecursion = true;
	  String [] stringArray = new String [stringSplitter.size ()];
	  stringSplitter.copyInto (stringArray);
	  if (verbose) {
	    System.out.println ("Tokenized `" + args [i] + "' looks as follows:");
	    for (int j = 0; j < stringArray.length; j++)
	      System.out.println (stringArray [j]);
	  }
	  parseCommandLine (stringArray);
	} else
	  throw new Exception ();
      }      
    } catch (Exception e) {
      if (verbose)
	e.printStackTrace ();
      usage ();
      System.exit (1);
    }
  }

  public static void main (String args []) {
    for (int i = 0; i < args.length; i++) {
      if (args [i].equalsIgnoreCase ("-?")
	  || args [i].equalsIgnoreCase ("h")
	  || args [i].equalsIgnoreCase ("-h")
	  || args [i].equalsIgnoreCase ("-help")
	  || args [i].equalsIgnoreCase ("--help")) {
	JdClientCommand.usage ();
	System.exit (0);
      }
    }

    JdClientCommand jdClientCommand = new JdClientCommand (args);
    System.exit (jdClientCommand.process ());
  }
}
