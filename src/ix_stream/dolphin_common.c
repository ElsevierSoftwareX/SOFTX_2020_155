
#define NO_CALLBACK         NULL
#define NO_FLAGS            0
#define DATA_TRANSFER_READY 8
#define CMD_READY           1234
/* Use upper 4 KB of segment for synchronization. */
// #define SYNC_OFFSET ((segmentSize) / 4 - 1024)
#define IX_SYNC_OFFSET 0x1000

/*
 * Remote nodeId:
 *
 * DIS_BROADCAST_NODEID_GROUP_ALL is a general broadcast remote nodeId and 
 * must be used as the remote nodeId in SCIConnectSegment() function
 */


sci_error_t             error;
sci_desc_t              sd;
sci_local_segment_t     localSegment;
sci_remote_segment_t    remoteSegment;
sci_map_t               localMap;
sci_map_t               remoteMap;
sci_sequence_t        sequence   = NULL;
unsigned int            localAdapterNo = 0;
unsigned int            remoteNodeId   = 0;
unsigned int            localNodeId    = 0;
unsigned int            segmentId;
unsigned int            segmentSize    = 0x400000;
unsigned int            offset         = 0;
unsigned int            client         = 0;
unsigned int            server         = 1;
unsigned int            *localbufferPtr;
int                     rank           = 0;
int                     nodes          = 0;
unsigned int 		memcpyFlag     = NO_FLAGS;
volatile unsigned int *readAddr;
volatile unsigned int *writeAddr;


void PrintParameters(void)
{
    printf("Test parameters for %s \n",(client) ?  "client" : "server" );
    printf("----------------------------\n\n");
    printf("Local adapter no.     : %d\n",localAdapterNo);
    printf("Local nodeId.         : %d\n",localNodeId);
    printf("Segment size          : %d\n",segmentSize);
    printf("My Rank               : %d\n",rank);
    printf("RM SegmentId          : %d\n",segmentId);
    printf("Number of nodes in RM : %d\n",nodes);
    printf("----------------------------\n\n");
}

