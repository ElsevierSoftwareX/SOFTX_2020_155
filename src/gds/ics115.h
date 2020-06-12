/* Version: $Id$ */
#ifndef _ics115_h_
#define _ics115_h_

enum ICS115_DRIVER_COMMANDS {
ICS115_GET_MUTE=100,            /* 100 */
ICS115_SET_MUTE,
ICS115_GET_DIAG,
ICS115_GET_STATUS,
ICS115_GET_CONTROL,
ICS115_SET_CONTROL,
ICS115_GET_VSB_CONTROL,
ICS115_SET_VSB_CONTROL,
ICS115_GET_CONFIG,            
ICS115_SET_CONFIG,
ICS115_ENABLE_DAC,              /* 110 */
ICS115_DISABLE_DAC,
ICS115_FRAME_DAC,
ICS115_SET_CLOCK,
ICS115_BOARD_RESET,                     
ICS115_DAC_RESET,
ICS115_GET_SEQUENCER,                         
ICS115_SET_SEQUENCER,
ICS115_WAIT_DAC_INT,
ICS115_WAIT_VME_INT,
ICS115_SET_VME_INTERRUPT,       /* 120 */
ICS115_SET_VME_IVECTOR,                         
ICS115_SET_VME_A64_SECONDARY,
ICS115_VME_DMA,
ICS115_DIAG_ACCESS
};

#define ICS115_DEVICE_ALREADY_CREATED       -101
#define ICS115_ALLOCATE_DEVICE_FAILED       -102
#define ICS115_HARDWARE_INIT_FAILED         -103
#define ICS115_ADD_DEVICE_FAILED            -104
#define ICS115_INT_HANDLER_INSTALL_FAILED   -105
#define ICS115_CREATE_SEMAPHORE_FAILED      -106

#define ICS115_DRIVER_NOT_CREATED           -200
#define ICS115_DRIVER_IN_USE                -201
#define ICS115_DRIVER_NOT_IN_USE            -202


#define ICS115_MAX_DEV                      20

#define ICS115_MASTER       0
#define ICS115_SECONDARY    1

#define ICS115_DISABLE      0
#define ICS115_ENABLE       1

#define ICS115_CONT         0
#define ICS115_LOOP         1
#define ICS115_ONESHOT1     2
#define ICS115_ONESHOT2     3

#define ICS115_VME          0
#define ICS115_VSB          1
#define ICS115_FPDP         2
#define ICS115_FPDP_SUSP    3

#define ICS115_INTERNAL     0
#define ICS115_EXTERNAL     1
#define ICS115_PLL_INTERNAL 2
#define ICS115_PLL_EXTERNAL 3

#define ICS115_SYNC         0
#define ICS115_ASYNC        1

#define ICS115_LEVEL        0
#define ICS115_EDGE         1
#define ICS115_FRAME        3

#define ICS115_LEFT         0
#define ICS115_RIGHT        1

#define ICS115_A64          0
#define ICS115_A32          1
#define ICS115_A24          2

#define ICS115_NONE         0
#define ICS115_MBLT         1
#define ICS115_BLT          2

#define ICS115_ALTERNATE    1
#define ICS115_IO           2
#define ICS115_SYSTEM       3

#ifdef PROCESSOR_BAJA47
typedef struct {
		unsigned int    filler1 : 25;
		unsigned int    trig : 1;
		unsigned int    dir : 1;
		unsigned int    pio : 2;
		unsigned int    irq : 1;
		unsigned int    dacirq : 1;
		unsigned int    vmeirq : 1;
} ICS115_STAT;

typedef struct {
		unsigned int    filler1 : 16;
		unsigned int    pllrange : 2;
		unsigned int    en : 1;
		unsigned int    async : 1;
		unsigned int    trigmode : 2;
		unsigned int    trigsel : 1;
		unsigned int    clksel : 2;
		unsigned int    mode : 2;
		unsigned int    inpsel : 2;
		unsigned int    diag : 1;
		unsigned int    dacinten : 1;
		unsigned int    vmeinten : 1;
} ICS115_CONTROL;

typedef struct {
		unsigned int    filler1 : 11;
		unsigned int    respen : 1;
		unsigned int    callen : 1;
		unsigned int    casten : 1;
		unsigned int    space : 2;
		unsigned int    call : 4;
		unsigned int    cast : 4;
		unsigned int    respond : 8;
} ICS115_VSBCTRL;  

typedef struct {
		unsigned int    filler1 : 16;
		unsigned int    frame : 11;
		unsigned int    numchan : 5;
		unsigned int    filler2 : 12;
		unsigned int    split : 1;
		unsigned int    sblen : 19;
		unsigned int    filler3 : 24;
		unsigned int    dec : 8;
		unsigned int    filler4 : 16;
		unsigned int    fcount : 16;
} ICS115_CONFIG;

typedef struct {
		unsigned short  vector;
		unsigned short  level;
} ICS115_VMEINT;

typedef struct {
	unsigned long       address[2];
	unsigned long       count;
	int                 addressMode;
	int                 blockMode;
} ICS115_VMEDMA;

#elif defined(PROCESSOR_I486)

typedef struct {
		unsigned int    vmeinten : 1;
		unsigned int    dacinten : 1;
		unsigned int    diag : 1;
		unsigned int    inpsel : 2;
		unsigned int    mode : 2;
		unsigned int    clksel : 2;
		unsigned int    trigsel : 1;
		unsigned int    trigmode : 2;
		unsigned int    async : 1;
		unsigned int    en : 1;
		unsigned int    pllrange : 2;
		unsigned int    filler1 : 16;
} ICS115_CONTROL;

typedef struct {
		unsigned int    numchan : 5;
		unsigned int    frame : 11;
		unsigned int    filler1 : 16;

		unsigned int    sblen : 19;
		unsigned int    split : 1;
		unsigned int    filler2 : 12;

		unsigned int    dec : 8;
		unsigned int    filler3 : 24;

		unsigned int    fcount : 16;
		unsigned int    filler4 : 16;
} ICS115_CONFIG;

#else
#error "Define processor dependent structures"
#endif

#endif
