#!/usr/bin/perl

#//     \page feCodeGen feCodeGen.pl
#//     Documentation for feCodeGen.pl - Controls parsing of Matlab files and code generation.
#//

#// \n\n This script is invoked by the auto generated build/src/epics/util/Makefile. \n\n\n
use File::Path;
use Cwd;
require "lib/SUM.pm";
require "lib/AND.pm";
require "lib/MULTIPLY.pm";
require "lib/DIVIDE.pm";
require "lib/SATURATE.pm";
require "lib/MUX.pm";
require "lib/DEMUX.pm";
require "lib/RelationalOperator.pm";
require "lib/Switch.pm";
require "lib/Gain.pm";
require "lib/Abs.pm";
require "lib/MATH.pm";
require "lib/Dac20.pm";

#// \b REQUIRED \b ARGUMENTS: \n
#//	- Model file name with .mdl extension \n
#//	- Output file name (from Makefile, this is same name without .mdl extension.\n
#//	
#// Remaining arguments listed in code are optional and not normally used by RCG Makefile. \n\n

# Normal call from Makefile is just first two args.

#//	
#// \b PRODUCTS: \n
#//	- C code 
#//		- C source file <em> (build/src/fe/model_name/model_name.c)</em>, representing user model parts and processing sequence.\n
#//		- C code Makefile <em>(build/fe/model_name/Makefile)</em>. \n
#//	- Header file <em>(build/src/include/model_name/model_name.h)</em>, containing data passing structure between real-time code and EPICS sequencer code. \n
#//	- EPICS channel list <em>(build/src/epics/fmseq/model_name)</em>, to be used later by <em>fmseq.pl</em> to produce EPICS products. Included in this file: \n
#//		- List of all filter modules \n
#//		- List of all EPICS channel names and types. \n
#//		- List of extra test points ie those not associated with Filter modules. \n
#//	- EPICS state code Makefile <em>(build/config/Makefile.model_nameepics) </em>
#//	- File containing list of all code source files <em>(build/src/epics/util/sources.model_name)</em> \n
#//	- File containing list of all DAQ channels and rates <em>(build/src/epics/fmseq/model_name_daq)</em>.
#//		- NOTE: Opened here, but actually written to by <em>lib/Parser3.pm</em>
#//	- Foton IIR filter definition file <em>(build/build/model_nameepics/config/MODEL_NAME.txt)</em>
#//	- Various common MEDM screen files, including:
#//		- MODEL_NAME_GDS_TP.adl: Contains primary runtime diags, including timing, networks, DAQ. 
#//			- This code calls sub in <em>lib/medmGenGdsTp.pm</em> to actually produce the MEDM file.
#//		- MODEL_NAME_DAC_MONITOR_num.adl: Outputs from DAC modules, directly from the main sequencer code. 
#//			- This code calls sub in <em>lib/DAC.pm</em> or <em>lib/DAC18.pm</em> to actually produce the MEDM files.
#//		- MODEL_NAME_MONITOR_ADCnum.adl: Inputs to all ADC channels used by the model. 
#//			- This code calls sub in <em>lib/ADC.pm</em> to actually produce the MEDM files.
#//		- MODEL_NAME_MATRIXNAME.adl: Inputs from matrix elements.
#//			- This code calls sub in <em>mkmatrix.pl</em> to actually produce the MEDM files.
#//		- MODEL_NAME_FILTERNAME.adl: Interface for Filter modules.
#//			- This code calls sub in <em>/lib/Filt.pm</em> to actually produce the MEDM files.
#//	- Parser diagnostics file <em>(build/epics/util/diags.txt)</em>, which lists all parts and their connections after model parsing.

#	
#//	
#// <b>BASIC CODE SEQUENCE:</b>: \n
#//	

die "Usage: $PROGRAM_NAME <MDL file> <Output file name> [<DCUID number>] [<site>] [<speed>]\n\t" . "site is (e.g.) H1, M1; speed is 2K, 16K, 32K or 64K\n"
        if (@ARGV != 2 && @ARGV != 3 && @ARGV != 4 && @ARGV != 5);

#Setup current working directory and pointer to RCG source directory.
$currWorkDir = &Cwd::cwd();
$rcg_src_dir = $ENV{"RCG_SRC_DIR"};
$lrpciefile = $rcg_src_dir . "/src/include/USE_LR_PCIE";
$pciegenfile = $rcg_src_dir . "/src/include/USE_DOLPHIN_GEN2";
$zmqfile = $rcg_src_dir . "/src/include/USE_ZMQ";
$usezmq = 0;
$mbufsymfile = $ENV{"MBUFSYM"};
$gpssymfile = $ENV{"GPSSYM"};

if (-e "$zmqfile") {
        print "Using ZMQ for DAQ\n";
        $usezmq = 1;
}
if (-e "$lrpciefile") {
        print "PCIE LR exists\n";
        $rfm_via_pcie = 1;
} else {
        print "PCIE LR DOES NOT exist\n";
        $rfm_via_pcie = 0;
}
if (! length $rcg_src_dir) { $rcg_src_dir = "$currWorkDir/../../.."; }

@sources = ();

#//	- Search for the Matlab file in the RCG_LIB_PATH; exit if not found \n
@rcg_lib_path = split(':', $ENV{"RCG_LIB_PATH"});
push @rcg_lib_path, "$rcg_src_dir/src/epics/simLink";
#print join "\n", @rcg_lib_path, "\n";
my $model_file_found = 0;
$mdlfile = $ARGV[0];
foreach $i (@rcg_lib_path) {
	my $fname = $i;
	$fname .= "/";
	$fname .= $ARGV[0];
	if (-r $fname) {
		print "Model file found $fname", "\n";
		print "RCG_LIB_PATH=". join(":", @rcg_lib_path)."\n";
		$ARGV[0] = $fname;
		$model_file_found = 1;
		push @sources, $fname;
		last;
	}
}

# Exit if cannot find the Matlab file.
die "Could't find model file $ARGV[0] on RCG_LIB_PATH " . join(":", @rcg_lib_path). "\n"  unless $model_file_found;

# Default is to run LIGO patched Linux; Windriver support removed 1/14/2013 RGB.
print "Generating Firm Real-time code for patched vanilla Linux kernel\n";

# Get MAX_DIO_MODULES allowed from header file.
# This is used by Parser3.pm to stop compile if RCG limits exceeded.
my $mdmStr = `grep "define MAX_DIO_MODULES" ../../include/drv/cdsHardware.h`;
my @mdmNum = ($mdmStr =~ m/(\d+)/);
$maxDioMod = pop(@mdmNum);

# Initialize default settings.
$site = "M1"; # Default value for the site name
$location = "mit"; # Default value for the location name
$rate = "60"; # In microseconds (default setting)
$brate = "52";
$dcuId = 10; # Default dcu Id
$targetHost = "localhost"; # Default target host name
$edcusync = "none";
$specificCpu = -1; # Defaults is to run the FE on the first available CPU
$adcMaster = -1;
$dacWdOverride = -1;
$adcSlave = -1;
$timeMaster = -1;
$timeSlave = -1;
$iopTimeSlave = -1;
$rfmTimeSlave = -1;
$diagTest = -1;
$flipSignals = 0;
$ipcrate = 0;
$ipccycle = 0;
$virtualiop = 0;
$no_cpu_shutdown = 0;
$edcu = 0;
$casdf = 0;
$globalsdf = 0;
$pciNet = -1;
$shmem_daq = 0; # Do not use shared memory DAQ connection
$no_sync = 0; # Sync up to 1PPS by default
$no_daq = 0; # Enable DAQ by default
$gdsNodeId = 0;
$ifoid = 0; # Default ifoid for the DAQ
$nodeid = 0; # Default GDS node id for awgtpman
$dac_internal_clocking = 0; # Default is DAC external clocking
$no_oversampling = 0; # Default is to iversample
$no_dac_interpolation = 0; # Default is to interpolate D/A outputs
$max_name_len = 39;	# Maximum part name length
$dacKillMod[0][0] = undef;
$dacKillModCnt[0] = undef;
@dacKillDko = qw(x x x x x x x x x x x x x);
$dkTimesCalled = 0;
$remoteGpsPart = 0;
$remoteGPS = 0;
$daq2dc = 0;
$requireIOcnt = 0;
$adcclock = 64;
$adcrate = 64;
$adc_std_rate = 64;

# Normally, ARGV !> 2, so the following are not invoked in a standard make
# This is legacy.
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
		$location = "stn";
	} elsif ($site =~ /^K/) {
		$location = "kamioka";
	} elsif ($site =~ /^X/) {
		$location = "tst";
	} elsif ($site =~ /^A/) {
		$location = "anu";
	} elsif ($site =~ /^I/) {
		$location = "indigo";
	} elsif ($site =~ /^U/) {
		$location = "uwa";
	} elsif ($site =~ /^W/) {
		$location = "cardiff";
	} elsif ($site =~ /^B/) {
		$location = "bham";
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
	} elsif ($param_speed eq "256K") {
		$rate = 4;
	} elsif ($param_speed eq "512K") {
		$rate = 2; 
	} elsif ($param_speed eq "1024K") {
		$rate = 1;
	} else  { die "Invalid speed $param_speed specified\n"; }
}



# Load model name without .mdl extension.
$skeleton = $ARGV[1];

# Check to verify model name begins with a valid IFO designator.
if ($skeleton !~ m/^[acghiklmsuwx]\d.*/) {
   die "***ERROR: Model name must begin with <ifo><subsystem>: $skeleton\n";
}

# First two chars of model name must be IFO, such as h1, l1, h2, etc.
$ifo = substr($skeleton, 0, 2);

#//	- Create the paths for RCG output files. \n
print "file out is $skeleton\n";
$cFile = "../../fe/";
$cFile .= $ARGV[1];
$cFileDirectory = $cFile;
$cFileDirectory2 = $cFileDirectory . "_usp";
$cFile .= "/";
$cFile .= $ARGV[1];
$cFile .= ".c";
$cFile2 = $cFileDirectory2;
$cFile2 .= "/";
$cFile2 .= $ARGV[1];
$cFile2 .= ".c";
$hFile = "../../include/";
$hFile .= $ARGV[1];
$hFile .= ".h";
$mFile = "../../fe/";
$mFile .= $ARGV[1];
$mFile .= "/";
$mFile .= "Makefile";
$mFile2 = $cFileDirectory2;
$mFile2 .= "/";
$mFile2 .= "Makefile";
$meFile = "../../../config/";
$meFile .= "Makefile\.";
$meFile .= $ARGV[1];
$meFile .= epics;
$epicsScreensDir = "../../../build/" . $ARGV[1] . "epics/medm";
$configFilesDir = "../../../build/" . $ARGV[1] . "epics/config";
$compileMessageDir = "../../../";
$warnMsgFile = $compileMessageDir . $ARGV[1] . "_warnings.log";
$connectErrFile = $compileMessageDir . $ARGV[1] . "_partConnectErrors.log";
$partConnectFile = $compileMessageDir . $ARGV[1] . "_partConnectionList.txt";

# This is where the various RCG output files are created and opened.
if (@ARGV == 2) { $skeleton = $ARGV[1]; }
# Open files for EPICS generation by fmseq.pl
# Need to open early as Parser3.pm will write filter name info first.
open(EPICS,">../fmseq/".$ARGV[1]) || die "cannot open output file for writing";
open(DAQ,">../fmseq/".$ARGV[1]."_daq") || die "cannot open DAQ output file for writing";
# Open compilation message files
open(WARNINGS,">$warnMsgFile") || die "cannot open compile warnings output file for writing";
open(CONN_ERRORS,">$connectErrFile") || die "cannot open compile warnings output file for writing";
mkdir $cFileDirectory, 0755;
mkdir $cFileDirectory2, 0755;
open(OUT,">./".$cFile) || die "cannot open c file for writing $cFile";
open(OUT2,">./".$cFile2) || die "cannot open c file for writing $cFile2";
# Save existing front-end Makefile
@months = qw(Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec);
  my ($second, $minute, $hour, $dayOfMonth, $month, $yearOffset, $dayOfWeek, $dayOfYear, $daylightSavings) = localtime();
  my $year = 1900 + $yearOffset;
  $theTime = sprintf("%d_%s_%02d_%02d:%02d:%02d", $year, $months[$month], $dayOfMonth, $hour, $minute, $second);
my $hfname = "$rcg_src_dir/src/include/$ARGV[1].h";
if (-e $hfname) {
	system("/bin/mv -f $hfname $hfname~");
}
# Need to open header file early, as calls in Parser3.pm will start the writing process.
open(OUTH,">./".$hFile) || die "cannot open header file for writing";


$mySeq = 0;
$connects = 0;
$ob = 0;
$subSys = 0;
$inBranch = 0;
$endBranch = 0;
$adcCnt = 0;
$dacCnt = 0;
$dacKillCnt = 0;
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
$dac16Cnt = 0;
$dac18Cnt = 0;

# IPCx PART CODE VARIABLES
$ipcxCnt = 0;                                                              # ===  IPCx  ===
$ipcxDeclDone = 0;                                                         # ===  IPCx  ===
$ipcxInitDone = 0;                                                         # ===  IPCx  ===
$ipcxBlockTags[0] = undef;
$ipcxParts[0][0] = undef;
$ipcxTagCount = 0;
$ipcxReset = "";
# END IPCx PART CODE VARIABLES

