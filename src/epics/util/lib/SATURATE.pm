package CDS::SATURATE;
use Exporter;
@ISA = ('Exporter');

#//     \page SATURATE SATURATE.pm
#//     Documentation for SATURATE.pm
#//
#// \n

sub partType {
        my ($node, $i) = @_;
	return SATURATE;
}

# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub printHeaderStruct {
        my ($i) = @_;
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
	print ::OUT "double \L$::xpartName[$i] = 0.0;\n";
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
	my $from = $::partInNum[$i][$j];
        return "\L$::xpartName[$from]";
}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string

sub frontEndCode {
	my ($i) = @_;
	my $part_name = "\L$::xpartName[$i]";
	my $upper_limit = "$::partInputs[$i]";
	my $lower_limit = "$::partInputs1[$i]";

	$calcExp = <<END
// SATURATE
$part_name = $::fromExp[0];
if ($part_name > $upper_limit) {
  $part_name  = $upper_limit;
} else if ($part_name < $lower_limit) {
  $part_name  = $lower_limit;
};
END
;
        return $calcExp;
}
