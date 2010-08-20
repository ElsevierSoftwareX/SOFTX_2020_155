#!/usr/bin/perl

use File::Path;

die "Usage: $PROGRAM_NAME <MDL file> <Output file name> [<DCUID number>] [<site>] [<speed>]\n\t" . "site is (e.g.) H1, M1; speed is 2K, 16K, 32K or 64K\n"
        if (@ARGV != 2 && @ARGV != 3 && @ARGV != 4 && @ARGV != 5);

# See if we are not running RTLinux
$no_rtl = system("/sbin/lsmod | grep rtl");

#ifeq ($(rtl_module),)
#CFLAGS += -DNO_RTL=1
#endif

if ($no_rtl) {
	print "Generating CPU Shutdown Real-time code\n";
}

my $mdmStr = `grep "define MAX_DIO_MODULES" ../../include/drv/cdsHardware.h`;
my @mdmNum = ($mdmStr =~ m/(\d+)/);
$maxDioMod = pop(@mdmNum);

# See if this is the latest Wind River system
$kernel_release = `uname -r`;
chomp $kernel_release;
if ($kernel_release eq '2.6.21.7') {
	print "Running on WindRiver RTLinux release\n";
	$wind_river_rtlinux = 1;
}

$site = "M1"; # Default value for the site name
$location = "mit"; # Default value for the location name
$rate = "60"; # In microseconds (default setting)
$dcuId = 10; # Default dcu Id
$targetHost = "localhost"; # Default target host name
$specificCpu = -1; # Defaults is to run the FE on the first available CPU
$adcMaster = -1;
$adcSlave = -1;
$pciNet = -1;
$shmem_daq = 0; # Do not use shared memory DAQ connection
$no_sync = 0; # Sync up to 1PPS by default
$no_daq = 0; # Enable DAQ by default
$gdsNodeId = 1;
$adcOver = 0;
$ifoid = 0; # Default ifoid for the DAQ
$nodeid = 0; # Default GDS node id for awgtpman
$dac_internal_clocking = 0; # Default is DAC external clocking
$no_oversampling = 0; # Default is to iversample
$no_dac_interpolation = 0; # Default is to interpolate D/A outputs
$compat_initial_ligo = 0; # Default is not to build for initial LIGO compatibility

if (@ARGV > 2) {
	$dcuId = $ARGV[2];
}
if (@ARGV > 3) {
	$site = $ARGV[3];
	if ($site =~ /^M/) {
		$location = "mit";
	} elsif ($site =~ /^G/) {
		$location = "geo";
	} elsif ($site =~ /^H/) {
		$location = "lho";
	} elsif ($site =~ /^L/) {
		$location = "llo";
	} elsif ($site =~ /^C/) {
		$location = "caltech";
	} elsif ($site =~ /^S/) {
		$location = "stanford";
	} elsif ($site =~ /^K/) {
		$location = "kamioka";
	}
}
if (@ARGV > 4) {
	my $param_speed = $ARGV[4];
	if ($param_speed eq "2K") {
		$rate = 480;
	} elsif ($param_speed eq "4K") {
		$rate = 240;
	} elsif ($param_speed eq "16K") {
		$rate = 60;
	} elsif ($param_speed eq "32K") {
		$rate = 30;
	} elsif ($param_speed eq "64K") {
		$rate = 15;
	} else  { die "Invalid speed $param_speed specified\n"; }
}
$skeleton = $ARGV[1];

if ($skeleton !~ m/^[cghklms]\d.*/) {
   die "***ERROR: Model name must begin with <ifo><subsystem>: $skeleton\n";
}

$ifo = substr($skeleton, 0, 2);
print "file out is $skeleton\n";

$cFile = "../../fe/";
$cFile .= $ARGV[1];
$cFileDirectory = $cFile;
$cFile .= "/";
$cFile .= $ARGV[1];
$cFile .= ".c";
$hFile = "../../include/";
$hFile .= $ARGV[1];
$hFile .= ".h";
$mFile = "../../fe/";
$mFile .= $ARGV[1];
$mFile .= "/";
$mFile .= "Makefile";
$meFile = "../../../config/";
$meFile .= "Makefile\.";
$meFile .= $ARGV[1];
$meFile .= epics;
$epicsScreensDir = "../../../build/" . $ARGV[1] . "epics/medm";
$configFilesDir = "../../../build/" . $ARGV[1] . "epics/config";

#print "DCUID = $dcuId\n";
#if($dcuId < 16) {$rate = 60;}
#if($dcuId > 16) {$rate = 480;}
if (@ARGV == 2) { $skeleton = $ARGV[1]; }
open(EPICS,">../fmseq/".$ARGV[1]) || die "cannot open output file for writing";
mkdir $cFileDirectory, 0755;
open(OUT,">./".$cFile) || die "cannot open c file for writing $cFile";
# Save existing front-end Makefile
@months = qw(Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec);
if (-s $mFile) {
  my ($second, $minute, $hour, $dayOfMonth, $month, $yearOffset, $dayOfWeek, $dayOfYear, $daylightSavings) = localtime();
  my $year = 1900 + $yearOffset;
  #my $theTime = $year . "_$months[$month]" . "_$dayOfMonth" . "_$hour:$minute:$second";
  $theTime = sprintf("%d_%s_%02d_%02d:%02d:%02d", $year, $months[$month], $dayOfMonth, $hour, $minute, $second);
  system ("/bin/mv $mFile $mFile" . "_$theTime");
  #open(OUTM, "/dev/null") || die "cannot open /dev/null for writing";
}
open(OUTM,">./".$mFile) || die "cannot open Makefile file for writing";
open(OUTME,">./".$meFile) || die "cannot open EPICS Makefile file for writing";
open(OUTH,">./".$hFile) || die "cannot open header file for writing";

$diag = "./diags\.txt";
#$diag = "/dev/null";
open(OUTD,">".$diag) || die "cannot open diag file for writing";

$mySeq = 0;
$connects = 0;
$ob = 0;
$subSys = 0;
$inBranch = 0;
$endBranch = 0;
$adcCnt = 0;
$dacCnt = 0;
$boCnt = 0;
$filtCnt = 0;
$firCnt = 0;
$useWd = 0;
$gainCnt = 0;
$busPort = -1;
$trigCnt = 0;                                                              # ===  MA  ===
$trigOut = 0;                                                              # ===  MA  ===
$convDeg2Rad = 0;                                                          # ===  MA  ===
$groundDecl = 0;                                                           # =+=  MA  =+=
$groundInit = 0;                                                           # =+=  MA  =+=
$ipcxCnt = 0;                                                              # ===  IPCx  ===
$ipcxDeclDone = 0;                                                         # ===  IPCx  ===
$ipcxInitDone = 0;                                                         # ===  IPCx  ===
$oscUsed = 0;
$useFIRs = 0;
# ***  DEBUG  ***
# $useFIRs = 1;
# ***  DEBUG  ***

# set debug level (0 - no debug messages)
$dbg_level = 2;

# Print debug message
# Example:
# debug (0, "debug test: openBrace=$openBrace");
#
sub debug {
  if ($dbg_level > shift @_) {
	print @_, "\n";
  }
}

sub init_vars {
# Global variables set by parser
$epics_fields[0] = undef; # list of lists; for each part number, epics fields
$extraTestPoints;	# a list of test point names not related to filters
$extraTpcount = 0;		# How many extra TPs we have
@top_names; 	# array of top-level subsytem names marked with "top_names" tag
$systemName = "";	# model name
$adcCnt = 0;	# Total A/D converter boards
$adcType[0] = 0;	# A/D board types
$adcNum[0] = 0;	# A/D board numbers, sequential
$dacCnt = 0;	# Total D/A converter boards
$dacType[0] = 0;	# D/A board types
$dacNum[0] = 0;	# D/A board numbers, sequential
$boCnt = 0;	# Total binary output boards
$boType[0] = 0;	# Binary output board types
$boNum[0] = 0;	# Binary output board numbers, sequential
$nonSubCnt = 0; # Total of non-sybsystem parts found in the model

# Keeps non-subsystem part numbers
$nonSubPart[0] = 0;	# $nonSubPart[0 .. $nonSubCnt]

$partCnt = 0;	# Total parts found in the simulink model

# Element is set to one for each CDS parts
$cdsPart[0] = 0;	# $cdsPart[0 .. $partCnt]

$ppFIR[0] = 0;          # Set to one for PPFIR filters

# Total number of inputs for each part
# i.e. how many parts are connected to it with lines (branches)
$partInCnt[0] = 0;	# $partInCnt[0 .. $partCnt]
# Source part name (a string) for each part, for each input
# This shows which source part is connected to that input
$partInput[0][0] = "";	# $partInput[0 .. $partCnt][0 .. $partInCnt[0]]
# Source port number
# This shows which source parts' port is connected to each input
$partInputPort[0][0] = 0;	# $partInputPort[0 .. $partCnt][0 .. $partInCnt[$_]]
$partInputs[0] = 0;		# Stores 'Inputs' field of the part declaration in SUM part, 'Operator' in RelationaOperator part, 'Value' Constant part, etc
$partInputs1[0] = 0;		# Same as $partInputs, i.e. extra part parameter, used with Saturation part

$partOutputs[0] = 0; 	# Stores 'Outputs' field value for some parts
# Total number of outputs for each part
# i.e. how many parts are connected with lines (branches) to it
$partOutCnt[0] = 0;	# $partOutCnt[0 .. $partCnt]
# Destination part name (a string) for each part, for each output
# Shows which destination part is connected to that output
$partOutput[0][0] = "";	# $partOutput[0 .. $partCnt][0 .. $partOutCnt[$_]]
# Destination port number
# This shows which destination parts' port is connected to each output
$partOutputPort[0][0] = 0;	# $partOutputPort[0 .. $partCnt][0 .. $partOutCnt[$_]]

# Output port source number
# For each part, for each output port it keeps the output port source number
$partOutputPortUsed[0][0] = 0;	# $partOutputPortUsed[0 .. $partCnt][0 .. $partOutCnt[$_]]

# Part names annotated with subsystem names
$xpartName[0] = "";	# $xpartName[0 .. $partCnt]

# Part names not annotated with subsystem names
$partName[0] = "";	# $partName[0 .. $partCnt]

# Part type
$partType[0] = "";	# $partType[0 .. $partCnt]

# Name of subsystem where part belongs
$partSubName[0] = "";	# $partSubName[0 .. $partCnt]

# For each part its subsystem number
$partSubNum[0] = 0;	# $partSubNum[0 .. $partCnt]
$subSys = 0;	# Subsystems counter
$subSysName[0] = "";	# Subsystem names

# Subsystem part number ranges
$subSysPartStart[0] = 0;
$subSysPartStop[0] = 0;

# IPC output code
$ipcOutputCode = "";

# Front-end tailing code
$feTailCode = "";

# Remote IPC hosts total number
$remoteIPChosts = 0;

# Remote IPC nodes (host:port) mapped to index
%remoteIPChostIdx;

# Remote IPC nodes in a list
@remoteIPCnodes;

# Can only connect to this many remote IPC hosts
# and send this many variables
$maxRemoteIPCHosts = 4;
$maxRemoteIPCVars = 4;

# My remote IPC MX port
$remoteIPCport = 0;

#
# IPCx parameter names
#
$ipcxMissing[0] = "Signal Name";
$ipcxMissing[1] = "ipcType";
$ipcxMissing[2] = "ipcRate";
$ipcxMissing[3] = "ipcNum";

$ipcxType[0] = "SHMEM";
$ipcxType[1] = "RFM";
$ipcxType[2] = "PCIE";

for ($ii = 0; $ii < 3; $ii++) {
   $ipcxMaxNum[$ii] = -999;
}

$ipcxBlockTags[0] = undef;
$ipcxTagCount = 0;

# Remove leading subsystems name
sub remove_subsystem {
        my ($s) = @_;
        return substr $s, 1 + rindex $s, "_";
}


# Clear the part input and output counters
for ($ii = 0; $ii < 2000; $ii++) {
  $partInCnt[$ii] = 0;
  $partOutCnt[$ii] = 0;
  $partInUsed[$ii] = 0;
}
}

my $system_name = $ARGV[1];
print OUTH "\#ifndef \U$system_name";
print OUTH "_H_INCLUDED\n\#define \U$system_name";
print OUTH "_H_INCLUDED\n";
print OUTH "\#define SYSTEM_NAME_STRING_LOWER \"\L$system_name\"\n";


require "lib/ParsingDiagnostics.pm";
# Old parser
if (0) {
init_vars();

# Parser input file (Simulink model)
require "lib/Parser1.pm";
#open(IN,"<../simLink/".$ARGV[0]) || die "cannot open mdl file $ARGV[0]\n";
open(IN,"<".$ARGV[0]) || die "cannot open mdl file $ARGV[0]\n";
die unless CDS::Parser::parse();
close(IN);

# Print diagnostics
CDS::ParsingDiagnostics::print_diagnostics("parser_diag_good.txt");
}

init_vars();
require "lib/Parser2.pm";
#print "###################   mdl file is $ARGV[0] #############\n";
#open(IN,"<../simLink/".$ARGV[0]) || die "cannot open mdl file $ARGV[0]\n";
open(IN,"<".$ARGV[0]) || die "cannot open mdl file $ARGV[0]\n";
die unless CDS::Parser::parse();
close(IN);


#CDS::ParsingDiagnostics::print_diagnostics("parser_diag.txt");

# By default, set DAC input counts to 16
#for($ii=0;$ii<$dacCnt;$ii++) {
#  $partInCnt[$dacPartNum[$ii]] = 16;
#}

$systemName = substr($systemName, 2, 3);
$plantName = $systemName; # Default plant name is the model name

#
# Find all IPCx parts and start building IPCx parts matrix
#
for ($ii = 0; $ii < $partCnt; $ii++) {
   if ($partType[$ii] =~ /^IPCx/) {
      $ipcxParts[$ipcxCnt][0] = $xpartName[$ii];
      $ipcxParts[$ipcxCnt][1] = "I" . substr($ipcxBlockTags[$ii], 8, 3);
      $ipcxParts[$ipcxCnt][2] = undef;
      $ipcxParts[$ipcxCnt][3] = undef;
      $ipcxParts[$ipcxCnt][4] = $ii;
      $ipcxCnt++;
   }
   #
   # We will need the location and site parameters from
   # cdsParameters, so keep track of this part as well
   #
   elsif ($partType[$ii] eq "Parameters") {
      $oo = $ii;
   }
}