$oscUsed = 0; # Needed by OSC part for one time inits.
$useFIRs = 0;
# ***  DEBUG  ***
# $useFIRs = 1;
# ***  DEBUG  ***

# set debug level (0 - no debug messages)
$dbg_level = 2;


my $system_name = $ARGV[1];
print OUTH "\#ifndef \U$system_name";
print OUTH "_H_INCLUDED\n\#define \U$system_name";
print OUTH "_H_INCLUDED\n";
print OUTH "\#define SYSTEM_NAME_STRING_LOWER \"\L$system_name\"\n";


require "lib/ParsingDiagnostics.pm";

#Initialize various parser variables.
init_vars();

#//	- Read .mdl file and flatten all subsystems to top level subsystem part. \n
#//		- Done by making calls to subs in <em>lib/Parers3.pm</em>
require "lib/Parser3.pm";
open(IN,"<".$ARGV[0]) || die "cannot open mdl file $ARGV[0]\n";
die unless CDS::Parser::parse();
die unless CDS::Parser::process();
die unless CDS::Parser::sortDacs();

close(IN);

if(($adcMaster == 1) and ($adcrate > $adcclock)) 
{
	die "Error:\nModel rate $adcrate > ADC clock $adcclock\nFix adcclock in Param Block\n*****\n";
}

#//	
#// Model now consists of top level parts and single level subsystem(s). <em>Parser3.pm</em> has taken care of all part
#// removals/connections for lower level subsystems.\n
#// Following parsing code now needs to finish top level connections. \n

$systemName = substr($systemName, 2, 3);
$plantName = $systemName; # Default plant name is the model name

#//	- Perform some specific part checks/processing.
#//		- Check parts which require GROUND input in fact have ground connections. \n
# 
# Make sure all EpicsCounter, EpicsMbbi, and
# EpicsStringIn modules have Ground as input
#
for ($ii = 0; $ii < $partCnt; $ii++) {
   if ( ($partType[$ii] eq "EpicsCounter") ||
#       ($partType[$ii] eq "EpicsMbbi") ||
        ($partType[$ii] eq "EpicsStringIn") ) {
      if ( ($partInCnt[$ii] != 1) || ($partInput[$ii][0] !~ /Ground/) ) {
         die "\n***ERROR: $partType[$ii] with name $xpartName[$ii] must have Ground as input\n";
      }
   }
}

#//		- Process all IPC parts in one go. Requires <em>lib/IPCx.pm</em>, with call to procIpc. \n
require "lib/IPCx.pm";
("CDS::IPCx::procIpc") -> ($partCnt);


#//		- Check that all subsystem INPUT parts are connected; else exit w/error. \n
$kk = 0;
require "lib/Input.pm";
$kk = ("CDS::Input::checkInputConnect") -> ($partCnt,0);
if($kk > 0)
{
         die "\n***ERROR: Found total of ** $kk ** INPUT_X parts not connected\n\n";
}



#//	
#//	- Need to process BUSS, FROM and GOTO parts so they can be removed later.

#//		- Find all parts which have input from Bus Selector parts and feed thru actual part connections.\n
#//			- Change part input connect from BUSS to part feeding signal to BUSS.
#//			- Change output of part feeding BUSS directly to part receiving signal.
require "lib/BusSelect.pm";
("CDS::BusSelect::linkBusSelects") -> ($partCnt);

####################
#print "Looped thru $partCnt looking for BUSS \n\n\n";

#//		-  FIND and replace all GOTO links
# Supports MATLAB tags ie types Goto and From parts.
#//			- This section searches all part inputs for From tags, finds the real name
#// of the signal being sent to this tag, and substitutes that name at the part input. \n

require "lib/Tags.pm";
("CDS::Tags::replaceGoto") -> ($partCnt);

################


#//		- FIND and replace all FROM links \n
# Supports MATLAB tags ie types Goto and From parts.
# This section searches all part inputs for From tags, finds the real name
# of the signal being sent to this tag, and substitutes that name at the part input.

("CDS::Tags::replaceFrom") -> ($partCnt);

##################


#//	- Continue to make part connections.
# ********************************************************************
#//		- Take all of the part outputs and find connections.
#// Fill in connected part numbers and types.\n
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

#//		-  Take all of the part inputs and find connections.
#// Fill in connected part numbers and types. \n
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

#//	- Start the process of removing subsystems OUTPUT parts \n
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

				# Do not try to find the output by name
				if (0) {
				$fromPort = $partInputPort[$ii][0];
				for($xxx=0;$xxx<$partOutCnt[$fromNum];$xxx++)
				{
					if($xpartName[$ii] eq $partOutput[$fromNum][$xxx])
					{
						$fromPort = $xxx;
					}
				}
				} # 0

				# Add to the output count, do not try to find and replace
				$fromPort = $partOutCnt[$fromNum];
				$partOutCnt[$fromNum]++;

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
		print WARNINGS "WARNING  *********** No connection to subsystem output named  $xpartName[$ii] $partOutput[$ii][0] $partOutputPort[$ii][0]\n";
	}
	}
}

#//	- Find connections for non subsystem (top level) parts \n
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
				
				# print "OUTPUT COUNT for $xpartName[$xx] is $partOutCnt[$kk]  ** $mm ** \n";
				for($ll=0;$ll<$partOutCnt[$kk];$ll++)
				{
					$toNum = $partOutNum[$kk][$ll];
					$toPort = $partOutputPort[$kk][$ll];
					$toPort1 = $partOutputPortUsed[$kk][0];
					#$toPort1 = $partOutputPortUsed[$kk][$ll];
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

#//	-  Remove all parts which will not require further processing in the code for the part
#// total. \n
$ftotal = $partCnt;
   for($kk=0;$kk<$partCnt;$kk++)
   {
	 if(($partType[$kk] eq "INPUT") || ($partType[$kk] eq "OUTPUT") || ($partType[$kk] eq "BUSC") || ($partType[$kk] eq "BUSS") || ($partType[$kk] eq "EpicsIn") || ($partType[$kk] eq "TERM") || ($partType[$kk] eq "FROM") || ($partType[$kk] eq "GOTO") || ($partType[$kk] eq "GROUND") || ($partType[$kk] eq "CONSTANT") || ($partType[$kk] eq "Adc") || ($partType[$kk] eq "Gps") || ($partType[$kk] eq "StateWord") || ($partType[$kk] eq "ModelRate") || ($partType[$kk] eq "EXC"))
	{
		$ftotal --;
	}
   }

print "Total parts to process $ftotal\n";

# DIAGNOSTIC
print "Found $subSys subsystems\n";

#//	
#// \n
#// At this point, all parts should have all of their defined inputs and output connections made.
#//	- Write a parts and connection list to file <em>(build/modelname_partConnectionList.txt)</em> for diagnostics. \n\n

# Diags file will provide list of all parts and their connections. 
writeDiagsFile($partConnectFile);


#//	- Verify that all parts now have input connections. If not,
#//		- Write missing connections list to <em>(build/modelname_partConnectionErrors.log)</em>
#//		- Exit on error
$kk = 0;
# Check all INPUT parts to verify they are all connected.
require "lib/Input.pm";
$kk = ("CDS::Input::checkInputConnect") -> ($partCnt,1);

if($kk > 0)
{
        close CONN_ERRORS;
        die "\n***ERROR: Found total of ** $kk ** INPUT_Y parts not connected\n\n";
}

# Check all of the parts to verify they have all their required inputs connected.
for($ii=0;$ii<$partCnt;$ii++)
{
        if ( -e "lib/$partType[$ii].pm") {
           $mask = ("CDS::" . $partType[$ii] . "::checkInputConnect") -> ($ii);
           if ($mask eq "ERROR") { $kk ++; }
        }
}
close CONN_ERRORS;
if($kk > 0)
{
	print "***\n***ERROR: Found total of ** $kk ** parts with one or more inputs not connected.\n";
	my $emsg = "cat ";
	$emsg .= $compileMessageDir;
	$emsg .= $skeleton;
	$emsg .= "_partConnectErrors.log";
	system($emsg);
         die "\nSee $skeleton\_partConnectErrors.log file for details.\nA complete list of model part connections can be found in $skeleton\_partConnectionList.txt\n\n";
}

#//	
#// \n\n Start the process of producing the code sequencing. 
print "Found $adcCnt ADC modules part is $adcPartNum[0]\n";
die "***ERROR: At least one ADC part is required in the model\n" if ($adcCnt < 1);
print "Found $dacCnt DAC modules part is $dacPartNum[0]\n";
print "Found $boCnt Binary modules part is $boPartNum[0]\n";

($::maxAdcModules, $::maxDacModules) =
	CDS::Util::findDefine("src/include/drv/cdsHardware.h",
			"MAX_ADC_MODULES", "MAX_DAC_MODULES");

die "***ERROR: Too many ADC modules (MAX = $::maxAdcModules): ADC defined = $adcCnt\n" if ($adcCnt > $::maxAdcModules);
die "***ERROR: Too many DAC modules (MAX = $::maxDacModules): DAC defined = $dacCnt\n" if  ($dacCnt > $::maxDacModules);

#//	- Need to remove some parts from processing list, such as BUSS, DELAY, GROUND, as code is not produced for these parts.
#//		- Loop thru all of the parts within a subsystem.
for($ii=0;$ii<$subSys;$ii++)
{
	$partsRemaining = $subSysPartStop[$ii] - $subSysPartStart[$ii];
	$counter = 0;
	$ssCnt = 0;
 print "SUB $ii has $partsRemaining parts *******************\n";
	for($jj=$subSysPartStart[$ii];$jj<$subSysPartStop[$ii];$jj++)
	{
		if(($partType[$jj] eq "INPUT") || ($partType[$jj] eq "BUSS") || ($partType[$jj] eq "GROUND") || ($partType[$jj] eq "EpicsIn") || ($partType[$jj] eq "CONSTANT") || ($partType[$jj] eq "EzCaRead") || ($partType[$jj] eq "DELAY") || ($partType[$jj] eq "Gps") || ($partType[$jj] eq "StateWord") || ($partType[$jj] eq "ModelRate") || ($partType[$jj] eq "EXC"))
		{
			if(($partType[$jj] ne "DELAY"))
			{
				$partsRemaining --;
				$partUsed[$jj] = 1;
			}
			for($kk=0;$kk<$partOutCnt[$jj];$kk++)
			{
				
				#if(($partType[$jj] eq "BUSS") && ($partInputType[$jj][0] eq "Adc"))
				if(($partType[$jj] eq "BUSS"))
				{
					#print "BUSS FOUND ****  $xpartName[$jj] $seqNum[0][$counter] $partInputType[$jj][0]\n";
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
		{
			$partsRemaining --;
			$partUsed[$jj] = 1;
		}
	}
	print "Found $counter Inputs for subsystem $ii with $partsRemaining parts*********************************\n";
	$xx = 0;
	$ts = 1;
	#until(($partsRemaining < 1) || ($xx > 200))
	until($xx > 100)
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
#//		- Loop thru all of the parts at model top level.
for($ii=0;$ii<$nonSubCnt;$ii++)
{
			$xx = $nonSubPart[$ii];
	if(($partType[$xx] ne "BUSC") && ($partType[$xx] ne "FROM") &&($partType[$xx] ne "GOTO") && ($partType[$xx] ne "BUSS")
        && ($partType[$xx] ne "Adc") && ($partUsed[$xx] != 1))
	{
		$searchPart[$partsRemaining] = $xx;
		$searchCnt ++;
		$partsRemaining ++;
		#print "Part num $xx $partName[$xx] is remaining\n";
	}
}
$subRemaining = $subSys;
$seqCnt = 0;

#//	-  Construct parts linked list \n
#
foreach $i (0 .. $subSys-1) {
	debug(0, "Subsystem $i ", $subSysName[$i]);
}

# First pass defines processing step 0
# It finds all input sybsystems
#//		- First pass

#//			- Determine if subsystem has all of its inputs from ADC; if so, it can be added to process list.
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
		 #print "Subsys $ii $subSysName[$ii] has all ADC inputs and can go $seqCnt\n";
		$subRemaining --;
	}
}
#print "Searching parts $searchCnt\n";
#//			- Check if top level parts have ADC connections and therefore can go first.
for($ii=0;$ii<$searchCnt;$ii++)
{
	$allADC = 1;
	$xx = $searchPart[$ii];
	if($partUsed[$xx] == 0)
	{
		for($jj=0;$jj<$partInCnt[$xx];$jj++)
		{
			if($partInputType[$xx][$jj] ne "Adc")

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
		}
	}
}
print "first pass done $partsRemaining $subRemaining\n";

#//		- Make 50 more passes through parts to complete linked list
# Second multiprocessing step
$numTries = 0;
until((($partsRemaining < 1) && ($subRemaining < 1)) || ($numTries > 60))
{
$numTries ++;
#//			- Continue through subsystem parts.
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
			}
		}
	}
#//			- Continue through top level parts.
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
				#print "Part $xx $xpartName[$xx] can go next\n";
				$partUsed[$xx] = 1;
				$partsRemaining --;
				$seqList[$seqCnt] = $xx;
				$seqType[$seqCnt] = "PART";
				$seqCnt ++;
			}
		}
	}
}
#//		- If still have parts that are not in linked list, then print error and exit.
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
		if(($partUsed[$ii] == 0) && ($partType[$ii] ne "BUSC") && ($partType[$ii] ne "BUSS"))
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

