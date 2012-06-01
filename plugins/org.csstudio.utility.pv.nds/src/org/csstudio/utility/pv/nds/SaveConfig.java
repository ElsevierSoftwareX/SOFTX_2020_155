package org.csstudio.utility.pv.nds;

import java.io.*;
import java.util.*;
import java.awt.*;

/**
 * Save configuration file
 */
public class SaveConfig  {
  PrintWriter out;
  FileSaveFormat fileSaveFormat;
  Preferences preferences;
  public SaveConfig (PrintWriter out, Preferences preferences) {
    this.out = out;
    this.preferences = preferences;
  }

  /**
   * Write channel config file
   */
  public boolean saveFile () throws IOException {
    for (Enumeration em = preferences.getChannelSet ().elements (); em.hasMoreElements ();) {
      Channel ch = (Channel) em.nextElement ();
      out.println ("channel\t" +  ch.getName () + "\t" +  Integer.toString (ch.getRate ()));
    }

    // write data file format
    out.println ("format\t" + preferences.getExportFormat ());

    return true;
  }


}
