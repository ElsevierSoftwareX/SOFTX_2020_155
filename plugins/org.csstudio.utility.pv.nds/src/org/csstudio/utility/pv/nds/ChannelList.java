package org.csstudio.utility.pv.nds;

import java.awt.*;

/**
 * List GUI component for selected DAQD data channels
 */
class ChannelList extends List {
  public ChannelList() {
    setMultipleMode (true);
    //    setBackground(Color.white);
  }

  public ChannelList(int n) {
    super (n);
    setMultipleMode (true);
    //    setBackground(Color.white);
  }

  /**
   * Add name and rate to the selection list
   */
  public void addChannel (Channel ch) {
    addItem (getItemString (ch));
  }

  /**
   * Add name and rate to the selection list at the position
   */
  public void addChannel (Channel ch, int i) {
    addItem (getItemString (ch), i);
  }

  /**
   * Replace indexed item
   */
  public void replaceChannel (Channel ch, int i) {
    replaceItem (getItemString (ch), i);
  }

  /**
   * Return item display string for the channel
   */
  String getItemString (Channel ch) {
    return ch.getName () + ", " + ch.getRate ();
  }
}
