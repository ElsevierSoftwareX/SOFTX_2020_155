package CDS::Rio;
use Exporter;
@ISA = ('Exporter');


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

	#print "partOutputPortUsed = ", $::partOutputPortUsed[$i][0], "\n";
	#print "partOutputPortUsed = ", $::partOutputPortUsed[$i][1], "\n";
	print "DBG: partOutputPortUsed = ", $::partOutputPortUsed[$i][0], "\n";
	print "DBG: partOutputPortUsed = ", $::partOutputPortUsed[$i][1], "\n";
	my $rioReadInitVal = 0;
	# First output has  digital inputs
	if ($::partOutputPortUsed[$i][0] != undef) {
	  if ($::partOutputPortUsed[$i][0] == 0) { $rioReadInitVal |= 1; }
	  if ($::partOutputPortUsed[$i][0] == 1) { $rioReadInitVal |= 2; }
	}
	# print "rioReadInitVal = ", $rioReadInitVal, "\n";
	# Second output has digital outputs
	if ($::partOutputPortUsed[$i][1] != undef) {
	  if ($::partOutputPortUsed[$i][1] == 0) { $rioReadInitVal |= 1;}
	  if ($::partOutputPortUsed[$i][1] == 1) { $rioReadInitVal |= 2; }
	}

	# print "rioReadInitVal = ", $rioReadInitVal, "\n";
	# Initialize board read operations selector
	# my $bnum = $::boNum[$::boCnt - 1]; # This board number
        # my $bnum = $::boNum[$i] - 1;
        my $bnum = -1;
        for (0 .. $::maxDioMod-1) {
           if ($::boPartNum[$_] == $i) {
              $bnum = $_;
           }
        }
        if ($bnum == -1) {
           die "ERROR: Rio index not found\n";
        }
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
