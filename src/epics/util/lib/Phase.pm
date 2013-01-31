package CDS::Phase;
use Exporter;
@ISA = ('Exporter');

#//     \page Phase Phase.pm
#//     Documentation for Phase.pm
#//
#// \n


sub partType {
	return Phase;
}

# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub printHeaderStruct {
        my ($i) = @_;
        print ::OUTH "\tdouble $::xpartName[$i]\[2\];\n";
}

# Print Epics variable definitions
# Current part number is passed as first argument
sub printEpics {
        my ($i) = @_;
	print ::EPICS "PHASE $::xpartName[$i] $::systemName\.$::xpartName[$i] float ai 0 field(PREC,\"3\")\n";
}


# Print variable declarations int front-end file
# Current part number is passed as first argument
sub printFrontEndVars  {
        my ($i) = @_;
        print ::OUT "static double \L$::xpartName[$i]\[2\];\n";
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
	my ($i) = @_;
	return "";
}


# Return front end code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndCode {
	my ($i) = @_;
        my $calcExp = "// PHASE:  $::xpartName[$i]\n";
        $calcExp .= "\L$::xpartName[$i]\[0\] = \(";
        $calcExp .=  "$::fromExp[0]";
        $calcExp .=  " * ";
        $calcExp .=  "pLocalEpics->$::systemName\.$::xpartName[$i]";
        $calcExp .=  "\[1\]\) + \(";
        $calcExp .=  "$::fromExp[1]";
        $calcExp .=  " * ";
        $calcExp .=  "pLocalEpics->$::systemName\.$::xpartName[$i]";
        $calcExp .=  "\[0\]);\n";
        $calcExp .= "\L$::xpartName[$i]\[1\] = \(";
        $calcExp .=  "$::fromExp[1]";
        $calcExp .=  " * ";
        $calcExp .=  "pLocalEpics->$::systemName\.$::xpartName[$i]";
        $calcExp .=  "\[1\]\) - \(";
        $calcExp .=  "$::fromExp[0]";
        $calcExp .=  " * ";
        $calcExp .=  "pLocalEpics->$::systemName\.$::xpartName[$i]";
        $calcExp .=  "\[0\]);\n";
        return $calcExp;
}
