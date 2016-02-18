package CDS::Rms;
use Exporter;
@ISA = ('Exporter');

sub partType {
	return Rms;
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
        print ::OUT "static double \L$::xpartName[$i]\_avg;\n";
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
	my ($i) = @_;
	return "\L$::xpartName[$i]\_avg = 0\.0;\n";
}


# Return front end code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndCode {
	my ($i) = @_;
        my $calcExp = "// RMS:  $::xpartName[$i]\n";
        $calcExp .= "\L$::xpartName[$i]";
        $calcExp .= " = ";
        $calcExp .= $::fromExp[0];
        $calcExp .= ";\n";
        $calcExp .= "if(\L$::xpartName[$i]\ \> 200000\) \L$::xpartName[$i] = 200000;\n";
        $calcExp .= "if(\L$::xpartName[$i]\ \< -200000\) \L$::xpartName[$i] = -200000;\n";
        $calcExp .= "\L$::xpartName[$i] = \L$::xpartName[$i] * \L$::xpartName[$i];\n";
        $calcExp .= "\L$::xpartName[$i]\_avg = \L$::xpartName[$i] * \.00005 + ";
        $calcExp .= "\L$::xpartName[$i]\_avg * 0\.99995;\n";
        $calcExp .= "\L$::xpartName[$i] = lsqrt(\L$::xpartName[$i]\_avg);\n";
	return $calcExp;
}
