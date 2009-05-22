#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "alarm.h"
#include "callback.h"
#include "cvtTable.h"
#include "dbDefs.h"
#include "dbAccess.h"
#include "recGbl.h"
#include "recSup.h"
#include "devSup.h"
#include "link.h"
#include "dbCommon.h"
#include "aiRecord.h"
#include "biRecord.h"
#include "boRecord.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include "dscud.h"


/* Create the dset for devAiTestAsyn */
static long init_record();
static long read_ai();
static long read_bi();
static long write_bo();
struct {
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	read_ai;
	DEVSUPFUN	special_linconv;
}devAiAthena={
	6,
	NULL,
	NULL,
	init_record,
	NULL,
	read_ai,
	NULL};

#if 0
struct {
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	read_ai;
	DEVSUPFUN	special_linconv;
}devBiAthena={
	6,
	NULL,
	NULL,
	init_record,
	NULL,
	read_bi,
	NULL};

struct {
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	read_ai;
	DEVSUPFUN	special_linconv;
}devBoAthena={
	6,
	NULL,
	NULL,
	init_record,
	NULL,
	write_bo,
	NULL};
#endif


static BYTE result;  // returned error code
static ERRPARAMS errorParams;  // structure for returning error code and error         string
static DSCB dscb;    // handle used to refer to the board
static DSCCB dsccb;  // structure containing board settings
static DSCADSETTINGS dscadsettings; // structure containing A/D conversion settings
static DSCSAMPLE sample;       // sample reading


static long init_record(pai)
    struct aiRecord	*pai;
{
  return(0);
}


static long read_ai(pai)
    struct aiRecord	*pai;
{
  struct vmeio *pvmeio;
  pvmeio = (struct vmeio *)&(pai->inp.value);
  if (pvmeio->signal >= 0 && pvmeio->signal < 16) {
	  //sprintf(buf,DevName, pvmeio->signal);
  } else {
	  fprintf(stderr, "Athena: Invalid channel number %d in AI record\n", pvmeio->signal);
	  exit(2);
  }
    if( dscInit( DSC_VERSION ) != DE_NONE )
    {
	dscGetLastError(&errorParams);
	fprintf( stderr, "dscInit error: %s %s\n", dscGetErrorString(errorParams.ErrCode), errorParams.errstring );
	return 1;
    }
    dsccb.base_address = 0x280;
    dsccb.int_level = 5;
    if(dscInitBoard(DSC_PROM, &dsccb, &dscb)!= DE_NONE)
    {
	dscGetLastError(&errorParams);
	fprintf( stderr, "dscInit error: %s %s\n", dscGetErrorString(errorParams.ErrCode), errorParams.errstring );
	return 1;
    }
  memset(&dscadsettings, 0, sizeof(DSCADSETTINGS));

  dscadsettings.range = RANGE_10;
  dscadsettings.polarity = BIPOLAR;
  dscadsettings.gain = GAIN_1;
  dscadsettings.load_cal = 0;

  dscadsettings.current_channel = pvmeio->signal;
  if ((result = dscADSetSettings( dscb, &dscadsettings ) ) != DE_NONE )
   {
       dscGetLastError(&errorParams);
       fprintf( stderr, "dscADSetSettings error: %s %s\n", dscGetErrorString(errorParams.ErrCode), errorParams.errstring );    
       exit(2);
    }

  //printf ("0x%x\n", &dscb);
  if ((result = dscADSample (dscb, &sample)) != DE_NONE)
  {
	dscGetLastError(&errorParams);
	fprintf( stderr, "dscADSample error: %s %s\n", dscGetErrorString(errorParams.ErrCode), errorParams.errstring ); 
	exit(2);
  }
 //printf("%d\n", sample);
  pai->rval = sample;
 //printf("%d\n", sample);
  dscFreeBoard(dscb);
  dscFree();
  return 0;
}
		
static long write_bo(pai)
    struct boRecord	*pai;
{
#if 0
  char *DevName = "/dev/pci-das08_adc00";
  int fdADC;                   /* A/D file descriptors */
  struct vmeio *pvmeio;

  pvmeio = (struct vmeio *)&(pai->out.value);
  if (pvmeio->signal >= 0 && pvmeio->signal < 4) {
		;
  } else {
	  fprintf(stderr, "DAS08: Invalid bit number %d in BO record\n", pvmeio->signal);
	  exit(2);
  }

#ifdef DAS_LOCKING
  lockf (lock_fd, F_LOCK, 0);
#endif
  if (( fdADC = open(DevName, ADC_SOFT_TRIGGER)) < 0 ) {
    printf("error opening device %s\n", DevName);
    exit(2);
  }

  if (pai->rval & 1) val |=  1 << (pvmeio->signal);
  else val &= ~(1 << (pvmeio->signal));

  ioctl(fdADC, ADC_SET_DIO, val);

  printf("%d\n", val);

  close(fdADC);
#ifdef DAS_LOCKING
  lockf (lock_fd, F_ULOCK, 0);
#endif

#endif
  return(0);
}

static long read_bi(pai)
    struct biRecord	*pai;
{
#if 0
  char *DevName = "/dev/pci-das08_adc00";
  int fdADC;                   /* A/D file descriptors */
  struct vmeio *pvmeio;
  unsigned short val;

  pvmeio = (struct vmeio *)&(pai->inp.value);
  if (pvmeio->signal >= 0 && pvmeio->signal < 3) {
		;
  } else {
	  fprintf(stderr, "DAS08: Invalid bit number %d in BI record\n", pvmeio->signal);
	  exit(2);
  }

#ifdef DAS_LOCKING
  lockf (lock_fd, F_LOCK, 0);
#endif
  if (( fdADC = open(DevName, ADC_SOFT_TRIGGER)) < 0 ) {
    printf("error opening device %s\n", DevName);
    exit(2);
  }

  ioctl(fdADC, ADC_GET_DIO, &val);

  close(fdADC);
#ifdef DAS_LOCKING
  lockf (lock_fd, F_ULOCK, 0);
#endif

  pai->rval = (val >> pvmeio->signal) & 1;
#endif
  return(0);
}
