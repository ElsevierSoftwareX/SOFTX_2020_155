package CDS::AND;
use Exporter;
@ISA = ('Exporter');

#//     \page AND AND.pm
#//     Documentation for AND.pm
#//
#// \n

sub partType {
        my ($node, $i) = @_;
	return AND;
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
	print ::OUT "int \L$::xpartName[$i];\n";
	print "ANDPART int \L$::xpartName[$i];\n";
}
# Check inputs are connected
sub checkInputConnect {
        my ($i) = @_;
	my $incnt = $::partInputs[$i];
	if($::partInCnt[$i] != $incnt) {
                print ::CONN_ERRORS "***\n$op with name $::xpartName[$i] has missing inputs\nRequires $incnt; Only $::partInCnt[$i] provided.\n";
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
	my $op = $::blockDescr[$i];
        unless ($op) { $op = "AND"; } # The default operator is AND
        my $calcExp = "// Logical $op\n";
	$calcExp .= "\L$::xpartName[$i]";
	$calcExp .= " = ";
	my $calcExp1 = "";
        my $cop = "";
        my $cnv = "";
        my $neg = 0;
	my $inCnt = $::partInCnt[$i];

	if ($op eq "AND") { $cop = " && "; }
                elsif ($op eq "OR") { $cop = " || "; }
                elsif ($op eq "NAND") { $cop = " && "; $neg = 1; }
                elsif ($op eq "NOR") { $cop = " || "; $neg = 1; }
                elsif ($op eq "XOR") { $cop = " ^ "; $cnv = "!!"}
                elsif ($op eq "NOT") { $cop = " error "; $neg = 1; }
                else { $cop = " && "; }
	for ($qq=0; $qq<$inCnt; $qq++) {
                $calcExp1 .= "$cnv(" . $::fromExp[$qq] . ")";
                if(($qq + 1) < $inCnt) { $calcExp1 .= " $cop "; }
        }
        if ($neg) {
		$calcExp .= "!(";
		$calcExp .= $calcExp1;
		$calcExp .= ");\n";
        } else {
		$calcExp .= $calcExp1;
		$calcExp .= ";\n";
	}
        return $calcExp;
}
