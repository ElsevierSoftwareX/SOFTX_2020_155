package org.csstudio.utility.pv.nds;

import java.awt.*;
import java.net.*;
import java.io.*;
import java.util.*;

/**
 * DAQD network connection and communication protocol
 */
public class DaqdNet9_1 extends DaqdNet9_0 {
  static final int version = 9;
  static final int revision = 1;

  public DaqdNet9_1 () {
      super ();
  }

  public DaqdNet9_1 (Label infoLabel, Preferences pref) {
    super (infoLabel, pref);
  }
} 
