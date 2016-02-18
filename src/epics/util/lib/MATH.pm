package CDS::MATH;
use Exporter;
@ISA = ('Exporter');

#//     \page MATH MATH.pm
#//     Documentation for MATH.pm
#//
#// \n

sub partType {
        my ($node, $i) = @_;
	return MATH;
}

# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub printHeaderStruct {
        my ($i) = @_;
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
	print ::OUT "double \L$::xpartName[$i];\n";
}

# Check inputs are connected
sub checkInputConnect {
        my ($i) = @_;
	my $inCnt = 1;
    	my $op = $::partInputs[$i];

	if ($op eq "mod") {
		$inCnt = 2;
	}
	if($inCnt != $::partInCnt[$i]) {
		print ::CONN_ERRORS "***\n$::partType[$i] with name $::xpartName[$i] has missing inputs\nRequires $inCnt; $::partInCnt[$i] provided:  \n";
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
        return "\L$::xpartName[$from]";
}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string

sub frontEndCode {
	my ($i) = @_;
    	my $op = $::partInputs[$i];

        if ($op eq "square") { 
	return <<SQEND;
// MATH FUNCTION - SQUARE
\L$::xpartName[$i] = $::fromExp[0] * $::fromExp[0];
SQEND
        }

        if ($op eq "sqrt") { 
	return <<SQRTEND;
// MATH FUNCTION - SQUARE ROOT
if ($::fromExp[0] > 0.0) {
	\L$::xpartName[$i] = lsqrt($::fromExp[0]);
}
else {
	\L$::xpartName[$i] = 0.0;
}
SQRTEND
        }

        if ($op eq "reciprocal") { 
	return <<RECEND;
// MATH FUNCTION - RECIPROCAL
if ($::fromExp[0] != 0.0) {
	\L$::xpartName[$i] = 1.0/$::fromExp[0];
}
else {
	\L$::xpartName[$i] = 0.0;
}
RECEND
        }

        if ($op eq "mod") { 
	return <<MODEND;
// MATH FUNCTION - MODULO
if ((int) $::fromExp[1] != 0) {
	\L$::xpartName[$i] = (double) ((int)$::fromExp[0] % (int)$::fromExp[1]);
}
else {
	\L$::xpartName[$i] = 0.0;
}
MODEND
        }

        if ($op eq "log10") { 
return <<LOGEND;
// MATH FUNCTION - LOG10
	\L$::xpartName[$i] = llog10($::fromExp[0]);
LOGEND
        }
}
