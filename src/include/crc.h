///	@file crc.h
///	@brief Contains prototype defs for CRC checking functions found in
////drv/crc.c
unsigned int crc_ptr( char* cp, unsigned int bytes, unsigned int crc );
unsigned int crc_len( unsigned int bytes, unsigned int crc );
