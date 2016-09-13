#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sisci_types.h"
#include "sisci_error.h"
#include <malloc.h>
#include "./dx.h"

main()
{
DX_INFO mydxinfo;
volatile void *rdAddr;
volatile void *wrAddr;

sci_error_t             error;

    mydxinfo.segmentId = 2;
    mydxinfo.localAdapterNo = 0;
    mydxinfo.segmentSize = 32000000;
    mydxinfo.mode = DX_SND_AND_RCV;

    error = dx_init(&mydxinfo);
    if(mydxinfo.mode != DX_SEND_ONLY) {
    	rdAddr = dx_attach_local_segment(&mydxinfo);
	    if (rdAddr != NULL) {
		printf("Local segment is mapped to user space at 0x%lx. \n" ,(unsigned long)rdAddr);
	    } else {
		fprintf(stderr,"SCIMapLocalSegment failed - Error code 0x%x\n",error);
		return error;
	    }
    }

    if(mydxinfo.mode != DX_RCVR_ONLY) {
    	wrAddr = dx_attach_remote_segment(&mydxinfo);
	    if (wrAddr != NULL) {
		printf("Remote segment is mapped to user space at 0x%lx. \n" ,(unsigned long)rdAddr);
	    } else {
		fprintf(stderr,"SCIMapRemoteSegment failed - Error code 0x%x\n",error);
		return error;
	    }
    }


sleep(5);

error = dx_cleanup(&mydxinfo);
}
