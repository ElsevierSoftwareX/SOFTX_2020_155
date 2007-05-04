package CDS::Bitwise;
use Exporter;
@ISA = ('Exporter');

sub partType {
	return Bitwise;
}

# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub printHeaderStruct {
}

# Print Epics variable definitions
# Current part number is passed as first argument
sub printEpics {
}


# Print variable declarations int front-end file
# Current part number is passed as first argument
sub printFrontEndVars  {
        my ($i) = @_;
        print ::OUT "unsigned int \L$::xpartName[$i];\n";
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
	my $op = "";
	if ("\L$::xpartName[$i]" =~ /^and/) {
	  $op = "&";
	} elsif ("\L$::xpartName[$i]" =~ /^or/) {
	  $op = "|";
	} elsif ("\L$::xpartName[$i]" =~ /^xor/) {
	  $op = "^";
	}
        my $calcExp = "// Bitwise $op\n";
        $calcExp .= "\L$::xpartName[$i] = ";
        $calcExp .= "((unsigned int)(". $::fromExp[0] . "))$op((unsigned int)(" . $::fromExp[1] ."))";
        $calcExp .= ";\n";
	return $calcExp;
}
