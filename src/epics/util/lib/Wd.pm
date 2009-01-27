package CDS::Wd;
use Exporter;
@ISA = ('Exporter');

sub partType {
	return Wd;
}

# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub printHeaderStruct {
        my ($i) = @_;
        print ::OUTH "\tint $::xpartName[$i];\n";
        print ::OUTH "\tint $::xpartName[$i]_STAT;\n";
        print ::OUTH "\tint $::xpartName[$i]\_MAX;\n";
        print ::OUTH "\tfloat $::xpartName[$i]\_VAR\[$::partInCnt[$i]\];\n";
}

# Print Epics variable definitions
# Current part number is passed as first argument
sub printEpics {
        my ($i) = @_;
        #       print EPICS "INVARIABLE $::xpartName[$i] $::systemName\.$::xpartName[$i] int bi 0 field(ZNAM,\"OFF\") field(ONAM,\"ON\")\n";
        print ::EPICS "MOMENTARY $::xpartName[$i] $::systemName\.$::xpartName[$i] int ai 0\n";
        print ::EPICS "OUTVARIABLE $::xpartName[$i]\_STAT $::systemName\.$::xpartName[$i]_STAT int ai 0 \n";
	for (0 .. $::partInCnt[$i]-1) {
          my $a = 1 + $_;
          print ::EPICS "OUTVARIABLE $::xpartName[$i]\_VAR_$a $::systemName\.$::xpartName[$i]\_VAR\[$_\] float ai 0 field(PREC,\"1\")\n";
        }
        print ::EPICS "INVARIABLE $::xpartName[$i]\_MAX $::systemName\.$::xpartName[$i]\_MAX int ai 0 field(PREC,\"0\")\n";
}

# Print variable declarations int front-end file
# Current part number is passed as first argument
sub printFrontEndVars  {
        my ($i) = @_;
	print ::OUT "static int \L$::xpartName[$i];\n";
	print ::OUT "static float \L$::xpartName[$i]\_avg\[$::partInCnt[$i]\];\n";
        print ::OUT "static float \L$::xpartName[$i]\_var\[$::partInCnt[$i]\];\n";
        print ::OUT "float \L$::xpartName[$i]\_vabs;\n";
}

# Return front end initialization code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndInitCode {
        my ($i) = @_;
        return "\L$::xpartName[$i] = 0;\n"
        . "for \(ii=0; ii<$::partInCnt[$i]; ii++\) {\n"
        . "\t\L$::xpartName[$i]\_avg\[ii\] = 0.0;\n"
        . "\t\L$::xpartName[$i]\_var\[ii\] = 0.0;\n"
        . "}\n"
        . "pLocalEpics->$::systemName\." . $::xpartName[$i] . " = 1;\n";
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
        my $calcExp = "// Wd (Watchdog) MODULE\n";
        $calcExp .= "if((clock16K \% (FE_RATE/1024)) == 0) {\n";
        $calcExp .= "if (pLocalEpics->$::systemName\.";
        $calcExp .= $::xpartName[$i];
        $calcExp .= " == 1) {\n";
        $calcExp .= "\t\L$::xpartName[$i] = 1;\n";
        $calcExp .= "\tpLocalEpics->$::systemName\." . $::xpartName[$i] . " = 0;\n";
        $calcExp .= "};\n";
        $calcExp .= "double ins[$::partInCnt[$i]]= {\n";
        for (0 .. $::partInCnt[$i]-1) {
          $calcExp .= "\t$::fromExp[$_],\n";
        }
        $calcExp .= "};\n";
        $calcExp .= "   for\(ii=0; ii<$::partInCnt[$i];ii++\) {\n";
        $calcExp .= "\t\L$::xpartName[$i]\_avg\[ii\]";
        $calcExp .= " = ins[ii] * \.00005 + ";
        $calcExp .= "\L$::xpartName[$i]\_avg\[ii\] * 0\.99995;\n";
        $calcExp .= "\t\L$::xpartName[$i]\_vabs = ins[ii] - \L$::xpartName[$i]\_avg\[ii\];\n";
        $calcExp .= "\tif\(\L$::xpartName[$i]\_vabs < 0) \L$::xpartName[$i]\_vabs *= -1.0;\n";
        $calcExp .= "\t\L$::xpartName[$i]\_var\[ii\] = \L$::xpartName[$i]\_vabs * \.00005 + ";
        $calcExp .= "\L$::xpartName[$i]\_var\[ii\] * 0\.99995;\n";
        $calcExp .= "\tpLocalEpics->$::systemName\.";
        $calcExp .= $::xpartName[$i];
        $calcExp .= "_VAR\[ii\] = ";
        $calcExp .= "\L$::xpartName[$i]\_var\[ii\];\n";

        $calcExp .= "\tif(\L$::xpartName[$i]\_var\[ii\] \> ";
        $calcExp .= "pLocalEpics->$::systemName\.";
        $calcExp .= $::xpartName[$i];
        $calcExp .= "_MAX\) ";
        $calcExp .= "\L$::xpartName[$i] = 0;\n";
        $calcExp .= "   }\n";
        $calcExp .= "\tpLocalEpics->$::systemName\.";
        $calcExp .= $::xpartName[$i];
        $calcExp .= "_STAT = \L$::xpartName[$i];\n";
        return $calcExp . "}\n";
}