if(($partType[$xx] eq "TERM") || ($partType[$xx] eq "GROUND") || ($partType[$xx] eq "FROM") || ($partType[$xx] eq "GOTO") || ($partType[$xx] eq "EpicsIn") || ($partType[$xx] eq "CONSTANT") || ($partType[$xx] eq "Gps") || ($partType[$xx] eq "StateWord") || ($partType[$xx] eq "ModelRate") || ($partType[$xx] eq "EXC")) 
{$ftotal ++;}
		$processSeqType{$xpartName[$xx]} = $seqType[$ii];
		$processPartNum[$processCnt] = $xx;
		#print "******* $processCnt $processName[$processCnt] $processPartNum[$processCnt] $partType[$xx]\n";
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
			&& $pt ne "EpicsIn" && $pt ne "CONSTANT" && $pt ne "GOTO") {
			print $_, " ", $partType[$CDS::Parser::parts{$_}], "\n";
		}
	}
	print "Please check the model for missing links around these parts.\n";
	exit(1);
}

$fpartCnt = 0;
$inCnt = 0;

# END OF CODE PARSING and LINKED LIST GENERATION**************************************************************************
#//	
#// Now have all info necessary to produce the supporting code and text files. \n
#// Start the process of writing files.\n
#//	- Write Epics/real-time data structures to header file.
#//	- Write Epics structs common to all CDS front ends to the .h file.
print OUTH "#define MAX_FIR \t $firCnt\n";
print OUTH "#define MAX_FIR_POLY \t $firCnt\n\n";
print EPICS "\nEPICS CDS_EPICS dspSpace coeffSpace epicsSpace\n\n";
print EPICS "\n\n";
print OUTH "typedef struct CDS_EPICS_IN {\n";
print OUTH "\tint vmeReset;\n";
print EPICS "MOMENTARY FEC\_$dcuId\_VME_RESET epicsInput.vmeReset int ao 0\n";
print OUTH "\tint ipcDiagReset;\n";
print EPICS "MOMENTARY FEC\_$dcuId\_IPC_DIAG_RESET epicsInput.ipcDiagReset int ao 0\n";
print OUTH "\tint burtRestore;\n";
print EPICS "INVARIABLE FEC\_$dcuId\_BURT_RESTORE epicsInput.burtRestore int ai 0\n";
print OUTH "\tint dcuId;\n";
print OUTH "\tint diagReset;\n";
print EPICS "MOMENTARY FEC\_$dcuId\_DIAG_RESET epicsInput.diagReset int ao 0\n";
print OUTH "\tint overflowReset;\n";
print EPICS "MOMENTARY FEC\_$dcuId\_OVERFLOW_RESET epicsInput.overflowReset int ao 0\n";
print OUTH "\tint burtRestore_mask;\n";
print OUTH "\tint dacDuoSet_mask;\n";
print OUTH "\tint dacDuoSet;\n";
print EPICS "INVARIABLE FEC\_$dcuId\_DACDT_ENABLE epicsInput.dacDuoSet int bi 0 field(ZNAM,\"OFF\") field(ONAM,\"ON\")\n";
print OUTH "\tint pad1;\n";
if($diagTest > -1)
{
print OUTH "\tint bumpCycle;\n";
print OUTH "\tint bumpAdcRd;\n";
}
print OUTH "} CDS_EPICS_IN;\n\n";
print OUTH "typedef struct CDS_EPICS_OUT {\n";
print OUTH "\tint dcuId;\n";
my $subs = substr($skeleton,5);
if (0 == length($subs)) {
	print EPICS "OUTVARIABLE  DCU_ID epicsOutput.dcuId int ao 0\n";
} else {
	print EPICS "OUTVARIABLE  \U$subs\E_DCU_ID epicsOutput.dcuId int ao 0\n";
}
print OUTH "\tint tpCnt;\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_TP_CNT epicsOutput.tpCnt int ao 0\n";

print OUTH "\tint cpuMeter;\n";
$frate = $rate;
if($frate <= 15)
{
	$brate =  13;
	$mrate = 15;
} else {
	$frate =  $rate * .85;
	$brate = $frate;
	$mrate = $rate;
}
$cpuM = $site . ":FEC-" . $dcuId . "_CPU_METER";
print EPICS "OUTVARIABLE FEC\_$dcuId\_CPU_METER epicsOutput.cpuMeter int ao 0 field(HOPR,\"$mrate\") field(LOPR,\"0\") field(HIHI,\"$mrate\") field(HHSV,\"MAJOR\") field(HIGH,\"$brate\") field(HSV,\"MINOR\") field(EGU,\"usec\")\n";

print OUTH "\tint cpuMeterMax;\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_CPU_METER_MAX epicsOutput.cpuMeterMax int ao 0 field(HOPR,\"$mrate\") field(LOPR,\"0\") field(HIHI,\"$mrate\") field(HHSV,\"MAJOR\") field(HIGH,\"$brate\") field(EGU,\"usec\") field(HSV,\"MINOR\")\n";

print OUTH "\tint adcWaitTime;\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_ADC_WAIT epicsOutput.adcWaitTime int ao 0 field(HOPR,\"$mrate\") field(EGU,\"usec\") field(LOPR,\"0\")\n";

print OUTH "\tint adcWaitMin;\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_ADC_WAIT_MIN epicsOutput.adcWaitMin int ao 0 field(HOPR,\"$mrate\") field(EGU,\"usec\") field(LOPR,\"0\")\n";

print OUTH "\tint adcWaitMax;\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_ADC_WAIT_MAX epicsOutput.adcWaitMax int ao 0 field(HOPR,\"$mrate\") field(EGU,\"usec\") field(LOPR,\"0\")\n";

print OUTH "\tint timeErr;\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_TIME_ERR epicsOutput.timeErr int ao 0\n";

if ($edcu) {
print OUTH "\tint timeDiag;\n";
print EPICS "DUMMY FEC\_$dcuId\_TIME_DIAG int ai 0\n";
print OUTH "\tint daqByteCnt;\n";
print EPICS "DUMMY FEC\_$dcuId\_DAQ_BYTE_COUNT int ao 0 field(HOPR,\"4000\") field(LOPR,\"0\") field(HIHI,\"4000\") field(HHSV,\"MINOR\")\n";
print EPICS "DUMMY FEC\_$dcuId\_EDCU_CHAN_NOCON int ao 0 field(HOPR,\"4000\") field(LOPR,\"0\") field(HIHI,\"1\") field(HHSV,\"MAJOR\")\n";
print EPICS "DUMMY FEC\_$dcuId\_EDCU_CHAN_CONN int ao 0 field(HOPR,\"4000\") field(LOPR,\"0\") field(HIHI,\"1\") field(HHSV,\"MAJOR\")\n";
print EPICS "DUMMY FEC\_$dcuId\_EDCU_CHAN_CNT int ao 0 field(HOPR,\"4000\") field(LOPR,\"0\") field(HIHI,\"1\") field(HHSV,\"MAJOR\")\n";
print EPICS "DUMMY FEC\_$dcuId\_EDCU_DAQ_RESET int ao 0 field(HOPR,\"4000\") field(LOPR,\"0\") field(HIHI,\"1\") field(HHSV,\"MAJOR\")\n";
}elsif ($globalsdf) {
print OUTH "\tint timeDiag;\n";
print EPICS "DUMMY FEC\_$dcuId\_TIME_DIAG int ai 0\n";
print OUTH "\tint datadump;\n";
print EPICS "DUMMY FEC\_$dcuId\_GSDF_DUMP_DATA int ao 0\n";
print OUTH "\tint daqByteCnt;\n";
print EPICS "DUMMY FEC\_$dcuId\_DAQ_BYTE_COUNT int ao 0 field(HOPR,\"4000\") field(LOPR,\"0\") field(HIHI,\"4000\") field(HHSV,\"MINOR\")\n";
print EPICS "DUMMY FEC\_$dcuId\_GSDF_CHAN_NOCON int ao 0 field(HOPR,\"4000\") field(LOPR,\"0\") field(HIHI,\"1\") field(HHSV,\"MAJOR\")\n";
print EPICS "DUMMY FEC\_$dcuId\_GSDF_CHAN_CONN int ao 0 field(HOPR,\"4000\") field(LOPR,\"0\") field(HIHI,\"1\") field(HHSV,\"MAJOR\")\n";
print EPICS "DUMMY FEC\_$dcuId\_GSDF_CHAN_CNT int ao 0 field(HOPR,\"4000\") field(LOPR,\"0\") field(HIHI,\"1\") field(HHSV,\"MAJOR\")\n";
}elsif ($casdf) {
print OUTH "\tint timeDiag;\n";
print EPICS "DUMMY FEC\_$dcuId\_TIME_DIAG int ai 0\n";
print OUTH "\tint daqByteCnt;\n";
print EPICS "DUMMY FEC\_$dcuId\_DAQ_BYTE_COUNT int ao 0 field(HOPR,\"4000\") field(LOPR,\"0\") field(HIHI,\"4000\") field(HHSV,\"MINOR\")\n";
}else{
print OUTH "\tint timeDiag;\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_TIME_DIAG epicsOutput.timeDiag int ao 0\n";
print OUTH "\tint daqByteCnt;\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_DAQ_BYTE_COUNT epicsOutput.daqByteCnt int ao 0 field(HOPR,\"4000\") field(LOPR,\"0\") field(HIHI,\"4000\") field(HHSV,\"MINOR\")\n";
}

print OUTH "\tint diagWord;\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_DIAG_WORD epicsOutput.diagWord int ao 0\n";
print EPICS "\n\n";

print OUTH "\tint statAdc[$adcCnt];\n";
for($ii=0;$ii<$adcCnt;$ii++)
{
	print EPICS "OUTVARIABLE FEC\_$dcuId\_ADC_STAT_$ii epicsOutput.statAdc\[$ii\] int ao 0\n";
}

print OUTH "\tint overflowAdc[$adcCnt][32];\n";
for($ii=0;$ii<$adcCnt;$ii++)
{
	for($jj=0;$jj<32;$jj++)
	{
		print EPICS "OUTVARIABLE FEC\_$dcuId\_ADC_OVERFLOW_$ii\_$jj epicsOutput.overflowAdc\[$ii\]\[$jj\] int ao 0\n";
	}
}

print OUTH "\tint overflowAdcAcc[$adcCnt][32];\n";
for($ii=0;$ii<$adcCnt;$ii++)
{
	for($jj=0;$jj<32;$jj++)
	{
		print EPICS "OUTVARIABLE FEC\_$dcuId\_ADC_OVERFLOW_ACC_$ii\_$jj epicsOutput.overflowAdcAcc\[$ii\]\[$jj\] int ao 0\n";
	}
}

print OUTH "\tint statDac[$dacCnt];\n";
for($ii=0;$ii<$dacCnt;$ii++)
{
	print EPICS "OUTVARIABLE FEC\_$dcuId\_DAC_STAT_$ii epicsOutput.statDac\[$ii\] int ao 0\n";
}

print OUTH "\tint buffDac[$dacCnt];\n";
for($ii=0;$ii<$dacCnt;$ii++)
{
	print EPICS "OUTVARIABLE FEC\_$dcuId\_DAC_BUFF_$ii epicsOutput.buffDac\[$ii\] int ao 0\n";
}

print OUTH "\tint overflowDac[$dacCnt][16];\n";
for($ii=0;$ii<$dacCnt;$ii++)
{
	for($jj=0;$jj<16;$jj++)
	{
		print EPICS "OUTVARIABLE FEC\_$dcuId\_DAC_OVERFLOW_$ii\_$jj epicsOutput.overflowDac\[$ii\]\[$jj\] int ao 0\n";
	}
}

print OUTH "\tint overflowDacAcc[$dacCnt][16];\n";
for($ii=0;$ii<$dacCnt;$ii++)
{
	for($jj=0;$jj<16;$jj++)
	{
		print EPICS "OUTVARIABLE FEC\_$dcuId\_DAC_OVERFLOW_ACC_$ii\_$jj epicsOutput.overflowDacAcc\[$ii\]\[$jj\] int ao 0\n";
	}
}

print OUTH "\tint dacValue[$dacCnt][16];\n";
for($ii=0;$ii<$dacCnt;$ii++)
{
	for($jj=0;$jj<16;$jj++)
	{
		print EPICS "OUTVARIABLE FEC\_$dcuId\_DAC_OUTPUT_$ii\_$jj epicsOutput.dacValue\[$ii\]\[$jj\] int ao 0\n";
	}
}

print OUTH "\tint ovAccum;\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_ACCUM_OVERFLOW epicsOutput.ovAccum int ao 0\n";

print OUTH "\tint userTime;\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_USR_TIME epicsOutput.userTime int ao 0\n";

print OUTH "\tint ipcStat;\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_IPC_STAT epicsOutput.ipcStat int ao 0\n";

print OUTH "\tint fbNetStat;\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_FB_NET_STATUS epicsOutput.fbNetStat int ao 0\n";

# print OUTH "\tint daqByteCnt;\n";
# print EPICS "OUTVARIABLE FEC\_$dcuId\_DAQ_BYTE_COUNT epicsOutput.daqByteCnt int ao 0 field(HOPR,\"4000\") field(LOPR,\"0\") field(HIHI,\"4000\") field(HHSV,\"MINOR\")\n";

print OUTH "\tint dacEnable;\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_DAC_MASTER_STAT epicsOutput.dacEnable int ao 0\n";

print OUTH "\tint cycle;\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_CYCLE_CNT epicsOutput.cycle int ao 0\n";
print OUTH "\tint stateWord;\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_STATE_WORD_FE epicsOutput.stateWord int ao 0\n";
print OUTH "\tint epicsSync;\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_EPICS_SYNC epicsOutput.epicsSync int ao 0\n";

