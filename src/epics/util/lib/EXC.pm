package CDS::EXC;
use Exporter;
@ISA = ('Exporter');

#//     \page EXC EXC.pm
#//     Documentation for EXC.pm
#//
#// \n


sub partType {
	$::extraEXCcount = 0;
	return EXC;
}

# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub printHeaderStruct {
        my ($i) = @_;
	# there is 50 "testpoint" array in controller.c
	die "Too many extra excitations (max 49)\n" unless $::extraEXCcount < 49;
	my $key = $::xpartName[$i];
	$::extraEXCmap{$key} = $::extraEXCcount;
	$::extraEXCcount ++;
	$::extraExcitations .= " $key";
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
	#print ::OUT "static double \L$::xpartName[$i];\n";
        ;
}

# Return front end initialization code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndInitCode {
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
	my $excNum = $::extraEXCmap{$::xpartName[$from]};
	return "xExc[$excNum]";
}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string

sub frontEndCode {
	my ($i) = @_;
        my $ret = "// Extra Test Point\n";
	#$ret .= "\L$::xpartName[$i]";
	#$ret .= " = $::fromExp[0];\n";
	#return $ret;
	return "";
}
