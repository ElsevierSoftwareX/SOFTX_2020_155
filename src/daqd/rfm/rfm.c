/*
 *------------------------------------------------------------------------
 *                       COPYRIGHT NOTICE
 *
 *       Copyright (C) 2000 VMIC
 *       International copyright secured.  All rights reserved.
 *------------------------------------------------------------------------
 *      rfm.c 1.22 5/12/00 15:50:00
 *------------------------------------------------------------------------
 *      PCIbus RFM Device Driver For Solaris-2.X
 *------------------------------------------------------------------------
 * NOTE: Throughout this driver there is what could appear to be nonsense
 * redundant code after every write to one of the 5588's device registers.
 * This is not a mistake.  There is a non-flushable hardware writeback
 * cache inside the SPARC-10 (and maybe others, for that matter) that may
 * delay or even reorder the sequence of writes to device registers.  To
 * avoid this crock, Sun advises that every write of a register should be
 * immediately read back (the results may be discarded); when the cache
 * detects a read of a location which has a write pending in the cache, the
 * write will be forced out and then the register's contents will be read
 * back.
 *--------------------------------------------------------------------------
 */

#ifndef	lint
static char rfm_c_sccs_id[] = "@(#)rfm.c 1.22 00/05/12 VMIC";
#endif

#include </usr/include/sys/types.h>			/* Must be first system include	*/
#include <sys/debug.h>			/* For ASSERT()			*/
#include <sys/file.h>			/* For FKIOCTL			*/
#include <sys/pci.h>			/* Get PCIbus-related stuff	*/

#include <sys/cmn_err.h>
#include <sys/conf.h>
#include <sys/errno.h>
#include <sys/modctl.h>
#include <sys/open.h>
#include <sys/stat.h>
#include <sys/systm.h>
#include <sys/kmem.h>
#include <sys/varargs.h>		/* Need this for xxlog()	*/

#include <sys/ddi.h>			/* Must be penultimate include	*/
#include <sys/sunddi.h>			/* Must be last system include	*/

#include <rfm_reg.h>
#include <rfm_io.h>

#if !defined(_LP64)                    /* needed for 2.7 and later timeout */
#if !defined(SOL2_7_32BIT)             /* timeout stuff                    */
typedef int  timeout_id_t; 
#endif
#endif


#include "debugnames.h"			/* Names of DEBUG flags	        */
#include "eventnames.h"			/* Names of RFM events		*/
#include "regnames.h"			/* Names of RFM registers	*/


#define CLEAR_AND_DISABLE_DMA_INTERRUPTS	0x003C1000
#define DISABLE_DMA_INTERRUPTS			0x00001000
#define CLEAR_RFM_AND_ENABLE_DMA_INTERRUPTS	0x0002D000

/*
 *------------------------------------------------------------------------
 * Configuration constants
 *------------------------------------------------------------------------
 */

#define RFM_PATIENCE	        2	/* Event timeout (seconds)	 */
#define	FIXME			1	/* Include provisional code	 */
#define	NOPE			0	/* Exclude provisional code	 */

/*
 *------------------------------------------------------------------------
 * Macros to win friends and influence people
 *------------------------------------------------------------------------
 */

#define USECONDS	1000000UL	    /* Microseconds per second	 */
#define SEC2USEC(t) 	( (t) * USECONDS )  /* Seconds to microseconds	 */
#define	SEC2TICKS(t) 	drv_usectohz( SEC2USEC((t)) )

#define	WHENDEBUG(x)	if( (x) & rfmDebug )
#define	EVENT2MASK(e)	(1 << (e))	    /* Convert eventId to mask	 */

#define	LOCK_ECB(t) 	mutex_enter( &((t)->ecb_mutex) )
#define	UNLOCK_ECB(t)	mutex_exit(  &((t)->ecb_mutex) )

#if 0
static uint_t		rfmDebug = {RFM_DBRING|RFM_DBINTR|RFM_DBERROR|RFM_DBPROBE|RFM_DBMMAP}; /* Debug level */
#endif
static uint_t		rfmDebug = {RFM_DBERROR}; /* Debug level */

static const char	*me = "rfm5565";	          /* Generic driver name */
static const char	enteringMsg[] = { "TRACE - Entering %s" };
static const char	 leavingMsg[] = { "TRACE - Leaving %s --> %d" };

/*
 * Global variables to manage driver load and unload
 */

static volatile int	busy;		/* Counts attached devices	 */
static kmutex_t		busy_mu;	/* 'busy' access control	 */

/*
 *------------------------------------------------------------------------
 * Jim Linick - Ring Buffer Information
 *------------------------------------------------------------------------
 */

#define RINGBUFFER_EMPTY 256
#define RINGBUFFER_FULL  257
#define RINGBUFFER_OK    258

#define RINGBUF_SIZE 513

typedef struct
{
	uchar_t ringBuf[ RINGBUF_SIZE ];
	ushort_t putPtr;
	ushort_t takePtr;
	uint_t eventNo;
} ringBuffer;

ushort_t ringBufHandler( ringBuffer *, uint_t, uchar_t);


/* Each access map uses the same characteristics; here they are */

static ddi_device_acc_attr_t access_attr =
{
	DDI_DEVICE_ATTR_V0,		/* Boilerplate value		*/
	DDI_STRUCTURE_LE_ACC,	        /* Little-endian 		*/
	DDI_STRICTORDER_ACC		/* Don't reorder accesses	*/
};

/* Each DMA map uses the same characteristics; here they are */

static ddi_dma_attr_t attributes =
{
	DMA_ATTR_V0,		/* Version number		 	*/
	0x00000000,		/* Low address			 	*/
	0xFFFFFFFF,		/* High address			 	*/
	RFMOR_DMA_MAX,		/* Largest DMA count (mask)		*/
	4,			/* DMA object alignment		 	*/
	0x1FC,			/* DMA burst sizes		 	*/
	RFMOR_DMA_MIN,		/* Minimum DMA transfer		 	*/
	RFMOR_DMA_MAX,		/* Maximum DMA transfer		 	*/
	0xFFFFFFFF,		/* Segment register limitation	        */
	1,			/* Scatter/gather list length	        */
	4,			/* DMA granularity			*/
	0			/* Flags (reserved, must be 0)	        */
};

/*
 *------------------------------------------------------------------------
 * Interrupt callback routines get invoked for RFM interrupt events
 *------------------------------------------------------------------------
 */

typedef void (*callback_t)( void * );   /* Actually called w/ECB        */

/*
 *------------------------------------------------------------------------
 * Shape of event control block
 *------------------------------------------------------------------------
 */

#define	ECB_FLAGS_MUTEX	(1<<0)		/* MUTEX has been initialized	 */
#define	ECB_FLAGS_CV	(1<<1)		/* CV has been initialized	 */
#define	ECB_FLAGS_WAIT	(1<<2)		/* There is interest in this one */
#define	ECB_FLAGS_TELL	(1<<3)		/* Notify process of event	 */
#define	ECB_FLAGS_TIMO	(1<<4)		/* Timer has expired		 */

typedef struct ecb_s
{
	ushort_t	ecb_eventId;	/* Internal code for event	 */
	ushort_t	ecb_flags;	/* Timer control block flags	 */
	struct ucb_s	*ecb_ucb;	/* Make UCB easy to find	 */
	                                /* Information on the timeout	 */
	timeout_id_t	ecb_timeoutId;	/* Timeout ID (or -1 if idle)	 */
	clock_t		ecb_duration;	/* Length of timeout (or 0)	 */
	                                /* Thread synchronization stuff	 */
	kmutex_t	ecb_mutex;	/* Interrupt world guard	 */
	kcondvar_t	ecb_cv; 	/* Used to await event		 */
	                                /* How to handle the event	 */
	callback_t	ecb_callback;
	                                /* What to tell user processes   */
	uchar_t 	ecb_senderId;	/* What RFM node sent event	 */
	uint_t	 	ecb_eventMsg;	/* Message with event		 */
	uint_t		ecb_signal;	/* What signal to send		 */
	uint_t		ecb_nEvents;	/* How many events we've had	 */
	void		*ecb_userProc;	/* Process awaiting event	 */
} ecb_t, *ECB;

/*
 *------------------------------------------------------------------------
 * Shape of per-instance unit control block (UCB)
 *------------------------------------------------------------------------
 */

#define	RFMNAMELEN       64                 /* Length of minor device name  */
#define	MAXMSGLEN       256                 /* Max debug message length     */

#define UCB_FLAGS_OPEN      (1U << 0)       /* Device is opened		 */
#define UCB_FLAGS_TIMEOUT   (1U << 1)       /* Device timeout occurred	 */
#define UCB_FLAGS_TCPIP     (1U << 2)       /* Has TCP/IP enabled	 */
#define UCB_FLAGS_DMAERROR  (1U << 3)       /* Last DMA had an error	 */
#define UCB_FLAGS_DMADONE   (1U << 4)       /* DMA transfer is complete	 */

#define UCB_RSRC_INTR       (1U << 0)	    /* Interrupt connected       */
#define UCB_RSRC_MUTEX      (1U << 1)       /* Mutex installed		 */
#define UCB_RSRC_CV         (1U << 2)       /* Mutex installed		 */
#define UCB_RSRC_RFM        (1U << 3)       /* Dev's regs mapped	 */
#define UCB_RSRC_RFMOR      (1U << 4)       /* controls sand status regs mapped */
#define UCB_RSRC_MINOR      (1U << 5)       /* Device nodes created 	 */
#define UCB_RSRC_DMAHDL     (1U << 6)	    /* DMA handle allocated	 */
#define UCB_RSRC_RUNTIME    (1U << 7)	    /* runtime regs mapped */


typedef struct ucb_s
{
	dev_info_t	*ucb_dip;           /* Backlink to dev_info           */
	rfmGetConfig_t	ucb_rgc;	    /* Device configuration info      */
	uchar_t		ucb_canDma;         /* Able to DMA if true	      */
	kmutex_t	ucb_mu;             /* For exclusive access to H/W    */
	kcondvar_t	ucb_cv;             /* H/W (DMA) condition variable   */
	ddi_iblock_cookie_t  ucb_iblock_cookie;  /* Intr		      */
	ddi_idevice_cookie_t ucb_idevice_cookie; /* Devs		      */
	ddi_acc_handle_t     ucb_rfmor_accHndl;	 /* RFMOR data access	      */
	ddi_acc_handle_t     ucb_runtime_accHndl; /* Runtime regs access handle */
	ddi_acc_handle_t     ucb_rfm_accHndl;	 /* RFM data access           */
	ushort_t	     ucb_flags;		 /* Activity flags            */
	ushort_t	ucb_rsrc;		 /* Attached resources	      */
	dev_t		ucb_dev;	         /* Device major/minor id's   */
	uchar_t 	ucb_lun;	         /* Logical unit number device*/
	RFM		ucb_rfm;	         /* Addr of board's RFM memory */
	RFMOR		ucb_rfmor;	         /* Control and Status Registers */
	unsigned int	*ucb_runtime_regs;	 /* Runtime Regsiters */
	char		ucb_msg[ MAXMSGLEN ];	 /* Local message buffer      */
	rfmDmaInfo_t	ucb_rdi;		 /* DMA threshold information */
	ecb_t		ucb_ecbs[ RFM_EVENTID_NEVENTS ]; /* Timer blocks      */
	                                         /* Support DMA & buffered I/O*/
	volatile ushort_t ucb_dmaBusy;	         /* True if DMA is active     */
	ddi_dma_handle_t  ucb_dmahandle;         /* DMA handle                */
	ddi_dma_cookie_t  ucb_dmacookie;	 /* DMA cookie                */
	off_t		  ucb_rfmOffset;	 /* Offset of DMA in RFM      */
	uint_t		  ucb_ncookies;		 /* Cookies per window        */
	int		  ucb_partial;		 /* DMA obj partially mapped  */
	int		  ucb_nwin;		 /* Number DMA windows for bp */
	int		  ucb_windex;		 /* Index of active DMA window*/
	off_t		  ucb_dmaOffset;	 /* Current window offset     */
	uint_t		  ucb_dmaLength;	 /* Current window length     */
	struct buf	 *ucb_bp;		 /* Active buffer for I/O     */
	ringBuffer	  ucb_ringBuf[3];	 /* Ring buffer structure     */
} ucb_t, *UCB;

#define UNULL	( (UCB) NULL )		         /* A UCB-typed null address  */

/* UCB flag management */

#define	has_TCPIP(u)	( (u)->ucb_flags & UCB_FLAGS_TCPIP )

/* Resource flag management */

#define	HAS_RESOURCE(u,r)	( (u)->ucb_rsrc &   (r) )
#define	ADD_RESOURCE(u,r)	  (u)->ucb_rsrc |=  (r)
#define	DEL_RESOURCE(u,r)	  (u)->ucb_rsrc &= ~(r)

/* Use our min() and max() definitions */

#undef	min
#undef	max

#define	min(x,y)	( ((x) < (y)) ? (x) : (y) )
#define	max(x,y)	( ((x) > (y)) ? (x) : (y) )

/*
 *------------------------------------------------------------------------
 * xxlog: format and display error messages (similar to cmn_err & printf)
 *------------------------------------------------------------------------
 * The first character of the format string is special: '!' messages go
 * only to the "putbuf"; '^' messages go only to the console; and '?'
 * always goes to the "putbuf" and to the console iff we were booted in
 * verbose mode.
 *------------------------------------------------------------------------
 * xxlog( UCB, const char *, ... );
 *------------------------------------------------------------------------
 */

static void xxlog
(
	UCB			ucb,
	const char	*fmt,
	...
)
{
	char		*text;		/* Which buffer we actually use	*/
	char		*bp;		/* Walks down output buffer	*/
	char		*buf;		/* Private messaging area	*/
	va_list 	ap;		/* Address of arg on stack	*/

	buf = (char *) NULL;	        /* No allocated buffer		*/
	va_start( ap, fmt );	        /* Start variable arguments	*/
	/*
         * Use message buffer in the UCB if we have one
         */
	if( ucb )
	{
		/* Use message area specific to this device */
		text = bp = (char *) ucb->ucb_msg;
	}
	else
	{
		/* Don't have a UCB, so use our own private area */
		if( (buf = kmem_alloc( MAXMSGLEN, KM_NOSLEEP )) != (char *) NULL )
		{
			/* We got a dynamic buffer */
			text = bp = buf;
		}
		else
		{
			/* Well, use a fixed one if nothing else */
			static char	lastResort[ MAXMSGLEN ];
			text = bp = lastResort;
		}
	}

	/* At this point, 'buf' is non-zero if we've allocated memory!! */
	/* The first character of message is special to cmn_err(9F)	*/
	switch( fmt[0] & 0xFF )
	{
		case '!':			/* syslog only			*/
		case '^':			/* console only 		*/
		case '?':			/* syslog and verbose console	*/
			*bp++ = *fmt++;
			break;
		default:
			break;
	}

	/* Prepend "rfm" and optional LUN number to message */
	if( ucb )
	{
		/* I know the logical unit number! */
		(void) sprintf( (char *) bp, "%s%d: ", me, ucb->ucb_lun );
	}
	else
	{
		/* This is an anonymous message */
		(void) sprintf( (char *) bp, "%s:  ", me );
	}

	while( *bp ) ++bp;	           /* Advance to NULL at end */
	vsprintf( bp, fmt, ap );           /* Output user's text     */
	va_end( ap );                      /* Complete variable arg  */
	cmn_err( CE_CONT, "%s.\n", text ); /* Write to console       */

	/*
	 * Allow message to reach the syslogd() if we must. We do this by
	 * blindly waiting about 0.25 seconds after each message. This
	 * should work OK, even if we happen to be in an interrupt
	 * routine; some random process will be delayed, but who cares?
	 * Our interrupt handlers take care to clear this flag while the
	 * are running.
	 */
	if( rfmDebug & RFM_DBSLOW )
	{
		delay(drv_usectohz( USECONDS / 20 ));
	}

	/* Free the dynamic buffer if we allocated one */
	if( buf )
	{
		kmem_free( buf, MAXMSGLEN );
	}
}

/*
 *------------------------------------------------------------------------
 * xxprint: allow kernel to display message about exception in driver
 *------------------------------------------------------------------------
 */

static int xxprint
(
	dev_t		dev,	/* Complex device number	 */
	char		*msg	/* Message to be printed	 */
)
{
	cmn_err( CE_CONT, "rfm: xxprint( '%s' )\n", msg );
	xxlog( UNULL, "%s", msg );
}

/*
 *------------------------------------------------------------------------
 * Unit Control Blocks (aka soft state info) and other driver-related
 * global storage
 *------------------------------------------------------------------------
 */

static void *ucbs;	/* Global data */

/*
 *------------------------------------------------------------------------
 * xxidentify: called to verify that we're the driver for this device
 *------------------------------------------------------------------------
 * N. B.: this routine is not needed after Solaris-2.5.1, but we supply
 * it for backwards compatibility.  If it causes you trouble, just replace
 * its entry in the 'cb_ops' array with 'nodev'.
 *------------------------------------------------------------------------
 */

static int xxidentify
(
	dev_info_t	*dip
)
{
	/* Establish invariant information			 */
	static const char	*names[] =
	{
		"pci114a,5588",		/* A guess		 */
		"pci114a,5576",		/* Another guess	 */
		"pci114a,5579",		/* Another guess	 */
		"pci114a,5587",		/* Another guess	 */
		"pci10b5,9656",         /* vmic 5565 board    */
		(char *) NULL		/* List terminator	 */
	};

	char	*dev_name = ddi_get_name(dip);
	                                /* Local variables, if any  */
	const char	**list;	        /* Walks down name list     */
	const char	*name;	        /* Name under consideration */

	/*
         * Compare the device name against what we think is should be
         */

	for( list = names; (name = *list) != NULL; ++list )
	{
            WHENDEBUG( RFM_DBPROBE )
	    {
	       xxlog( UNULL, "compare '%s' to my '%s' device", dev_name, name );
	    }

	    if( strcmp(dev_name, name) == 0 )
	    {
	       WHENDEBUG( RFM_DBPROBE )
	       {
		  xxlog( UNULL, "ah-ha; my long-lost %s device", name );
	       }
	       return( DDI_IDENTIFIED );
	    }
	}
	return( DDI_NOT_IDENTIFIED );
}

/*
 *------------------------------------------------------------------------
 * xxprobe: check device
 *------------------------------------------------------------------------
 */

static int xxprobe
(
	dev_info_t	*dip	/* Device info pointer */
)
{
   WHENDEBUG( RFM_DBPROBE )
      xxlog( UNULL, "probing (%s) unit=%d", ddi_get_name( dip ),
                                        ddi_get_instance( dip ));

    if( ddi_dev_is_sid( dip ) == DDI_SUCCESS )
    {
       WHENDEBUG( RFM_DBPROBE ) xxlog( UNULL, "self-identifying device" );
       return( DDI_PROBE_DONTCARE );
    }

    WHENDEBUG( RFM_DBPROBE ) xxlog( UNULL, "not self-identifying" );
    return( DDI_FAILURE );
}

/*
 *------------------------------------------------------------------------
 * xxgetinfo: a pretty generic getinfo routine as described in the manual.
 *------------------------------------------------------------------------
 */

/*ARGSUSED*/
static int xxgetinfo
(
	dev_info_t		*dip,
	ddi_info_cmd_t	infocmd,
	void			*arg,
	void			**result
)
{
	/*
         * Establish invariant information, Local variables, if any
         */
	int		status; 	/* Results returned to caller */
	UCB		ucb;		/* Per-device storage area    */

	switch (infocmd)
	{
		case DDI_INFO_DEVT2DEVINFO:
			ucb = (UCB) ddi_get_soft_state( ucbs, getminor( (dev_t) arg ) );
			if( ucb == NULL)
			{
				/* Device hasn't been instantiated yet */
				*result = NULL;
				status = DDI_FAILURE;
			}
			else
			{
				/*
			 	 * Don't need to use a MUTEX even though we are
			 	 * accessing our instance structure; ucb->dip
			 	 * never changes.
			 	 */
				*result = ucb->ucb_dip;
				status = DDI_SUCCESS;
			}
			break;
		case DDI_INFO_DEVT2INSTANCE:
			*result = (void *) getminor( (dev_t) arg );
			status = DDI_SUCCESS;
			break;
		default:
			*result = NULL;
			status = DDI_FAILURE;
	}
	return( status );
}

/*
 *------------------------------------------------------------------------
 * Strncasecmp: case-independent counted string compare (not in kernel!)
 *------------------------------------------------------------------------
 */

static int Strncasecmp
(
	const char	*s1,	/* Left string              */
	const char	*s2,	/* Right string             */
	size_t		n	/* Number of chars to count */
)
{
	/*
         * Establish invariant information, Local variables, if any
         */
	int		status;
	unsigned char	s1c;	/* Left string character	*/
	unsigned char	s2c;	/* Right string character	*/

	for( status = 0; n-- != 0; )
	{
		/*
                 * Fetch next characters from the strings
                 */
		s1c = *s1++ & 0xFF;
		s2c = *s2++ & 0xFF;

		/*
                 * Force to uppercase
                 */
		if( (s1c >= 'a') && (s1c <= 'z') ) s1c -= ('a' - 'A');
		if( (s2c >= 'a') && (s2c <= 'z') ) s2c -= ('a' - 'A');

		/*
                 * Get out if characters are not the same
                 */
		if( (status = s2c - s1c) ) break;

		/*
                 * We're done if we're at the end of both strings
                 */
		if( !s1c ) break;

	}
	return( (!status ? 0 : ((status < 0) ? -1 : 1)) );
}

