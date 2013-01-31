package CDS::FunctionCall;
use Exporter;
use Env;
use File::Basename;

#//     \page FunctionCall FunctionCall.pm
#//     Documentation for FunctionCall.pm
#//
#// \n


@ISA = ('Exporter');

# This one gets node pointer (with all parsed info) as the first arg
# and as the second arguments this gets part number
sub partType {
        my ($node, $i) = @_;
	my $desc = ${$node->{FIELDS}}{"Description"};
	if ($desc =~ /^inline/) {
		if (3 != split(/\s+/, $desc)) {
			$a = split(/\s+/, $desc);
			die "Part $::xpartName[$i] needs three fields in Description: inline funcName \$ENV_VAR/path/to/file/filename.c $a\n";
		}
		$::inlinedFunctionCall[$i] = $desc;
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
	  my $dirname  = dirname($::cFile);
	  push @::sources, "$dirname/$::xpartName[$i].c";
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
                $ret = "// Inlined Function:  $::xpartName[$i]\n";
		$ret .= "{\n";
		my ($inline_keyword, $func_name, $pathed_name)
			= split(/\s/, $::inlinedFunctionCall[$i]);
		# Expand variables in $pathed_name
		# Note the usage of Env, all environment variables were
		# turned into Perl variables
		$pathed_name =~ s/(\$\w+)/$1/eeg;
		$ret .= "#define CURRENT_SUBSYS $::subSysName[$::partSubNum[$i]]\n";
		$ret .= "#include \"$pathed_name\"\n";
		push @::sources, $pathed_name;
		$ret .= "$func_name(";
	} else {
                $ret = "// Function Call:  $::xpartName[$i]\n";
        	$ret .= "$::xpartName[$i](";
	}
	
        $ret .= "$::fromExp[0], $::partInCnt[$::partInNum[$i][0]], \L$::partOutput[$i][0], $::partOutputs[$::partOutNum[$i][0]]);\n";
	if ($::inlinedFunctionCall[$i] ne undef) {
		$ret .= "#undef CURRENT_SUBSYS\n";
		$ret .= "}\n";
	}
	return $ret;
}
