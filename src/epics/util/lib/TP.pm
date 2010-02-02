package CDS::TP;
use Exporter;
@ISA = ('Exporter');

sub partType {
	return TP;
}

# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub printHeaderStruct {
        my ($i) = @_;
	# there is 50 "testpoint" array in controller.c
	die "Too many extra test points (max 49)\n" unless $::extraTPcount < 49;
	$::extraTPcount ++;
	$::extraTestPoints .= " $::xpartName[$i]";
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
	print ::OUT "static float \L$::xpartName[$i];\n";
        ;
}

# Return front end initialization code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndInitCode {
        my ($i) = @_;
	my $tpn = $::extraTPcount;
	$ret = "testpoint[$tpn] = &\L$::xpartName[$i];\n";
	$ret .= "\L$::xpartName[$i] = .0;\n";
        return $ret;
}

# Figure out part input code
# Argument 1 is the part number
# Argument 2 is the input number
# Returns calculated input code
sub fromExp {
        my ($i, $j) = @_;
        my $from = $::partInNum[$i][$j];
	return ""; #\L$::xpartName[$i];
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
