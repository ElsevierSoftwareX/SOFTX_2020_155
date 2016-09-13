typedef struct DX_INFO {
	sci_desc_t              sd;
	sci_local_segment_t     localSegment;
	sci_remote_segment_t    remoteSegment;
	sci_map_t               localMap;
	sci_map_t               remoteMap;
	unsigned int 		segmentId;
	unsigned int 		localNodeId;
	unsigned int 		localAdapterNo;
	unsigned int            segmentSize;
	sci_sequence_t        	sequence;
	unsigned int		mode;
} DX_INFO;

sci_error_t dx_init(DX_INFO *);
volatile void *dx_attach_local_segment(DX_INFO *);
volatile void *dx_attach_remote_segment(DX_INFO *);
sci_error_t dx_cleanup(DX_INFO *);

#define DX_SEND_ONLY		0
#define DX_RCVR_ONLY		1
#define DX_SND_AND_RCV		2
