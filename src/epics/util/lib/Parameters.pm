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
			if ($spp[0] eq "site") {
				$spp[1] =~ s/,/ /g;

                                if (lc($spp[1]) ne $::ifo) {
                                   $errmsg = "***ERROR: Model <ifo> name part does not match cdsParameters: ";
                                   $errmsg .= $::ifo . ", " . $spp[1] . "\n";

                                #   die $errmsg;
                                    die $errmsg;
                                }

				print "Site is set to $spp[1]\n";
				$::site = $spp[1];
			        if ($::site =~ /^M/) {
                			$::location = "mit";
        			} elsif ($::site =~ /^G/) {
                			$::location = "geo";
        			} elsif ($::site =~ /^H/) {
                			$::location = "lho";
        			} elsif ($::site =~ /^L/) {
                			$::location = "llo";
        			} elsif ($::site =~ /^C/) {
                			$::location = "caltech";
        			} elsif ($::site =~ /^S/) {
                			$::location = "stn";
        			} elsif ($::site =~ /^K/) {
                			$::location = "kamioka";
        			} elsif ($::site =~ /^X/) {
                			$::location = "tst";
        			}
			} elsif ($spp[0] eq "rate") {
				print "Rate set to $spp[1]\n";
        			my $param_speed = $spp[1];
        			if ($param_speed eq "2K") {
                			$::rate = 480;
        			} elsif ($param_speed eq "4K") {
                			$::rate = 240;
        			} elsif ($param_speed eq "16K") {
                			$::rate = 60;
        			} elsif ($param_speed eq "32K") {
                			$::rate = 30;
        			} elsif ($param_speed eq "64K") {
                			$::rate = 15;
        			} elsif ($param_speed eq "128K") {
                			$::rate = 8;
        			} elsif ($param_speed eq "256K") {
                			$::rate = 4;
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
				print "Shared memory DAQ connection (No Myrinet)\n";
				$::shmem_daq = 1;
			} elsif ($spp[0] eq "no_sync" && $spp[1] == 1) {
				print "Will not sync up to 1PPS\n";
				$::no_sync = 1;
			} elsif ($spp[0] eq "no_daq" && $spp[1] == 1) {
				print "Will not connect to DAQ\n";
				$::no_daq = 1;
			} elsif ($spp[0] eq "dac_internal_clocking" && $spp[1] == 1) {
				print "Will clock D/A converter internally\n";
				$::dac_internal_clocking = 1;
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
				$::adcMaster = $spp[1];
			} elsif ($spp[0] eq "diagTest") {
				print "FE Compiles as DIAG TEST CODE\n";
				$::diagTest = $spp[1];
			} elsif ($spp[0] eq "dacwdoverride") {
				print "FE Compiles with override of bad DAC error\n";
				$::dacWdOverride = $spp[1];
			} elsif ($spp[0] eq "adcSlave") {
				print "FE will run as SLAVE to IOP\n";
				$::adcSlave = $spp[1];
			} elsif ($spp[0] eq "time_master") {
				$::timeMaster = $spp[1];
			} elsif ($spp[0] eq "time_slave") {
				$::timeSlave = $spp[1];
			} elsif ($spp[0] eq "iop_time_slave") {
				$::iopTimeSlave = $spp[1];
			} elsif ($spp[0] eq "no_cpu_shutdown") {
				$::no_cpu_shutdown = $spp[1];
			} elsif ($spp[0] eq "rfm_time_slave") {
				$::rfmTimeSlave = $spp[1];
			} elsif ($spp[0] eq "pciRfm") {
				print "FE will run with PCIE RFM Network\n";
				$::pciNet = $spp[1];
			} elsif ($spp[0] eq "remoteGPS") {
				print "FE will run with EPICS for GPS Time\n";
				$::remoteGPS = $spp[1];
			} elsif ($spp[0] eq "remote_ipc_port") {
				$::remoteIPCport = $spp[1];
        			die "Invalid remote_ipc_port specified in cdsParamters\n" unless $::remoteIPCport >= 0;
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
			} elsif ($spp[0] eq "direct_dac_write") { 
				$::directDacWrite = $spp[1];
			} elsif ($spp[0] eq "requireIOcnt") { 
				$::requireIOcnt = $spp[1];
			} elsif ($spp[0] eq "virtualIOP") { 
				$::virtualiop = $spp[1];
			} elsif ($spp[0] eq "optimizeIO") { 
				$::optimizeIO = $spp[1];
			} elsif ($spp[0] eq "no_zero_pad") { 
				$::noZeroPad = $spp[1];
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
