#include <unistd.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <zmq.h>
#include <assert.h>
#include "zmq_daq.h"


int main (int argc, char *argv [])
{

nds_data_r ndschannel;
char *ndsptr = (char *)&ndschannel;
zmq_msg_t message;
int num_vals;

int ii;
char loc[32];
float fdata[4096];
int idata[4096];
unsigned int uidata[4096];
double ddata[4096];

    sprintf(loc,"%s%d","tcp://x2daqdc0-out:",DAQ_DATA_PORT);
    void *context = zmq_ctx_new ();
    void *subscriber = zmq_socket (context, ZMQ_SUB);
    int rc = zmq_connect (subscriber, loc);
    assert (rc == 0);
    printf ("Collecting updates from NDS proxy %s\n",loc);

    for(ii=1;ii<argc;ii++) {
    	char *filter = argv [ii];
    	rc = zmq_setsockopt (subscriber, ZMQ_SUBSCRIBE,
                         filter, strlen (filter));
	assert (rc == 0);
	}
     int update_nbr;
     for (update_nbr = 0; update_nbr < 150; update_nbr++) {
        zmq_msg_init(&message);
	int size = zmq_msg_recv(&message,subscriber,0);
	assert(size >= 0);
	char *string = (char *)zmq_msg_data(&message);
	memcpy(ndsptr,string,size);
	zmq_msg_close(&message);
	num_vals = ndschannel.ndschan.datarate / 16;
	if(ndschannel.ndschan.type == 2 && num_vals > 0) {
	    for(ii=0;ii<num_vals;ii++) idata[ii] = ndschannel.ndsdata.i[ii];
	    printf("Name = %-44s\t%d\t",ndschannel.ndschan.name,num_vals);
	    printf("data = \t%d\tinteger\n",idata[0]);
	}
	if(ndschannel.ndschan.type == 7 && num_vals > 0) {
	    for(ii=0;ii<num_vals;ii++) uidata[ii] = ndschannel.ndsdata.ui[ii];
	    printf("Name = %-44s\t%d\t",ndschannel.ndschan.name,num_vals);
	    printf("data = \t%u\tunsigned int\n",uidata[0]);
	}
	if(ndschannel.ndschan.type == 4 && num_vals > 0) {
	    for(ii=0;ii<num_vals;ii++) fdata[ii] = ndschannel.ndsdata.f[ii];
	    printf("Name = %-44s\t%d\t",ndschannel.ndschan.name,num_vals);
	    printf("data = \t%f\tfloat\n",fdata[0]);
	}
	if(ndschannel.ndschan.type == 5 && num_vals > 0) {
	    for(ii=0;ii<num_vals;ii++) ddata[ii] = ndschannel.ndsdata.d[ii];
	    printf("Name = %-44s\t%d\t",ndschannel.ndschan.name,num_vals);
	    printf("data = \t%f\tfloat\n",ddata[0]);
	}
     }

     zmq_close (subscriber);
         zmq_ctx_destroy (context);
     return 0;
}