/*
 *------------------------------------------------------------------------
 * eventControlBlock_ctor: timer control block constructor
 *------------------------------------------------------------------------
 */

static void eventControlBlock_ctor
(
	UCB		ucb,		/* Per-device storage		*/
	ECB		ecb,		/* Timer control block to init	*/
	ushort_t	eventId,	/* Event to initialize		*/
	clock_t		duration,	/* Default duration		*/
	uchar_t		myNode		/* Our RFM node ID		*/
)
{
	/* Establish invariant information	*/
	char	*name = kmem_alloc( RFMNAMELEN, KM_SLEEP );
	/* Local variables, if any			*/

	bzero( (caddr_t) ecb, sizeof( *ecb ) );
	ecb->ecb_eventId	= eventId;
	ecb->ecb_ucb 		= ucb;
	ecb->ecb_timeoutId 	= (timeout_id_t)-1;
	ecb->ecb_duration 	= duration;
	ecb->ecb_callback	= (callback_t) NULL;
	sprintf( name, "rfm,emutex,%u", eventId );

	mutex_init( (kmutex_t *) &ecb->ecb_mutex, name, MUTEX_DRIVER,
		                              ucb->ucb_iblock_cookie );

	ecb->ecb_flags |= ECB_FLAGS_MUTEX;
	sprintf( name, "rfm,ecv,%u", eventId );

	cv_init( (kcondvar_t *) &ecb->ecb_cv,
		name,		/* Name of resource	 */
		CV_DRIVER,	/* Class of resource	 */
		NULL		/* Must be NULL in drvrs */
	);

	ecb->ecb_flags |= ECB_FLAGS_CV;
	ecb->ecb_senderId = myNode;	/* Right for local events only	 */
	WHENDEBUG( RFM_DBTIMER )
	{
		xxlog( ucb, "event id %u timer created (duration=%d)", eventId, duration );
	}
	
	kmem_free( name, RFMNAMELEN );  /* Release dynamic memory */
}

/*
 *------------------------------------------------------------------------
 * killTimer: turn off an active RFM event timer
 *------------------------------------------------------------------------
 */

static void killTimer
(
	ECB		ecb		/* Timer control block	*/
)
{
	/*
         * Establish invariant information, Local variables, if any
         */
	UCB		ucb = ecb->ecb_ucb;
	timeout_id_t	timeoutId;

	if( (timeoutId = ecb->ecb_timeoutId) != (timeout_id_t)-1 )
	{
		ecb->ecb_timeoutId = (timeout_id_t)-1;
		untimeout( timeoutId );
		WHENDEBUG( RFM_DBTIMER )
		{
		   xxlog( ucb, "event %u timer killed", ecb->ecb_eventId );
		}
	}
}

/*
 *------------------------------------------------------------------------
 * eventControlBlock_dtor: timer control block destructor
 *------------------------------------------------------------------------
 */

static void eventControlBlock_dtor
(
	ECB		ecb		/* Timer control block	*/
)
{
	/* Establish invariant information		*/
	UCB		ucb = ecb->ecb_ucb;

	if( ecb->ecb_timeoutId != (timeout_id_t)-1 )
	{
		WHENDEBUG( RFM_DBTIMER )
		{
			xxlog( ucb, "destroying event timer %u with active timeout",
				ecb->ecb_eventId );
		}
		untimeout( ecb->ecb_timeoutId );
		ecb->ecb_timeoutId 	= (timeout_id_t)-1;
	}
	if( ecb->ecb_userProc )
	{
		WHENDEBUG( RFM_DBTIMER )
		{
			xxlog( ucb, "user process still attached" );
		}
		proc_unref( ecb->ecb_userProc );
		ecb->ecb_userProc;
	}
	if( ecb->ecb_flags & ECB_FLAGS_MUTEX )
	{
		mutex_destroy( &ecb->ecb_mutex);
		ecb->ecb_flags &= ~ECB_FLAGS_MUTEX;
	}
	if( ecb->ecb_flags & ECB_FLAGS_CV )
	{
		cv_destroy( &ecb->ecb_cv );
		ecb->ecb_flags &= ~ECB_FLAGS_CV;
	}
	WHENDEBUG( RFM_DBTIMER )
	{
		if( ecb->ecb_flags )
		{
			xxlog( ucb, "event Id %u timer control flags are 0x%X",
				ecb->ecb_flags );
		}
	}
	bzero( (caddr_t) ecb, sizeof( *ecb ) );
}

/*
 *------------------------------------------------------------------------
 * timedout: signal that the operation has timed out
 *------------------------------------------------------------------------
 */

static void timedout
(
	caddr_t		arg		/* Really ECB address	*/
)
{
	/* Establish invariant information			*/
	ECB		ecb = (ECB) arg;
	UCB		ucb = ecb->ecb_ucb;

	WHENDEBUG( RFM_DBTIMER ) xxlog( ucb, "operation timedout" );

	LOCK_ECB( ecb );
	   ecb->ecb_timeoutId = (timeout_id_t)-1;
	   ecb->ecb_flags |= ECB_FLAGS_TIMO;
	   cv_broadcast( &ecb->ecb_cv );
	UNLOCK_ECB( ecb );
}

/*
 *------------------------------------------------------------------------
 * disableInterrupts: disable RFM interrupts
 *------------------------------------------------------------------------
 * Caveat Programmae: we assume that the RFM is locked
 *------------------------------------------------------------------------
 */

static void disableInterrupts
(
	UCB		ucb		/* Per-device storage		*/
)
{
    	unsigned int *regs = (unsigned int*)(ucb -> ucb_rfmor);
	uchar_t		icsr;		/* Interrupt control/status	*/
	uint_t		intcsr;		/* Interrupt status		*/

	WHENDEBUG( RFM_DBINTR )  xxlog( ucb, "disabling RFM interrupts" );

        mutex_enter( &ucb->ucb_mu );
	{
		unsigned int flushCache; /* ???? */

	        /* Disable local interrupts */
        	regs[0x14/4] &= ~7;
        	flushCache = regs[0x14/4];

        	/* Global interrupts disable */
        	regs[0x10/4] &= ~0x4000;
        	flushCache = regs[0x10/4];

        	/* PCI interrupts disable */
        	ucb -> ucb_runtime_regs[0x68/4] &= ~0x900;
        	flushCache = ucb -> ucb_runtime_regs[0x68/4];
	
#if 0
	   /*
            * Disable RFM interrupts
            */
	   rfm->rfm_icsr &= ~RFM_ICSR_GLOBALENABLE;
	   icsr = rfm->rfm_icsr;	  /* flush cache	*/
           /*
            * Disable interrupt across PCI
            */
	   ddi_putl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_intcsr,0x0);
	   intcsr = ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_intcsr); /* flush cache	*/
#endif
	}
        mutex_exit( &ucb->ucb_mu );
}

/*
 *------------------------------------------------------------------------
 * enableInterrupts: enable RFM interrupts
 *------------------------------------------------------------------------
 * Caveat Programmae: this routine assumes that the RFM is already locked
 *------------------------------------------------------------------------
 */

static void enableInterrupts
(
	UCB		ucb,		/* Per-device storage	*/
	int		dmaFlags	/* DMA enable flags	*/
)
{
    unsigned int *regs = (unsigned int*)(ucb -> ucb_rfmor);
    uchar_t	senderId;
    uchar_t 	icsr;
    uint_t	intcsr;

    WHENDEBUG( RFM_DBINTR ) xxlog( ucb, "enabling RFM interrupts" );

    mutex_enter( &ucb->ucb_mu );
    {
      /* clean the FIFOs */
	*(unsigned char*)(&regs[0x24/4]) = 0;
	*(unsigned char*)(&regs[0x2C/4]) = 0;
	*(unsigned char*)(&regs[0x34/4]) = 0;

      	/* Enable local interrupts */
      	regs[0x14/4] |= 7;

	/* Global interrupts enable */
	regs[0x10/4] = 0x4000;

	/* PCI interrupts enable */
	ucb -> ucb_runtime_regs[0x68/4] |= 0x900;

#if 0
	/*
         * Clean all pending RFM event interrupts
         */
	rfm->rfm_sid1 = 0;
        senderId=rfm->rfm_sid1;                 /* Flush cache		*/
	rfm->rfm_sid2 = 0;
        senderId=rfm->rfm_sid2;                 /* Flush cache		*/
	rfm->rfm_sid3 = 0;
        senderId=rfm->rfm_sid3;                 /* Flush cache		*/

	/* Enable RFM interrupts */
	rfm->rfm_icsr = ( RFM_ICSR_GLOBALENABLE	| RFM_ICSR_BDATA	|
		          RFM_ICSR_RCVHALF	| RFM_ICSR_NODERESET	|
		          RFM_ICSR_INT3PENDING	| RFM_ICSR_INT2PENDING	|
		          RFM_ICSR_INT1PENDING  );
	icsr = rfm->rfm_icsr;			/* Flush cache		*/
	/*
         * Enable interrupt across the PCI bus
         */
	ddi_putl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_intcsr,RFMOR_INTCSR_ENABLEINT);
	intcsr = ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_intcsr);	        /* Flush cache	        */
#endif
    }
    mutex_exit( &ucb->ucb_mu );
}

/*
 *------------------------------------------------------------------------
 * xxdetach: clean up and free resources allocated by xxattach
 *------------------------------------------------------------------------
 * In general, this routine frees the resources in the reverse order from 
 * the sequence used by 'xxattach()'.
 *
 * The table below shows the step number, as assigned by 'xxattach()'
 * in the order that we do them here.
 *+
 * Step 7: destroy DMA handle
 * Step 6: destroy the minor device node
 * Step 5: delete the interrupt handler.
 * Step 4: destroy the CV
 * Step 3: destroy the MUTEX that controls the CV
 * Step 2: unmap the device registers and RFM memory
 * Step 1: unmap the PCIbus operation registers
 *+
 *------------------------------------------------------------------------
 */

static int xxdetach
(
	dev_info_t	*dip,	/* Device info pointer		*/
	ddi_detach_cmd_t cmd	/* Function to perform		*/
)
{
	/* Establish invariant information			*/
	/* Local variables, if any				*/
	int		results;	/* Results of operation	*/

	results = DDI_FAILURE;	        /* Pessimistic assumption */
	switch( cmd )
	{
		case DDI_DETACH:
		{
			/* Establish invariant information  */
			/* Local variables, if any	    */
			UCB		ucb;
			int		instance;

			instance = ddi_get_instance(dip);
			ucb = (UCB) ddi_get_soft_state(ucbs, instance);
			if( ucb == UNULL )
			{
				WHENDEBUG( RFM_DBERROR )
				{
				 xxlog( UNULL, "cannot detach null UCB" );
				}
				break;
			}
			WHENDEBUG( RFM_DBPROBE )
			{
				xxlog( ucb, "detaching" );
			}
			/*
                         * Turn off device (disable interrupts)
                         */
			disableInterrupts( ucb );

			/*
			 *------------------------------------------------
			 * Step 7: destroy the DMA handle
			 *------------------------------------------------
			 */
			if( HAS_RESOURCE( ucb, UCB_RSRC_DMAHDL ) )
			{
				WHENDEBUG( RFM_DBPROBE )
				{
					xxlog( ucb, "destroying DMA handle" );
				}
				ddi_dma_free_handle( &ucb->ucb_dmahandle );
				DEL_RESOURCE( ucb, UCB_RSRC_DMAHDL );
			}

			/*
			 *------------------------------------------------
			 * Step 6: destroy the minor device nodes
			 *------------------------------------------------
			 */
			if( HAS_RESOURCE( ucb, UCB_RSRC_MINOR ) )
			{
				WHENDEBUG( RFM_DBPROBE )
				{
					xxlog( ucb, "removing minor nodes" );
				}
				ddi_remove_minor_node( ucb->ucb_dip, NULL );
				DEL_RESOURCE( ucb, UCB_RSRC_MINOR );
			}

			/*
			 *------------------------------------------------
			 * Step 5: delete the interrupt handler.
			 *------------------------------------------------
			 */
			if( HAS_RESOURCE( ucb, UCB_RSRC_INTR ) )
			{
				ddi_remove_intr( ucb->ucb_dip, 0, ucb->ucb_iblock_cookie );
				DEL_RESOURCE( ucb, UCB_RSRC_INTR );
			}
			/*
			 *------------------------------------------------
			 * Step 4: destroy the CV
			 *------------------------------------------------
			 */
			if( HAS_RESOURCE( ucb, UCB_RSRC_CV ) )
			{
				WHENDEBUG( RFM_DBPROBE )
				{
					xxlog( ucb, "destroying cv" );
				}
				cv_destroy( &ucb->ucb_cv );
				DEL_RESOURCE( ucb, UCB_RSRC_CV );
			}
			/*
			 *------------------------------------------------
			 * Step 3: destroy the MUTEX that controls the CV
			 *------------------------------------------------
			 */
			if( HAS_RESOURCE( ucb, UCB_RSRC_MUTEX ) )
			{
				WHENDEBUG( RFM_DBPROBE )
				{
					xxlog( ucb, "removing rmutex" );
				}
				mutex_destroy( &ucb->ucb_mu );
				DEL_RESOURCE( ucb, UCB_RSRC_MUTEX );
			}

			/*
			 *------------------------------------------------
			 * Step 2: unmap the device registers and RFM memory
			 *------------------------------------------------
			 */
			if( HAS_RESOURCE( ucb, UCB_RSRC_RFM ) )
			{
				WHENDEBUG( RFM_DBPROBE )
				{
					xxlog( ucb, "unmapping device" );
				}
				ddi_regs_map_free( &ucb->ucb_rfm_accHndl );
				DEL_RESOURCE( ucb, UCB_RSRC_RFM );
			}

			/*
			 *------------------------------------------------
			 * Step 1: unmap the PCIbus operation registers
			 *------------------------------------------------
			 */
			if( HAS_RESOURCE( ucb, UCB_RSRC_RFMOR ) )
			{
				WHENDEBUG( RFM_DBPROBE )
				{
					xxlog( ucb, "unmapping controls and status registers" );
				}
				ddi_regs_map_free( &ucb->ucb_rfmor_accHndl );
				DEL_RESOURCE( ucb, UCB_RSRC_RFMOR );
			}

			/* Unmap runtime registers */
			if( HAS_RESOURCE( ucb, UCB_RSRC_RUNTIME ) )
			{
				WHENDEBUG( RFM_DBPROBE )
				{
					xxlog( ucb, "unmapping runtime registers" );
				}
				ddi_regs_map_free( &ucb->ucb_runtime_accHndl );
				DEL_RESOURCE( ucb, UCB_RSRC_RUNTIME );
			}
#if 0
			/* Safety check any other assigned resources	 */
			if( ucb->ucb_rsrc != 0 )
			{
				WHENDEBUG( RFM_DBERROR )
				{
				  xxlog( ucb, "residual resources = %b", ucb->ucb_rsrc,
						"\020"	 /* As HEX	 */
				  #include "resources.h" /* AUTOMATICALLY GENERATED */
					);
				}
			}
			WHENDEBUG( RFM_DBPROBE )
			{
				xxlog( ucb, "leaving kernel memory" );
			}

#endif
			/* Destroy the UCB */
			ddi_soft_state_free( ucbs, instance );
			ucb = UNULL;
			/* Grab mutex, forget this device */
			mutex_enter( &busy_mu );
			--busy;
			mutex_exit( &busy_mu );
			/* Fini! */
			results = DDI_SUCCESS;
		}
		break;
		default:
			WHENDEBUG( RFM_DBERROR )
			{
				xxlog( UNULL, "unsupported detach command (%d)", cmd );
			}
	}
	return( results );
}

/*
 *------------------------------------------------------------------------
 * useDmaCookie: start a DMA transfer, given information in a cookie
 *------------------------------------------------------------------------
 * Returns which DMA interrupt needs enabling
 *------------------------------------------------------------------------
 */

static ushort_t useDmaCookie
(
	UCB		ucb		/* Per-device local storage	*/
)
{
	/*
         * Establish invariant information, Local variables, if any
         */
	RFM		  rfm = ucb->ucb_rfm;
	RFMOR		rfmor = ucb->ucb_rfmor;

	ushort_t	enables;	/* Which DMA interrupt was used	 */
	ushort_t	junk;		/* Gotta have a place for it	 */
	uint_t		intcsr;

	/*
	 *================================================================
	 * Program the DMA engine
	 *----------------------------------------------------------------
	 * Here we use a sneaky-programmer trick: by loading both the DMA
	 * read and DMA write transfer count (but by enabling only one of
	 * them to count) we have the original DMA count in an
	 * easy-to-find place so that we can calculate the residual DMA
	 * count when the transfer terminates. This trick also helps us
	 * keep the DMA transfer counters non-zero, so they don't generate
	 * interrupts when they are not being used.
	 *----------------------------------------------------------------
	 * Don't forget to read back the RFM/RFMOR registers to flush the
	 * cache!
	 *================================================================
	 */
	if( (ucb->ucb_bp->b_flags & B_WRITE) != 0 )
	{
	   /*
	    * For a "write(2)" system call, the DMA engine on the RFM
	    * must do a READ transfer.
	    */
	   WHENDEBUG( RFM_DBSTRAT ) xxlog( ucb, "using write(2) cookie for DMA read" );

           mutex_enter( &ucb->ucb_mu );

		ucb -> ucb_runtime_regs[0x80/4] = 0x205C3; /* set MODE & enable DONE interrupt */
		ucb -> ucb_runtime_regs[0x84/4] = ucb->ucb_dmacookie.dmac_address; /* PCI starting address */
		ucb -> ucb_runtime_regs[0x88/4] = ucb->ucb_rfmOffset;
		ucb -> ucb_runtime_regs[0x8c/4] = ucb->ucb_dmacookie.dmac_size;
		ucb -> ucb_runtime_regs[0x90/4] = 0; /* PCI to local */
		ucb -> ucb_runtime_regs[0x68/4] |= 0x40900;
		ucb -> ucb_runtime_regs[0xA8/4] = 0x0003;

#if 0
		ddi_putl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_mcsr,RFMOR_MCSR_PCIRFIFO);
		junk = ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_mcsr);
	        WHENDEBUG( RFM_DBSTRAT ) xxlog( ucb, "rfmor_mcsr    = 0x%04X",ddi_getl(ucb->ucb_rfmor_accHndl,
                                                                                       (uint32_t *) &rfmor->rfmor_mcsr) );

		ddi_putl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_mrar,ucb->ucb_dmacookie.dmac_address);
		junk = ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_mrar);
	        WHENDEBUG( RFM_DBSTRAT ) xxlog( ucb, "rfmor_mrar    = 0x%04X",ddi_getl(ucb->ucb_rfmor_accHndl,
                                                                                       (uint32_t *) &rfmor->rfmor_mrar) );

		ddi_putl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_mrtc,ucb->ucb_dmacookie.dmac_size);
		junk = ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_mrtc);
	        WHENDEBUG( RFM_DBSTRAT ) xxlog( ucb, "rfmor_mrtc    = 0x%04X",ddi_getl(ucb->ucb_rfmor_accHndl,
                                                                                       (uint32_t *) &rfmor->rfmor_mrtc) );

		rfm->rfm_dmaoff   = ucb->ucb_rfmOffset;
		junk =   rfm->rfm_dmaoff;
	        WHENDEBUG( RFM_DBSTRAT ) xxlog( ucb, "rfm_dmaoff    = 0x%04X",rfm->rfm_dmaoff );

		rfm->rfm_csr1     = RFM_CSR1_DMAREAD;
		junk =   rfm->rfm_csr1;
	        WHENDEBUG( RFM_DBSTRAT ) xxlog( ucb, "rfm_csr1      = 0x%04X",rfm->rfm_csr1 );

		intcsr = ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_intcsr) & RFMOR_INTCSR_ENABLEINT;
		ddi_putl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_intcsr,RFMOR_INTCSR_ENABLERTC | intcsr);
		junk = ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_intcsr);

		ddi_putl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_mcsr,RFMOR_MCSR_RTE);
		junk = ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_mcsr);
	        WHENDEBUG( RFM_DBSTRAT ) xxlog( ucb, "rfmor_mcsr    = 0x%04X",ddi_getl(ucb->ucb_rfmor_accHndl,
                                                                                       (uint32_t *) &rfmor->rfmor_mcsr) );

