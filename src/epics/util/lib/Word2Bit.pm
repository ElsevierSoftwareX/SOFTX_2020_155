package CDS::Word2Bit;
use Exporter;
@ISA = ('Exporter');

sub partType {
	return Word2Bit;
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
        print ::OUT "unsigned int \L$::xpartName[$i]\[16\];\n";
}

# Check inputs are connected
sub checkInputConnect {
        my ($i) = @_;
	if($::partInCnt[$i] < 1) {
                print ::CONN_ERRORS "***\n$::partType[$i] with name $::xpartName[$i] has no input connected.\n\n";
                return "ERROR";
        }
        return "";
}

# Figure out part input code
# Argument 1 is the part number
# Argument 2 is the input number
# Returns calculated input code
sub fromExp {
        my ($i, $j) = @_;
        my $from = $::partInNum[$i][$j];
        my $fromPort = $::partInputPort[$i][$j];
        return "\L$::xpartName[$from]\[" . $fromPort . "\]";
}

# Return front end initialization code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndInitCode {
#       my ($i) = @_;
}


# Return front end code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndCode {
	my ($i) = @_;
        my $calcExp = "// Word2Bit:  $::xpartName[$i]\n{\n";
        $calcExp .= "unsigned int in = $::fromExp[0];\n";
        $calcExp .= "for (ii = 0; ii < 16; ii++)\n{\n";
        $calcExp .= "if (in%2) {\n";
        $calcExp .= "\L$::xpartName[$i]\[ii\] = 1;\n";
        $calcExp .= "}\n";
        $calcExp .= "else {\n";
        $calcExp .= "\L$::xpartName[$i]\[ii\] = 0;\n";
        $calcExp .= "}\n";
        $calcExp .= "in = in>>1;\n";
        $calcExp .= "}\n";
        $calcExp .= "}\n";
	return $calcExp;
}
