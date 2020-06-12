/* Version: $Id$ */
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: hardware.h						*/
/*                                                         		*/
/* Module Description: LIGO Data Acquisition System Reflective Memory   */
/* header file. This file defines the layout of the DAQ RFM.            */
/* This defines structures and field values for DAQ                     */
/* Interprocess Communication (IPC) and data transmission               */
/* from the DCU and EDCU to the Frame Builders.                         */
/*                                                         		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date    Engineer   Comments			   		*/
/* 00    01Jul98 R. Bork    First Release.		   		*/
/* 01    10Jul98 D. Barker  Added field defines, added EDCU offset.     */
/*                          Add channel data status defines.            */
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages:							*/
/*	References:							*/
/*                                                         		*/
/* Author Information:							*/
/*	Name          Telephone    Fax          e-mail 			*/
/*	David Barker. (509)3736203 (509)3722178 barker@ligo.caltech.edu */
/*                                                         		*/
/* Code Compilation and Runtime Specifications:				*/
/*	Code Compiled on: Sun Ultra Enterprise 2 running Solaris2.5.1   */
/*	Compiler Used: Heurikon's gcc-sde				*/
/*	Runtime environment: Baja47 running VxWorks 5.2 Beta B.		*/
/*                                                         		*/
/* Code Standards Conformance:						*/
/*	Code Conforms to: LIGO standards.	TBD			*/
/*			  Lint.			TBD			*/
/*			  ANSI			TBD			*/
/*			  POSIX			TBD			*/
/*									*/
/* Known Bugs, Limitations, Caveats:					*/
/*	BUGS LIMITATIONS AND CAVEATS					*/
/*                                                         		*/
/*                      -------------------                             */
/*                                                         		*/
/*                             LIGO					*/
/*                                                         		*/
/*        THE LASER INTERFEROMETER GRAVITATIONAL WAVE OBSERVATORY.	*/
/*                                                         		*/
/*                     (C) The LIGO Project, 1997.			*/
/*                                                         		*/
/*                                                         		*/
/* California Institute of Technology			   		*/
/* LIGO Project MS 51-33				   		*/
/* Pasadena CA 91125					   		*/
/*                                                         		*/
/* Massachusetts Institute of Technology		   		*/
/* LIGO Project MS 20B-145				   		*/
/* Cambridge MA 01239					   		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/


/* Include File Duplication Lock */
#ifndef _GDS_HARDWARE_H
#define _GDS_HARDWARE_H

#include "dtt/gdsmain.h"
#include "dtt/targets.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "PConfig.h"

/* GDS configuration */
#if defined(LIGO_GDS)

#define RMEM_LAYOUT 			2

#if (TARGET == TARGET_M_GDS_UNIX || TARGET == TARGET_L_GDS_UNIX || TARGET == TARGET_H_GDS_UNIX || TARGET == TARGET_G_GDS_UNIX || TARGET == TARGET_C_GDS_UNIX )
#define GDS_UNIX_TARGET
#endif

#ifdef OS_SOLARIS
#define GDS_UNIX_TARGET
#endif

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Reflective memory modules						*/
/*                                                         		*/
/* master - if non-zero, board gets initializes at startup	 	*/
/* base address - base address of reflective memory			*/
/* board offset - offset to base address, where the start address of	*/
/*                of the specified board is located			*/
/* mem size - board reflective memory size				*/
/* int level - VME interrupt level					*/
/* int vec - VME interrupt vector					*/
/* node ID - RM node ID							*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

/******* first board */
/* LHO 4K LVEA excitation engine */
#if defined(OS_VXWORKS) && (TARGET == TARGET_H1_GDS_AWG1)
#define VMIVME5888_0_MASTER		1
#define VMIVME5588_0_BASE_ADDRESS	0x50000000
#define VMIVME5588_0_ADRMOD		0x09
#define VMIVME5588_0_BOARD_OFFSET	0x00200000
#define VMIVME5588_0_MEM_SIZE		0x00200000
#define VMIVME5588_0_INT_LEVEL      	5
#define VMIVME5588_0_INT_VEC     	0x8a
#define VMIVME5588_0_DEFINED
#endif