#endif

           mutex_exit( &ucb->ucb_mu );
	}
	else
	{
	   /*
            * For a "read(2)" system call, the DMA engine on the RFM
	    * must do a WRITE transfer.
	    */
	   WHENDEBUG( RFM_DBSTRAT ) xxlog( ucb, "using read(2) cookie for DMA write" );

           mutex_enter( &ucb->ucb_mu );

		ucb -> ucb_runtime_regs[0x80/4] = 0x205C3; /* set MODE & enable DONE interrupt */
		ucb -> ucb_runtime_regs[0x84/4] = ucb->ucb_dmacookie.dmac_address; /* PCI starting address */
		ucb -> ucb_runtime_regs[0x88/4] = ucb->ucb_rfmOffset;
		ucb -> ucb_runtime_regs[0x8c/4] = ucb->ucb_dmacookie.dmac_size;
		ucb -> ucb_runtime_regs[0x90/4] = 8; /* local to PCI */
		ucb -> ucb_runtime_regs[0x68/4] |= 0x40900;
		ucb -> ucb_runtime_regs[0xA8/4] = 0x0003;
#if 0
		ddi_putl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_mcsr,RFMOR_MCSR_PCIWFIFO);
		junk = ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_mcsr);
	        WHENDEBUG( RFM_DBSTRAT ) xxlog( ucb, "rfmor_mcsr    = 0x%04X",ddi_getl(ucb->ucb_rfmor_accHndl,
                                                                                       (uint32_t *) &rfmor->rfmor_mcsr) );

		ddi_putl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_mwar,ucb->ucb_dmacookie.dmac_address );
		junk = ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_mwar);
	        WHENDEBUG( RFM_DBSTRAT ) xxlog( ucb, "rfmor_mwar    = 0x%04X",ddi_getl(ucb->ucb_rfmor_accHndl,
                                                                                       (uint32_t *) &rfmor->rfmor_mwar) );

		ddi_putl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_mwtc,ucb->ucb_dmacookie.dmac_size);
		junk = ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_mwtc);
                WHENDEBUG( RFM_DBSTRAT ) xxlog( ucb, "rfmor_mwtc    = 0x%04X",ddi_getl(ucb->ucb_rfmor_accHndl,
                                                                                       (uint32_t *) &rfmor->rfmor_mwtc) );

		rfm->rfm_dmaoff   = ucb->ucb_rfmOffset;
		junk =   rfm->rfm_dmaoff;
	        WHENDEBUG( RFM_DBSTRAT ) xxlog( ucb, "rfm_dmaoff    = 0x%04X", rfm->rfm_dmaoff );

		rfm->rfm_csr1     = RFM_CSR1_DMAWRITE;
		junk =   rfm->rfm_csr1;
	        WHENDEBUG( RFM_DBSTRAT ) xxlog( ucb, "rfm_csr1      = 0x%04X",rfm->rfm_csr1 );

		intcsr = ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_intcsr) & RFMOR_INTCSR_ENABLEINT;
		ddi_putl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_intcsr, RFMOR_INTCSR_ENABLEWTC | intcsr);
		junk = ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_intcsr);

		ddi_putl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_mcsr, RFMOR_MCSR_WTE);
		junk = ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_mcsr);
	        WHENDEBUG( RFM_DBSTRAT ) xxlog( ucb, "rfmor_mcsr    = 0x%04X",ddi_getl(ucb->ucb_rfmor_accHndl,
                                                                                       (uint32_t *) &rfmor->rfmor_mcsr) );

#endif

           mutex_exit( &ucb->ucb_mu );
	}
	return( enables );
}

/*
 *------------------------------------------------------------------------
 * beginDmaTransfer: begin a DMA transfer
 *------------------------------------------------------------------------
 */

static int beginDmaTransfer
(
	UCB		ucb,		/* Per-device local information	 */
	struct buf	*bp		/* Buffer descriptor		 */
)
{
	/*
         * Establish invariant information
         */
	RFM	  rfm = ucb->ucb_rfm;
	RFMOR	rfmor = ucb->ucb_rfmor;
	/*
         * Local variables, if any
         */
	int		flags;		       /* DMA mapping flags	*/
	int		status;		       /* Mapping results	*/


        mutex_enter( &ucb->ucb_mu );
	   while( ucb->ucb_dmaBusy ) cv_wait( &ucb->ucb_cv, &ucb->ucb_mu );
	   ucb->ucb_dmaBusy = 1;	       /* Sole control of DMA resources */
	mutex_exit( &ucb->ucb_mu );

	ucb->ucb_bp = bp;
	if( ucb->ucb_bp->b_flags & B_WRITE )   /* Figure out the DMA direction */
	{
		flags = DDI_DMA_WRITE;
	}
	else
	{
		flags = DDI_DMA_READ;
	}
	flags |= DDI_DMA_PARTIAL;
	status = ddi_dma_buf_bind_handle(
		ucb->ucb_dmahandle,
		ucb->ucb_bp,			/* Buffer pointer		*/
		flags,				/* DMA direction flags		*/
		DDI_DMA_SLEEP,			/* Waits until resources avail	*/
		(caddr_t) NULL, 		/* Arg for callback routine	*/
		&ucb->ucb_dmacookie,	        /* DMA cookie address	 	*/
		&ucb->ucb_ncookies		/* Returned cookie count	*/
	);
	switch( status )
	{
		default:			/* No resources available	*/
			WHENDEBUG( RFM_DBERROR )
			{
			  xxlog( ucb, "ddi_dma_buf_bind_handle() returned 0x%X",
					(unsigned) status );
			}
			bioerror( ucb->ucb_bp, EIO );
                        
	                /* Maybe someone else wants the DMA channel we grabbed */
                        mutex_enter( &ucb->ucb_mu );
	                     ucb->ucb_dmaBusy = 0;
	                     cv_broadcast( &ucb->ucb_cv );
                        mutex_exit( &ucb->ucb_mu );
	                /* All done with RFM hardware */
	                return( -1 );

		case DDI_DMA_MAPPED:		/* Mapped the whole buffer	*/
			ucb->ucb_partial = 0;
			break;
		case DDI_DMA_PARTIAL_MAP:	/* Got some of it		*/
			ucb->ucb_partial = 1;
			ucb->ucb_windex  = 0;
			break;
	}
	ucb->ucb_flags &= ~UCB_FLAGS_DMADONE;
	(void) useDmaCookie( ucb );

        mutex_enter( &ucb->ucb_mu );
	   while( (ucb->ucb_flags & UCB_FLAGS_DMADONE) == 0 )
	   {
		 cv_wait( &ucb->ucb_cv, &ucb->ucb_mu );
	   }

#if 0
	   while( (ucb -> ucb_runtime_regs[0xA8/4] & 0x11) != 0x11)
	   {
		delay(1);
	   }

	  /* This lines are to be done in the xxintr() DMA service */
          ddi_dma_unbind_handle( ucb->ucb_dmahandle );
          ucb->ucb_dmaBusy = 0;
#endif

        mutex_exit ( &ucb->ucb_mu );

	return( 0 );
}

/*
 *------------------------------------------------------------------------
 * xxintr: interrupt handler
 *------------------------------------------------------------------------
 * Caveat Programmae: this routine might be called before the RFM registers
 * have been mapped if this system daisy chains interrupt handlers.  If 
 * the RFM pointer is null, no register map has been allocated.
 *------------------------------------------------------------------------
 */

typedef struct hwInterruptInfo_s
{
	ushort_t	hii_eventId;	/* RFM_EVENT_foo		 */
	uchar_t		hii_irsFlag;	/* IRS register indicator flag	 */
	uchar_t		hii_sidOffset;	/* Reg offset to get sender Id	 */
} hwInterruptInfo_t, *HWINTERRUPTINFO;

/* All IRS flags we're interested in					 */
static uchar_t		irsAny = (
	RFM_IRS_INT1	|
	RFM_IRS_INT2	|
	RFM_IRS_INT3	|
	RFM_IRS_RESET	|
	RFM_IRS_BDATA
);

const static hwInterruptInfo_t	hiis[] =
{
	{ RFM_EVENTID_A,	RFM_IRS_INT1,	RFM_REGOFFSET(rfm_sid1)	},
	{ RFM_EVENTID_B,	RFM_IRS_INT2,	RFM_REGOFFSET(rfm_sid2)	},
	{ RFM_EVENTID_C,	RFM_IRS_INT3,	RFM_REGOFFSET(rfm_sid3)	},
	{ RFM_EVENTID_RESET,	RFM_IRS_RESET,	RFM_REGOFFSET(rfm_nid)	},
	{ RFM_EVENTID_BDATA,	RFM_IRS_BDATA,	RFM_REGOFFSET(rfm_nid)	}
};

static const int	Nhiis = sizeof(hiis) / sizeof(hiis[0]);

static u_int xxintr
(
	caddr_t		arg		/* UCB in disguise		*/
)
{
   /*
    *  Establish invariant information & Local variables, if any
    */
   UCB		ucb = (UCB) arg;
   unsigned int *regs = (unsigned int*)(ucb -> ucb_rfmor);
   RFM		rfm;
   RFMOR	rfmor;
   uint_t	checkThese;	/* INTCSR bits of interest	*/
   uint_t	intcsr; 	/* Local copy of register	*/
   uint_t	imb1;		/* Mailbox to clear PCI ireq	*/
   uchar_t	irs;		/* Local copy of register	*/
   uchar_t	csr1;		/* Local copy of register	*/
   ECB		ecb;		/* Event control block          */
   ushort_t	enables;	/* We expect DMA to intr later	*/
   ushort_t	junk;		/* Gotta have some place for it */

   uchar_t	icsr;		/* Local copy of register	*/
   uint_t	myLevel;
   ushort_t	rtnVal;

   /*
    * We must have a UCB, the registers must be mapped, and the
    * device must be opened before this could be our interrupt.
    */

   if( !ucb || !ucb->ucb_rfm || !(ucb->ucb_flags & UCB_FLAGS_OPEN) )
   {
	return( DDI_INTR_UNCLAIMED );
   }

   rfmor  = ucb->ucb_rfmor;
   rfm    = ucb->ucb_rfm;

   checkThese = 208000; /* Local interrupt or DMA channel 0 interrupt */

#if 0
   if( ucb->ucb_canDma )
   {
	checkThese |= ( RFMOR_INTCSR_TABORT   | RFMOR_INTCSR_MABORT | 
			RFMOR_INTCSR_READDONE | RFMOR_INTCSR_WRITEDONE );
   }
#endif

   mutex_enter( &ucb->ucb_mu );
NotDoneYet:
#if 0
	intcsr = ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_intcsr);
        ddi_putl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_intcsr,(intcsr & checkThese));
        irs = rfm->rfm_irs;
	/*
         * Quickly check to see if this is our interrupt
         */
#endif
	intcsr = ucb -> ucb_runtime_regs[0x68/4];
	if( (intcsr & checkThese) == 0 )
	{
           WHENDEBUG( RFM_DBINTR )
             {
               xxlog( ucb,"NOT MY JOB! intcsr> 0x%X irs> 0x%X",intcsr,irs);
             }
#if 0
             ddi_putl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_intcsr,intcsr);
#endif
           mutex_exit( &ucb->ucb_mu );
	   return( DDI_INTR_UNCLAIMED );
	}

   /*
    * Service a DMA interrupt
    */
   if( ucb->ucb_canDma && (intcsr & 0x200000) ) { /* channel 0 DMA int */
      WHENDEBUG( RFM_DBINTR )
      {
	xxlog( ucb, "dma interrupt; intcsr> 0x%X", intcsr);
      }

	  ucb -> ucb_runtime_regs[0xA8/4] = 0x0008; /* Clear interrupt */
	  ucb -> ucb_runtime_regs[0x68/4] &= ~( 1 << 21 ); /* ??? */

          /*
           * Count interrupt event
           */
	  ecb = &ucb->ucb_ecbs[ RFM_EVENTID_DMA ];
          LOCK_ECB( ecb );
               ecb->ecb_nEvents++;
          UNLOCK_ECB( ecb );

#if 0

TODO: handle DMA error

          if( intcsr & ( RFMOR_INTCSR_TABORT | RFMOR_INTCSR_MABORT) )
          {
               WHENDEBUG( RFM_DBERROR ) xxlog( ucb, "DMA error" );
               bioerror( ucb->ucb_bp, EIO );
               ucb->ucb_partial = 0;
          }
#endif

          /*
           * Free the DMA handle; synchronizes caches automatically.
           */
          ddi_dma_unbind_handle( ucb->ucb_dmahandle );
          /*
           * Wake up all interested processes
           */
          ucb->ucb_dmaBusy = 0;
          ucb->ucb_flags |= UCB_FLAGS_DMADONE;
          cv_broadcast( &ucb->ucb_cv );
   }

#if 0
   /*
    * Service a DMA interrupt
    */
   if( ucb->ucb_canDma && (intcsr & ( RFMOR_INTCSR_TABORT    |
                                      RFMOR_INTCSR_MABORT    |
                                      RFMOR_INTCSR_READDONE  |
                                      RFMOR_INTCSR_WRITEDONE )) )
   {
      WHENDEBUG( RFM_DBINTR )
      {
	xxlog( ucb, "dma interrupt; intcsr> 0x%X", intcsr);
      }
          /*
           * Turn off bus mastership and internal DMA
           */
          ddi_putl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_mcsr,0x0);
          junk = ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_mcsr);      /* flush cache	*/
          csr1 = rfm->rfm_csr1;
          csr1 &= ~( RFM_CSR1_DMAWRITE | RFM_CSR1_DMAREAD);
          rfm->rfm_csr1 = csr1;
          csr1 = rfm->rfm_csr1;          /* flush cache	*/
          /*
           * Count interrupt event
           */
	  ecb = &ucb->ucb_ecbs[ RFM_EVENTID_DMA ];
          LOCK_ECB( ecb );
               ecb->ecb_nEvents++;
          UNLOCK_ECB( ecb );
          if( intcsr & ( RFMOR_INTCSR_TABORT | RFMOR_INTCSR_MABORT) )
          {
               WHENDEBUG( RFM_DBERROR ) xxlog( ucb, "DMA error" );
               bioerror( ucb->ucb_bp, EIO );
               ucb->ucb_partial = 0;
          }
          /*
           * Free the DMA handle; synchronizes caches automatically.
           */
          ddi_dma_unbind_handle( ucb->ucb_dmahandle );
          /*
           * Wake up all interested processes
           */
          ucb->ucb_dmaBusy = 0;
          ucb->ucb_flags |= UCB_FLAGS_DMADONE;
          cv_broadcast( &ucb->ucb_cv );
          /*
           * Turn off DMA interrupt enables & reset DMA status bits
           */
          intcsr &= ~( RFMOR_INTCSR_ENABLEWTC | RFMOR_INTCSR_ENABLERTC );
   }
#endif

   /* read sender ID and this will de-assert the int */
   icsr = regs[0x10/4];
   if (icsr & 7) {
     uchar_t     eventId = 0;
     uchar_t     pendingFlag = 0;
     uchar_t    senderId;
     callback_t callback;

     if (icsr & 1) {
	eventId = 0;
	senderId = *(((unsigned char *)regs) + 0x24);
        WHENDEBUG( RFM_DBINTR ) xxlog( ucb, "INT1 from node %d", senderId);
     } else if (icsr & 2) {
	eventId = 1;
	senderId = *(((unsigned char *)regs) + 0x2C);
        WHENDEBUG( RFM_DBINTR ) xxlog( ucb, "INT2 from node %d", senderId);
     } else  {
	eventId = 2;
	senderId = *(((unsigned char *)regs) + 0x34);
        WHENDEBUG( RFM_DBINTR ) xxlog( ucb, "INT3 from node %d", senderId);
     }
     ecb = &ucb->ucb_ecbs[ eventId ];
     LOCK_ECB( ecb );
         ecb->ecb_senderId = senderId;
         {
           /*
            * Put Fiber Int ON Ring Buffer
            */
           rtnVal = ringBufHandler( &(ucb->ucb_ringBuf[ eventId ]), 0, ecb->ecb_senderId);
           WHENDEBUG( RFM_DBRING )
            {
               xxlog( ucb, "Ring buffer put: rtnVal = %d", rtnVal);
            }
         }
         ecb->ecb_nEvents++;
         ecb->ecb_eventMsg = ~0;
         killTimer( ecb );
         if( (ecb->ecb_flags & ECB_FLAGS_TELL) && ecb->ecb_userProc )
         {
           /*
            * Handle notifications
            */
           WHENDEBUG( RFM_DBINTR )
           {
             xxlog( ucb, "event %u from node %u; %u total",
                    (unsigned) eventId,
                    (unsigned) ecb->ecb_senderId,
                    ecb->ecb_nEvents );
           }
           proc_signal( ecb->ecb_userProc, ecb->ecb_signal );
         }
         callback = ecb->ecb_callback;
         if( callback != (callback_t) NULL )
         {
            WHENDEBUG( RFM_DBINTR ) xxlog( ucb, "TCP/IP event" );
            (*callback)( ecb );
         }
         else
         {
            cv_signal( &ecb->ecb_cv );       /* Wake up sleepers */
         }
     UNLOCK_ECB( ecb );
   }

#if 0
   /*
    * Service an RFM interrupt event (if any)
    */
   if( (intcsr & RFMOR_INTCSR_INCOMING) != 0)
   {
     irs &= irsAny;
     while( irs )
     {
        /*
         * Process RFM event interrupts as long as any remain indicated
         */
        const struct hwInterruptInfo_s  *hii;
        const struct hwInterruptInfo_s  *lHii;
        int                              gotOne;

        WHENDEBUG( RFM_DBINTR ) xxlog( ucb, "irs=0x%02X", irs );
        /*
         * Process each interrupt we care about
         */
        gotOne = 0;
        for( hii = hiis, lHii = hii + Nhiis; hii < lHii; ++hii )
        {
            /*
             * Establish invariant information
             */
            uchar_t     eventId     = hii->hii_eventId;
            uchar_t     pendingFlag = hii->hii_irsFlag;
             /*
              * Local variables, if any
              */
             uchar_t    senderId;
             callback_t callback;

             if( (irs & pendingFlag) == pendingFlag )
             {
               /*
                * Get invariant stuff first then RFM node that sent event
                */
               uchar_t senderOffset = hii->hii_sidOffset;
               ++gotOne;
               ecb = &ucb->ucb_ecbs[ eventId ];
               LOCK_ECB( ecb );
                   ecb->ecb_senderId = rfm->rfm_ram[ senderOffset ];
                   if ( (eventId == RFM_EVENTID_A) ||
                        (eventId == RFM_EVENTID_B) ||
                        (eventId == RFM_EVENTID_C) )

                   {
                       /*
                        * Put Fiber Int ON Ring Buffer
                        */
                       rtnVal = ringBufHandler( &(ucb->ucb_ringBuf[ eventId ]), 0,
                                             ecb->ecb_senderId);
                       WHENDEBUG( RFM_DBRING )
                       {
                         xxlog( ucb, "Ring buffer put: rtnVal = %d", rtnVal);
                       }
                   }
                   ecb->ecb_nEvents++;
                   ecb->ecb_eventMsg = ~0;
                   killTimer( ecb );
                   if( (ecb->ecb_flags & ECB_FLAGS_TELL) && ecb->ecb_userProc )
                   {
                       /*
                        * Handle notifications
                        */
                       WHENDEBUG( RFM_DBINTR )
                       {
                         xxlog( ucb, "event %u from node %u; %u total",
                                      (unsigned) eventId,
                                      (unsigned) ecb->ecb_senderId,
                                                 ecb->ecb_nEvents );
                       }
                       proc_signal( ecb->ecb_userProc, ecb->ecb_signal );
                   }
                   callback = ecb->ecb_callback;
                   if( callback != (callback_t) NULL )
                   {
                       WHENDEBUG( RFM_DBINTR ) xxlog( ucb, "TCP/IP event" );
                       (*callback)( ecb );
                   }
                   else
                   {
                       cv_signal( &ecb->ecb_cv );       /* Wake up sleepers */
                   }
               UNLOCK_ECB( ecb );
               /*
                * Don't service this again this time
                */
               irs &= ~pendingFlag;
          } /* end_of_if( (irs & pendingFlag) == pendingFlag )              */
      }  /* end_of_for( hii = hiis, lHii = hii + Nhiis; hii < lHii; ++hii ) */

      if( !gotOne ) break;    /* Stop if no interrupts recognized this pass */
    } /* end_of_while() */

       	imb1 = ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_imb1);              /* read Mailbox first     */
        rfm->rfm_irs = 0;
        irs  = rfm->rfm_irs;                   /* flush cache	*/
        icsr  = rfm->rfm_icsr;
        icsr |= RFM_ICSR_GLOBALENABLE;
        rfm->rfm_icsr = icsr;
        icsr  = rfm->rfm_icsr;                 /* flush cache	*/
 }
#endif /* 0 */

#if 0
 /*
  *================================================================
  * preserve the DMA Write/Read enable bits and set Interrupt Enable
  * bit.
  *================================================================
  */
     if ( ucb->ucb_canDma )
     {
          if( (ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_mcsr) & RFMOR_MCSR_WTE) !=0 )
          {
               intcsr |= RFMOR_INTCSR_ENABLEWTC;
          }
          if( (ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_mcsr) & RFMOR_MCSR_RTE) !=0 )
          {
               intcsr |= RFMOR_INTCSR_ENABLERTC;
          }
     }
