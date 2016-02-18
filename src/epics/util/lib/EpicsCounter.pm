package CDS::EpicsCounter;
use Exporter;
@ISA = ('Exporter');
 
sub partType {
	return EpicsCounter;
}
 
# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub printHeaderStruct {
	my ($i) = @_;
	print ::OUTH "\tint $::xpartName[$i]\_RESET;\n";
	print ::OUTH "\tint $::xpartName[$i]\_COUNTEROUT;\n";
}
 
# Print Epics variable definitions
# Current part number is passed as first argument
sub printEpics {
	my ($i) = @_;
	print ::EPICS "MOMENTARY $::xpartName[$i]\_RESET $::systemName\.$::xpartName[$i]\_RESET int ao 0\n";
	print ::EPICS "OUTVARIABLE $::xpartName[$i]\_COUNTEROUT $::systemName\.$::xpartName[$i]\_COUNTEROUT int ao 0\n";
}
 
# Print variable declarations int front-end file
# Current part number is passed as first argument
sub printFrontEndVars  {
	my ($i) = @_;
}

# Check inputs are connected
sub checkInputConnect {
        my ($i) = @_;
        return "";
}

# Return front end initialization code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndInitCode {
	my ($i) = @_;
	my $calcExp = "pLocalEpics->$::systemName\.$::xpartName[$i]\_RESET = 0;\n"; 
	$calcExp .= "pLocalEpics->$::systemName\.$::xpartName[$i]\_COUNTEROUT = 0;\n";
	return $calcExp;
}
 
# Figure out part input code
# Argument 1 is the part number
# Argument 2 is the input number
# Returns calculated input code
sub fromExp {
	my ($i, $j) = @_;
	my $from = $::partInNum[$i][$j];
	return "pLocalEpics->" . $::systemName . "\." . $::xpartName[$from];
}
 
# Return front end code
# Argument 1 is the part number
# Returns calculated code string
 
sub frontEndCode {
	my ($i) = @_;
	my $calcExp = "// EpicsCounter: $::xpartName[$i]\n";
	$calcExp .= "if (pLocalEpics->$::systemName\.$::xpartName[$i]_RESET) {\n";
	$calcExp .= "\tpLocalEpics->$::systemName\.$::xpartName[$i]_COUNTEROUT = 0;\n";
	$calcExp .= "\tpLocalEpics->$::systemName\.$::xpartName[$i]_RESET = 0;\n";
	$calcExp .= "}\n";
	$calcExp .= "else {\n";
	$calcExp .= "\tif (cycle == 0) {\n";
	$calcExp .= "\t\tpLocalEpics->$::systemName\.$::xpartName[$i]_COUNTEROUT++;\n";
	$calcExp .= "\t}\n";
	$calcExp .= "}\n";
	return $calcExp;
}