if($adcMaster > -1)
{
print OUTH "\tint dtTime;\n";
print OUTH "\tint dacDtTime;\n";
print OUTH "\tint irigbTime;\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_DUOTONE_TIME epicsOutput.dtTime int ao 0\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_DUOTONE_TIME_DAC epicsOutput.dacDtTime int ao 0\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_IRIGB_TIME epicsOutput.irigbTime int ao 0 field(HIHI,\"24\") field(HHSV,\"MAJOR\") field(HIGH,\"18\") field(HSV,\"MINOR\") field(LOW,\"5\") field(LSV,\"MAJOR\")\n";
}
print OUTH "\tint awgStat;\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_AWGTPMAN_STAT epicsOutput.awgStat int ao 0\n";
print OUTH "\tint gdsMon[32];\n";
for($ii=0;$ii<32;$ii++)
{
	print EPICS "OUTVARIABLE FEC\_$dcuId\_GDS_MON_$ii epicsOutput.gdsMon\[$ii\] int ao 0\n";
}
print OUTH "\tint startgpstime;\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_START_GPS epicsOutput.startgpstime int ao 0\n";
print OUTH "\tint fe_status;\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_FE_STATUS epicsOutput.fe_status int ao 0\n";

print EPICS "DUMMY FEC\_$dcuId\_UPTIME_DAY int ao 0\n";
print EPICS "DUMMY FEC\_$dcuId\_UPTIME_HOUR int ao 0\n";
print EPICS "DUMMY FEC\_$dcuId\_UPTIME_MINUTE int ao 0\n";


# The following code is in solely for automated testing.
if($diagTest > -1)
{
print OUTH "\tint timingTest[10];\n";
print EPICS "MOMENTARY FEC\_$dcuId\_BUMP_CYCLE epicsInput.bumpCycle int ao 0\n";
print EPICS "MOMENTARY FEC\_$dcuId\_BUMP_ADC epicsInput.bumpAdcRd int ao 0\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_TIMING_TEST_64K epicsOutput.timingTest[0] int ao 0\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_TIMING_TEST_32K epicsOutput.timingTest[1] int ao 0\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_TIMING_TEST_16K epicsOutput.timingTest[2] int ao 0\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_TIMING_TEST_04K epicsOutput.timingTest[3] int ao 0\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_TIMING_TEST_02K epicsOutput.timingTest[4] int ao 0\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_TIMING_TEST_64KA epicsOutput.timingTest[5] int ao 0\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_TIMING_TEST_32KA epicsOutput.timingTest[6] int ao 0\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_TIMING_TEST_16KA epicsOutput.timingTest[7] int ao 0\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_TIMING_TEST_04KA epicsOutput.timingTest[8] int ao 0\n";
print EPICS "OUTVARIABLE FEC\_$dcuId\_TIMING_TEST_02KA epicsOutput.timingTest[9] int ao 0\n";
}

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
#//		- Make call to <em>::printHeaderStruct</em> in supporting lib/.pm part files for app specific data structure.
my $header_masks;
for($ii=0;$ii<$partCnt;$ii++)
{
	if (($cdsPart[$ii]) && (($partType[$ii] eq "IPCx") || ($partType[$ii] eq "FiltCtrl") || ($partType[$ii] eq "FiltCtrl2") || ($partType[$ii] eq "EpicsBinIn") || ($partType[$ii] eq "DacKill") || ($partType[$ii] eq "DacKillIop") || ($partType[$ii] eq "DacKillTimed") || ($partType[$ii] eq "EpicsMomentary") || ($partType[$ii] eq "EpicsCounter") || ($partType[$ii] eq "Word2Bit") || ($partType[$ii] eq "EpicsOutLong") )) {

	  $masks = ("CDS::" . $partType[$ii] . "::printHeaderStruct") -> ($ii);
	  if (length($masks) > 5) {
	  	$header_masks .= $masks;
	  }
	}
}
for($ii=0;$ii<$partCnt;$ii++)
{
		if (($cdsPart[$ii]) && ($partType[$ii] ne "IPCx") && ($partType[$ii] ne "FiltCtrl") && ($partType[$ii] ne "FiltCtrl2") && ($partType[$ii] ne "EpicsBinIn") && ($partType[$ii] ne "DacKill") && ($partType[$ii] ne "DacKillIop") && ($partType[$ii] ne "DacKillTimed") && ($partType[$ii] ne "EpicsMomentary") && ($partType[$ii] ne "EpicsCounter") && ($partType[$ii] ne "EzCaRead") && ($partType[$ii] ne "EzCaWrite") && ($partType[$ii] ne "EpicsOutLong")) {
		  $masks = ("CDS::" . $partType[$ii] . "::printHeaderStruct") -> ($ii);
		  if (length($masks) > 5) {
			$header_masks .= $masks;
		  }
		}
}
for($ii=0;$ii<$partCnt;$ii++)
{
		if (($cdsPart[$ii]) && (($partType[$ii] eq "EzCaRead") || ($partType[$ii] eq "EzCaWrite"))) {
		  $masks = ("CDS::" . $partType[$ii] . "::printHeaderStruct") -> ($ii);
		  if (length($masks) > 5) {
			$header_masks .= $masks;
		  }
		}
}

	die "Unspecified \"host\" parameter in cdsParameters block\n" if ($targetHost eq "localhost");

	# Print masks
	#for($ii=0;$ii<$partCnt;$ii++)
	#{
		#if ($cdsPart[$ii] &&  $partType[$ii] eq "EpicsIn" ) {
			#print ::OUTH "\tchar $::xpartName[$ii]_mask;\n";
		#}
	#}
	print OUTH $header_masks;
	print OUTH "} \U$systemName;\n\n";

	print OUTH "\n\n#define MAX_MODULES \t $filtCnt\n";
	$filtCnt *= 10;
	print OUTH "#define MAX_FILTERS \t $filtCnt\n\n";
	print OUTH "typedef struct CDS_EPICS {\n";
	print OUTH "\tCDS_EPICS_IN epicsInput;\n";
	print OUTH "\tCDS_EPICS_OUT epicsOutput;\n";
	print OUTH "\t\U$systemName \L$systemName;\n";
	print OUTH "} CDS_EPICS;\n";
	print OUTH "\#define TARGET_HOST_NAME $targetHost\n";
	if ($requireIOcnt) {
		print OUTH "\#define TARGET_ADC_COUNT $adcCnt\n";
		for (0 .. $dacCnt-1) {
			if($dacType[$_] eq "GSC_16AO16") {
				$dac16Cnt ++;
			}elsif($dacType[$_] eq "GSC_20AO8") {
			        $dac20Cnt ++;
			} else {
				$dac18Cnt ++;
			}
		}
		print OUTH "\#define TARGET_DAC16_COUNT $dac16Cnt\n";
		print OUTH "\#define TARGET_DAC18_COUNT $dac18Cnt\n";
		print OUTH "\#define TARGET_DAC20_COUNT $dac20Cnt\n";
	} else {
		if($virtualiop == 0 and $adcMaster == 1) {
			print OUTH "\#define TARGET_ADC_COUNT 1\n";
		} else {
			print OUTH "\#define TARGET_ADC_COUNT 0\n";
		}
		print OUTH "\#define TARGET_DAC16_COUNT 0\n";
		print OUTH "\#define TARGET_DAC18_COUNT 0\n";
		print OUTH "\#define TARGET_DAC20_COUNT 0\n";
	}
    if ($edcu) {
        $no_daq = 1;
	    print OUTH "\#define TARGET_RT_FLAG 0\n";
    } else {
	    print OUTH "\#define TARGET_RT_FLAG 1\n";
    }
	print OUTH "\#define TARGET_DAQ_FLAG $no_daq\n";
	if ($specificCpu > -1) {
  		print OUTH "\#define TARGET_CPU $specificCpu\n";
	} else {
  		print OUTH "\#define TARGET_CPU 1\n";
	}
	print OUTH "\#define TARGET_METER $cpuM\n";
	print OUTH "\#endif\n";

	#//	- Write EPICS database info file for later use by fmseq.pl in generating EPICS code/database.
	#//		- Write info common to all models.
	print EPICS "DAQVAR $dcuId\_LOAD_CONFIG int ao 0\n";
	print EPICS "DAQVAR $dcuId\_CHAN_CNT int ao 0\n";
	print EPICS "DAQVAR $dcuId\_EPICS_CHAN_CNT int ao 0\n";
	print EPICS "DAQVAR $dcuId\_TOTAL int ao 0\n";
	print EPICS "DAQVAR $dcuId\_MSG int ao 0\n";
	print EPICS "DAQVAR  $dcuId\_DCU_ID int ao 0\n";


	#Load EPICS I/O Parts
	#//		- Write part specific info by making call to <em>/lib/.pm</em> part support module.
	for($ii=0;$ii<$partCnt;$ii++)
	{
		if ($cdsPart[$ii]) {
		  ("CDS::" . $partType[$ii] . "::printEpics") -> ($ii);
		}
	}
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

	$pref = uc(substr($skeleton, 5, length $skeleton));

	for($ii = 0; $ii < $dacCnt; $ii++) {
	   for($jj = 0; $jj < 16; $jj++) {
		if ($pref) {
		  $dac_testpoint_names .= "${pref}_";
		}
		$dac_testpoint_names .= "MDAC". $ii . "_TP_CH" . $jj . " ";
	   }
	}

	#//		- Add extra test point channels.
	print EPICS "test_points $dac_testpoint_names $::extraTestPoints\n";
	#print EPICS "test_points $::extraTestPoints\n";
	if ($::extraExcitations) {
		print EPICS "excitations $::extraExcitations\n";
	}
	#//		- for casdf signal that the CA version of the SDF database sould be generated
	if ($::casdf) {
		print EPICS "sdf_flavor_ca\n";
	}
	#//		- Add GDS info.
    $gdsrate = get_freq();
    if($gdsrate > 524768) {
        $gdsrate = 524768;
    }
	print EPICS "gds_config $gdsXstart $gdsTstart 1250 1250 $gdsNodeId $site $gdsrate $dcuId $ifoid\n";
	print EPICS "\n\n";
	close EPICS;

	#//	- Write C source code file.
	# Start process of writing .c file. **********************************************************************
	#//		- Standard opening information.
    print OUT <<END;
    // ******* This is a computer generated file *******
    // ******* DO NOT HAND EDIT ************************

    #include "fe.h"

END

    # Define the code cycle rate
    print OUT "#define FE_RATE\t$gdsrate\n";
    if($gdsrate > 65536) {
        print OUT "#define IPC_RATE\t65536\n\n";
    } elsif ($ipcrate > 0) {
        print OUT "#define IPC_RATE\t$ipcrate\n\n";
        $ipccycle = $gdsrate / $ipcrate;
	} else {
        print OUT "#define IPC_RATE\t$gdsrate\n\n";
    }


	@adcCardNum;
	@dacCardNum;
	$adcCCtr=0;
	$dacCCtr=0;
	# Hardware configuration
	#//		- PCIe Hardware information.
	print OUT "/* Hardware configuration */\n";
	print OUT "CDS_CARDS cards_used[] = {\n";
	for (0 .. $adcCnt-1) {
		print OUT "\t{", $adcType[$_], ",", $adcNum[$_], "},\n";
		$adcCardNum[$adcCCtr] = substr($adcNum[$_],0,1);
		$adcCCtr ++;
	}
	for (0 .. $dacCnt-1) {
		print OUT "\t{", $dacType[$_], ",", $dacNum[$_], "},\n";
		$dacCardNum[$dacCCtr] = substr($dacNum[$_],0,1);
		$dacCCtr ++;
	}
	for (0 .. $boCnt-1) {
		print OUT "\t{", $boType[$_], ",", $boNum[$_], "},\n";
	}
	print OUT "};\n\n";


	#//		- Includes for User defined function calls.
	# Group includes for function calls at beginning
	for ($ii = 0; $ii < $partCnt; $ii++) {
	   if ($cdsPart[$ii]) {
	      if ($partType[$ii] eq "FunctionCall") {
		 ("CDS::" . $partType[$ii] . "::printFrontEndVars") -> ($ii);
	      }
	   }
	}

	#//		- Variable definitions.
	printVariables();

	#//		- Main user code subroutine call opening information.
	#//			- <em>feCode(
	#//				int cycle, double dWord[][32],double dacOut[][16],
	#//				FILT_MOD *dsp_ptr, COEF *dspCoeff, CDS_EPICS *pLocalEpics,int feInit)</em>
	print OUT <<END;