#endif

#if 0
     ddi_putl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_intcsr,intcsr);
     junk = ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_intcsr);          /* flush cache	*/

     if( (ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_intcsr) & (checkThese & ~(intcsr & checkThese)) ) !=0 )
     {
         WHENDEBUG( RFM_DBINTR )
         {
              xxlog( ucb,"rfmor_intcsr> 0x%X intcsr> 0x%X",
                                  ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_intcsr),intcsr);
         }
         goto NotDoneYet;
     }
#endif

  mutex_exit( &ucb->ucb_mu );

  return( DDI_INTR_CLAIMED );
}

/*
 *------------------------------------------------------------------------
 * xxattach: called once for each PCIbus RFM device in the system
 *------------------------------------------------------------------------
 */

static int xxattach
(
	dev_info_t 			*dip,	/* Device info node pointer	 */
	ddi_attach_cmd_t 	cmd		/* Function to be performed	 */
)
{
	/* Establish invariant information				 */
	/* Local variables, if any						 */
	UCB		ucb;		/* Per-device storage		 */
	int		results;	/* Results of this routine	 */
	int		instance;
        uint16_t        pci_command;

	results = DDI_FAILURE;	/* Pessimistic assumption */
	switch( cmd )
	{
		case DDI_ATTACH:
		{
			/* Establish invariant information	 */
			/* Local variables, if any			 */
			RFM		rfm;	/* Device registers	 */
                        uchar_t		csr3;	/* My copy of CSR3       */
			char	*rawname;
			ddi_acc_handle_t conf_handle;	/* Config space hdl */
			ushort_t	eventId;

			instance = ddi_get_instance( dip );
			WHENDEBUG( RFM_DBPROBE )
			{
				xxlog( UNULL, "attaching instance %d", instance );
			}
			/* We do not support hi-level interrupt handlers */
			if( ddi_intr_hilevel( dip, instance ) )
			{
				WHENDEBUG( RFM_DBERROR )
				{
					xxlog( UNULL, "cannot be high-level intr");
				}
				return( results );
			}
			/*
			 *================================================
			 * Create the UCB for this instance
			 *================================================
			 * After this point, you must abort via a jump to
			 * the 'BailOut' label, so allocated resources
			 * can be freed, if something goes wrong.
			 *================================================
			 */
			if( ddi_soft_state_zalloc(ucbs, instance) != 0)
			{
				WHENDEBUG( RFM_DBERROR )
				{
					xxlog( UNULL, "can't create UCB" );
				}
				return( results );
			}
			/* You did read the above comment, didn't you? */
			ucb = (UCB) ddi_get_soft_state( ucbs, instance );
			WHENDEBUG( RFM_DBPROBE )
			{
				xxlog( UNULL, "ucb address = 0x%x", ucb );
			}
			ucb->ucb_dip = dip;
			/* The instance is the LUN */
			ucb->ucb_lun = instance;
			WHENDEBUG( RFM_DBPROBE )
			{
				xxlog( ucb, "instance %d will henceforth be known as '%s%d'",
					instance, me, ucb->ucb_lun );
			}
			/*
			 *------------------------------------------------
			 * Map the config-space registers for a moment
			 *------------------------------------------------
			 */
			if( pci_config_setup( dip, &conf_handle ) != DDI_SUCCESS )
			{
				WHENDEBUG( RFM_DBERROR )
				{
					xxlog( ucb, "cannot access config registers" );
				}
				goto BailOut;
			}

#if 0
#define PCI_CONF_VENID          0x0     /* vendor id, 2 bytes */
#define PCI_CONF_DEVID          0x2     /* device id, 2 bytes */
#define PCI_CONF_COMM           0x4     /* command register, 2 bytes */
#define PCI_CONF_STAT           0x6     /* status register, 2 bytes */
#define PCI_CONF_REVID          0x8     /* revision id, 1 byte */
#define PCI_CONF_PROGCLASS      0x9     /* programming class code, 1 byte */
#define PCI_CONF_SUBCLASS       0xA     /* sub-class code, 1 byte */
#define PCI_CONF_BASCLASS       0xB     /* basic class code, 1 byte */
#define PCI_CONF_CACHE_LINESZ   0xC     /* cache line size, 1 byte */
#define PCI_CONF_LATENCY_TIMER  0xD     /* latency timer, 1 byte */
#define PCI_CONF_HEADER         0xE     /* header type, 1 byte */
#define PCI_CONF_BIST           0xF     /* builtin self test, 1 byte */
#define PCI_CONF_BASE0          0x10    /* base register 0, 4 bytes */
#define PCI_CONF_BASE1          0x14    /* base register 1, 4 bytes */
#define PCI_CONF_BASE2          0x18    /* base register 2, 4 bytes */
#define PCI_CONF_BASE3          0x1c    /* base register 3, 4 bytes */
#define PCI_CONF_BASE4          0x20    /* base register 4, 4 bytes */
#define PCI_CONF_BASE5          0x24    /* base register 5, 4 bytes */
#define PCI_CONF_CIS            0x28    /* Cardbus CIS Pointer */
#define PCI_CONF_SUBVENID       0x2c    /* Subsystem Vendor ID */
#define PCI_CONF_SUBSYSID       0x2e    /* Subsystem ID */
#define PCI_CONF_ROM            0x30    /* ROM base register, 4 bytes */
#define PCI_CONF_CAP_PTR        0x34    /* capabilities pointer, 1 byte */
#define PCI_CONF_ILINE          0x3c    /* interrupt line, 1 byte */
#define PCI_CONF_IPIN           0x3d    /* interrupt pin, 1 byte */
#define PCI_CONF_MIN_G          0x3e    /* minimum grant, 1 byte */
#define PCI_CONF_MAX_L          0x3f    /* maximum grant, 1 byte */
#endif
			ucb->ucb_rgc.rgc_vendorId = pci_config_get16( conf_handle, PCI_CONF_VENID );
			ucb->ucb_rgc.rgc_deviceId = pci_config_get16( conf_handle, PCI_CONF_DEVID );
			ucb->ucb_rgc.rgc_revId    = pci_config_get8( conf_handle, PCI_CONF_REVID );
			WHENDEBUG( RFM_DBPROBE )
			{
				unsigned int bar[6];
				unsigned int i;
				for (i = PCI_CONF_BASE0; i <=PCI_CONF_BASE5;
						i+=(PCI_CONF_BASE1-PCI_CONF_BASE0))
					bar[(i-PCI_CONF_BASE0)/(PCI_CONF_BASE1-PCI_CONF_BASE0)]
						= pci_config_get32( conf_handle, i);

				xxlog( ucb, "venid=%04X, devid=%04X, revid=%02X",
					ucb->ucb_rgc.rgc_vendorId, ucb->ucb_rgc.rgc_deviceId,
					ucb->ucb_rgc.rgc_revId );
				xxlog( ucb, "bar0=%04X, bar1=%04X, bar2=%04X, bar3=%04X, bar4=%04X, bar5=%04X",
					bar[0], bar[1], bar[2], bar[3], bar[4], bar[5] );
			}

                        /*
                         * Insure the I/O Space device, Memory Space, & Bus Master bits are set
                         * in the PCI command register.
                         */
                        pci_command = pci_config_get16(conf_handle, PCI_CONF_COMM);
                        pci_command |= ( PCI_COMM_IO | PCI_COMM_MAE | PCI_COMM_ME );
                        pci_config_put16(conf_handle, PCI_CONF_COMM, pci_command );


			pci_config_teardown( &conf_handle );

			/*
			 *------------------------------------------------
			 * Step 1:  Map Control and Status Registers
			 *------------------------------------------------
			 */
			if( ddi_regs_map_setup(
				dip,				/* Device Info Pointer	 */
				3,				/* Register set number	 */
				(caddr_t *) &ucb->ucb_rfmor,	/* Put addr of regs here */
				0,				/* Offset                */
				0x40,				/* Size of registers	 */
				&access_attr,			/* Access attributes	 */
				&ucb->ucb_rfmor_accHndl		/* Data access hdl       */
				) != DDI_SUCCESS )
			{
				WHENDEBUG( RFM_DBERROR )
				{
					xxlog( ucb, "cannot map controls and status registesr" );
				}
				goto BailOut;
			}
                        ADD_RESOURCE( ucb, UCB_RSRC_RFMOR );

			
			/* Map Runtime registers */
                        if( ddi_regs_map_setup(
                                dip,                            /* Device Info Pointer   */
                                1,                              /* Register set number   */
                                (caddr_t *) &ucb->ucb_runtime_regs, /* Put addr of regs here */
                                0,                              /* Offset                */
                                0x200,                           /* Size of registers     */
                                &access_attr,                   /* Access attributes     */
                                &ucb->ucb_runtime_accHndl       /* Data access hdl       */
                                ) != DDI_SUCCESS )
                        {
                                WHENDEBUG( RFM_DBERROR )
                                {
                                        xxlog( ucb, "cannot map runtime registers" );
                                }
                                goto BailOut;
                        }
                        ADD_RESOURCE( ucb, UCB_RSRC_RUNTIME );

#if 0

			/*
			 *------------------------------------------------
			 * Step 2: Map the device registers and RFM memory
			 *------------------------------------------------
			 */
			if( ddi_regs_map_setup(
				dip,				/* Device Info Pointer	*/
				2,			/* Register set number	*/
				(caddr_t *) &ucb->ucb_rfm,	/* Return addr here	*/
				0,				/* Offset		*/
				RFM_REGSIZ,			/* Size of registers	*/
				&access_attr,			/* Access attributes	*/
				&ucb->ucb_rfm_accHndl		/* Data access hdl	*/
				) != DDI_SUCCESS )
			{
				WHENDEBUG( RFM_DBERROR )
				{
					xxlog( ucb, "cannot device registers" );
				}
				goto BailOut;
			}
			ADD_RESOURCE( ucb, UCB_RSRC_RFM );
			rfm = ucb->ucb_rfm;

			/*
			 *------------------------------------------------
			 * At this point we've only got the first
			 * RFM_REGSIZ bytes of the memory space mapped.
			 * Look at the CSR3 register for an encoding of
			 * the actual memory size provisioned on the
			 * interface. Once we know that we can unmap the
			 * RFM_MEM_REGS space and remap it with the
			 * correct size.
			 *------------------------------------------------
			 */

                        if ( (ucb->ucb_rgc.rgc_deviceId == DEVICE_ID_PCI5579) ||
                             (ucb->ucb_rgc.rgc_deviceId == DEVICE_ID_PCI5587) )
                        {
                         csr3 = rfm->rfm_ecsr3;
                         csr3 &= (RFM_CSR3_CONFIG_MASK | 0x80);
                        } else if ( (ucb->ucb_rgc.rgc_deviceId == DEVICE_ID_PCI5565)) {
		         csr3 = RFM_CSR3_CONFIG_64M;
                        } else
                        {
                         csr3 = rfm->rfm_csr3;
                         csr3 &= RFM_CSR3_CONFIG_MASK;
                        }

			switch( csr3 )
			{
                          default:
                                  WHENDEBUG( RFM_DBERROR )
                                   xxlog( ucb, "cannot determine memory capacity (CSR3=0x%02X)",
                                                csr3 );
                                   ucb->ucb_rgc.rgc_ramSize = (256 * 1024);
                          break;

                          case RFM_CSR3_CONFIG_256K: ucb->ucb_rgc.rgc_ramSize =       (256 * 1024); break;
                          case RFM_CSR3_CONFIG_512K: ucb->ucb_rgc.rgc_ramSize =       (512 * 1024); break;
                          case RFM_CSR3_CONFIG_1M:   ucb->ucb_rgc.rgc_ramSize = ( 1 * 1024 * 1024); break;
                          case RFM_CSR3_CONFIG_2M:   ucb->ucb_rgc.rgc_ramSize = ( 2 * 1024 * 1024); break;
                          case RFM_CSR3_CONFIG_4M:   ucb->ucb_rgc.rgc_ramSize = ( 4 * 1024 * 1024); break;
                          case RFM_CSR3_CONFIG_8M:   ucb->ucb_rgc.rgc_ramSize = ( 8 * 1024 * 1024); break;
                          case RFM_CSR3_CONFIG_16M:  ucb->ucb_rgc.rgc_ramSize = (16 * 1024 * 1024); break;
                          case RFM_CSR3_CONFIG_NONE: ucb->ucb_rgc.rgc_ramSize = RFM_REGSIZ;         break;
                          case RFM_CSR3_CONFIG_32M:  ucb->ucb_rgc.rgc_ramSize = (32 * 1024 * 1024); break;
                          case RFM_CSR3_CONFIG_64M:  ucb->ucb_rgc.rgc_ramSize = (64 * 1024 * 1024); break;

			}

#endif
			ucb->ucb_rgc.rgc_ramSize = 64 * 1024 * 1024;

			WHENDEBUG( RFM_DBPROBE )
			{
				xxlog( ucb, "provisioned memory is %u (0x%X) bytes",
					(unsigned) ucb->ucb_rgc.rgc_ramSize,
					(unsigned) ucb->ucb_rgc.rgc_ramSize );
			}


#if	NOPE
			/*
			 *------------------------------------------------
			 * Note that each RFM device gobbles a *HUGE*
			 * chunk of MMU virtual memory slots. In fact,
			 * there's a good chance that we could starve the
			 * rest of the system. One symptom of this is that
			 * 'drvconfig(1M)' panics the system while trying
			 * to reload this driver. You can verify this by
			 * enabling this chunk of code: it limits the size
			 * of the RFM memory to 4K. You should then be
			 * able to load and unload this driver to your
			 * heart's content.
			 *------------------------------------------------
			 */
			{
				unsigned newSize = (4 * 1024);
				WHENDEBUG( RFM_DBPROBE )
				{
					xxlog( ucb, "reducing map from %u to %u",
						ucb->ucb_rgc.rgc_ramSize, newSize );
				}
				ucb->ucb_rgc.rgc_ramSize = newSize;
			}
#endif	/* NOPE */

#if 0
			WHENDEBUG( RFM_DBPROBE )
			{
				xxlog( ucb, "unmapping registers before remapping" );
			}

			ddi_regs_map_free( &ucb->ucb_rfm_accHndl );
			DEL_RESOURCE( ucb, UCB_RSRC_RFM );
#endif

			if( ddi_regs_map_setup(
				dip,				/* Device Info Pointer	*/
				4,			/* Register set number	*/
				(caddr_t *) &ucb->ucb_rfm,	/* Return addr here	*/
				0,				/* Offset		*/
				ucb->ucb_rgc.rgc_ramSize,	/* Full monty	 	*/
				&access_attr,			/* Access attributes	*/
				&ucb->ucb_rfm_accHndl		/* Data access hdl	*/
				) != DDI_SUCCESS )
			{
				WHENDEBUG( RFM_DBERROR )
				{
					xxlog( ucb, "cannot map reflected memory" );
				}
				goto BailOut;
			}
			ADD_RESOURCE( ucb, UCB_RSRC_RFM );

			rfm = ucb->ucb_rfm;

			/*
                         * Figure out the DMA capabilities, if any
                         */
			switch( ucb->ucb_rgc.rgc_deviceId )
			{
				default:			/* Unknown devices   */
					ucb->ucb_canDma = 0;
					break;
                        
				case DEVICE_ID_PCI5565:
				case DEVICE_ID_PCI5576:		/* Any 5576 can DMA  */
				case DEVICE_ID_PCI5579:		/* Any 5579 can DMA  */
				case DEVICE_ID_PCI5587:		/* Any 5587 can DMA  */
					ucb->ucb_canDma = 1;
					break;
				case DEVICE_ID_PCI5588:		/* Only revision 2 can */
					ucb->ucb_canDma = (ucb->ucb_rgc.rgc_revId >= 2);
					break;
			}

			WHENDEBUG( RFM_DBPROBE )
			{
				xxlog( ucb, "%s DMA", ucb->ucb_canDma ? "can" : "cannot" );
			}

			if( ucb->ucb_canDma )
			{
				if( ddi_slaveonly( dip ) == DDI_SUCCESS )
				{
					WHENDEBUG( RFM_DBPROBE )
					{
						xxlog( ucb, "device can DMA, but the slot can't" );
					}
					ucb->ucb_canDma = 0;
				}
			}

#if 0
			/*
                         * Fill in the information about this device
                         */
			ucb->ucb_rgc.rgc_boardId      = rfm->rfm_bid;
			ucb->ucb_rgc.rgc_nodeId       = rfm->rfm_nid;
			ucb->ucb_rgc.rgc_freeFireZone = RFM_REGSIZ;
			ucb->ucb_rgc.rgc_regSize      = RFM_REGSIZ;

			switch( ucb->ucb_rgc.rgc_deviceId )
			{
				default:
					WHENDEBUG( RFM_DBPROBE )
					{
						xxlog( ucb, "can't happen: file %s, line %d",
							__FILE__, __LINE__ );
					}
					sprintf( (char *) ucb->ucb_rgc.rgc_name, "pci%04x:%04x",
						ucb->ucb_rgc.rgc_vendorId, ucb->ucb_rgc.rgc_deviceId );
					break;
				case DEVICE_ID_PCI5565:	/* 5565 		 */
					strncpy( (char *) ucb->ucb_rgc.rgc_name, "VMIPCI-5565",
						sizeof( ucb->ucb_rgc.rgc_name ) );
					break;
				case DEVICE_ID_PCI5576:	/* Any 5576 		 */
					strncpy( (char *) ucb->ucb_rgc.rgc_name, "VMIPCI-5576",
						sizeof( ucb->ucb_rgc.rgc_name ) );
					break;
				case DEVICE_ID_PCI5579:		/* Any 5579 can DMA  */
					strncpy( (char *) ucb->ucb_rgc.rgc_name, "VMIPCI-5579",
						sizeof( ucb->ucb_rgc.rgc_name ) );
					break;
				case DEVICE_ID_PCI5587:		/* Any 5587 can DMA  */
					strncpy( (char *) ucb->ucb_rgc.rgc_name, "VMIPCI-5587",
						sizeof( ucb->ucb_rgc.rgc_name ) );
					break;
				case DEVICE_ID_PCI5588:	/* Any 5588 		 */
					strncpy( (char *) ucb->ucb_rgc.rgc_name,
						( ucb->ucb_canDma ? "VMIPCI-5588DMA" : "VMIPCI-5588"),
						sizeof( ucb->ucb_rgc.rgc_name ) );
					break;
			}
			strncpy( (char *) ucb->ucb_rgc.rgc_driverVersion,
				RFM_VERSION, sizeof( ucb->ucb_rgc.rgc_driverVersion ) );
#endif

			/*
			 * To set up the interrupt handler, we need to:
			 *
			 * A. Get the iblock_cookie for interrupt handler.
			 *
			 * B. Create the MUTEX and CV that control access
			 * to the handler. We gotta have the handler's
			 * cookie to do this.
			 *
			 * C. Install the interrupt handler.
			 */
			if( ddi_get_iblock_cookie( dip, 0, &ucb->ucb_iblock_cookie ) != DDI_SUCCESS )
			{
				WHENDEBUG( RFM_DBERROR )
				{
					xxlog( ucb, "cannot determine iblock cookie" );
				}
				goto BailOut;
			}
			/*
			 *------------------------------------------------
			 * Step 3: create the MUTEX that controls the CV
			 *------------------------------------------------
			 * Create the MUTEX that controls access to the
			 * handler.
			 *------------------------------------------------
			 */
			mutex_init( &ucb->ucb_mu, "rfm,mu", MUTEX_DRIVER, ucb->ucb_iblock_cookie );
			ADD_RESOURCE( ucb, UCB_RSRC_MUTEX );
			/*
			 *------------------------------------------------
			 * Step 4: create the CV
			 *------------------------------------------------
			 */
			cv_init(
				&ucb->ucb_cv,
				"rfm,cv",		/* Name of resource		 */
				CV_DRIVER,		/* Class of resource	 */
				NULL			/* Must be NULL in drvrs */
			);
			ADD_RESOURCE( ucb, UCB_RSRC_CV );
			/*
			 *------------------------------------------------
			 * Step 5: Define the interrupt handler.
			 *------------------------------------------------
			 */
			if( ddi_add_intr(
				ucb->ucb_dip,
				0,
				&ucb->ucb_iblock_cookie,
				&ucb->ucb_idevice_cookie,
				xxintr,
				(caddr_t) ucb
				) != DDI_SUCCESS )
			{
				WHENDEBUG( RFM_DBERROR )
				{
					xxlog( ucb, "cannot add interrupt handler" );
				}
				goto BailOut;
			}
			ADD_RESOURCE( ucb, UCB_RSRC_INTR );

			/*
			 *------------------------------------------------
			 * Step 6: create the minor device node
			 *------------------------------------------------
			 */
			rawname = kmem_alloc( RFMNAMELEN, KM_SLEEP );

			sprintf( rawname, "%d,%s,%d", ucb->ucb_lun,
				ucb->ucb_canDma ? "dma" : "pio",
				ucb->ucb_rgc.rgc_ramSize );

			if( ddi_create_minor_node(
				dip,		/* Device info pointer		*/
				rawname,	/* Raw device name		*/
				S_IFCHR,	/* Only a character device	*/
				instance,	/* Minor device number 	 	*/
				"rfm",		/* See devlinks(1M)		*/
				NULL
				) != DDI_SUCCESS )
			{
				WHENDEBUG( RFM_DBERROR )
				{
					xxlog( ucb, "cannot create raw device %s", instance );
				}
				goto BailOut;
			}
			ADD_RESOURCE( ucb, UCB_RSRC_MINOR );
			WHENDEBUG( RFM_DBPROBE )
			{
				xxlog( ucb, "minor device '%s' created", rawname );
			}
			kmem_free( rawname, RFMNAMELEN );
			/*
			 *------------------------------------------------
			 * Step 7: allocate DMA handle, if needed
			 *------------------------------------------------
			 */
			if( ucb->ucb_canDma )
			{
                          int error_status;
				if( (error_status = ddi_dma_alloc_handle(
					dip,			/* Device Info pointer	*/
					&attributes,		/* DMA attributes	*/
					DDI_DMA_SLEEP,		/* No callback	 	*/
					(void *) ucb,		/* Arg for callback 	*/
					&ucb->ucb_dmahandle
					)) != DDI_SUCCESS )
				{
					WHENDEBUG( RFM_DBERROR )
					{
					 xxlog( ucb, "cannot allocate DMA handle ; " "disabling DMA error=0x%x",
                                                      error_status);
					}
					ucb->ucb_canDma = 0;
				} else {
					ADD_RESOURCE( ucb, UCB_RSRC_DMAHDL );
				}
			}

			/* Initialize any other stuff we need	*/
			/* Annouce device into dmesg(1M) log	*/
			ddi_report_dev( dip );
			/* Grab mutex, count this device	*/
			mutex_enter( &busy_mu );
			    ++busy;
			mutex_exit( &busy_mu );

			results = DDI_SUCCESS;
		}
		break;
		default:
			WHENDEBUG( RFM_DBERROR )
			{
				xxlog( ucb, "unknown attach command(%d)", cmd );
			}
	}
	return( results );

	BailOut:

	WHENDEBUG( RFM_DBPROBE )
	{
		xxlog( ucb, "bailing out" );
	}
	/*
	 * The xxdetach() routine knows how to reverse everything done
	 * here. It will also be done in the proper order.
	 */
	(void) xxdetach( dip, DDI_DETACH );
	return( DDI_FAILURE);
}

