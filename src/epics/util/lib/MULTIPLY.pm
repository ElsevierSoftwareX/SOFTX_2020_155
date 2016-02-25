package CDS::MULTIPLY;
use Exporter;
@ISA = ('Exporter');

#//     \page MULTIPLY MULTIPLY.pm
#//     Documentation for MULTIPLY.pm
#//
#// \n

sub partType {
        my ($node, $i) = @_;
	return MULTIPLY;
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
	print ::OUT "double \L$::xpartName[$i];\n";
}

# Check inputs are connected
sub checkInputConnect {
        my ($i) = @_;
	my $inCnt = $::partInputs[$i];
	if($::partInCnt[$i] != $inCnt) {
                print ::CONN_ERRORS "***\n$::partType[$i] with name $::xpartName[$i] has missing inputs\nRequires $inCnt; Only $::partInCnt[$i] provided.\n";
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
        my $calcExp = "// MULTIPLY\n";
	$calcExp .= "\L$::xpartName[$i]";
	$calcExp .= " = ";
	my $inCnt = $::partInCnt[$i];

	for ($qq=0; $qq<$inCnt; $qq++) {
		$zz = $qq+1;
		if(($zz - $inCnt) == 0)
		{
			$calcExp .= $::fromExp[$qq];
		        $calcExp .= ";\n";
		} else {
		        $calcExp .= $::fromExp[$qq];
		        $calcExp .= " * ";
		}
        }
        return $calcExp;
}
