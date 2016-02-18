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
	if (!length($expr)) {
		$expr = $::defFcnExpr;
 	}
        $::functionExpr[$i] = $expr;
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
	my $numin = $::partInNum[$i][0];
	# Find the MUX that feeds this part.
	my $muxName = "\L$::xpartName[$::partInNum[$i][0]]";
	# Determine number of MUX inputs
	my $muxChans = $::partInCnt[$numin];
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
	# print "Num inputs required = $maxIndex $muxChans\n";
	# Number of matrix inputs should equal number of variables indexed by function.
	if($maxIndex != $muxChans) {
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
