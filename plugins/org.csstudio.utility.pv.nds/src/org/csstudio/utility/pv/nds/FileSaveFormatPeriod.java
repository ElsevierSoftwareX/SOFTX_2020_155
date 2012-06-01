package org.csstudio.utility.pv.nds;

import java.io.*;
import java.awt.*;
import java.util.*;

/**
 * Creates new file per preferences period
 */
public class FileSaveFormatPeriod extends FileSaveFormat implements Debug {
  PrintWriter printWriter = null;
  boolean createNewPrintWriter = true;
  DataOutputStream dataOutputStream = null;
  boolean createNewDataOutputStream = true;
  String fileName = null;

  int accumTime = 0;

  public FileSaveFormatPeriod (Preferences preferences, Label fileLabel) {
    super (preferences, fileLabel);
  }

  public boolean saveBinaryWithHeaderFile (DataBlock block) {
    return saveBinaryFile (block);
  }
  
  public boolean saveBinaryFile (DataBlock block) {
    byte [] wave = block.getWave ();

    try {
      if (createNewDataOutputStream) {
	fileName = getFileName (block.getTimestamp ());
	dataOutputStream = createDataOutputStream (fileName);
	createNewDataOutputStream = false;
      }

      dataOutputStream.write (wave, 0, wave.length);

      accumTime += block.getPeriod ();
      
      if (accumTime >= preferences.getTimePerFile ()) {
	accumTime = 0;
	createNewDataOutputStream = true;
	closeDataOutputStream (dataOutputStream, fileName);
	dataOutputStream = null;
      }
    } catch (IOException e) {
      closeDataOutputStreamError (dataOutputStream, e, fileName);
      dataOutputStream = null;
      return false;
    }
    return true;
  }

  public boolean saveTextFile (DataBlock block) {
    byte [] wave = block.getWave ();
    boolean fullDaqMode = preferences.fullAcquisitionModeSelected ();

    try {
      if (createNewPrintWriter) {
	fileName = getFileName (block.getTimestamp ());
	printWriter = createPrintWriter (fileName);
	createNewPrintWriter = false;
      }

      String separator = preferences.csvFormatSelected ()? ",": "\t";
      ChannelSet channelSet = preferences.getChannelSet ();

      DataInputStream in = new DataInputStream (new ByteArrayInputStream (wave));

      if (fullDaqMode) {
	// Print one channel per iteration
	for (Enumeration e = channelSet.elements (); e.hasMoreElements ();) {
	  Channel ch = (Channel) e.nextElement ();
	  int rate = ch.getRate ();
	  int type = ch.getDataType ();
	
	  printWriter.print (ch.getName () + separator + Integer.toString (block.getTimestamp ()));
	  DataType.csprint (printWriter, type, in, rate, separator);
	  printWriter.println ();
	}
      } else {
	int period = block.getPeriod ();

	if (period == 1) {
	  for (Enumeration e = channelSet.elements (); e.hasMoreElements ();) {
	    Channel ch = (Channel) e.nextElement ();
	    int minmax_type = ch.isInteger ()? DataType._32bit_integer: ch.getDataType ();
	    int [] types = {minmax_type, minmax_type, DataType._64bit_double};
	    
	    for (int i = 0; i < 3; i++) { // foreach trend suffix
	      printWriter.print (ch.getName () + DataType.trendSuffixes [i] + separator
				 + Integer.toString (block.getTimestamp ()));
	      DataType.csprint (printWriter, types [i], in, 1, separator);
	      printWriter.println ();
	    }
	  }
	} else {
	    // Trend, block period is longer than one second

	    int ndata = period;
	    int value_span = 1; // in seconds

	    for (Enumeration e = channelSet.elements (); e.hasMoreElements ();) {
		Channel ch = (Channel) e.nextElement ();
		int minmax_type = ch.isInteger ()? DataType._32bit_integer: ch.getDataType ();
		int [] types = {minmax_type, minmax_type, DataType._64bit_double};
		int rate = ch.getRate ();

		for (int i = 0; i < 3; i++) { // foreach trend suffix
		    printWriter.print (ch.getName () + DataType.trendSuffixes [i] + separator
				       + Integer.toString (block.getTimestamp ()));
		    DataType.csprint (printWriter, types [i], in, rate, separator);
		    printWriter.println ();
		}
	    }
	}
      }
      
      if (printWriter.checkError ())
	throw new IOException ("printWriter error");

      if (_debug > 8)
	System.err.println ("FileSaveFormatPeriod.saveTextFile() saved one block");

      if (fullDaqMode) {
	accumTime += block.getPeriod ();
      } else {
	  ;
      }
      
      if (accumTime >= preferences.getTimePerFile ()) {
	accumTime = 0;
	createNewPrintWriter = true;
	closePrintWriter (printWriter, fileName);
	printWriter = null;
      }
    } catch (IOException e) {
      closePrintWriterError (printWriter, e, fileName);
      printWriter = null;
      if (_debug > 8)
	System.err.println ("FileSaveFormatPeriod.saveTextFile() --> IOException");
      return false;
    }
    return true;
  }

  public void startSavingSession () {
    super.startSavingSession ();
    createNewPrintWriter = true;
    createNewDataOutputStream = true;
    accumTime = 0;
  }

  public void endSavingSession () {
    if (printWriter != null) {
      try {
	closePrintWriter (printWriter, fileName);
      } catch (IOException e) {
	closePrintWriterError (printWriter, e, fileName);
      }
    } else if (dataOutputStream != null) {
      try {
	closeDataOutputStream (dataOutputStream, fileName);
      } catch (IOException e) {
	closeDataOutputStreamError (dataOutputStream, e, fileName);
      }
    }
    printWriter = null;
    dataOutputStream = null;
    super.endSavingSession ();
  }
}
