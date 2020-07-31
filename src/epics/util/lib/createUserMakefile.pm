package CDS::USP_makefile;
use Exporter;
@ISA = ('Exporter');

#// \b sub \b createUsermakefile \n
#// Generate the user C code Makefile  \n\n
sub createUSPmakefile{

open(OUTM,">./".$::mFile2) || die "cannot open Makefile file for writing";

print OUTM "# User Space Linux\n";
print OUTM "CFLAGS += -O -w -I../../include\n";

print OUTM "CFLAGS += $::servoflag \n";

if ($::iopModel > -1) {  #************ SETUP FOR IOP ***************
#Following used for IOP running at 128K 
  if($::adcclock ==128) {
  print OUTM "CFLAGS += -DIOP_IO_RATE=131072\n";
  } else {
  print OUTM "CFLAGS += -DIOP_IO_RATE=65536\n";
  }
# Invoked if IOP cycle rate slower than ADC clock rate
  print OUTM "CFLAGS += -DUNDERSAMPLE=$::clock_div\n";
  print OUTM "CFLAGS += -DADC_MEMCPY_RATE=$::adcclock\n";
} 

print OUTM "CFLAGS += -D";
print OUTM "\U$::skeleton";
print OUTM "_CODE\n";
print OUTM "CFLAGS += -DFE_SRC=\\\"\L$::skeleton/\L$::skeleton.c\\\"\n";
print OUTM "CFLAGS += -DFE_HEADER=\\\"\L$::skeleton.h\\\"\n";
#print OUTM "CFLAGS += -DFE_PROC_FILE=\\\"\L$::{skeleton}_proc.h\\\"\n";

print OUTM "CFLAGS += -g\n";

if ($::remoteGPS) {
  print OUTM "CFLAGS += -DREMOTE_GPS\n";
}
if($::systemName eq "sei" || $::useFIRs)
{
print OUTM "CFLAGS += -DFIR_FILTERS\n";
}
if ($::no_sync) {
  print OUTM "#Comment out to enable 1PPS synchronization\n";
  print OUTM "CFLAGS += -DNO_SYNC\n";
} else {
  print OUTM "#Uncomment to disable 1PPS signal sinchronization (channel 31 (last), ADC 0)\n";
  print OUTM "#CFLAGS += -DNO_SYNC\n";
}
if (0 == $::dac_testpoint_names && 0 == $::::extraTestPoints && 0 == $::filtCnt) {
	print "Not compiling DAQ into the front-end\n";
	$::no_daq = 1;
}
if ($::no_daq) {
  print OUTM "#Comment out to enable DAQ\n";
  print OUTM "CFLAGS += -DNO_DAQ\n";
}

# Use oversampling code if not 64K system
#if($::modelrate < 64) {
#  if ($::no_oversampling) {
#    print OUTM "#Uncomment to oversample A/D inputs\n";
#    print OUTM "#CFLAGS += -DOVERSAMPLE\n";
#    print OUTM "#Uncomment to interpolate D/A outputs\n";
#    print OUTM "#CFLAGS += -DOVERSAMPLE_DAC\n";
#  } else {
#    print OUTM "#Comment out to stop A/D oversampling\n";
#    print OUTM "CFLAGS += -DOVERSAMPLE\n";
#    if ($::no_dac_interpolation) {
#    } else {
#      print OUTM "#Comment out to stop interpolating D/A outputs\n";
#      print OUTM "CFLAGS += -DOVERSAMPLE_DAC\n";
#    }
#  }
#}

#Following used with IOP running at 64K (NORMAL) 
if($::modelrate < 64) {
  if($::adcclock ==64) {
    print OUTM "CFLAGS += -DIOP_IO_RATE=65536\n";
    if($::modelrate < 64) {
        my $drate = 64/$::modelrate;
        if($drate == 8 or $drate > 32) {
            die "RCG does not support a user model rate $::modelrate" . "K with IOP data at $::adcclock" ."K\n"  ;
        }
        print OUTM "CFLAGS += -DOVERSAMPLE\n";
        print OUTM "CFLAGS += -DOVERSAMPLE_DAC\n";
        print OUTM "CFLAGS += -DOVERSAMPLE_TIMES=$drate\n";
        print OUTM "CFLAGS += -DFE_OVERSAMPLE_COEFF=feCoeff$drate"."x\n";
        print OUTM "CFLAGS += -DADC_MEMCPY_RATE=1\n";
    }
  }
}
    print OUTM "CFLAGS += -DUNDERSAMPLE=1\n";
if ($::iopModel > -1) {
  $::modelType = "IOP";
  if($::diagTest > -1) {
  print OUTM "CFLAGS += -DDIAG_TEST\n";
  }
  if($::dacWdOverride > -1) {
  print OUTM "CFLAGS += -DDAC_WD_OVERRIDE\n";
  }
  # ADD DAC_AUTOCAL to IOPs
  print OUTM "CFLAGS += -DDAC_AUTOCAL\n";
} else {
  print OUTM "#Uncomment to run on an I/O Master \n";
  print OUTM "#CFLAGS += -DIOP_MODEL\n";
}
if ($::iopModel < 1) {
  print OUTM "CFLAGS += -DCONTROL_MODEL\n";
  $::modelType = "CONTROL";
} 
if ($::dolphin_time_xmit > -1) {
  print OUTM "CFLAGS += -DXMIT_DOLPHIN_TIME=1\n";
} 
if ($::dolphinTiming > -1) {
  print OUTM "CFLAGS += -DUSE_DOLPHIN_TIMING=1\n";
} 
if ($::flipSignals) {
  print OUTM "CFLAGS += -DFLIP_SIGNALS=1\n";
}

 if ($::virtualiop != 1) {
if ($::pciNet > 0) {
print OUTM "#Enable use of PCIe RFM Network Gen 2\n";
print OUTM "DOLPHIN_PATH = /opt/srcdis\n";
print OUTM "CFLAGS += -DHAVE_CONFIG_H -I\$::(DOLPHIN_PATH)/src/include/dis -I\$::(DOLPHIN_PATH)/src/include -I\$::(DOLPHIN_PATH)/src/SISCI/cmd/test/lib -I\$::(DOLPHIN_PATH)/src/SISCI/src -I\$::(DOLPHIN_PATH)/src/SISCI/api -I\$::(DOLPHIN_PATH)/src/SISCI/cmd/include -I\$::(DOLPHIN_PATH)/src/IRM_GX/drv/src -I\$::(DOLPHIN_PATH)/src/IRM_GX/drv/src/LINUX -DOS_IS_LINUX=196616 -DLINUX -DUNIX  -DLITTLE_ENDIAN -DDIS_LITTLE_ENDIAN -DCPU_WORD_IS_64_BIT -DCPU_ADDR_IS_64_BIT -DCPU_WORD_SIZE=64 -DCPU_ADDR_SIZE=64 -DCPU_ARCH_IS_X86_64 -DADAPTER_IS_IX   -m64 -D_REENTRANT\n";
}
}

if ($::specificCpu > -1) {
  print OUTM "#Comment out to run on first available CPU\n";
  print OUTM "CFLAGS += -DSPECIFIC_CPU=$::specificCpu\n";
} else {
  print OUTM "#Uncomment to run on a specific CPU\n";
  print OUTM "#CFLAGS += -DSPECIFIC_CPU=2\n";
}

# Set BIQUAD as default starting RCG V2.8
  print OUTM "#Comment out to go back to old iir_filter calculation form\n";
  print OUTM "CFLAGS += -DALL_BIQUAD=1 -DCORE_BIQUAD=1\n";

if ($::::directDacWrite) {
  print OUTM "CFLAGS += -DDIRECT_DAC_WRITE=1\n";
} else {
  print OUTM "#CFLAGS += -DDIRECT_DAC_WRITE=1\n";
}

if ($::::noZeroPad) {
  print OUTM "CFLAGS += -DNO_ZERO_PAD=1\n";
} else {
  print OUTM "#CFLAGS += -DNO_ZERO_PAD=1\n";
}

if ($::::optimizeIO) {
  print OUTM "CFLAGS += -DNO_DAC_PRELOAD=1\n";
} else {
  print OUTM "#CFLAGS += -DNO_DAC_PRELOAD=1\n";
}

if ($::::rfmDelay) {
  print OUTM "#Comment out to run without RFM Delayed by 1 cycle\n";
  print OUTM "CFLAGS += -DRFM_DELAY=1\n";
} else {
  print OUTM "#Clear comment to run with RFM Delayed by 1 cycle\n";
  print OUTM "#CFLAGS += -DRFM_DELAY=1\n";
}
  print OUTM "CFLAGS += -DUSER_SPACE=1\n";
  print OUTM "CFLAGS += -fno-builtin-sincos\n";

print OUTM "export LD_LIBRARY_PATH=\$LD_LIBRARY_PATH:/opt/DIS/lib64\n";
print OUTM "API_LIB_PATH=/opt/DIS/lib64\n";
print OUTM "\n\n";

print OUTM "\n";
#print OUTM "all: \$(ALL)\n";
print OUTM "\n";
#print OUTM "clean:\n";
#print OUTM "\trm -f \$(ALL) *.o\n";
print OUTM "\n";

print OUTM "CFLAGS += -I\$(SUBDIRS)/../../include -I$::rcg_src_dir\/src/drv -I$::rcg_src_dir\/src/include \n";
if ($::pciNet > 0 && $::virtualiop != 1) {
print OUTM "LDFLAGS = -L \$(API_LIB_PATH) -lsisci\n";
} else {
print OUTM "LDFLAGS = -L \$(API_LIB_PATH) \n";
}

print OUTM "TARGET=$::skeleton\n\n\n";
print OUTM "$::skeleton: $::skeleton.o rfm.o \n\n";
print OUTM "rfm.o: $::rcg_src_dir\/src/drv/rfm.c \n";
my $ccf = "\$\(CC\) \$\(CFLAGS\) \$\(CPPFLAGS\) \-c \$\< \-o \$\@";
print OUTM "\t$ccf \n";
print OUTM ".c.o: \n";
print OUTM "\t$ccf \n";

print OUTM "\n";
close OUTM;
}