/*
 *------------------------------------------------------------------------
 * boardReset: set board to known state
 *------------------------------------------------------------------------
 */

static void boardReset
(
	UCB		ucb,		/* Per-device storage		 */
	int		forOnline	/* How to initialize		 */
)
{
	/*
         * Establish invariant information, Local variables, if any
         */
	unsigned int *regs = (unsigned int*)(ucb -> ucb_rfmor);
	int		fifoDepth;
	int		clearedOne;
	uchar_t		senderId;

	WHENDEBUG( RFM_DBPROBE )  xxlog( ucb, "reset" );

	disableInterrupts( ucb );

	mutex_enter( &ucb->ucb_mu );
	/* turn off failed LED */
	regs[0x8/4] &= ~0x80000000; /* set bit 31 to 0 in LCSR1 */

#if 0
	  rfm->rfm_irs = 0;
	  rfm->rfm_csr1 = 0;
	  rfm->rfm_csr2 |= RFM_CSR2_NOLED;
	  rfm->rfm_csr3 = RFM_CSR3_ENDIAN_NONE;
	  rfm->rfm_icsr = 0;
	  /* Empty the incoming event FIFO's */
	  for( clearedOne = 1, fifoDepth = 0; clearedOne && (fifoDepth < (8 * 1024)); ++fifoDepth )
	  {
		clearedOne = 0;
		if( rfm->rfm_irs & RFM_IRS_INT1 )
		{
			senderId = rfm->rfm_sid1;
			++clearedOne;
		}
		if( rfm->rfm_irs & RFM_IRS_INT2 )
		{
			senderId = rfm->rfm_sid2;
			++clearedOne;
		}
		if( rfm->rfm_irs & RFM_IRS_INT3 )
		{
			senderId = rfm->rfm_sid3;
			++clearedOne;
		}
	  }
#endif

        mutex_exit( &ucb->ucb_mu );
}

/*
 *------------------------------------------------------------------------
 * xxopen: called in response to the open(2) system call
 *------------------------------------------------------------------------
 */

static int xxopen
(
	dev_t	*dev,		/* Pointer to complex device no. */
	int      openflags,	/* Flags to open(2) system call  */
	int      otyp,		/* Open type (must be OTYP_CHR)	 */
	cred_t  *credp          /* Pointer to credentials        */
)
{
	/*
         * Establish invariant information & Local variables, if any
         */
	UCB      ucb;           /* Per-device information        */
	uint_t   i;

	WHENDEBUG( RFM_DBOPEN ) xxlog( UNULL, "opening" );

	if( (ucb = (UCB) ddi_get_soft_state(ucbs, getminor(*dev))) == NULL )
	{
		WHENDEBUG( RFM_DBERROR )
		{
			xxlog( UNULL, "no (xxopen) UCB for rfm(%d,%d)",
				(int) getmajor(*dev), (int) getminor(*dev) );
		}
		xxlog( UNULL, "no (xxopen) UCB for rfm(%d,%d)",
			(int) getmajor(*dev), (int) getminor(*dev) );
		return( ENXIO );
	}

	if( otyp != OTYP_CHR )
	{
		WHENDEBUG( RFM_DBOPEN )
		{
			xxlog( ucb, "illegal open type (%d)", otyp );
		}
		return( EINVAL );
	}

	  /* Some things we do only on the first open of the device */
	  if( (ucb->ucb_flags & UCB_FLAGS_OPEN) == 0 )
	  {
		/* Device is not currently opened */
		ushort_t	eventId;

		ucb->ucb_flags |= UCB_FLAGS_OPEN;
		ucb->ucb_dev = *dev;
		for( eventId = 0; eventId < RFM_EVENTID_NEVENTS; ++eventId )
		{
			/* Create timer objects; need icookie first	*/
			eventControlBlock_ctor(
				ucb,
				&ucb->ucb_ecbs[eventId],
				eventId,
				SEC2USEC(2),
				ucb->ucb_rgc.rgc_nodeId
			);
		}
		ucb->ucb_rdi.rdi_threshold = RFMOR_DMA_MIN;

		boardReset( ucb, 1 );			/* Reset interface for online	*/

		/* JHL - 6 */
		for(i = 0; i < 3; i++)
		{
			ucb->ucb_ringBuf[ i ].putPtr  = 0;
			ucb->ucb_ringBuf[ i ].takePtr = 0;
			ucb->ucb_ringBuf[ i ].eventNo = 0;
		}
		enableInterrupts( ucb, 0 );		/* Enable the interrupts        */

  	  }

	return( 0 );
}

/*
 *------------------------------------------------------------------------
 * xxclose: called after last process having device open close(2)'s it.
 *------------------------------------------------------------------------
 *
 * since we enforce exclusive open, xxclose will always be called when
 * the user process calls close(2)
 *------------------------------------------------------------------------
 */

static int xxclose
(
	dev_t 		dev,		/* Complex device number	 */
	int 		openflags,	/* Flags from open(2) call	 */
	int 		otyp,		/* Open type (OTYP_CHR)		 */
	cred_t 		*credp		/* Pointer to credentials	 */
)
{
	/* Establish invariant information				 	 */
	UCB		ucb = (UCB) ddi_get_soft_state( ucbs, getminor(dev) );
	/* Local variables, if any					 	  	 */
	ushort_t	eventId;	/* Counts events		 	 */

	if( !ucb )
	{
		return( ENXIO );
	}
	WHENDEBUG( RFM_DBCLOSE )
	{
		xxlog( ucb, "closing device" );
	}
	boardReset( ucb, 0 );
	for( eventId = 0; eventId < RFM_EVENTID_NEVENTS; ++eventId )
	{
		WHENDEBUG( RFM_DBTIMER )
		{
			xxlog( ucb, "destroying event %u timer", eventId );
		}
		eventControlBlock_dtor( &ucb->ucb_ecbs[ eventId ] );
	}
	/* Finish up before we release the UCB */
	ucb->ucb_flags = 0;
	WHENDEBUG( RFM_DBCLOSE )
	{
		xxlog( ucb, "so long until tomorrow" );
	}
	return( 0 );
}

/*
 *------------------------------------------------------------------------
 * xxminphys: determine maximum DMA transfer size
 *------------------------------------------------------------------------
 */

static void xxminphys
(
	struct buf	*bp		/* Buffer to check */
)
{
	/* Limit transfer from this buffer to DMA maximum */
	if( bp->b_bcount > RFMOR_DMA_MAX )
	{
		WHENDEBUG( RFM_DBMINPHYS )
		{
			xxlog( UNULL, "reducing transfer count from %u to %u",
				(unsigned) bp->b_bcount, (unsigned) RFMOR_DMA_MAX );
		}
		bp->b_bcount = RFMOR_DMA_MAX;
	}
	minphys( bp );	/* Enfore system limits, too */
	WHENDEBUG( RFM_DBMINPHYS )
	{
		xxlog( UNULL, "minphys transfer count is %u",
			(unsigned) bp->b_bcount );
	}
}

/*
 *------------------------------------------------------------------------
 * xxstrategy: use DMA for transfers if possible
 *------------------------------------------------------------------------
 */

#define MAX_PIO  0x170 /* max number of bytes allowed to PIO using copyin/out */

static int xxstrategy
(
	struct buf	*bp		     /* Buffer w/offset            */
)
{
	/*
         * Establish invariant information, Local variables, if any
         */
	int	instance = getminor(bp->b_edev);
	UCB	ucb = (UCB) ddi_get_soft_state( ucbs, instance );
	RFM	rfm = ucb->ucb_rfm;
	off_t	offset;		             /* RFM file position offset    */

	                                     /* Collect initial information */
	offset = (off_t) bp->b_private;	     /* 'xxread/xxwrite' stashed it */

	/*
         * See if any of this qualifies for DMA
         */
	if( offset >= ucb->ucb_rgc.rgc_ramSize )
	{
		/* Cannot begin transfer off the end of RFM	 */
		WHENDEBUG( RFM_DBSTRAT )
		{
			xxlog( ucb, "cannot begin I/O off end of RFM" );
		}
		bioerror( bp, EINVAL );
	}
	else
	{
		uint_t	resid;

		/* We haven't transferred anything yet		*/
		for( bp->b_resid = bp->b_bcount; (resid = bp->b_resid) != 0;
			bp->b_resid -= resid, offset += resid )
		{
			caddr_t	raddr = (caddr_t) &rfm->rfm_ram[ offset ];
			caddr_t	kaddr = bp->b_un.b_addr;
			off_t	offAlign;               /* Offset */
			off_t	bufAlign;               /* Part of kernel address */
			int	canDma;			/* Do a chunk with DMA?   */
                        int     i;

			WHENDEBUG( RFM_DBSTRAT )
			{
				xxlog( ucb, "offset=0x%X, resid=0x%X bp->b_resid=0x%x", (unsigned) offset,
					(unsigned) resid, (unsigned) bp->b_resid );
			}

			/* JHL - 1 */
			ucb->ucb_rfmOffset = offset;

			offAlign = offset & RFMOR_DMA_AMASK;
			bufAlign = ((off_t) kaddr) & RFMOR_DMA_AMASK;

			  canDma =        ucb->ucb_canDma &&
                                          (bufAlign == 0) &&
                                          (offAlign == 0) &&
                         ((resid & RFMOR_DMA_CMASK) == 0) &&
				 (resid >= RFMOR_DMA_MIN) &&
                                 (resid >= ucb->ucb_rdi.rdi_threshold);

			WHENDEBUG( RFM_DBSTRAT )
			{
				xxlog( ucb, "on first look, %s use DMA", canDma ? "can" : "cannot" );
			}

			/* DMA can be done only on a 32-bit boundary */
			if( canDma && bufAlign )
			{
				/* Move up to 32-bit boundary via PIO	 */
				canDma = 0;
				resid = min( RFMOR_DMA_AMASK - bufAlign, resid );
				WHENDEBUG( RFM_DBSTRAT )
				{
					xxlog( ucb, "%u-byte PIO to reach DMA boundary", resid );
				}
			}
			else if( canDma )
			{
				/* We can DMA only 32-bit multiples	 */
				resid = resid & ~RFMOR_DMA_CMASK;
				WHENDEBUG( RFM_DBSTRAT )
				{
					xxlog( ucb, "prepared for %u-byte DMA", resid );
				}
			}

			if( canDma )
			{
				WHENDEBUG( RFM_DBSTRAT )
				{
					xxlog( ucb, "beginning %u-byte DMA", resid );
				}
				if( beginDmaTransfer( ucb, bp ) )
				{
					WHENDEBUG( RFM_DBERROR )
					{
						xxlog( ucb, "DMA failed to start" );
					}
				}
			}
			else
			{


				/* Use PIO to move the data		 */
				if( bp->b_flags & B_WRITE )
				{
					/* Kernel --> RFM			 */
					WHENDEBUG( RFM_DBSTRAT )
					{
						xxlog( ucb, "xxstrategy: %d-byte move kernel 0x%X to RFM 0x%X",
							resid, kaddr, raddr );
					}

                                       for ( i=0 ; i < resid/MAX_PIO ; i++)
                                       {
                                        WHENDEBUG( RFM_DBSTRAT )
                                         {
	                                   xxlog( ucb, "xxstrategy: 1st %d-byte move kernel 0x%X to RFM 0x%X",
							MAX_PIO,kaddr,raddr );
                                         }
					if( copyin( kaddr, raddr, MAX_PIO ) )
					{
                                         WHENDEBUG( RFM_DBSTRAT )
                                          {
                                            xxlog( ucb, "fault writing user data" );
                                          }
                                         bioerror( bp, EFAULT );
                                         break;
					}

                                        kaddr += MAX_PIO;
                                        raddr += MAX_PIO;
                                       }

                                       if ( (i*MAX_PIO) != resid)
                                       {
                                        WHENDEBUG( RFM_DBSTRAT )
                                         {
	                                  xxlog( ucb, "xxstrategy: 2st %d-byte move kernel 0x%X to RFM 0x%X",
							(resid-(i*MAX_PIO)), kaddr, raddr );
                                         }
                                        if( copyin( kaddr, raddr, (resid-(i*MAX_PIO)) ) )
					{
                                         WHENDEBUG( RFM_DBSTRAT )
                                          {
                                            xxlog( ucb, "fault writing user data" );
                                          }
                                         bioerror( bp, EFAULT );
                                         break;
					}
                                       }
				}
				else
				{
					/* RFM --> Kernel			*/
				       WHENDEBUG( RFM_DBSTRAT )
				       {
						xxlog( ucb, "xxstrategy: %d-byte move RFM 0x%X to kernel 0x%X",
							resid, raddr, kaddr );
				       }

                                       for ( i=0 ; i < resid/MAX_PIO ; i++)
                                       {
                                        WHENDEBUG( RFM_DBSTRAT )
                                         {
	                                   xxlog( ucb, "xxstrategy: 1st %d-byte move RFM 0x%X to kernel 0x%X",
							MAX_PIO,raddr,kaddr );
                                         }
					if( copyout(raddr,kaddr,MAX_PIO ) )
					{
                                         WHENDEBUG( RFM_DBSTRAT )
                                          {
                                            xxlog( ucb, "fault reading user data" );
                                          }
                                         bioerror( bp, EFAULT );
                                         break;
					}

                                        kaddr += MAX_PIO;
                                        raddr += MAX_PIO;
                                       }

                                       if ( (i*MAX_PIO) != resid)
                                       {
                                        WHENDEBUG( RFM_DBSTRAT )
                                         {
	                                  xxlog( ucb, "xxstrategy: 2st %d-byte move RFM 0x%X to kernel 0x%X",
							(resid-(i*MAX_PIO)), raddr,kaddr );
                                         }
                                        if( copyout(raddr,kaddr,(resid-(i*MAX_PIO)) ) )
					{
                                         WHENDEBUG( RFM_DBSTRAT )
                                          {
                                            xxlog( ucb, "fault reading user data" );
                                          }
                                         bioerror( bp, EFAULT );
                                         break;
					}
                                       }
				}
			}
			WHENDEBUG( RFM_DBSTRAT )
			{
				xxlog( ucb, "bookkeeping; resid=0x%X, b_resid=0x%X",
					(unsigned) resid, (unsigned) bp->b_resid );
			}
		}
		bp->b_private = (void *)((unsigned)bp->b_private + bp->b_bcount);
	}
	biodone( bp );
	return( 0 );
}

/*
 *------------------------------------------------------------------------
 * xxread: service the read(2) system call
 *------------------------------------------------------------------------
 */
int dma_count = 0;

int xxread
(
	dev_t		dev,		/* Major/minor device number */
	struct uio	*uio,		/* Pointer to uio structure  */
	cred_t		*cred		/* Access mode of device     */
)
{
	/*
         * Establish invariant information, Local variables, if any
         */
	int     	instance = getminor(dev);
	UCB    		ucb = (UCB) ddi_get_soft_state(ucbs, instance);

	struct buf	*bp;            /* Points to raw buffer header  */
	int		status;		/* Results of physio()          */
  
	if( !ucb ) return( ENXIO );

	WHENDEBUG( RFM_DBREAD )
	{
		xxlog( ucb, "%u-byte read from RFM offset %u", uio->uio_resid,
			uio->uio_offset );
	}

	if( (uio->uio_offset < ucb->ucb_rgc.rgc_freeFireZone) && drv_priv( cred ) )
	{
		WHENDEBUG( RFM_DBSTRAT )
		{
			xxlog( ucb, "only 'root' can read inside free fire zone" );
		}
		return( EPERM );
	}

	bp = getrbuf( KM_SLEEP );
	bp->b_private = (void *) uio->uio_offset;
	status = physio( xxstrategy, bp, dev, B_READ, xxminphys, uio );
	freerbuf( bp );
	return( status );
}

/*
 *------------------------------------------------------------------------
 * xxwrite: service the write(2) system call
 *------------------------------------------------------------------------
 */

int xxwrite
(
	dev_t		dev,		/* Major/minor device number */
	struct uio	*uio,		/* Pointer to uio structure  */
	cred_t		*cred		/* Access mode of device     */
)
{
	/*
         * Establish invariant information, Local variables, if any
         */
	int     	instance = getminor(dev);
	UCB    		ucb = (UCB) ddi_get_soft_state(ucbs, instance);

	struct buf	*bp;	        /* Points to raw buffer header */
	int		status;		/* Results of physio()         */
  
	if( !ucb ) return( ENXIO );

	WHENDEBUG( RFM_DBWRITE )
	{
		xxlog( ucb, "%u-byte write to RFM offset %u", uio->uio_resid,
			uio->uio_offset );
	}

	if( (uio->uio_offset < ucb->ucb_rgc.rgc_freeFireZone) && drv_priv( cred ) )
	{
		WHENDEBUG( RFM_DBSTRAT )
		{
			xxlog( ucb, "only 'root' can write inside free fire zone" );
		}
		return( EPERM );
	}

	bp = getrbuf( KM_SLEEP );
	bp->b_private = (void *) uio->uio_offset;
	status = physio( xxstrategy, bp, dev, B_WRITE, xxminphys, uio );
	freerbuf( bp );
	return( status );
}

/*
 *------------------------------------------------------------------------
 * awaitSpecificEvent: pend waiting for specific event to happen
 *------------------------------------------------------------------------
 */