/* LHO 4K LVEA excitation engine & TP manager: INTEL */
#if defined(OS_VXWORKS) && (TARGET == TARGET_H1_GDS_AWG1 + 10)
#define RMEM_LAYOUT 			1
#define VMIVME5888_0_MASTER		1
#define VMIVME5588_0_BOARD_OFFSET	0
#define VMIVME5588_0_BASE_ADDRESS	0
#define VMIVME5588_0_ADRMOD		0x09
#define VMIVME5588_0_MEM_SIZE		(64*1024*1024)
#define VMIVME5588_0_INT_LEVEL      	5
#define VMIVME5588_0_INT_VEC     	0x8a
#define VMIVME5588_0_DEFINED
#endif

/* LHO 2K LVEA excitation engine */
#if defined(OS_VXWORKS) && (TARGET == TARGET_H2_GDS_AWG1)
#define VMIVME5888_0_MASTER		1
#define VMIVME5588_0_BASE_ADDRESS	0x50000000
#define VMIVME5588_0_ADRMOD		0x09
#define VMIVME5588_0_BOARD_OFFSET	0x00200000
#define VMIVME5588_0_MEM_SIZE		0x00200000
#define VMIVME5588_0_INT_LEVEL      	5
#define VMIVME5588_0_INT_VEC     	0x8a
#define VMIVME5588_0_DEFINED
#endif

/* LHO 2K LVEA excitation engine & TP manager: INTEL */
#if defined(OS_VXWORKS) && (TARGET == TARGET_H2_GDS_AWG1 + 10)
#define RMEM_LAYOUT 			1
#define VMIVME5888_0_MASTER		1
#define VMIVME5588_0_BOARD_OFFSET	0
#define VMIVME5588_0_BASE_ADDRESS	0
#define VMIVME5588_0_ADRMOD		0x09
#define VMIVME5588_0_MEM_SIZE		(64*1024*1024)
#define VMIVME5588_0_INT_LEVEL      	5
#define VMIVME5588_0_INT_VEC     	0x8a
#define VMIVME5588_0_DEFINED
#endif

/* LHO test point manager */
#if defined(OS_VXWORKS) && (TARGET == TARGET_H_RM_MANAGER)
#define VMIVME5888_0_MASTER		0
#define VMIVME5588_0_BASE_ADDRESS	0x60000000
#define VMIVME5588_0_ADRMOD		0x09
#define VMIVME5588_0_BOARD_OFFSET	0x00000000
#define VMIVME5588_0_MEM_SIZE		0x00400000
#define VMIVME5588_0_INT_LEVEL      	5
#define VMIVME5588_0_INT_VEC     	0x8a
#define VMIVME5588_0_DEFINED
#endif

/* LHO test point manager: INTEL */
#if defined(OS_VXWORKS) && (TARGET == TARGET_H_RM_MANAGER + 10)
#define VMIVME5888_0_MASTER		0
#define VMIVME5588_0_BASE_ADDRESS	0x60000000
#define VMIVME5588_0_ADRMOD		0x0D
#define VMIVME5588_0_BOARD_OFFSET	0x00000000
#define VMIVME5588_0_MEM_SIZE		0x00400000
#define VMIVME5588_0_INT_LEVEL      	5
#define VMIVME5588_0_INT_VEC     	0x8a
#define VMIVME5588_0_DEFINED
#endif

/* LLO 4K LVEA excitation engine */
#if defined(OS_VXWORKS) && (TARGET == TARGET_L1_GDS_AWG1)
#define VMIVME5888_0_MASTER		1
#define VMIVME5588_0_BASE_ADDRESS	0x50000000
#define VMIVME5588_0_ADRMOD		0x09
#define VMIVME5588_0_BOARD_OFFSET	0x00200000
#define VMIVME5588_0_MEM_SIZE		0x00200000
#define VMIVME5588_0_INT_LEVEL      	5
#define VMIVME5588_0_INT_VEC     	0x8a
#define VMIVME5588_0_DEFINED
#endif

