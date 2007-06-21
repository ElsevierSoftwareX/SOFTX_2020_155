package CDS::Osc;
use Exporter;
@ISA = ('Exporter');

sub partType {
	return Osc;
}

# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub printHeaderStruct {
        my ($i) = @_;
        print ::OUTH "\tfloat $::xpartName[$i]\_FREQ;\n";
        print ::OUTH "\tfloat $::xpartName[$i]\_CLKGAIN;\n";
        print ::OUTH "\tfloat $::xpartName[$i]\_SINGAIN;\n";
        print ::OUTH "\tfloat $::xpartName[$i]\_COSGAIN;\n";
}

# Print Epics variable definitions
# Current part number is passed as first argument
sub printEpics {
        my ($i) = @_;
	print ::EPICS "INVARIABLE $::xpartName[$i]\_FREQ $::systemName\.$::xpartName[$i]\_FREQ float ai 0 field(PREC,\"1\")\n";
        print ::EPICS "INVARIABLE $::xpartName[$i]\_CLKGAIN $::systemName\.$::xpartName[$i]\_CLKGAIN float ai 0 field(PREC,\"1\")\n";
        print ::EPICS "INVARIABLE $::xpartName[$i]\_SINGAIN $::systemName\.$::xpartName[$i]\_SINGAIN float ai 0 field(PREC,\"1\")\n";
        print ::EPICS "INVARIABLE $::xpartName[$i]\_COSGAIN $::systemName\.$::xpartName[$i]\_COSGAIN float ai 0 field(PREC,\"1\")\n";
}


# Print variable declarations int front-end file
# Current part number is passed as first argument
sub printFrontEndVars  {
        my ($i) = @_;
        print ::OUT "static double \L$::xpartName[$i]\[3\];\n";
        print ::OUT "static double \L$::xpartName[$i]\_freq;\n";
        print ::OUT "static double \L$::xpartName[$i]\_delta;\n";
        print ::OUT "static double \L$::xpartName[$i]\_alpha;\n";
        print ::OUT "static double \L$::xpartName[$i]\_beta;\n";
        print ::OUT "static double \L$::xpartName[$i]\_cos_prev;\n";
        print ::OUT "static double \L$::xpartName[$i]\_sin_prev;\n";
        print ::OUT "static double \L$::xpartName[$i]\_cos_new;\n";
        print ::OUT "static double \L$::xpartName[$i]\_sin_new;\n";
        if ($::oscUsed == 0) {
          print ::OUT "double lsinx, lcosx, valx;\n";
          $::oscUsed = 1;
        }
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
        my $calcExp =  "\L$::xpartName[$i]_freq = ";
        $calcExp .=  "pLocalEpics->$::systemName\.$::xpartName[$i]\_FREQ;\n";
        $calcExp .=  "printf(\"OSC Freq = \%f\\n\",\L$::xpartName[$i]_freq\);\n";
        $calcExp .= "\L$::xpartName[$i]\_delta = 2.0 * 3.1415926535897932384626 * ";
        $calcExp .=  "\L$::xpartName[$i]_freq / \UFE_RATE;\n";
        $calcExp .= "valx = \L$::xpartName[$i]\_delta \/ 2.0;\n";
        $calcExp .= "sincos\(valx, \&lsinx, \&lcosx\);\n";
        $calcExp .= "\L$::xpartName[$i]\_alpha = 2.0 * lsinx * lsinx;\n";
        $calcExp .= "valx = \L$::xpartName[$i]\_delta\;\n";
        $calcExp .= "sincos\(valx, \&lsinx, \&lcosx\);\n";
        $calcExp .= "\L$::xpartName[$i]\_beta = lsinx;\n";
        $calcExp .= "\L$::xpartName[$i]\_cos_prev = 1.0;\n";
        $calcExp .= "\L$::xpartName[$i]\_sin_prev = 0.0;\n";
	return $calcExp;
}


# Return front end code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndCode {
	my ($i) = @_;
        my $calcExp = "// Osc\n";
        $calcExp .= "\L$::xpartName[$i]\_cos_new = \(1.0 - ";
        $calcExp .= "\L$::xpartName[$i]\_alpha\) * \L$::xpartName[$i]\_cos_prev - ";
        $calcExp .= "\L$::xpartName[$i]\_beta * \L$::xpartName[$i]\_sin_prev;\n";
        $calcExp .= "\L$::xpartName[$i]\_sin_new = \(1.0 - ";
        $calcExp .= "\L$::xpartName[$i]\_alpha\) * \L$::xpartName[$i]\_sin_prev + ";
        $calcExp .= "\L$::xpartName[$i]\_beta * \L$::xpartName[$i]\_cos_prev;\n";
        $calcExp .= "\L$::xpartName[$i]\_sin_prev = \L$::xpartName[$i]\_sin_new;\n";
        $calcExp .= "\L$::xpartName[$i]\_cos_prev = \L$::xpartName[$i]\_cos_new;\n";
        $calcExp .= "\L$::xpartName[$i]\[0\] = \L$::xpartName[$i]\_sin_new * ";
        $calcExp .= "pLocalEpics->$::systemName\.$::xpartName[$i]\_CLKGAIN;\n";
        $calcExp .= "\L$::xpartName[$i]\[1\] = \L$::xpartName[$i]\_sin_new * ";
        $calcExp .= "pLocalEpics->$::systemName\.$::xpartName[$i]\_SINGAIN;\n";
        $calcExp .= "\L$::xpartName[$i]\[2\] = \L$::xpartName[$i]\_cos_new * ";
        $calcExp .= "pLocalEpics->$::systemName\.$::xpartName[$i]\_COSGAIN;\n";

        $calcExp .= "if((\L$::xpartName[$i]_freq \!= ";
        $calcExp .= "pLocalEpics->$::systemName\.$::xpartName[$i]\_FREQ) \&\& ";
        $calcExp .= "((clock16K + 1) == \UFE_RATE))\n";
        $calcExp .= "{\n";
        $calcExp .= "\t\L$::xpartName[$i]_freq = ";
        $calcExp .= "pLocalEpics->$::systemName\.$::xpartName[$i]\_FREQ;\n";
        $calcExp .= "\tprintf(\"OSC Freq = \%f\\n\",\L$::xpartName[$i]_freq\);\n";
        $calcExp .= "\t\L$::xpartName[$i]\_delta = 2.0 * 3.1415926535897932384626 * ";
        $calcExp .= "\L$::xpartName[$i]_freq / \UFE_RATE;\n";
        $calcExp .= "\tvalx = \L$::xpartName[$i]\_delta \/ 2.0;\n";
        $calcExp .= "\tsincos\(valx, \&lsinx, \&lcosx\);\n";
        $calcExp .= "\t\L$::xpartName[$i]\_alpha = 2.0 * lsinx * lsinx;\n";
        $calcExp .= "\tvalx = \L$::xpartName[$i]\_delta\;\n";
        $calcExp .= "\tsincos\(valx, \&lsinx, \&lcosx\);\n";
        $calcExp .= "\t\L$::xpartName[$i]\_beta = lsinx;\n";
        $calcExp .= "\t\L$::xpartName[$i]\_cos_prev = 1.0;\n";
        $calcExp .= "\t\L$::xpartName[$i]\_sin_prev = 0.0;\n";
        return $calcExp . "}\n";
}