static int awaitSpecificEvent
(
	UCB		ucb,    /* Per-board local storage      */
	RFMNOTIFYEVENT	ren     /* RFM Await Event descriptor   */
)
{
	/*
         * Establish invariant information,  Local variables, if any
         */
	static const char  subr[] = { "awaitSpecificEvent()" };
	ECB                   ecb = &ucb->ucb_ecbs[ ren->ren_eventId ];

	int                 status;     /* Return code */

	ushort_t rtnVal;

	WHENDEBUG( RFM_DBTRACE ) xxlog( ucb, enteringMsg, subr );

	status = 0;			/* Default status to OK */

	/*
	 * Flag that we want it, schedule a timeout (if a period is
	 * defined), and then wait for the event to happen.
	 */

	rtnVal = ringBufHandler( &(ucb->ucb_ringBuf[ ren->ren_eventId ]), 1, 0 );
	if( rtnVal < RINGBUFFER_EMPTY )
	{
           ren->ren_nEvent   = ucb->ucb_ringBuf[ ren->ren_eventId ].eventNo;
           ren->ren_senderId = rtnVal;
           ren->ren_message  = ecb->ecb_eventMsg;
           return( status );
	}

	LOCK_ECB( ecb );
	/* Start timer if we have a non-zero time */
	if( ecb->ecb_duration )
	{
#if defined(_LP64)
                ecb->ecb_timeoutId = timeout( (void (*)(void *))timedout,
                                              ecb,
                                              drv_usectohz(ecb->ecb_duration) );
#else
                ecb->ecb_timeoutId = timeout( (void (*)())timedout,
                                              (caddr_t)ecb,
                                              drv_usectohz(ecb->ecb_duration) );
#endif
	}
	else
	{
		ecb->ecb_timeoutId = (timeout_id_t)-1;
	}

	ecb->ecb_flags |= ECB_FLAGS_WAIT;
	if( cv_wait_sig( &ecb->ecb_cv, &ecb->ecb_mutex ) == 0 )
	{
		/* Got a signal before we saw the event	 		*/
		if( ecb->ecb_timeoutId != (timeout_id_t)-1 )
		{
			untimeout( ecb->ecb_timeoutId );
			ecb->ecb_timeoutId = (timeout_id_t)-1;
		}
		WHENDEBUG( RFM_DBTIMER )
		{
			xxlog( ucb, "event %s wait interrupted by signal",
				eventNames[ren->ren_eventId] );
		}
		status = EINTR;

		/* JHL - 1 */
		/* ecb->ecb_flags |= ECB_FLAGS_TIMO; */
	}
	ecb->ecb_flags &= ~ECB_FLAGS_WAIT;
	/*
	 * If we have a timeout flag and status is still 0, then we had a
	 * timeout.  If no timeout flag, but status = EINTR, then got the 
	 * event before the timeout, but notify for the event was on and
	 * a signal was generated in response to the event.  If no timeout
	 * flag and notify for the event was off, then got the event
	 * before the timeout.
	 */
	if( (ecb->ecb_flags & ECB_FLAGS_TIMO) && !status )
	{
		status = ETIMEDOUT;

		/* JHL - 1 */
		ecb->ecb_flags = ecb->ecb_flags & ~ECB_FLAGS_TIMO;
	}
	/* JHL - 15 */
	else if( !status )
	{
		rtnVal = ringBufHandler( &(ucb->ucb_ringBuf[ ren->ren_eventId ]), 1, 0 );
		if( rtnVal < RINGBUFFER_EMPTY )
		{
			ren->ren_nEvent   = ucb->ucb_ringBuf[ ren->ren_eventId ].eventNo;
			ren->ren_senderId = rtnVal;
			ren->ren_message  = ecb->ecb_eventMsg;
		}
		else
		{
			status = ETIMEDOUT;
			ecb->ecb_flags = ecb->ecb_flags & ~ECB_FLAGS_TIMO;
		}
	}
	/* Whether we got event or not, we're no longer interested	*/

/* JHL - 1 */
#if 0
	if( !status )
	{
		/* No error so fill in RFM event information		 	*/
		ren->ren_nEvent   = ecb->ecb_nEvents;
		ren->ren_senderId = ecb->ecb_senderId;
		ren->ren_message  = ecb->ecb_eventMsg;
	}
/* JHL - 1 */
#endif

	UNLOCK_ECB( ecb );
	WHENDEBUG( RFM_DBTRACE )
	{
		xxlog( ucb, leavingMsg, subr, status );
	}
	return( status );
}

/*
 *------------------------------------------------------------------------
 * validatePeekPoke: check consistency of RFM_PEEK/RFM_POKE request
 *------------------------------------------------------------------------
 */

static int validatePeekPoke
(
	UCB		ucb,	/* Per-device storage		*/
	RFM_ATOM	ra,	/* Request pointer		*/
	char		*func	/* What we are doing		*/
)
{
	/* Offset must be on the RFM interface board	*/
	if( (ra->ra_offset + ra->ra_size) > ucb->ucb_rgc.rgc_ramSize )
	{
		WHENDEBUG( RFM_DBIOCTL )
		{
			xxlog( ucb, "%s object offset (%u) off board",
				func, ra->ra_offset );
		}
		return( EINVAL );
	}
	/* Validate the object size					 	*/
	switch( ra->ra_size )
	{
		default:
			WHENDEBUG( RFM_DBIOCTL )
			{
				xxlog( ucb, "%s object size (%u) not 1, 2 or 4", func, ra->ra_size );
			}
			return( EINVAL );
		case 1:
		case 2:
		case 4:
		break;
	}
	/* Offset must be aligned on a natural boundary */
	if( ra->ra_offset % ra->ra_size )
	{
		WHENDEBUG( RFM_DBIOCTL )
		{
			xxlog( ucb, "%s object offset (%u) not on %u-byte boundary",
				func, ra->ra_offset, ra->ra_size );
		}
		return( EINVAL );
	}
	return( 0 );
}

/*
 *------------------------------------------------------------------------
 * notificationControl: turn RFM event notification on or off
 *------------------------------------------------------------------------
 */

static int notificationControl
(
	UCB		ucb,		/* Per-device local storage	 */
	RFMEVENT	re		/* Event to be controlled	 */
)
{
	/*
         * Establish invariant information, Local variables, if any
         */
	static const char  subr[] = { "notificationControl()" };
	ushort_t	   eventId;	/* Event code (subscript)	 */
	ECB		   ecb;         /* Event control block addr	 */

	WHENDEBUG( RFM_DBTRACE ) xxlog( ucb, enteringMsg, subr );

	  switch( (eventId = re->re_eventId) )
	  {
                default:
                {
                  WHENDEBUG( RFM_DBIOCTL )
                      xxlog( ucb, "cannot notify of %d events", eventId );
		}
		return( EINVAL );

		case RFM_EVENTID_A:
		case RFM_EVENTID_B:
		case RFM_EVENTID_C:
		case RFM_EVENTID_F:
		case RFM_EVENTID_RESET:
		case RFM_EVENTID_BDATA:
			break;
	  }

	/*
	 * Here we get to be a bit draconian: because we don't bother to
	 * keep a list of processes wanting to be notified, we adopt the
	 * rule of "latest-is-greatest" and discard any prior notification
	 * registry.
	 */

	ecb = &ucb->ucb_ecbs[ eventId ];
	LOCK_ECB( ecb );
	  if( ecb->ecb_userProc )
	  {
		/* Turn off flag first, since interrupt world checks it */
		ecb->ecb_flags &= ~ECB_FLAGS_TELL;
		proc_unref( ecb->ecb_userProc );
		ecb->ecb_userProc = 0;
	  }

	  /*
           * Manipulate the notification event
           */
	  if( re->re_signalId == 0 )
	  {
		WHENDEBUG( RFM_DBIOCTL )
		{
			xxlog( ucb, "event %s notification disabled",
				eventNames[eventId].ent_name );
		}
		/* Turn off flag first, since interrupt world checks it */
		ecb->ecb_flags &= ~ECB_FLAGS_TELL;
		ecb->ecb_signal = 0;
	  }
	  else
	  {
		WHENDEBUG( RFM_DBIOCTL )
		{
			xxlog( ucb, "send signal %d for event %s notification",
				re->re_signalId, eventNames[eventId].ent_name );
		}
		/* Turn flag on last, since interrupt world checks it */
		ecb->ecb_userProc = proc_ref();
		ecb->ecb_signal   = re->re_signalId;
		ecb->ecb_flags   |= ECB_FLAGS_TELL;
	  }
	UNLOCK_ECB( ecb );

	WHENDEBUG( RFM_DBTRACE ) xxlog( ucb, leavingMsg, subr, 0 );

	return( 0 );
}

/*
 *------------------------------------------------------------------------
 * xxioctl: called by kernel to service an ioctl(2) system call
 *------------------------------------------------------------------------
 */

