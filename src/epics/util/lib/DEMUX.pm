package CDS::DEMUX;
use Exporter;
@ISA = ('Exporter');

#//     \page DEMUX DEMUX.pm
#//     Documentation for DEMUX.pm
#//
#// \n

sub partType {
        my ($node, $i) = @_;
	return DEMUX;
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
	print ::OUT "double \L$::xpartName[$i]\[$::partOutputs[$i]\];\n";
}

# Check inputs are connected
sub checkInputConnect {
        my ($i) = @_;
	my $inCnt = $::partInCnt[$i];
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
        return "\L$::xpartName[$from]\[$::partInputPort[$i][$j]\]";
}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string

sub frontEndCode {
	my ($i) = @_;
	my $outCnt = $::partOutputs[$i];
	my $calcExp = "";
	if($::partInputType[$i][0] ne "FunctionCall") {
		$calcExp = "// DEMUX\n";
		for (0 .. $outCnt - 1) {
			    $calcExp .= "\L$::xpartName[$i]\[$_\]" . "= $::fromExp[0]\[". $_ . "\];\n";
			}
	}
        return $calcExp;
}
