package CDS::Contec1616DIO;
use Exporter;
@ISA = ('Exporter');

sub initCDIO1616 {
	my ($node) = @_;
	$::boPartNum[$::boCnt] = $::partCnt;
	my $desc = ${$node->{FIELDS}}{"Name"};
	my $l = length($desc);
        my $CDIO1616num = substr($desc, ($l-1), 1);
        if ($CDIO1616num =~ m/\D/) {
           die "Last character of module name must be digit\: $desc\n";
        } 
	$::boType[$::boCnt] = "CON_1616DIO";
	$::boNum[$::boCnt] = $CDIO1616num;
	$::boCnt ++;
}

sub partType {
	return Contec1616DIO;
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
       	return "(CDIO1616InputInput\[" . $card . "\] & 0xffff)";
}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string

sub frontEndCode {
	my ($i) = @_;
	my $l = length($::partName[$i]);
        my $CDIO1616Num = substr($::partName[$i], ($l-1), 1);
        my $calcExp = "// CDIO1616 number is $CDIO1616Num name $::partName[$i]\n";
        my $fromType = $::partInputType[$i][$_];
	my $l = ($::fromExp[0] eq undef || $::fromExp[0] eq "") ? "0" : $::fromExp[0];
        if (($fromType ne "GROUND") && ($::partInput[$i][0] ne "NC")) {
                $calcExp .= "CDIO1616Output\[";
                $calcExp .= $CDIO1616Num;
                $calcExp .= "\] = ((int)";
                $calcExp .= $l;
                $calcExp .= " << 16) + ((int)";
                $calcExp .= $l;
		$calcExp .= " & 0xffff);\n";
        }
        return $calcExp;

        return "";
}