// ************************************************************************************* 
sci_error_t dolphin_init(void)
{
    /* Initialize the SISCI library */
    SCIInitialize(NO_FLAGS, &error);
    if (error != SCI_ERR_OK) {
        fprintf(stderr,"SCIInitialize failed - Error code: 0x%x\n",error);
        return(-1);
    }

    /* Open a file descriptor */
    SCIOpen(&sd,NO_FLAGS,&error);
    if (error != SCI_ERR_OK) {
        if (error == SCI_ERR_INCONSISTENT_VERSIONS) {
            fprintf(stderr,"Version mismatch between SISCI user library and SISCI driver\n");
        }
        fprintf(stderr,"SCIOpen failed - Error code 0x%x\n",error);
        return(-1); 
    }

    /* Get local nodeId */
    SCIGetLocalNodeId(localAdapterNo,
                      &localNodeId,
                      NO_FLAGS,
                      &error);

    if (error != SCI_ERR_OK) {
        fprintf(stderr,"Could not find the local adapter %d\n", localAdapterNo);
        SCIClose(sd,NO_FLAGS,&error);
        SCITerminate();
        return(-1);
    }

    /*
     * Set remote nodeId to BROADCAST NODEID
     */
    remoteNodeId = DIS_BROADCAST_NODEID_GROUP_ALL;

    /* Print parameters */
    PrintParameters();

    /* 
     * The segmentId paramter is used to set the reflective memory group id 
     * when the flag SCI_FLAG_BROADCAST is specified. 
     *
     * For Dolphin Express DX, the reflective memory group id is limited to 0-5
     * For Dolphin Express IX, the reflective memory group id is limited to 0-3
     *
     * All nodes within the broadcast group must have the same segmentId to communicate.
     */

    /* Create local reflective memory segment */    
    SCICreateSegment(sd,&localSegment,segmentId, segmentSize, NO_CALLBACK, NULL, SCI_FLAG_BROADCAST, &error);

    if (error == SCI_ERR_OK) {
        printf("Local segment (id=0x%x, size=%d) is created. \n", segmentId, segmentSize);  
    } else {
        fprintf(stderr,"SCICreateSegment failed - Error code 0x%x\n",error);
        return error;
    }

    /* Prepare the segment */
    SCIPrepareSegment(localSegment,localAdapterNo, SCI_FLAG_BROADCAST, &error); 
    
    if (error == SCI_ERR_OK) {
        printf("Local segment (id=0x%x, size=%d) is prepared. \n", segmentId, segmentSize);  
    } else {
        fprintf(stderr,"SCIPrepareSegment failed - Error code 0x%x\n",error);
        return error;
    }

    /* Map local segment to user space - this is the address to read back data from the reflective memory region */
    readAddr = SCIMapLocalSegment(localSegment,&localMap, offset,segmentSize, NULL,NO_FLAGS,&error);
    if (error == SCI_ERR_OK) {
        printf("Local segment (id=0x%x) is mapped to user space at 0x%lx\n", segmentId,(unsigned long)readAddr); 
    } else {
        fprintf(stderr,"SCIMapLocalSegment failed - Error code 0x%x\n",error);
        return error;
    } 

    /* Set the segment available */
    SCISetSegmentAvailable(localSegment, localAdapterNo, NO_FLAGS, &error);
    if (error == SCI_ERR_OK) {
        printf("Local segment (id=0x%x) is available for remote connections. \n", segmentId); 
    } else {
        fprintf(stderr,"SCISetSegmentAvailable failed - Error code 0x%x\n",error);
        return error;
    } 

    /* Connect to remote segment */
    printf("Connect to remote segment .... ");

    do { 
        SCIConnectSegment(sd,
                          &remoteSegment,
                          remoteNodeId,
                          segmentId,
                          localAdapterNo,
                          NO_CALLBACK,
                          NULL,
                          SCI_INFINITE_TIMEOUT,
                          SCI_FLAG_BROADCAST,
                          &error);

        sleep(1);

    } while (error != SCI_ERR_OK);

    int remoteSize = SCIGetRemoteSegmentSize(remoteSegment);
    printf("Remote segment (id=0x%x) is connected with size %d.\n", segmentId,remoteSize);

    /* Map remote segment to user space */
    writeAddr = SCIMapRemoteSegment(remoteSegment,&remoteMap,offset,segmentSize,NULL,SCI_FLAG_BROADCAST,&error);
    if (error == SCI_ERR_OK) {
        printf("Remote segment (id=0x%x) is mapped to user space. \n", segmentId);         
    } else {
        fprintf(stderr,"SCIMapRemoteSegment failed - Error code 0x%x\n",error);
        return error;
    } 

    /* Create a sequence for data error checking*/ 
    SCICreateMapSequence(remoteMap,&sequence,NO_FLAGS,&error);
    if (error != SCI_ERR_OK) {
        fprintf(stderr,"SCICreateMapSequence failed - Error code 0x%x\n",error);
        return error;
    }

    /* The reflective memory functionality is operational at this point. */
    printf(" END OF DOLPHIN INIT ************************************* \n");
    sleep(1);
	return(0);
}
sci_error_t dolphin_closeout()
{
    /* Remove the Sequence */
    SCIRemoveSequence(sequence,NO_FLAGS, &error);
    if (error != SCI_ERR_OK) {
        fprintf(stderr,"SCIRemoveSequence failed - Error code 0x%x\n",error);
        return error;
    }
    
    /* Unmap local segment */
    SCIUnmapSegment(localMap,NO_FLAGS,&error);
    
    if (error == SCI_ERR_OK) {
        printf("The local segment is unmapped\n"); 
    } else {
        fprintf(stderr,"SCIUnmapSegment failed - Error code 0x%x\n",error);
        return error;
    }
    
    /* Remove local segment */
    SCIRemoveSegment(localSegment,NO_FLAGS,&error);
    if (error == SCI_ERR_OK) {
        printf("The local segment is removed\n"); 
    } else {
        fprintf(stderr,"SCIRemoveSegment failed - Error code 0x%x\n",error);
        return error;
    } 
    
    /* Unmap remote segment */
    SCIUnmapSegment(remoteMap,NO_FLAGS,&error);
    if (error == SCI_ERR_OK) {
        printf("The remote segment is unmapped\n"); 
    } else {
        fprintf(stderr,"SCIUnmapSegment failed - Error code 0x%x\n",error);
        return error;
    }
    
    /* Disconnect segment */
    SCIDisconnectSegment(remoteSegment,NO_FLAGS,&error);
    if (error == SCI_ERR_OK) {
        printf("The segment is disconnected\n"); 
    } else {
        fprintf(stderr,"SCIDisconnectSegment failed - Error code 0x%x\n",error);
        return error;
    } 

    // Close out Dolphing connection and exit
    if (error!= SCI_ERR_OK) {
        fprintf(stderr,"SCIClose failed - Error code: 0x%x\n",error);
        SCITerminate();
        return(-1);
    }

    /* Close the file descriptor */
    SCIClose(sd,NO_FLAGS,&error);
    if (error != SCI_ERR_OK) {
        fprintf(stderr,"SCIClose failed - Error code: 0x%x\n",error);
        SCITerminate();
        return(-1);
    }

    /* Free allocated resources */
    SCITerminate();

    return SCI_ERR_OK;
}
