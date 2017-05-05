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

typedef struct channel_t {
    char name[64];
    int type;
    int value;
}channel_t;

int main (int argc, char *argv [])
{

channel_t ndschannel;
channel_t *ndsptr = &ndschannel;
zmq_msg_t message;

int ii;

char chnames[2][32] = {"X1:ATS-CPU_METER","X1:ATS-TIME_DIAG"};
printf ("Collecting updates from NDS proxy\n");
    void *context = zmq_ctx_new ();
    void *subscriber = zmq_socket (context, ZMQ_SUB);
    int rc = zmq_connect (subscriber, "tcp://scipe19:6666");
    assert (rc == 0);

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
	printf("Name = %s\ttype = %d\tValue = %d\n",ndschannel.name,ndschannel.type,ndschannel.value);
     }

     zmq_close (subscriber);
         zmq_ctx_destroy (context);
     return 0;
}