int feCode(int cycle, 
			  double dWord[][32],
			  double dacOut[][16],
              FILT_MOD *dsp_ptr,      /* Filter Mod variables */
              COEF *dspCoeff,         /* Filter Mod coeffs */
              CDS_EPICS *pLocalEpics, /* EPICS variables */
              int feInit)     /* Initialization flag */
{

int ii, dacFault;

if(feInit)
{
END
;

	#//		- Variable declarations and initializations in subroutine which take place on first call at runtime.
	#//			- For most parts, added by call to <em>::frontEndInitCode</em> in /lib/.pm support module.
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
	print OUT "// Enabling DAC outs for those who don't have DAC KILL WD \n";
	print OUT "dacFault = 1;\n";

	#//		- Main processing thread.
	# IPCx PART CODE
	#
	#//			- All IPCx data receives are to occur first in the processing loop
	#
	if ($ipcxCnt > 0 ) {
	   #print OUT "\ncommData3Receive(myIpcCount, ipcInfo, timeSec , cycle);\n\n";
	   print OUT "\nif((cycle % UNDERSAMPLE) == 0)commData3Receive(myIpcCount, ipcInfo, timeSec , (cycle / UNDERSAMPLE));\n\n";
	}
	# END IPCx PART CODE

	#print "*****************************************************\n";

	  &printSubsystem(".");

	sub printSubsystem {
my ($subsys) = @_;

$ts = 0;
$xx = 0;
$do_print = 0;


#//			- Produce code for all parts in the linked list.
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
	$inCnt = $partInCnt[$mm];
	for($qq=0;$qq<$inCnt;$qq++)
	{
		$indone = 0;
		if ( -e "lib/$partInputType[$mm][$qq].pm" ) {
		  require "lib/$partInputType[$mm][$qq].pm";
	  	  $fromExp[$qq] = ("CDS::" . $partInputType[$mm][$qq] . "::fromExp") -> ($mm, $qq);
	    	  $indone = $fromExp[$qq] ne "";
		}

		if($indone == 0)
		{
			$from = $partInNum[$mm][$qq]; #part number for input $qq
			$fromExp[$qq] = "\L$xpartName[$from]";
		}

		# Avoid a bunch of ground signals and combine into one.
                if ($fromExp[$qq] =~ /ground/)  {
                   $fromExp[$qq] = "ground";
                }
	}

	# Create the FE code for this part.
        if ( -e "lib/$partType[$mm].pm" ) {
	  	  print OUT ("CDS::" . $partType[$mm] . "::frontEndCode") -> ($mm);
	}	

	# ******** GROUND INPUT ********************************************************************
	if(($partType[$mm] eq "GROUND") && ($partUsed[$mm] == 0))
	{
	   #print "Found GROUND $xpartName[$mm] in loop\n";
	}

	# ******** DELAY ************************************************************************
	if($partType[$mm] eq "DELAY")
	{
		$calcExp = "\L$xpartName[$mm]";
		$calcExp .= " = ";
		$calcExp .= $fromExp[0];
		$calcExp .= ";\n";
		$unitDelayCode .= "$calcExp";
	}

print OUT "\n";
}
}

#//			- Add all UNIT DELAY part code.
print OUT "    // Unit delays\n";
print OUT "$unitDelayCode";
#//			- Add all IPC Output code.
print OUT "    // All IPC outputs\n";
print OUT "    if (_ipc_shm != 0) {\n";
print OUT "$ipcOutputCode";
print OUT "    }\n";
print OUT "$feTailCode";

# IPCx PART CODE
# The actual sending of IPCx data is to occur
# as the last step of the processing loop
#
if ($ipcxCnt > 0) {
   print OUT "      if(!cycle && pLocalEpics->epicsInput.ipcDiagReset) pLocalEpics->epicsInput.ipcDiagReset = 0;\n";

   if($ipccycle > 0) {
   print OUT "\n    if((cycle % $ipccycle) == 0) commData3Send(myIpcCount, ipcInfo, timeSec, (cycle / $ipccycle));\n\n";
   } else {
   print OUT "\n    if((cycle % UNDERSAMPLE) == 0) commData3Send(myIpcCount, ipcInfo, timeSec, (cycle / UNDERSAMPLE));\n\n";
    }
}
# END IPCx PART CODE

print OUT "  }\n";
print OUT "  return(dacFault);\n\n";
print OUT "}\n";
if(($remoteGpsPart != 0) && ($remoteGPS != 0))
{
print "RGPS DIAG **********************************\n";
print "\tPart number is $remoteGpsPart\n";
	print OUT "unsigned int remote_time(CDS_EPICS *pLocalEpics) {\n\treturn ";
	$calcExp = ("CDS::EzCaRead::remoteGps") -> ($remoteGpsPart);
	print "$calcExp \n";
	print OUT "$calcExp;\n}\n";

}

if($virtualiop == 1) {
print OUT "#include \"$rcg_src_dir/src/fe/controllerVirtual.c\"\n";
} elsif ($virtualiop == 2) {
  print OUT "#include \"$rcg_src_dir/src/fe/controllerIop.c\"\n";
} elsif ($virtualiop == 3) {
print OUT "#include \"$rcg_src_dir/src/fe/controllerLR.c\"\n";
} elsif ($virtualiop == 4) {
print OUT "#include \"$rcg_src_dir/src/fe/controllerCymac.c\"\n";
} else {
  if($adcMaster == 1) {
  	print OUT "#include \"$rcg_src_dir/src/fe/controllerIop.c\"\n";
  } else {
  	print OUT "#include \"$rcg_src_dir/src/fe/controllerApp.c\"\n";
  }
}


if($partsRemaining != 0) {
	print WARNINGS "WARNING -- NOT ALL PARTS CONNECTED !!!!\n";
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


close OUTD;
close WARNINGS;

#// Write the User Space Code Here
open(OUT,"<./".$cFile) || die "cannot open c file for reading $cFile";
open(OUT2,">./".$cFile2) || die "cannot open c file for writing $cFile2";
while(my $line = <OUT>) {
	if(index($line,"fe.h") != -1) {
		print OUT2 "#include \"feuser.h\" \n";
		print OUT2 "#include \<stdbool.h\> \n";
	} 
	elsif(index($line,"COMMDATA_INLINE") != -1 && $adcMaster != 1) {
		print OUT2 "#define COMMDATA_USP\n";
	}
	elsif(index($line,"controller") != -1 && $adcMaster != 1) {
		print OUT2 "#include \"$rcg_src_dir/src/fe/controllerAppUser.c\"\n";
	}
	elsif(index($line,"controller") != -1 && $adcMaster == 1) {
		print OUT2 "#include \"$rcg_src_dir/src/fe/controllerIopUser.c\"\n";
	} else {
		print OUT2 "$line";
	}
}
close OUT;
close OUT2;

#//	- Write C code MAKEFILE
createCmakefile();
createUsermakefile();

#//	- Write EPICS code MAKEFILE
createEpicsMakefile();

mkpath $configFilesDir, 0, 0755;


#//	- Write FOTON filter definition file.
# Create Foton filter file (with header)
$jj = $filtCnt / 40;
$jj ++;
#print OUTG "$jj lines to print\n";
my $filtFile = $configFilesDir . "/$site" . uc($skeleton) . "\.txt";
open(OUTG, ">" . $filtFile) || die "cannot open  $filtFile file for writing";
print OUTG "# FILTERS FOR ONLINE SYSTEM\n".
	"#\n".
	"# Computer generated file: DO NOT EDIT\n".
	"# SAMPLING RATE " . get_freq() . "\n" .
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

my $sampling_rate = get_freq();
$jj  = $filtCnt / 10;

for($ii=0;$ii<$jj;$ii++) {
print OUTG <<EOF;
################################################################################
#### $filterName[$ii]
#################################################################################
## SAMPLING $filterName[$ii] $sampling_rate
####                                                                          ###
#

EOF
}
close OUTG;

#//	- Generate standard set of MEDM screen files.
# Take care of generating Epics screens *****************************************************

# Used to develop sed command to allow Filter module screens to link generic FILTERALH screen.
@rcg_util_path = split('/', $ENV{"RCG_SRC_DIR"});
my $ffmedm = "";
foreach $i (@rcg_util_path) {
$ffmedm .= $i;
$ffmedm .= "\\/";
}

mkpath $epicsScreensDir, 0, 0755;
my $usite = uc $site;
my $lsite = lc $site;
my $sysname = "FEC";
my $medmDir = "\\/opt\\/rtcds\\/" . $location . "\\/" . $lsite . "\\/medm\\/" . $skeleton . "\\/";
$sed_arg = "s/SITE_NAME/$site/g;s/CONTROL_SYSTEM_SYSTEM_NAME/" . uc($skeleton) . "/g;s/SYSTEM_NAME/" . uc($sysname) . "/g;s/GDS_NODE_ID/" . $gdsNodeId . "/g;";
$sed_arg .= "s/LOCATION_NAME/$location/g;";
$sed_arg .= "s/DCU_NODE_ID/$dcuId/g;";
$sysname = uc($skeleton);
$sed_arg .= "s/FBID/$sysname/g;";
$sed_arg .= "s/MEDMDIR/$skeleton/g;";
$sed_arg .= "s/IFO_LC/$lsite/g;";
$sed_arg .= "s/MODEL_LC/$skeleton/g;";
$sed_arg .= "s/TARGET_MEDM/$medmDir/g;";
$sed_arg .= "s/RCGDIR/$ffmedm/g;";
$sitelc = lc($site);
$mxpt = 215;
$mypt = 172;
$mbxpt = 32 + $mxpt;
$mbypt = $mypt + 1;
$dacMedm = 0;
$totalMedm = $adcCnt + $dacCnt;
print "Found $adcCnt ADC modules part is $adcPartNum[0]\n";
print "Found $dacCnt DAC modules part is $dacPartNum[0]\n";

#//		-  Generate Guardian Alarm Monitor Screen
system("cp $rcg_src_dir/src/epics/util/ALARMS.adl ALARMS.adl");
system("cat ALARMS.adl | sed '$sed_arg' > $epicsScreensDir/$sysname" . "_ALARM_MONITOR.adl");
system("cp $rcg_src_dir/src/epics/util/BURT_RESTORE.adl BURT_RESTORE.adl");
system("cat BURT_RESTORE.adl | sed '$sed_arg' > $epicsScreensDir/$sysname" . "_BURT_RESTORE.adl");
system("cp $rcg_src_dir/src/epics/util/SDF_RESTORE.adl SDF_RESTORE.adl");
system("cat SDF_RESTORE.adl | sed '$sed_arg' > $epicsScreensDir/$sysname" . "_SDF_RESTORE.adl");
system("cp $rcg_src_dir/src/epics/util/SDF_SAVE.adl SDF_SAVE.adl");
system("cat SDF_SAVE.adl | sed '$sed_arg' > $epicsScreensDir/$sysname" . "_SDF_SAVE.adl");
if ($::casdf) {
system("cp $rcg_src_dir/src/epics/util/SDF_TABLE_CA.adl SDF_TABLE.adl");
} else {
system("cp $rcg_src_dir/src/epics/util/SDF_TABLE.adl SDF_TABLE.adl");
}
system("cat SDF_TABLE.adl | sed '$sed_arg' > $epicsScreensDir/$sysname" . "_SDF_TABLE.adl");

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
		if ($partType[$cur_part_num] eq "RampMuxMatrix") {
                        # RampMuxMatrix uses mux and demux parts
                        $outcnt = $partOutputs[$partOutNum[$cur_part_num][0]];
                        $incnt = $partInCnt[$partInNum[$cur_part_num][0]];
                }
		my $basename = $partName[$cur_part_num];
		if ($partSubName[$cur_part_num] ne "") {
			$basename = $partSubName[$cur_part_num] . "_" . $basename;
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
		  my $filtername1 = $usite . $basename;
		  if ($partType[$cur_part_num] eq "FiltMuxMatrix") {
		    my $subDirName = "$epicsScreensDir/$usite" . "$basename";
		    mkdir $subDirName;
		    system("$rcg_src_dir/src/epics/util/mkfiltmatrix.pl --cols=$incnt --collabels=$collabels --rows=$outcnt --rowlabels=$rowlabels --chanbase=$basename1 --filterbase=$filtername1 > $epicsScreensDir/$usite" . $basename . ".adl");
		    for ($row = 1; $row < $outcnt+1; $row ++) {
		      for ($col = 1; $col < $incnt+1; $col ++) {
			my $filt_name = "$partName[$cur_part_num]" . "_" . "$row" . "_" . "$col";
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
			$sargs .= "s/RCGDIR/$ffmedm/g;";
			$sargs .= "s/DCU_NODE_ID/$dcuId/g";
			system("cat $rcg_src_dir/src/epics/util/FILTER.adl | sed '$sargs' > $subDirName/$usite" . $filt_name . ".adl");
		      }
		    }
		  } elsif ($partType[$cur_part_num] eq "RampMuxMatrix") {
			system("$rcg_src_dir/src/epics/util/mkrampmatrix.pl --cols=$incnt --collabels=$collabels --rows=$outcnt --rowlabels=$rowlabels --chanbase=$basename1 > $epicsScreensDir/$usite" . $basename . ".adl");
		  } else {
		    system("$rcg_src_dir/src/epics/util/mkmatrix.pl --cols=$incnt --collabels=$collabels --rows=$outcnt --rowlabels=$rowlabels --chanbase=$basename1 > $epicsScreensDir/$usite" . $basename . ".adl");
		  }

	  
		} else {
		  $sysname = substr($sysname, 2, 3);
		  my $basename1 = $usite . ":" .$sysname ."-" . $basename . "_";
		  my $filtername1 = $usite . $sysname . "_" . $basename;
		  #print "Matrix $basename $incnt X $outcnt\n";
		  if ($partType[$cur_part_num] eq "FiltMuxMatrix") {
		    my $subDirName = "$epicsScreensDir/$usite" . "$sysname" . "_" . "$basename";
		    mkdir $subDirName;
		    system("$rcg_src_dir/src/epics/util/mkfiltmatrix.pl --cols=$incnt --collabels=$collabels --rows=$outcnt --rowlabels=$rowlabels --chanbase=$basename1 --filterbase=$filtername1 > $epicsScreensDir/$usite$sysname" . "_" . $basename . ".adl");
                    for ($row = 1; $row < $outcnt+1; $row ++) {
		      for ($col = 1; $col < $incnt+1; $col ++) {
			my $filt_name = "$partName[$cur_part_num]" . "_" . "$row" . "_" . "$col";
			print "FILTER Part $filt_name $partName[$cur_part_num] $partType[$cur_part_num] input partInput=$partInput[$cur_part_num][0] type='$partInputType[$cur_part_num][0]' \n";
			if ($partSubName[$cur_part_num] ne "") {
			    $filt_name = $partSubName[$cur_part_num] . "_" . $filt_name;
			}
			my $sargs;

			$sargs = $sed_arg . "s/FILTERNAME/$sysname-$filt_name/g;";
			$sargs .= "s/RCGDIR/$ffmedm/g;";
			$sargs .= "s/DCU_NODE_ID/$dcuId/g";
			system("cat $rcg_src_dir/src/epics/util/FILTER.adl | sed '$sargs' > $subDirName/$usite$sysname" . "_" . $filt_name . ".adl");
		      }
		    }
		  } elsif ($partType[$cur_part_num] eq "RampMuxMatrix") {
			system("$rcg_src_dir/src/epics/util/mkrampmatrix.pl --cols=$incnt --collabels=$collabels --rows=$outcnt --rowlabels=$rowlabels --chanbase=$basename1 > $epicsScreensDir/$usite$sysname" . "_" . $basename . ".adl");
		  } else {
		    system("$rcg_src_dir/src/epics/util/mkmatrix.pl --cols=$incnt --collabels=$collabels --rows=$outcnt --rowlabels=$rowlabels --chanbase=$basename1 > $epicsScreensDir/$usite$sysname" . "_" . $basename . ".adl");
		  }
		}
	}
	if ((($partType[$cur_part_num] =~ /^Filt/)
	     && (not ($partType[$cur_part_num] eq "FiltMuxMatrix")))
	    || ($partType[$cur_part_num] =~ /^InputFilt/)
	    || ($partType[$cur_part_num] =~ /^InputFilter1/)) {
		my $filt_name = $partName[$cur_part_num];
	if ($partInputType[$cur_part_num][0] eq "Adc") {
		#exit(1);
	}
		if ($partSubName[$cur_part_num] ne "") {
			$filt_name = $partSubName[$cur_part_num] . "_" . $filt_name;
		}
		my $sys_name = uc($skeleton);
		my $sargs;
		$sysname = uc($skeleton);
		if (is_top_name($filt_name)) {
			my $tfn = top_name_transform($filt_name);
			my $nsys = system_name_part($tfn);
			$sargs = "s/CONTROL_SYSTEM_SYSTEM_NAME/" . uc($skeleton) . "/g;";
			$sargs .= "s/SITE_NAME/$site/g;s/SYSTEM_NAME/" . $nsys . "/g;";
                        $sargs .= "s/LOCATION_NAME/$location/g;";
			$sargs .= "s/FILTERNAME/$tfn/g;";
			$sargs .= "s/RCGDIR/$ffmedm/g;";
			$sargs .= "s/DCU_NODE_ID/$dcuId/g";
			if ($partType[$cur_part_num] =~ /^InputFilter1/) {
				system("cat INPUT_FILTER1.adl | sed '$sargs' > $epicsScreensDir/$site" . $filt_name . ".adl");
			} elsif ($partType[$cur_part_num] =~ /^InputFilt/) {
				system("cat $rcg_src_dir/src/epics/util/INPUT_FILTER.adl | sed '$sargs' > $epicsScreensDir/$site" . $filt_name . ".adl");
			} elsif ($partType[$cur_part_num] =~ /^FiltCtrl2/) {
				system("cat $rcg_src_dir/src/epics/util/FILTER_CTRL_2.adl | sed '$sargs' > $epicsScreensDir/$site" . $filt_name . ".adl");
			} elsif ($partType[$cur_part_num] =~ /^FiltCtrl/) {
				system("cat $rcg_src_dir/src/epics/util/FILTER_CTRL.adl | sed '$sargs' > $epicsScreensDir/$site" . $filt_name . ".adl");
			} else {
				system("cat $rcg_src_dir/src/epics/util/FILTER.adl | sed '$sargs' > $epicsScreensDir/$site" . $filt_name . ".adl");
			}
		} else {
		  	$sys_name = substr($sys_name, 2, 3);
			$sargs = $sed_arg . "s/FILTERNAME/$sys_name-$filt_name/g;";
			$sargs .= "s/RCGDIR/$ffmedm/g;";
			$sargs .= "s/DCU_NODE_ID/$dcuId/g";
			if ($partType[$cur_part_num] =~ /^InputFilter1/) {
				system("cat INPUT_FILTER1.adl | sed '$sargs' > $epicsScreensDir/$sysname" . "_" . $filt_name . ".adl");
			} elsif ($partType[$cur_part_num] =~ /^InputFilt/) {
				system("cat $rcg_src_dir/src/epics/util/INPUT_FILTER.adl | sed '$sargs' > $epicsScreensDir/$sysname" . "_" . $filt_name . ".adl");
			} elsif ($partType[$cur_part_num] =~ /^FiltCtrl2/) {
				system("cat $rcg_src_dir/src/epics/util/FILTER_CTRL_2.adl | sed '$sargs' > $epicsScreensDir/$sysname" . "_" . $filt_name . ".adl");
			} elsif ($partType[$cur_part_num] =~ /^FiltCtrl/) {
				system("cat $rcg_src_dir/src/epics/util/FILTER_CTRL.adl | sed '$sargs' > $epicsScreensDir/$sysname" . "_" . $filt_name . ".adl");
			} else {
				system("cat $rcg_src_dir/src/epics/util/FILTER.adl | sed '$sargs' > $epicsScreensDir/$sysname" . "_" . $filt_name . ".adl");
			}
		}
	}
		  $sysname = uc($skeleton);
}
# ******************************************************************************************
#//		- GENERATE SORTED ADC LIST FILE
$adcFile = "./diags2.txt";
$adcFileSorted = $configFilesDir . "/adcListSorted\.txt";
system ("sort $adcFile -k 1,1n -k 2,2n > $adcFileSorted");

