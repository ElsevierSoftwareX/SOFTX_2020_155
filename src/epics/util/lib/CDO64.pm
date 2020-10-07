package CDS::CDO64;
use Exporter;
@ISA = ('Exporter');

#//     \page CDO64 CDO64.pm
#//     Documentation for CDO64.pm
#//
#// \n

sub initCDO64 {
	my ($node) = @_;
	$::boPartNum[$::boCnt] = $::partCnt;
	my $desc = ${$node->{FIELDS}}{"Description"};
	my ($CDO64num) = $desc =~ m/card_num=([^,]+)/g;
        if ($CDO64num =~ m/\D/) {
           die "Must have card number defined: $desc\n";
        } 
	$::boType[$::boCnt] = "CDO64";
	$::boNum[$::boCnt] = $CDO64num;
	$::card2array[$::partCnt] = $::boCnt;
	$::boCnt ++;
	$::bo64Cnt ++;
}

sub partType {
	return CDO64;
}

# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub printHeaderStruct {
        my ($i) = @_;
        ;
}

# Print Epics variable definitions
# Current part number is passed as first argument
sub printEpics {
        my ($i) = @_;
        ;
}

# Print variable declarations int front-end file
# Current part number is passed as first argument
sub printFrontEndVars  {
        my ($i) = @_;
        ;
}

# Check inputs are connected
sub checkInputConnect {
        my ($i) = @_;
	if($::partInCnt[$i] < 1) {
                print ::CONN_ERRORS "***\n$::partType[$i] with name $::xpartName[$i] has no input connected.\n\n";
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
	my $l = length($::partInput[$i][$j]);
        #my $card =  substr($::partInput[$i][$j], ($l-1), 1);
       	return "CDO64InputInput\[" . $::card2array[$i] . "\]";
}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string

sub frontEndCode {
	my ($i) = @_;
	my $CDO64Num = $::card2array[$i];
        my $calcExp = "// CDO64 number is $CDO64Num name $::partName[$i]\n";
        my $fromType = $::partInputType[$i][$_];
        if (($fromType ne "GROUND") && ($::partInput[$i][0] ne "NC")) {
                $calcExp .= "CDIO6464Output\[";
                $calcExp .= $CDO64Num;
                $calcExp .= "\] = ";
                $calcExp .= $::fromExp[0];
		$calcExp .= ";\n";
        }
        return $calcExp;

        return "";
}
