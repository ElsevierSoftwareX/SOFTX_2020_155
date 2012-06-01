package org.csstudio.utility.pv.nds;

import java.io.*;
import java.awt.*;
import java.util.*;

/**
 * Save data into one file
 */
public class FileSaveFormatSingle extends FileSaveFormat implements Debug {
  PrintWriter printWriter = null;
  DataOutputStream dataOutputStream = null;
  String fileName = null;
  byte [] nullArray = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

  public FileSaveFormatSingle (Preferences preferences, Label fileLabel) {
    super (preferences, fileLabel);
  }

  public FileSaveFormatSingle (Preferences preferences, Label fileLabel, OutputStream outputStream) {
    super (preferences, fileLabel, outputStream);
  }

  
  void writeHeader (int waveLength, int period, int gps,
		    String name, int dataType, int ndata) throws IOException {
      //dataOutputStream.writeByte (name.length());
      dataOutputStream.writeBytes (name);
      dataOutputStream.write (nullArray, 0, 32-name.length());
      dataOutputStream.writeInt (gps); // timestamp of the first ADC sample
      dataOutputStream.writeInt (period); // second interval if the following ADC data
      dataOutputStream.writeInt (ndata); // number of ADC samples in the following data
      dataOutputStream.writeByte (dataType); // data type of the following ADC data
      dataOutputStream.writeInt (waveLength); // length of the following data
  }

  public boolean saveBinaryWithHeaderFile (DataBlock block) {
    byte [] wave = block.getWave ();
    boolean fullDaqMode = preferences.fullAcquisitionModeSelected ();

    try {
      if (dataOutputStream == null) {
        fileName = getFileName (block.getTimestamp ());
        dataOutputStream = createDataOutputStream (fileName);

        if (preferences.isServlet ()) {
          dataOutputStream.writeBytes ("Content-Type: application/octet-stream; name=\""+fileName+"\"\n");
          dataOutputStream.writeBytes ("Content-Disposition: attachment; filename=\""+fileName+"\"\n\n");
	  if (_debug > 8)
 		System.err.println ("writing content-type header for nds-data");
        }
      }

      ChannelSet channelSet = preferences.getChannelSet ();
      if (fullDaqMode) {
          int offset = 0;

	  // Print one channel per iteration
	  for (Enumeration e = channelSet.elements (); e.hasMoreElements ();) {
	      Channel ch = (Channel) e.nextElement ();
	      int slen = ch.getRate () * DataType.length(ch.getDataType ());

              writeHeader (slen, block.getPeriod(), block.getTimestamp(),
			   ch.getName(), ch.getDataType (), ch.getRate ());
              dataOutputStream.write (wave, offset, slen);
	      offset += slen;
	  }
          dataOutputStream.flush ();
      } else {
	int period = block.getPeriod ();
	int ndata = period;
	if (preferences.minuteTrendAcquisitionModeSelected ()) {
	  if (ndata < 60)
		ndata = 1;
	  else
		ndata /= 60;
	}
	
	// This is how the NDS sends the trend data: 
	// RMS is always sent as double;
	// MIN and MAX as 32 bit integer for integer signals or else as signal type

        int offset = 0;
	for (Enumeration e = channelSet.elements (); e.hasMoreElements ();) {
	  Channel ch = (Channel) e.nextElement ();
	  int minmax_type = ch.isInteger ()? DataType._32bit_integer: ch.getDataType ();
	  int [] types = {minmax_type, minmax_type, DataType._64bit_double};
	    
	  for (int i = 0; i < 3; i++) { // foreach trend suffix
	    int slen = ndata * DataType.length(types[i]);
            writeHeader (slen, period, block.getTimestamp(),
			 ch.getName() + DataType.trendSuffixes [i], types[i], ndata);
            dataOutputStream.write (wave, offset, slen);
            offset += slen;
	  }
	}
      }
    } catch (IOException e) {
      closeDataOutputStreamError (dataOutputStream, e, fileName);
      dataOutputStream = null;
      return false;
    }
    return true;
  }
  
