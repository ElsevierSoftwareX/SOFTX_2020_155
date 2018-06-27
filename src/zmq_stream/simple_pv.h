#ifndef __STREAMING_SIMPLE_PV_H__
#define __STREAMING_SIMPLE_PV_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SimplePV {
    char *name;
    int *data;

    int alarm_high;
    int alarm_low;
    int warn_high;
    int warn_low;
} SimplePV;

// write out a list of pv updates to the given file.
// The data is written out as a json text blob prefixed by a binary length (sizeof size_t host byte order)
// if there is a failure, nothing happens
// it is safe to call with a negative fd
extern void send_pv_update(int fd, const char *prefix, SimplePV *pvs, int pv_count);

#ifdef __cplusplus
}
#endif

#endif /* __STREAMING_SIMPLE_PV_H__ */