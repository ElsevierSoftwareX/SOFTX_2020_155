#ifndef DC_UTILS_H
#define DC_UTILS_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Generate a well formed connection string, typically used with zmq.
 * @param dest Destination buffer of dest_len bytes
 * @param src Input string
 * @param dest_len Size of *dest
 * @param src_iface Optional source interface name, used to identify which
 * interface the connection should originate from.
 * @return 0 on failure, 1 on success.  On failure the contents of *dest are
 * undefined.
 * @note This looks for protocol, hostname, port of the form
 * protocol://hostname:port and applies default values if no protocol or port is
 * found.
 */
extern int dc_generate_connection_string( char*       dest,
                                          const char* src,
                                          size_t      dest_len,
                                          const char* src_iface );

extern int dc_set_zmq_options( void* z_socket );

#ifdef __cplusplus
}
#endif

#endif /* DC_UTILS_H */