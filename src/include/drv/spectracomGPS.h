// Spectracom GPS input card

#define TSYNC_VID 0x1ad7
#define TSYNC_TID 0x8000
#define TSYNC_SEC 0x1
#define TSYNC_USEC 0x2
#define TSYNC_RCVR 0x2
typedef struct TSYNC_REGISTER {
  unsigned int SUPER_SEC_LOW;  // Host Super-Sec Low + Mid Low
  unsigned int SUPER_SEC_HIGH; // Host Super-Sec Mid High + High
  unsigned int SUB_SEC;        // Sub-Second Low + High
  unsigned int BCD_SEC;        // Super Second binary time
  unsigned int BCD_SUB_SEC;    // Sub Second binary time
} TSYNC_REGISTER;
