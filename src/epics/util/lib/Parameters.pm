package CDS::Parameters;
use Exporter;
use Env qw(RCG_HOST);
@ISA = ('Exporter');


#//     \page Parameters Parameter.pm
#//     Documentation for Parameters.pm
#//
#// \n

use Switch;

# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub partType {
	return Parameters;
}

sub parseParams {
        my ($i) = @_;
	my @sp = split(/\\n/, $i);
	#print "Model Parameters are $i;\n";
	#print "Split array is @sp\n";
	for (@sp) {
		@spp = split(/=/);
        # Find and convert params due for deprecation in later releases
		if (@spp == 2) {
            if ($spp[0] eq "site" ) {
                $::sitedepwarning = 1;
                $spp[0] = "ifo";
            }
            if($spp[0] eq "adcMaster") {
                $::adcmasterdepwarning = 1;
                $spp[0] = "iop_model";
            }
            if($spp[0] eq "time_master") {
                $::timemasterdepwarning = 1;
                $spp[0] = "dolphin_time_xmit";
            }
            switch($spp[0])
            {
                case  "ifo" {
				    print "PARAM ifo set to $spp[1]\n";
				    $spp[1] =~ s/,/ /g;
                    if (lc($spp[1]) ne $::ifo_from_mdl_name) {
                        $errmsg = "***ERROR: Model <ifo> name part does not match cdsParameters: ";
                        $errmsg .= $::ifo . ", " . $spp[1] . "\n";
                        die $errmsg;
                    }

				    $::ifo = $spp[1];
			        if ($::ifo =~ /^M/) {
                			$::location = "mit";
        			} elsif ($::ifo =~ /^A/) {
                			$::location = "lao";
        			} elsif ($::ifo =~ /^G/) {
                			$::location = "geo";
        			} elsif ($::ifo =~ /^H/) {
                			$::location = "lho";
        			} elsif ($::ifo =~ /^L/) {
                			$::location = "llo";
        			} elsif ($::ifo =~ /^N/) {
                			$::location = "anu";
        			} elsif ($::ifo =~ /^C/) {
                			$::location = "caltech";
        			} elsif ($::ifo =~ /^S/) {
                			$::location = "stn";
        			} elsif ($::ifo =~ /^K/) {
                			$::location = "kamioka";
        			} elsif ($::ifo =~ /^X/) {
                			$::location = "tst";
        			}
                }
                case "rate"
                {
				    print "PARAM Rate set to $spp[1]\n";
        			my $param_speed = $spp[1];
                    switch($param_speed)
                    {
        			case "2K" {
                			$::rate = 480;
                			$::modelrate = 2;
                			$::servoflag = "-DSERVO2K";
        			} 
        			case "4K" {
                			$::rate = 240;
                			$::modelrate = 4;
                			$::servoflag = "-DSERVO4K";
        			} 
                    case "16K" {
                			$::rate = 60;
                			$::modelrate = 16;
                			$::servoflag = "-DSERVO16K";
                    }
                    case "32K" {
                			$::rate = 30;
                			$::modelrate = 32;
                			$::servoflag = "-DSERVO32K";
                    }
                    case "64K" {
                			$::rate = 15;
                			$::modelrate = 64;
                			$::servoflag = "-DSERVO64K";
                    }
                    case "128K" {
                			$::rate = 8;
                			$::modelrate = 128;
                			$::servoflag = "-DSERVO128K";
                    }
                    case "256K" {
                			$::rate = 4;
                			$::modelrate = 256;
                			$::servoflag = "-DSERVO256K";
                    }
                    case "512K" {
                			$::rate = 2;
                			$::modelrate = 512;
                			$::servoflag = "-DSERVO512K";
        			} 
                    else  { 
                        $errmsg = "\n************\n";
                        $errmsg .= "***ERROR: Invalid Parameter Block Entry: rate = $param_speed \n";
                        $errmsg .= "************\n\n";
                        die $errmsg;
                    }
                    }
                }

                case "dcuid"
                {
				    print "PARAM Dcu Id is set to $spp[1]\n";
				    $::dcuId = $spp[1];
				    print "GDS node id is set to $::dcuId \n";
				    $::gdsNodeId = $::dcuId;
                }
                case "host"
                {
				    print "PARAM Target host name is set to $spp[1]\n";
				    $::targetHost = $spp[1];
				    if ($RCG_HOST) {
					    $::targetHost = $RCG_HOST;
				    }
                }
                case "plant_name"
                {
				    print "Plant name is set to $spp[1]\n";
				    $::plantName = $spp[1];
                }
                case "daq_prefix"
                {
				    $::daq_prefix = $spp[1];
                }
                case "no_sync"
                {
                    # This essentially set up IOP for a Cymac
				    print "Will not sync up to 1PPS\n";
				    $::no_sync = $spp[1];
                }
                case "test1pps"
                {
                    # This forces sync to 1pps for testing
				    print "Force sync up to 1PPS\n";
				    $::test1pps = $spp[1];
                }
                case "no_daq"
                {
                    # Will compile code not to use DAQ
				    print "Will not connect to DAQ\n";
				    $::no_daq = 1;
                }
                case "enable_fir"
                {
                    # Will compile code to use FIR filtes
				    print "Will use FIR filters\n";
				    $::useFIRs = 1;
                }
                case "no_oversampling"
                {
				    print "Will not oversample\n";
				    $::no_oversampling = 1;
                }
                case "no_dac_interpolation"
                {
				    print "Will not interpolate DAC\n";
				    $::no_dac_interpolation = 1;
                }
                case "specific_cpu"
                {
				    print "PARAM FE will run on CPU $spp[1]\n";
				    $::specificCpu = $spp[1];
                }
                case "iop_model"
                {
				    print "PARAM FE is IOP\n";
				    print "FE will run as IOP\n";
				    $::iopModel = $spp[1];
                    if($::location eq "lho" or $::location eq "llo")
                    {
                        $::requireIOcnt = 1;
                    }
                }
                case "diagTest"
                {
				    print "PARAM FE Compiles as DIAG TEST CODE\n";
				    $::diagTest = $spp[1];
                }
                case "dacwdoverride"
                {
				    print "FE Compiles with override of bad DAC error\n";
				    $::dacWdOverride = $spp[1];
                }
                case "dolphin_time_xmit"
                {
				    $::dolphin_time_xmit = $spp[1];
                }
                case "dolphin_recover"
                {
				    $::dolphin_recover = $spp[1];
                }
                case "dolphin_time_rcvr"
                {
				    $::dolphinTiming = $spp[1];
                }
                case "no_cpu_shutdown"
                {
				    $::no_cpu_shutdown = $spp[1];
                }
                case "pciRfm"
                {
				    print "PARAM FE will run with PCIE RFM Network\n";
				    $::pciNet = $spp[1];
                }
                case "dolphingen"
                {
				    print "PARAM FE will run with PCIE RFM Network\n";
                    # Set Dolphin Gen to run with; default=2
				    $::dolphinGen = $spp[1];
                }
                case "remoteGPS"
                {
				    print "FE will run with EPICS for GPS Time\n";
				    $::remoteGPS = $spp[1];
                }
                case "rfm_delay"
                {
				    $::rfmDelay = 1;
                }
                case "flip_signals"
                {
				    $::flipSignals = $spp[1];
                }
                case "sdf"
                {
				    $::globalsdf = $spp[1];
                }
                case "casdf"
                {
				    $::casdf = $spp[1];
                }
                case "requireIOcnt"
                {
				    $::requireIOcnt = $spp[1];
                }
                case "virtualIOP"
                {
				    $::virtualiop = $spp[1];
                }
                case "use_shm_ipc"
                {
				    $::force_shm_ipc = $spp[1];
                }
                case "adcclock"
                {
				    $::adcclock = $spp[1];
                }
                case "clock_div"
                {
				    $::clock_div = $spp[1];
                }
                case "sync"
                {
				    $::edcusync = $spp[1];
                }
                case "bio_test"
                {
				    $::biotest = $spp[1];
                }
                case "optimizeIO"
                {
				    $::optimizeIO = $spp[1];
                }
                case "no_zero_pad"
                {
				    $::noZeroPad = $spp[1];
                }
                case "lhomid"
                {
				    $::lhomid = $spp[1];
                }
                case "ipc_rate"
                {
                    # Specify IPC rate if lower than model rate
				    $::ipcrate = $spp[1];
                }
                # Following are old options that are no longer required
                case "biquad"
                {
				    $nolongerused = 1;
                }
                case "adcSlave"
                {
				    $nolongerused = 2;
                }
                case "accum_overflow"
                {
				    $nolongerused = 2;
                }
                case "shmem_daq"
                {
				    $nolongerused = 2;
                }
                case "rfm_dma"
                {
				    $nolongerused = 2;
                }
			    else {
                $errmsg = "***ERROR: Unknown Parameter Block Entry: ";
                $errmsg .=  $spp[0] . "\n";
                die $errmsg;
                }
		    }
        }
	}
    # Check that all required Parameter block entries have been set
    if($::ifo eq "dummy")
    {
        $errmsg = "\n************\n";
        $errmsg .= "***ERROR: Missing Required Parameter Block Entry: ifo\n";
        $errmsg .= "************\n\n";
        die $errmsg;
    }
    if($::targetHost eq "dummy")
    {
        $errmsg = "\n************\n";
        $errmsg .= "***ERROR: Missing Required Parameter Block Entry: host\n";
        $errmsg .= "********: Please add host=targetname, where: \n";
        $errmsg .= "********: \ttargetname is name of computer on which code will run\n";
        $errmsg .= "************\n\n";
        die $errmsg;
    }
    if($::rate eq 0)
    {
        $errmsg = "\n************\n";
        $errmsg .= "***ERROR: Missing Required Parameter Block Entry: rate\n";
        $errmsg .= "************\n\n";
        die $errmsg;
    }
    if($::dcuId eq 0)
    {
        $errmsg = "\n************\n";
        $errmsg .= "***ERROR: Missing Required Parameter Block Entry: dcuid\n";
        $errmsg .= "************\n\n";
        die $errmsg;
    }
    if($::specificCpu eq -1 && $::iopModel ne 1)
    {
        $errmsg = "\n************\n";
        $errmsg .= "***ERROR: Missing Required Parameter Block Entry: specific_cpu\n";
        $errmsg .= "********: Please add specific_cpu=x, where: \n";
        $errmsg .= "********: \tx = CPU core on which code will run\n";
        $errmsg .= "************\n\n";
        die $errmsg;
    }
}

sub printHeaderStruct {
}

# Print Epics variable definitions
# Current part number is passed as first argument
sub printEpics {
        my ($i) = @_;
}


# Print variable declarations int front-end file
# Current part number is passed as first argument
sub printFrontEndVars  {
        my ($i) = @_;
}

# Check inputs are connected
sub checkInputConnect {
        my ($i) = @_;
				    print "PARAM doin connect check\n";
        return "";
}

# Figure out part input code
# Argument 1 is the part number
# Argument 2 is the input number
# Returns calculated input code
sub fromExp {
        my ($i, $j) = @_;
        return "";
}

# Return front end initialization code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndInitCode {
	my ($i) = @_;
	return "";
}


# Return front end code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndCode {
	my ($i) = @_;
	return "";
}