  public boolean saveBinaryFile (DataBlock block) {
    byte [] wave = block.getWave ();


    try {
      if (dataOutputStream == null) {
	fileName = getFileName (block.getTimestamp ());
	dataOutputStream = createDataOutputStream (fileName);

	if (preferences.isServlet ()) {
	  dataOutputStream.writeBytes ("Content-Type: application/octet-stream; name=\""+fileName+"\"\n");
	  dataOutputStream.writeBytes ("Content-Disposition: attachment; filename=\""+fileName+"\"\n\n");
	  if (_debug > 8)
	  	System.err.println ("writing content-type header for nds-data");
	}
      }

      dataOutputStream.write (wave, 0, wave.length);
      dataOutputStream.flush ();
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
      if (printWriter == null) {
	fileName = getFileName (block.getTimestamp ());
	printWriter = createPrintWriter (fileName);

	if (preferences.isServlet ()) {
	  printWriter.print ("Content-Type: application/octet-stream; name=\""+fileName+"\"\n");
	  printWriter.print ("Content-Disposition: attachment; filename=\""+fileName+"\"\n\n");
	  if (_debug > 8)
	  	System.err.println ("writing content-type header for nds-data");
	}
      }

      String separator = preferences.csvFormatSelected ()? ",": "\t";
      ChannelSet channelSet = preferences.getChannelSet ();

      DataInputStream in = new DataInputStream (new ByteArrayInputStream (wave));

      if (fullDaqMode) {
	  // Print one channel per iteration
	  for (Enumeration e = channelSet.elements (); e.hasMoreElements ();) {
	      Channel ch = (Channel) e.nextElement ();

	      String name = ch.getName ()  + separator;
	      printWriter.print (name  + Integer.toString (block.getTimestamp ()));
	      DataType.csprint (printWriter, ch.getDataType (), in, ch.getRate (), separator);
	      printWriter.println ();
	  }
      } else {
	int period = block.getPeriod ();
	
	// This is how the NDS sends the trend data: 
	// RMS is always sent as double;
	// MIN and MAX as 32 bit integer for integer signals or else as signal type

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

	  // This is executed for the periods more than a second long.
	  // There is extra job to be done here: the outou is printed out into the
	  // string arrays sorted by channel (as it comes from the server), then 
	  // the string array is printed out sorted by time.
	  int ndata = period;
	  int value_span = 1; // in seconds

	  if (preferences.minuteTrendAcquisitionModeSelected ()) {
	    if (ndata < 60)
	      ndata = 1;
	    else
	      ndata /= 60;

	    value_span = 60;
	  }

	  String [][] lines = new String [channelSet.size ()] [ndata];
	  int chn = 0;

	  // Print data into the buffer
	  for (Enumeration e = channelSet.elements (); e.hasMoreElements (); chn++) {
	    Channel ch = (Channel) e.nextElement ();
	    int minmax_type = ch.isInteger ()? DataType._32bit_integer: ch.getDataType ();
	    int [] types = {minmax_type, minmax_type, DataType._64bit_double};

	    // Create string writers (string buffers)
	    StringWriter [] sw = new StringWriter [ndata];
	    PrintWriter [] pw = new PrintWriter [ndata];
	    for (int i = 0; i  < ndata; i++)
	      pw [i] = new PrintWriter ((Writer) (sw [i] = new StringWriter ()));

	    // Print the data out into the string buffers
	    for (int i = 0; i < 3; i++) { // foreach trend suffix
	      for (int j = 0; j < ndata; j++) { // foreach data point within the block
		pw [j].print (ch.getName () + DataType.trendSuffixes [i] + separator
				   + Integer.toString (block.getTimestamp () + j * value_span));
		DataType.csprint (pw [j], types [i], in, 1, separator);
		pw [j].println ();
	      }
	    }

	    // Save the strings from the writers into the buffer
	    for (int i = 0; i  < ndata; i++) {
	      pw [i].close ();
	      lines [chn][i] = sw [i].toString ();
	    }
	    
	  }

	  // Print the buffered lines out sorted by time
	  for (int i = 0; i < ndata; i++) { // foreach second within the block
	    for (int j = 0; j < channelSet.size (); j++) { // foreach channel
	      printWriter.print (lines [j] [i]);
	    }
	  }
	}
      }

      if (printWriter.checkError ())
	throw new IOException ("printWriter error");

      if (_debug > 8)
	System.err.println ("FileSaveFormatSingle.saveTextFile() saved one block");
    } catch (IOException e) {
      closePrintWriterError (printWriter, e, fileName);
      printWriter = null;
      if (_debug > 8)
	System.err.println ("FileSaveFormatSingle.saveTextFile() --> IOException");
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
	if (preferences.isServlet ()) {
	  printWriter.print ("\n-----\n");
	  printWriter.flush ();
	  if (_debug > 8)
	  	System.err.println ("writing finishing line for nds-data");
	}

	closePrintWriter (printWriter, fileName);
      } catch (IOException e) {
	closePrintWriterError (printWriter, e, fileName);
      }
    }
    if (dataOutputStream != null) {
      try {
	if (preferences.isServlet ()) {
	  dataOutputStream.writeBytes ("\n-----\n");
	  dataOutputStream.flush ();
	  if (_debug > 8)
	  	System.err.println ("writing finishing line for nds-data");
	}

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
