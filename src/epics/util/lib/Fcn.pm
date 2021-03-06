package CDS::Fcn;
use Exporter;
@ISA = ('Exporter');

#//     \page Fcn Fcn.pm
#//     Documentation for Fcn.pm
#//
#// \n

sub partType {
    my ($node, $i) = @_;
    my $expr = ${$node->{FIELDS}}{"Expr"};
	my $expr2 = "";
	if (!length($expr)) {
		$expr = $::defFcnExpr;
 	}
    $::functionExpr[$i] = $expr;
    $expr =~ s/u\((\d+)\)/u$1/g;
	print "EXP $expr \n";
	my $offset = 0;
	my $el = length($expr);
	if($expr =~ m/\^/) {
		my $lp = index($expr,"^",$offset) - 1;
		my $chk = rindex($expr,")",$lp);
		if($lp == $chk) {
			my $stc = rindex($expr,"(",$lp);
			my $sl = $lp - $stc + 1;
			my $newexp = substr $expr, $stc,$sl;
			my $ne1 = substr $expr, 0,$stc;
			my $es = $lp + 3;
			my $exp = $lp + 2;
			my $mult = int(substr $expr, $exp,1);
			my $ne2 = substr $expr,$es;
			$expr2 = $ne1;
			$expr2 .= "("; 
			$expr2 .= $newexp; 
			$expr2 .= "*"; 
			$expr2 .= $newexp; 
			$expr2 .= ")"; 
			$expr2 .= $ne2; 
		}
		$::functionExpr[$i] = $expr2;
	}
	if ($::functionExpr[$i] =~ m/\^/) {
		die "\nRCG Compile Error ******\nPart $::xpartName[$i] \nFcn does not support exponents \n $::functionExpr[$i] \n$expr\n";
	}

	return Fcn;
}

# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub printHeaderStruct {
        my ($i) = @_;
	# Make sure input is a MUX
	if ($::partInCnt[$i] != 1) {
		die "Part $::xpartName[$i] needs a single input\n";
	}
	if ($::partInputType[$i][0] ne "MUX") {
		die "Part $::xpartName[$i] needs a single MUX input, detected $::partInputType[$i][0]\n";
	}
}

# Print Epics variable definitions
# Current part number is passed as first argument
sub printEpics {
}

# Print variable declarations int front-end file
# Current part number is passed as first argument
sub printFrontEndVars  {
        my ($i) = @_;
        print ::OUT "double \L$::xpartName[$i];\n";
        my $expr = $::functionExpr[$i];
        while ($expr =~ /cos|sin/gc) {
           $::trigCnt++;
           print ::OUT "double lcos$::trigCnt, lsin$::trigCnt;\n";
        }
}

# Check inputs are connected
sub checkInputConnect {
        my ($i) = @_;
	my $inCnt = $::partInCnt[$i]; 
	# Do a basic check that part has an input connection.
	if($inCnt == 0) {
                print ::CONN_ERRORS "***\n$::partType[$i] with name $::xpartName[$i] is missing an input connection \n";
        	return "ERROR";
	}
	# Get the function equation
        my $expr = $::functionExpr[$i];
        $expr =~ s/\[(\d+)\]/$1-1/eg;
	# Split up the function string
	my @words = split /[:*(,\s\/]+/,$expr;
	my $maxIndex = 0;
	# Search for u, the function variable, and count indexes
	foreach my $lc (@words) {
		if (index($lc, "u") != -1) {
			# print "$lc \n";
			my @nw = split /u/,$lc;
			my $id = int($nw[1]);
			if($id > $maxIndex) {
				$maxIndex = $id;
			}
		}
	}
	$maxIndex ++;
	# Typical input is from MUX, so see how many inputs provided to part connected to Fcn input
	# Get number of inputs to part feeding Fcn
	my $numin = $::partInNum[$i][0];
	# Get number of inputs to that part.
	my $muxChans = $::partInCnt[$numin];
	# Verify MUX input has at least as many connections into it as used in the Fcn part.
	if($maxIndex > $muxChans) {
                print ::CONN_ERRORS "***\n$::partType[$i] with name $::xpartName[$i] has missing inputs\nRequires $maxIndex; Only $muxChans provided. Function defined as: $expr\n";
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
        return "";
}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndCode {
	my ($i) = @_;
	my $muxName = "\L$::xpartName[$::partInNum[$i][0]]";
        my $calcExp = "// Inline Function:  $::xpartName[$i]\n";
        my $expr = $::functionExpr[$i];
        $deg2rad = "";
        $trigExp = "";
        $expr =~ s/\[(\d+)\]/$1-1/eg;
             #3\.141592654/180\.0
	if ($expr =~ m/(cos|sin)deg/) {
		$deg2rad .= "(3.141592654/180.0)*";
	}
        $expr =~ s/(cos|sin)deg\(u(\d+)\)#/$1\(u$2\)/gx;
        $expr =~ s/(cos|sin)\(u(\d+)\)(?{$::trigOut++;})
                   (?{$trigExp .= "sincos\($deg2rad\(u$2\), &lsin$::trigOut, &lcos$::trigOut\);\n";})
                  /l$1$::trigOut/gx;
        $expr =~ s/u(\d+)/$muxName\[$1\]/g;
        $expr =~ s/u\((\d+)\)/$muxName\[$1\]/g;
        $trigExp =~ s/u(\d+)/$muxName\[$1\]/g;
        $expr =~ s/fabs/lfabs/g;
        $expr =~ s/log10/llog10/g;
        $expr =~ s/sqrt/lsqrt/g;
        $calcExp .= $trigExp;
        $calcExp .= "\L$::xpartName[$i] = ";
        $calcExp .= $expr;
        $calcExp .= ";\n";

        return $calcExp;
}
