package org.csstudio.utility.pv.nds;

import java.io.*;

/**
 * DAQD data channel group description
 */
public class Group implements DataTypeConstants, Cloneable {

  public Group (String name, int number) {
    this.number = number; this.name = name;
  }

  // Group properties
  String name;
  int number;

  /**
   * Get a copy of this group
   */
  public Group cloneGroup () {
    Group ch;

    try {
      ch = (Group) this.clone ();
    } catch (CloneNotSupportedException e) {
      return null;
    }
    return ch;
  }

  public void setName (String name) { this.name = name; }
  public String getName () { return name; }
  public void setNumber (int number) { this.number = number; }
  public int getNumber () { return number; }
}
