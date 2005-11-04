/*----------------------------------------------------------------------*/
/*                                                                      */
/* Module Name: vmic5579.h                                              */
/*                                                                      */
/* Module Description: Definitions for vmic5579 RFM modules             */
/*                                                                      */
/* Module Arguments:                                                    */
/*                                                                      */
/* Revision History:                                                    */
/* Rel   Date    Engineer   Comments                                    */
/* 1.0  29Oct99  R. Bork                                                */
/*                                                                      */
/* Documentation References:                                            */
/*      Man Pages:                                                      */
/*      References:                                                     */
/*                                                                      */
/* Author Information:                                                  */
/*      Name          Telephone    Fax          e-mail                  */
/*      Rolf Bork   . (626)3953182 (626)5770424 rolf@ligo.caltech.edu   */
/*                                                                      */
/* Code Compilation and Runtime Specifications:                         */
/*      Code Compiled on: Sun Ultra60  running Solaris2.7               */
/*      Compiler Used: cc386		                                */
/*      Runtime environment: PentiumIII running VxWorks 5.3.1           */
/*                                                                      */
/* Code Standards Conformance:                                          */
/*      Code Conforms to: LIGO standards.       OK                      */
/*                        Lint.                 TBD                     */
/*                        ANSI                  OK                      */
/*                        POSIX                 TBD                     */
/*                                                                      */
/* Known Bugs, Limitations, Caveats:                                    */
/*                                                                      */
/*                      -------------------                             */
/*                                                                      */
/*                             LIGO                                     */
/*                                                                      */
/*        THE LASER INTERFEROMETER GRAVITATIONAL WAVE OBSERVATORY.      */
/*                                                                      */
/*                     (C) The LIGO Project, 1999.                      */
/*                                                                      */
/*                                                                      */
/*                                                                      */
/* California Institute of Technology                                   */
/* LIGO Project MS 18-34                                                */
/* Pasadena CA 91125                                                    */
/*                                                                      */
/* Massachusetts Institute of Technology                                */
/* LIGO Project MS 20B-145                                              */
/* Cambridge MA 01239                                                   */
/*                                                                      */
/*----------------------------------------------------------------------*/

#define SWAP_ON_SIZE		0x7	/* Only used for Baja4700 */
#define TURN_OFF_FAIL_LITE	0x80
#define PMC_RAISE_INTR1_ALL	0x41
#define BUS_ID		0
#ifndef LINUX
#endif
#define PCI_ADDR	2

/* BASE 1 Register Structure */
struct VMIC5579_MEM_REGISTER {
	unsigned char rsv0;	/* 0x0 */
	unsigned char BID;	/* 0x1 */
	unsigned char rsv2;	/* 0x2 */
	unsigned char rsv3;	/* 0x3 */
	unsigned char NID;	/* 0x4 */
	unsigned char rsv5;	/* 0x5 */
	unsigned char rsv6;	/* 0x6 */
	unsigned char rsv7;	/* 0x7 */
	unsigned char IRS;	/* 0x8 */
	unsigned char CSR1;	/* 0x9 */
	unsigned char rsvA;	/* 0xa */
	unsigned char rsvB;	/* 0xb */
	unsigned char CSR2;	/* 0xc */
	unsigned char CSR3;	/* 0xd */
	unsigned char rsvE;	/* 0xe */
	unsigned char rsvF;	/* 0xf */
	unsigned char CMDND;	/* 0x10 */
	unsigned char CMD;	/* 0x11 */
	unsigned char CDR1;	/* 0x12 */
	unsigned char CDR2;	/* 0x13 */
	unsigned char ICSR;	/* 0x14 */
	unsigned char rsv15;	/* 0x15 */
	unsigned char rsv16;	/* 0x16 */
	unsigned char rsv17;	/* 0x17 */
	unsigned char SID1;	/* 0x18 */
	unsigned char IFR1;	/* 0x19 */
	unsigned short IDR1;	/* 0x1a & 0x1b */
	unsigned char SID2;	/* 0x1c */
	unsigned char IFR2;	/* 0x1d */
	unsigned short IDR2;	/* 0x1e & 0x1f */
	unsigned char SID3;	/* 0x20 */
	unsigned char IFR3;	/* 0x21 */
	unsigned short IDR3;	/* 0x22 & 0x23 */
	unsigned long DADD;	/* 0x24 to 0x27 */
	unsigned char EIS;	/* 0x28 */
	unsigned char ECSR3;	/* 0x29 */
	unsigned char rsv2A;	/* 0x2a */
	unsigned char rsv2B;	/* 0x2b */
	unsigned char rsv2C;	/* 0x2c */
	unsigned char MACR;	/* 0x2d */
	unsigned char rsv2E;	/* 0x2e */
	unsigned char rsv2F;	/* 0x2f */
};

/* BASE 0 Register Structure */
struct VMIC5579_PCI_REGISTER {
	unsigned int OMB1;
	unsigned int OMB2;
	unsigned int OMB3;
	unsigned int OMB4;
	unsigned int IMB1;
	unsigned int RSV1;
	unsigned int RSV2;
	unsigned int RSV3;
	unsigned int RSV4;
	unsigned int MWAR;
	unsigned int MWTC;
	unsigned int MRAR;
	unsigned int MRTC;
	unsigned int RSV5;
	unsigned int INTCSR;
	unsigned int MCSR;
};

struct VMIC5579_MEM_REGISTER *p5579Csr;
