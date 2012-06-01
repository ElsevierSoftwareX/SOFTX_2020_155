package org.csstudio.utility.pv.nds;

import java.io.*;

/**
 * DAQD data channel description
 */
public class Channel implements DataTypeConstants, Cloneable, Serializable {
  /*
  public Channel (String name, int rate) {
    this.name = name; this.rate = rate;
    group = bps = dataType = -1;
    trend = false;
  }
  */
  public Channel (String name, int rate, int group,
		  int bps, int dataType, boolean trend, int seq) {
    this.name = name; this.rate = rate; this.group = group;
    this.bps = bps; this.dataType = dataType;
    this.trend = trend;
    sequenceNumber = seq;
  }

  int sequenceNumber; // Channels are sequenced in the server's order
  
  // Channel properties
  String name;
  int rate;
  boolean trend;
  int group;
  int bps;
  int dataType;
  

  /**
   * Returns true if this channel's data type is integer
   */
  public boolean isInteger () {
    return dataType == _16bit_integer 
      || dataType == _32bit_integer
      || dataType == _64bit_integer
      ;
  }

  /**
   * Returns true if this channel's data type is of floating point type
   */
  public boolean isFloat () {
    return dataType == _32bit_float
      || dataType == _64bit_double
      ;
  }

  /**
   * Returns true if this channel's data type is a string
   */
  public boolean isString () {
    return dataType == _32byte_string;
  }

  /**
   * Get a copy of this channel
   */
  public Channel cloneChannel () {
    Channel ch;

    try {
      ch = (Channel) this.clone ();
    } catch (CloneNotSupportedException e) {
      return null;
    }
    return ch;
  }

  public void setRate (int rate) { this.rate = rate; }
  public int getRate () { return rate; }
  public void setName (String name) { this.name = name; }
  public String getName () { return name; }
  public void setTrendFlag (boolean trend) { this.trend = trend; }
  public boolean getTrendFlag () { return trend; }
  public void setBps (int bpc) { this.bps = bps; }
  public int getBps () { return bps; }
  public void setGroup (int group) { this.group = group; }
  public int getGroup () { return group; }
  public void setDataType (int dataType) { this.dataType = dataType; }
  public int getDataType () { return dataType; }
  public void setSequenceNumber (int sequenceNumber) { this.sequenceNumber = sequenceNumber; }
  public int getSequenceNumber () { return sequenceNumber; }
}
