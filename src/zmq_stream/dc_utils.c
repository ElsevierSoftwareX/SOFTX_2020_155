#include <string.h>
#include <stdio.h>

#include "dc_utils.h"
#include "daq_core.h"

#include <zmq.h>

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
        name_start = cur + strlen(proto_sep);
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

int dc_set_zmq_options(void *z_socket)
{
    char *env = 0;
    int i_value = 0;

    if (!z_socket) {
        return 0;
    }
    env = getenv("ZMQ_MULTICAST_MAXTPDU");
    if (env) {
        i_value = atoi(env);
        zmq_setsockopt(z_socket, ZMQ_MULTICAST_MAXTPDU, &i_value, sizeof(int));
    }
    env = getenv("ZMQ_RATE");
    if (env) {
        i_value = atoi(env);
        zmq_setsockopt(z_socket, ZMQ_RATE, &i_value, sizeof(int));
    }
    env = getenv("ZMQ_RECONNECT_IVL");
    if (env) {
        i_value = atoi(env);
        zmq_setsockopt(z_socket, ZMQ_RECONNECT_IVL, &i_value, sizeof(int));
    }
    env = getenv("ZMQ_HANDSHAKE_IVL");
    if (env) {
        i_value = atoi(env);
        zmq_setsockopt(z_socket, ZMQ_HANDSHAKE_IVL, &i_value, sizeof(int));
    }
    env = getenv("ZMQ_CONNECT_TIMEOUT");
    if (env) {
        i_value = atoi(env);
        zmq_setsockopt(z_socket, ZMQ_CONNECT_TIMEOUT, &i_value, sizeof(int));
    }
    return 1;
}
