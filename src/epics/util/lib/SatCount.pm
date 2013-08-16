package CDS::SatCount;
use Exporter;
@ISA = ('Exporter');
 
sub partType {
        return SatCount;
}

# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub printHeaderStruct {
        my ($i) = @_;
        print ::OUTH "\tdouble $::xpartName[$i]\_RESET;\n";
        print ::OUTH "\tdouble $::xpartName[$i]\_TRIGGER;\n";
$here = <<END;
\tchar $::xpartName[$i]\_RESET_mask;\n
\tchar $::xpartName[$i]\_TRIGGER_mask;\n
END
	return $here;
}
 
# Print Epics variable definitions
# Current part number is passed as first argument
sub printEpics {
        my ($i) = @_;
        print ::EPICS "MOMENTARY $::xpartName[$i]\_RESET $::systemName\.$::xpartName[$i]\_RESET double ai 0\n";
        print ::EPICS "INVARIABLE $::xpartName[$i]\_TRIGGER $::systemName\.$::xpartName[$i]\_TRIGGER double ai 0 field(PREC,\"3\")\n";
}

# Print variable declarations int front-end file
# Current part number is passed as first argument
sub printFrontEndVars  {
        my ($i) = @_;
        print ::OUT "int \L$::xpartName[$i]\[2\];\n";
        print ::OUT "static int \L$::xpartName[$i]_first_time_through = 1;\n";
        print ::OUT "static int \L$::xpartName[$i]_total_counter;\n";
        print ::OUT "static int \L$::xpartName[$i]_running_counter;\n";
}

# Return front end initialization code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndInitCode {
        my ($i) = @_;
        my $calcExp = "if (\L$::xpartName[$i]_first_time_through) {\n";
        $calcExp .= "   \L$::xpartName[$i]_total_counter = 0;\n";
        $calcExp .= "   \L$::xpartName[$i]_running_counter = 0;\n";
        $calcExp .= "   \L$::xpartName[$i]_first_time_through = 0;\n";
        $calcExp .= "}\n";
        return $calcExp;
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

# Return front end code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndCode {
        my ($i) = @_;
        my $calcExp ="// SatCount:  $::xpartName[$i]\n";
        $calcExp .= "if (pLocalEpics->$::systemName\.$::xpartName[$i]\_RESET == 1) {\n";
        $calcExp .= "   \L$::xpartName[$i]_total_counter = 0;\n";
        $calcExp .= "   pLocalEpics->$::systemName\.$::xpartName[$i]\_RESET = 0;\n";
        $calcExp .= "}\n";
        $calcExp .= "else if (lfabs($::fromExp[0]) >= pLocalEpics->$::systemName\.$::xpartName[$i]\_TRIGGER) {\n";
        $calcExp .= "   \L$::xpartName[$i]_total_counter++;\n";
        $calcExp .= "   \L$::xpartName[$i]_total_counter%=100000000;\n";
        $calcExp .= "   \L$::xpartName[$i]_running_counter++;\n";
        $calcExp .= "   \L$::xpartName[$i]_running_counter%=100000000;\n";
        $calcExp .= "}\n";
        $calcExp .= "else {\n";
        $calcExp .= "   \L$::xpartName[$i]_running_counter = 0;\n";
        $calcExp .= "}\n";
        $calcExp .= "\L$::xpartName[$i]\[0\] = \L$::xpartName[$i]_total_counter;\n";
        $calcExp .= "\L$::xpartName[$i]\[1\] = \L$::xpartName[$i]_running_counter;\n";
        return $calcExp;
}

