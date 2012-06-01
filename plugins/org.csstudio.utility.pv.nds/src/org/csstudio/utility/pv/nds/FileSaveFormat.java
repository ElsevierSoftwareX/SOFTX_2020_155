package org.csstudio.utility.pv.nds;

import java.io.*;
import java.awt.*;
import java.util.*;

/**
 * Save data files in one of the formats
 */
public abstract class FileSaveFormat extends FileSaver  {
  Preferences preferences;
  Label fileLabel;
  String lastFileWritten;
  int filesWritten;
  boolean textFile = true;
  OutputStream outputStream;

  public FileSaveFormat (Preferences preferences, Label fileLabel) {
    this.preferences = preferences;
    this.fileLabel = fileLabel;
    filesWritten = 0;
    lastFileWritten = "";
    textFile = preferences.csvFormatSelected () || preferences.tabsFormatSelected ();
    outputStream = null;
  }

  public FileSaveFormat (Preferences preferences, Label fileLabel, OutputStream outputStream) {
    this (preferences, fileLabel);
    this.outputStream = outputStream;
  }

  /**
   * Save data block in a file
   * Timestamp and export type define the filename
   */
  public synchronized boolean saveFile (DataBlock block) {
    if (textFile)
      return saveTextFile (block);
    else if (preferences.binaryWithHeaderFormatSelected ()) 
      return saveBinaryWithHeaderFile (block);
    else
      return saveBinaryFile (block);
  }

  public void startSavingSession () {
    if (fileLabel != null)
      fileLabel.setText ("no files written yet");
    filesWritten = 0;
  }

  public synchronized void endSavingSession () {
    if (fileLabel != null) {
      if (filesWritten == 1)
	fileLabel.setText ("single file `" + lastFileWritten  + "' written");
      else if (filesWritten == 0)
	fileLabel.setText ("no files written");	     
      else
	fileLabel.setText (filesWritten + " files written");
    }
  }

  public final String getFileName (int time) {
    return preferences.getFileDirectory ()
      + (preferences.getFileDirectory ().length () != 0? File.separator: "")
      + time
      + preferences.getModeFileNameString ()
      + "." + preferences.getExportFormat ();
  }

  public final String getFileName (int time, String channelName) {
    return preferences.getFileDirectory ()
      + (preferences.getFileDirectory ().length () != 0? File.separator: "")
      + time + "_" + channelName
      + preferences.getModeFileNameString ()
      + "." + preferences.getExportFormat ();
  }

  public final PrintWriter createPrintWriter (String fileName) throws IOException {      
    if (outputStream != null)
      return new PrintWriter (outputStream, true);
    else
      return new PrintWriter (new BufferedWriter (new FileWriter (fileName)));
  }

  public final void closePrintWriterError (PrintWriter printWriter, Exception e, String fileName) {
    if (fileLabel != null)
      fileLabel.setText ("IO Error printing to " + fileName);
    System.err.println ("While printing to a file:");
    System.err.println (e);
    try {
      if (printWriter != null)
	printWriter.close ();
    } catch (Exception ee) {
      ;;
    }
  }

  public final void closePrintWriter (PrintWriter printWriter, String fileName) throws IOException {
    printWriter.flush ();
    printWriter.close ();
    lastFileWritten = fileName;
    if (fileLabel != null)
      fileLabel.setText (fileName + " written");
    filesWritten++;
  }

  public final DataOutputStream createDataOutputStream (String fileName) throws IOException {
    if (outputStream != null)
      return new DataOutputStream (outputStream);
    else
      return new DataOutputStream (new BufferedOutputStream (new FileOutputStream (fileName)));
  }

  public final void closeDataOutputStreamError (DataOutputStream dataOutputStream, Exception e, String fileName) {
    if (fileLabel != null)
      fileLabel.setText ("IO Error writing to " + fileName);
    System.err.println ("While writing to a file:");
    System.err.println (e);
    try {
      if (dataOutputStream != null)
	dataOutputStream.close ();
    } catch (Exception ee) {
      ;;
    }
  }

  public final void  closeDataOutputStream (DataOutputStream dataOutputStream, String fileName) throws IOException {
    dataOutputStream.flush ();
    dataOutputStream.close ();
    lastFileWritten = fileName;
    if (fileLabel != null)
      fileLabel.setText (fileName + " written");
    filesWritten++;
  }

  public abstract boolean saveTextFile (DataBlock block);
  public abstract boolean saveBinaryFile (DataBlock block);
  public abstract boolean saveBinaryWithHeaderFile (DataBlock block);
}
