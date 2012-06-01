package org.csstudio.utility.pv.nds;

import java.io.*;
import java.awt.*;
import java.util.*;

/**
 * Creates new file per input block
 */
public class FileSaveFormatBlock extends FileSaveFormat implements Debug {
  PrintWriter printWriter = null;
  DataOutputStream dataOutputStream = null;
  String fileName = null;

  int accumTime = 0;

  public FileSaveFormatBlock (Preferences preferences, Label fileLabel) {
    super (preferences, fileLabel);
  }

  public FileSaveFormatBlock (Preferences preferences, Label fileLabel, OutputStream outputStream) {
    super (preferences, fileLabel, outputStream);
  }

  public boolean saveBinaryWithHeaderFile (DataBlock block) {
    return saveBinaryFile (block);
  }

  public boolean saveBinaryFile (DataBlock block) {
    byte [] wave = block.getWave ();

    try {
      fileName = getFileName (block.getTimestamp ());
      dataOutputStream = createDataOutputStream (fileName);

      if (preferences.isServlet ()) {
	dataOutputStream.writeBytes ("Content-Type: application/octet-stream; name=\""+fileName+"\"\n");
	dataOutputStream.writeBytes ("Content-Disposition: attachment; filename=\""+fileName+"\"\n\n");
	System.err.println ("writing content-type header for nds-data");
      }
      dataOutputStream.write (wave, 0, wave.length);

      if (preferences.isServlet ()) {
	dataOutputStream.writeBytes ("\n-----\n");
	dataOutputStream.flush ();
	System.err.println ("writing finishing line for nds-data");
      }

      closeDataOutputStream (dataOutputStream, fileName);
      dataOutputStream = null;
    } catch (IOException e) {
      closeDataOutputStreamError (dataOutputStream, e, fileName);
      return false;
    }
    return true;
  }

  public boolean saveTextFile (DataBlock block) {
    byte [] wave = block.getWave ();

    try {
      fileName = getFileName (block.getTimestamp ());
      printWriter = createPrintWriter (fileName);
      String separator = preferences.csvFormatSelected ()? ",": "\t";
      ChannelSet channelSet = preferences.getChannelSet ();

      DataInputStream in = new DataInputStream (new ByteArrayInputStream (wave));

      // Print one channel per iteration
      for (Enumeration e = channelSet.elements (); e.hasMoreElements ();) {
	Channel ch = (Channel) e.nextElement ();
	int rate = ch.getRate ();
	int type = ch.getDataType ();
	
	printWriter.print (ch.getName () + separator + Integer.toString (block.getTimestamp ()));
	DataType.csprint (printWriter, type, in, rate, separator);
	printWriter.println ();
      }
      closePrintWriter (printWriter, fileName);
      printWriter = null;

      if (_debug > 8)
	System.err.println ("FileSaveFormatBlock.saveTextFile() saved one block");

    } catch (IOException e) {
      closePrintWriterError (printWriter, e, fileName);
      printWriter = null;
      if (_debug > 8)
	System.err.println ("FileSaveFormatBlock.saveTextFile() --> IOException");
      return false;
    }
    return true;
  }

  public void startSavingSession () {
    super.startSavingSession ();
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
