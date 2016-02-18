package CDS::Sqrt;
use Exporter;
@ISA = ('Exporter');

sub partType {
	return Sqrt;
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
        return "";
}

# Return front end initialization code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndInitCode {
}


# Return front end code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndCode {
	my ($i) = @_;
        my $calcExp = "// SquareRoot:  $::xpartName[$i]\n";
        $calcExp .= "if ($::fromExp[0] > 0.0) {\n";
        $calcExp .= "   \L$::xpartName[$i] = ";
        $calcExp .= "lsqrt($::fromExp[0]);\n";
        $calcExp .= "}\n";
        $calcExp .= "else {\n";
        $calcExp .= "   \L$::xpartName[$i] = 0.0;\n";
        $calcExp .= "}\n";
	return $calcExp;
}

