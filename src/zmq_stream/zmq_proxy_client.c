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

nds_data_t ndschannel;
char *ndsptr = (char *)&ndschannel;
zmq_msg_t message;

int ii;
char loc[32];

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
     for (update_nbr = 0; update_nbr < 100; update_nbr++) {
        zmq_msg_init(&message);
	int size = zmq_msg_recv(&message,subscriber,0);
	assert(size >= 0);
	char *string = (char *)zmq_msg_data(&message);
	memcpy(ndsptr,string,size);
	#if 0
 	char name[64];
     	int datatype, datavalue;
        sscanf (string, "%s %d %d",
                name, &datatype, &datavalue);
	#endif
	zmq_msg_close(&message);
	if(ndschannel.ndschan.type == 2) {
	    int *idata = (int *)&ndschannel.ndsdata[0];
	    printf("Name = %s\t",ndschannel.ndschan.name);
	    printf("data = \t%d\n",*idata);
	}
     }

     zmq_close (subscriber);
         zmq_ctx_destroy (context);
     return 0;
}