if ($ipcxCnt > 0) {
   #
   # This model does include IPCx parts, so extract location and
   # site from cdsParameters and read the IPCx parameter file
   #
   ("CDS::Parameters::printHeaderStruct") -> ($oo);

   $iFile = "/cvs/cds/";
   #$iFile = "/opt/rtcds/";
   $iFile .= $location;
   $iFile .= "/chans/ipc/";
   $iFile .= $site;
   $iFile .= "\.ipc";

   open(IPCIN, "<$iFile") || die "***ERROR: IPCx parameter file $iFile not found\n";
   chomp(@inData=<IPCIN>);
   close IPCIN;

   #
   # Process one line at a time from the IPCx parameter file
   #
   $ipcxParamCnt = -1;
   $skip = 0;

   foreach $value (@inData) {
      #
      # Skip lines that begin with '#' or 'desc='
      # Also, skip blank lines and default lines
      #
      if ( ($value =~ /^#/) || ($value =~ /^desc=/) ) {
         next;
      }
      elsif ( (length($value) == 0) || ($value =~ /\s/) ) {
         next;
      }

      if ($value =~ /^\[default\]/) {
         $skip = 1;
         next;
      }

      #
      # Find the Signal Name line and copy the
      # value to the IPCx parts matrix
      #
      if ($value =~ /^\[([\w\:\-\_]+)\]/) {
         $skip = 0;
         $ipcxParamCnt++;

         $ipcxData[$ipcxParamCnt][0] = $1;
         $ipcxData[$ipcxParamCnt][1] = undef;
         $ipcxData[$ipcxParamCnt][2] = undef;
         $ipcxData[$ipcxParamCnt][3] = undef;

         next;
      }

      #
      # Copy the IPCx Communication Mechanism, the
      # Sender Data Rate, and the IPCx Number
      # to the IPCx parts matrix as well
      #
      if ($skip == 0)  {
         if ($value =~ /^ipcType=(\w+)/) {
            my $typeString = $1;
            $ipcxData[$ipcxParamCnt][1] = "I" . substr($typeString, 0, 3);

            if ($typeString =~ /^SHM/) {
               $typeIndex = 0;
            }
            elsif ($typeString =~ /^RFM/) {
               $typeIndex = 1;
            }
            elsif ($typeString =~ /^PCI/) {
               $typeIndex = 2;
            }
            else {
               die "***ERROR: IPCx Communication Mechanism not recognized: $typeString\n";
            }
         }
         elsif ($value =~ /^ipcRate=(\d+)/) {
            $ipcxData[$ipcxParamCnt][2] = $1;
         }
         elsif ($value =~ /^ipcNum=(\d+)/) {
            $ipcxData[$ipcxParamCnt][3] = $1;

            if ($1 > $ipcxMaxNum[$typeIndex]) {
               $ipcxMaxNum[$typeIndex] = $1;
            }
         }
      }
   }

   $ipcxParamCnt++;

   $ipcxNotFound = 0;
   $ipcxRcvrCnt = 0;

   for ($ii = 0; $ii < 3; $ii++) {
      if ($ipcxMaxNum[$ii] == -999) {
         $ipcxMaxNum[$ii] = -1;
      }
   }

   #
   # Locate each IPCx module in the IPCx parts matrix
   #
   for ($ii = 0; $ii < $ipcxCnt; $ii++) {
      $found = 0;

      if ($ipcxParts[$ii][0] =~ /^[A-Z]\d\:/) {
         $ipcxPartComp = $ipcxParts[$ii][0];
      }
      else {
         if ($ipcxParts[$ii][0] =~ /^\w+([A-Z]\d\:.+)/) {
            $ipcxPartComp = $1;
         }
         else {
            $ipcxPartComp = "";
         }
      }

      for ($jj = 0; $jj < $ipcxParamCnt; $jj++) {
         if ($ipcxPartComp eq $ipcxData[$jj][0]) {
            #
            # Make sure no IPCx parameters are missing
            #
            for ($kk = 2; $kk < 4; $kk++) {
               if (defined($ipcxData[$jj][$kk]) ) {
                  $ipcxParts[$ii][$kk] = $ipcxData[$jj][$kk];
               }
               else {
                  die "***ERROR: Data missing for IPCx component $ipcxParts[$ii][0] - $ipcxMissing[$kk]\n";
               }
            }

            $kk = $ipcxParts[$ii][4];
            $ipcxParts[$ii][5] = $partInput[$kk][0];
            $found = 1;
            $typeComp = $ipcxParts[$ii][1];

            if ($ipcxData[$jj][1] ne $typeComp) {
               die "***ERROR: IPCx type mis-match for IPCx component $ipcxParts[$ii][0]: $typeComp vs\. $ipcxData[$jj][1]\n";
            }

            #
            # Make sure each IPCx part has exactly one input
            #
            if ($partInCnt[$kk] != 1) {
               die "***ERROR: IPCx SENDER/RECEIVER component $ipcxParts[$ii][0] has $partInCnt[$kk] input(s)\n";
            }

            #
            # If the input to an IPCx part is 'Ground',
            # then this part is a RECEIVER of data,
            # which means there should be either one
            # or two outputs
            #
            if ( ($partInput[$kk][0] =~ /^Ground/) || ($partInput[$kk][0] =~ /\_Ground/) ) {
               if ( ($partOutCnt[$kk] < 1) || ($partOutCnt[$kk] > 2) ) {
                  die "***ERROR: IPCx RECEIVER component $ipcxParts[$ii][0] has $partOutCnt[$kk] output(s)\n";
               }
            }
            #
            # If the input to an IPCx part is NOT 'Ground',
            # then this part is a SENDER of data, which
            # means there should be no outputs
            #
            else {
               if ($partOutCnt[$kk] != 0) {
                  die "***ERROR: IPCx SENDER component $ipcxParts[$ii][0] has $partOutCnt[$kk] output(s)\n";
               }
            }

            last;
         }
      }

      #
      # Check if the IPCx module was found in the IPCx parts matrix
      #
      if ($found == 0) {
         print "### IPCx component ($ii) $ipcxParts[$ii][0] not found in IPCx parameter file $iFile\n";

         $ipcxAdd[$ipcxNotFound][0] = $ii;

         $kk = $ipcxParts[$ii][4];

         if ( ($partInput[$kk][0] =~ /^Ground/) || ($partInput[$kk][0] =~ /\_Ground/) ) {
            if ( ($partOutCnt[$kk] < 1) || ($partOutCnt[$kk] > 2) ) {
               die "***ERROR: IPCx RECEIVER component $ipcxParts[$ii][0] has $partOutCnt[$kk] output(s)\n";
            }
            else {
               $ipcxAdd[$ipcxNotFound][1] = 2;
               $ipcxRcvrCnt++;
            }
         }
         else {
            if ($partOutCnt[$kk] != 0) {
               die "***ERROR: IPCx SENDER component $ipcxParts[$ii][0] has $partOutCnt[$kk] output(s)\n";
            }
            else {
               $ipcxAdd[$ipcxNotFound][1] = 1;
            }
         }

         $ipcxNotFound++;
      }
   }

   #
   # Check if there are any IPCx modules to add to the IPCx parts matrix
   #
   if ($ipcxNotFound > 0) {
      open (IPCOUT, ">>$iFile") || die "***ERROR: Could not open IPCx parameter file $iFile for output\n";

      $ipcxRate = 983040/$rate;
      $ipcxNew = 0;

      for ($jj = 0; $jj < $ipcxNotFound; $jj++) {
         if ($ipcxAdd[$jj][1] == 1) {
            #
            # Get type index
            #
            $ipcxTypeIndex = -999;

            $ipcxCommMech = substr($ipcxParts[$ipcxAdd[$jj][0]][1], 1, 3);

            for ($kk = 0; $kk < 3; $kk++) {
               if ($ipcxCommMech eq substr($ipcxType[$kk], 0,3) ) {
                  $ipcxTypeIndex = $kk;
               }
            }

            if ($ipcxTypeIndex < 0) {
               die "***ERROR: IPCx Communication Mechanism not recognized: $ipcxCommMech\n";
            }

            #
            # Add data to the IPCx parameter file
            #
            if (++$ipcxMaxNum[$ipcxTypeIndex] > 999) {
               die "***ERROR: IPCx number > 999 for ipcType = $ipcxType[$ipcxTypeIndex]\n";
            }

            $signalName = $ipcxParts[$ipcxAdd[$jj][0]][0];
            if ($signalName =~ /^\w+([A-Z]\d\:.+)/) {
               $signalName = $1;
            }

            print IPCOUT "\[$signalName\]\n";
            print IPCOUT "ipcType=$ipcxType[$ipcxTypeIndex]\n";
            print IPCOUT "ipcRate=$ipcxRate\n";
            print IPCOUT "ipcNum=$ipcxMaxNum[$ipcxTypeIndex]\n";
            print IPCOUT "desc=Automatically generated by feCodeGen\.pl on $theTime\n\n";

            $ipcxDataAdded[$ipcxNew][0] = $ipcxParts[$ipcxAdd[$jj][0]];
            $ipcxDataAdded[$ipcxNew][1] = "I" . substr($ipcxType[$ipcxTypeIndex], 0, 3);
            $ipcxDataAdded[$ipcxNew][2] = $ipcxRate;
            $ipcxDataAdded[$ipcxNew][3] = $ipcxMaxNum[$ipcxTypeIndex];

            $ipcxParts[$ipcxAdd[$jj][0]][2] = $ipcxRate;
            $ipcxParts[$ipcxAdd[$jj][0]][3] = $ipcxMaxNum[$ipcxTypeIndex];

            $ipcxNew++;
         }
      }

      close IPCOUT;

      #
      # This code can only automatically add IPCx SENDER modules
      #
      if ($ipcxRcvrCnt > 0) {
         print "\n\n***ERROR: The following IPCx RECEIVER modules not found in the file $iFile:\n\n";

         for ($jj = 0; $jj < $ipcxNotFound; $jj++) {
            if ($ipcxAdd[$jj][1] == 2) {
               print "\t\t$ipcxParts[$ipcxAdd[$jj][0]][0]\n";
            }
         }

         die "\n***ERROR: Aborting\n\n";
      }
   }
}

for($ii=0;$ii<$partCnt;$ii++)
{
	if($partType[$ii] eq "INPUT")
	{
		#print "INPUT $xpartName[$ii]  $partInput[$ii][0] $partInputPort[$ii][0] $partOutput[$ii][0] $partOutputPort[$ii][0]\n";
	}
}
# Loop thru all parts
for($ii=0;$ii<$partCnt;$ii++)
{
	# Loop thru all part input connections
        for($jj=0;$jj<$partInCnt[$ii];$jj++)
        {
	   # Loop thru all parts looking for part name that matches input name
           for($kk=0;$kk<$partCnt;$kk++)
           {
		# If name match
                if($partInput[$ii][$jj] eq $xpartName[$kk])
                {
			# If input is from a BUSS part
                        if($partType[$kk] eq "BUSS")
                        {
				#print "BUSS connect to $xpartName[$ii] port $jj from $xpartName[$kk] \n";
				$gfrom = $partInputPort[$ii][$jj];
				$adcName = $partInput[$kk][$gfrom];
				$adcTest = substr($adcName,0,3);
				if($adcTest eq "adc")
				{
    					($var1,$var2) = split(' ',$adcName);
					# print  "\t this is an adc connect $var1 $var2\n";
					$partInputType[$kk][$gfrom] = "Adc";

				} else {
    					($var1,$var2) = split(' ',$adcName);
					print "\t $xpartName[$kk] this other part connect $var1 port $var2\n";
					$partInput[$ii][$jj] = $var1;
        				for($ll=0;$ll<$partInCnt[$kk];$ll++)
					{
						#print "\tInput $ll = $partInput[$kk][$ll]\n";
                				if($partInput[$kk][$ll] eq $adcName)
						{
							$port = $partInputPort[$kk][$ll];
							 $ports = $partOutputPort[$kk][$ll];
							$ports = $var2;
							#print "\t Input port number is $port $ports from $xpartName[$kk]\n";
							$partInputPort[$ii][$jj] = $ports;
							for($xx=0;$xx<$partCnt;$xx++)
							{
								if($xpartName[$xx] eq $var1)
								{
									$partOutput[$xx][$ports] = $xpartName[$ii];
									$partOutputPort[$xx][$ports] = $jj;
									#print "\tMatched to $xpartName[$xx] port $ports\n";
									#print "\t$partOutput[$xx][$ports] $partOutputPort[$xx][$jj]\n\n";
								}
							}
						}
					}
					
				}
                        }
                }
           }
        }
}

#exit(1);

#print "Looped thru $partCnt looking for BUSS \n\n\n";

# FIND all FROM links
# Supports MATLAB tags ie types Goto and From parts.
# This section searches all part inputs for From tags, finds the real name
# of the signal being sent to this tag, and substitutes that name at the part input.

# Loop thru all parts
for($ii=0;$ii<$partCnt;$ii++)
{
	# Loop thru all part output connections
        for($jj=0;$jj<$partOutCnt[$ii];$jj++)
        {
$xp = 0;
	   # Loop thru all parts looking for part name that matches input name
           for($kk=0;$kk<$partCnt;$kk++)
           {
		# If name match
                if($partOutput[$ii][$jj] eq $xpartName[$kk])
                {
			# If output is to a GOTO part
                        if($partType[$kk] eq "GOTO")
                        {
				# Search thru all parts looking for a matching GOTO tag
                                for($mm=0;$mm<$partCnt;$mm++)
                                {
                                        if(($partType[$mm] eq "FROM") && ($partInput[$mm][3] eq $partOutput[$kk][3]) )
                                        {
						# Remove parts input FROM tag name with actual originating part name
						#print "MATCHED GOTO $xpartName[$kk] with FROM $xpartName[$mm] $partInput[$ii][$jj] to $partInput[$mm][1] $partType[$ii] $partOutCnt[$mm] $partOutput[$mm][0]\n";
						$partGoto[$mm] = $xpartName[$ii];
	   					$totalPorts = $partOutCnt[$ii];
						$xp = 0;
						for($yy=0;$yy<$partOutCnt[$mm];$yy++)
						{
							# $partInput[$ii][$jj] = $partInput[$mm][0];
							if($yy > 0)
							{
								$port = $totalPorts;
								$totalPorts ++;
								$xp ++;
							} else {
								$port = $jj;
							}
							$partOutput[$ii][$port] = $partOutput[$mm][$yy];
							#$partOutput[$mm][0] = $xpartName[$ii];
							#$partOutput[$mm][0] = "";
							if($partType[$ii] eq "OUTPUT")
							{
							$partOutputPort[$ii][$port] = $partOutputPort[$mm][$yy]+1;;
							} else {
							$partOutputPort[$ii][$port] = $partOutputPort[$mm][$yy];;
							}
						#print "\t $partOutput[$ii][$jj] $partOutput[$mm][0] $partOutputPort[$ii][$jj]\n";
						#print "\t $xpartName[$ii] $partOutput[$ii][$jj] $partOutputPort[$ii][$jj]\n";
						}
							$partOutCnt[$mm] = 0;

					#$partOutput[$xx][$jj] = $xpartName[$toNum];
					#$partOutputPort[$xx][$jj] = $toPort;
					#$partOutNum[$xx][$jj] = $toNum;
					#$partOutputType[$xx][$jj] = $partType[$toNum];

                                        }
                                }
                        }
                }
           }
        }
	if($xp > 0) 
	{
		  $partOutCnt[$ii]  = $totalPorts;
		print "\t **** $xpartName[$ii] has new output count $totalParts $partOutCnt[$ii]\n";
	}
}

# exit(1);

# FIND all FROM links
# Supports MATLAB tags ie types Goto and From parts.
# This section searches all part inputs for From tags, finds the real name
# of the signal being sent to this tag, and substitutes that name at the part input.

# Loop thru all parts
for($ii=0;$ii<$partCnt;$ii++)
{
        # Loop thru all part input connections
        for($jj=0;$jj<$partInCnt[$ii];$jj++)
        {
           # Loop thru all parts looking for part name that matches input name
           for($kk=0;$kk<$partCnt;$kk++)
           {
                # If name match
                if($partInput[$ii][$jj] eq $xpartName[$kk])
                {
                        # If input is from a FROM part
                        if($partType[$kk] eq "FROM")
                        {
 #print "Found FROM $partInput[$kk][1] in part $xpartName[$ii]\n";
                                # Search thru all parts looking for a matching GOTO tag
                                for($ll=0;$ll<$partCnt;$ll++)
                                {
                                        # If GOTO tag matches FROM tag
                                        if(($partType[$ll] eq "GOTO") && ($partOutput[$ll][3] eq $partInput[$kk][3]) )
                                        {
                                                # Remove parts input FROM tag name with actual originating part name
                                                 #print "\t matched port $jj $partInCnt[$ii] $xpartName[$ii] $partInput[$kk][$jj] to $partOutput[$kk][1] $partInput[$kk][1]\n";
                                                $partInput[$ii][$jj] = $partGoto[$kk];
                                        }
                                }
                        }
                }
           }
        }
}
#print "\nLooped thru Froms  \n\n\n";




# ********************************************************************
# Take all of the part outputs and find connections.
# Fill in connected part numbers and types.
for($ii=0;$ii<$partCnt;$ii++)
{
	for($jj=0;$jj<$partOutCnt[$ii];$jj++)
	{
	   for($kk=0;$kk<$partCnt;$kk++)
	   {
		if($partOutput[$ii][$jj] eq $xpartName[$kk])
		{
			$partOutNum[$ii][$jj] = $kk;
			$partOutputType[$ii][$jj] = $partType[$kk];
		}
	   }
	}
}

# Take all of the part inputs and find connections.
# Fill in connected part numbers and types.
for($ii=0;$ii<$partCnt;$ii++)
{
	for($jj=0;$jj<$partInCnt[$ii];$jj++)
	{
	   for($kk=0;$kk<$partCnt;$kk++)
	   {
		if($partInput[$ii][$jj] eq $xpartName[$kk])
		{
			#print "Part Input Number $xpartName[$ii] $jj eq $kk\n";
			$partInNum[$ii][$jj] = $kk;
			$partInputType[$ii][$jj] = $partType[$kk];
			$partSysFromx[$ii][$jj] = -1;
		}
	   }
	}
}

# Start the process of removing subsystems OUTPUT parts 
for($ii=0;$ii<$partCnt;$ii++)
{
$foundCon = 0;
$foundSysCon = 0;
	if($partType[$ii] eq "OUTPUT")
	{
	for($jj=0;$jj<$partOutCnt[$ii];$jj++)
	{
	   for($kk=0;$kk<$partCnt;$kk++)
	   {
		# If OUTPUT connects to INPUT of another subsystem
		if($partType[$kk] eq "INPUT")
		{
	      for($ll=0;$ll<$partOutCnt[$kk];$ll++)
        	{
		if(($partOutput[$ii][$jj] eq $partInput[$kk][0]) && ($partOutputPort[$ii][$jj] == $partInputPort[$kk][0]))
		{
			#$partOutput[$ii][$jj] = $xpartName[$kk];
			#$partOutNum[$ii][$jj] = $kk;
			$partSysFrom[$kk] = $partSubNum[$ii];
                       $fromNum = $partInNum[$ii][0];
                       $fromPort = $partInputPort[$ii][0];
                       $toNum = $partOutNum[$kk][$ll];
                       $toPort = $partOutputPort[$kk][$ll];
			#print "Connection from $xpartName[$ii] $jj $fromNum $fromPort to $xpartName[$kk] $toNum $toPort\n";
			#print"\t$xpartName[$fromNum] $fromPort to $xpartName[$toNum] $toPort\n";
			for($vv=0;$vv<$partOutCnt[$fromNum];$vv++)
			{
				if($partOutput[$fromNum][$vv] eq $xpartName[$ii])
				{
					$fromLink = $vv;
				}
			}
                       $partOutput[$fromNum][$fromLink] = $xpartName[$toNum];
                       $partOutputType[$fromNum][$fromLink] = $partType[$toNum];
                       $partOutNum[$fromNum][$fromLink] = $toNum;
                       $partOutputPort[$fromNum][$fromLink] = $toPort;
                       $partInput[$toNum][$toPort] = $xpartName[$fromNum];
                       $partInputType[$toNum][$toPort] = $partType[$fromNum];
                       $partInNum[$toNum][$toPort] = $fromNum;
                       $partInputPort[$toNum][$toPort] = $fromPort;
                       $partSysFrom[$kk] = $partSubNum[$ii];
                       $partInputType[$kk][0] = "SUBSYS";
                       $partInNum[$kk][0] = $partSubNum[$ii];

			$foundSysCon = 1;
		}
		}
		}
	   }
	}

	# OUTPUT did not connect to INPUT, so find the part connections.
	if($foundCon == 0){
	for($jj=0;$jj<$partOutCnt[$ii];$jj++)
	{
	   for($kk=0;$kk<$nonSubCnt;$kk++)
	   {
	      $xx = $nonSubPart[$kk];
			if($partOutput[$ii][$jj] eq $partName[$xx])
			{
				$fromNum = $partInNum[$ii][0]; # OUTPUT part has only one input!
#  If OUTPUT part has only one input, then the line below can NOT be correct!
#				$fromPort = $partInputPort[$ii][$jj];
#  Replace with the following line:
				$fromPort = $partInputPort[$ii][0];
				for($xxx=0;$xxx<$partOutCnt[$fromNum];$xxx++)
				{
					if($xpartName[$ii] eq $partOutput[$fromNum][$xxx])
					{
						$fromPort = $xxx;
					}
				}
			# print " OUTPUT TO $xpartName[$ii] $xpartName[$xx] $partType[$xx]\n";
			# print "Maybe $xpartName[$xx] port $partOutputPort[$ii][$jj] $xpartName[$fromNum] $partType[$fromNum] port $fromPort\n";
				# Make output connection at source part
				$partOutput[$fromNum][$fromPort] = $xpartName[$xx];
				$partOutputType[$fromNum][$fromPort] = $partType[$xx];
				$partOutNum[$fromNum][$fromPort] = $xx;
				$partOutputPort[$fromNum][$fromPort] = $partOutputPort[$ii][$jj];
#print "$xpartName[$xx] $partType[$xx] $xx $partOutputPort[$ii][$jj]\n";
#print "$xpartName[$fromNum] $fromPort\n\n";
                       		# $partSysFromx[$xx][$fromCnt[$xx]] = $partSubNum[$ii];
				# Make input connection at destination part
#  If OUTPUT part has only one input, then the line below can NOT be correct!
#				$fromPort = $partInputPort[$ii][$jj];
#  Replace with the following line:
 				$fromPort = $partInputPort[$ii][0];
				$qq = $partOutputPort[$ii][$jj] - 1;
				$partInput[$xx][$qq] = $xpartName[$fromNum];
				$partInputType[$xx][$qq] = $partType[$fromNum];
				$partInNum[$xx][$qq] = $fromNum;
				$partInputPort[$xx][$qq] = $fromPort;
                       		$partSysFromx[$xx][$qq] = $partSubNum[$ii];
				$fromCnt[$xx] ++;
				$foundCon = 1;
			}
	   }
	}

	}

	# Did not find any connections to subsystem OUTPUT, so print error.
	if($foundCon == 0 && $foundSysCon == 0){
	print "No connect for $xpartName[$ii] $partOutput[$ii][0] $partOutputPort[$ii][0]\n";
	}
	}
}

# Find connections for non subsystem parts
for($ii=0;$ii<$partCnt;$ii++)
{
	$xx = $ii;
	# If part is BUSS, indicating an ADC input part
	if($partType[$xx] eq "BUSS")
	{
		for($jj=0;$jj<$partOutCnt[$xx];$jj++)
		{
		   $mm = $partOutputPort[$xx][$jj]+1;
		   for($kk=0;$kk<$partCnt;$kk++)
		   {
			# Handle ADC input connections to a subsystem, as indicated by
			# an INPUT part.
			if($partType[$kk] eq "INPUT")
			{
				if(($partOutput[$xx][$jj] eq $partInput[$kk][0]) && ($mm == $partInputPort[$kk][0]))
				{
				$fromNum = $xx;
				$fromPort = $partOutputPortUsed[$xx][$jj];
				# $fromPort = $partInputPort[$kk][0];
				$adcName = $partInput[$xx][$fromPort];
				$adcTest = substr($adcName,0,3);
				if($adcTest eq "adc")
                                {
				$partInput[$kk][0] = $xpartName[$xx];
				$partInputType[$kk][0] = "Adc";
				$partInNum[$kk][0] = $xx;
				#$partInputPort[$kk][0] = $fromPort;
				$partSysFrom[$kk] = 100 + $xx;
				#print "Found ADC INPUT connect from $xpartName[$xx] to $xpartName[$kk] $partOutputPort[$xx][$jj] $jj  input $fromPort $partInput[$xx][$fromPort]\n";
				for($ll=0;$ll<$partOutCnt[$kk];$ll++)
				{
					$adcName = $partInput[$xx][$fromPort];
				$adcTest = substr($adcName,0,8);
				#print "FOUND INPUT BUSS connection  $xpartName[$kk] $adcTest!!!!!!!!!!!!!!!!!\n";
					$adcNum = substr($adcName,4,1);
					$adcChan = substr($adcName,6,2);
					$toNum = $partOutNum[$kk][$ll];
				        $toPort = $partOutputPort[$kk][$ll];
					$partInNum[$toNum][$toPort] = $adcNum;
					#$partInput[$toNum][$toPort] = $xpartName[$xx];
					$partInput[$toNum][$toPort] = $adcName;
					$partInputPort[$toNum][$toPort] = $adcChan;
					$partInputType[$toNum][$toPort] = "Adc";
					#print "\tNew adc connect $xpartName[$toNum] $adcNum $adcChan to partnum $partInput[$toNum][$toPort]\n";
				}
				} else {
    					($var1,$var2) = split(' ',$adcName);
				        for($yy=0;$yy<$partCnt;$yy++)
				        {
						if($xpartName[$yy] eq $var1)
						{
							$conPartType = $partType[$yy];
							$conPortNum = $var1;
							$conInNum = $yy;
							#$partInputType[$kk][0] = "PART";
							#$partInputPort[$kk][0] = $var2;
							#$$partInNum[$kk][0] = $yy;
							#$partInNum[$kk][0] = $partOutNum[$xx][$jj];;
							#print "\t SUBSYS input with bus name $xpartName[$yy] $conPartType $conPortNum $conInNum\n";
						}
					}
					for($ll=0;$ll<$partOutCnt[$kk];$ll++)
					{
					#print "FOUND INPUT BUSS connection  $xpartName[$kk] $adcTest!!!!!!!!!!!!!!!!!\n";
						$toNum = $partOutNum[$kk][$ll];
						$toPort = $partOutputPort[$kk][$ll];
						$partInNum[$toNum][$toPort] = $conInNum;
						#$partInput[$toNum][$toPort] = $xpartName[$xx];
						$partInput[$toNum][$toPort] = $var1;
						$partInputPort[$toNum][$toPort] = $var2;
						$partInputType[$toNum][$toPort] = $conPartType;
						#print "\tNew BUSS connect $xpartName[$toNum]  to partnum $partInput[$toNum][$toPort]\n";
						$partOutput[$conInNum][$var2] = $xpartName[$toNum];
						$partOutNum[$conInNum][$var2] = $toNum;
						$partOutputType[$conInNum][$var2] = $partType[$toNum];
					}
				}
				}
			}
			# Handle ADC input connections directly to a part not in a subsystem.
			if($partType[$kk] ne "INPUT")
			{
				if(($partOutput[$xx][$jj] eq $xpartName[$kk]))
				{
				 # print "Found ADC NP connect $xpartName[$xx] to $xpartName[$kk] $partOutputPort[$xx][$jj]\n";
				 # print "$jj $partOutputPort[$xx][$jj] $partOutputPortUsed[$xx][$jj]\n";
				$fromNum = $xx;
				#$fromPort = $jj;
				#$fromPort = $partInputPort[$kk][0];
				#$fromPort = $partOutputPort[$xx][$jj];

				#print "FOUND BUSS connection  $xpartName[$kk] $adcTest!!!!!!!!!!!!!!!!!\n";

				$fromPort = $partOutputPortUsed[$xx][$jj];
				$toPort = $partOutputPort[$xx][$jj];
				$adcName = $partInput[$xx][$fromPort];
				$adcTest = substr($adcName,0,3);
				if($adcTest eq "adc")
				{
				#print "\tFOUND ADC  connection  from $xpartName[$xx] to $xpartName[$kk] $adcName $adcTest!!!!!!!!!!!!!!!!!\n";
				$partInput[$kk][$toPort] = $xpartName[$xx];
				$partInputType[$kk][$toPort] = "Adc";
				$partInNum[$kk][$toPort] = $xx;
				$partInputPort[$kk][$toPort] = $fromPort;
					$adcNum = substr($adcName,4,1);
					$adcChan = substr($adcName,6,2);
					$partInput[$kk][$toPort] = $adcName;
					$partInputPort[$kk][$toPort] = $adcChan;
					$partInNum[$kk][$toPort] = $adcNum;
				} else {
					#print "FOUND BUSS connection which is not ADC $xpartName[$kk] $adcTest!!!!!!!!!!!!!!!!!\n";
				}
				}
			}
		   }
		}
	}
}

	# Handle part connections, where neither part is an ADC part.
	#if(($partType[$xx] ne "BUSS") && ($partType[$xx] ne "FROM") && ($partType[$xx] ne "GOTO") )
for($ii=0;$ii<$nonSubCnt;$ii++)
{
	$xx = $nonSubPart[$ii];
	if($partType[$xx] ne "BUSS")
	{
		for($jj=0;$jj<$partOutCnt[$xx];$jj++)
		{
		   $mm = $partOutputPort[$xx][$jj]+1;
		   for($kk=0;$kk<$partCnt;$kk++)
		   {
			if($partType[$kk] eq "INPUT")
			{
				if(($partOutput[$xx][$jj] eq $partInput[$kk][0]) && ($mm == $partInputPort[$kk][0]))
				{
			         $partInputType[$kk][0] = "PART";
				$partInNum[$kk][0] = $xx;
				
				for($ll=0;$ll<$partOutCnt[$kk];$ll++)
				{
					$toNum = $partOutNum[$kk][$ll];
					$toPort = $partOutputPort[$kk][$ll];
					$toPort1 = $partOutputPortUsed[$kk][$ll];
				#print "Found nonADC connect from $xpartName[$xx] port $mm to $xpartName[$toNum] $partInputPort[$xx][$jj] $partOutput[$xx][$jj]\n";
					$partInNum[$toNum][$toPort] = $xx;
					$partInput[$toNum][$toPort] = $xpartName[$xx];
					$partInputPort[$toNum][$toPort] = $toPort1;
					$partInputType[$toNum][$toPort] = $partType[$xx];

					$partOutput[$xx][$jj] = $xpartName[$toNum];
					$partOutputPort[$xx][$jj] = $toPort;
					$partOutNum[$xx][$jj] = $toNum;
					$partOutputType[$xx][$jj] = $partType[$toNum];
				}
				$partSysFrom[$kk] = 100 + $xx;
				}
			}
		   }
		}
	}
}

# Remove all parts which will not require further processing in the code for the part
# total.
$ftotal = $partCnt;
   for($kk=0;$kk<$partCnt;$kk++)
   {
	 if(($partType[$kk] eq "INPUT") || ($partType[$kk] eq "OUTPUT") || ($partType[$kk] eq "BUSC") || ($partType[$kk] eq "BUSS") || ($partType[$kk] eq "EpicsIn") || ($partType[$kk] eq "TERM") || ($partType[$kk] eq "FROM") || ($partType[$kk] eq "GOTO") || ($partType[$kk] eq "GROUND") || ($partType[$kk] eq "CONSTANT") || ($partType[$kk] eq "Adc"))
	{
		$ftotal --;
	}
   }

print "Total parts to process $ftotal\n";

# DIAGNOSTIC
print "Found $subSys subsystems\n";

# Write a parts and connection list to file for diagnostics.
for ($ll = 0; $ll < $subSys; $ll++) {
  $xx = $subSysPartStop[$ll] - $subSysPartStart[$ll]; # Parts count for this subsystem
  $subCntr[$ll] = 0;
  #print "SubSys $ll $subSysName[$ll] from $subSysPartStart[$ll] to $subSysPartStop[$ll]\n";
  print OUTD "\nSubSystem $ll has $xx parts ************************************\n";
  for ($ii = $subSysPartStart[$ll]; $ii < $subSysPartStop[$ll]; $ii++) {
    print OUTD "Part $ii $xpartName[$ii] is type $partType[$ii] with $partInCnt[$ii] inputs and $partOutCnt[$ii] outputs\n";
    print OUTD "INS FROM:\n";
    print OUTD "\tPart Name\tType\tNum\tPort\n";
    for($jj=0;$jj<$partInCnt[$ii];$jj++) {
      #$from = $partInNum[$ii][$jj];
      print OUTD "\t$partInput[$ii][$jj]\t$partInputType[$ii][$jj]\t$partInNum[$ii][$jj]\t$partInputPort[$ii][$jj]\n";
      if (($partType[$ii] eq "INPUT") && ($partOutCnt[$ii] > 0)) {
	print OUTD "From Subsystem $partSysFrom[$ii]\n";
	$subInputs[$ll][$subCntr[$ll]] = $partSysFrom[$ii];
	$subInputsType[$ll][$subCntr[$ll]] = $partInputType[$ii][$jj];
	$subCntr[$ll] ++;
      }
    }
    print OUTD "OUT TO:\n";
    print OUTD "\tPart Name\tType\tNum\tPort\tPort Used\n";
    for($jj = 0;$jj < $partOutCnt[$ii]; $jj++) {
      #$to = $partOutNum[$ii][$jj];
      print OUTD "\t$partOutput[$ii][$jj]\t$partOutputType[$ii][$jj]\t$partOutNum[$ii][$jj]\t$partOutputPort[$ii][$jj]\t$partOutputPortUsed[$ii][$jj]\n";
    }
    print OUTD "\n****************************************************************\n";
  }
}
print OUTD "Non sub parts ************************************\n";
for ($ii = 0; $ii < $nonSubCnt; $ii++) {
  $xx = $nonSubPart[$ii];
  print OUTD "Part $xx $xpartName[$xx] is type $partType[$xx] with $partInCnt[$xx] inputs and $partOutCnt[$xx] outputs\n";
  print OUTD "INS FROM:\n";
  for ($jj = 0; $jj < $partInCnt[$xx]; $jj++) {
    #$from = $partInNum[$xx][0];
    if ($partSysFromx[$xx][$jj] == -1) {
      print OUTD "\t$partInput[$xx][$jj]\t$partInputType[$xx][$jj]\t$partInNum[$xx][$jj]\t$partInputPort[$xx][$jj] subsys NONE\n";
    } else {
      print OUTD "\t$partInput[$xx][$jj]\t$partInputType[$xx][$jj]\t$partInNum[$xx][$jj]\t$partInputPort[$xx][$jj] subsys $partSysFromx[$xx][$jj]\n";
    }
  }
  print OUTD "OUT TO:\n";
  print OUTD "\tPart Name\tType\tNum\tPort\tPort Used\n";
  for ($jj = 0; $jj < $partOutCnt[$xx]; $jj++) {
    $to = $partOutNum[$xx][$jj];
    print OUTD "\t$partOutput[$xx][$jj]\t$partOutputType[$xx][$jj]\t$partOutNum[$xx][$jj]\t$partOutputPort[$xx][$jj]\t$partOutputPortUsed[$xx][$jj]\n";
  }
  print OUTD "\n****************************************************************\n";
}
for ($ii = 0; $ii < $subSys; $ii++) {
  print OUTD "\nSUBS $ii $subSysName[$ii] *******************************\n";
  for($ll=0;$ll<$subCntr[$ii];$ll++) {
    print OUTD "$ll $subInputs[$ii][$ll] $subInputsType[$ii][$ll]\n";
  }
}
close(OUTD);
# End DIAGNOSTIC

print "Found $adcCnt ADC modules part is $adcPartNum[0]\n";
print "Found $dacCnt DAC modules part is $dacPartNum[0]\n";
print "Found $boCnt Binary modules part is $boPartNum[0]\n";

for($ii=0;$ii<$subSys;$ii++)
{
	$partsRemaining = $subSysPartStop[$ii] - $subSysPartStart[$ii];
	$counter = 0;
	$ssCnt = 0;
 print "SUB $ii has $partsRemaining parts *******************\n";
	for($jj=$subSysPartStart[$ii];$jj<$subSysPartStop[$ii];$jj++)
	{
		#if(($partType[$jj] eq "INPUT") || ($partType[$jj] eq "BUSS") || ($partType[$jj] eq "GROUND") || ($partType[$jj] eq "FROM") || ($partType[$jj] eq "GOTO") || ($partType[$jj] eq "EpicsIn") || ($partType[$jj] eq "CONSTANT"))
		if(($partType[$jj] eq "INPUT") || ($partType[$jj] eq "BUSS") || ($partType[$jj] eq "GROUND") || ($partType[$jj] eq "EpicsIn") || ($partType[$jj] eq "CONSTANT"))
				{
					$partsRemaining --;
					$partUsed[$jj] = 1;
					for($kk=0;$kk<$partOutCnt[$jj];$kk++)
					{
						
						if(($partType[$jj] eq "BUSS") && ($partInputType[$jj][0] eq "Adc"))
						{
							print "BUSS FOUND ****  $xpartName[$jj] $seqNum[0][$counter] $partInputType[$jj][0]\n";
							$ll = $partOutNum[$jj][$kk];
							$seqNum[0][$counter] = $ll;
							$counter ++;
						}
						if($partType[$jj] ne "BUSS")
						{
							$ll = $partOutNum[$jj][$kk];
							$seqNum[0][$counter] = $ll;
							$counter ++;
						}
					    
					}
				}
				if(($partType[$jj] eq "OUTPUT") || ($partType[$jj] eq "FROM") || ($partType[$jj] eq "GOTO") ||($partType[$jj] eq "TERM") || ($partType[$jj] eq "BUSC") || ($partType[$jj] eq "Adc"))
				#if(($partType[$jj] eq "OUTPUT") || ($partType[$jj] eq "TERM") || ($partType[$jj] eq "BUSC") || ($partType[$jj] eq "Adc"))
				{
					$partsRemaining --;
					$partUsed[$jj] = 1;
				}
			}
			print "Found $counter Inputs for subsystem $ii with $partsRemaining parts*********************************\n";
			$xx = 0;
			$ts = 1;
			until(($partsRemaining < 1) || ($xx > 200))
			{
				$xx ++;
				$loop = $counter ++;
				$counter = 0;
				if($ts == 1) {
					$ts = 0;
					$ns = 1;
				}
				else {
					$ts = 1;
					$ns = 0;
				}
				for($jj=0;$jj<$loop;$jj++)
				{
					$mm = $seqNum[$ts][$jj];
					$partInUsed[$mm] ++;
					if(($partInUsed[$mm] >= $partInCnt[$mm]) && ($partUsed[$mm] == 0))
					{
						$partUsed[$mm] = 1;
						$partsRemaining --;
						$seq[$ii][$ssCnt] = $mm;
						$seqName[$ii][$ssCnt] = $xpartName[$mm];
						 #print "Sub $ii part $ssCnt = $mm $xpartName[$mm]\n";
						$ssCnt ++;
						for($kk=0;$kk<$partOutCnt[$mm];$kk++)
						{
							$ll = $partOutNum[$mm][$kk];
							if(($ll >= $subSysPartStart[$ii]) && ($ll < $subSysPartStop[$ii]))
							{
								$seqNum[$ns][$counter] = $ll;
								$counter ++;
							}
						}
					}
				}

			}
			print " ********************* Parts remaining = $partsRemaining\n";
			$seqParts[$ii] = $ssCnt;
		}

		$partsRemaining = 0;
		$searchCnt = 0;
		for($ii=0;$ii<$nonSubCnt;$ii++)
		{
			$xx = $nonSubPart[$ii];
			if(($partType[$xx] ne "BUSC") && ($partType[$xx] ne "FROM") &&($partType[$xx] ne "GOTO") && ($partType[$xx] ne "BUSS") && ($partType[$xx] ne "Adc"))
			{
				$searchPart[$partsRemaining] = $xx;
				$searchCnt ++;
				$partsRemaining ++;
				print "Part num $xx $partName[$xx] is remaining\n";
			}
		}
		$subRemaining = $subSys;
		$seqCnt = 0;

		#
		$old_style_multiprocessing = 0;

		# Total number of CPUs available to us
		$cpus = 2;

		# subSysName -> step*10 + cpu
		# 'step' is the processing step from one  sync point to the next
		# 'cpu' is the processor number: 0, 1 ... $cpus
		%sys_cpu_step;

		# Current processing step (a running counter)
		$cur_step = 1;

		# How many processors are available on the current processing step
		# Running counter
		$cur_step_cpus = $cpus;

		# Parts tree root name
		$tree_root_name = "__root__";

	# Parts "tree" root
	# It points to ADC input groups (they are grouped for each subsystem)
$root = {
	NAME => $tree_root_name,
	NEXT => [], # array of references to leaves
};

# Run a function on all tree leaves
# Arguments:
#	tree root reference
#	function reference
#
sub do_on_leaves {
	my($tree, $f) = @_;
	if (0 == @{$tree->{NEXT}}) {
		&$f($tree);
	} else {
		for (@{$tree->{NEXT}}) {
                	do_on_leaves($_, $f);
        	}
	}
}

# Run a function on all tree nodes
# Arguments:
#	tree root reference
#	function reference
#	function argument
sub do_on_nodes {
	my($tree, $f, $arg) = @_;
	&$f($tree, $arg);
	for (@{$tree->{NEXT}}) {
               	do_on_nodes($_, $f, $arg);
	}
}

# Find graph node by name
#
sub find_node {
	my($tree, $name) = @_;
	if ($tree->{NAME} eq $name) {
		return $tree;
	} else {
		for (@{$tree->{NEXT}}) {
			#print $_->{NAME}, "\n";
                	$res = find_node($_, $name);
			if ($res) {
				return $res;
			}
        	}
	}
	return undef;
}

# Construct parts linked list
#
foreach $i (0 .. $subSys-1) {
	debug(0, "Subsystem $i ", $subSysName[$i]);
}

if ($cpus > 2) {
foreach $i (0 .. $subSys-1) {
	debug(2, "Subsystem $i (", $subSysName[$i], "):");
	debug(2, " inputs=", $subCntr[$i]);
	debug(2, "  parts=", $seqParts[$i]);
	# See if this subsystem has ADC inputs
	$has_inputs = 0;
	for ($subSysPartStart[$i] .. $subSysPartStop[$i]) {
		if ($partType[$_] eq "INPUT" && $partInputType[$_][0] eq "Adc") {
			$has_inputs = 1;
			break;
		}
	}

	# insert new tree node for the group of inputs
	# and the input parts afterwards
	if ($has_inputs) {
		$inputs_node = {
			NAME => $subSysName[$i] . "_INPUTS",
			TYPE => "input_group",
			NEXT => [],
			PART => -1,
			SUBSYS => $i,
		};
		push @{$root->{NEXT}}, $inputs_node;

		# Insert individual ADC inputs after the input group
		for ($subSysPartStart[$i] .. $subSysPartStop[$i]) {
			if ($partInput[$_][0] =~ m/adc_(\d+)_(\d+)/
			    && $partInputType[$_][0] eq "Adc") {
				$node = {
					NAME => $xpartName[$_],
					TYPE => $partType[$_],
					NEXT => [],
					PART => $_,
					SUBSYS => $i,
				};
				push @{$inputs_node->{NEXT}}, $node;
			}
		}
	}

	# Insert all ground and const inputs into the root
	for ($subSysPartStart[$i] .. $subSysPartStop[$i]) {
		if (($partType[$_] eq "GROUND") || ($partType[$_] eq "CONSTANT")) {
			$node = {
				NAME => $xpartName[$_],
				TYPE => $partType[$_],
				NEXT => [],
				PART => $_,
				SUBSYS => $i,
			};
			push @{$root->{NEXT}}, $node;
		}
	}

	debug(2, "  {");
	foreach $j ($subSysPartStart[$i] .. $subSysPartStop[$i]) {
		debug(2, "\tPart $j ", $xpartName[$j], " type=", $partType[$j], " inputs=", $partInCnt[$j], " outputs=", $partOutCnt[$j]);
		
		#debug(2, "\tPart Name ", $seqName[$i][$j]);
		#debug(2, "\tPart Type ", $partType[$i][$j]);
		#debug(2, "\t    seq=", $seq[$i][$j]);
	}
	debug(2, "  }");
}
}



# Recursively insert a node into the tree
#
sub insert_node {
my($tree, $pnum) = @_;

# See if the node is in the tree already and return
if (find_node($root, $xpartName[$pnum])) {
	return;
}

#debug 1, "Node ", $xpartName[$pnum], " #", $pnum, " type=", $partType[$pnum], " not in the tree";

# Make sure each input node is in the tree before inserting current node
for (0 .. $partInCnt[$pnum] - 1) {
	if ($partType[$pnum] eq "INPUT") {
		debug 1, "Input Node";
	} elsif ($partInput[$pnum][$_] =~ m/adc_(\d+)_(\d+)/) {
		debug 1, "\tadc input node";
		debug 1, "\t\t $partInput[$pnum][$_]\t input_type=$partInputType[$pnum][$_]\t$partInNum[$pnum][$_]";
		# create a node for this adc input and then create a link to it from
		# the corresponding second level node
	} else {
		#debug 1, "\t Recursively insert node $partInput[$pnum][$_]\t input_type=$partInputType[$pnum][$_]\t$partInNum[$pnum][$_]";
		insert_node($tree, $partInNum[$pnum][$_]);
	}
}

# At this point all input nodes are alredy resolved
# Insert this node then.
# Find in which subsystem this node is
#
my $subsys = -1;

foreach $i (0 .. $subSys-1) {
	if ($subSysPartStart[$i] <= $pnum && $pnum <  $subSysPartStop[$i]) {
		$subsys = $i;
		break;
	}
}

$my_node = {
	NAME => $xpartName[$pnum],
	TYPE => $partType[$pnum],
	NEXT => [],
	PART => $pnum,
	SUBSYS => $subsys,
};

# Find each input in the tree and update its NEXT field with the pointer to the node

for (0 .. $partInCnt[$pnum] - 1) {
	my $node;
	if (($node = find_node($tree, $partInput[$pnum][$_]))) {
		push @{$node->{NEXT}}, $my_node;
	} else {
		die "Node not found: $partInput[$pnum][$_]\n";
	}
}
}

if ($cpus > 2) {

# Insert all parts into the tree
#
foreach $i (0 .. $subSys-1) {
	for ($subSysPartStart[$i] .. $subSysPartStop[$i]-1) {
	  if ($partType[$_] eq "INPUT") {
		next;
	  }
	  insert_node($root, $_);
	}
}

# Print all terminator nodes
#do_on_leaves($root, sub {if ($_->{PRINTED} != 1) {print $_->{NAME}, "\n"; $_->{PRINTED} = 1; }});

# Print the tree
$print_no_repeats = 0;
sub print_tree {
	my($tree, $level) = @_;
	my($space);
	if ($print_no_repeats && $tree->{PRINTED}) { return; }
	for (0 .. $level) { $space .= ' '; }
	debug 0, $space, "{", $tree->{NAME}, " nref=", scalar @{$tree->{NEXT}}," subsys=", $tree->{SUBSYS}," type=", $tree->{TYPE}, "}";
	for (@{$tree->{NEXT}}) {
		print_tree($_, $level + 1);
	}
	if ($print_no_repeats) { $tree->{PRINTED} = 1; }
}

#debug(0, "Tree:\n");
#print_tree($root);
#debug(0, "\n");


# Tag each node with the subsystem number
sub tag_node {
	my ($node, $subsys) = @_;

	# see if this node is already tagged
	#
	for (@{$node->{INPUTS}}) {
		if ($_ == $subsys) {
			return; # already tagged
		}
	}

	push @{$node->{INPUTS}}, $subsys;
	#print "Tagged ", $node->{NAME}, " with $subsys\n";
}

for (@{$root->{NEXT}}) {
	if ($_->{NAME} =~ m/.*INPUTS/) {
		my $subsys = $_->{SUBSYS};
		#print "Marking ", $_->{NAME}, " with $subsys\n";
		do_on_nodes($_, \&tag_node, $subsys);
	}
}

#do_on_nodes($root, sub {if ($_->{PRINTED} != 1) {print $_->{NAME}, " ", @{$_->{INPUTS}}, "\n"; $_->{PRINTED} = 1; }});

# See how many different groups we have got
%prog_groups;
%prog_group_fmods; # How many filter modules in a group

sub find_prog_groups {
	my ($node) = @_;
	my $nm;
	if ($node->{VISITED}) { return; }
	$node->{VISITED} = 1;
	for (sort @{$node->{INPUTS}}) {
		if (length $nm) { $nm .= "_"; }
		$nm .= $_;
	}
	$prog_groups{$nm}++;
	if ($node->{TYPE} eq "Filt") {
		$prog_group_fmods{$nm}++;
	}
	$node->{GROUP} = $nm;
}

sub reset_visited { 
	my ($node) = @_;
	$node->{VISITED} = 0;
}


do_on_nodes($root,\&reset_visited);
do_on_nodes($root,\&find_prog_groups);


sub processing_order {
	my $idx = index $a, $b;
	#print "processing_order $a $b\n";
	if ($a eq $b) { return 0; }
	if ($idx >= 0) { return 1; }
	$idx = index $b, $a;
	if ($idx >= 0) { return -1; }
	# independent, go by length
	return length $a <=> length $b;
}

print "\n";
for (sort processing_order keys %prog_groups) {
	#print "cluster=$_ -> parts=", $prog_groups{$_}, " fmods=", $prog_group_fmods{$_}, "\n";
}


# Construct cluster tree
$root1 = {
	NAME => $tree_root_name,
	NEXT => [], # array of references to leaves
};

sub get_nums_array {
	my ($str) = @_;
	return @ret = $str =~ m/(\d+)/g;
}

sub add_refs {
	my ($node, $ref_node) = @_;
	if ($node->{VISITED}) { return; }
	if ($ref_node->{NAME} eq $node->{NAME}) { return; }
	#@ref_node_nums = $ref_node->{NAME} =~ m/(\d+)/g;
	#@node_nums =  $node->{NAME} =~ m/(\d+)/g;
	@ref_node_nums =  get_nums_array($ref_node->{NAME});
	@node_nums =  get_nums_array($node->{NAME});
	#print "ref node '", $ref_node->{NAME} ,"' nums: ", @ref_node_nums, "\n";
	#print "node '", $node->{NAME}, "' nums: ", @node_nums, "\n";
	$pass = 1;
	for (@node_nums) {
		$in_it = 0;
		$cur_num = $_;
		for (@ref_node_nums) {
			if ($cur_num == $_) {
				$in_it = 1;
				break;
			}
		}
		if (!$in_it) {
			$pass = 0;
			break;
		}
	}
	if ($pass) {
		push @{$node->{NEXT}}, $ref_node;
		push @{$ref_node->{PARENTS}}, $node;
	}
	$node->{VISITED} = 1;
}

for (sort processing_order keys %prog_groups) {
	if (length($_) == 0) { next; }
	$node = {
		NAME => $_,
		NEXT => [],
		PARTS => $prog_groups{$_},
		FILTERS => $prog_group_fmods{$_},
	};
	# initial node
	if (index($_, "_") == -1) {
		#print "Initial node $_\n";
		push @{$root1->{NEXT}}, $node;
		push @{$node->{PARENTS}}, $root1;
	} else {
		do_on_nodes($root1,\&reset_visited);
		$root1->{VISITED} = 1;
		do_on_nodes($root1, \&add_refs, $node);
	}
}


$exit_node_name = "__exit__";
$exit_node = {
	NAME => $exit_node_name,
	NEXT => [], # array of references to leaves
};

# Insert exit node
for (sort processing_order keys %prog_groups) {
	if (length($_) == 0) { next; }
	$node = find_node($root1, $_);
	die "Node $_ not found\n" unless $node != undef;
	if (scalar @{$node->{NEXT}} == 0) {
		push @{$node->{NEXT}}, $exit_node;
		push @{$exit_node->{PARENTS}}, $node;
	}
}

# Calculate t-levels for each node
for ($tree_root_name, sort processing_order keys %prog_groups, $exit_node_name) {
	if (length($_) == 0) { next; }
	$node = find_node($root1, $_);
	die "Node $_ not found\n" unless $node != undef;
	#print "Node $_ has parents: ";
	$max_t_level = 0;
	for (@{$node->{PARENTS}}) {
		#print $_->{NAME}, " ";
		$pt = $_->{T_LEVEL} + $_->{FILTERS};
		if ($pt > $max_t_level) {
			$max_t_level = $pt;
		}
	}
	$node->{T_LEVEL} = $max_t_level;
	#print "t_level=", $node->{T_LEVEL}, "\n";
}

# Calculate b-levels for each node
for ($exit_node_name, reverse(sort processing_order keys %prog_groups), $tree_root_name) {
	if (length($_) == 0) { next; }
	$node = find_node($root1, $_);
	die "Node $_ not found\n" unless $node != undef;
	$max_b_level = 0;
	for (@{$node->{NEXT}}) {
		if ($_->{B_LEVEL} > $max_b_level) {
			$max_b_level = $_->{B_LEVEL};
		}
	}
	$node->{B_LEVEL} = $max_b_level + $node->{FILTERS};
	#print "Node ", $node->{NAME} ," b_level=", $node->{B_LEVEL}, "\n";
}

sub b_level_order {
	if (length($a) == 0) { return 0; }
	if (length($b) == 0) { return 0; }
	$anode = find_node($root1, $a);
	$bnode = find_node($root1, $b);
	die "Node $_ not found\n" unless $anode != undef && $bnode != undef;
	#print $anode->{B_LEVEL}, " ", $bnode->{B_LEVEL}, "\n";
	return $bnode->{B_LEVEL} <=> $anode->{B_LEVEL};
}

print "-----------------------\n";
print "CPU allocation schedule\n";
print "---------------------------------------------------\n";
print "Node\tB-level\tcpu\tlevel\tweight\tlevel time\n";
print "---------------------------------------------------";
# Assign clusters to CPUs (b-level based list algorithm)
$level = 0;
$cpu = 1; # goes from 1 to $cpus
$max_level_time = 0;
$total_time = 0;
$cum_time = 0;
@sorted_clusters = sort b_level_order keys %prog_groups;
@running_clusters = (); # currently running clusters (on CPUs)
@saved_nodes = ();
while(scalar @sorted_clusters) {
	$_ = shift @sorted_clusters;
	if (length($_) == 0) { next; }
	$node = find_node($root1, $_);
	die "\nNode $_ not found\n" unless $node != undef;
	# See if just popped cluster can be run on this processing level
	$repeat = 0;
	for (@running_clusters) {
		if (find_node(find_node($root1, $_), $node->{NAME}) != undef) {
			# Current node depends on this running cluster...
			unshift @saved_nodes, $node->{NAME};
			$repeat = 1;
			break;
		}
	}
	if ($repeat) {
		next;
	} else {
		for (@saved_nodes) {
			unshift @sorted_clusters, $_;
		}
		@saved_nodes = ();
	}
	push @{$cpu_clusters[$cpu]}, $_;
	$node->{CPU} = $cpu;
	$node->{LEVEL} = $level;
	push @running_clusters, $_;
	print "\n", $node->{NAME} ,"\t", $node->{B_LEVEL}, "\t", $cpu, "\t", $level, "\t", $node->{FILTERS};
	$cpu++;
	$cum_time += $node->{FILTERS};
	if ($node->{FILTERS} > $max_level_time) {
		$max_level_time = $node->{FILTERS};
	}
	if ($cpu == $cpus) {
		$cpu = 1;
		print "\t", $max_level_time;
		@running_clusters = ();
		$total_time += $max_level_time,;
		$max_level_time = 0;
		$level++;
	}
}

if ($cpu != 1) {
	print "\t", $max_level_time;
	$total_time += $max_level_time,;
}
print "\n---------------------------------------------------\n";
print "Total time is ", $total_time, "\n";
print "Cumulative time is ", $cum_time, "(", $cum_time/($cpus - 1), " per cpu)\n";
print "TODO: need to eliminate unnecessary CPU sync points (merge levels)\n";
#$print_no_repeats = 1;
#print_tree($root1);

# Check all nodes are scheduled
for (sort processing_order keys %prog_groups) {
	if (length($_) == 0) { next; }
	$node = find_node($root1, $_);
	die "Node $_ not found\n" unless $node != undef;
	die "Node $_ was not scheduled for execution\n" unless $node->{CPU};
}

} # If $cpus > 2

# First pass defines processing step 0
# It finds all input sybsystems

for($ii=0;$ii<$subSys;$ii++)
{
$subUsed[$ii] = 0;
$allADC = 1;
	for($jj=0;$jj<$subCntr[$ii];$jj++)
	{
		if($subInputsType[$ii][$jj] ne "Adc")
		{
			$allADC = 0;
		}
	}
	if($allADC == 1) 
	{
		$subUsed[$ii] = 1;
		$seqList[$seqCnt] = $ii;
		$seqType[$seqCnt] = "SUBSYS";
		$seqCnt ++;
		 print "Subsys $ii $subSysName[$ii] has all ADC inputs and can go $seqCnt\n";
		$subRemaining --;
		if ($cur_step_cpus == 1) { $cur_step_cpus = $cpus; }
		$sys_cpu_step{$subSysName[$ii]} = --$cur_step_cpus;
	}
}
#print "Searching parts $searchCnt\n";
for($ii=0;$ii<$searchCnt;$ii++)
{
	$allADC = 1;
	$xx = $searchPart[$ii];
	if($partUsed[$xx] == 0)
	{
		for($jj=0;$jj<$partInCnt[$xx];$jj++)
		{
			if(($partInputType[$xx][$jj] ne "Adc") && ($partInputType[$xx][$jj] ne "DELAY"))

			{
					$allADC = 0;
			}
		}
		if($allADC == 1) {
			#print "Part $xx $xpartName[$xx] can go next\n";
			$partUsed[$xx] = 1;
			$partsRemaining --;
			$seqList[$seqCnt] = $xx;
			$seqType[$seqCnt] = "PART";
			$seqCnt ++;
			if ($cur_step_cpus == 1) { $cur_step_cpus = $cpus; }
			$sys_cpu_step{$xpartName[$xx]} = --$cur_step_cpus;
		}
	}
}
print "first pass done $partsRemaining $subRemaining\n";

# Second multiprocessing step
$cur_step = 1;
$cur_step_cpus = $cpus;
$numTries = 0;
until((($partsRemaining < 1) && ($subRemaining < 1)) || ($numTries > 50))
{
$numTries ++;
	for($ii=0;$ii<$subSys;$ii++)
	{
		$allADC = 1;
		if($subUsed[$ii] == 0)
		{
			for($jj=0;$jj<$subCntr[$ii];$jj++)
			{
				$yy = $subInputs[$ii][$jj];
				if(($yy<100) && ($subUsed[$yy] != 1))
				{
					$allADC = 0;
				}
				if($yy > 99)
				{
				$yy = $subInputs[$ii][$jj] - 100;
				for($kk=0;$kk<$searchCnt;$kk++)
				{
					if(($partUsed[$yy] != 1) && ($subInputsType[$ii][$jj] ne "Adc") && ($partType[$yy] ne "DELAY"))
					{
						$allADC = 0;
					}
				}
				}
			}
			if($allADC == 1) {
				# print "Subsys $ii $subSysName[$ii] can go next\n";
				$subUsed[$ii] = 1;
				$subRemaining --;
				$seqList[$seqCnt] = $ii;
				$seqType[$seqCnt] = "SUBSYS";
				$seqCnt ++;
				if ($cur_step_cpus == 1) { $cur_step_cpus = $cpus; }
				$sys_cpu_step{$subSysName[$ii]} = --$cur_step_cpus + 10 * $cur_step;
			}
		}
	}
	for($ii=0;$ii<$searchCnt;$ii++)
	{
		$allADC = 1;
		$xx = $searchPart[$ii];
		if($partUsed[$xx] == 0)
		{
			for($jj=0;$jj<$partInCnt[$xx];$jj++)
			{
				$yy = $partSysFromx[$xx][$jj];
				if($yy < 0)
				{
					
					$zz = $partInNum[$xx][$jj];
					if((!$partUsed[$zz]) && ($partInputType[$xx][$jj] ne "DELAY"))
					{
						$allADC = 0;
					}
				}
				else {
				if(($subUsed[$yy] != 1) && ($partInputType[$xx][$jj] ne "Adc") && ($partInputType[$xx][$jj] ne "DELAY"))
				{
						$allADC = 0;
				}
				}
			}
			if($allADC == 1) {
				print "Part $xx $xpartName[$xx] can go next\n";
				$partUsed[$xx] = 1;
				$partsRemaining --;
				$seqList[$seqCnt] = $xx;
				$seqType[$seqCnt] = "PART";
				$seqCnt ++;
				if ($cur_step_cpus == 1) { $cur_step_cpus = $cpus; }
				$sys_cpu_step{$xpartName[$xx]} = --$cur_step_cpus + 10* $cur_step;
			}
		}
	}
}
if(($partsRemaining > 0) || ($subRemaining > 0)) {
        print "Linkage failed (parts remaining $partsRemaining; subs remaining $subRemaining)\n";
# FIXME: the following code doen't report correctly failed parts
	for($ii=0;$ii<$subSys;$ii++)
	{
		if($subUsed[$ii] == 0)
		{
		print "Subsys $ii $subSysName[$ii] failed to connect\n";
		}
	}
	for($ii=0;$ii<$partCnt;$ii++)
	{
		if($partUsed[$ii] == 0)
		{
		print "Part $ii $xpartName[$ii] failed to connect\n";
		}
	}
	 exit(1);
}
$processCnt = 0;
$processSeqCnt = 0;
for($ii=0;$ii<$seqCnt;$ii++)
{
	#print "$ii $seqList[$ii] $seqType[$ii] $seqParts[$seqList[$ii]]\n";
	$processSeqStart[$processSeqCnt] = $processCnt;
	$processSeqCnt ++;
	if($seqType[$ii] eq "SUBSYS")
	{
		$xx = $seqList[$ii];
		# Save the name for subsystem for later use in code gennerator
		$processSeqSubsysName[$processCnt] = $subSysName[$xx];
		$processSeqType{$subSysName[$xx]} = $seqType[$ii];
		for($jj=0;$jj<$seqParts[$xx];$jj++)
		{
			$processName[$processCnt] = $seqName[$xx][$jj];
			$processPartNum[$processCnt] = $seq[$xx][$jj];
			# print "SUBSYS $processCnt $processName[$processCnt] $processPartNum[$processCnt]\n";
			$processCnt ++;
		}
	}
	if($seqType[$ii] eq "PART")
	{
		$xx = $seqList[$ii];
		$processName[$processCnt] = $xpartName[$xx];
if(($partType[$xx] eq "TERM") || ($partType[$xx] eq "GROUND") || ($partType[$xx] eq "FROM") || ($partType[$xx] eq "GOTO") || ($partType[$xx] eq "EpicsIn") || ($partType[$xx] eq "CONSTANT")) 
{$ftotal ++;}
		$processSeqType{$xpartName[$xx]} = $seqType[$ii];
		$processPartNum[$processCnt] = $xx;
		#print "******* $processCnt $processName[$processCnt] $processPartNum[$processCnt]\n";
		$processSeqSubsysName[$processCnt] = "__PART__";
		$processCnt ++;
	}
	$processSeqEnd[$processSeqCnt] = $processCnt;
}
print "Counted $processCnt parts out of total $ftotal\n";
if($processCnt != $ftotal)
{
	print "Fatal error - not all parts are in processing list!\n";
	%seen = ();
	@missing = ();
	@seen{@processName} = ();
	foreach $item (@xpartName) {
		push (@missing, $item) unless exists $seen{$item};
	}
	print "List of parts not counted:\n";
	foreach  (@missing) {
		my $pt = $partType[$CDS::Parser::parts{$_}];
		if ($pt ne "INPUT" && $pt ne "OUTPUT" && $pt ne "GROUND" 
			&& $pt ne "TERM" && $pt ne "BUSS" && $pt ne "BUSC"
			&& $pt ne "EpicsIn") {
			print $_, " ", $partType[$CDS::Parser::parts{$_}], "\n";
		}
	}
	print "Please check the model for missing links around these parts.\n";
	exit(1);
}

#print "CPU_STEPS:\n";
#while (($k, $v) = each %sys_cpu_step) {
#	print "$k => $v\n";
#}

$fpartCnt = 0;
$inCnt = 0;

# Write Epics structs common to all CDS front ends to the .h file.
print OUTH "#define MAX_FIR \t $firCnt\n";
print OUTH "#define MAX_FIR_POLY \t $firCnt\n\n";
# ########    TEST    ############
$svnVer = `svnversion`;
print "\nVersion = $svnVer\n";
$size = length($svnVer);
print "\nLength = $size\n";
$svnVerSub = substr($svnVer, 0, ($size - 1));
print OUTH "#define BUILD_SVN_VERSION_NO \t \"$svnVerSub\"\n\n";
# ########    TEST    ############
print OUTH "typedef struct CDS_EPICS_IN {\n";
print OUTH "\tfloat vmeReset;\n";
print OUTH "\tint burtRestore;\n";
print OUTH "\tint dcuId;\n";
print OUTH "\tint diagReset;\n";
print OUTH "\tint syncReset;\n";
print OUTH "\tint overflowReset;\n";
print OUTH "} CDS_EPICS_IN;\n\n";
print OUTH "typedef struct CDS_EPICS_OUT {\n";
print OUTH "\tint onePps;\n";
print OUTH "\tint timeErr;\n";
print OUTH "\tint adcWaitTime;\n";
print OUTH "\tint diagWord;\n";
print OUTH "\tint timeDiag;\n";
print OUTH "\tint cpuMeter;\n";
print OUTH "\tint cpuMeterMax;\n";
print OUTH "\tint gdsMon[32];\n";
print OUTH "\tint diags[10];\n";
print OUTH "\tint overflowAdc[8][32];\n";
print OUTH "\tint overflowDac[8][16];\n";
print OUTH "\tint dacValue[8][16];\n";
print OUTH "\tint ovAccum;\n";
print OUTH "\tint statAdc[16];\n";
print OUTH "\tint statDac[16];\n";
print OUTH "} CDS_EPICS_OUT;\n\n";
if($useWd)
{
print OUTH "typedef struct SEI_WATCHDOG {\n";
print OUTH "\tint trip;\n";
print OUTH "\tint reset;\n";
print OUTH "\tint status\[3\];\n";
print OUTH "\tint senCount[20];\n";
print OUTH "\tint senCountHold[20];\n";
print OUTH "\tint filtMax[20];\n";
print OUTH "\tint filtMaxHold[20];\n";
print OUTH "\tint tripSetF[5];\n";
print OUTH "\tint tripSetR[5];\n";
print OUTH "\tint tripState;\n";
print OUTH "} SEI_WATCHDOG;\n\n";
}
print OUTH "typedef struct \U$systemName {\n";
print EPICS "\nEPICS CDS_EPICS dspSpace coeffSpace epicsSpace\n\n";
for($ii=0;$ii<$partCnt;$ii++)
{
	if ($cdsPart[$ii]) {
	  ("CDS::" . $partType[$ii] . "::printHeaderStruct") -> ($ii);
	}
}

die "Unspecified \"host\" parameter in cdsParameters block\n" if ($targetHost eq "localhost");

print EPICS "\n\n";
print OUTH "} \U$systemName;\n\n";

print OUTH "\n\n#define MAX_MODULES \t $filtCnt\n";
$filtCnt *= 10;
print OUTH "#define MAX_FILTERS \t $filtCnt\n\n";

my $gdsNode = $gdsNodeId - 1;
print EPICS "MOMENTARY FEC\_$dcuId\_VME_RESET epicsInput.vmeReset int ai 0\n";
print EPICS "MOMENTARY FEC\_$dcuId\_DIAG_RESET epicsInput.diagReset int ai 0\n";
print EPICS "MOMENTARY FEC\_$dcuId\_SYNC_RESET epicsInput.syncReset int ai 0\n";
print EPICS "MOMENTARY FEC\_$dcuId\_OVERFLOW_RESET epicsInput.overflowReset int ai 0\n";
print EPICS "DAQVAR $dcuId\_LOAD_CONFIG int ai 0\n";
print EPICS "DAQVAR $dcuId\_CHAN_CNT int ai 0\n";
print EPICS "DAQVAR $dcuId\_TOTAL int ai 0\n";
print EPICS "DAQVAR $dcuId\_MSG int ai 0\n";
print EPICS "DAQVAR  $dcuId\_DCU_ID int ai 0\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_CPU_METER epicsOutput.cpuMeter int ai 0 field(HOPR,\"$rate\") field(LOPR,\"0\")\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_CPU_METER_MAX epicsOutput.cpuMeterMax int ai 0 field(HOPR,\"$rate\") field(LOPR,\"0\")\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_ADC_WAIT epicsOutput.adcWaitTime int ai 0 field(HOPR,\"$rate\") field(LOPR,\"0\")\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_ONE_PPS epicsOutput.onePps int ai 0\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_TIME_ERR epicsOutput.timeErr int ai 0\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_TIME_DIAG epicsOutput.timeDiag int ai 0\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_DIAG_WORD epicsOutput.diagWord int ai 0\n";
print EPICS "INVARIABLE FEC\_$dcuId\_BURT_RESTORE epicsInput.burtRestore int ai 0\n";
for($ii=0;$ii<32;$ii++)
{
	print EPICS "OUTVARIABLE FEC\_$dcuId\_GDS_MON_$ii epicsOutput.gdsMon\[$ii\] int ai 0\n";
}
print EPICS "OUTVARIABLE FEC\_$dcuId\_USR_TIME epicsOutput.diags[0] int ai 0\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_RESYNC_COUNT epicsOutput.diags[1] int ai 0\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_FB_NET_STATUS epicsOutput.diags[2] int ai 0\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_DAQ_BYTE_COUNT epicsOutput.diags[3] int ai 0\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_DUOTONE_TIME epicsOutput.diags[4] int ai 0\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_IRIGB_TIME epicsOutput.diags[5] int ai 0\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_ADC_STAT epicsOutput.diags[6] int ai 0\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_DAC_STAT epicsOutput.diags[7] int ai 0\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_DAC_MASTER_STAT epicsOutput.diags[8] int ai 0\n";
print EPICS "\n\n";
#Load EPICS I/O Parts
for($ii=0;$ii<$partCnt;$ii++)
{
	if ($cdsPart[$ii]) {
	  ("CDS::" . $partType[$ii] . "::printEpics") -> ($ii);
	}
}
print EPICS "\n\n";
for($ii=0;$ii<$adcCnt;$ii++)
{
	print EPICS "OUTVARIABLE FEC\_$dcuId\_ADC_STAT_$ii epicsOutput.statAdc\[$ii\] int ai 0\n";
	for($jj=0;$jj<32;$jj++)
	{
		print EPICS "OUTVARIABLE FEC\_$dcuId\_ADC_OVERFLOW_$ii\_$jj epicsOutput.overflowAdc\[$ii\]\[$jj\] int ai 0\n";
	}
}
print EPICS "\n\n";
for($ii=0;$ii<$dacCnt;$ii++)
{
	print EPICS "OUTVARIABLE FEC\_$dcuId\_DAC_STAT_$ii epicsOutput.statDac\[$ii\] int ai 0\n";
	for($jj=0;$jj<16;$jj++)
	{
		print EPICS "OUTVARIABLE FEC\_$dcuId\_DAC_OVERFLOW_$ii\_$jj epicsOutput.overflowDac\[$ii\]\[$jj\] int ai 0\n";
		print EPICS "OUTVARIABLE FEC\_$dcuId\_DAC_OUTPUT_$ii\_$jj epicsOutput.dacValue\[$ii\]\[$jj\] int ai 0\n";
	}
}
print EPICS "OUTVARIABLE FEC\_$dcuId\_ACCUM_OVERFLOW epicsOutput.ovAccum int ai 0\n";
print EPICS "\n\n";
print EPICS "systems \U$systemName\-\n";
if ($plantName ne $systemName) {
	print EPICS "plant \U$plantName\n";
}
#$gdsXstart = ($dcuId - 5) * 1250;
#$gdsTstart = $gdsXstart + 10000;
if($rate == 480 || $rate == 240) {
  $gdsXstart = 20001;
  $gdsTstart = 30001;
} else {
  $gdsXstart = 1;
  $gdsTstart = 10001;
}

$dac_testpoint_names = "";
for($ii = 0; $ii < $dacCnt; $ii++) {
   for($jj = 0; $jj < 16; $jj++) {
	$dac_testpoint_names .= "MDAC". $ii . "_TP_CH" . $jj . " ";
   }
}

print EPICS "test_points ONE_PPS $dac_testpoint_names $::extraTestPoints\n";
if ($::extraExcitations) {
	print EPICS "excitations $::extraExcitations\n";
}
print EPICS "gds_config $gdsXstart $gdsTstart 1250 1250 $gdsNodeId $site " . get_freq() . " $dcuId $ifoid\n";
print EPICS "\n\n";

# Start process of writing .c file.
print OUT "// ******* This is a computer generated file *******\n";
print OUT "// ******* DO NOT HAND EDIT ************************\n";
print OUT "\n\n";
print OUT "\#ifdef SERVO64K\n";
print OUT "\t\#define FE_RATE\t65536\n";
print OUT "\#endif\n";
print OUT "\#ifdef SERVO32K\n";
print OUT "\t\#define FE_RATE\t32768\n";
print OUT "\#endif\n";
print OUT "\#ifdef SERVO16K\n";
print OUT "\t\#define FE_RATE\t16384\n";
print OUT "\#endif\n";
print OUT "\#ifdef SERVO4K\n";
print OUT "\t\#define FE_RATE\t4096\n";
print OUT "\#endif\n";
print OUT "\#ifdef SERVO2K\n";
print OUT "\t\#define FE_RATE\t2048\n";
print OUT "\#endif\n";
print OUT "\n\n";

# Hardware configuration
print OUT "/* Hardware configuration */\n";
print OUT "CDS_CARDS cards_used[] = {\n";
for (0 .. $adcCnt-1) {
	print OUT "\t{", $adcType[$_], ",", $adcNum[$_], "},\n";
}
for (0 .. $dacCnt-1) {
	print OUT "\t{", $dacType[$_], ",", $dacNum[$_], "},\n";
}
for (0 .. $boCnt-1) {
	print OUT "\t{", $boType[$_], ",", $boNum[$_], "},\n";
}
print OUT "};\n\n";

# Group includes for function calls at beginning
for ($ii = 0; $ii < $partCnt; $ii++) {
   if ($cdsPart[$ii]) {
      if ($partType[$ii] eq "FunctionCall") {
         ("CDS::" . $partType[$ii] . "::printFrontEndVars") -> ($ii);
      }
   }
}

# Define remote IPC stuff (if any)
if ($remoteIPChosts) {
	print OUT "// Remote IPC buffers\n";
	print OUT "double remote_ipc_send[$maxRemoteIPCHosts][$maxRemoteIPCVars];\n";
	print OUT "double remote_ipc_rcv[$maxRemoteIPCHosts][$maxRemoteIPCVars];\n\n";

	print OUT "// The number of remote IPC nodes\n";
	my $nodes = @remoteIPCnodes;
	print OUT "unsigned int cds_remote_ipc_nodes = $nodes;\n";
	print OUT "// The size of remote IPC data buffer\n";
	print OUT "unsigned int cds_remote_ipc_size = $maxRemoteIPCVars;\n\n";
	print OUT "// Remote IPC nodes\n";
	print OUT "CDS_REMOTE_NODES remote_nodes[] = {\n";
	foreach (@remoteIPCnodes) {
		@f = /(\w+):(\d+)/g;
		print OUT "\t{\"$f[0]\", $f[1]},\n";
	}
	print OUT "};\n\n";
	print OUT "// My remote IPC MX port\n";
	print OUT "unsigned int remote_ipc_mx_port = $remoteIPCport;\n";
	print OUT "\n";
}

sub printVariables {
for($ii=0;$ii<$partCnt;$ii++)
{
#       print "DBG: cdsPart = $cdsPart[$ii]   partType = $partType[$ii]\n";          # DBG
	if ($cdsPart[$ii]) {
           if ($partType[$ii] ne "FunctionCall") {
              ("CDS::" . $partType[$ii] . "::printFrontEndVars") -> ($ii);
           }
	}

	if($partType[$ii] eq "MUX") {
		$port = $partInCnt[$ii];
		print OUT "double \L$xpartName[$ii]\[$port\];\n";
	}
	if($partType[$ii] eq "DEMUX") {
		$port = $partOutputs[$ii];
		print OUT "double \L$xpartName[$ii]\[$port\];\n";
	}
	if($partType[$ii] eq "SUM"
	   || $partType[$ii] eq "Switch"
	   || $partType[$ii] eq "Gain"
	   || $partType[$ii] eq "Abs"
	   || $partType[$ii] eq "RelationalOperator"
	   || $partType[$ii] eq "SATURATE") {
		$port = $partInCnt[$ii];
		print OUT "double \L$xpartName[$ii];\n";
	}
	if($partType[$ii] eq "MULTIPLY") {
		$port = $partInCnt[$ii];
		print OUT "double \L$xpartName[$ii];\n";
	}
	if($partType[$ii] eq "DIVIDE") {
		$port = $partInCnt[$ii];
		print OUT "double \L$xpartName[$ii];\n";
	}
	if($partType[$ii] eq "M_SQR") {                                    # ===  MA  ===
		$port = $partInCnt[$ii];                                   # ===  MA  ===
		print OUT "double \L$xpartName[$ii];\n";                   # ===  MA  ===
	}                                                                  # ===  MA  ===
	if($partType[$ii] eq "M_SQT") {                                    # ===  MA  ===
		$port = $partInCnt[$ii];                                   # ===  MA  ===
		print OUT "double \L$xpartName[$ii];\n";                   # ===  MA  ===
	}                                                                  # ===  MA  ===
	if($partType[$ii] eq "M_REC") {                                    # ===  MA  ===
		$port = $partInCnt[$ii];                                   # ===  MA  ===
		print OUT "double \L$xpartName[$ii];\n";                   # ===  MA  ===
	}                                                                  # ===  MA  ===
	if($partType[$ii] eq "M_MOD") {                                    # ===  MA  ===
		$port = $partInCnt[$ii];                                   # ===  MA  ===
		print OUT "double \L$xpartName[$ii];\n";                   # ===  MA  ===
	}                                                                  # ===  MA  ===
	if($partType[$ii] eq "DELAY") {
		print OUT "static double \L$xpartName[$ii];\n";
	}
	if($partType[$ii] eq "GROUND")  {
            if ($groundDecl == 0)  {                                       # =+=  MA  =+=
                print OUT "static float ground;\n";                        # =+=  MA  =+=
                $groundDecl++;                                             # =+=  MA  =+=
            }                                                              # =+=  MA  =+=
	}
	if($partType[$ii] eq "CONSTANT")  {
		print OUT "static double \L$xpartName[$ii];\n";
	}
	if($partType[$ii] eq "AND") {
		$port = $partInCnt[$ii];
		print OUT "int \L$xpartName[$ii];\n";
	}
}
print OUT "\n\n";
}

if ($cpus > 2) {

# Output multiprocessing functions
print OUT "/* Multi-cpu synchronization primitives */\n";
for ($i = 2; $i < $cpus; $i++) {
	print OUT "volatile int go$i = 0;\n";
	print OUT "volatile int done$i = 0;\n";
}
print OUT "\n";


# Print common to all functions variables as globals
printVariables();

for ($i = 2; $i < $cpus; $i++) {
    print OUT "/* CPU $i code */\n";
    print OUT "void cpu${i}_start(){\n";
    print OUT "\tint ii, jj;\n";
    print OUT "\twhile(!stop_working_threads) {\n";

if ($old_style_multiprocessing) {
    # Processing steps
    for ($step = 0; $step <= $cur_step; $step++) {
      # Wait for signal from main thread
      print OUT "\t\twhile(!go$i && !stop_working_threads);\n";
      print OUT "\t\tgo$i = 0;\n";

      # Processing for this step
      while (($k, $v) = each %sys_cpu_step) {
	#printf "feCode: $k => $v  %d %d\n", $v % 10, $v / 10;
	if ($v == $step*10 + $i) {
  	  &printSubsystem("$k");
	}
      }
      print OUT "\t\tdone$i = 1;\n";
    }
} else {
    for (@{$cpu_clusters[$i]}) {
      # Wait for signal from main thread
      print OUT "\t\twhile(!go$i && !stop_working_threads);\n";
      print OUT "\t\tgo$i = 0;\n";
      &printCluster($_);
      print OUT "\t\tdone$i = 1;\n";
    }
}

    print OUT "\t}\n";
    print OUT "\tdone$i = 1;\n";
    print OUT "}\n\n";
}

sub printCluster {
	my ($cluster) = @_;
	print OUT "/* Do $cluster cluster */\n";
        &printSubsystem(".", $cluster);
}

print OUT "/* CPU 1 code */\n";
}

print OUT "\nvoid feCode(int cycle, double dWord[][32],\t\/* ADC inputs *\/\n";
print OUT "\t\tdouble dacOut[][16],\t\/* DAC outputs *\/\n";
print OUT "\t\tFILT_MOD *dsp_ptr,\t\/* Filter Mod variables *\/\n";
print OUT "\t\tCOEF *dspCoeff,\t\t\/* Filter Mod coeffs *\/\n";
print OUT "\t\tCDS_EPICS *pLocalEpics,\t\/* EPICS variables *\/\n";
print OUT "\t\tint feInit)\t\/* Initialization flag *\/\n";
print OUT "{\n\nint ii;\n\n";
if ($cpus < 3) {
  printVariables();
}
print OUT "if(feInit)\n\{\n";

# removed for ADC PART CHANGE
#for($ii=0;$ii<$adcCnt;$ii++) {
#   print OUT ("CDS::Adc::frontEndInitCode") -> ($ii);
#}

for($ii=0;$ii<$partCnt;$ii++)
{
	if ( -e "lib/$partType[$ii].pm" ) {
	   print OUT ("CDS::" . $partType[$ii] . "::frontEndInitCode") -> ($ii);
	}	

	if($partType[$ii] eq "GROUND") {
            if ($groundInit == 0)  {                                       # =+=  MA  =+=
                print OUT "ground = 0\.0;\n";                              # =+=  MA  =+=
                $groundInit++;                                             # =+=  MA  =+=
            }                                                              # =+=  MA  =+=
	}
	if($partType[$ii] eq "CONSTANT") {
		print OUT "\L$xpartName[$ii] = (double)$partInputs[$ii];\n";
	}
}
print OUT "\} else \{\n";

#
# All IPCx data receives are to occur
# first in the processing loop
#
if ($ipcxCnt > 0) {
   print OUT "\ncommData2Receive(myIpcCount, ipcInfo, pLocalEpics->epicsOutput\.timeDiag, cycle);\n\n";
}

#print "*****************************************************\n";

# Print subsystems processing step by step
# along with signalling primitives to other threads
if ($cpus > 2) {
if ($old_style_multiprocessing) {
  for ($step = 0; $step <= $cur_step; $step++) {
    # Signal other threads to go
    for ($i = 2; $i < $cpus; $i++) {
	print OUT "go$i = 1;\n";
    }

    # Print cpu 1 subsystems for $step
    while (($k, $v) = each %sys_cpu_step) {
	#printf "feCode: $k => $v  %d %d\n", $v % 10, $v / 10;
	if ($v == $step*10 + 1) {
  	  &printSubsystem("$k");
	}
    }

    # Wait for other threads to finish this processing step
    for ($i = 2; $i < $cpus; $i++) {
	print OUT "while(!done$i && !stop_working_threads);\n";
	print OUT "done$i = 0;\n";
    }
  }
} else {
    for (@{$cpu_clusters[1]}) {
      # Signal other threads to go
      for ($i = 2; $i < $cpus; $i++) {
	  print OUT "go$i = 1;\n";
      }

      &printCluster($_);

      # Wait for other threads to finish this processing step
      for ($i = 2; $i < $cpus; $i++) {
	print OUT "while(!done$i && !stop_working_threads);\n";
	print OUT "done$i = 0;\n";
      }
    }
}

  # Output standalone parts (DAC output)
  &printSubsystem("__PART__");
} else {
  &printSubsystem(".");
}

sub printSubsystem {
my ($subsys, $cluster) = @_;

$do_cluster = 0;

if (scalar(@_) > 1) {
	$do_cluster = 1;
}

$ts = 0;
$xx = 0;
$do_print = 0;
@cluster_parts = ();

if ($do_cluster) {
	# get all cluster part names
	do_on_nodes($root, \&reset_visited);
	do_on_nodes($root, sub{ if (!$_->{VISITED} && $_->{GROUP} eq $cluster) { push @cluster_parts, $_->{NAME}; $_->{VISITED} = 1;}});

	#print ("Cluster $cluster has these parts: ");
	for (@cluster_parts) {
		#print $_, " ";
	}
	#print "\n";
}

for($xx=0;$xx<$processCnt;$xx++) 
{
	if($xx == $processSeqEnd[$ts]) {
		if ($do_print && ($processSeqType{$cur_sys_name} eq "SUBSYS")) {
		    print OUT "\n\/\/End of subsystem   $cur_sys_name **************************************************\n\n\n";
		}
		$do_print = 0;
	}
	if($xx == $processSeqStart[$ts])
	{
		if ($processSeqSubsysName[$xx] =~ /$subsys/) {
		  $do_print = 1; 
		  $cur_sys_name = $processSeqSubsysName[$xx];
		  if ($processSeqType{$cur_sys_name} eq "SUBSYS") {
		    print OUT "\n\/\/Start of subsystem $cur_sys_name **************************************************\n\n";
		  }
		}
		$ts ++;
	}

	if (! $do_print)  { next; };
	$mm = $processPartNum[$xx];
	#print "looking for part $mm type=$partType[$mm] name=$xpartName[$mm]\n";
	if ($do_cluster) {
		my $do_print = 0;
		for (@cluster_parts) {
			if ($xpartName[$mm] eq $_) {
				$do_print = 1;
				break;
			}
		}
		if (! $do_print)  { next; };
	}
	$inCnt = $partInCnt[$mm];
#print "Processing $xpartName[$mm]\n";
	for($qq=0;$qq<$inCnt;$qq++)
	{
		$indone = 0;
		if ( -e "lib/$partInputType[$mm][$qq].pm" ) {
		  require "lib/$partInputType[$mm][$qq].pm";
	  	  $fromExp[$qq] = ("CDS::" . $partInputType[$mm][$qq] . "::fromExp") -> ($mm, $qq);
#print "%%%%%  index-1 = $qq\n";
#print "%%%%%  fromExp = $fromExp[$qq]\n";
	    	  $indone = $fromExp[$qq] ne "";
		}

		if($indone == 0)
		{
			$from = $partInNum[$mm][$qq]; #part number for input $qq
			if ($partInputType[$mm][$qq] eq "DEMUX") {
				$fromExp[$qq] = "\L$xpartName[$from]\[$partInputPort[$mm][$qq]\]";
			} else {
				$fromExp[$qq] = "\L$xpartName[$from]";
			}
#print "%%%%%  index-2 = $qq\n";
#print "%%%%%  fromExp = $fromExp[$qq]\n";
			#print "$xpartName[$mm]  $fromExp[$qq] $partInputType[$mm][$qq]\n";
		}

                if ($fromExp[$qq] =~ /ground/)  {
#                   print "%%%%%  GROUND\n";
                   $fromExp[$qq] = "ground";
                }
	}
	# ******** FILTER *************************************************************************



	#print "@@@  partType = $partType[$mm]\n";
        if ( -e "lib/$partType[$mm].pm" ) {
	  	  print OUT ("CDS::" . $partType[$mm] . "::frontEndCode") -> ($mm);
	}	

	if ($partType[$mm] eq "MUX") {
		print OUT "// MUX\n";
		my $calcExp;
		for (0 .. $inCnt - 1) {
		  $calcExp .= "\L$xpartName[$mm]\[$_\]" . "= $fromExp[$_];\n";
		}
		print OUT $calcExp;
	}
	if ($partType[$mm] eq "DEMUX" && $::partInputType[$mm][0] ne "FunctionCall") {
		print OUT "// DEMUX\n";
		my $calcExp;
		for (0 .. $partOutputs[$mm]  - 1) {
		  $calcExp .= "\L$xpartName[$mm]\[$_\]" . "= $fromExp[0]\[". $_ . "\];\n";
		}
		print OUT $calcExp;
	}
	# Switch
	if ($partType[$mm] eq "Switch") {
	  print OUT "// Switch\n";
	  my $op = $partInputs[$mm];
	  print OUT "\L$xpartName[$mm]". " = ((($fromExp[1]) $op)? ($fromExp[0]): ($fromExp[2]));";
	}
	# Relational Operator
	if ($partType[$mm] eq "RelationalOperator") {
	  my $op = $partInputs[$mm];
	  if ($op eq "~=") { $op = "!="; };
	  print OUT "// Relational Operator\n";
	  print OUT "\L$xpartName[$mm]". " = (($fromExp[0]) $op ($fromExp[1]));";
	}
	# ******** SUMMING JUNC ********************************************************************
	if($partType[$mm] eq "SUM")
	{
	   print OUT "// SUM\n";
		#print "\tUsed Sum $xpartName[$mm] $partOutCnt[$mm]\n";
		$calcExp = "\L$xpartName[$mm]";
		$calcExp .= " =";
		#print "Sum $xpartName[$mm] functions are $partInputs[$mm]\n";
		for (0 .. $inCnt - 1) {
		    my $op = substr($partInputs[$mm], $_, 1);
		    if ($op eq "") { $op = "+"; }
		    if ($_ > 0 || $op eq "-")  { $calcExp .=  " " . $op; } 
		    $calcExp .= " " . $fromExp[$_];
		}
		$calcExp .= ";\n";
		print OUT "$calcExp";
	}

	# ******** MULTIPLY ********************************************************************
	if($partType[$mm] eq "MULTIPLY")
	{
	   print OUT "// MULTIPLY\n";
		#print "\tUsed Sum $xpartName[$mm] $partOutCnt[$mm]\n";
		$calcExp = "\L$xpartName[$mm]";
		$calcExp .= " = ";
		for($qq=0;$qq<$inCnt;$qq++)
		{
		    $zz = $qq+1;
		    if(($zz - $inCnt) == 0)
	 	    {
			$calcExp .= $fromExp[$qq];
			$calcExp .= ";\n";
		    }
		    else {
			$calcExp .= $fromExp[$qq];
			$calcExp .= " * ";
		    }
		}
		print OUT "$calcExp";
	}
	if($partType[$mm] eq "Gain")
	{
	   print OUT "// Gain\n";
	   $calcExp = "\L$xpartName[$mm] = " . $fromExp[0] . " * " . $partInputs[$mm] . ";\n";
	   print OUT "$calcExp";
	}
	if($partType[$mm] eq "Abs")
	{
	   print OUT "// Abs\n";
	   $calcExp = "\L$xpartName[$mm] = " . $fromExp[0] . " < 0.0? - " . $fromExp[0] . ": " . $fromExp[0] . ";\n";
	   print OUT "$calcExp";
	}
	# ******** AND ********************************************************************
	if($partType[$mm] eq "AND")
	{
	   print OUT "// Logical AND\n";
		# print "\tUsed AND $xpartName[$mm] $partOutCnt[$mm]\n";
		$calcExp = "\L$xpartName[$mm]";
		$calcExp .= " = ";
		for($qq=0;$qq<$inCnt;$qq++)
		{
		    $zz = $qq+1;
		    if(($zz - $inCnt) == 0)
	 	    {
			$calcExp .= $fromExp[$qq];
			$calcExp .= ";\n";
		    }
		    else {
			$calcExp .= $fromExp[$qq];
			$calcExp .= " && ";
		    }
		}
		print OUT "$calcExp";
	}
	# ******** DIVIDE ********************************************************************
	if($partType[$mm] eq "DIVIDE")
	{
	   print OUT "// DIVIDE\n";
		# print "\tUsed Divide $xpartName[$mm] $partOutCnt[$mm]\n";
		$calcExp = "if\(";
		$calcExp .= "$fromExp[1] \!= 0.0)\n";
		print OUT "$calcExp";
		print OUT "\{\n";
		$calcExp = "\t\L$xpartName[$mm]";
		$calcExp .= " = ";
		$calcExp .= $fromExp[0];
		$calcExp .= " \/ ";
		$calcExp .= $fromExp[1];
		$calcExp .= ";\n";
		print OUT "$calcExp";
		print OUT "\}\n";
		print OUT "else\{\n";
		$calcExp = "\t\L$xpartName[$mm] = 0.0;\n";
		print OUT "$calcExp";
		print OUT "\}\n";
	}
        # Process Math Function blocks  ========================================  MA  ===
	if ($partType[$mm] eq "M_SQR") {                                   # ===  MA  ===
	   print OUT "// MATH FUNCTION - SQUARE\n";                        # ===  MA  ===
		$calcExp = "\L$xpartName[$mm]";                            # ===  MA  ===
		$calcExp .= " = ";                                         # ===  MA  ===
		$calcExp .= $fromExp[0];                                   # ===  MA  ===
		$calcExp .= " \* ";                                        # ===  MA  ===
		$calcExp .= $fromExp[0];                                   # ===  MA  ===
		$calcExp .= ";\n";                                         # ===  MA  ===
		print OUT "$calcExp";                                      # ===  MA  ===
	}                                                                  # ===  MA  ===
	if ($partType[$mm] eq "M_SQT") {                                   # ===  MA  ===
	   print OUT "// MATH FUNCTION - SQUARE ROOT\n";                   # ===  MA  ===
		$calcExp = "if \(";                                        # ===  MA  ===
		$calcExp .= "$fromExp[0] \> 0.0) \{\n";                    # ===  MA  ===
		print OUT "$calcExp";                                      # ===  MA  ===
		$calcExp = "\t\L$xpartName[$mm]";                          # ===  MA  ===
		$calcExp .= " = ";                                         # ===  MA  ===
		$calcExp .= "lsqrt\($fromExp[0]\);\n";                     # ===  MA  ===
		print OUT "$calcExp";                                      # ===  MA  ===
		print OUT "\}\n";                                          # ===  MA  ===
		print OUT "else \{\n";                                     # ===  MA  ===
		$calcExp = "\t\L$xpartName[$mm] = 0.0;\n";                 # ===  MA  ===
		print OUT "$calcExp";                                      # ===  MA  ===
		print OUT "\}\n";                                          # ===  MA  ===
	}                                                                  # ===  MA  ===
	if ($partType[$mm] eq "M_REC") {                                   # ===  MA  ===
	   print OUT "// MATH FUNCTION - RECIPROCAL\n";                    # ===  MA  ===
		$calcExp = "if \(";                                        # ===  MA  ===
		$calcExp .= "$fromExp[0] \!= 0.0) \{\n";                   # ===  MA  ===
		print OUT "$calcExp";                                      # ===  MA  ===
		$calcExp = "\t\L$xpartName[$mm]";                          # ===  MA  ===
		$calcExp .= " = ";                                         # ===  MA  ===
		$calcExp .= "1.0/$fromExp[0];\n";                          # ===  MA  ===
		print OUT "$calcExp";                                      # ===  MA  ===
		print OUT "\}\n";                                          # ===  MA  ===
		print OUT "else \{\n";                                     # ===  MA  ===
		$calcExp = "\t\L$xpartName[$mm] = 0.0;\n";                 # ===  MA  ===
		print OUT "$calcExp";                                      # ===  MA  ===
		print OUT "\}\n";                                          # ===  MA  ===
	}                                                                  # ===  MA  ===
	if ($partType[$mm] eq "M_MOD") {                                   # ===  MA  ===
	   print OUT "// MATH FUNCTION - MODULO\n";                        # ===  MA  ===
		$calcExp = "if \(";                                        # ===  MA  ===
		$calcExp .= "(int) $fromExp[1] \!= 0) \{\n";               # ===  MA  ===
		print OUT "$calcExp";                                      # ===  MA  ===
		$calcExp = "\t\L$xpartName[$mm]";                          # ===  MA  ===
		$calcExp .= " = ";                                         # ===  MA  ===
		$calcExp .= "(double) ((int) $fromExp[0]\%";               # ===  MA  ===
                $calcExp .= "(int) $fromExp[1]);\n";                       # ===  MA  ===
		print OUT "$calcExp";                                      # ===  MA  ===
		print OUT "\}\n";                                          # ===  MA  ===
		print OUT "else\ {\n";                                     # ===  MA  ===
		$calcExp = "\t\L$xpartName[$mm] = 0.0;\n";                 # ===  MA  ===
		print OUT "$calcExp";                                      # ===  MA  ===
		print OUT "\}\n";                                          # ===  MA  ===
	}                                                                  # ===  MA  ===

	# ******** GROUND INPUT ********************************************************************
	if(($partType[$mm] eq "GROUND") && ($partUsed[$mm] == 0))
	{
	   #print "Found GROUND $xpartName[$mm] in loop\n";
	}

	# ******** DELAY ************************************************************************
	if($partType[$mm] eq "DELAY")
	{
	   print OUT "// DELAY\n";
		$calcExp = "\L$xpartName[$mm]";
		$calcExp .= " = ";
		$calcExp .= $fromExp[0];
		$calcExp .= ";\n";
		print OUT "$calcExp";
	}

	if($partType[$mm] eq "SATURATE")
	{
	   print OUT "// SATURATE\n";
		my $part_name = "\L$xpartName[$mm]";
		my $upper_limit = "$partInputs[$mm]";
		my $lower_limit = "$partInputs1[$mm]";
		$calcExp = $part_name;
		$calcExp .= " = ";
		$calcExp .= $fromExp[0];
		$calcExp .= ";\n";
		$calcExp .= "if ($part_name > $upper_limit) {\n";
		$calcExp .= "  $part_name  = $upper_limit;\n";
		$calcExp .= "} else if ($part_name < $lower_limit) {\n";
		$calcExp .= "  $part_name  = $lower_limit;\n";
		$calcExp .= "};\n";
		print OUT "$calcExp";
	}
print OUT "\n";
}
}

print OUT "    // All IPC outputs\n";
print OUT "    if (_ipc_shm != 0) {\n";
print OUT "$ipcOutputCode";
print OUT "    }\n";
print OUT "$feTailCode";

#
# The actual sending of IPCx data is to occur
# as the last step of the processing loop
#
if ($ipcxCnt > 0) {
   print OUT "\n    commData2Send(myIpcCount, ipcInfo, pLocalEpics->epicsOutput\.timeDiag, cycle);\n\n";
}

print OUT "  }\n";
print OUT "}\n\n";
print OUTH "typedef struct CDS_EPICS {\n";
print OUTH "\tCDS_EPICS_IN epicsInput;\n";
print OUTH "\tCDS_EPICS_OUT epicsOutput;\n";
print OUTH "\t\U$systemName \L$systemName;\n";
print OUTH "} CDS_EPICS;\n";

if ($remoteIPChosts) {
	print OUTH "#define MAX_REMOTE_IPC_VARS $maxRemoteIPCVars\n";
	print OUTH "#define MAX_REMOTE_IPC_HOSTS $maxRemoteIPCHosts\n";
}


if($partsRemaining != 0) {
	print "WARNING -- NOT ALL PARTS CONNECTED !!!!\n";
for($ii=0;$ii<$partCnt;$ii++)
{
	if($partUsed[$ii] == 0)
	{
	print "$ii $xpartName[$ii] $partOutCnt[$ii] $partUsed[$ii]\n";
	}
}
}

close IN;
close OUT;


print OUTH "\#define TARGET_HOST_NAME $targetHost\n";
print OUTH "\#endif\n";
close OUTH;
close OUTD;
close EPICS;
if ($no_rtl) {
	system ("/bin/cp GNUmakefile  ../../fe/$skeleton");
	system ("echo '#include \"../controller.c\"' > ../../fe/$skeleton/$skeleton" . "fe.c");
} else {
	system ("/bin/rm -f ../../fe/$skeleton/GNUmakefile");
}

if ($no_rtl) {
print OUTM "# CPU Shutdown Real Time Linux\n";
} else {
print OUTM "# RTLinux makefile\n";
if ($wind_river_rtlinux) {
  print OUTM "include /home/controls/common_pc_64_build/build/rtcore-base-5.1/rtl.mk\n";
} else {
  print OUTM "include /opt/rtldk-2.2/rtlinuxpro/rtl.mk\n";
}
print OUTM "\n";
print OUTM "\n";
print OUTM "TARGET_RTL := $skeleton";
print OUTM "fe\.rtl\n";
print OUTM "LIBRARY_OBJS := map.o myri.o myriexpress.o  fb.o\n";
print OUTM "LDFLAGS_\$(TARGET_RTL) := -g \$(LIBRARY_OBJS)\n";
print OUTM "\n";
print OUTM "\$(TARGET_RTL): \$(LIBRARY_OBJS)\n";
print OUTM "\n";
print OUTM "$skeleton";
print OUTM "fe\.o: ../controller.c\n";
print OUTM "\t\$(CC) \$(EXTRA_CFLAGS) -c \$< -o \$\@\n";
print OUTM "map.o: ../map.c\n";
print OUTM "\t\$(CC) \$(EXTRA_CFLAGS) -D__KERNEL__ -c \$<\n";
print OUTM "myri.o: ../myri.c\n";
print OUTM "\t\$(CC) \$(EXTRA_CFLAGS) -D__KERNEL__ -c \$<\n";
print OUTM "myriexpress.o: ../myriexpress.c\n";
print OUTM "\t\$(CC) \$(EXTRA_CFLAGS) -D__KERNEL__ -DMX_KERNEL=1 -c \$<\n";
print OUTM "fb.o: ../fb.c\n";
print OUTM "\t\$(CC) \$(EXTRA_CFLAGS) -D__KERNEL__ -c \$<\n";
print OUTM "fm10Gen.o: fm10Gen.c\n";
if($rate == 480) {
	print "SERVO IS 2K\n";
	print OUTM "\t\$(CC) \$(EXTRA_CFLAGS) -D__KERNEL__ -DSERVO2K -c \$<\n";
} elsif ($rate == 240) {
	print "SERVO IS 4K\n";
	print OUTM "\t\$(CC) \$(EXTRA_CFLAGS) -D__KERNEL__ -DSERVO4K -c \$<\n";
} elsif ($rate == 60) {
	print "SERVO IS 16K\n";
	print OUTM "\t\$(CC) \$(EXTRA_CFLAGS) -D__KERNEL__ -DSERVO16K -c \$<\n";
} elsif ($rate == 30) {
	print "SERVO IS 32K\n";
	print OUTM "\t\$(CC) \$(EXTRA_CFLAGS) -D__KERNEL__ -DSERVO32K -c \$<\n";
} elsif ($rate == 15) {
	print "SERVO IS 64K\n";
	print OUTM "\t\$(CC) \$(EXTRA_CFLAGS) -D__KERNEL__ -DSERVO64K -c \$<\n";
}
print OUTM "crc.o: crc.c\n";
print OUTM "\t\$(CC) \$(EXTRA_CFLAGS) -D__KERNEL__ -c \$<\n";
print OUTM "\n";
}
print OUTM "ALL \+= user_mmap \$(TARGET_RTL)\n";
print OUTM "EXTRA_CFLAGS += -I../../include\n";
print OUTM "EXTRA_CFLAGS += -I/opt/gm/include\n";
print OUTM "EXTRA_CFLAGS += -I/opt/mx/include\n";

if($rate == 480) { print OUTM "EXTRA_CFLAGS += -DSERVO2K\n"; }
elsif($rate == 240) { print OUTM "EXTRA_CFLAGS += -DSERVO4K\n"; }
elsif($rate == 60) { print OUTM "EXTRA_CFLAGS += -DSERVO16K\n"; }
elsif($rate == 30) { print OUTM "EXTRA_CFLAGS += -DSERVO32K\n"; }
elsif($rate == 15) { print OUTM "EXTRA_CFLAGS += -DSERVO64K\n"; }

print OUTM "EXTRA_CFLAGS += -D";
print OUTM "\U$skeleton";
print OUTM "_CODE\n";
if($systemName eq "sei" || $useFIRs)
{
print OUTM "EXTRA_CFLAGS += -DFIR_FILTERS\n";
}
if ($cpus > 2) {
print OUTM "EXTRA_CFLAGS += -DRESERVE_CPU2\n";
}
if ($cpus > 3) {
print OUTM "EXTRA_CFLAGS += -DRESERVE_CPU3\n";
}
print OUTM "EXTRA_CFLAGS += -D_ADVANCED_LIGO=1\n";
print OUTM "EXTRA_CFLAGS += -g\n";
if ($adcOver) {
  print OUTM "EXTRA_CFLAGS += -DROLLING_OVERFLOWS\n";
}
if ($no_sync) {
  print OUTM "#Comment out to enable 1PPS synchronization\n";
  print OUTM "EXTRA_CFLAGS += -DNO_SYNC\n";
} else {
  print OUTM "#Uncomment to disable 1PPS signal sinchronization (channel 31 (last), ADC 0)\n";
  print OUTM "#EXTRA_CFLAGS += -DNO_SYNC\n";
}
if ($no_daq) {
  print OUTM "#Comment out to enable DAQ\n";
  print OUTM "EXTRA_CFLAGS += -DNO_DAQ\n";
} else {
  print OUTM "#Uncomment to disable DAQ and testpoints\n";
  print OUTM "#EXTRA_CFLAGS += -DNO_DAQ\n";
}
if ($remoteIPChosts) {
  print OUTM "#Comment out to disable remote IPC over MX\n";
  print OUTM "EXTRA_CFLAGS += -DUSE_MX=1\n";
} else {
  print OUTM "#Uncomment to enable remote IPC over MX\n";
  print OUTM "#EXTRA_CFLAGS += -DUSE_MX=1\n";
}
if ($shmem_daq) {
  print OUTM "#Comment out to disable local frame builder connection; uncomment USE_GM setting too\n";
  print OUTM "EXTRA_CFLAGS += -DSHMEM_DAQ\n";
  print OUTM "#EXTRA_CFLAGS += -DUSE_GM=1\n";
} else {
  print OUTM "#Uncomment to enable local frame builder; comment out USE_GM setting too\n";
  print OUTM "#EXTRA_CFLAGS += -DSHMEM_DAQ\n";
if ($no_daq) {
  print OUTM "#EXTRA_CFLAGS += -DUSE_GM=1\n";
} else {
  print OUTM "EXTRA_CFLAGS += -DUSE_GM=1\n";
}
}
# Use oversampling code if not 64K system
if($rate != 15) {
  if ($no_oversampling) {
    print OUTM "#Uncomment to oversample A/D inputs\n";
    print OUTM "#EXTRA_CFLAGS += -DOVERSAMPLE\n";
    print OUTM "#Uncomment to interpolate D/A outputs\n";
    print OUTM "#EXTRA_CFLAGS += -DOVERSAMPLE_DAC\n";
  } else {
    print OUTM "#Comment out to stop A/D oversampling\n";
    print OUTM "EXTRA_CFLAGS += -DOVERSAMPLE\n";
    if ($no_dac_interpolation) {
    } else {
      print OUTM "#Comment out to stop interpolating D/A outputs\n";
      print OUTM "EXTRA_CFLAGS += -DOVERSAMPLE_DAC\n";
    }
  }
}
if ($dac_internal_clocking) {
  print OUTM "#Comment out to enable external D/A converter clocking\n";
  print OUTM "EXTRA_CFLAGS += -DDAC_INTERNAL_CLOCKING\n";
}
if ($compat_initial_ligo) {
  print OUTM "#Comment out to disable initial LIGO compatibility\n";
  print OUTM "EXTRA_CFLAGS += -DCOMPAT_INITIAL_LIGO\n";
} else {
  print OUTM "#Uncomment to enable initial LIGO compatibility\n";
  print OUTM "#EXTRA_CFLAGS += -DCOMPAT_INITIAL_LIGO\n";
}
if ($adcMaster > -1) {
  print OUTM "EXTRA_CFLAGS += -DADC_MASTER\n";
} else {
  print OUTM "#Uncomment to run on an I/O Master \n";
  print OUTM "#EXTRA_CFLAGS += -DADC_MASTER\n";
}
if ($adcSlave > -1) {
  print OUTM "EXTRA_CFLAGS += -DADC_SLAVE\n";
} else {
  print OUTM "#Uncomment to run on an I/O slave process\n";
  print OUTM "#EXTRA_CFLAGS += -DADC_SLAVE\n";
}
if ($pciNet > -1) {
  print OUTM "#Enable use of PCIe RFM Network\n";
  print OUTM "DISDIR = /home/controls/DIS\n";
  print OUTM "EXTRA_CFLAGS += -DOS_IS_LINUX=1 -D_KERNEL=1 -I\$(DISDIR)/src/IRM/drv/src -I\$(DISDIR)/src/IRM/drv/src/LINUX -I\$(DISDIR)/src/include -I\$(DISDIR)/src/include/dis -DDOLPHIN_TEST=1  -DDIS_BROADCAST=0x80000000\n";
} else {
  print OUTM "#Uncomment to use PCIe RFM Network\n";
  print OUTM "#DISDIR = /home/controls/DIS\n";
  print OUTM "#EXTRA_CFLAGS += -DOS_IS_LINUX=1 -D_KERNEL=1 -I\$(DISDIR)/src/IRM/drv/src -I\$(DISDIR)/src/IRM/drv/src/LINUX -I\$(DISDIR)/src/include -I\$(DISDIR)/src/include/dis -DDOLPHIN_TEST=1  -DDIS_BROADCAST=0x80000000\n";
}
if ($specificCpu > -1) {
  print OUTM "#Comment out to run on first available CPU\n";
  print OUTM "EXTRA_CFLAGS += -DSPECIFIC_CPU=$specificCpu\n";
} else {
  print OUTM "#Uncomment to run on a specific CPU\n";
  print OUTM "#EXTRA_CFLAGS += -DSPECIFIC_CPU=2\n";
}

print OUTM "\n";
print OUTM "ifneq (\$(CDIR),)\n";
print OUTM "override EXTRA_CFLAGS += \$(patsubst %,-I../../../%,\$(CDIR))\n";
print OUTM "endif\n";

print OUTM "\n";
print OUTM "all: \$(ALL)\n";
print OUTM "\n";
print OUTM "clean:\n";
print OUTM "\trm -f \$(ALL) *.o\n";
print OUTM "\n";
if ($no_rtl) {

print OUTM "EXTRA_CFLAGS += -DNO_RTL=1\n";
print OUTM "EXTRA_CFLAGS += -I\$(SUBDIRS)/../../include\n";
print OUTM "EXTRA_CFLAGS += -ffast-math -msse2\n";

print OUTM "obj-m += $skeleton" . "fe.o\n";

} else {
if ($wind_river_rtlinux) {
  print OUTM "EXTRA_CFLAGS += -DKBUILD_MODNAME=1\n";
  print OUTM "include /home/controls/common_pc_64_build/build/rtcore-base-5.1/Rules.make\n";
} else {
  print OUTM "include /opt/rtldk-2.2/rtlinuxpro/Rules.make\n";
}
}
print OUTM "\n";
close OUTM;

print OUTME "\n";
print OUTME "# Define Epics system name. It should be unique.\n";
print OUTME "TARGET = $skeleton";
print OUTME "epics\n";
print OUTME "\n";
print OUTME "SRC = build/\$(TARGET)/";
print OUTME "$skeleton";
print OUTME "\.st\n";
print OUTME "\n";
print OUTME "SRC += src/drv/rfm.c\n";
print OUTME "SRC += src/drv/param.c\n";
print OUTME "SRC += src/drv/crc.c\n";
print OUTME "SRC += src/drv/fmReadCoeff.c\n";
#print OUTME "SRC += src/epics/seq/get_local_time.st\n";
for($ii=0;$ii<$useWd;$ii++)
{
	print OUTME "SRC += src/epics/seq/hepiWatchdog";
	print OUTME "\U$useWdName[$ii]";
	print OUTME "\L\.st\n";
}
print OUTME "\n";
#print OUTME "DB += src/epics/db/local_time.db\n";
print OUTME "DB += build/\$(TARGET)/";
print OUTME "$skeleton";
print OUTME "1\.db\n";
print OUTME "\n";
print OUTME "IFO = $site\n";
print OUTME "SITE = $location\n";
print OUTME "\n";
print OUTME "SEQ += \'";
print OUTME "$skeleton";
print OUTME ",(\"ifo=$site, site=$location, sys=\U$systemName\, \Lsysnum= $dcuId\, \Lsysfile=\U$skeleton \")\'\n";
#print OUTME "SEQ += \'get_local_time,(\"ifo=$site, sys=\U$systemName\")\'\n";
for($ii=0;$ii<$useWd;$ii++)
{
print OUTME "SEQ += \'";
print OUTME "hepiWatchdog";
print OUTME "\U$useWdName[$ii]";
print OUTME ",(\"ifo=$site, sys=\U$systemName\,\Lsubsys=\U$useWdName[$ii]\")\'\n";
}
print OUTME "\n";
print OUTME "EXTRA_CFLAGS += -D";
print OUTME "\U$skeleton";
print OUTME "_CODE\n";
print OUTME "\n";
print OUTME "LIBFLAGS += -lezca\n";
if($systemName eq "sei" || $useFIRs)
{
print OUTME "EXTRA_CFLAGS += -DFIR_FILTERS\n";
}
print OUTME "include config/Makefile.linux\n";
print OUTME "\n";
print OUTME "build/\$(TARGET)/";
print OUTME "$skeleton";
print OUTME "1\.db: build/\$(TARGET)/";
print OUTME "$skeleton";
print OUTME "\.db\n";
print OUTME "\tsed 's/%SYS%/";
print OUTME "\U$systemName";
print OUTME "/g;s/%SUBSYS%//g' \$< > \$\@\n";
print OUTME "\n";
print OUTME "\n";
close OUTME;


#exit;

sub get_freq {
if($rate == 480) {
	return 2*1024;
} elsif ($rate == 240) {
	return 4*1024;
} elsif ($rate == 60) {
	return 16*1024;
} elsif ($rate == 30) {
	return 32*1024;
} elsif ($rate == 15) {
	return 64*1024;
}
}

mkpath $configFilesDir, 0, 0755;

# Create DAQ config file (default section and a few ADC input channels)
#my $daqFile = $configFilesDir . "/$site" . uc($skeleton) . "\.ini";
#open(OUTG,">".$daqFile) || die "cannot open $daqFile file for writing";
#print OUTG 	"[default]\n".
#		"gain=1.00\n".
#		"acquire=0\n".
#		"dcuid=$dcuId\n".
#		"ifoid=$ifoid\n".
#		"datatype=4\n".
#		"datarate=" . get_freq() . "\n".
#		"offset=0\n".
#		"slope=6.1028e-05\n".
#		"units=V\n".
#		"\n";

#for ( 0 .. 2 ) {
	#print OUTG "[$site:" . uc($skeleton) . "-CHAN_" . $_ ."]\n";
	#print OUTG "chnnum=" . ($gdsTstart + 3*$_) . "\n";
	#print OUTG "#acquire=1\n";
#}

# Open testpoints file
#my $parFile = "../../../build/" . $ARGV[1] . "epics/" . $skeleton . "\.par";
#open(INTP,"<".$parFile) || die "cannot open $parFile file for reading";
## Read all lines into the array
#@tp_data=<INTP>;
#close INTP;

#my %sections;
#my @section_names;
#my $section_name;
#my $def_datarate;
#foreach (@tp_data) {
# s/\s+//g;
# if (@a = m/\[(.+)\]/) { $section_name = $a[0]; push @section_names, $a[0]; }
# elsif (@a = m/(.+)=(.+)/) {
#	$sections{$section_name}{$a[0]} = $a[1];
#	if ($a[0] eq "datarate") { $def_datarate = $a[1]; }
# }
#}
#my $cnt = 0;
## Print chnnum, datarate, 
#foreach (sort @section_names) {
#	if ($cnt < 2 && m/_OUT$/) {
#		$comment = "";
#		$cnt++;
#	} else {
#		$comment = "#";
#	}
#	print OUTG "${comment}[${_}_${def_datarate}]\n"; 
#	print OUTG  "${comment}acquire=0\n";
#	foreach $sec (keys %{$sections{$_}}) {
#	  if ($sec eq "chnnum" || $sec eq "datarate" || $sec eq "datatype") { 
#		print OUTG  "${comment}$sec=${$sections{$_}}{$sec}\n";
#	  }
#	}
#}

#print %sections;

close OUTG;

# Create Foton filter file (with header)
$jj = $filtCnt / 40;
$jj ++;
#print OUTG "$jj lines to print\n";
my $filtFile = $configFilesDir . "/$site" . uc($skeleton) . "\.txt";
open(OUTG, ">" . $filtFile) || die "cannot open  $filtFile file for writing";
print OUTG "# FILTERS FOR ONLINE SYSTEM\n".
	"#\n".
	"# Computer generated file: DO NOT EDIT\n".
	"#\n";

for($ii=0;$ii<$jj;$ii++)
{
	$kk = $ii * 4;
	print OUTG "\# MODULES $filterName[$kk] ";
	$kk ++;
	print OUTG "$filterName[$kk] ";
	$kk ++;
	print OUTG "$filterName[$kk] ";
	$kk ++;
	print OUTG "$filterName[$kk]\n";
}
close OUTG;

# Append if statements into header and front-end code select files
my $header_select_fname = "../../include/feSelectHeader.h";
open(IN,"<$header_select_fname") || die "cannot open $header_select_fname for reading\n";
my @lines = <IN>;
close IN;

# Search for our line
my $sname = uc $skeleton;
$sname .= "_CODE";
my $not_found = 1;
for (@lines) {
	if (/$sname/) {
		print "Found $skeleton system in $header_select_fname\n";
		$not_found = 0;
	}
}

if ($not_found) {
	# Output all the lines up to #else line, then add our new lines
	# then append the rest
	my $new_header_select_fname = "../../include/feSelectHeader_new.h";
	open(OUT,">$new_header_select_fname") || die "cannot open $new_header_select_fname for writing\n";
	for (@lines) {
		if (/#else/) {
			# Print out lines
			print OUT "#elif defined($sname)\n";
			print OUT "\t#include \"$skeleton.h\"\n";
		}
		print OUT $_;
	}
	close OUT;
	rename($header_select_fname, $header_select_fname . "~");
	rename($new_header_select_fname, $header_select_fname);
}

# Append into front-end code selection file
my $header_select_fname = "../../fe/feSelectCode.c";
open(IN,"<$header_select_fname") || die "cannot open $header_select_fname for reading\n";
my @lines = <IN>;
close IN;

# Search for our line
my $sname = uc $skeleton;
$sname .= "_CODE";
my $not_found = 1;
for (@lines) {
	if (/$sname/) {
		print "Found $skeleton system in $header_select_fname\n";
		$not_found = 0;
	}
}

if ($not_found) {
	# Output all the lines up to #else line, then add our new lines
	# then append the rest
	my $new_header_select_fname = "../../fe/feSelectCode_new.c";
	open(OUT,">$new_header_select_fname") || die "cannot open $new_header_select_fname for writing\n";
	for (@lines) {
		if (/#else/) {
			# Print out lines
			print OUT "#elif defined($sname)\n";
			print OUT "\t#include \"$skeleton/$skeleton.c\"\n";
		}
		print OUT $_;
	}
	close OUT;
	rename($header_select_fname, $header_select_fname . "~");
	rename($new_header_select_fname, $header_select_fname);
}



# Take care of generating Epics screens
mkpath $epicsScreensDir, 0, 0755;
my $usite = uc $site;
my $sysname = "FEC";
$sed_arg = "s/SITE_NAME/$site/g;s/CONTROL_SYSTEM_SYSTEM_NAME/" . uc($skeleton) . "/g;s/SYSTEM_NAME/" . uc($sysname) . "/g;s/GDS_NODE_ID/" . ($gdsNodeId - 1) . "/g;";
$sed_arg .= "s/LOCATION_NAME/$location/g;";
$sed_arg .= "s/DCU_NODE_ID/$dcuId/g;";
$sysname = uc($skeleton);
$sed_arg .= "s/FBID/$sysname/g;";
system("cat GDS_TP.adl | sed '$sed_arg' > $epicsScreensDir/$sysname" . "_GDS_TP.adl");
my $monitor_args = $sed_arg;
my $cur_subsys_num = 0;

# Determine whether passed name need to become a top name
# i.e. whether the system/subsystem parts need to excluded 
sub is_top_name {
   ($_) =  @_;
   @d = split(/_/);
   $d = shift @d;
   #print "$d @top_names\n";
   foreach $item (@top_names) {
        #print  "   $item $d\n";
        if ($item eq $d) { return 1; }
   }
   return 0;
};

# Transform record name for exculsion of sys/subsystem parts
# This function replaces first underscode with the hyphen
sub top_name_transform {
   ($name) =  @_;
   $name =~ s/_/-/;
   return $name;
};

# Get the system name (the part before the hyphen)
sub system_name_part {
   ($name) =  @_;
   $name =~ s/([^_]+)-\w+/$1/;
   return $name;
}

foreach $cur_part_num (0 .. $partCnt-1) {
	if ($cur_part_num >= $subSysPartStop[$cur_subsys_num]) {
		$cur_subsys_num += 1;
	}
	if ($partType[$cur_part_num] =~ /Matrix$/) {
		my $outcnt = $::matOuts[$cur_part_num]; #$partOutCnt[$cur_part_num];
		my $incnt = $partInCnt[$cur_part_num];
		if ($partType[$cur_part_num] eq "MuxMatrix") {
			# MuxMatrix uses mux and demux parts 
			$outcnt = $partOutputs[$partOutNum[$cur_part_num][0]];
			$incnt = $partInCnt[$partInNum[$cur_part_num][0]];
		}
		if ($partType[$cur_part_num] eq "FiltMuxMatrix") {
			# FiltMuxMatrix uses mux and demux parts
			$outcnt = $partOutputs[$partOutNum[$cur_part_num][0]];
			$incnt = $partInCnt[$partInNum[$cur_part_num][0]];
		}
		my $basename = $partName[$cur_part_num];
		if ($partSubName[$cur_part_num] ne "") {
			$basename = $partSubName[$cur_part_num] . "_" . $basename;
		}

# Create comma separated string from the lements of an array
sub commify_series {
    my $sepchar = grep(/,/ => @_) ? ";" : ",";
    (@_ == 0) ? ''                                      :
    (@_ == 1) ?  $_[0]                                   :
                join("$sepchar", @_[0 .. $#_]);
}
		$collabels = commify_series(@{$partInput[$cur_part_num]});

		# This doesn't work so well if output has branches
		#$rowlabels = commify_series(@{$partOutput[$cur_part_num]});
		$rowlabels = "";

        	for (0 .. $::partOutCnt[$cur_part_num]-1) {
           	  $::portUsed[$_] = 0;
        	}
        
        	for (0 .. $::partOutCnt[$cur_part_num]-1) {
          	  my $fromPort = $::partOutputPortUsed[$cur_part_num][$_];
          	  if ($::portUsed[$fromPort] == 0) {
            	    $::portUsed[$fromPort] = 1; 
		    if ($rowlabels) { $rowlabels .= ",";}
		    $rowlabels .= $partOutput[$cur_part_num][$_];
          	  }
        	}


		if (is_top_name($basename)) {

		  my $tn = top_name_transform($basename);
		  my $basename1 = $usite . ":" . $tn . "_";
		  my $filtername1 = $usite . $tn;
		  if ($partType[$cur_part_num] eq "FiltMuxMatrix") {
		    my $subDirName = "$epicsScreensDir/$usite" . "$basename";
		    mkdir $subDirName;
		    system("./mkfiltmatrix.pl --cols=$incnt --collabels=$collabels --rows=$outcnt --rowlabels=$rowlabels --chanbase=$basename1 --filterbase=$filtername1 > $epicsScreensDir/$usite" . $basename . "     .adl");
		    for ($row = 0; $row < $outcnt; $row ++) {
		      for ($col = 0; $col < $incnt; $col ++) {
			my $filt_name = "$partName[$cur_part_num]" . "_" . "$col" . "_" . "$row";
			print "FILTER Part $filt_name $partType[$cur_part_num] input partInput=$partInput[$cur_part_num][0] type='$partInputType[$cur_part_num][0]' \n";
		       	if ($partSubName[$cur_part_num] ne "") {
			    $filt_name = $partSubName[$cur_part_num] . "_" . $filt_name;
			}
			my $sys_name = uc($skeleton);
			my $sargs;
			
			my $tfn = top_name_transform($filt_name);
			my $nsys = system_name_part($tfn);
			$sargs = "s/CONTROL_SYSTEM_SYSTEM_NAME/" . uc($skeleton) . "/g;";
			$sargs .= "s/SITE_NAME/$site/g;s/SYSTEM_NAME/" . $nsys . "/g;";
                        $sargs .= "s/LOCATION_NAME/$location/g;";
			$sargs .= "s/FILTERNAME/$tfn/g;";
			$sargs .= "s/DCU_NODE_ID/$dcuId/g";
			system("cat FILTER.adl | sed '$sargs' > $subDirName/$usite" . $filt_name . ".adl");
		      
		      }
		    }
		  } else {
		    system("./mkmatrix.pl --cols=$incnt --collabels=$collabels --rows=$outcnt --rowlabels=$rowlabels --chanbase=$basename1 > $epicsScreensDir/$usite" . $basename . ".adl");
		  }

	  
		} else {
		  $sysname = substr($sysname, 2, 3);
		  my $basename1 = $usite . ":" .$sysname ."-" . $basename . "_";
		  my $filtername1 = $usite . $sysname . "_" . $basename;
		  $sysname = uc($skeleton);
		  #print "Matrix $basename $incnt X $outcnt\n";
		  if ($partType[$cur_part_num] eq "FiltMuxMatrix") {
		    my $subDirName = "$epicsScreensDir/$usite" . "$sysname" . "_" . "$basename";
		    mkdir $subDirName;
		    system("./mkfiltmatrix.pl --cols=$incnt --collabels=$collabels --rows=$outcnt --rowlabels=$rowlabels --chanbase=$basename1 --filterbase=$filtername1 > $epicsScreensDir/$usite$sysname" . "_" .      $basename . ".adl");
                    for ($row = 0; $row < $outcnt; $row ++) {
		      for ($col = 0; $col < $incnt; $col ++) {
			my $filt_name = "$partName[$cur_part_num]" . "_" . "$col" . "_" . "$row";
			print "FILTER Part $filt_name $partName[$cur_part_num] $partType[$cur_part_num] input partInput=$partInput[$cur_part_num][0] type='$partInputType[$cur_part_num][0]' \n";
			if ($partSubName[$cur_part_num] ne "") {
			    $filt_name = $partSubName[$cur_part_num] . "_" . $filt_name;
			}
			my $sys_name = uc($skeleton);
			my $sargs;

			$sys_name = substr($sys_name,0,3);
			$sargs = $sed_arg . "s/FILTERNAME/$sys_name-$filt_name/g;";
			$sargs .= "s/DCU_NODE_ID/$dcuId/g";
			$sysname = uc($skeleton);
			system("cat FILTER.adl | sed '$sargs' > $subDirName/$usite$sysname" . "_" . $filt_name . ".adl");
		      }
		    }
		  } else {
		    system("./mkmatrix.pl --cols=$incnt --collabels=$collabels --rows=$outcnt --rowlabels=$rowlabels --chanbase=$basename1 > $epicsScreensDir/$usite$sysname" . "_" . $basename . ".adl");
		  }
		}
	}
	if (($partType[$cur_part_num] =~ /^Filt/) && (not ($partType[$cur_part_num] eq "FiltMuxMatrix"))) {
		#print "FILTER No=$cur_part_num Part $partName[$cur_part_num] $partType[$cur_part_num] input partInput=$partInput[$cur_part_num][0] type='$partInputType[$cur_part_num][0]' \n";
		my $filt_name = $partName[$cur_part_num];
	if ($partInputType[$cur_part_num][0] eq "Adc") {
		#exit(1);
	}
		if ($partSubName[$cur_part_num] ne "") {
			$filt_name = $partSubName[$cur_part_num] . "_" . $filt_name;
		}
		my $sys_name = uc($skeleton);
		my $sargs;
		if (is_top_name($filt_name)) {
			my $tfn = top_name_transform($filt_name);
			my $nsys = system_name_part($tfn);
			$sargs = "s/CONTROL_SYSTEM_SYSTEM_NAME/" . uc($skeleton) . "/g;";
			$sargs .= "s/SITE_NAME/$site/g;s/SYSTEM_NAME/" . $nsys . "/g;";
                        $sargs .= "s/LOCATION_NAME/$location/g;";
			$sargs .= "s/FILTERNAME/$tfn/g;";
			$sargs .= "s/DCU_NODE_ID/$dcuId/g";
			system("cat FILTER.adl | sed '$sargs' > $epicsScreensDir/" . $filt_name . ".adl");
		} else {
		  $sys_name = substr($sys_name, 2, 3);
			$sargs = $sed_arg . "s/FILTERNAME/$sys_name-$filt_name/g;";
			$sargs .= "s/DCU_NODE_ID/$dcuId/g";
		  $sysname = uc($skeleton);
			system("cat FILTER.adl | sed '$sargs' > $epicsScreensDir/$sysname" . "_" . $filt_name . ".adl");
		}
	}
	#print "No=$cur_part_num\n";
	if ($partInputType[$cur_part_num][0] eq "Adc") {
		  $sysname = uc($skeleton);
		  $sysname = substr($sysname, 2, 3);
		my $part_name = $partName[$cur_part_num];
		if (is_top_name($partSubName[$cur_part_num])) {
		  $sysname = substr($partSubName[$cur_part_num], 2, 3);
	print "ADC MONITOR IS TOP =$sysname\n";
		}
		#print "ADC input Part $part_name $partType[$cur_part_num] has Adc input \'$partInput[$cur_part_num][0]\'\n";
		if ($partType[$cur_part_num] eq "Filt") {
		  $monitor_args .= "s/\"$partInput[$cur_part_num][0]\"/\"" . $subSysName[$cur_subsys_num]  . ($subSysName[$cur_subsys_num] eq "" ? "": "_") . "$part_name ($partInput[$cur_part_num][0])\"/g;";
		  $monitor_args .= "s/\"$partInput[$cur_part_num][0]_EPICS_CHANNEL\"/\"" . $site . "\:$sysname-" . $subSysName[$cur_subsys_num]  . ($subSysName[$cur_subsys_num] eq "" ? "": "_") . $part_name . "_INMON"  .  "\"/g;";
		} elsif ($partType[$cur_part_num] eq "EpicsOut") {
		  $monitor_args .= "s/\"$partInput[$cur_part_num][0]\"/\"" . $subSysName[$cur_subsys_num]  . ($subSysName[$cur_subsys_num] eq "" ? "": "_") . "$part_name ($partInput[$cur_part_num][0])\"/g;";
		  $monitor_args .= "s/\"$partInput[$cur_part_num][0]_EPICS_CHANNEL\"/\"" . $site . "\:$sysname-" . $subSysName[$cur_subsys_num]  . ($subSysName[$cur_subsys_num] eq "" ? "": "_") . $part_name . "\"/g;";
		}
        }
		  $sysname = uc($skeleton);
}
#print $monitor_args;
for (0 .. $adcCnt - 1) {
   my $adc_monitor_args = $monitor_args;
   $adc_monitor_args .= "s/MONITOR_ADC/MONITOR_ADC$_/g;";
#print $adc_monitor_args;
   system("cat MONITOR.adl | sed 's/adc_0/adc_$_/' |  sed '$adc_monitor_args' > $epicsScreensDir/$sysname" . "_MONITOR_ADC$_.adl");
}
