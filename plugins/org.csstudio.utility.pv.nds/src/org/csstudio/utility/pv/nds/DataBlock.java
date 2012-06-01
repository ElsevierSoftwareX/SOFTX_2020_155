package org.csstudio.utility.pv.nds;

import java.io.*;

/**
 * Data is sent from the server in blocks
 */
public abstract class DataBlock implements Debug {
  public abstract int getTimestamp ();
  public abstract int getNano ();

  public abstract byte [] getWave ();
  public abstract boolean getEot ();
  public abstract int getPeriod ();
  public abstract boolean getHeartbeat ();
}

