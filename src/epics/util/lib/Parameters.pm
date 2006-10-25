package CDS::Parameters;
use Exporter;
@ISA = ('Exporter');

sub partType {
	return Parameters;
}

# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub printHeaderStruct {
        my ($i) = @_;
	my @sp = split(/\\n/, $::xpartName[$i]);
	#print "Model Parameters are $::xpartName[$i];\n";
	#print "Split array is @sp\n";
	for (@sp) {
		@spp = split(/=/);
		if (@spp == 2) {
			if ($spp[0] eq "site") {
				$spp[1] =~ s/,/ /g;
				print "Site is set to $spp[1]\n";
				$::site = $spp[1];
			} elsif ($spp[0] eq "rate") {
				print "Rate set to $spp[1]\n";
        			my $param_speed = $spp[1];
        			if ($param_speed eq "2K") {
                			$::rate = 480;
        			} elsif ($param_speed eq "16K") {
                			$::rate = 60;
        			} elsif ($param_speed eq "32K") {
                			$::rate = 30;
        			} elsif ($param_speed eq "64K") {
                			$::rate = 15;
        			} else  { die "Invalid speed $param_speed specified\n"; }

			} elsif ($spp[0] eq "dcuid") {
				print "Dcu Id is set to $spp[1]\n";
				$::dcuId = $spp[1];
			}
		}
	}
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
