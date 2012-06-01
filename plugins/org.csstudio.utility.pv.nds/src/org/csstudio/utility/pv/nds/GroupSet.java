package org.csstudio.utility.pv.nds;

import java.util.*;
import java.io.*;

/**
 * A vector of server channel groups
 */
public class GroupSet extends Vector implements DataTypeConstants {

  /**
   * Add group
   */
  public void addGroup (String name, int number) {
    addElement (new Group (name, number));
  }

  /**
   * Delete group by name from the set
   */
  public boolean delGroup (String name) {
    return removeElement (groupObject (name));
  }

  /**
   * Delete all elements from the vector
   */
  public void removeAllGroups () {
    removeAllElements ();
  }

  /**
   * Useful to check if particular channel group is in the set
   */
  public Group groupObject (String name) {
    Group ch;

    // Find group object, if any
    for (Enumeration e = elements (); e.hasMoreElements ();)
      if (name.equals ((ch = (Group) e.nextElement ()).name))
	return ch;
    return null;
  }
}
