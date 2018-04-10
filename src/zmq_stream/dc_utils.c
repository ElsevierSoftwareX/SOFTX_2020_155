#include <string.h>
#include <stdio.h>

#include "dc_utils.h"
#include "daq_core.h"

int dc_generate_connection_string(char *dest, const char *src, size_t dest_len)
{
    static const char *proto_sep = "://";
    static const char *port_sep = ":";
    int port = DAQ_DATA_PORT;
    char *proto = "tcp";
    char *cur = 0;
    char *name_start = 0;
    char *port_start = 0;
    size_t length = 0;
    char tmp_proto[10];
    char tmp_name[100];

    if (!dest || !src || dest_len == 0) return 0;
    name_start = (char*)src;
    cur = strstr(src, proto_sep);
    if (cur) {
        length = cur - src;
        if (length >= sizeof(tmp_proto)) {
            return 0;
        }
        strncpy(tmp_proto, src, length);
        tmp_proto[length] = 0;
        proto = tmp_proto;
        name_start = cur + strlen(tmp_proto);
    }
    port_start = strstr(name_start, port_sep);
    if (port_start) {
        /* name first */
        length = port_start - name_start;
        if (length > sizeof(tmp_name)) {
            return 0;
        }
        strncpy(tmp_name, name_start, length);
        tmp_name[length] = 0;

        /* port */
        port_start += strlen(port_sep);
        length = strlen(port_start);
        if (length > 0) {
            port = atoi(port_start);
        }
    } else {
        length = strlen(name_start);
        if (length >= sizeof(tmp_name)) {
            return 0;
        }
        strncpy(tmp_name, name_start, length);
        tmp_name[length] = 0;
    }
    if (port == 0) {
        port = DAQ_DATA_PORT;
    }
    snprintf(dest, dest_len, "%s://%s:%d", proto, tmp_name, port);
    return 1;
}