/* LLO 4K LVEA excitation engine & TP manager: INTEL */
#if defined(OS_VXWORKS) && (TARGET == TARGET_L1_GDS_AWG1 + 10)
#define RMEM_LAYOUT 			1
#define VMIVME5888_0_MASTER		1
#define VMIVME5588_0_BOARD_OFFSET	0
#define VMIVME5588_0_BASE_ADDRESS	0
#define VMIVME5588_0_ADRMOD		0x09
#define VMIVME5588_0_MEM_SIZE		(64*1024*1024)
#define VMIVME5588_0_INT_LEVEL      	5
#define VMIVME5588_0_INT_VEC     	0x8a
#define VMIVME5588_0_DEFINED
#endif

/* CIT 40m excitation engine & TP manager: INTEL */
#if defined(OS_VXWORKS) && (TARGET == TARGET_L1_GDS_AWG1 + 20)
#define RMEM_LAYOUT 			1
#define VMIVME5888_0_MASTER		1
#define VMIVME5588_0_BOARD_OFFSET	0
#define VMIVME5588_0_BASE_ADDRESS	0
#define VMIVME5588_0_ADRMOD		0x09
#define VMIVME5588_0_MEM_SIZE		(64*1024*1024)
#define VMIVME5588_0_INT_LEVEL      	5
#define VMIVME5588_0_INT_VEC     	0x8a
#define VMIVME5588_0_DEFINED
#endif

/* MIT excitation engine & TP manager: Baja */
#if defined(OS_VXWORKS) && (TARGET == TARGET_L1_GDS_AWG1 + 21)
#define RMEM_LAYOUT 			1
#define VMIVME5888_0_MASTER		1
#define VMIVME5588_0_BOARD_OFFSET	0
#define VMIVME5588_0_BASE_ADDRESS	0
#define VMIVME5588_0_ADRMOD		0x09
#define VMIVME5588_0_MEM_SIZE		(64*1024*1024)
#define VMIVME5588_0_INT_LEVEL      	5
#define VMIVME5588_0_INT_VEC     	0x8a
#define VMIVME5588_0_DEFINED
#endif

/* LLO test point manager */
#if defined(OS_VXWORKS) && (TARGET == TARGET_L_RM_MANAGER)
#define VMIVME5888_0_MASTER		0
#define VMIVME5588_0_BASE_ADDRESS	0x50000000
#define VMIVME5588_0_ADRMOD		0x09
#define VMIVME5588_0_BOARD_OFFSET	0x00000000
#define VMIVME5588_0_MEM_SIZE		0x00400000
#define VMIVME5588_0_INT_LEVEL      	5
#define VMIVME5588_0_INT_VEC     	0x8a
#define VMIVME5588_0_DEFINED
#endif

/* LLO test point manager: INTEL */
#if defined(OS_VXWORKS) && (TARGET == TARGET_L_RM_MANAGER + 10)
#define RMEM_LAYOUT 			1
#define VMIVME5888_0_MASTER		0
#define VMIVME5588_0_BASE_ADDRESS	0x50000000
#define VMIVME5588_0_ADRMOD		0x0D
#define VMIVME5588_0_BOARD_OFFSET	0x00000000
#define VMIVME5588_0_MEM_SIZE		0x00400000
#define VMIVME5588_0_INT_LEVEL      	5
#define VMIVME5588_0_INT_VEC     	0x8a
#define VMIVME5588_0_DEFINED
#endif

/* default VxWorks */
#if defined(OS_VXWORKS) && !defined (VMIVME5588_0_DEFINED)
#define VMIVME5888_0_MASTER		1
#define VMIVME5588_0_BASE_ADDRESS	0x50000000
#define VMIVME5588_0_ADRMOD		0x09
#define VMIVME5588_0_BOARD_OFFSET	0x00200000
#define VMIVME5588_0_MEM_SIZE		0x00200000
#define VMIVME5588_0_INT_LEVEL      	5
#define VMIVME5588_0_INT_VEC     	0x8a
#define VMIVME5588_0_DEFINED
#endif

/* default others */
#ifndef VMIVME5588_0_DEFINED
#define VMIVME5888_0_MASTER		0
#define VMIVME5588_0_BASE_ADDRESS	0x00000000
#define VMIVME5588_0_ADRMOD		0x00
#define VMIVME5588_0_BOARD_OFFSET	0x00000000
#define VMIVME5588_0_MEM_SIZE		0x00000000
#define VMIVME5588_0_INT_LEVEL      	0
#define VMIVME5588_0_INT_VEC     	0x00
#define VMIVME5588_0_DEFINED
#endif

