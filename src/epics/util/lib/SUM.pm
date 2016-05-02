package CDS::SUM;
use Exporter;
@ISA = ('Exporter');

#//     \page MatlabSum SUM.pm
#//     Documentation for SUM.pm
#//
#// \n

sub partType {
        my ($node, $i) = @_;
	return SUM;
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
	my $inCnt = $::partInCnt[$i];
	my $ops = length($::partInputs[$i]);
	# Sometimes, Matlab does not write equation field when only 2 values are summed.
	if ($ops == 0) {
		$ops = 2;
	}
	if($ops != $inCnt) {
		print ::CONN_ERRORS "***\n$::partType[$i] with name $::xpartName[$i] has missing inputs\nRequires $ops; Only $inCnt provided:  \n";
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
	my $calcExp = "// SUM\n";
	#my $calcExp = "// SUM: $::xpartName[$i]\n";
	$calcExp .= "\L$::xpartName[$i]";
        $calcExp .= " =";
	for (0 .. $inCnt - 1) {
                    my $op = substr($::partInputs[$i], $_, 1);
                    if ($op eq "") { $op = "+"; }
                    if ($_ > 0 || $op eq "-")  { $calcExp .=  " " . $op; }
                    $calcExp .= " " . $::fromExp[$_];
                }
         $calcExp .= ";\n";
        return $calcExp;
}