# ******************************************************************************************
#//		- GENERATE IPC SCREENS
	("CDS::IPCx::createIpcMedm") -> ($epicsScreensDir,$sysname,$usite,$dcuId,$medmTarget,$ipcxCnt);
# ******************************************************************************************
#//		- GENERATE GDS_TP SCREEN
if($daq2dc == 0) {
	require "lib/medmGenGdsTp.pm";
	my $medmTarget = "/opt/rtcds/$location/$lsite/medm";
	my $scriptTarget = "/opt/rtcds/$location/$lsite/chans/tmp/$sysname\.diff";
	my $scriptArgs = "-s $location -i $lsite -m $skeleton -d $dcuId &"; 
	("CDS::medmGenGdsTp::createGdsMedm") -> ($epicsScreensDir,$sysname,$usite,$dcuId,$medmTarget,$scriptTarget,$scriptArgs,$adcCnt,$dacCnt,$adcMaster,$virtualiop,@dacType);
}else {
	require "lib/medmGenGdsTp2dc.pm";
	my $medmTarget = "/opt/rtcds/$location/$lsite/medm";
	my $scriptTarget = "/opt/rtcds/$location/$lsite/chans/tmp/$sysname\.diff";
	my $scriptArgs = "-s $location -i $lsite -m $skeleton -d $dcuId &"; 
	("CDS::medmGenGdsTp2dc::createGdsMedm") -> ($epicsScreensDir,$sysname,$usite,$dcuId,$medmTarget,$scriptTarget,$scriptArgs,$adcCnt,$dacCnt,$adcMaster,@dacType);
}
	require "lib/medmGenStatus.pm";
	("CDS::medmGenStatus::createStatusMedm") -> ($epicsScreensDir,$sysname,$usite,$dcuId,$medmTarget,$scriptTarget,$scriptArgs);


# ******************************************************************************************
#//		- GENERATE ADC SCREENS
# Open the diags2.txt file, which contains list of ADC connections.
open(my $fh, $adcFile)
  or die "Could not open file";

# Need the 3 letter system name to complete EPICS channel names.
$sysname = uc($skeleton);
$sname = substr($sysname,2,3);
@adcScreen;
$ii = 0;
$jj = 0;
while (my $line = <$fh>) {
        chomp($line);
        my @word = split /\t/,$line;
        $ii = $word[0];
        $jj = $word[1];
	$adcSname = $site . "\:$sname-" . $word[2];
	if (is_top_name($word[2])) {
                  my $tn = top_name_transform($word[2]);
                  $adcSname = $site . "\:" . $tn;
        } 
	# If this is a Filter type part, need to add INMON to get EPICS channel 
	if (($word[3] =~ /^Filt/)
	     && (not ($word[3] eq "FiltMuxMatrix"))) {
		$adcSname .= "_INMON";
	}
	# Only place Filter and EpicsOut type parts in the list
	if (($word[3] =~ /^Filt/ && $word[3] ne "FiltMuxMatrix")
		|| $word[3] eq "EpicsOut") {
        	$adcScreen[$ii][$jj] = $adcSname;
	}
}
close($fg);

for($ii=0;$ii<$adcCnt;$ii++)
{
   ("CDS::Adc::createAdcMedm") -> ($epicsScreensDir,$sysname,$usite,$dcuId,$medmTarget,$ii,@adcScreen);
}
# ******************************************************************************************
#//		- GENERATE DAC SCREENS
for($ii=0;$ii<$dacCnt;$ii++)
{
   if ($dacType[$ii] eq "GSC_16AO16") {
      ("CDS::Dac::createDac16Medm") -> ($epicsScreensDir,$sysname,$usite,$dcuId,$medmTarget,$ii);
   } elsif ($dacType[$ii] eq "GSC_20AO8") {
      ("CDS::Dac20::createDac20Medm") -> ($epicsScreensDir,$sysname,$usite,$dcuId,$medmTarget,$ii);
   } else {
      ("CDS::Dac18::createDac18Medm") -> ($epicsScreensDir,$sysname,$usite,$dcuId,$medmTarget,$ii);
   }
}

#//	-  Print source file names into a file
#
open(OUT,">sources.\L$sysname\E") || die "cannot open \"sources.$sysname\" file for writing ";
print OUT join("\n", @sources), "\n";
close OUT;
close OUTH;

#//	
#// \n \b SUBROUTINES ******************************************************************************\n\n
#// \b sub \b remove_subsystem \n
#// Remove leading subsystems name \n\n
sub remove_subsystem {
        my ($s) = @_;
        return substr $s, 1 + rindex $s, "_";
}

#// \b sub \b debug \n
#// Print debug message \n
#// Example: \n
#// debug (0, "debug test: openBrace=$openBrace"); \n\n
#
sub debug {
  if ($dbg_level > shift @_) {
	print @_, "\n";
  }
}
#// \b sub \b get_freq \n
#// Determine user code sample rate \n\n
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
	} elsif ($rate == 4) {
		return 256*1024;
	} elsif ($rate == 2) {
		return 512*1024;
	} elsif ($rate == 1) {
		return 1024*1024;
	}
}

#// \b sub \b init_vars \n
#// Initialize global variables used by the model parser \n\n
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
$card2array[0] = 0;
$bo64Cnt = 0;
$bi64Cnt = 0;
$nonSubCnt = 0; # Total of non-sybsystem parts found in the model
$blockDescr[0] = undef;

# Keeps non-subsystem part numbers
$nonSubPart[0] = 0;	# $nonSubPart[0 .. $nonSubCnt]

$partCnt = 0;	# Total parts found in the simulink model

# Element is set to one for each CDS parts
$cdsPart[0] = 0;	# $cdsPart[0 .. $partCnt]

$ppFIR[0] = 0;          # Set to one for PPFIR filters

$biQuad[0] = 0;          # Set to one for biquad IIR filters

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

# Set if all filters are biquad
$allBiquad = 0;

# Set if doing direct DAC writed (no DMA)
$directDacWrite = 0;

# Set to disable zero padding DAC data
$noZeroPad = 0;

# Set if DAC writes with no FIFO preload
$optimizeIO = 0;

