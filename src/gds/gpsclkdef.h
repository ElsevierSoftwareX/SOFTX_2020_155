/* Version: $Id$ */

#ifndef _GDS_GPSCLKDEF_H
#define _GDS_GPSCLKDEF_H

#ifdef __cplusplus
extern "C" {
#endif


/* VME32DEF.H 23APR96 JCK VME-SYNCCLOCK32 register bit definitions */
/* see also VMESYN32.DEF for defining register addresses */
/* Bit assignments for:
 Heartbeat_Intr_Ctl,Match_Intr_Ctl,Time_Tag_Intr_Ctl, Ram_Fifo_Intr_Ctl */ 
#define Int_Enb                 0x10    /* Interrupt enable for CH A-D */
#define Int_Autoclear           0x08    /* Interrupt autoclear for CH A-D */
#define IRQ_Level_1             0x01    /* IRQ level 1 pattern for CH A-D */
#define IRQ_Level_2             0x02    /* IRQ level 2 pattern for CH A-D */
#define IRQ_Level_3             0x03    /* IRQ level 3 pattern for CH A-D */
#define IRQ_Level_4             0x04    /* IRQ level 4 pattern for CH A-D */
#define IRQ_Level_5             0x05    /* IRQ level 5 pattern for CH A-D */
#define IRQ_Level_6             0x06    /* IRQ level 6 pattern for CH A-D */
#define IRQ_Level_7             0x07    /* IRQ level 7 pattern for CH A-D */
/* Bit assignments for Status */
#define Ext_Ready               1       /* Ext. Time Tag Data Ready if 1*/
#define Sync_OK                 2       /* In-sync to time reference if 1 */
#define RAM_FIFO_Ready          4       /* Ram_Fifo_Offset byte Ready if 1 */
#define Match                   8       /* Match register toggles on Match */
#define Heartbeat               0x10    /* Heartbeat pulse detected if 1 */
#define Tag16_Ready             0x40    
#define Response_Ready          0x80    /* READ ONLY Command/dual port response ready */
/* Bit assignments for Resets */
#define Assert_Board_Reset      0xfe    /* WR bit0 = 0: Assert cold start */
#define Release_Board_Reset     0xfd    /* WR bit1 = 0: Release cold start */
#define Trigger_Sim_Ext_Time_Tag 0xfb    /* WR bit2 = 0: Fake ext. time tag */
#define Reset_Match             0xf7    /* WR bit3 = 0: Reset Match reg */
#define Reset_Heartbeat         0xef    /* WR bit4 = 0: Reset Heartbeat flag*/
#define Reset_Tag16             0xdf    /* WR bit5 = 0: Reset TAG16 FIFO */

/* VMEDPDEF.H 29 JUL 97 JCK fix DP_CodeSelect_NASA36, add DP_CodeSelect_IRIGA */
/* VMEDPDEF.H 11 JAN 97 JCK add GPS_Readlock,GPS_Readunlock*/
/* DVIDEF.H 19 NOV 96 JCK*/
#define DP_Command 0xff
#define No_op                   0 /* no operation */
#define Command_Set_Major       2 /* Set clock seconds..days to Major seconds..days */
#define Command_Set_Years       4 /* Set clock years to dual port years  */
#define Command_Set_RAM_FIFO    6 /* Set RAM FIFO external time tag mode */
#define Command_Reset_RAM_FIFO  8 /* Reset RAM FIFO external time tag mode */
#define Command_Empty_RAM_FIFO 10 /* Empty RAM FIFO */
#define Command_Set_Ctr0       12 /* Set 82C54 ctr 2 ("lowrate") params */
#define Command_Set_Ctr1       14 /* Set 82C54 ctr 2 ("heartbeat") params */
#define Command_Set_Ctr2       16 /* Set 82C54 ctr 2 ("rate2") params */
#define Command_Rejam          18 /* Re-jam at start of next second */
#define Command_Spfun1         20 /* Reserved for special function #1 */
#define Command_Spfun2         22 /* Reserved for special function #2 */
#define GPS_Readlock           24 /* lock GPS position buffer for read */
#define GPS_Readunlock         26 /* unlock GPS position buffer to allow update*/
/*   DUAL PORT RAM LOCATIONS 0x00..0x7F ARE READ-ONLY */
#define DP_Extd_Sts 0x00 /* Extended status READ ONLY */
#define DP_Extd_Sts_Nosync 0x01 /* Set if NOT in sync */
#define DP_Extd_Sts_Nocode 0x02 /* Set if selected input code NOT decodeable */
#define DP_Extd_Sts_NoPPS  0x04 /* Set if PPS input invalid */
#define DP_Extd_Sts_NoMajT 0x08 /* Set if major time NOT set since jam */
#define DP_Extd_Sts_NoYear 0x10 /* Set if year NOT set */
#define DP_Code_CtlA 0x0f       /* Control field "A" read data */
#define DP_Code_CtlB 0x10       /* Control field "B" read data */
#define DP_Code_CtlC 0x11       /* Control field "C" read data */
#define DP_Code_CtlD 0x12       /* Control field "D" read data */
#define DP_Code_CtlE 0x13       /* Control field "E" read data */
#define DP_Code_CtlF 0x14       /* Control field "F" read data */
#define DP_Code_CtlG 0x15       /* Control field "G" read data */


#define DP_Control0 0x80 /* Dual Port Ram Address for Control Register */
#define DP_Control0_Leapyear     0x01 /* Current year is leap year*/
#define DP_Control0_CodePriority 0x02 /* Time code input has priority over PPS*/
#define DP_Control0_NegCodePropD 0x04 /* code input prop delay setting is - */
#define DP_Control0_NegPPSPropD  0x08 /* PPS input prop delay setting is - */

#define DP_CodeSelect 0x81              /* Time code input select */
#define DP_CodeSelect_IRIGB 0x0b        /* IRIG-B */
#define DP_CodeSelect_IRIGA 0x0a        /* IRIG-A */
#define DP_CodeSelect_NASA36 0x06       /* NASA36 */
#define DP_CodeSelect_2137 0x07         /* 2137   */
#define DP_CodeSelect_XR3 0x03          /* XR3    */
#define DP_CodeSelect_IRIGG 0x0f        /* IRIG-G */
#define DP_CodeSelect_IRIGE 0x0e        /* IRIG-E */

#define DP_LeapSec_Day10Day1 0x82       /* Day (10s & 1s) ending in leap sec*/

#define DP_LeapSec_Day1000Day100 0x83   /* Day (0,100s) ending in leap sec*/

#define DP_CodePropDel_ns100ns10  0x84  /* time code prop. delay 100,10 ns */
#define DP_CodePropDel_us10us1    0x85  /* time code prop. delay 10,1 us */#define DP_CodePropDel_us32  8  /* time code prop. delay 1000,100 us */
#define DP_CodePropDel_ms1us100   0x86       /* time code prop. delay 10,1 us */#define DP_CodePropDel_us32  8  /* time code prop. delay 1000,100 us */
#define DP_CodePropDel_ms100ms10  0x87       /* time code prop. delay 100,10 ms */


#define DP_PPS_PropDel_ns100ns10  0x88       /* PPS prop. delay 100,10 ns  */
#define DP_PPS_PropDel_us10us1    0x89       /* PPS prop. delay 10,1 us    */
#define DP_PPS_PropDel_ms1us100   0x8a       /* PPS prop. delay 1000,100 us*/
#define DP_PPS_PropDel_ms100ms10  0x8b       /* PPS prop. delay 100,10 ms  */

#define DP_PPS_Time_ns100ns10     0x8c          /* PPS time 100,10ns */
#define DP_PPS_Time_us10us1       0x8d          /* PPS time 10,1us */
#define DP_PPS_Time_ms1us100      0x8e          /* PPS time 1000,100us */
#define DP_PPS_Time_ms100ms10     0x8f          /* PPS time 100,10ms */
#define DP_Major_Time_s10s1       0x90          /* Major time 10,1second */
#define DP_Major_Time_m10m1       0x91          /* Major time 10,1minute */
#define DP_Major_Time_h10h1       0x92          /* Major time 10,1hour */
#define DP_Major_Time_d10d1       0x93          /* Major time 10,1 day */
#define DP_Major_Time_d1000d10    0x94           /* Major time 0, 100 day */
#define DP_Year10Year1       0x95               /* 10,1 years */
#define DP_Year1000Year100   0x96               /* 1000,100 years */
#define DP_Codebypass        0X97               /* #frames to validate code */

#define DP_Ctr2_ctl          0x98     /* ctr 2 control word */
#define DP_Ctr1_ctl          0x99     /* ctr 1 control word */
#define DP_Ctr0_ctl          0x9A     /* ctr 0 control word */
#define DP_Ctr2_ctl_sel                 0x80  /* ALWAYS used for ctr 2 */
#define DP_Ctr1_ctl_sel                 0x40  /* ALWAYS used for ctr 1 */
#define DP_Ctr0_ctl_sel                 0x00  /* ALWAYS used for ctr 0 */
#define DP_ctl_rw                       0x30  /* ALWAYS used */
#define DP_ctl_mode0                    0x00  /* Ctrx mode 0 select */
#define DP_ctl_mode1                    0x02  /* Ctrx mode 1 select */
#define DP_ctl_mode2                    0x04  /* Ctrx mode 2 select */
#define DP_ctl_mode3                    0x06  /* Ctrx mode 3 select */
#define DP_ctl_mode4                    0x08  /* Ctrx mode 4 select */
#define DP_ctl_mode5                    0x0A  /* Ctrx mode 5 select */
#define DP_ctl_bin                      0x00  /* Ctrx binary mode select */
#define DP_ctl_bcd                      0x01  /* Ctrx bcd mode select */
#define DP_Ctr2_lsb          0x9B     /* ctr 2 count LSB    */
#define DP_Ctr2_msb          0x9C     /* ctr 2 count MSB    */
#define DP_Ctr1_lsb          0x9D     /* ctr 1 count LSB    */
#define DP_Ctr1_msb          0x9E     /* ctr 1 count MSB    */
#define DP_Ctr0_lsb          0x9F     /* ctr 0 count LSB    */
#define DP_Ctr0_msb          0xA0     /* ctr 0 count MSB    */


/* GPSDEF.H 28apr98 DP_GPS_Altbin3124..DP_GPS_Altbin0700 added  */
/* GPSDEF.H 31jul97 DP_GPS_UpdateFlag..DP_GPS_RTCM_Ack  */
/* GPSDEF.H 8jan97 JCK add DP_HQ_TFOM */
/* GPSDEF.H 7jun96 JCK */
#define DP_GPS_Semaphore          0xA3     /* 0=unlocked,1=read lock,-1=wr lock */

#define DP_GPS_Status             0xA4
#define DP_GPS_Status_Sats        0x0F     /* lo order nibble: BIN # sats tracked */
#define DP_GPS_Status_South       0x80     /* Bit 7: 1=South,0=North latitude */
#define DP_GPS_Status_West        0x40     /* Bit 6: 1=West,0=East longitude */
#define DP_GPS_Status_Nav         0x10     /* Bit 4: 1=Navigate, 0=Acquiring */

#define DP_GPS_Lat_MinEM34        0xA5     /* BCD 10-3,10-4 min latitude */
#define DP_GPS_Lat_MinEM12        0xA6     /* BCD 10-1,10-2 min latitude */
#define DP_GPS_Lat_MinE10         0xA7     /* BCD 10+1,10+0 min latitude */
#define DP_GPS_Lat_DegE10         0xA8     /* BCD 10+1,10+0 deg latitude */
#define DP_GPS_Lon_MinEM34        0xA9     /* BCD 10-3,10-4 min longitude */
#define DP_GPS_Lon_MinEM12        0xAA     /* BCD 10-1,10-2 min longitude */
#define DP_GPS_Lon_MinE10         0xAB     /* BCD 10+1,10+0 min longitude */
#define DP_GPS_Lon_DegE10         0xAC     /* BCD 10+1,10+0 deg longitude */
#define DP_GPS_Lon_DegE32         0xAD     /* BCD 10+3,10+2 deg longitude */
#define DP_GPS_SOG0M1             0xAE     /* BCD speed over ground, 1,10-1 m/sec */
#define DP_GPS_SOG21              0xAF     /* BCD speed over ground, 100,10 m/sec */
#define DP_GPS_HOG0M1             0xB0     /* BCD heading over ground, 1,10-1 deg */
#define DP_GPS_HOG21              0xB1     /* BCD heading over ground, 100,10 deg */
#define DP_HQ_TFOM                0xB2     /* HaveQuick Time Figure of Merit */
#define DP_GPS_UpdateFlag         0xB3     /* set non-0 by board when GPS data updated */ 
#define DP_GPS_Latbin3124         0xB4     /* Oncore: ms byte lat arc milliseconds */
#define DP_GPS_Latbin2316         0xB5     /* Oncore: <23:16> lat arc milliseconds */
#define DP_GPS_Latbin1508         0xB6     /* Oncore: <15:08> lat arc milliseconds */
#define DP_GPS_Latbin0700         0xB7     /* Oncore: ls byte lat arc milliseconds */
#define DP_GPS_Lonbin3124         0xB8     /* Oncore: ms byte lon arc milliseconds */
#define DP_GPS_Lonbin2316         0xB9     /* Oncore: <23:16> lon arc milliseconds */
#define DP_GPS_Lonbin1508         0xBA     /* Oncore: <15:08> lon arc milliseconds */
#define DP_GPS_Lonbin0700         0xBB     /* Oncore: ls byte lon arc milliseconds */
#define DP_GPS_RTCM_Ack           0xBC     /* Oncore: Ack count of RTCM104 messages received */
#define DP_GPS_Altbin3124         0xC0     /* Oncore: ms byte alt above geoid in cm.*/
#define DP_GPS_Altbin2316         0xC1     /* Oncore: <23:16> alt above geoid in cm.*/
#define DP_GPS_Altbin1508         0xC2     /* Oncore: <15:08> alt above geoid in cm.*/
#define DP_GPS_Altbin0700         0xC3     /* Oncore: ls byte alt above geoid in cm.*/


#ifdef __cplusplus
}
#endif

#endif /*_GDS_GPSCLKDEF_H */
