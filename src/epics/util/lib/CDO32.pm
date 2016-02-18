package CDS::CDO32;
use Exporter;
@ISA = ('Exporter');

sub initCDO32 {
	my ($node) = @_;
	$::boPartNum[$::boCnt] = $::partCnt;
	my $desc = ${$node->{FIELDS}}{"Name"};
	my $l = length($desc);
        my $CDO32Num = substr($desc, ($l-1), 1);
        if ($CDO32Num =~ m/\D/) {
           die "Last character of module name must be digit\: $desc\n";
        } 
	$::boType[$::boCnt] = "CON_32DO";
	$::boNum[$::boCnt] = $CDO32Num;
	$::boCnt ++;
}

sub partType {
	return CDO32;
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
        my $card =  substr($::partInput[$i][$j], ($l-1), 1);
	if ($::partInputPort[$i][$j] == 0)
	{
        	return "(CDO32Input\[" . $card . "\] >> 16)";
	} else {
        	return "(CDO32Input\[" . $card . "\] & 0xffff)";
	}
}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string

sub frontEndCode {
	my ($i) = @_;
	my $l = length($::partName[$i]);
        my $CDO32Num = substr($::partName[$i], ($l-1), 1);
        my $calcExp = "// CDO32 number is $CDO32Num name $::partName[$i]\n";
        my $fromType = $::partInputType[$i][$_];
        if (($fromType ne "GROUND") && ($::partInput[$i][0] ne "NC")) {
                $calcExp .= "CDO32Output\[";
                $calcExp .= $CDO32Num;
                $calcExp .= "\] = ((int)";
                $calcExp .= $::fromExp[1];
                $calcExp .= " << 16) + ((int)";
                $calcExp .= $::fromExp[0];
		$calcExp .= " & 0xffff);\n";
        }
        return $calcExp;

        return "";
}
