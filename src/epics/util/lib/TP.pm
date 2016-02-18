package CDS::TP;
use Exporter;
@ISA = ('Exporter');


sub partType {
	$::feInitCodeTP = "";
	return TP;
}


# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub printHeaderStruct {
        my ($i) = @_;
	# there is 500 "testpoint" array in controller.c
	die "Too many extra test points (max 499)\n" unless $::extraTPcount < 499;
	if ($::extraTPcount == 0) {
		# Add 16 per DAC
		$::extraTPcount += 16 * $::dacCnt;
	}
	$::extraTestPoints .= " $::xpartName[$i]";
	my $tpn = $::extraTPcount;
	$::feInitCodeTP .= "testpoint[$tpn] = &\L$::xpartName[$i];\n";
	$::feInitCodeTP .= "\L$::xpartName[$i] = .0;\n";
	$::extraTPcount ++;
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
	print ::OUT "static double \L$::xpartName[$i];\n";
}

# Check inputs are connected
sub checkInputConnect {
        my ($i) = @_;
	if($::partInCnt[$i] < 1) {
                print ::CONN_ERRORS "***\n$::partType[$i] with name $::xpartName[$i] has no input connected.\n\n";
        	return "ERROR";
        }
        return "";
}

# Return front end initialization code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndInitCode {
	my $a = $::feInitCodeTP;
	$::feInitCodeTP = undef;
	return $a
}

# Figure out part input code
# Argument 1 is the part number
# Argument 2 is the input number
# Returns calculated input code
sub fromExp {
	return ""; 
}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string

sub frontEndCode {
	my ($i) = @_;
        my $ret = "// Extra Test Point\n";
	$ret .= "\L$::xpartName[$i]";
        $ret .= " = $::fromExp[0];\n";
	return $ret;
}
