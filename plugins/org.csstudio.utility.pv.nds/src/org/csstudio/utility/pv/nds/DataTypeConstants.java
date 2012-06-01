package org.csstudio.utility.pv.nds;

/**
 * Communication interface constants
 */
public interface DataTypeConstants {
  static final int _16bit_integer = 1;
  static final int _32bit_integer = 2;
  static final int _64bit_integer = 3;
  static final int _32bit_float = 4;
  static final int _64bit_double = 5;
  static final int _32byte_string = 6;

  static final int MAX_CHANNEL_NAME_LENGTH = 60; // defined in server's channel.h
  static final int [] validRates = {1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768,65536};

  static final int DEFAULT_DAQD_PORT = 8088;
  static final String [] trendSuffixes = {".min", ".max", ".rms"};
}
