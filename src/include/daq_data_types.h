#ifndef DAQD_DATA_TYPES_H
#define DAQD_DATA_TYPES_H

/* numbering must be contiguous */
typedef enum {
    _undefined = 0,
    _16bit_integer = 1,
    _32bit_integer = 2,
    _64bit_integer = 3,
    _32bit_float = 4,
    _64bit_double = 5,
    _32bit_complex = 6,
    _32bit_uint = 7
} daq_data_t;

inline static int
data_type_size (short dtype) {
    switch (dtype) {
        case _16bit_integer: // 16 bit integer
            return 2;
        case _32bit_integer: // 32 bit integer
        case _32bit_float: // 32 bit float
        case _32bit_uint: // 32 bit unsigned integer
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

#endif