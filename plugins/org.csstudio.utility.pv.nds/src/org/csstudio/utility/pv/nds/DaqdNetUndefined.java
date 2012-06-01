package org.csstudio.utility.pv.nds;

import java.awt.*;
import java.net.*;
import java.io.*;

class EmptyDataBlock extends DataBlock {
  public int getTimestamp () { return 0; }
  public byte [] getWave () { return null; }
  public boolean getEot () { return true; }
  public int getPeriod () { return 0; }
  public boolean getHeartbeat () { return false; }
@Override
public int getNano() {
	// TODO Auto-generated method stub
	return 0;
}
}

/**
 * DaqdNet protocol undefined version and revision
 */
public class DaqdNetUndefined extends Net implements Debug {
  Label infoLabel;

  public void setLabel (Label infoLabel) {
    this.infoLabel = infoLabel;
  }

  public Label getLabel () {
    return infoLabel;
  }

  public ChannelSet getChannels () { return null; }
  public GroupSet getGroups () { return null; }
  public boolean startNetWriter (ChannelSet a, long b, int c) { return false; }
  public boolean stopNetWriter () { return false; }
  public DataBlock getDataBlock () { return new EmptyDataBlock (); }
  public int getVersion () { return 0; }
  public int getRevision () { return 0; }
}
