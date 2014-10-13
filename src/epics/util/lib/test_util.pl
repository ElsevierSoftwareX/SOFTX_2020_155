#!/usr/bin/perl -I..

require "lib/Util.pm";

@res = CDS::Util::findDefine("/opt/rtcds/rtscore/release/src/include/commData2.h", "MAX_IPC", "MAX_IPC_RFM");

#define MAX_ADC_MODULES		12	
#define MAX_DAC_MODULES		12
#define MAX_DIO_MODULES		8
#define MAX_RFM_MODULES		2
#define MAX_IO_MODULES		24

@res = CDS::Util::findDefine("/opt/rtcds/rtscore/release/src/include/drv/cdsHardware.h", "MAX_ADC_MODULES", "MAX_DAC_MODULES", "MAX_DIO_MODULES", "MAX_RFM_MODULES", "MAX_IO_MODULES");