/******* second board */
/* LHO test point manager */
#if defined(OS_VXWORKS) && (TARGET == TARGET_H_RM_MANAGER)
#define VMIVME5888_1_MASTER		0
#define VMIVME5588_1_BASE_ADDRESS	0x50000000
#define VMIVME5588_1_ADRMOD		0x09
#define VMIVME5588_1_BOARD_OFFSET	0x00000000
#define VMIVME5588_1_MEM_SIZE		0x00400000
#define VMIVME5588_1_INT_LEVEL		5
#define VMIVME5588_1_INT_VEC		0x9b
#define VMIVME5588_1_DEFINED
#endif

/* LHO test point manager: INTEL */
#if defined(OS_VXWORKS) && (TARGET == TARGET_H_RM_MANAGER + 10)
#define VMIVME5888_1_MASTER		0
#define VMIVME5588_1_BASE_ADDRESS	0x50000000
#define VMIVME5588_1_ADRMOD		0x0D
#define VMIVME5588_1_BOARD_OFFSET	0x00000000
#define VMIVME5588_1_MEM_SIZE		0x00400000
#define VMIVME5588_1_INT_LEVEL		5
#define VMIVME5588_1_INT_VEC		0x9b
#define VMIVME5588_1_DEFINED
#endif

/* others */
#ifndef VMIVME5588_1_DEFINED
#define VMIVME5888_1_MASTER		0
#define VMIVME5588_1_BASE_ADDRESS	0x00000000
#define VMIVME5588_1_ADRMOD		0x00
#define VMIVME5588_1_BOARD_OFFSET	0x00000000
#define VMIVME5588_1_MEM_SIZE		0x00000000
#define VMIVME5588_1_INT_LEVEL		0
#define VMIVME5588_1_INT_VEC		0x00
#define VMIVME5588_1_DEFINED
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Digital to analog converters						*/
/*                                                         		*/
/* base address - base address of board					*/
/* address modifier - VME address modifier     				*/
/* chn num - number of DAC channel on board				*/
/*                of the specified board is located			*/
/* int level - VME interrupt level					*/
/* int vec - VME interrupt vector					*/
/* use timing card - use CDS timing card for synchronization		*/
/*     otherwise the heartbeat is used					*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

#if defined(OS_VXWORKS) && (TARGET == TARGET_H1_GDS_AWG1)
#define ICS115_0_BASE_ADDRESS		0x60000000
#define ICS115_0_ADRMOD			0x09
#define ICS115_0_CHN_NUM		32
#define ICS115_0_CONVERSION		(32767.0 / 10.0 / 1.08)
#define ICS115_0_INT_LEVEL      	3
#define ICS115_0_INT_VEC        	0xac
#define ICS115_0_USE_TIMINGCARD
#define ICS115_0_DEFINED
#endif

#if defined(OS_VXWORKS) && (TARGET == TARGET_H1_GDS_AWG1 + 10)
#define ICS115_0_BASE_ADDRESS		0x60000000
#define ICS115_0_ADRMOD			0x0D
#define ICS115_0_CHN_NUM		32
#define ICS115_0_CONVERSION		(32767.0 / 10.0 / 1.08)
#define ICS115_0_INT_LEVEL      	3
#define ICS115_0_INT_VEC        	0xac
#define ICS115_0_USE_TIMINGCARD
#define ICS115_0_DEFINED
#endif

#if defined(OS_VXWORKS) && (TARGET == TARGET_H1_GDS_AWG2)
#define ICS115_0_BASE_ADDRESS		0x60000000
#define ICS115_0_ADRMOD			0x09
#define ICS115_0_CHN_NUM		4
#define ICS115_0_CONVERSION		(32767.0 / 10.0 / 1.08)
#define ICS115_0_INT_LEVEL      	3
#define ICS115_0_INT_VEC        	0xac
#define ICS115_0_DEFINED
#endif

#if defined(OS_VXWORKS) && (TARGET == TARGET_H1_GDS_AWG3)
#define ICS115_0_BASE_ADDRESS		0x60000000
#define ICS115_0_ADRMOD			0x09
#define ICS115_0_CHN_NUM		4
#define ICS115_0_CONVERSION		(32767.0 / 10.0 / 1.08)
#define ICS115_0_INT_LEVEL      	3
#define ICS115_0_INT_VEC        	0xac
#define ICS115_0_DEFINED
#endif

#if defined(OS_VXWORKS) && (TARGET == TARGET_H2_GDS_AWG1)
#define ICS115_0_BASE_ADDRESS		0x60000000
#define ICS115_0_ADRMOD			0x09
#define ICS115_0_CHN_NUM		32
#define ICS115_0_CONVERSION		(32767.0 / 10.0 / 1.08)
#define ICS115_0_INT_LEVEL      	3
#define ICS115_0_INT_VEC        	0xac
#define ICS115_0_USE_TIMINGCARD
#define ICS115_0_DEFINED
#endif

#if defined(OS_VXWORKS) && (TARGET == TARGET_H2_GDS_AWG1 + 10)
#define ICS115_0_BASE_ADDRESS		0x60000000
#define ICS115_0_ADRMOD			0x0D
#define ICS115_0_CHN_NUM		32
#define ICS115_0_CONVERSION		(32767.0 / 10.0 / 1.08)
#define ICS115_0_INT_LEVEL      	3
#define ICS115_0_INT_VEC        	0xac
#define ICS115_0_USE_TIMINGCARD
#define ICS115_0_DEFINED
#endif

#if defined(OS_VXWORKS) && (TARGET == TARGET_H2_GDS_AWG2)
#define ICS115_0_BASE_ADDRESS		0x60000000
#define ICS115_0_ADRMOD			0x09
#define ICS115_0_CHN_NUM		4
#define ICS115_0_CONVERSION		(32767.0 / 10.0 / 1.08)
#define ICS115_0_INT_LEVEL      	3
#define ICS115_0_INT_VEC        	0xac
#define ICS115_0_DEFINED
#endif

#if defined(OS_VXWORKS) && (TARGET == TARGET_H2_GDS_AWG3)
#define ICS115_0_BASE_ADDRESS		0x60000000
#define ICS115_0_ADRMOD			0x09
#define ICS115_0_CHN_NUM		4
#define ICS115_0_CONVERSION		(32767.0 / 10.0 / 1.08)
#define ICS115_0_INT_LEVEL      	3
#define ICS115_0_INT_VEC        	0xac
#define ICS115_0_DEFINED
#endif

#if defined(OS_VXWORKS) && (TARGET == TARGET_L1_GDS_AWG1) 
#define ICS115_0_BASE_ADDRESS		0x60000000
#define ICS115_0_ADRMOD			0x09
#define ICS115_0_CHN_NUM		32
#define ICS115_0_CONVERSION		(32767.0 / 10.0 / 1.08)
#define ICS115_0_INT_LEVEL      	3
#define ICS115_0_INT_VEC        	0xac
#define ICS115_0_USE_TIMINGCARD
#define ICS115_0_DEFINED
#endif

#if defined(OS_VXWORKS) && (TARGET == TARGET_L1_GDS_AWG1 + 10)
#define ICS115_0_BASE_ADDRESS		0x60000000
#define ICS115_0_ADRMOD			0x0D
#define ICS115_0_CHN_NUM		32
#define ICS115_0_CONVERSION		(32767.0 / 10.0 / 1.08)
#define ICS115_0_INT_LEVEL      	3
#define ICS115_0_INT_VEC        	0xac
#define ICS115_0_USE_TIMINGCARD
#define ICS115_0_DEFINED
#endif

#if defined(OS_VXWORKS) && (TARGET == TARGET_L1_GDS_AWG1 + 21)
#define ICS115_0_BASE_ADDRESS		0x60000000
#define ICS115_0_ADRMOD			0x09
#define ICS115_0_CHN_NUM		32
#define ICS115_0_CONVERSION		(32767.0 / 10.0 / 1.08)
#define ICS115_0_INT_LEVEL      	3
#define ICS115_0_INT_VEC        	0xac
#define ICS115_0_USE_TIMINGCARD
#define ICS115_0_DEFINED
#endif

#if defined(OS_VXWORKS) && (TARGET == TARGET_L1_GDS_AWG2)
#define ICS115_0_BASE_ADDRESS		0x60000000
#define ICS115_0_ADRMOD			0x09
#define ICS115_0_CHN_NUM		4
#define ICS115_0_CONVERSION		(32767.0 / 10.0 / 1.08)
#define ICS115_0_INT_LEVEL      	3
#define ICS115_0_INT_VEC        	0xac
#define ICS115_0_DEFINED
#endif

#if defined(OS_VXWORKS) && (TARGET == TARGET_L1_GDS_AWG3)
#define ICS115_0_BASE_ADDRESS		0x60000000
#define ICS115_0_ADRMOD			0x09
#define ICS115_0_CHN_NUM		4
#define ICS115_0_CONVERSION		(32767.0 / 10.0 / 1.08)
#define ICS115_0_INT_LEVEL      	3
#define ICS115_0_INT_VEC        	0xac
#define ICS115_0_DEFINED
#endif

#ifndef ICS115_0_DEFINED
#define ICS115_0_BASE_ADDRESS		0x00000000
#define ICS115_0_ADRMOD			0x00
#define ICS115_0_CHN_NUM		0
#define ICS115_0_CONVERSION		1.0
#define ICS115_0_INT_LEVEL      	0
#define ICS115_0_INT_VEC        	0x00
#define ICS115_0_DEFINED
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* CDS timing card							*/
/*                                                         		*/
/* base address - base address of board					*/
/* address modifier - VME address modifier     				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

#if defined(OS_VXWORKS) && (TARGET == TARGET_H1_GDS_AWG1)
#define TIMINGCARD_0_BASE_ADDRESS	0xE000
#define TIMINGCARD_0_ADRMOD		0x29
#define TIMINGCARD_0_DEFINED
#endif

#if defined(OS_VXWORKS) && (TARGET == TARGET_H1_GDS_AWG1 + 10)
#define TIMINGCARD_0_BASE_ADDRESS	0xE000
#define TIMINGCARD_0_ADRMOD		0x2D
#define TIMINGCARD_0_DEFINED
#endif

#if defined(OS_VXWORKS) && (TARGET == TARGET_H2_GDS_AWG1)
#define TIMINGCARD_0_BASE_ADDRESS	0xE000
#define TIMINGCARD_0_ADRMOD		0x29
#define TIMINGCARD_0_DEFINED
#endif

#if defined(OS_VXWORKS) && (TARGET == TARGET_H2_GDS_AWG1 + 10)
#define TIMINGCARD_0_BASE_ADDRESS	0xE000
#define TIMINGCARD_0_ADRMOD		0x2D
#define TIMINGCARD_0_DEFINED
#endif

#if defined(OS_VXWORKS) && (TARGET == TARGET_L1_GDS_AWG1)
#define TIMINGCARD_0_BASE_ADDRESS	0xE000
#define TIMINGCARD_0_ADRMOD		0x29
#define TIMINGCARD_0_DEFINED
#endif

#if defined(OS_VXWORKS) && (TARGET == TARGET_L1_GDS_AWG1 + 10)
#define TIMINGCARD_0_BASE_ADDRESS	0xE000
#define TIMINGCARD_0_ADRMOD		0x2D
#define TIMINGCARD_0_DEFINED
#endif

#if defined(OS_VXWORKS) && (TARGET == TARGET_L1_GDS_AWG1 + 20)
#define TIMINGCARD_0_BASE_ADDRESS	0xE000
#define TIMINGCARD_0_ADRMOD		0x2D
#define TIMINGCARD_0_DEFINED
#endif

#if defined(OS_VXWORKS) && (TARGET == TARGET_L1_GDS_AWG1 + 21)
#define TIMINGCARD_0_BASE_ADDRESS	0xE000
#define TIMINGCARD_0_ADRMOD		0x29
#define TIMINGCARD_0_DEFINED
#endif

#ifndef TIMINGCARD_0_DEFINED
#define TIMINGCARD_0_BASE_ADDRESS	0x0000
#define TIMINGCARD_0_ADRMOD		0x00
#define TIMINGCARD_0_DEFINED
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* GPS clock								*/
/*                                                         		*/
/* type - board type:							*/
/*        type 0 - standard GPS clock; 3MHz secondary clock		*/
/*        type 1 - older GPS clock; 131072Hz secondary clock		*/
/* base address - base address of board					*/
/* address modifier - VME address modifier of board			*/
/* master - if non-zero indicates a baord with a GPS receiver, 		*/
/*          otherwise it is just an IRIG B secondary			*/
/* int source - source of interrupt					*/
/* 		type 0 - interrupt generated @ 16Hz by other Baja	*/
/* 		type 1 - interrupt generated @ 64Hz by GPS board	*/
/* int level - VME interrupt level					*/
/* int vec - VME interrupt vector					*/
/* ntp server - IP address of an NTP server (used to set the year)	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/


/* LHO test point manager */
#if defined(OS_VXWORKS) && (TARGET == TARGET_H_RM_MANAGER)
#define SYNCCLOCK32_0_BASE_ADDRESS	0x010000
#define SYNCCLOCK32_0_ADRMOD		0x39
#define SYNCCLOCK32_0_MASTER		1
#define SYNCCLOCK32_0_INT_SOURCE	1
#define SYNCCLOCK32_0_INT_LEVEL		6
#define SYNCCLOCK32_0_INT_VEC		0x19
#define SYNCCLOCK32_0_DEFINED
#endif

