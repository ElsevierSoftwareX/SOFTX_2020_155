package CDS::FunctionCall;
use Exporter;
@ISA = ('Exporter');

sub partType {
	return FunctionCall;
}

# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub printHeaderStruct {
        my ($i) = @_;
	# Make sure input is a MUX and Output is DEMUX
	if ($::partInCnt[$i] != 1) {
		die "Part $::xpartName[$i] needs a single input\n";
	}
	if ($::partInputType[$i][0] ne "MUX") {
		die "Part $::xpartName[$i] needs a single MUX input, detected $::partInputType[$i][0]\n";
	}
	if ($::partOutCnt[$i] != 1) {
		die "Part $::xpartName[$i] needs a single output\n";
	}
	if ($::partOutputType[$i][0] ne "DEMUX") {
		die "Part $::xpartName[$i] needs a single MUX input, detected $::partOutputType[$i][0]\n";
	}
}

# Print Epics variable definitions
# Current part number is passed as first argument
sub printEpics {}

# Print variable declarations int front-end file
# Current part number is passed as first argument
sub printFrontEndVars  {
        my ($i) = @_;
	# Figure out input/output dimensions
	# declare data arrays
	print ::OUT "#include \"$::xpartName[$i].c\"\n";
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

        #"pLocalEpics->" . $::systemName . "\." . $::xpartName[$from];
	#print "FunctionCall from=$from; $::xpartName[$from]\n";
	return "";
}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string

sub frontEndCode {
	my ($i) = @_;
        #print "Found Function call $::xpartName[$i] $::partInputType[$i][0]\n";
        my $ret = "// Function Call\n";
        $ret .= "$::xpartName[$i](";
        $ret .= "$::fromExp[0], $::partInCnt[$::partInNum[$i][0]], \L$::partOutput[$i][0], $::partOutCnt[$::partOutNum[$i][0]]);\n";
	return $ret;
}
