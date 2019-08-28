// Symmertricom GPS input card
// model BC635PCI-U
typedef struct SYMCOM_REGISTER {
  unsigned int TIMEREQ;
  unsigned int EVENTREQ;
  unsigned int UNLOCK1;
  unsigned int UNLOCK2;
  unsigned int CONTROL;
  unsigned int ACK;
  unsigned int MASK;
  unsigned int INTSTAT;
  unsigned int MINSTRB;
  unsigned int MAJSTRB;
  unsigned int EVENT2_0;
  unsigned int EVENT2_1;
  unsigned int TIME0;
  unsigned int TIME1;
  unsigned int EVENT0;
  unsigned int EVENT1;
  unsigned int RESERV1;
  unsigned int UNLOCK3;
  unsigned int EVENT3_0;
  unsigned int EVENT3_1;
} SYMCOM_REGISTER;

#define SYMCOM_VID 0x12e2
#define SYMCOM_BC635_TID 0x4013
#define SYMCOM_BC635_TIMEREQ 0
#define SYMCOM_BC635_EVENTREQ 4
#define SYMCOM_BC635_CONTROL 0x10
#define SYMCOM_BC635_TIME0 0x30
#define SYMCOM_BC635_TIME1 0x34
#define SYMCOM_BC635_EVENT0 0x38
#define SYMCOM_BC635_EVENT1 0x3C
#define SYMCOM_RCVR 0x1
