#ifndef ZMQ_DAQ_H
#define ZMQ_DAQ_H

#include "daq_core.h"

typedef struct channel_t {
	char name[64];
	int type;
	int datarate;
	int datasize;
	unsigned int timesec;
	unsigned int timensec;
	int unused;	// Necessary to maintain 8byte boundary
}channel_t;

typedef struct nds_data_t {
	channel_t ndschan;
	char ndsdata[10000];
} nds_data_t;

typedef union ndsdatau {
	char s[10000];
	float f[4096];
	int i[4096];
	unsigned int ui[4096];
	double d[4096];
} ndsdatau;

typedef struct nds_data_r {
	channel_t ndschan;
	union ndsdatau ndsdata;
}nds_data_r;

#endif /* ZMQ_DAQ_H */