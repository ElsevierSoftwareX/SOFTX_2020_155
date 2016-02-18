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
	  print ::OUT qq(#include "$::xpartName[$i].c"\n);
	  my $dirname  = dirname($::cFile);
	  push @::sources, "$dirname/$::xpartName[$i].c";
        }
}

# Check inputs are connected
sub checkInputConnect {
        my ($i) = @_;
	my $headerFile = "$::rcg_src_dir\/src/include/ccodeio.h";
	my $found = 0;
	my $ins = $::partInCnt[$::partInNum[$i][0]];
	my $outs = $::partOutputs[$::partOutNum[$i][0]];
        my $search1 = "argin\\[0\\]";
        my $start = 0;
        my $inCnt = 0;
        my $outCnt = 0;
	my $inargUsed = [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0];
        my $outargUsed = [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0];
        my $maxCnt = 25;
	my $diagsTxt = "";
	my $pendingFail = 0;

	# First, try parsing the C file to determine number of inputs and outputs.
	if ($::inlinedFunctionCall[$i] ne undef) {
		($inline_keyword, $func_name, $pathed_name)
			= split(/\s/, $::inlinedFunctionCall[$i]);
		# Expand variables in $pathed_name
		# Note the usage of Env, all environment variables were
		# turned into Perl variables
		$pathed_name =~ s/(\$\w+)/$1/eeg;
	} else {
                print ::WARNINGS "***\nNOT A FUNCTION: C function in list. \n\t-File is $pathed_name\n";
		return "";
	}
	my $search = "void $func_name *\\(";

	open(my $fh,"<".$pathed_name) || die "Cannot find $pathed_name \n";

	while (my $line = <$fh>) {
                chomp $line;
# skip comment lines
                if ( $line =~ "^\t* *//" || $line =~ "^ *#" ) {
                        next;
                }
# skip out if end of function
                if ( $start == 1 && $line =~ "^}" ) {
                        #print "End of Function\n";
                        last;
                }
# skip out if beginning of another function
                if ( $start == 1 && $line =~ "void " ) {
                        #print "Start of next function\n";
                        last;
                }
                if ($start == 1 && $line =~ $invar) {
                        #print $line," \n";
                        for($ii=0;$ii<$maxCnt;$ii++) {
                                $jj = $ii + 1;
                                if ($line =~ $arginsearch[$ii]) {
                                        $inargUsed[$ii] = 1;
                                        if ($jj > $inCnt) {
                                                $inCnt = $jj;
                                        }
                                } 

                        }
                }
                if ($start == 1 && $line =~ $outvar) {
                        #print $line," \n";
                        for($ii=0;$ii<$maxCnt;$ii++) {
                                $jj = $ii + 1;
                                if ($line =~ $argoutsearch[$ii]) {
                                        $outargUsed[$ii] = 1;
                                        if ($jj > $outCnt) {
                                                $outCnt = $jj;
                                        }
                                }

                        }
                }
                if ($start == 0 && $line =~ $search) {
                        $diagsTxt .="Found start of function = $search\n\t";
			$diagsTxt .=  $line;
			$diagsTxt .= "\n";
                        #print $line, "\n";
                        my @words = split /[:*(,\s\/]+/,$line;
                        $invar = $words[3];
                        $outvar = $words[7];
                        # print " InargVar = ",$invar, " and OutargVar = ",$outvar,"\n";
                        for($ii=0;$ii<$maxCnt;$ii++) {
                                $arginsearch[$ii] = $invar . "\\[" . $ii . "\\]";
                                $argoutsearch[$ii] = $outvar . "\\[" . $ii . "\\]";
                        }
                        #print "BASE SEARCH = ",$search1, " and ", $arginsearch[0], "\n";
			$start = 1;
                }
        }


	close($fh);
	# print $diagsTxt;
        $inused = 0;
        for($ii=0;$ii<$inCnt;$ii++) {
                if($inargUsed[$ii]) { $inused ++; }
        }
        $outused = 0;
        for($ii=0;$ii<$outCnt;$ii++) {
                if($outargUsed[$ii]) { $outused ++; }
        }
	if($inCnt > 0 && $outCnt > 0 && ($ins != $inCnt || $outs != $outCnt)) {
                $pendingFail = 1;
	}

	if($ins != $inCnt || $outs != $outCnt || $ins != $inused || $outs != $outused) {
		if ($::inlinedFunctionCall[$i] ne undef) {
			($inline_keyword, $func_name, $pathed_name)
				= split(/\s/, $::inlinedFunctionCall[$i]);
			# Expand variables in $pathed_name
			# Note the usage of Env, all environment variables were
			# turned into Perl variables
			$pathed_name =~ s/(\$\w+)/$1/eeg;

			open(my $fh2,"<".$headerFile) || die "Cannot open $headerFile \n";
			while (my $line = <$fh2>) {
				chomp $line;
				my @word = split /[:*(,\s\t]+/,$line;
				if($word[0] eq $pathed_name and $word[1] eq $func_name) {
					$inCnt = $word[2];
					$outCnt = $word[3];
					if($inCnt == -1) {$inCnt = $ins;}
					if($outCnt == -1) {$outCnt = $outs;}
					$found = 1;
				}
			}
			close($fh2);
		}
	} else {
		$found = 1;
	}
        if(!$found && !$pendingFail) {
                print ::WARNINGS "***\nCannot parse C function $func_name in list. \n\t-File is $pathed_name\n";
                print ::WARNINGS "\tCannot find C function listed in ccodeio.h file.\n";
                return "";
        }
        if($ins != $inCnt || $outs != $outCnt) {
                print ::CONN_ERRORS "***\nC function has wrong number of inputs/outputs. \n\t- File is $pathed_name \n\t- Function is: $func_name\n\t- Code requires $inCnt inputs and model has $ins inputs\n\t- Code requires $outCnt outputs and model has $outs outputs\n";
                return "ERROR";
        }
        return "";
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
