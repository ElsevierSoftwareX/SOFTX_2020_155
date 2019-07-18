#ifndef IX_STREAM_DOLPHIN_COMMON_HH
#define IX_STREAM_DOLPHIN_COMMON_HH

#ifdef __cplusplus
#include <stdexcept>
#include <stdio.h>
#endif

#include "sisci_types.h"
#include "sisci_api.h"
#include "sisci_error.h"
#include "sisci_demolib.h"
#include "testlib.h"

#define NO_CALLBACK         NULL
#define NO_FLAGS            0
#define DATA_TRANSFER_READY 8
#define CMD_READY           1234
#define IX_BLOCK_SIZE		0x800000
#define IX_BLOCK_COUNT		4

/*
 * Remote nodeId:
 *
 * DIS_BROADCAST_NODEID_GROUP_ALL is a general broadcast remote nodeId and
 * must be used as the remote nodeId in SCIConnectSegment() function
 */

extern "C" {

extern sci_error_t error;
extern sci_desc_t sd;
extern sci_local_segment_t localSegment;
extern sci_remote_segment_t remoteSegment;
extern sci_map_t localMap;
extern sci_map_t remoteMap;
extern sci_sequence_t sequence;
extern unsigned int localAdapterNo;
extern unsigned int remoteNodeId;
extern unsigned int localNodeId;
extern unsigned int segmentId;
extern unsigned int segmentSize;
extern unsigned int offset;
extern unsigned int client;
extern unsigned int server;
extern unsigned int *localbufferPtr;
extern int rank;
extern int nodes;
extern unsigned int memcpyFlag;
extern volatile unsigned int *readAddr;
extern volatile unsigned int *writeAddr;

extern sci_error_t dolphin_init(void);
extern sci_error_t dolphin_closeout();
};

#ifdef __cplusplus

/**
 * Quick class to manage dolphin, mainly to make sure that the closeout call always happens
 */
class simple_dolphin {
public:
    simple_dolphin()
    {
        sci_error_t err = dolphin_init();
        if (err != SCI_ERR_OK)
        {
            char buf[100];
            snprintf(buf, sizeof(buf), "dolphin error %d", static_cast<int>(err));
            throw std::runtime_error(buf);
        }
    };

    ~simple_dolphin()
    {
        dolphin_closeout();
    };
private:
    simple_dolphin(const simple_dolphin& other);
    simple_dolphin& operator=(const simple_dolphin& other);
};

#endif /* __cplusplus */

#endif //  IX_STREAM_DOLPHIN_COMMON_HH