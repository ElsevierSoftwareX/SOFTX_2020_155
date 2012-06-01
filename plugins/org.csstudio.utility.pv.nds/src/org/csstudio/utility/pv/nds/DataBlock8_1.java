package org.csstudio.utility.pv.nds;

import java.io.*;

/**
 * Data is sent from the server in blocks
 */
public class DataBlock8_1 extends DataBlock implements Debug {
  static final int version = 8;
  static final int revision = 1;
  final int hdrLen = 16;
  public byte [] wave;
  int timestamp;
  int dataSecs;
  int nanoResidual;
  int seqNum;
  boolean eot;

  /**
   * Construct data block from the data input stream
   */
  public DataBlock8_1 (DataInputStream is) {

    // Receive the header
    try {
      for (;;) {
	int blockLen = is.readInt ();
	dataSecs = is.readInt ();
	timestamp = is.readInt ();
	nanoResidual = is.readInt ();
	seqNum = is.readInt ();

	if (_debug > 6)
	  System.err.println ("blockLen=" + blockLen + " hdrLen=" + hdrLen + " dataSecs=" + dataSecs
			      + " timestamp=" + timestamp + " nanoResidual=" + nanoResidual + " seqNum=" + seqNum);
    
	if (blockLen == hdrLen) {
	  if (dataSecs == -1) {
	    // heartbeat block -- skip it
	    heartbeat = true;
	    return;
	  }

	  // end of transmission
	  if (_debug > 6)
	    System.err.println ("EOT");

	  eot = true;
	  return;
	}

	eot = false;
	wave = new byte [blockLen - hdrLen];
	is.readFully (wave);

	// skip informational block
	if (dataSecs == -1) {
	  continue;
	}
	return;
      }
    } catch (IOException e) {
      eot = true;
      return;
    }
  }

  private boolean heartbeat = false;

  public static int getVersion () { return version; }
  public static int getRevision () { return revision; }
  public int getTimestamp () { return timestamp; }
  public int getNano () { return nanoResidual; }

  public byte [] getWave () { return wave; }
  public boolean getEot () { return eot; }
  public int getPeriod () { return dataSecs; }
  public boolean getHeartbeat () { return heartbeat; }
}
