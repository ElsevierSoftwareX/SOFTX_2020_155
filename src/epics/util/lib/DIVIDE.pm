package CDS::DIVIDE;
use Exporter;
@ISA = ('Exporter');

#//     \page DIVIDE DIVIDE.pm
#//     Documentation for DIVIDE.pm
#//
#// \n

sub partType {
        my ($node, $i) = @_;
	return DIVIDE;
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
	# Use the length of the calculation string to determine number of inputs
	my $nc = length($::partInputs[$i]);
	print "DIVIDE ins = $::xpartName[$i] $nc  $::partInCnt[$i]\n";
	if($::partInCnt[$i] != $nc) {
                print ::CONN_ERRORS "***\n$::partType[$i] with name $::xpartName[$i] has missing inputs\nRequires $nc; Only $::partInCnt[$i] provided.\n";
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
	my $from, $to;
	my $numIn = length($::partInputs[$i]);

	my @chars = split('',$::partInputs[$i]);
	my $jj = 0;
	my $ii;
	my $ptr = 0;
	my @fn;
	my @operator;
	# Find the multiplies first
	foreach $ii (@chars) {
		if($ii eq "*") {
			if($ptr > 0) {$operator[($ptr - 1)] = "*"; }
			$fn[$ptr] = $jj;
			$ptr ++;
		}
		$jj ++;
	}
	$jj = 0;
	# Find the divides
	foreach $ii (@chars) {
		if($ii eq "/") {
			if($ptr > 0) {$operator[($ptr - 1)] = "/"; }
			$fn[$ptr] = $jj;
			$ptr ++;
		}
		$jj ++;
	}
	print "$::xpartName[$i] PTR = $ptr \n";
	my $math = "";
	my $nq = 0;
	my @quals;
	my $calcExp = "//DIVIDE \n";
	for($ii=0;$ii<$ptr;$ii++) {
		$math .= $::fromExp[$fn[$ii]] ;
		$math .= " ";
		$math .= $operator[$ii] ;
		$math .= " ";
		# If we have any divides, will need to verify the specified variables are not zero.
		# If any are zero, we will not do calc and output 0.0.
		if($operator[$ii] eq "/") {
			if($nq == 0) {
				$calcExp .= "if( ";
				$calcExp .= $::fromExp[$fn[($ii+1)]]
			}
			if($nq > 0) {
				$calcExp .= " && ";
				$calcExp .= $::fromExp[$fn[($ii+1)]]
			}
			$calcExp .= " != 0.0 ";
			$nq ++;
		}
	}
	if($nq > 0) {
		$calcExp .= ") {\n   ";
	}
	$calcExp .= "\L$::xpartName[$i]\E = ";
	$calcExp .= $math;
	$calcExp .= ";\n";
	if($nq > 0) {
		$calcExp .= "} else { \n   ";
		$calcExp .= "\L$::xpartName[$i]\E = 0.0; \n} \n";
	}

        return $calcExp;
}
