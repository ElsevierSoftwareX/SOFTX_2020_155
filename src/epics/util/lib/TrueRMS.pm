package CDS::TrueRMS;
use Exporter;
@ISA = ('Exporter');
 
sub partType {
	return TrueRMS;
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
	print ::OUTH "#define \U$::xpartName[$i]\_WINSZ \t $::windowSize\n\n";
	print ::OUT "static int \L$::xpartName[$i]\_first\_time\_through;\n";
	print ::OUT "int \L$::xpartName[$i]\_index;\n";
	print ::OUT "int \L$::xpartName[$i]\_n;\n";
	print ::OUT "double \L$::xpartName[$i];\n";
	print ::OUT "double \L$::xpartName[$i]\_indatsqrd\[\U$::xpartName[$i]\_WINSZ\];\n";
	print ::OUT "double \L$::xpartName[$i]\_sqrsum;\n";
	print ::OUT "double \L$::xpartName[$i]\_sqrval;\n";
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
	my $calcExp = "\L$::xpartName[$i]";
	$calcExp .= " = 0.0;\n";
	$calcExp .= "\L$::xpartName[$i]\_first\_time\_through";
	$calcExp .= " = 1;\n";
	$calcExp .= "\L$::xpartName[$i]\_n";
	$calcExp .= " = ";
	$calcExp .= "1;\n";
	$calcExp .= "\L$::xpartName[$i]\_index";
	$calcExp .= " = ";
	$calcExp .= "0;\n";
	return $calcExp;
}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndCode {
	my ($i) = @_;
	my $calcExp = "// TrueRMS:  $::xpartName[$i]\n";
	$calcExp .= "if (\L$::xpartName[$i]\_first\_time\_through) {\n";
	$calcExp .= "\t\L$::xpartName[$i]\_first\_time\_through";
	$calcExp .= " = 0;\n";
	$calcExp .= "\t\L$::xpartName[$i]";
	$calcExp .= " = ";
	$calcExp .= $::fromExp[0];
	$calcExp .= ";\n";
	$calcExp .= "\t\L$::xpartName[$i]\_sqrsum";
	$calcExp .= " = ";
	$calcExp .= $::fromExp[0];
	$calcExp .= " * ";
	$calcExp .= $::fromExp[0];
	$calcExp .= ";\n";
	$calcExp .= "\t\L$::xpartName[$i]\_indatsqrd\[0\]";
	$calcExp .= " = ";
	$calcExp .= "\L$::xpartName[$i]\_sqrsum;\n";
	$calcExp .= "}\nelse {\n";
	$calcExp .= "\tif (\L$::xpartName[$i]\_n < \U$::xpartName[$i]\_WINSZ) {\n";
	$calcExp .= "\t\t\L$::xpartName[$i]\_index";
	$calcExp .= " = ";
	$calcExp .= "\L$::xpartName[$i]\_n++;\n";
	$calcExp .= "\t}\n\telse {\n";
	$calcExp .= "\t\t\L$::xpartName[$i]\_index";
	$calcExp .= " = ";
	$calcExp .= "(1+\L$::xpartName[$i]\_index)\%\U$::xpartName[$i]\_WINSZ;\n";
	$calcExp .= "\t\t\L$::xpartName[$i]\_sqrsum";
	$calcExp .= " -= ";
	$calcExp .= "\L$::xpartName[$i]\_indatsqrd\[\L$::xpartName[$i]\_index\];\n";
	$calcExp .= "\t}\n";
	$calcExp .= "\t\L$::xpartName[$i]\_indatsqrd\[\L$::xpartName[$i]\_index\]";
	$calcExp .= " = ";
	$calcExp .= $::fromExp[0];
	$calcExp .= " * ";
	$calcExp .= $::fromExp[0];
	$calcExp .= ";\n";
	$calcExp .= "\t\L$::xpartName[$i]\_sqrsum";
	$calcExp .= " += ";
	$calcExp .= "\L$::xpartName[$i]\_indatsqrd\[\L$::xpartName[$i]\_index\];\n";
	$calcExp .= "\t\L$::xpartName[$i]\_sqrval = \L$::xpartName[$i]\_sqrsum\/(double) \L$::xpartName[$i]\_n;\n";
	$calcExp .= "\tif (\L$::xpartName[$i]\_sqrval > 0.0)  \n";
	$calcExp .= "\t\t\L$::xpartName[$i]";
	$calcExp .= " = ";
	$calcExp .= "lsqrt(\L$::xpartName[$i]\_sqrval);\n";
	$calcExp .= "}\n";
	return $calcExp;
}

