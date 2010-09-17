#ifndef CHANNEL_H
#define CHANNEL_H

#include <values.h>

/* Allowed maximum length for DMT channels */
#define MAX_LONG_CHANNEL_NAME_LENGTH 255
/* Allowed maximum length for DAQ channels */
#define MAX_CHANNEL_NAME_LENGTH 40
/* Groups are absolete and need to be removed from the source code */
#define MAX_CHANNEL_GROUPS 150
/* Hard limit on the number of channel names supported:
   it needs to be eliminated, dynamically allocated arrays should be used */
#define MAX_CHANNELS 60000
#define MAX_TREND_CHANNELS  60000

/* numbering must be contiguous */
typedef enum {
  _undefined = 0,
  _16bit_integer = 1,
  _32bit_integer = 2,
  _64bit_integer = 3,
  _32bit_float = 4,
  _64bit_double = 5,
  _32bit_complex = 6
} daq_data_t;

/* should be equal to the last data type   */
#define MAX_DATA_TYPE _32bit_complex
#define MIN_DATA_TYPE _16bit_integer

inline static int
data_type_size (short dtype) {
  switch (dtype) {
  case _16bit_integer: // 16 bit integer
    return 2;
  case _32bit_integer: // 32 bit integer
  case _32bit_float: // 32 bit float
    return 4;
  case _64bit_integer: // 64 bit integer
  case _64bit_double: // 64 bit double
    return 8;
  case _32bit_complex: // 32 bit complex
    return 4*2;
  default:
    return _undefined;
  }
}

inline static double
data_type_max(short dtype) {
  switch (dtype) {
  case _16bit_integer: // 16 bit integer
    return MAXSHORT;
  case _32bit_integer: // 32 bit integer
    return MAXINT;
  case _32bit_float: // 32 bit float
    return MAXFLOAT;
  case _64bit_integer: // 64 bit integer
    return MAXLONG;
  case _64bit_double: // 64 bit double
    return MAXDOUBLE;
  case _32bit_complex: // 32 bit complex
    return MAXFLOAT;
  default:
    return _undefined;
  }
}

#endif
