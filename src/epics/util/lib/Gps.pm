package CDS::Gps;
use Exporter;
@ISA = ('Exporter');

#//     \page Gps Gps.pm
#//     Gps.pm - provides standard subroutines for handling GPS parts in CDS PARTS library.
#//
#// \n
#// \n

sub partType {
	return Gps;
}

# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub printHeaderStruct {
        my ($i) = @_;
        ;
}

# Print Epics variable definitions
# Current part number is passed as first argument
sub printEpics {
        my ($i) = @_;
        ;
}

# Print variable declarations int front-end file
# Current part number is passed as first argument
sub printFrontEndVars  {
        my ($i) = @_;
 	#if ( !$::gps_vars_printed) {
		#$::gps_vars_printed = 1;
		#print ::OUT "unsigned int cycle_gps_ns;\n";
 		#print ::OUT "double cycle_gps_time = getGpsTime(&cycle_gps_ns);\n";
	#}
}

# Check inputs are connected
sub checkInputConnect {
        my ($i) = @_;
        return "";
}

# Return front end initialization code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndInitCode {
	my ($i) = @_;
        return "";
}

# Figure out part input code
# Argument 1 is the part number
# Argument 2 is the input number
# Returns calculated input code
sub fromExp {
        my ($i, $j) = @_;
        my $port = $::partInputPort[$i][$j];

	if ($port == 0) { return "cycle_gps_time"; }
        elsif ($port == 1) { return "(unsigned long)cycle_gps_time"; }
        elsif ($port == 2) { return "cycle_gps_time - (unsigned long)cycle_gps_time"; }
        elsif ($port == 3) { return "cycle_gps_ns"; }
}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string

sub frontEndCode {
        return "";
}
