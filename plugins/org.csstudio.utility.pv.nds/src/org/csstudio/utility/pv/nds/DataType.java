package org.csstudio.utility.pv.nds;

import java.io.*;

public class DataType implements DataTypeConstants {
  public static int length (int dt) {
    if (dt == _16bit_integer)
      return 2;
    else if (dt == _32bit_integer)
      return 4;
    else if (dt == _64bit_integer)
      return 8;
    else if (dt == _32bit_float)
      return 4;
    else if (dt == _64bit_double)
      return 8;
    else if (dt == _32byte_string)
      return 32;
    else
      return -1;
  }


  /**
   * Print `rate' samples of type `type' from `wave' starting at `wave[idx]' to `out'
   */
  /*
  public static void println (PrintWriter out, int type, byte [] wave,
			      int idx, int rate) throws IOException {
    if (type == _16bit_integer) {
      rate = 2*rate + idx;
      for (int i = idx; i < rate; i+= 2) {
	out.println ((((short) wave [i]) << 8) + (short) wave [i+1]);
      }
    } else if (type == _64bit_double) {
      rate = 8*rate + idx;
      for (int i = idx; i < rate; i+= 8) {
	out.println ((((short) wave [i]) << 8) + (short) wave [i+1]);
      }
    } else
      throw (new IOException ("Invalid data type #" + type));
  }
  */


  /**
   * Print `rate' data samples of type `type' from `in' to `out'
   * Comma separated
   */
  public final static void csprint (PrintWriter out, int type,
				    DataInputStream in, int rate, String separator) throws IOException {
    if (type == _16bit_integer)
      for (int i = 0; i < rate; i++)
	out.print (separator + in.readShort ());
    else if (type == _64bit_double)
      for (int i = 0; i < rate; i++)
	out.print (separator + in.readDouble ());
    else if (type == _32bit_float)
      for (int i = 0; i < rate; i++)
	out.print (separator + in.readFloat ());
    else if (type == _32bit_integer)
      for (int i = 0; i < rate; i++)
	out.print (separator + in.readInt ());
    else if (type == _64bit_integer)
      for (int i = 0; i < rate; i++)
	out.print (separator + in.readLong ());
    else if (type == _32byte_string) {
      byte [] str = new byte [32];
      for (int i = 0; i < rate; i++) {
	in.readFully (str);
	out.print (separator + str);
      }
    } else
      throw (new IOException ("Invalid data type #" + type));
  }

  /**
   * Print `rate' data samples of type `type' from `in' to `out'
   * Newline separated
   */
  public final static void println (PrintWriter out, int type,
				    DataInputStream in, int rate) throws IOException {
    if (type == _16bit_integer)
      for (int i = 0; i < rate; i++)
	out.println (in.readShort ());
    else if (type == _64bit_double)
      for (int i = 0; i < rate; i++)
	out.println (in.readDouble ());
    else if (type == _32bit_float)
      for (int i = 0; i < rate; i++)
	out.println (in.readFloat ());
    else if (type == _32bit_integer)
      for (int i = 0; i < rate; i++)
	out.println (in.readInt ());
    else if (type == _64bit_integer)
      for (int i = 0; i < rate; i++)
	out.println (in.readLong ());
    else if (type == _32byte_string) {
      byte [] str = new byte [32];
      for (int i = 0; i < rate; i++) {
	in.readFully (str);
	out.println (str);
      }
    } else
      throw (new IOException ("Invalid data type #" + type));
  }

  /**
   * Convert `rate' data samples of type `type' read from `in' to an array of doubles.
   */
  public final static void toDoubleArray (double [] outputArray, int type, DataInputStream in,
					       int index, int rate) {
    try {
      if (type == _16bit_integer)
	for (int i = 0; i < rate; i++)
	  outputArray [index + i] = (double) in.readShort ();
      else if (type == _64bit_double)
	for (int i = 0; i < rate; i++)
	  outputArray [index + i] = (double) in.readDouble ();
      else if (type == _32bit_float)
	for (int i = 0; i < rate; i++)
	  outputArray [index + i] = (double) in.readFloat ();
      else if (type == _32bit_integer)
	for (int i = 0; i < rate; i++)
	  outputArray [index + i] = (double) in.readInt ();
      else if (type == _64bit_integer)
	for (int i = 0; i < rate; i++)
	  outputArray [index + i] = (double) in.readLong ();
    } catch (IOException e) {
      ;
    }
  }
}
