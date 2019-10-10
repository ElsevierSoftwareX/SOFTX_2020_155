#ifndef CHANNEL_H
#define CHANNEL_H

#ifdef __APPLE__
  #include <limits.h>
  #include <float.h>
#else
  #include <values.h>
#endif

#include <daq_data_types.h>

/* Allowed maximum length for DMT channels */
#define MAX_LONG_CHANNEL_NAME_LENGTH 255
/* Allowed maximum length for DAQ channels */
#define MAX_CHANNEL_NAME_LENGTH 60
/* Allowed maximum length for engineering units */
#define MAX_ENGR_UNIT_LENGTH 40
/* Groups are absolete and need to be removed from the source code */
#define MAX_CHANNEL_GROUPS 1024
/* Hard limit on the number of channel names supported:
   it needs to be eliminated, dynamically allocated arrays should be used */
/* #define MAX_CHANNELS 60000 */
#define MAX_CHANNELS 524288
#define MAX_TREND_CHANNELS  MAX_CHANNELS

/* should be equal to the last data type   */
#define MAX_DATA_TYPE _32bit_uint
#define MIN_DATA_TYPE _16bit_integer

inline static double
data_type_max(short dtype) {
  switch (dtype) {
  case _16bit_integer: // 16 bit integer
    #ifdef __APPLE__
      return SHRT_MAX;
    #else
      return MAXSHORT;
    #endif
  case _32bit_integer: // 32 bit integer
  case _32bit_uint: // 32 bit unsigned integer
    #ifdef __APPLE__
      return INT_MAX;
    #else
      return MAXINT;
    #endif
  case _32bit_float: // 32 bit float
    #ifdef __APPLE__
      return FLT_MAX;
    #else
      return MAXFLOAT;
    #endif
  case _64bit_integer: // 64 bit integer
    #ifdef __APPLE__
      return LONG_MAX;
    #else
      return MAXLONG;
    #endif
  case _64bit_double: // 64 bit double
    #ifdef __APPLE__
      return DBL_MAX;
    #else
      return MAXDOUBLE;
    #endif
  case _32bit_complex: // 32 bit complex
    #ifdef __APPLE__
      return FLT_MAX;
    #else
      return MAXFLOAT;
    #endif
  default:
    return _undefined;
  }
}

#endif
