package CDS::Product;
use Exporter;
@ISA = ('Exporter');

sub partType {
	return Product;
}

# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub printHeaderStruct {
        my ($i) = @_;
	print ::OUTH "\tdouble $::xpartName[$i];\n";
        print ::OUTH "\tint $::xpartName[$i]\_TRAMP;\n";
        print ::OUTH "\tint $::xpartName[$i]\_RMON;\n";
}

# Print Epics variable definitions
# Current part number is passed as first argument
sub printEpics {
        my ($i) = @_;
	print ::EPICS "INVARIABLE $::xpartName[$i] $::systemName\.$::xpartName[$i] double ai 0 field(PREC,\"3\")\n";
        print ::EPICS "INVARIABLE $::xpartName[$i]\_TRAMP $::systemName\.$::xpartName[$i]\_TRAMP int ai 0 field(PREC,\"0\")\n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_RMON $::systemName\.$::xpartName[$i]\_RMON int ao 0 field(PREC,\"0\")\n";
}


# Print variable declarations int front-end file
# Current part number is passed as first argument
sub printFrontEndVars  {
        my ($i) = @_;
        print ::OUT "double \L$::xpartName[$i]\[8\];\n";
        print ::OUT "double $::xpartName[$i]\_CALC;\n";
}

# Figure out part input code
# Argument 1 is the part number
# Argument 2 is the input number
# Returns calculated input code
sub fromExp {
        my ($i, $j) = @_;
        my $from = $::partInNum[$i][$j];
        my $fromPort = $::partInputPort[$i][$j];
        return "\L$::xpartName[$from]" . "\[" . $fromPort . "\]";
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
        my $calcExp = "// PRODUCT:  $::xpartName[$i]\n";
        $calcExp .= "pLocalEpics->$::systemName\.$::xpartName[$i]";
        $calcExp .= "_RMON = \n\tgainRamp(pLocalEpics->$::systemName\.$::xpartName[$i],";
        $calcExp .= "pLocalEpics->$::systemName\.$::xpartName[$i]";
        $calcExp .= "_TRAMP,";
        $calcExp .= "$::gainCnt\,\&$::xpartName[$i]\_CALC, FE_RATE);";
        $calcExp .= "\n\n";
        for (0 .. $::inCnt-1) {
          $calcExp .= "\L$::xpartName[$i]";
          $calcExp .= "\[";
          $calcExp .= $_;
          $calcExp .= "\] = ";
          $calcExp .= "$::xpartName[$i]";
          $calcExp .= "_CALC * ";
          $calcExp .= $::fromExp[$_];
          $calcExp .= ";\n";
        }
        $::gainCnt ++;
	return $calcExp;
}
