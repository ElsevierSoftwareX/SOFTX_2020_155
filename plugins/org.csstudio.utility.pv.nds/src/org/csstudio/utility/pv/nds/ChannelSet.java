package org.csstudio.utility.pv.nds;

import java.util.*;
import java.io.*;

/**
 * A vector of channels
 */
public class ChannelSet extends Vector implements DataTypeConstants, Serializable {
  transient ChannelList list;

  /**
   * A set of known server channels
   */
  transient ChannelSet serverSet;

  /**
   * Vector copy constructor
   */
  public ChannelSet (ChannelSet channelSet, ChannelList channelList) {
    this.list = channelList;

    if (channelSet != null) {
       // Add all elements from the `channelSet'
       // No cloning since I am not changing channel objects here
       for (Enumeration e = channelSet.elements (); e.hasMoreElements ();) {
         Object obj;

         addElement (obj = e.nextElement ());
         list.addChannel ((Channel) obj);
       }

       // Copy the server set
       serverSet = channelSet.getServerSet ();
    } else
       serverSet = null;
  }

  
  public ChannelSet (ChannelList list, Net net) {
    serverSet = net.getChannels ();
    this.list = list;
  }

  public ChannelSet () {
    list = null;
    serverSet = null;
  }

  public ChannelSet (ChannelList list) {
    this.list = list;
    serverSet = null;
  }


  /**
   * Add channel to the list, default rate
   */
  public boolean addChannelDefaultRate (String name) {
    if (serverSet == null)
      return false;

    Channel ch = serverSet.channelObject (name);
    if (ch == null) {
      System.err.println ("`" + name + "' was not found on server; channel ignored");
      return false;
    }

    addElement (ch = ch.cloneChannel ());

    if (list != null) {
      list.addChannel (ch);
    }
    return true;
  }
  
  /**
   * Add channel to the set and update channel display list
   */
  public boolean addChannel (String name, int rate) {
    Channel ch;

    // Check for the power of two
    int i;
    for (i = 0; i < validRates.length; i++)
      if (rate == validRates [i])
	break;

    if (i == validRates.length) {
      System.err.println ("`" + name + "' has invalid data rate " + rate + "; channel ignored");
      return false;
    }

    if (serverSet != null) {
      ch = serverSet.channelObject (name);
      if (ch == null) {
	System.err.println ("`" + name + "' was not found on server; channel ignored");
	return false;
      }

      if (rate > ch.getRate ()) {
	System.err.println ("Warning:`" + name + "' has invalid data rate " + rate
			    + "; maximum rate is " + ch.getRate ()
			    + "; data rate set to " + ch.getRate ());
	rate = ch.getRate ();
      }
      ch = ch.cloneChannel ();
      ch.setRate (rate);
    } else {
      System.err.println ("Can't add a channel to a set: no server channel set");
      return false;
    }

    for (i = 0; i < size (); i++)
      if (((Channel) elementAt (i)).getSequenceNumber () > ch.getSequenceNumber ())
	break;

    insertElementAt (ch, i);
    if (list != null) {
      list.addChannel (ch, i);
    }
    return true;
  }

  /**
   * Add channel, all params; It doesn't try to update channel list or check arguments for validity
   */
  public void addChannel (String name, int rate, int group,
			     int bps, int data_type, boolean trend, int seq) {
    addElement (new Channel (name, rate, group, bps, data_type, trend, seq));
  }

  /**
   * Add channel to the set; rate is 16
   */
  public boolean addChannel (String name) {
    return addChannel (name, 16);
  }

  /**
   * Add selected channels from the other set
   */
  public void addSelectedChannels (ChannelSet channelSet) {
    int [] selectedIndexes = channelSet.getChannelList ().getSelectedIndexes ();
    for (int i = 0; i < selectedIndexes.length; i++)
      addChannel (((Channel) channelSet.elementAt (selectedIndexes [i])).getName (),
		  ((Channel) channelSet.elementAt (selectedIndexes [i])).getRate ());
  }

  /**
   * Delete all selected channels from the set
   */
  public void removeSelectedChannels () {
    int [] selectedIndexes = list.getSelectedIndexes ();
    for (int i = selectedIndexes.length -1; i >= 0; --i)
      if (selectedIndexes [i] < size ()) {
	removeElementAt (selectedIndexes [i]);
	list.delItem (selectedIndexes [i]);
      }    
  }

  /**
   * Replace channel set with another
   * Update display list too
   */
  public void replaceChannels (ChannelSet channelSet) {
    removeAllChannels ();
    for (Enumeration e = channelSet.elements (); e.hasMoreElements ();) {
      Channel ch = (Channel) e.nextElement ();
      addChannel (ch.getName (), ch.getRate ());
    }
  }

  /**
   * Delete channel by name from the set
   */
  public boolean delChannel (String name) {
    return removeElement (channelObject (name));
  }

  /**
   * Delete all elements from the vector and the display list
   */
  public void removeAllChannels () {
    removeAllElements ();
    if (list != null)
      if (list.getItemCount () > 0)
	list.removeAll ();
  }

  /**
   * Useful to check if particular channel is in the set
   */
  public Channel channelObject (String name) {
    Channel ch;

    // Find channel object, if any
    for (Enumeration e = elements (); e.hasMoreElements ();)
      if (name.equals ((ch = (Channel) e.nextElement ()).name))
	return ch;
    return null;
  }

  /**
   * Change rate for indexed channels
   */
  void changeRate (int [] selectedIndexes, int rate) {
    for (int i = 0; i < selectedIndexes.length; i++)
      if (selectedIndexes [i] < size ()) {
	Channel ch = (Channel) elementAt (selectedIndexes [i]);

	if (serverSet == null) 
	  ch.setRate (rate);
	else {
	  // Channel rate cannot be set higher than maximum
	  if (rate <= serverSet.channelObject (ch.getName ()).getRate ())
	    ch.setRate (rate);
	}

	if (list != null)
	  list.replaceChannel (ch, selectedIndexes [i]);
      }
  }


  /**
   * Change rate for all channels
   */
  void changeRate (int rate) {
    int [] selectedIndexes = new int [size ()];
    for (int i = 0; i < size (); i++)
      selectedIndexes [i] = i;
    changeRate (selectedIndexes, rate);
  }

  public ChannelList getChannelList () { return list; }
  public ChannelSet getServerSet () { return serverSet; }
}