/*ARGSUSED*/
static int xxioctl
(
	dev_t 	dev,			/* Complex device number   */
	int 	cmd,			/* Command code			 	*/
	intptr_t  arg,			/* Argument to command		 	*/
	int 	flags,			/* Open(2) flags 		 	*/
	cred_t 	*credp,			/* Credentials (IGNORED)	 	*/
	int 	*rvalp			/* Return value pointer		 	*/
)
{
	/*
         * Establish invariant information, Local variables, if any
         */
	int	instance = getminor(dev);
	UCB	ucb;			/* Per-device local storage     */
	RFM	rfm;			/* Hardware address		*/
	char *spelling;		        /* Name of ioctl command	*/
	int	retval;			/* Results of system call	*/
	int	x;			/* Incoming CPU processor status*/
        ushort_t junk;		        /* Gotta have some place for it */


	/* JHL - 3 */
	uint_t dummy;
	uint_t dummy1;
	ucb = (UCB) ddi_get_soft_state( ucbs, instance );

	if( !ucb )
	{
		return( ENXIO );
	}
	rfm = ucb->ucb_rfm;
	spelling = "UNKNOWN";
	retval = EFAULT;		/* Pessimistic assumption	 */


        WHENDEBUG( RFM_DBIOCTL )
        {
            xxlog( ucb, "ioctl(2) command = 0x%X   sizeof(long): 0x%X", 
                                                cmd, sizeof(long) );
        }
 
	/* Dispatch based on the command code				 */

	switch( cmd )
	{
		default:
		{
			WHENDEBUG( RFM_DBIOCTL )
			{
				xxlog( ucb, "unknown ioctl(2) command = 0x%X", cmd );
			}
			retval = ENOTTY;
		}
		break;

		case RFM_DEBUG:			/* User gives new debug flags	*/
		{
			uint_t	newFlags;

			spelling = "RFM_DEBUG";
			if( ddi_copyin( (caddr_t) arg, (caddr_t) &newFlags,
				sizeof( uint_t ), flags ) == -1 )
			{
				goto BailOut;
			}
			WHENDEBUG( RFM_DBIOCTL )
			{
				xxlog( ucb, "new debug flags are 0x%08X", newFlags );
			}
			rfmDebug = newFlags;
			retval = 0;
		}
		break;

		case RFM_GDEBUG:		/* User wants debug flags		*/
		{
			spelling = "RFM_GDEBUG";
			WHENDEBUG( RFM_DBIOCTL )
			{
				xxlog( ucb, "debug flags are 0x%08X", rfmDebug );
			}
			if( ddi_copyout( (caddr_t) &rfmDebug, (caddr_t) arg,
				sizeof( uint_t ), flags ) == -1 )
			{
				goto BailOut;
			}
			retval = 0;
		}
		break;

		case RFM_GDEBUG_BYNAME: /* Lookup debug flag name		*/
		{
			rfmDebugFlagName_t	rdfn;
			DEBUGNAMECLASS      dnt;
			uint_t	            bitno;

			spelling = "RFM_GDEBUG_BYNAME";
			if( ddi_copyin( (caddr_t) arg, (caddr_t) &rdfn,
				sizeof( rdfn ), flags ) == -1 )
			{
				goto BailOut;
			}
			/* Discard any trash in original argument        	*/
            bitno = rdfn.rdfn_bitno;
            bzero( (caddr_t) &rdfn, sizeof(rdfn) );
            rdfn.rdfn_bitno = bitno;
            if( bitno < NDEBUGNAMECLASS )
			{
            	/* Fill in info about this message class */
                dnt = &debugNameClasses[bitno];
                rdfn.rdfn_mask = dnt->dnt_flag;
                strncpy( rdfn.rdfn_name, dnt->dnt_name, sizeof(rdfn.rdfn_name) );
                strncpy( rdfn.rdfn_desc, dnt->dnt_desc, sizeof(rdfn.rdfn_desc) );
            }
			else
			{
            	/* Make a fake entry ('mask' will be 0)  */
                char	name[32];
                char    desc[64];
                sprintf( name, "BIT%02u", bitno );
                sprintf( desc, "Unassigned class %d", bitno );
                strncpy( rdfn.rdfn_name, name, sizeof(rdfn.rdfn_name) );
                strncpy( rdfn.rdfn_desc, desc, sizeof(rdfn.rdfn_desc) );
            }
			WHENDEBUG( RFM_DBIOCTL )
			{
				xxlog( ucb, "class=%u, mask=0x%04X, name='%s', desc='%s'",
					rdfn.rdfn_bitno, rdfn.rdfn_mask, rdfn.rdfn_name,
					rdfn.rdfn_desc );
			}
			/* Return the results						*/
			if( ddi_copyout( (caddr_t) &rdfn, (caddr_t) arg,
				sizeof( rdfn ), flags ) == -1 )
			{
				goto BailOut;
			}
			retval = 0;
		}
		break;

		case RFM_GETNEVENTS:		/* Get performance stats	 */
		case RFM_GETNEVENTSCLEAR:	/* ... and clear them, too	 */
		{
			rfmGetNevents_t	rge;
			ushort_t		eventId;
			ECB				ecb;
			int				clearThem;

			if( cmd == RFM_GETNEVENTS )
			{
				spelling = "RFM_GETNEVENTS";
				clearThem = 0;
			}
			else
			{
				spelling = "RFM_GETNEVENTSCLEAR";
				clearThem = 1;
			}
			mutex_enter( &ucb->ucb_mu );
			   for( eventId = 0; eventId < RFM_EVENTID_NEVENTS; ++eventId )
			   {
				ecb = &ucb->ucb_ecbs[ eventId ];
				rge.rge_nEvents[ eventId ] = ecb->ecb_nEvents;
				if( clearThem )
				{
					ecb->ecb_nEvents = 0;
				}
			   }
			mutex_exit( &ucb->ucb_mu );
			if( ddi_copyout( (caddr_t) &rge, (caddr_t) arg,
				sizeof( rge ), flags ) == -1 )
			{
				goto BailOut;
			}
			retval = 0;
		}
		break;

		case RFM_AWAITNEXTEVENT:	/* Wait for next async event	*/
		{
			rfmEventNotify_t	ren;

			spelling = "RFM_AWAITNEXTEVENT";
			/* Get struct to see which event is expected	 		*/
			if( ddi_copyin( (caddr_t) arg, (caddr_t) &ren,
				sizeof( ren ), flags ) == -1 )
			{
				goto BailOut;
			}
			/* Validate the eventID									*/
			switch( ren.ren_eventId )
			{
				default:
				{
					WHENDEBUG( RFM_DBIOCTL )
					{
						xxlog( ucb, "improper await eventId (%u)", ren.ren_eventId );
					}
					retval = EINVAL;
				}
				goto BailOut;

				case RFM_EVENTID_A:
				case RFM_EVENTID_B:
				case RFM_EVENTID_C:
				case RFM_EVENTID_F:
				case RFM_EVENTID_RESET:
				case RFM_EVENTID_BDATA:
					break;
			}
			retval = awaitSpecificEvent( ucb, &ren );
			if( retval != 0 )
			{
				WHENDEBUG( RFM_DBIOCTL )
				{
					xxlog( ucb, "no event %u detected", ren.ren_eventId );
				}
				goto BailOut;
			}
			/* Return the results				 				*/
			if( ddi_copyout( (caddr_t) &ren, (caddr_t) arg,
				sizeof( ren ), flags ) == -1 )
			{
				goto BailOut;
			}
			retval = 0;
		}
		break;

		case RFM_PEEK:			/* Return RFM memory contents	*/
		case RFM_POKE:			/* Set new RFM memory content	*/
		{
			rfmAtom_t	ra;
			char		*func;

			if( cmd == RFM_PEEK )
			{
				spelling = "RFM_PEEK";
				func = "peek";
			}
			else
			{
				spelling = "RFM_POKE";
				func = "poke";
			}
			if( ddi_copyin( (caddr_t) arg, (caddr_t) &ra,
				sizeof( ra ), flags ) == -1 )
			{
				goto BailOut;
			}
			retval = validatePeekPoke( ucb, &ra, func );
			if( retval != 0 )
			{
				goto BailOut;
			}
			/* Do the operation */
			if( cmd == RFM_PEEK )
			{
				/* Get the contents and return them */
				switch( ra.ra_size )
				{
					default:
						WHENDEBUG( RFM_DBERROR )
						{
							xxlog( ucb, "can't happen: file %s, line %d",
								__FILE__, __LINE__ );
						}
						ra.ra_contents = 0;
						break;
					case 1:
						ra.ra_contents = ucb->ucb_rfm->rfm_ram[ ra.ra_offset ];
						break;
					case 2:
						ra.ra_contents = ucb->ucb_rfm->rfm_ramw[ ra.ra_offset / 2 ];

						/* JHL - 4 */
						dummy = ra.ra_contents & 0xFF;
						dummy = dummy << 8;
						ra.ra_contents = (ra.ra_contents >> 8) & 0xFF;
						ra.ra_contents = ra.ra_contents | dummy;

						break;
					case 4:
						ra.ra_contents = ucb->ucb_rfm->rfm_raml[ ra.ra_offset / 4 ];

						/* JHL - 17 */
						dummy = ra.ra_contents & 0xFF;
						dummy = dummy << 24;
						dummy1 = dummy;

						dummy = ra.ra_contents & 0xFF00;
						dummy = dummy << 8;
						dummy1 = dummy1 | dummy;

						dummy = ra.ra_contents & 0xFF0000;
						dummy = dummy >> 8;
						dummy1 = dummy1 | dummy;

						dummy = ra.ra_contents & 0xFF000000;
						dummy = dummy >> 24;
						dummy1 = dummy1 | dummy;

						ra.ra_contents = dummy1;

						break;
				}
				WHENDEBUG( RFM_DBIOCTL )
				{
					xxlog( ucb, "%s; size=%u, offset=0x%X, contents=0x%X",
						spelling, ra.ra_size, ra.ra_offset, ra.ra_contents );
				}
				if( ddi_copyout( (caddr_t) &ra, (caddr_t) arg,
					sizeof( ra ), flags ) == -1 )
				{
					goto BailOut;
				}
			}
			else /* its a poke */
			{
				WHENDEBUG( RFM_DBIOCTL )
				{
					xxlog( ucb, "%s; size=%u, offset=0x%X, contents=0x%X",
						spelling, ra.ra_size, ra.ra_offset, ra.ra_contents );
				}
				/* Set the new contents */
				switch( ra.ra_size )
				{
					default:
						WHENDEBUG( RFM_DBERROR )
						{
							xxlog( ucb, "can't happen: file %s, line %d",
								__FILE__, __LINE__ );
						}
						break;
					case 1:
						ucb->ucb_rfm->rfm_ram[ ra.ra_offset ] = (uchar_t) ra.ra_contents;
						break;
					case 2:

						/* JHL - 4 */
						dummy = ra.ra_contents & 0xFF;
						dummy = dummy << 8;
						ra.ra_contents = (ra.ra_contents >> 8) & 0xFF;
						ra.ra_contents = ra.ra_contents | dummy;

						ucb->ucb_rfm->rfm_ramw[ ra.ra_offset / 2 ] = (ushort_t) ra.ra_contents;
						break;
					case 4:

						/* JHL - 17 */
						dummy = ra.ra_contents & 0xFF;
						dummy = dummy << 24;
						dummy1 = dummy;

						dummy = ra.ra_contents & 0xFF00;
						dummy = dummy << 8;
						dummy1 = dummy1 | dummy;

						dummy = ra.ra_contents & 0xFF0000;
						dummy = dummy >> 8;
						dummy1 = dummy1 | dummy;

						dummy = ra.ra_contents & 0xFF000000;
						dummy = dummy >> 24;
						dummy1 = dummy1 | dummy;

						ra.ra_contents = dummy1;

						ucb->ucb_rfm->rfm_raml[ ra.ra_offset / 4 ] = (uint32_t) ra.ra_contents;
						break;
				}
			}
			retval = 0;
		}
		break;

		case RFM_TIMEOUT:		/* Set timeout (useconds)	 */
		{
			rfmTimeout_t	rto;

			spelling = "RFM_TIMEOUT";
			/* Get timeout info from user */
			if( ddi_copyin( (caddr_t) arg, (caddr_t) &rto,
				sizeof( rto ), flags ) == -1 )
			{
				goto BailOut;
			}
			/* Validate the eventID */
			if( rto.rto_eventId >= RFM_EVENTID_NEVENTS )
			{
				WHENDEBUG( RFM_DBIOCTL )
				{
					xxlog( ucb, "no such event ID (%u)", rto.rto_eventId );
				}
				retval = EINVAL;
				goto BailOut;
			}
			ucb->ucb_ecbs[ rto.rto_eventId ].ecb_duration = rto.rto_duration;
			WHENDEBUG( RFM_DBIOCTL )
			{
				xxlog( ucb, "new %s event timeout is %u useconds",
					eventNames[rto.rto_eventId],
					rto.rto_duration );
			}
			retval = 0;
		}
		break;

		case RFM_GTIMEOUT:		/* Get timeout (useconds)	 */
		{
			rfmTimeout_t	rto;

			spelling = "RFM_GTIMEOUT";
			/* Get timeout ID from user */
			if( ddi_copyin( (caddr_t) arg, (caddr_t) &rto,
				sizeof( rto ), flags ) == -1 )
			{
				goto BailOut;
			}
			/* Validate the event ID */
			if( rto.rto_eventId >= RFM_EVENTID_NEVENTS )
			{
				WHENDEBUG( RFM_DBIOCTL )
				{
					xxlog( ucb, "no such event ID (%u)", rto.rto_eventId );
				}
				retval = EINVAL;
				goto BailOut;
			}
			rto.rto_duration = ucb->ucb_ecbs[ rto.rto_eventId ].ecb_duration;
			WHENDEBUG( RFM_DBIOCTL )
			{
				xxlog( ucb, "event %u timeout is %u usecond(s)", rto.rto_eventId,
					rto.rto_duration );
			}
			if( ddi_copyout( (caddr_t) &rto, (caddr_t) arg,
				sizeof( rto ), flags ) == -1 )
			{
				goto BailOut;
			}
			retval = 0;
		}
		break;

		case RFM_SENDEVENT:		/* Send RFM event		 */
		{
			rfmSendEvent_t	rse;
			uchar_t		cmd;

			spelling = "RFM_SENDEVENT";
			/* Get event info from user */
			if( ddi_copyin( (caddr_t) arg, (caddr_t) &rse,
				sizeof( rse ), flags ) == -1 )
			{
				goto BailOut;
			}
			WHENDEBUG( RFM_DBIOCTL )
			{
				xxlog( ucb, "eventId=%u, dest=%u, me=%u",
					(unsigned) rse.rse_eventId,
					(unsigned) rse.rse_destNode,
					(unsigned) ucb->ucb_rgc.rgc_nodeId );
			}
			/* Cannot sent events to ourselves		 	*/
			if( rse.rse_destNode == ucb->ucb_rgc.rgc_nodeId )
			{
				WHENDEBUG( RFM_DBIOCTL )
				{
					xxlog( ucb, "I refuse to talk to myself" );
				}
				retval = EIO;
				break;
			}
			/* Some events cannot be sent			 	*/
			switch( rse.rse_eventId )
			{
				default:
				{
					WHENDEBUG( RFM_DBIOCTL )
					{
						xxlog( ucb, "event ID %u cannot be sent", rse.rse_eventId );
					}
					retval = EINVAL;
				}
				goto BailOut;

				case RFM_EVENTID_A:
					cmd = RFM_CMD_LEVEL_INT1;

					CheckForTcpip:

					if( has_TCPIP( ucb ) )
					{
						WHENDEBUG( RFM_DBIOCTL )
						{
							xxlog( ucb, "cannot send event %u with TCP/IP enabled",
								rse.rse_eventId );
						}
						retval = EIO;
						goto BailOut;
					}
					break;

				case RFM_EVENTID_B:
					cmd = RFM_CMD_LEVEL_INT2;
					goto CheckForTcpip;

				case RFM_EVENTID_C:
					cmd = RFM_CMD_LEVEL_INT3;
					break;
			}
			/* We could be broadcasting the event */
			if( rse.rse_destNode == RFM_RSE_BROADCAST )
			{
				cmd |= RFM_CMD_ALL;
				rse.rse_destNode = 0;
			}
			else if( rse.rse_destNode > 255 )
			{
				WHENDEBUG( RFM_DBIOCTL )
				{
					xxlog( ucb, "illegal destination node (%u)", rse.rse_destNode );
				}
				retval = EINVAL;
				goto BailOut;
			}
			WHENDEBUG( RFM_DBIOCTL )
			{
				xxlog( ucb, "cmdn=0x%X, cmd=0x%X", (unsigned) rse.rse_destNode,
					(unsigned) cmd );
			}
			mutex_enter( &ucb->ucb_mu );
			     rfm->rfm_cmdn = rse.rse_destNode;
                             junk = rfm->rfm_cmdn;             /* Flush */
			     rfm->rfm_cmd  = cmd;
                             junk = rfm->rfm_cmd;              /* Flush */
			mutex_exit( &ucb->ucb_mu );
			retval = 0;
		}
		break;

		case RFM_NOTIFY:		/* Set event notification state	 */
		{
			rfmEvent_t	re;

			spelling = "RFM_NOTIFY";
			/* Get notification setup from user	*/
			if( ddi_copyin( (caddr_t) arg, (caddr_t) &re,
				sizeof( re ), flags ) == -1 )
			{
				goto BailOut;
			}
			retval = notificationControl( ucb, &re );
		}
		break;

		case RFM_GNOTIFY:		/* Get event notification state	 */
		{
			rfmEvent_t	re;
			ECB			ecb;

			spelling = "RFM_GNOTIFY";
			/* Get event ID from user */
			if( ddi_copyin( (caddr_t) arg, (caddr_t) &re,
				sizeof( re ), flags ) == -1 )
			{
				goto BailOut;
			}
			/* Validate the event ID */
			switch( re.re_eventId )
			{
				default:
				{
					WHENDEBUG( RFM_DBIOCTL )
					{
						xxlog( ucb, "unknown event id (%u)", re.re_eventId );
					}
					retval = EINVAL;
				}
				goto BailOut;

				case RFM_EVENTID_A:
				case RFM_EVENTID_B:
				case RFM_EVENTID_C:
				case RFM_EVENTID_F:
				case RFM_EVENTID_RESET:
				case RFM_EVENTID_BDATA:
					break;
			}
			ecb = &ucb->ucb_ecbs[ re.re_eventId ];
			/* Fill in the argument structure */
			if( ecb->ecb_flags & ECB_FLAGS_TELL )
			{
				re.re_signalId = ecb->ecb_signal;
			}
			else
			{
				re.re_signalId = 0;
			}
			WHENDEBUG( RFM_DBIOCTL )
			{
				xxlog( ucb, "event='%s' (%u) signal=%u state='%s'",
					eventNames[ re.re_eventId ].ent_name,
					re.re_eventId, re.re_signalId,
					(re.re_signalId ? "enabled" : "disabled") );
			}
			/* Return the settings */
			if( ddi_copyout( (caddr_t) &re, (caddr_t) arg,
				sizeof( re ), flags ) == -1 )
			{
				goto BailOut;
			}
			retval = 0;
		}
		break;

		case RFM_DMAINFO:		/* Set DMA threshold		 */
		{
        	rfmDmaInfo_t	rdi;
 
            spelling = "RFM_DMAINFO";
            /* Get application's idea of new info            */
            if( ddi_copyin( (caddr_t) arg, (caddr_t) &rdi, sizeof(rdi), flags ) )
			{
				goto BailOut;
			}
            WHENDEBUG( RFM_DBIOCTL )
			{
				xxlog( ucb, "new dma threshold is %u byte(s)", rdi.rdi_threshold );
            }
            /* Update the DMA info settings                  */
            ucb->ucb_rdi = rdi;     /* Structure copy!       */
			retval = 0;
		}
		break;

		case RFM_GDMAINFO:		/* Return DMA threshold		 */
		{
        	spelling = "RFM_GDMAINFO";
            /* Return current settings to caller             */
            WHENDEBUG( RFM_DBIOCTL )
			{
				xxlog( ucb, "dma threshold is presently %u byte(s)",
					ucb->ucb_rdi.rdi_threshold );
            }
            if( ddi_copyout( (caddr_t) &ucb->ucb_rdi, (caddr_t) arg,
            	sizeof(ucb->ucb_rdi), flags ) )
			{
				goto BailOut;
			}
			retval = 0;
		}
		break;

		case RFM_GETCONFIG:		/* Return RFM configuration	 */
		{
			spelling = "RFM_GETCONFIG";
			/* Return what we've already found out */
            if( ddi_copyout( (caddr_t) &ucb->ucb_rgc, (caddr_t) arg,
            	sizeof(ucb->ucb_rgc), flags ) )
			{
				goto BailOut;
			}
			retval = 0;
		}
		break;

		case RFM_GETOPREGINFO:
		{
			rfmOpRegisterInfo_t	rri;
			REGNAME				rnt;
			REGNAME				lrnt;
			
			RFMOR		rfmor = ucb->ucb_rfmor;
			
			spelling = "RFM_GETOPREGINFO";
            if( ddi_copyin( (caddr_t) arg, (caddr_t) &rri, sizeof(rri), flags ) )
			{
				goto BailOut;
			}
            rri.rri_width = 0;
			rri.rri_value = 0;
            bzero( rri.rri_name, sizeof(rri.rri_name) );
            bzero( rri.rri_desc, sizeof(rri.rri_desc) );
			retval = -1;
            for( rnt = opRegNames, lrnt = rnt + NOPREGNAMES; rnt < lrnt; ++rnt )
			{
            	if( rri.rri_offset == rnt->rnt_offset )
				{
                	rri.rri_width = rnt->rnt_width;
                    rri.rri_flags = rnt->rnt_flags;
                    strncpy( rri.rri_name, rnt->rnt_name, sizeof(rri.rri_name) );
                    strncpy( rri.rri_desc, rnt->rnt_desc, sizeof(rri.rri_desc) );
					if(rri.rri_offset == 0x24)
						rri.rri_value = ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_mwar);
					else if(rri.rri_offset == 0x28)
						rri.rri_value = ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_mwtc);
					else if(rri.rri_offset == 0x2C)
						rri.rri_value = ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_mrar);
					else if(rri.rri_offset == 0x30)
						rri.rri_value = ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_mrtc); 
					else if(rri.rri_offset == 0x38)
						rri.rri_value = ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_intcsr); 
					else if(rri.rri_offset == 0x3C)
						rri.rri_value = ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_mcsr);
					retval = 0;
					break;
                }
            }
            WHENDEBUG( RFM_DBIOCTL )
			{
            	xxlog( ucb, "offset=0x%X, flags=0x%X, width=%u, name='%s', desc='%s'",
					rri.rri_offset, rri.rri_flags, rri.rri_width, rri.rri_name,
					rri.rri_desc );
            }
            if( ddi_copyout( (caddr_t) &rri, (caddr_t) arg, sizeof(rri), flags ) )
			{
				goto BailOut;
			}
		}
		break;

		case RFM_GETREGINFO:	/* Lookup register by offset */
		{
			rfmRegisterInfo_t	rri;
			REGNAME				rnt;
			REGNAME				lrnt;

			spelling = "RFM_GETREGINFO";

            /* Get argument (for the eventId) */
            if( ddi_copyin( (caddr_t) arg, (caddr_t) &rri, sizeof(rri), flags ) )
			{
				goto BailOut;
			}
            /* Clear width and such in case there isn't one */
            rri.rri_width = 0;
            bzero( rri.rri_name, sizeof(rri.rri_name) );
            bzero( rri.rri_desc, sizeof(rri.rri_desc) );
			/* JHL - 1 */
			retval = -1;
            /* Linear scan of the defined registers         */
            for( rnt = regNames, lrnt = rnt + NREGNAMES; rnt < lrnt; ++rnt )
			{
            	if( rri.rri_offset == rnt->rnt_offset )
				{
                	rri.rri_width = rnt->rnt_width;
                    rri.rri_flags = rnt->rnt_flags;
                    strncpy( rri.rri_name, rnt->rnt_name, sizeof(rri.rri_name) );
                    strncpy( rri.rri_desc, rnt->rnt_desc, sizeof(rri.rri_desc) );
					/* JHL - 1 */
					retval = 0;
					break;
                }
            }
            WHENDEBUG( RFM_DBIOCTL )
			{
            	xxlog( ucb, "offset=0x%X, flags=0x%X, width=%u, name='%s', desc='%s'",
					rri.rri_offset, rri.rri_flags, rri.rri_width, rri.rri_name,
					rri.rri_desc );
            }
            /* Return the results */
            if( ddi_copyout( (caddr_t) &rri, (caddr_t) arg, sizeof(rri), flags ) )
			{
				goto BailOut;
			}
			/* JHL - 1 */
			/* retval = 0; */
		}
		break;
		
		case RFM_GETOPREGINFOBYNAME:	/* Lookup register by name	 */
		{
			rfmOpRegisterInfo_t     rri;
			REGNAME                 rnt;
			REGNAME                 lrnt;
			char                    *name;
			size_t                  len;

			RFMOR		rfmor = ucb->ucb_rfmor;

			spelling = "RFM_GETOPREGINFOBYNAME";
			/* Get argument (for the eventId) */
			if( ddi_copyin( (caddr_t) arg, (caddr_t) &rri, sizeof(rri), flags ) )
			{
				goto BailOut;
			}
			/* Skip over leading "rfm_" if we have it */
			if( !Strncasecmp( rri.rri_name, regNamePrefix, LregNamePrefix) )
			{
				/* Have the "rfm_" prefix */
				name = rri.rri_name + LregNamePrefix;
				len = sizeof(rri.rri_name) - LregNamePrefix;
			}
			else
			{
				/* No prefix, check whole name */
				name = rri.rri_name;
				len = sizeof(rri.rri_name);
			}
			/* Clear width, offset and flags (just in case) */
			rri.rri_offset = 0;
			rri.rri_width = 0;
			rri.rri_value = 0;
			rri.rri_flags = 0;
			/* Linear search of the name table */
			for( rnt = opRegNames, lrnt = rnt + NOPREGNAMES; rnt < lrnt; ++rnt )
			{
				/* See if this is the right entry */
				if( !Strncasecmp( name, rnt->rnt_name, len ) )
				{
					/* Copy data for this name */
					rri.rri_offset = rnt->rnt_offset;
					rri.rri_flags = rnt->rnt_flags;
					rri.rri_width = rnt->rnt_width;
					if(rri.rri_offset == 0x24)
						rri.rri_value = ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_mwar);
					else if(rri.rri_offset == 0x28)
						rri.rri_value = ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_mwtc);
					else if(rri.rri_offset == 0x2C)
						rri.rri_value = ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_mrar);
					else if(rri.rri_offset == 0x30)
						rri.rri_value = ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_mrtc);
					else if(rri.rri_offset == 0x38)
						rri.rri_value = ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_intcsr);
					else if(rri.rri_offset == 0x3C)
						rri.rri_value = ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_mcsr);
					strncpy( rri.rri_desc, rnt->rnt_desc, sizeof(rri.rri_desc) );
					break;
				}
			}
			WHENDEBUG( RFM_DBIOCTL )
			{
				xxlog( ucb, "offset=0x%X, flags=0x%X, width=%u, name='%s', desc='%s'",
					rri.rri_offset, rri.rri_flags, rri.rri_width, rri.rri_name,
					rri.rri_desc );
			}
			/* Return the results */
			if( ddi_copyout( (caddr_t) &rri, (caddr_t) arg, sizeof(rri), flags ) )
			{
				goto BailOut;
			}
			retval = 0;
		}
		break;

		case RFM_GETREGINFOBYNAME:	/* Lookup register by name	 */
		{
			rfmRegisterInfo_t       rri;
			REGNAME                 rnt;
			REGNAME                 lrnt;
			char                    *name;
			size_t                  len;

			spelling = "RFM_GETREGINFOBYNAME";
			/* Get argument (for the eventId) */
			if( ddi_copyin( (caddr_t) arg, (caddr_t) &rri, sizeof(rri), flags ) )
			{
				goto BailOut;
			}
			/* Skip over leading "rfm_" if we have it */
			if( !Strncasecmp( rri.rri_name, regNamePrefix, LregNamePrefix) )
			{
				/* Have the "rfm_" prefix */
				name = rri.rri_name + LregNamePrefix;
				len = sizeof(rri.rri_name) - LregNamePrefix;
			}
			else
			{
				/* No prefix, check whole name */
				name = rri.rri_name;
				len = sizeof(rri.rri_name);
			}
			/* Clear width, offset and flags (just in case) */
			rri.rri_offset = 0;
			rri.rri_width = 0;
			rri.rri_flags = 0;
			/* Linear search of the name table */
			for( rnt = regNames, lrnt = rnt + NREGNAMES; rnt < lrnt; ++rnt )
			{
				/* See if this is the right entry */
				if( !Strncasecmp( name, rnt->rnt_name, len ) )
				{
					/* Copy data for this name */
					rri.rri_offset = rnt->rnt_offset;
					rri.rri_flags = rnt->rnt_flags;
					rri.rri_width = rnt->rnt_width;
					strncpy( rri.rri_desc, rnt->rnt_desc, sizeof(rri.rri_desc) );
					break;
				}
			}
			WHENDEBUG( RFM_DBIOCTL )
			{
				xxlog( ucb, "offset=0x%X, flags=0x%X, width=%u, name='%s', desc='%s'",
					rri.rri_offset, rri.rri_flags, rri.rri_width, rri.rri_name,
					rri.rri_desc );
			}
			/* Return the results */
			if( ddi_copyout( (caddr_t) &rri, (caddr_t) arg, sizeof(rri), flags ) )
			{
				goto BailOut;
			}
			retval = 0;
		}
		break;

		case RFM_GETEVENTINFO:		/* Lookup event by ID		 */
		{
			ushort_t                eventId;
			rfmEventInfo_t          rei;
			EVENTNAME               ent;

			spelling = "RFM_GETEVENTINFO";
			/* Get argument (for the ID value) */
			if( ddi_copyin( (caddr_t) arg, (caddr_t) &rei, sizeof(rei), flags ) )
			{
				goto BailOut;
			}
			/* Return info if we have it */
			eventId = rei.rei_eventId;
			if( eventId >= RFM_EVENTID_NEVENTS )
			{
				WHENDEBUG( RFM_DBIOCTL )
				{
					xxlog( ucb, "unknown event ID (%u)", eventId );
				}
				retval = EINVAL;
				goto BailOut;
			}
			/* Fill in the details */
			ent = &eventNames[ eventId ];
			bzero( (caddr_t) &rei, sizeof(rei) );
			rei.rei_eventId = eventId;
			strncpy( rei.rei_name, ent->ent_name, sizeof(rei.rei_name) );
			strncpy( rei.rei_desc, ent->ent_desc, sizeof(rei.rei_desc) );
			WHENDEBUG( RFM_DBIOCTL )
			{
				xxlog( ucb, "eventId=%u, name='%s', desc='%s'",
					rei.rei_eventId, rei.rei_name, rei.rei_desc );
			}
			if( ddi_copyout( (caddr_t) &rei, (caddr_t) arg, sizeof(rei), flags ) )
			{
				goto BailOut;
			}
			retval = 0;
		}
		break;

		case RFM_GETEVENTINFOBYNAME:	/* Lookup event by name		 */
		{
			rfmEventInfo_t          rei;
			EVENTNAME               ent;
			ushort_t                eventId;

			spelling = "RFM_GETEVENTINFOBYNAME";
			/* Get argument (for the name) */
			if( ddi_copyin( (caddr_t) arg, (caddr_t) &rei, sizeof(rei), flags ) )
			{
				goto BailOut;
			}
			/* Linear search of the name */
			for( eventId = 0; eventId < RFM_EVENTID_NEVENTS; ++eventId )
			{
				ent = &eventNames[eventId];
				/* Maybe we'll match the whole shebang */
				if( Strncasecmp( rei.rei_name, ent->ent_name, sizeof(rei.rei_name) ) == 0 )
				{
					break;
				}
				/* Maybe we'll match w/o the prefix */
				if( Strncasecmp( rei.rei_name, ent->ent_name+LeventNamePrefix,
					sizeof(rei.rei_name) ) == 0 )
				{ 
					break;
				}
			}
			/* Did we get an answer? */
			if( eventId >= RFM_EVENTID_NEVENTS )
			{
				WHENDEBUG( RFM_DBIOCTL )
				{
					xxlog( ucb, "unknown event name (%16s)", rei.rei_name );
				}
				retval = EINVAL;
				goto BailOut;
			}
			/* Fill in and return the results */
			bzero( (caddr_t) &rei, sizeof(rei) );
			rei.rei_eventId = eventId;
			strncpy( rei.rei_name, ent->ent_name, sizeof(rei.rei_name) );
			strncpy( rei.rei_desc, ent->ent_desc, sizeof(rei.rei_desc) );
			WHENDEBUG( RFM_DBIOCTL )
			{
				xxlog( ucb, "eventId=%u, name='%s', desc='%s'",
					rei.rei_eventId, rei.rei_name, rei.rei_desc );
			}
			if( ddi_copyout( (caddr_t) &rei, (caddr_t) arg, sizeof(rei), flags ) )
			{
				goto BailOut;
			}
			retval = 0;
		}
		break;

		case RFM_CONFIGUNIT:		/* Configure unit for TCP/IP	*/
		{
			spelling = "RFM_CONFIGUNIT";
			WHENDEBUG( RFM_DBIOCTL )
			{
				xxlog( ucb, "ioctl( %s ) not supported", spelling );
			}
			retval = EIO;
		}
		break;

		case RFM_GSWAPPING:		/* Return RFM's byte swapping	 	*/
		{
			rfmSwapping_t	rsw;
 
                        spelling = "RFM_GSWAPPING";
			mutex_enter( &ucb->ucb_mu );
                               rsw.rsw_swapping = *(unsigned char *)(&ucb -> ucb_runtime_regs[0xC/4]);
			mutex_exit( &ucb->ucb_mu );
                        rsw.rsw_swapping &= 0x20;
                        WHENDEBUG( RFM_DBIOCTL )
			   {
            	            xxlog( ucb, "swapping is 0x%X", rsw.rsw_swapping );
                           }
                        if( ddi_copyout( (caddr_t) &rsw, (caddr_t) arg, sizeof(rsw), flags ) )
			{
				goto BailOut;
			}
			retval = 0;
		}
		break;

		case RFM_SWAPPING:		/* Set RFM's byte swapping			*/
		{
			rfmSwapping_t	rsw;

			spelling = "RFM_SWAPPING";
			/* Get a copy of argument */
			if( ddi_copyin( (caddr_t) arg, (caddr_t) &rsw, sizeof(rsw), flags ) )
			{
				goto BailOut;
			}
			/* Validate the requested mode */
			switch( rsw.rsw_swapping )
			{
				default:
				{
					WHENDEBUG( RFM_DBIOCTL )
					{
						xxlog( ucb, "swapping mode 0x%X unknown", rsw.rsw_swapping );
					}
				}
				retval = EINVAL;
				goto BailOut;

				case RFM_CSR3_ENDIAN_NONE:
					break;
				case RFM_CSR3_ENDIAN_BYTE:
					break;
				case RFM_CSR3_ENDIAN_WORD:
					break;
				case RFM_CSR3_ENDIAN_BOTH:
					break;
				case RFM_CSR3_ENDIAN_SIZE:
					break;
			}

#if 0
			/* We don't allow this if TCP/IP is enabled */
			if( has_TCPIP( ucb ) && (rsw.rsw_swapping != RFM_CSR3_ENDIAN_NONE) )
			{
				WHENDEBUG( RFM_DBIOCTL )
				{
					xxlog( ucb, "no swap with TCP/IP enabled" );

				}
				retval = EBUSY;
				goto BailOut;
			}
			rsw.rsw_swapping &= RFM_CSR3_ENDIAN_MASK;
#endif
			
			mutex_enter( &ucb->ucb_mu );
				if (rsw.rsw_swapping == RFM_CSR3_ENDIAN_NONE)
                               		*(unsigned char *)(&ucb -> ucb_runtime_regs[0xC/4]) &= ~0x20;
				else
                               		*(unsigned char *)(&ucb -> ucb_runtime_regs[0xC/4]) |= 0x20;
#if 0
			      rfm->rfm_csr3 = rsw.rsw_swapping;
                                       junk = rfm->rfm_csr3;    /* Flush */
#endif
			mutex_exit( &ucb->ucb_mu );
			WHENDEBUG( RFM_DBIOCTL )
			{
				xxlog( ucb, "swapping is now 0x%X", rsw.rsw_swapping );
			}
			retval = 0;
		}
		break;

		case RFM_GBYPASS:		/* Return RFM's bypass setting	 */
		{
			rfmBypass_t		rbp;
            uchar_t    		csr2;
 
            spelling = "RFM_GBYPASS";
            /* Get current bypass control flag */
			mutex_enter( &ucb->ucb_mu );
                               csr2 = rfm->rfm_csr2;
			mutex_exit( &ucb->ucb_mu );
            rbp.rbp_bypass = (csr2 & RFM_CSR2_NOBYPASS) ? 0 : ~0;
            WHENDEBUG( RFM_DBIOCTL )
			{
            	xxlog( ucb, "optical bypass %s", (rbp.rbp_bypass ? "ON" : "OFF") );
            }
            if( ddi_copyout( (caddr_t) &rbp, (caddr_t) arg, sizeof(rbp), flags ) )
			{
				goto BailOut;
			}
			retval = 0;
		}
		break;

		case RFM_BYPASS:		/* Manage RFM's bypass setting	 */
		{
			rfmBypass_t		rbp;
			uchar_t         csr2;

			spelling = "RFM_BYPASS";
			/* Get a copy of argument */
			if( ddi_copyin( (caddr_t) arg, (caddr_t) &rbp, sizeof(rbp), flags ) )
			{
				goto BailOut;
			}

			/* Set new bypass control flag */
			mutex_enter( &ucb->ucb_mu );
			     csr2 = rfm->rfm_csr2;
			     if( rbp.rbp_bypass )
			     {
				csr2 &= ~RFM_CSR2_NOBYPASS;
			     }
			     else
			     {
				csr2 |= RFM_CSR2_NOBYPASS;
			     }
			     rfm->rfm_csr2 = csr2;
                             junk = rfm->rfm_csr2;  /* Flush */
			mutex_exit( &ucb->ucb_mu );

			WHENDEBUG( RFM_DBIOCTL )
			{
				xxlog( ucb, "optical bypass now %s", (rbp.rbp_bypass ? "ON" : "OFF") );
			}
			retval = 0;
		}
		break;

		case RFM_EVENTENABLE:		/* Manage RFM events		 */
		{
			rfmEventEnable_t	ree;
            uchar_t             eventMask;
            uchar_t             icsr;
 
            spelling = "RFM_EVENTENABLE";
            /* Get a copy of argument */
            if( ddi_copyin( (caddr_t) arg, (caddr_t) &ree, sizeof(ree), flags ) )
			{
				goto BailOut;
			}
            /* Validate arguments */
            switch( ree.ree_eventId )
			{
            	default:
                {
                	WHENDEBUG( RFM_DBIOCTL )
					{
                    	xxlog( ucb, "event Id %u cannot be enabled or disabled",
                        	ree.ree_eventId );
                    }
                }
				retval = EINVAL;
                goto BailOut;

                case RFM_EVENTID_A:
                {
					eventMask = RFM_ICSR_INT1PENDING;
				}
                goto CheckForTcpIp;

                case RFM_EVENTID_B:
                {
                    eventMask = RFM_ICSR_INT2PENDING;

					CheckForTcpIp:

                    if( has_TCPIP( ucb ) )
					{
                    	WHENDEBUG( RFM_DBIOCTL )
						{
                        	xxlog( ucb, "event Id %u reserved for TCP/IP layer",
								ree.ree_eventId );
                        }
						retval = EINVAL;
                        goto BailOut;
                    }
                }
                break;

                case RFM_EVENTID_C:
                {
                	eventMask = RFM_ICSR_INT3PENDING;
                }
                break;
            }
			/* Perform the function */
			mutex_enter( &ucb->ucb_mu );
                            icsr = rfm->rfm_icsr;
                            ree.ree_oldState = (icsr & eventMask) ? 1 : 0;
            switch( ree.ree_newState )
			{
            	default:
                {
					mutex_exit( &ucb->ucb_mu );
                    WHENDEBUG( RFM_DBIOCTL )
					{
                    	xxlog( ucb, "unknown new state %u for event ID %u",
                            ree.ree_newState, ree.ree_eventId );
                    }
					retval = EINVAL;
                }
                goto BailOut;

                case RFM_EVENTENABLE_QUERY:
                	break;
                case RFM_EVENTENABLE_ON:
                {
                	rfm->rfm_icsr = (icsr | eventMask);
				}
                break;

                case RFM_EVENTENABLE_OFF:
                {
                	rfm->rfm_icsr = (icsr & ~eventMask);
                }
                break;
            }
			mutex_exit( &ucb->ucb_mu );
            /* Return results to caller */
            if( ddi_copyout( (caddr_t) &ree, (caddr_t) arg, sizeof(ree), flags ) )
			{
				goto BailOut;
			}
			retval = 0;
		}
		break;

		case RFM_DELAY:			/* Fast-resolution delay	 */
		{
			/* Establish the invariant stuff first */
			ECB		ecb = &ucb->ucb_ecbs[ RFM_EVENTID_DMA ];
			/* Local variables (if any) */
			rfmDelay_t	rdl;

			spelling = "RFM_DELAY";
            /* Get a copy of argument */
            if( ddi_copyin( (caddr_t) arg, (caddr_t) &rdl, sizeof(rdl), flags ) )
			{
				goto BailOut;
			}
			WHENDEBUG( RFM_DBIOCTL )
			{
				xxlog( ucb, "%u-usec fast timer", rdl.rdl_usec );
			}
			retval = 0;
			/* Start timer if we have a non-zero time */
			if( rdl.rdl_usec )
			{
				LOCK_ECB( ecb );
#if defined(_LP64)
                ecb->ecb_timeoutId = timeout( (void (*)(void *))timedout,
                                              ecb,
                                              drv_usectohz(rdl.rdl_usec) );
#else
                ecb->ecb_timeoutId = timeout( (void (*)())timedout,
                                              (caddr_t)ecb,
                                              drv_usectohz(rdl.rdl_usec) );
#endif
				ecb->ecb_flags |= ECB_FLAGS_WAIT;
				if( cv_wait_sig( &ecb->ecb_cv, &ecb->ecb_mutex ) == 0 )
				{
					/* Got signal before the event */
					if( ecb->ecb_timeoutId != (timeout_id_t)
-1 )
					{
						untimeout( ecb->ecb_timeoutId );
						ecb->ecb_timeoutId = (timeout_id_t)-1;
					}
					WHENDEBUG( RFM_DBTIMER )
					{
						xxlog( ucb, "event %s wait interrupted by signal", "RFM_DELAY" );
					}
					retval = EINTR;
					ecb->ecb_flags |= ECB_FLAGS_TIMO;
				}
				ecb->ecb_flags &= ~ECB_FLAGS_WAIT;
				UNLOCK_ECB( ecb );
			}
		}
		break;

		case RFM_GOPREG:		/* Get may OPREG settings	 */
		{
			/* Establish the invariant stuff first */
			RFMOR		rfmor = ucb->ucb_rfmor;
			/* Local variables (if any) */
			rfmGopreg_t	rgo;

			spelling = "RFM_GOPREG";
			/* Get current values (no locking, take chances) */
			bzero( (caddr_t) &rgo, sizeof(rgo) );
			rgo.rgo_rfmor.rfmor_mwar = ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_mwar);
			rgo.rgo_rfmor.rfmor_mwtc = ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_mwtc);
			rgo.rgo_rfmor.rfmor_mrar = ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_mrar);
			rgo.rgo_rfmor.rfmor_mrtc = ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_mrtc);
			rgo.rgo_rfmor.rfmor_intcsr = ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_intcsr);
			rgo.rgo_rfmor.rfmor_mcsr = ddi_getl(ucb->ucb_rfmor_accHndl,(uint32_t *) &rfmor->rfmor_mcsr);
			/* Return results to caller */
			if( ddi_copyout( (caddr_t) &rgo, (caddr_t) arg, sizeof(rgo), flags ) )
			{
				goto BailOut;
			}
			retval = 0;
		}
		break;
	}

	BailOut:

	WHENDEBUG( RFM_DBIOCTL )
	{
		xxlog( ucb, "%s complete (retval=%d)", spelling, retval );
	}
	if(retval != 0)
		xxlog(NULL,"retval = %d",retval);
	return( retval );
}

