package CDS::FunctionCall;
use Exporter;
@ISA = ('Exporter');

# This one gets node pointer (with all parsed info) as the first arg
# and as the second arguments this gets part number
sub partType {
        my ($node, $i) = @_;
	my $desc = ${$node->{FIELDS}}{"Description"};
	if ($desc =~ /^\/\/inline/) {
		$::inlinedFunctionCall[$i] = $desc;
	#print $desc;
	}
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
	# Only include code if not inlined
	if ($::inlinedFunctionCall[$i] eq undef) {
	  print ::OUT "#include \"$::xpartName[$i].c\"\n";
        }
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
	my $ret;
	# See if this function is inlined into Description
	#print $::inlinedFunctionCall[$i];
	if ($::inlinedFunctionCall[$i] ne undef) {
        	$ret = "// Inlined Function\n";
		$ret .= "{\n";
		my $s = $::inlinedFunctionCall[$i];
		$s =~ s/\\n/\n/g; # Replace '\n' with newline character
		$ret .= $s;
		$ret .= "\n";
	} else {
        	$ret = "// Function Call\n";
	}
	
        $ret .= "$::xpartName[$i](";
        $ret .= "$::fromExp[0], $::partInCnt[$::partInNum[$i][0]], \L$::partOutput[$i][0], $::partOutCnt[$::partOutNum[$i][0]]);\n";
	if ($::inlinedFunctionCall[$i] ne undef) {
		$ret .= "\n}\n";
	}
	return $ret;
}