/* LHO test point manager: INTEL */
#if defined(OS_VXWORKS) && (TARGET == TARGET_H_RM_MANAGER + 10)
#define SYNCCLOCK32_0_BASE_ADDRESS	0x010000
#define SYNCCLOCK32_0_ADRMOD		0x3D
#define SYNCCLOCK32_0_MASTER		1
#define SYNCCLOCK32_0_INT_SOURCE	1
#define SYNCCLOCK32_0_INT_LEVEL		6
#define SYNCCLOCK32_0_INT_VEC		0x19
#define SYNCCLOCK32_0_DEFINED
#endif

/* LHO 4K LVEA excitation engine & TP manager: INTEL */
#if defined(OS_VXWORKS) && (TARGET == TARGET_H1_GDS_AWG1 + 10)
#define SYNCCLOCK32_0_BASE_ADDRESS	0x010000
#define SYNCCLOCK32_0_ADRMOD		0x3D
#define SYNCCLOCK32_0_MASTER		0
#define SYNCCLOCK32_0_INT_SOURCE	1
#define SYNCCLOCK32_0_INT_LEVEL		6
#define SYNCCLOCK32_0_INT_VEC		0x19
#define SYNCCLOCK32_0_DEFINED
#endif

/* LHO 2K LVEA excitation engine & TP manager: INTEL */
#if defined(OS_VXWORKS) && (TARGET == TARGET_H2_GDS_AWG1 + 10)
#define SYNCCLOCK32_0_BASE_ADDRESS	0x010000
#define SYNCCLOCK32_0_ADRMOD		0x3D
#define SYNCCLOCK32_0_MASTER		0
#define SYNCCLOCK32_0_INT_SOURCE	1
#define SYNCCLOCK32_0_INT_LEVEL		6
#define SYNCCLOCK32_0_INT_VEC		0x19
#define SYNCCLOCK32_0_DEFINED
#endif

