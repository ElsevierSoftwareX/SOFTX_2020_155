package CDS::MUX;
use Exporter;
@ISA = ('Exporter');

#//     \page MUX MUX.pm
#//     Documentation for MUX.pm
#//
#// \n

sub partType {
        my ($node, $i) = @_;
	return MUX;
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
	print ::OUT "double \L$::xpartName[$i]\[$::partInCnt[$i]\];\n";
}

# Check inputs are connected
sub checkInputConnect {
        my ($i) = @_;
	my $incnt = $::partInputs[$i];
	if($::partInCnt[$i] != $incnt) {
                print ::CONN_ERRORS "***\n$::partType[$i] with name $::xpartName[$i] has missing inputs\nRequires $incnt; Only $::partInCnt[$i] provided.\n";
	        return "ERROR";
        }
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
	my $inCnt = $::partInCnt[$i];
	my $calcExp = "// MUX\n";
	for (0 .. $inCnt - 1) {
                    $calcExp .= "\L$::xpartName[$i]\[$_\]" . "= $::fromExp[$_];\n";;
                }
        return $calcExp;
}
