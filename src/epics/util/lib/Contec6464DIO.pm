package CDS::Contec6464DIO;
use Exporter;
@ISA = ('Exporter');

sub initCDIO6464 {
	my ($node) = @_;
	$::boPartNum[$::boCnt] = $::partCnt;
	my $desc = ${$node->{FIELDS}}{"Name"};
	my $l = length($desc);
        my $CDIO6464num = substr($desc, ($l-1), 1);
        if ($CDIO6464num =~ m/\D/) {
           die "Last character of module name must be digit\: $desc\n";
        } 
	$::boType[$::boCnt] = "CON_6464DIO";
	$::boNum[$::boCnt] = $CDIO6464num;
	$::boCnt ++;
}

sub partType {
	return Contec6464DIO;
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

# Return front end initialization code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndInitCode {
	my ($i) = @_;
        return "";
}

# Check inputs are connected
sub checkInputConnect {
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
       	return "CDIO6464InputInput\[" . $card . "\]";
}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string

sub frontEndCode {
	if($::iopModel != 1) {
	my ($i) = @_;
	my $l = length($::partName[$i]);
        my $CDIO6464Num = substr($::partName[$i], ($l-1), 1);
        my $calcExp = "// CDIO6464 number is $CDIO6464Num name $::partName[$i]\n";
        my $fromType = $::partInputType[$i][$_];
        if (($fromType ne "GROUND") && ($::partInput[$i][0] ne "NC")) {
                $calcExp .= "CDIO6464Output\[";
                $calcExp .= $CDIO6464Num;
                $calcExp .= "\] = ";
                $calcExp .= $::fromExp[0];
		$calcExp .= ";\n";
        }
        return $calcExp;
	}

        return "";
}
