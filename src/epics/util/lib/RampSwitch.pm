package CDS::RampSwitch;
use Exporter;
@ISA = ('Exporter');

sub partType {
	return RampSwitch;
}

# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub printHeaderStruct {
        my ($i) = @_;
	print ::OUTH "\tint $::xpartName[$i];\n";
	return "\tchar $::xpartName[$i]_mask;\n";
}

# Print Epics variable definitions
# Current part number is passed as first argument
sub printEpics {
        my ($i) = @_;
	print ::EPICS "INVARIABLE $::xpartName[$i] $::systemName\.$::xpartName[$i] int bi 0 field(ZNAM,\"OFF\") field(ONAM,\"ON\")\n";
}


# Print variable declarations int front-end file
# Current part number is passed as first argument
sub printFrontEndVars  {
        my ($i) = @_;
	print ::OUT "double \L$::xpartName[$i]\[4\];\n";
}

# Check inputs are connected
sub checkInputConnect {
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
        my $fromPort = $::partInputPort[$i][$j];
        return "\L$::xpartName[$from]". "\[" . $fromPort . "\]";
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
        my $calcExp = "// RampSwitch:  $::xpartName[$i]\n";
        #$calcExp .= "\L$::xpartName[$i]";
        #$calcExp .= " = ";
        #$calcExp .= $::fromExp[0];
        #$calcExp .= ";\n";
        for(my $qq=0; $qq < $::inCnt; $qq++) {
          $calcExp .= "\L$::xpartName[$i]\[";
          $calcExp .= $qq;
          $calcExp .= "\] = ";
          $calcExp .= $::fromExp[$qq];
          $calcExp .= ";\n";
        }
        $calcExp .= "if (pLocalEpics->$::systemName\.$::xpartName[$i] == 0)\n";
        $calcExp .= "{\n";
        $calcExp .= "\t\L$::xpartName[$i]\[1\] = \L$::xpartName[$i]\[2\];\n";
        $calcExp .= "}\n";
        $calcExp .= "else\n";
        $calcExp .= "{\n";
        $calcExp .= "\t\L$::xpartName[$i]\[0\] = \L$::xpartName[$i]\[1\];\n";
        $calcExp .= "\t\L$::xpartName[$i]\[1\] = \L$::xpartName[$i]\[3\];\n";
        $calcExp .=  "}\n\n";
	return $calcExp;
}
