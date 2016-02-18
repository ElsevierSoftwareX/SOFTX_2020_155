package CDS::PhaseDeg;
use Exporter;
@ISA = ('Exporter');

sub partType {
	return PhaseDeg;
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
	print ::EPICS "PHASEDEG $::xpartName[$i] $::systemName\.$::xpartName[$i] double ai 0 field(PREC,\"3\")\n";
}


# Print variable declarations int front-end file
# Current part number is passed as first argument
sub printFrontEndVars  {
        my ($i) = @_;
        print ::OUT "static double \L$::xpartName[$i]\[2\];\n";
}

# Check inputs are connected
sub checkInputConnect {
        my ($i) = @_;
	if($::partInCnt[$i] < 2) {
                print ::CONN_ERRORS "***\n$::partType[$i] with name $::xpartName[$i] has missing inputs\nRequires 2; Only $::partInCnt[$i] provided: \n";
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
	my ($i) = @_;
	return "";
}


# Return front end code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndCode {
	my ($i) = @_;
        my $calcExp = "// PHASEDEG:  $::xpartName[$i]\n";
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
