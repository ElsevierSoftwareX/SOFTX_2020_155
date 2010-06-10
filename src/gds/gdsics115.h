/*----------------------------------------------------------------------*/
/*                                                                      */
/* Module Name: ICS115.h                                                */
/*                                                                      */
/* Module Description: LIGO driver code for ICS 115 DAC card.           */
/*                                                                      */
/*                                                                      */
/* Module Arguments:                                                    */
/*                                                                      */
/* Revision History:                                                    */
/* Rel   Date    Engineer   Comments                                    */
/* 0.1   19May99 C. Patton                                              */
/*                                                                      */
/* Documentation References:                                            */
/*      Man Pages: doc++ generated html                                 */
/*      References:                                                     */
/*                                                                      */
/* Author Information:                                                  */
/* Name           Telephone    Fax          e-mail                      */
/*Christine Patton(509)372-8121(509)372-8137patton_c@ligo-wa.caltech.edu*/
/*                                                                      */
/* Code Compilation and Runtime Specifications:                         */
/*      Code Compiled on: Ultra-Enterprise, Solaris 5.6                 */
/*      Compiler Used: sun workshop C 4.2                               */
/*      Runtime environment: sparc/solaris                              */
/*                                                                      */
/* Code Standards Conformance:                                          */
/*      Code Conforms to: LIGO standards.       OK                      */
/*                        Lint.                 TBD                     */
/*                        ANSI                  TBD                     */
/*                        POSIX                 TBD                     */
/*                                                                      */
/* Known Bugs, Limitations, Caveats:                                    */
/*                                                                      */
/*                                                                      */
/*                                                                      */
/*                      -------------------                             */
/*                                                                      */
/*                             LIGO                                     */
/*                                                                      */
/*        THE LASER INTERFEROMETER GRAVITATIONAL WAVE OBSERVATORY.      */
/*                                                                      */
/*                     (C) The LIGO Project, 1996.                      */
/*                                                                      */
/*                                                                      */
/* California Institute of Technology                                   */
/* LIGO Project MS 51-33                                                */
/* Pasadena CA 91125                                                    */
/*                                                                      */
/* Massachusetts Institute of Technology                                */
/* LIGO Project MS 20B-145                                              */
/* Cambridge MA 01239                                                   */
/*                                                                      */
/* LIGO Hanford Observatory                                             */
/* P.O. Box 1970 S9-02                                                  */
/* Richland WA 99352                                                    */
/*                                                                      */
/* LIGO Livingston Observatory                                          */
/* 19100 LIGO Lane Rd.                                                  */
/* Livingston, LA 70754                                                 */
/*                                                                      */
/*----------------------------------------------------------------------*/


#ifndef _GDS_ICS115_H
#define _GDS_ICS115_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef OS_VXWORKS
/* Offset definitions from ICS115 base address to registers */
#define SCV64_VINT              0x4808C
#define SCV64_IVECT		0x48024
#define SCV64_MODE		0x4803C
#define SCV64_GENCTL		0x48088

#define DAC_MUTE_OFFSET         0x50000
#define DAC_DIAG_OFFSET         0x50004
#define DAC_STAT_OFFSET         0x50008
#define DAC_CTRL_OFFSET         0x5000C
#define VSB_CTRL_OFFSET         0x50010
#define DAC_CONFIG_OFFSET       0x50014
#define DAC_FRAME_OFFSET        0x50024
#define DAC_CLOCK_OFFSET        0x50028
#define MRST_OFFSET             0x50030
#define DAC_RST_OFFSET          0x50034
#define SEQ_OFFSET              0x40000

/* Procedure Prototypes */
   int ICS115BoardInit(char *base, int intlvl, int ivector);
   int ICS115BoardReset(char *base);
   int ICS115DACReset(char *base);
   int ICS115ConfigSet(char *base, ICS115_CONFIG *cptr);
   int ICS115SeqSet(char *base, unsigned long *seq);
   int ICS115MuteSet(char *base, unsigned long *muter);
   int ICS115ControlSet(char *base, ICS115_CONTROL *cntrl);
   int ICS115DACReset(char *base);
   int ICS115DACEnable(char *base);
   int ICS115DACDisable(char *base);
   int ICS115IntEnable(char *base);
   int ICS115IntDisable(char *base);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _GDS_ICS115_H */
