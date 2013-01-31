package CDS::Rio;
use Exporter;
@ISA = ('Exporter');

#//     \page Rio Rio.pm
#//     Documentation for Rio.pm
#//
#// \n


sub initRio {
	my ($node) = @_;
	$::boPartNum[$::boCnt] = $::partCnt;
	my $desc = ${$node->{FIELDS}}{"Name"};
	my $l = length($desc);
        my $num = substr($desc, ($l-1), 1);
        if ($num =~ m/\D/) {
           die "Last character of module name must be digit\: $desc\n";
        }
	$::boType[$::boCnt] = "ACS_8DIO";
	$::boNum[$::boCnt] = $num;
	$::boCnt ++;
}

sub partType {
	return Rio;
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

# Figure out part input code
# Argument 1 is the part number
# Argument 2 is the input number
# Returns calculated input code
sub fromExp {
        my ($i, $j) = @_;
	my $l = length($::partInput[$i][$j]);
        my $card =  substr($::partInput[$i][$j], ($l-1), 1);
	if ($::partInputPort[$i][$j] == 0) {
        	return "rioInputInput\[" . $card . "\]";
	} else {
        	return "rioInputOutput\[" . $card . "\]";
	}
}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string

sub frontEndCode {
	my ($i) = @_;
	my $l = length($::partName[$i][$j]);
        my $rioNum = substr($::partName[$i], ($l-1), 1);
        my $calcExp = "// Rio number is $rioNum name $::partName[$i]\n";
        my $fromType = $::partInputType[$i][$_];
        if (($fromType ne "GROUND") && ($::partInput[$i][0] ne "NC")) {
                $calcExp .= "rioOutput\[";
                $calcExp .= $rioNum;
                $calcExp .= "\] = ";
                $calcExp .= $::fromExp[0];
                $calcExp .= ";\n";
        }
        return $calcExp;

        return "";
}
