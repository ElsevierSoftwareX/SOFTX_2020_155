package org.csstudio.utility.pv.nds;

import java.io.*;
import java.awt.*;
import java.util.*;

/**
 * Separate files are used for each data channel acquired
 */
public class FileSaveFormatChannel extends FileSaveFormat implements Debug {
  String fileName [] = null;
  PrintWriter printWriter [] = null;
  DataOutputStream dataOutputStream [] = null;

  public FileSaveFormatChannel (Preferences preferences, Label fileLabel) {
    super (preferences, fileLabel);
  }
  
  public boolean saveBinaryWithHeaderFile (DataBlock block) {
    return saveBinaryFile (block);
  }


  public boolean saveBinaryFile (DataBlock block) {
    byte [] wave = block.getWave ();
    int errIndex = 0;

    try {
      ChannelSet channelSet = preferences.getChannelSet ();
      if (dataOutputStream == null) {
	fileName = new String [channelSet.size ()];
	dataOutputStream = new DataOutputStream [channelSet.size ()];
	errIndex = 0;
	for (Enumeration e = channelSet.elements (); e.hasMoreElements (); errIndex++) {
	  fileName [errIndex] = getFileName (block.getTimestamp (), ((Channel) e.nextElement ()).getName ());
	  dataOutputStream [errIndex] = createDataOutputStream (fileName [errIndex]);
	}
      }

      errIndex = 0;
      int idx = 0;
      for (Enumeration e = channelSet.elements (); e.hasMoreElements (); errIndex++) {
	Channel ch = (Channel) e.nextElement ();
	int length = ch.getRate () * ch.getBps ();
	dataOutputStream [errIndex].write (wave, idx, length);
	idx += length;
      }

    } catch (IOException e) {
      closeAllDataOutputStreamsError (dataOutputStream, e, fileName, errIndex);
      return false;
    }
    return true;
  }

  public boolean saveTextFile (DataBlock block) {
    byte [] wave = block.getWave ();
    int errIndex = 0;

    try {
      String separator = preferences.csvFormatSelected ()? ",": "\t";
      ChannelSet channelSet = preferences.getChannelSet ();

      if (printWriter == null) {
	fileName = new String [channelSet.size ()];
	printWriter = new PrintWriter [channelSet.size ()];
	errIndex = 0;
	for (Enumeration e = channelSet.elements (); e.hasMoreElements (); errIndex++) {
	  fileName [errIndex] = getFileName (block.getTimestamp (), ((Channel) e.nextElement ()).getName ());
	  printWriter [errIndex] = createPrintWriter (fileName [errIndex]);
	}
      }

      DataInputStream in = new DataInputStream (new ByteArrayInputStream (wave));

      // Print one channel per iteration
      errIndex = 0;
      for (Enumeration e = channelSet.elements (); e.hasMoreElements (); errIndex++) {
	Channel ch = (Channel) e.nextElement ();
	int rate = ch.getRate ();
	int type = ch.getDataType ();
	
	printWriter [errIndex].print (ch.getName () + separator + Integer.toString (block.getTimestamp ()));
	DataType.csprint (printWriter [errIndex], type, in, rate, separator);
	printWriter [errIndex].println ();

	if (printWriter [errIndex].checkError ())
	  throw new IOException ("printWriter error");
	
	if (_debug > 8)
	  System.err.println ("FileSaveFormatChannel.saveTextFile() saved one channel");

      }
    } catch (IOException e) {
      closeAllPrintWritersError (printWriter, e, fileName, errIndex);
      if (_debug > 8)
	System.err.println ("FileSaveFormatChannel.saveTextFile() --> IOException");
      return false;
    }
    return true;
  }

  void closeAllPrintWriters (PrintWriter [] printWriter, String [] fileName) {
    for (int i = 0; i < printWriter.length; i++) {
      if (printWriter [i] != null) { 
	try {
	  closePrintWriter (printWriter [i], fileName [i]);
	} catch (IOException e) {
	  closePrintWriterError (printWriter [i], e, fileName [i]);
	}
	printWriter [i] = null;
      }
    }
  }

  void closeAllPrintWritersError (PrintWriter [] printWriter, IOException e, String [] fileName, int errIndex) {
    int messagesPrinted = 0;
    for (int i = 0; i < printWriter.length; i++) {
      if (errIndex == i)
	if (printWriter [i] != null) {
	  closePrintWriterError (printWriter [i], e, fileName [i]);
	  messagesPrinted++;
	}
      else {
	if (printWriter [i] != null) { 
	  try {
	    closePrintWriter (printWriter [i], fileName [i]);
	  } catch (IOException ee) {
	    closePrintWriterError (printWriter [i], ee, fileName [i]);
	  }
	  printWriter [i] = null;
	  messagesPrinted++;
	}
      }
    }
    if (messagesPrinted == 0) {
      System.err.println (e);
    }
  }


  void closeAllDataOutputStreams (DataOutputStream [] dataOutputStream, String [] fileName) {
    for (int i = 0; i < dataOutputStream.length; i++) {
      if (dataOutputStream [i] != null) {
	try {
	  closeDataOutputStream (dataOutputStream [i], fileName [i]);
	} catch (IOException e) {
	  closeDataOutputStreamError (dataOutputStream [i], e, fileName [i]);
	}
	dataOutputStream [i] = null;
      }
    }
  }

  void closeAllDataOutputStreamsError (DataOutputStream [] dataOutputStream, IOException e, String [] fileName, int errIndex) {
    int messagesPrinted = 0;

    for (int i = 0; i < dataOutputStream.length; i++) {
      if (errIndex == i)
	if (dataOutputStream [i] != null) {
	  closeDataOutputStreamError (dataOutputStream [i], e, fileName [i]);
	  messagesPrinted++;
	}
      else {
	if (dataOutputStream [i] != null) {
	  try {
	    closeDataOutputStream (dataOutputStream [i], fileName [i]);
	  } catch (IOException ee) {
	    closeDataOutputStreamError (dataOutputStream [i], ee, fileName [i]);
	  }
	  dataOutputStream [i] = null;
	  messagesPrinted++;
	}
      }
    }
    if (messagesPrinted == 0) {
      System.err.println (e);
    }
  }

  public void startSavingSession () {
    super.startSavingSession ();
  }

  public void endSavingSession () {
    if (printWriter != null) {
      closeAllPrintWriters (printWriter, fileName);
    }
    if (dataOutputStream != null) {
      closeAllDataOutputStreams (dataOutputStream, fileName);
    }
    printWriter = null;
    fileName = null;
    dataOutputStream = null;
    super.endSavingSession ();
  }

}
