package CDS::Parameters;
use Exporter;
use Env qw(RCG_HOST);
@ISA = ('Exporter');


#//     \page Parameters Parameter.pm
#//     Documentation for Parameters.pm
#//
#// \n


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
		if (@spp == 2) {
			if ($spp[0] eq "site" ) {
                $::sitedepwarning = 1;
            }
			if ($spp[0] eq "adcMaster" ) {
                $::adcmasterdepwarning = 1;
            }
			if (($spp[0] eq "site") or ($spp[0] eq "ifo") ) {
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
        			} elsif ($::ifo =~ /^C/) {
                			$::location = "caltech";
        			} elsif ($::ifo =~ /^S/) {
                			$::location = "stn";
        			} elsif ($::ifo =~ /^K/) {
                			$::location = "kamioka";
        			} elsif ($::ifo =~ /^X/) {
                			$::location = "tst";
        			}
			} elsif ($spp[0] eq "rate") {
				print "Rate set to $spp[1]\n";
        			my $param_speed = $spp[1];
        			if ($param_speed eq "2K") {
                			$::rate = 480;
                			$::modelrate = 2;
                			$::servoflag = "-DSERVO2K";
        			} elsif ($param_speed eq "4K") {
                			$::rate = 240;
                			$::modelrate = 4;
                			$::servoflag = "-DSERVO4K";
        			} elsif ($param_speed eq "16K") {
                			$::rate = 60;
                			$::modelrate = 16;
                			$::servoflag = "-DSERVO16K";
        			} elsif ($param_speed eq "32K") {
                			$::rate = 30;
                			$::modelrate = 32;
                			$::servoflag = "-DSERVO32K";
        			} elsif ($param_speed eq "64K") {
                			$::rate = 15;
                			$::modelrate = 64;
                			$::servoflag = "-DSERVO64K";
        			} elsif ($param_speed eq "128K") {
                			$::rate = 8;
                			$::modelrate = 128;
                			$::servoflag = "-DSERVO128K";
        			} elsif ($param_speed eq "256K") {
                			$::rate = 4;
                			$::modelrate = 256;
                			$::servoflag = "-DSERVO256K";
        			} elsif ($param_speed eq "512K") {
                			$::rate = 2;
                			$::modelrate = 512;
                			$::servoflag = "-DSERVO512K";
        			} elsif ($param_speed eq "1024K") {
                			$::rate = 1;
                			$::modelrate = 1024;
                			$::servoflag = "-DSERVO1024K";
        			} else  { die "Invalid speed $param_speed specified\n"; }

			} elsif ($spp[0] eq "dcuid") {
				print "Dcu Id is set to $spp[1]\n";
				$::dcuId = $spp[1];
				print "GDS node id is set to $::dcuId \n";
				$::gdsNodeId = $::dcuId;
			} elsif ($spp[0] eq "host") {
				print "Target host name is set to $spp[1]\n";
				$::targetHost = $spp[1];
				if ($RCG_HOST) {
					$::targetHost = $RCG_HOST;
				}
			} elsif ($spp[0] eq "plant_name") {
				print "Plant name is set to $spp[1]\n";
				$::plantName = $spp[1];
			} elsif ($spp[0] eq "shmem_daq" && $spp[1] == 1) {
                # This is no longer required as this is the default
                # It is left here for now to avoid changing the many
                # controls models which already specify this parameter
				print "Shared memory DAQ connection (No Myrinet)\n";
				$::shmem_daq = 1;
			} elsif ($spp[0] eq "no_sync" && $spp[1] == 1) {
                # This essentially set up IOP for a Cymac
				print "Will not sync up to 1PPS\n";
				$::no_sync = 1;
			} elsif ($spp[0] eq "no_daq" && $spp[1] == 1) {
                # Will compile code not to use DAQ
				print "Will not connect to DAQ\n";
				$::no_daq = 1;
			} elsif ($spp[0] eq "no_oversampling" && $spp[1] == 1) {
				print "Will not oversample\n";
				$::no_oversampling = 1;
			} elsif ($spp[0] eq "no_dac_interpolation" && $spp[1] == 1) {
				print "Will not interpolate DAC\n";
				$::no_dac_interpolation = 1;
			} elsif ($spp[0] eq "specific_cpu") {
				print "FE will run on CPU $spp[1]\n";
				$::specificCpu = $spp[1];
			} elsif ($spp[0] eq "adcMaster") {
				print "FE will run as IOP\n";
				$::iopModel = $spp[1];
			} elsif ($spp[0] eq "iop_model") {
				print "FE will run as IOP\n";
				$::iopModel = $spp[1];
			} elsif ($spp[0] eq "diagTest") {
				print "FE Compiles as DIAG TEST CODE\n";
				$::diagTest = $spp[1];
			} elsif ($spp[0] eq "dacwdoverride") {
				print "FE Compiles with override of bad DAC error\n";
				$::dacWdOverride = $spp[1];
			} elsif ($spp[0] eq "dophin_time_xmit") {
				$::dolphin_time_xmit = $spp[1];
			} elsif ($spp[0] eq "dolphin_recover") {
				$::dolphin_recover = $spp[1];
			} elsif ($spp[0] eq "dolphin_time_rcvr") {
				$::dolphinTiming = $spp[1];
			} elsif ($spp[0] eq "no_cpu_shutdown") {
				$::no_cpu_shutdown = $spp[1];
			} elsif ($spp[0] eq "pciRfm") {
				print "FE will run with PCIE RFM Network\n";
				$::pciNet = $spp[1];
			} elsif ($spp[0] eq "dolphingen") {
				print "FE will run with PCIE RFM Network\n";
                # Set Dolphin Gen to run with; default=2
				$::dolphinGen = $spp[1];
			} elsif ($spp[0] eq "remoteGPS") {
				print "FE will run with EPICS for GPS Time\n";
				$::remoteGPS = $spp[1];
			} elsif ($spp[0] eq "rfm_dma") {
				$::rfmDma = 1;
			} elsif ($spp[0] eq "rfm_delay") {
				$::rfmDelay = 1;
			} elsif ($spp[0] eq "flip_signals") { 
				$::flipSignals = $spp[1];
			} elsif ($spp[0] eq "edcu") { 
				$::edcu = $spp[1];
			} elsif ($spp[0] eq "sdf") { 
				$::globalsdf = $spp[1];
			} elsif ($spp[0] eq "casdf") {
				$::casdf = $spp[1];
			} elsif ($spp[0] eq "biquad") { 
				$::allBiquad = $spp[1];
				print "AllBiquad set\n";
			} elsif ($spp[0] eq "requireIOcnt") { 
				$::requireIOcnt = $spp[1];
			} elsif ($spp[0] eq "virtualIOP") { 
				$::virtualiop = $spp[1];
			} elsif ($spp[0] eq "use_shm_ipc") { 
				$::force_shm_ipc = $spp[1];
			} elsif ($spp[0] eq "adcclock") { 
				$::adcclock = $spp[1];
			} elsif ($spp[0] eq "clock_div") { 
				$::clock_div = $spp[1];
			} elsif ($spp[0] eq "sync") { 
				$::edcusync = $spp[1];
			} elsif ($spp[0] eq "optimizeIO") { 
				$::optimizeIO = $spp[1];
			} elsif ($spp[0] eq "no_zero_pad") { 
				$::noZeroPad = $spp[1];
			} elsif ($spp[0] eq "ipc_rate") { 
                # Specify IPC rate if lower than model rate
				$::ipcrate = $spp[1];
			}
		}
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