/*
 *------------------------------------------------------------------------
 * xxmmap: called by kernel to service a mmap(2) system call
 *------------------------------------------------------------------------
 * Our mmap routine gets called for each page of memory the user process
 * tries to map using mmap(2). Our routine actually gets called twice for
 * each page; the kernel checks to see that the mapping will succeed
 * before doing the mapping.
 *
 * To establish a user mapping, the kernel needs the PFN (page frame
 * number) of the physical memory, which we provide by calling
 * hat_getkpfnum() on an address mapped into the kernel. Once we have the
 * PFN, we no longer need to have that kernel mapping.
 *
 * The system does not need the device driver in order to maintain a user
 * mapping; nor does it need for the device to remain open by the user
 * process. So while our mmap routine gets called to perform a mapping, we
 * are not called at all as a result of munmap(2); and a process may close
 * our device while retaining - and using - the user mapping.
 *
 * This means that the mapping remains valid, even if the user has closed
 * the driver. This is unfortunate, since we will have free'd the memory
 * that we allocated as we opened the device. Isn't Solaris 2.3 wonderful?
 *------------------------------------------------------------------------
 */

static int xxmmap
(
	dev_t	dev,		/* Device to be mmap'ed 	 	*/
	off_t	off,		/* Beginning offset info region	*/
	int		prot		/* Permissions (ignored)	 	*/
)
{
	UCB		ucb;		/* Per-board info address	 	*/
	int		pfn;		/* Calculated Page Frame Number	*/
	caddr_t	kva;		/* Kernel virtual address		*/

	/* Make the UCB addressable */
	if( !(ucb = (UCB) ddi_get_soft_state(ucbs, getminor(dev))) )
	{
		WHENDEBUG( RFM_DBERROR )
		{
			xxlog( UNULL, "no (xxmmap) UCB for rfm(%d,%d)",
				(int) getmajor(dev), (int) getminor(dev) );
		}
		return( -1 );
	}
	WHENDEBUG( RFM_DBMMAP )
	{
		/*
		 * If we are mapping the entire RFM contents [we usually
		 * do], then this routine gets called for EACH AND EVERY
		 * virtual memory page. Since there are very, very many
		 * virtual memory pages worth of memory on RFM interfaces,
		 * we could swamp the system generating a log message each
		 * time that we are called.  Instead, to be polite, we 
		 * shall generate a log message only if the offset is zero
		 * [meaning this is the first call].
		 */
		if( off == 0 )
		{
			xxlog( ucb, "mmap( dev=0x%X, off=0x%X, prot=0x%X )",
				(int) dev, (int) off, (int) prot );
		}
	}
	if( off >= ucb->ucb_rgc.rgc_ramSize )
	{
		WHENDEBUG( RFM_DBERROR )
		{
			xxlog(ucb, "trying to mmap( 0x%x ) off end at 0x%x",
				off, ucb->ucb_rgc.rgc_ramSize );
		}
		return( -1 );
	}
	/*
	 * We have already mapped the device in, so simply return the PFN
	 * for the current offset.
	 */
	kva = ( (caddr_t) (ucb->ucb_rfm) ) + off;
	pfn = hat_getkpfnum( kva );

	if (pfn == 0xffffffff) {
	WHENDEBUG( RFM_DBMMAP )
	{
		xxlog( ucb, "PFN for kernel virtual address 0x%x (offset 0x%X) is 0x%X",
			kva, off, pfn );
	} }
	return( pfn );
}

/*
 *------------------------------------------------------------------------
 * When our driver is loaded or unloaded, the system calls our _init or
 * _fini routine with a modlinkage structure.  The modlinkage structure
 * contains:
 *
 *	modlinkage->
 *		modldrv->
 *			dev_ops->
 * 				cb_ops
 *
 * cb_ops contains the normal driver entry points and is roughly equivalent
 * to the cdevsw & bdevsw structures in previous releases.
 *
 * dev_ops contains, in addition to the pointer to cb_ops, the routines
 * that support loading and unloading our driver.
 *------------------------------------------------------------------------
 */

static struct cb_ops xx_cb_ops =
{
	xxopen,				/* Open the device		 			*/
	xxclose,			/* Close the device		 			*/
	nodev,				/* No block strategy entry point	*/
	xxprint,			/* Print routine		 			*/
	nodev,				/* No dump routine		 			*/
	xxread,				/* Read(2) entry point		 		*/
	xxwrite,			/* Write(2) entry point		 		*/
	xxioctl,			/* Everything's done here	 		*/
	nodev,				/* No devmap routine		 		*/
	xxmmap,				/* Mmap(2) entry point		 		*/
	ddi_segmap,			/* Default segmap entry point	 	*/
	nochpoll,			/* No chpoll routine		 		*/
	ddi_prop_op,		/* Standard prop_op handler	 		*/
	0,					/* Not a STREAMS driver		 		*/
	D_NEW | D_MP,		/* New format & MP safe 	 		*/
	CB_REV,				/* "cb_ops" revision number	 		*/
	nodev,				/* Async read			 			*/
	nodev				/* Async write			 			*/
};

static struct dev_ops xx_ops =
{
	DEVO_REV,				/* DEVO_REV indicated by manual	*/
	0,						/* Device reference count	 	*/
	xxgetinfo,				/* Info 			 			*/
	xxidentify,				/* Identify			 			*/
	xxprobe,				/* Probe			 			*/
	xxattach,				/* Attach			 			*/
	xxdetach,				/* Detach			 			*/
	nodev,					/* Reset			 			*/
	&xx_cb_ops,				/* Necessary backlink pos	 	*/
	(struct bus_ops *) 0,	/* Necessary bus operations pos	*/
	nodev					/* Power operations		 		*/
};

extern	struct	mod_ops mod_driverops;

static	struct modldrv modldrv =
{
	&mod_driverops,
	"RFM1 v" RFM_VERSION,
	&xx_ops,
};

static	struct modlinkage modlinkage =
{
	MODREV_1,			/* MODREV_1 indicated by manual	*/
	(void *) &modldrv,	/* Backlink to driver info	 	*/
	NULL,				/* List terminator		 		*/
};

/*
 *------------------------------------------------------------------------
 * _init: called when the driver is loaded
 *------------------------------------------------------------------------
 */

int _init
(
	void
)
{
	int		error;		/* Results */

	WHENDEBUG( RFM_DBPROBE )
	{
		xxlog( UNULL, "_init()" );
	}
	/* Create the busy flag and its protector */
	mutex_init( &busy_mu, "rfm busy mutex", MUTEX_DRIVER, NULL );
	busy = 0;
	/* Pre-allocate state space for 0 device (can add more later) */
	error = ddi_soft_state_init( &ucbs, sizeof( ucb_t ), 1 );
	if( error != 0 )
	{
		return( error );
	}
	/* Link us into the I/O and interrupt structure */
	error = mod_install( &modlinkage );
	if( error != 0 )
	{
		/* Release state memory allocated earlier */
		ddi_soft_state_fini( &ucbs );
	}
	else
	{

#if	0
		xxlog( UNULL, "DEBUG: RFM driver will never unload\n");
		busy = 1;		/* Prevent driver from unloading */
#endif	/* FIXME */

	}
	return( error );
}

/*
 *------------------------------------------------------------------------
 * _info: called to identify the driver
 *------------------------------------------------------------------------
 */

int _info
(
	struct modinfo	*modinfop
)
{
	/* Return info from our "rfm.conf" structures */
	return( mod_info( &modlinkage, modinfop ) );
}

/*
 *------------------------------------------------------------------------
 * _fini: called to unload the driver
 *------------------------------------------------------------------------
 * Release only those resources allocated by "_init()".
 *------------------------------------------------------------------------
 */

int _fini
(
	void
)
{
	int	error;

	if( 0 )
	{
		/* Still open, chum */
		error = -1;
		WHENDEBUG( RFM_DBPROBE )
		{
			xxlog( UNULL, "cannot unload driver; busy=%d", busy );
		}
	}
	else
	{
		WHENDEBUG( RFM_DBPROBE )
		{
			xxlog( UNULL, "unloading driver" );
		}
		error = mod_remove( &modlinkage );
		if( error != 0 )
		{
			return( error );
		}
		/* Free any per-module resources if any get allocated	*/
		/* Free state variable space				 			*/
		ddi_soft_state_fini( &ucbs );
	}
	return( error );
}

/* JHL - 63 */
/*
 *------------------------------------------------------------------------
 * ringBufHandler:  Performs ring buffer I/O 
 *------------------------------------------------------------------------
 * Three ring buffers, one for each interrupt level, 512 entries deep, 
 * three take and three put buffer pointers 
 *------------------------------------------------------------------------
 */

ushort_t ringBufHandler
(
	ringBuffer	*ucb_ringBuf,	/* pointer to ucb ringBuf struct		*/
	uint_t		opcode,		/* 0 = put, 1 = take				*/
	uchar_t		node 		/* sender id, ignored if opcode = 1		*/
)
{
	ushort_t rtnVal;

	if( opcode == 0 )
	{
		if( ((ucb_ringBuf->takePtr - ucb_ringBuf->putPtr) == 1) || 
			(ucb_ringBuf->takePtr == 0 && ucb_ringBuf->putPtr == ( RINGBUF_SIZE - 1 )) )
		{
			WHENDEBUG( RFM_DBRING )
			{
				xxlog( UNULL, "Ring buffer overflow: take = %d, put = %d",
					ucb_ringBuf->takePtr, ucb_ringBuf->putPtr );
			}
			return( RINGBUFFER_FULL );
		}
		else
		{
			WHENDEBUG( RFM_DBRING )
			{
				xxlog( UNULL, "Ring buffer put: take = %d, put = %d node = %d",
					ucb_ringBuf->takePtr, ucb_ringBuf->putPtr, node );
			}
			ucb_ringBuf->ringBuf[ ucb_ringBuf->putPtr ] = node;
			ucb_ringBuf->putPtr++;
			if( ucb_ringBuf->putPtr == RINGBUF_SIZE )
				ucb_ringBuf->putPtr = 0;
			return( RINGBUFFER_OK );
		}
	}
	else
	{
		if( ucb_ringBuf->takePtr == ucb_ringBuf->putPtr )
		{
			WHENDEBUG( RFM_DBRING )
			{
				xxlog( UNULL, "Ring buffer empty" );
			}
            return( RINGBUFFER_EMPTY );
		}
		else
		{
			rtnVal = (ushort_t)ucb_ringBuf->ringBuf[ ucb_ringBuf->takePtr ];
			WHENDEBUG( RFM_DBRING )
			{
				xxlog( UNULL, "Ring buffer take: take = %d, put = %d node = %d",
					ucb_ringBuf->takePtr, ucb_ringBuf->putPtr, rtnVal );
			}
			ucb_ringBuf->takePtr++;
			if( ucb_ringBuf->takePtr == RINGBUF_SIZE )
			{
				ucb_ringBuf->takePtr = 0;
			}
			ucb_ringBuf->eventNo++;
			return( rtnVal );
		}
	}
}