/* LLO test point manager */
#if defined(OS_VXWORKS) && (TARGET == TARGET_L_RM_MANAGER)
#define SYNCCLOCK32_0_BASE_ADDRESS	0x010000
#define SYNCCLOCK32_0_ADRMOD		0x39
#define SYNCCLOCK32_0_MASTER		1
#define SYNCCLOCK32_0_INT_SOURCE	0
#define SYNCCLOCK32_0_INT_LEVEL		6
#define SYNCCLOCK32_0_INT_VEC		0x19
#define SYNCCLOCK32_0_DEFINED
#endif

/* LLO test point manager: INTEL */
#if defined(OS_VXWORKS) && (TARGET == TARGET_L_RM_MANAGER + 10)
#define SYNCCLOCK32_0_BASE_ADDRESS	0x010000
#define SYNCCLOCK32_0_ADRMOD		0x3D
#define SYNCCLOCK32_0_MASTER		1
#define SYNCCLOCK32_0_INT_SOURCE	1
#define SYNCCLOCK32_0_INT_LEVEL		6
#define SYNCCLOCK32_0_INT_VEC		0x19
#define SYNCCLOCK32_0_DEFINED
#endif

/* CIT 40m excitation engine & TP manager: INTEL */
#if defined(OS_VXWORKS) && (TARGET == TARGET_L1_GDS_AWG1 + 20)
#define SYNCCLOCK32_0_BASE_ADDRESS	0x010000
#define SYNCCLOCK32_0_ADRMOD		0x3D
#define SYNCCLOCK32_0_MASTER		0
#define SYNCCLOCK32_0_INT_SOURCE	1
#define SYNCCLOCK32_0_INT_LEVEL		6
#define SYNCCLOCK32_0_INT_VEC		0x19
#define SYNCCLOCK32_0_DEFINED
#endif