# Clear the part input and output counters
# This implies a maximum part count of 2000 per model.
for ($ii = 0; $ii < 2000; $ii++) {
  $partInCnt[$ii] = 0;
  $partOutCnt[$ii] = 0;
  $partInUsed[$ii] = 0;
}
}
#// \b sub \b createEpicsMakefile \n
#// Create the EPICS Makefile \n\n
sub createEpicsMakefile {
	open(OUTME,">./".$meFile) || die "cannot open EPICS Makefile file for writing";
	print OUTME "\n";
	print OUTME "# Define Epics system name. It should be unique.\n";
	print OUTME "TARGET = $skeleton";
	print OUTME "epics\n";
	print OUTME "\n";
	print OUTME "SRC = build/\$(TARGET)/";
	print OUTME "$skeleton";
	print OUTME "\.st\n";
	print OUTME "\n";
	if ($edcu) {
	print OUTME "SRC += $rcg_src_dir/src/epics/seq/edcu.c\n";
	}elsif ($globalsdf) {
	print OUTME "SRC += $rcg_src_dir/src/epics/seq/sdf_monitor.c\n";
	} else {
	print OUTME "SRC += $rcg_src_dir/src/epics/seq/main.c\n";
	}
	print OUTME "SRC += $rcg_src_dir/src/drv/rfm.c\n";
	print OUTME "SRC += $rcg_src_dir/src/drv/param.c\n";
	print OUTME "SRC += $rcg_src_dir/src/drv/crc.c\n";
	print OUTME "SRC += $rcg_src_dir/src/drv/fmReadCoeff.c\n";
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
	# The CA SDF build does not need a SEQUENCER added
	if ($casdf==0) {
	print OUTME "SEQ += \'";
	print OUTME "$skeleton";
	print OUTME ",(\"ifo=$site, site=$location, sys=\U$systemName\, \Lsysnum=$dcuId\, \Lsysfile=\U$skeleton \")\'\n";
	}
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
	print OUTME "EXTRA_CFLAGS += -DFE_HEADER=\\\"\L$skeleton.h\\\"\n";
	if ($edcu) {
	  print OUTME "EXTRA_CFLAGS += -DEDCU=1\n";
	  print OUTME "EXTRA_CFLAGS += -DNO_DAQ_IN_SKELETON=1\n";
	  print OUTME "SYNC_SRC = $edcusync\n";
	}
	if ($globalsdf) {
	  print OUTME "EXTRA_CFLAGS += -DEDCU=1\n";
	  print OUTME "EXTRA_CFLAGS += -DNO_DAQ_IN_SKELETON=1\n";
	}
	if ($casdf) {
	  print OUTME "EXTRA_CFLAGS += -DCA_SDF=1\n";
	  print OUTME "EXTRA_CFLAGS += -DUSE_SYSTEM_TIME=1\n";
	}
	print OUTME "\n";
	#print OUTME "LIBFLAGS += -lezca\n";
	if($systemName eq "sei" || $useFIRs)
	{
	print OUTME "EXTRA_CFLAGS += -DFIR_FILTERS\n";
	}
	print OUTME "include $rcg_src_dir/config/Makefile.linux\n";
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
}
#// \b sub \b createCmakefile \n
#// Generate the user C code Makefile  \n\n
sub createCmakefile{

system ("/bin/cp GNUmakefile  ../../fe/$skeleton");
open(OUTM,">./".$mFile) || die "cannot open Makefile file for writing";

print OUTM "# CPU-Shutdown Real Time Linux\n";
print OUTM "KBUILD_EXTRA_SYMBOLS=$rcg_src_dir/src/drv/ExtraSymbols.symvers\n";
print OUTM "KBUILD_EXTRA_SYMBOLS += $mbufsymfile\n";
print OUTM "KBUILD_EXTRA_SYMBOLS += $gpssymfile\n";
print OUTM "KBUILD_EXTRA_SYMBOLS += \$(PWD)/ModuleIOP.symvers\n";
print OUTM "ALL \+= user_mmap \$(TARGET_RTL)\n";
print OUTM "EXTRA_CFLAGS += -O -w -I../../include\n";

if($rate == 480) { print OUTM "EXTRA_CFLAGS += -DSERVO2K\n"; }
elsif($rate == 240) { print OUTM "EXTRA_CFLAGS += -DSERVO4K\n"; }
elsif($rate == 60) { print OUTM "EXTRA_CFLAGS += -DSERVO16K\n"; }
elsif($rate == 30) { print OUTM "EXTRA_CFLAGS += -DSERVO32K\n"; }
elsif($rate == 15) { print OUTM "EXTRA_CFLAGS += -DSERVO64K\n"; }
elsif($rate == 7) { print OUTM "EXTRA_CFLAGS += -DSERVO128K\n"; }
elsif($rate == 4) { print OUTM "EXTRA_CFLAGS += -DSERVO256K\n"; }
elsif($rate == 2) { print OUTM "EXTRA_CFLAGS += -DSERVO512K\n"; }
elsif($rate == 1) { print OUTM "EXTRA_CFLAGS += -DSERVO1024K\n"; }


print OUTM "EXTRA_CFLAGS += -D";
print OUTM "\U$skeleton";
print OUTM "_CODE\n";
print OUTM "EXTRA_CFLAGS += -DFE_SRC=\\\"\L$skeleton/\L$skeleton.c\\\"\n";
print OUTM "EXTRA_CFLAGS += -DFE_HEADER=\\\"\L$skeleton.h\\\"\n";
#print OUTM "EXTRA_CFLAGS += -DFE_PROC_FILE=\\\"\L${skeleton}_proc.h\\\"\n";

if($systemName eq "sei" || $useFIRs)
{
print OUTM "EXTRA_CFLAGS += -DFIR_FILTERS\n";
}
print OUTM "EXTRA_CFLAGS += -g\n";

if ($daq2dc) {
  print OUTM "EXTRA_CFLAGS += -DDUAL_DAQ_DC\n";
}
if ($remoteGPS) {
  print OUTM "EXTRA_CFLAGS += -DREMOTE_GPS\n";
}
if ($no_sync) {
  print OUTM "#Comment out to enable 1PPS synchronization\n";
  print OUTM "EXTRA_CFLAGS += -DNO_SYNC\n";
} else {
  print OUTM "#Uncomment to disable 1PPS signal sinchronization (channel 31 (last), ADC 0)\n";
  print OUTM "#EXTRA_CFLAGS += -DNO_SYNC\n";
}
if (0 == $dac_testpoint_names && 0 == $::extraTestPoints && 0 == $filtCnt) {
	print "Not compiling DAQ into the front-end\n";
	$no_daq = 1;
}
if ($edcu) {
    $no_daq = 1;
}
if ($no_daq) {
  print OUTM "#Comment out to enable DAQ\n";
  print OUTM "EXTRA_CFLAGS += -DNO_DAQ\n";
} else {
  print OUTM "#Uncomment to disable DAQ and testpoints\n";
  print OUTM "#EXTRA_CFLAGS += -DNO_DAQ\n";
}

# SHMEM_DAQ set as the default for RCG V2.8 - No longer support GM
  print OUTM "#Comment out to disable local frame builder connection\n";
  print OUTM "EXTRA_CFLAGS += -DSHMEM_DAQ\n";

# Use oversampling code if not 64K system
if($rate > 15) {
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
if ($adcMaster > -1) {
  print OUTM "EXTRA_CFLAGS += -DADC_MASTER\n";
  $modelType = "MASTER";
  if($diagTest > -1) {
  print OUTM "EXTRA_CFLAGS += -DDIAG_TEST\n";
  }
  if($adcclock > $adc_std_clock) {
  	$undersample = $adcclock/$adc_std_rate;
  } else {
  	$undersample = $adcclock/$adcrate;
  }
  print OUTM "EXTRA_CFLAGS += -DUNDERSAMPLE=$undersample\n";
  if($dacWdOverride > -1) {
  print OUTM "EXTRA_CFLAGS += -DDAC_WD_OVERRIDE\n";
  }
  if ($no_cpu_shutdown > 0) {
    print OUTM "EXTRA_CFLAGS += -DNO_CPU_SHUTDOWN\n";
  }
  # ADD DAC_AUTOCAL to IOPs
  print OUTM "EXTRA_CFLAGS += -DDAC_AUTOCAL\n";
} else {
  print OUTM "#Uncomment to run on an I/O Master \n";
  print OUTM "#EXTRA_CFLAGS += -DADC_MASTER\n";
}
if ($adcSlave > -1) {
  print OUTM "EXTRA_CFLAGS += -DADC_SLAVE\n";
  print OUTM "EXTRA_CFLAGS += -DUNDERSAMPLE=1\n";
  $modelType = "SLAVE";
} else {
  print OUTM "#Uncomment to run on an I/O slave process\n";
  print OUTM "#EXTRA_CFLAGS += -DADC_SLAVE\n";
}
if ($timeMaster > -1) {
  print OUTM "EXTRA_CFLAGS += -DTIME_MASTER=1\n";
} else {
  print OUTM "#Uncomment to build a time master\n";
  print OUTM "#EXTRA_CFLAGS += -DTIME_MASTER=1\n";
}
if ($timeSlave > -1 or $virtualiop == 2) {
  print OUTM "EXTRA_CFLAGS += -DTIME_SLAVE=1\n";
} else {
  print OUTM "#Uncomment to build a time slave\n";
  print OUTM "#EXTRA_CFLAGS += -DTIME_SLAVE=1\n";
}
if ($iopTimeSlave > -1) {
  print OUTM "EXTRA_CFLAGS += -DIOP_TIME_SLAVE=1\n";
} else {
  print OUTM "#Uncomment to build an IOP time slave\n";
  print OUTM "#EXTRA_CFLAGS += -DIOP_TIME_SLAVE=1\n";
}
if($rfm_via_pcie == 1) {
  print OUTM "EXTRA_CFLAGS += -DRFM_VIA_PCIE=1\n";
}
if($usezmq == 1) {
  print OUTM "EXTRA_CFLAGS += -DUSE_ZMQ=1\n";
}
if ($rfmTimeSlave > -1) {
  print OUTM "EXTRA_CFLAGS += -DRFM_TIME_SLAVE=1\n";
} else {
  print OUTM "#Uncomment to build an RFM time slave\n";
  print OUTM "#EXTRA_CFLAGS += -DRFM_TIME_SLAVE=1\n";
}
if ($flipSignals) {
  print OUTM "EXTRA_CFLAGS += -DFLIP_SIGNALS=1\n";
}
if ($pciNet > 0) {
        if (-e "$pciegenfile") {
          print OUTM "#Enable use of PCIe RFM Network Gen 2\n";
          print OUTM "DISDIR = /opt/srcdis\n";
          print OUTM "EXTRA_CFLAGS += -DOS_IS_LINUX=1 -D_KERNEL=1 -I\$(DISDIR)/src/IRM_GX/drv/src -I\$(DISDIR)/src/IRM_GX/drv/src/LINUX -I\$(DISDIR)/src/include -I\$(DISDIR)/src/include/dis -I\$(DISDIR)/src/COMMON/osif/kernel/include -I\$(DISDIR)/src/COMMON/osif/kernel/include/LINUX -DDOLPHIN_TEST=1  -DDIS_BROADCAST=0x80000000\n";
        print OUTM "EXTRA_CFLAGS += -DOS_IS_LINUX=1 -D_DIS_KERNEL_=1 -I\$(DISDIR)/src/IRM_GX/drv/src -I\$(DISDIR)/src/IRM_GX/drv/src/LINUX -I\$(DISDIR)/src/include -I\$(DISDIR)/src/include/dis -I\$(DISDIR)/src/COMMON/osif/kernel/include -I\$(DISDIR)/src/COMMON/osif/kernel/include/LINUX -DDOLPHIN_TEST=1  -DDIS_BROADCAST=0x80000000\n";
	} else {
          print OUTM "#Enable use of PCIe RFM Network Gen 1\n";
          print OUTM "DISDIR = /opt/srcdis\n";
          print OUTM "EXTRA_CFLAGS += -DOS_IS_LINUX=1 -D_KERNEL=1 -I\$(DISDIR)/src/IRM/drv/src -I\$(DISDIR)/src/IRM/drv/src/LINUX -I\$(DISDIR)/src/include -I\$(DISDIR)/src/include/dis -DDOLPHIN_TEST=1  -DDIS_BROADCAST=0x80000000\n";
        }
}
if ($specificCpu > -1) {
  print OUTM "#Comment out to run on first available CPU\n";
  print OUTM "EXTRA_CFLAGS += -DSPECIFIC_CPU=$specificCpu\n";
} else {
  print OUTM "#Uncomment to run on a specific CPU\n";
  print OUTM "#EXTRA_CFLAGS += -DSPECIFIC_CPU=2\n";
}

# Set BIQUAD as default starting RCG V2.8
  print OUTM "#Comment out to go back to old iir_filter calculation form\n";
  print OUTM "EXTRA_CFLAGS += -DALL_BIQUAD=1 -DCORE_BIQUAD=1\n";

if ($::directDacWrite) {
  print OUTM "EXTRA_CFLAGS += -DDIRECT_DAC_WRITE=1\n";
} else {
  print OUTM "#EXTRA_CFLAGS += -DDIRECT_DAC_WRITE=1\n";
}

if ($::noZeroPad) {
  print OUTM "EXTRA_CFLAGS += -DNO_ZERO_PAD=1\n";
} else {
  print OUTM "#EXTRA_CFLAGS += -DNO_ZERO_PAD=1\n";
}

if ($::optimizeIO) {
  print OUTM "EXTRA_CFLAGS += -DNO_DAC_PRELOAD=1\n";
} else {
  print OUTM "#EXTRA_CFLAGS += -DNO_DAC_PRELOAD=1\n";
}
  

if ($::rfmDma) {
  print OUTM "#Comment out to run with RFM DMA\n";
  print OUTM "#EXTRA_CFLAGS += -DRFM_DIRECT_READ=1\n";
} else {
  print OUTM "#Comment out to run with RFM DMA\n";
  print OUTM "EXTRA_CFLAGS += -DRFM_DIRECT_READ=1\n";
}

if ($::rfmDelay) {
  print OUTM "#Comment out to run without RFM Delayed by 1 cycle\n";
  print OUTM "EXTRA_CFLAGS += -DRFM_DELAY=1\n";
} else {
  print OUTM "#Clear comment to run with RFM Delayed by 1 cycle\n";
  print OUTM "#EXTRA_CFLAGS += -DRFM_DELAY=1\n";
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

print OUTM "EXTRA_CFLAGS += -DMODULE -DNO_RTL=1\n";
print OUTM "EXTRA_CFLAGS += -I\$(SUBDIRS)/../../include -I$rcg_src_dir/src/include\n";
print OUTM "EXTRA_CFLAGS += -ffast-math -m80387 -msse2 -fno-builtin-sincos -mpreferred-stack-boundary=4\n";

print OUTM "obj-m += $skeleton" . ".o\n";

print OUTM "\n";
close OUTM;
}

#// \b sub \b createUsermakefile \n
#// Generate the user C code Makefile  \n\n
sub createUsermakefile{

open(OUTM,">./".$mFile2) || die "cannot open Makefile file for writing";

print OUTM "# User Space Linux\n";
print OUTM "CFLAGS += -O -w -I../../include\n";
print OUTM "CFLAGS += -I/opt/mx/include\n";

if($rate == 480) { print OUTM "CFLAGS += -DSERVO2K\n"; }
elsif($rate == 240) { print OUTM "CFLAGS += -DSERVO4K\n"; }
elsif($rate == 60) { print OUTM "CFLAGS += -DSERVO16K\n"; }
elsif($rate == 30) { print OUTM "CFLAGS += -DSERVO32K\n"; }
elsif($rate == 15) { print OUTM "CFLAGS += -DSERVO64K\n"; }
elsif($rate == 7) { print OUTM "CFLAGS += -DSERVO128K\n"; }
elsif($rate == 4) { print OUTM "CFLAGS += -DSERVO256K\n"; }
elsif($rate == 2) { print OUTM "CFLAGS += -DSERVO512K\n"; }
elsif($rate == 1) { print OUTM "CFLAGS += -DSERVO1024K\n"; }

print OUTM "CFLAGS += -D";
print OUTM "\U$skeleton";
print OUTM "_CODE\n";
print OUTM "CFLAGS += -DFE_SRC=\\\"\L$skeleton/\L$skeleton.c\\\"\n";
print OUTM "CFLAGS += -DFE_HEADER=\\\"\L$skeleton.h\\\"\n";
#print OUTM "CFLAGS += -DFE_PROC_FILE=\\\"\L${skeleton}_proc.h\\\"\n";

print OUTM "EXTRA_CFLAGS += -g\n";

if ($daq2dc) {
  print OUTM "CFLAGS += -DDUAL_DAQ_DC\n";
}
if ($remoteGPS) {
  print OUTM "CFLAGS += -DREMOTE_GPS\n";
}
if ($no_sync) {
  print OUTM "#Comment out to enable 1PPS synchronization\n";
  print OUTM "CFLAGS += -DNO_SYNC\n";
} else {
  print OUTM "#Uncomment to disable 1PPS signal sinchronization (channel 31 (last), ADC 0)\n";
  print OUTM "#CFLAGS += -DNO_SYNC\n";
}
if (0 == $dac_testpoint_names && 0 == $::extraTestPoints && 0 == $filtCnt) {
	print "Not compiling DAQ into the front-end\n";
	$no_daq = 1;
}
if ($no_daq) {
  print OUTM "#Comment out to enable DAQ\n";
  print OUTM "CFLAGS += -DNO_DAQ\n";
} else {
  print OUTM "#Uncomment to disable DAQ and testpoints\n";
  print OUTM "#CFLAGS += -DNO_DAQ\n";
}

# SHMEM_DAQ set as the default for RCG V2.8 - No longer support GM
  print OUTM "#Comment out to disable local frame builder connection\n";
  print OUTM "CFLAGS += -DSHMEM_DAQ\n";

# Use oversampling code if not 64K system
if($rate > 15) {
  if ($no_oversampling) {
    print OUTM "#Uncomment to oversample A/D inputs\n";
    print OUTM "#CFLAGS += -DOVERSAMPLE\n";
    print OUTM "#Uncomment to interpolate D/A outputs\n";
    print OUTM "#CFLAGS += -DOVERSAMPLE_DAC\n";
  } else {
    print OUTM "#Comment out to stop A/D oversampling\n";
    print OUTM "CFLAGS += -DOVERSAMPLE\n";
    if ($no_dac_interpolation) {
    } else {
      print OUTM "#Comment out to stop interpolating D/A outputs\n";
      print OUTM "CFLAGS += -DOVERSAMPLE_DAC\n";
    }
  }
}
    print OUTM "CFLAGS += -DUNDERSAMPLE=1\n";
if ($dac_internal_clocking) {
  print OUTM "#Comment out to enable external D/A converter clocking\n";
  print OUTM "CFLAGS += -DDAC_INTERNAL_CLOCKING\n";
}
if ($adcMaster > -1) {
  print OUTM "CFLAGS += -DADC_MASTER\n";
  $modelType = "MASTER";
  if($diagTest > -1) {
  print OUTM "CFLAGS += -DDIAG_TEST\n";
  }
  if($dacWdOverride > -1) {
  print OUTM "CFLAGS += -DDAC_WD_OVERRIDE\n";
  }
  # ADD DAC_AUTOCAL to IOPs
  print OUTM "CFLAGS += -DDAC_AUTOCAL\n";
} else {
  print OUTM "#Uncomment to run on an I/O Master \n";
  print OUTM "#CFLAGS += -DADC_MASTER\n";
}
if ($adcSlave > -1) {
  print OUTM "CFLAGS += -DADC_SLAVE\n";
  $modelType = "SLAVE";
} else {
  print OUTM "#Uncomment to run on an I/O slave process\n";
  print OUTM "#CFLAGS += -DADC_SLAVE\n";
}
if ($timeMaster > -1) {
  print OUTM "CFLAGS += -DTIME_MASTER=1\n";
} else {
  print OUTM "#Uncomment to build a time master\n";
  print OUTM "#CFLAGS += -DTIME_MASTER=1\n";
}
if ($timeSlave > -1) {
  print OUTM "CFLAGS += -DTIME_SLAVE=1\n";
} else {
  print OUTM "#Uncomment to build a time slave\n";
  print OUTM "#CFLAGS += -DTIME_SLAVE=1\n";
}
if ($iopTimeSlave > -1) {
  print OUTM "CFLAGS += -DIOP_TIME_SLAVE=1\n";
} else {
  print OUTM "#Uncomment to build an IOP time slave\n";
  print OUTM "#CFLAGS += -DIOP_TIME_SLAVE=1\n";
}
if($rfm_via_pcie == 1) {
  print OUTM "CFLAGS += -DRFM_VIA_PCIE=1\n";
}
if ($rfmTimeSlave > -1) {
  print OUTM "CFLAGS += -DRFM_TIME_SLAVE=1\n";
} else {
  print OUTM "#Uncomment to build an RFM time slave\n";
  print OUTM "#CFLAGS += -DRFM_TIME_SLAVE=1\n";
}
if ($flipSignals) {
  print OUTM "CFLAGS += -DFLIP_SIGNALS=1\n";
}

print OUTM "#Enable use of PCIe RFM Network Gen 2\n";
print OUTM "DOLPHIN_PATH = /opt/srcdis\n";
print OUTM "CFLAGS += -DHAVE_CONFIG_H -I\$(DOLPHIN_PATH)/src/include/dis -I\$(DOLPHIN_PATH)/src/include -I\$(DOLPHIN_PATH)/src/SISCI/cmd/test/lib -I\$(DOLPHIN_PATH)/src/SISCI/src -I\$(DOLPHIN_PATH)/src/SISCI/api -I\$(DOLPHIN_PATH)/src/SISCI/cmd/include -I\$(DOLPHIN_PATH)/src/IRM_GX/drv/src -I\$(DOLPHIN_PATH)/src/IRM_GX/drv/src/LINUX -DOS_IS_LINUX=196616 -DLINUX -DUNIX  -DLITTLE_ENDIAN -DDIS_LITTLE_ENDIAN -DCPU_WORD_IS_64_BIT -DCPU_ADDR_IS_64_BIT -DCPU_WORD_SIZE=64 -DCPU_ADDR_SIZE=64 -DCPU_ARCH_IS_X86_64 -DADAPTER_IS_IX   -m64 -D_REENTRANT\n";

if ($specificCpu > -1) {
  print OUTM "#Comment out to run on first available CPU\n";
  print OUTM "CFLAGS += -DSPECIFIC_CPU=$specificCpu\n";
} else {
  print OUTM "#Uncomment to run on a specific CPU\n";
  print OUTM "#CFLAGS += -DSPECIFIC_CPU=2\n";
}

# Set BIQUAD as default starting RCG V2.8
  print OUTM "#Comment out to go back to old iir_filter calculation form\n";
  print OUTM "CFLAGS += -DALL_BIQUAD=1 -DCORE_BIQUAD=1\n";

if ($::directDacWrite) {
  print OUTM "CFLAGS += -DDIRECT_DAC_WRITE=1\n";
} else {
  print OUTM "#CFLAGS += -DDIRECT_DAC_WRITE=1\n";
}

if ($::noZeroPad) {
  print OUTM "CFLAGS += -DNO_ZERO_PAD=1\n";
} else {
  print OUTM "#CFLAGS += -DNO_ZERO_PAD=1\n";
}

if ($::optimizeIO) {
  print OUTM "CFLAGS += -DNO_DAC_PRELOAD=1\n";
} else {
  print OUTM "#CFLAGS += -DNO_DAC_PRELOAD=1\n";
}
  

if ($::rfmDma) {
  print OUTM "#Comment out to run with RFM DMA\n";
  print OUTM "#CFLAGS += -DRFM_DIRECT_READ=1\n";
} else {
  print OUTM "#Comment out to run with RFM DMA\n";
  print OUTM "CFLAGS += -DRFM_DIRECT_READ=1\n";
}

if ($::rfmDelay) {
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

print OUTM "CFLAGS += -I\$(SUBDIRS)/../../include -I$rcg_src_dir\/src/drv -I$rcg_src_dir\/src/include \n";
print OUTM "LDFLAGS = -L \$(API_LIB_PATH) -lsisci\n";

print OUTM "TARGET=$skeleton\n\n\n";
print OUTM "$skeleton: $skeleton.o rfm.o \n\n";
print OUTM "rfm.o: $rcg_src_dir\/src/drv/rfm.c \n";
my $ccf = "\$\(CC\) \$\(CFLAGS\) \$\(CPPFLAGS\) \-c \$\< \-o \$\@";
print OUTM "\t$ccf \n";
print OUTM ".c.o: \n";
print OUTM "\t$ccf \n";

print OUTM "\n";
close OUTM;
}
#// \b sub \b printVariables \n
#// Add top level variable declarations to generated C Code for selected CDS parts. \n\n
sub printVariables {
for($ii=0;$ii<$partCnt;$ii++)
{
       #print "DBG: cdsPart = $cdsPart[$ii]   partType = $partType[$ii]\n";          # DBG
	if ($cdsPart[$ii]) {
           if ($partType[$ii] ne "FunctionCall") {
              if ($partType[$ii] =~ /^TrueRMS/) {
#                print "\n+++  TEST:  Found a TrueRMS\n";
#                print "\n+++  DESCR=$blockDescr[$ii]\n";

                 if ($blockDescr[$ii] =~ /^window_size=(\d+)/) {
#                   print "\n+++  VALUE=$1\n";
                    $windowSize = $1;
                 }
		 else {
                    $windowSize = 1024;
		 }
	      }
              ("CDS::" . $partType[$ii] . "::printFrontEndVars") -> ($ii);
           }
	}

	if($partType[$ii] eq "DELAY") {
		print OUT "static double \L$xpartName[$ii] = 0.0;\n";
	}
	if($partType[$ii] eq "GROUND")  {
            if ($groundDecl == 0)  {                                       # =+=  MA  =+=
                print OUT "static double ground;\n";                       # =+=  MA  =+=
                $groundDecl++;                                             # =+=  MA  =+=
            }                                                              # =+=  MA  =+=
	}
	if($partType[$ii] eq "CONSTANT")  {
		print OUT "static double \L$xpartName[$ii];\n";
	}
}
print OUT "\n\n";
}

#// \b sub \b commify_series \n
#// Create comma separated string from the lements of an array \n\n
sub commify_series {
    my $sepchar = grep(/,/ => @_) ? ";" : ",";
    (@_ == 0) ? ''                                      :
    (@_ == 1) ?  $_[0]                                   :
                join("$sepchar", @_[0 .. $#_]);
}
#// \b sub \b top_name_transform \n
#// Transform record name for exculsion of sys/subsystem parts \n
#// This function replaces first underscode with the hyphen \n\n
sub top_name_transform {
   ($name) =  @_;
   $name =~ s/_/-/;
   return $name;
};

#// \b sub \b system_name_part \n
#//  Get the system name (the part before the hyphen) \n\n
sub system_name_part {
   ($name) =  @_;
   $name =~ s/([^_]+)-\w+/$1/;
   return $name;
}

#// \b sub \b writeDiagsFile \n
#//  Write file with list of all parts and their connections. \n\n
sub writeDiagsFile {
my ($fileName) = @_;
#open(OUTD,">$fileName") || die "cannot open compile warnings output file for writing";
$diag = "./diags\.txt";
open(OUTD,">".$diag) || die "cannot open diag file for writing";
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
#Call a python script to create ADC channel list file
$rcg_parser =  $rcg_src_dir;
$rcg_parser .= "/src/epics/util/adcparser.py ";
#$rcg_parser .= $fileName;
my $cpcmd = "cp ";
$cpcmd .= " ./diags.txt ";
$cpcmd .= $fileName;
system($rcg_parser);
sleep(2);
system($cpcmd);
# End DIAGNOSTIC
}
