package CDS::RelationalOperator;
use Exporter;
@ISA = ('Exporter');

#//     \page RelationalOperator RelationalOperator.pm
#//     Documentation for RelationalOperator.pm
#//
#// \n

sub partType {
        my ($node, $i) = @_;
	return RelationalOperator;
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
		$::xpartName[$i] =~ s/\\n/_/g;
	print ::OUT "double \L$::xpartName[$i] = 0.0;\n";
}

# Check inputs are connected
sub checkInputConnect {
        my ($i) = @_;
	if($::partInCnt[$i] < 2) {
                print ::CONN_ERRORS "***\n$::partType[$i] with name $::xpartName[$i] has missing inputs\nRequires 2; Only $::partInCnt[$i] provided.\n";
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
	if ($op eq "~=") { $op = "!="; };
	my $calcExp = "// Relational Operator\n";
        $calcExp .= "\L$::xpartName[$i]". " = (($::fromExp[0]) $op ($::fromExp[1]));";
        return $calcExp;
}