/* LLO excitation engine & TP manger: INETL */
#if defined(OS_VXWORKS) && (TARGET == TARGET_L1_GDS_AWG1 + 10)
#define SYNCCLOCK32_0_BASE_ADDRESS	0x010000
#define SYNCCLOCK32_0_ADRMOD		0x3D
#define SYNCCLOCK32_0_MASTER		0
#define SYNCCLOCK32_0_INT_SOURCE	1
#define SYNCCLOCK32_0_INT_LEVEL		6
#define SYNCCLOCK32_0_INT_VEC		0x19
#define SYNCCLOCK32_0_DEFINED
#endif

/* MIT excitation engine & TP manager: Baja */
#if defined(OS_VXWORKS) && (TARGET == TARGET_L1_GDS_AWG1 + 21)
#define SYNCCLOCK32_0_BASE_ADDRESS	0x010000
#define SYNCCLOCK32_0_ADRMOD		0x39
#define SYNCCLOCK32_0_MASTER		1
#define SYNCCLOCK32_0_INT_SOURCE	1
#define SYNCCLOCK32_0_INT_LEVEL		6
#define SYNCCLOCK32_0_INT_VEC		0x19
#define SYNCCLOCK32_0_DEFINED
#endif

/* default VxWorks */
#if defined(OS_VXWORKS) && !defined (SYNCCLOCK32_0_DEFINED)
#define SYNCCLOCK32_0_BASE_ADDRESS	0x010000
#define SYNCCLOCK32_0_ADRMOD		0x39
#define SYNCCLOCK32_0_MASTER		0
#define SYNCCLOCK32_0_INT_SOURCE	1
#define SYNCCLOCK32_0_INT_LEVEL		6
#define SYNCCLOCK32_0_INT_VEC		0x19
#define SYNCCLOCK32_0_DEFINED
#endif

/* all other cases */
#ifndef SYNCCLOCK32_0_DEFINED
#define SYNCCLOCK32_0_BASE_ADDRESS	0
#define SYNCCLOCK32_0_ADRMOD		0x00
#define SYNCCLOCK32_0_INT_SOURCE	0
#define SYNCCLOCK32_0_INT_LEVEL		0
#define SYNCCLOCK32_0_INT_VEC		0
#define SYNCCLOCK32_0_MASTER		0
#define SYNCCLOCK32_0_DEFINED
#endif

/* ntp server address */
#if (SITE == GDS_SITE_MIT)
#define SYNCCLOCK32_0_NTPSERVER		"10.200.0.1"
#elif (SITE == GDS_SITE_CIT)
#define SYNCCLOCK32_0_NTPSERVER		"131.215.113.201"
#elif (SITE == GDS_SITE_LLO)
#define SYNCCLOCK32_0_NTPSERVER		"10.100.0.3"
#else
#define SYNCCLOCK32_0_NTPSERVER		"10.1.240.1"
#endif


#endif /* LIGO_GDS */

#ifdef __cplusplus
}
#endif
#if RMEM_LAYOUT != 2
#error
#endif
#endif /* _GDS_HARDWARE_H */
