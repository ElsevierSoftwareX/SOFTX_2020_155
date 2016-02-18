package CDS::RemoteIntlk;
use Exporter;
@ISA = ('Exporter');

sub partType {
	return RemoteIntlk;
}

# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub printHeaderStruct {
        my ($i) = @_;
	$temp = $::xpartName[$i];
	$temp =~ s/\-/\_/g;
        print ::OUTH "\tint $temp;\n";
}

# Print Epics variable definitions
# Current part number is passed as first argument
sub printEpics {
        my ($i) = @_;
	$temp = $::xpartName[$i];
	$temp =~ s/\-/\_/g;
        print ::EPICS "REMOTE_INTLK $::xpartName[$i] $::systemName\.$temp int ai 0 field(PREC,\"3\")\n";
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
        my ($i, $j) = @_;
        my $from = $::partInNum[$i][$j];
        return "pLocalEpics->" . $::systemName . "\." . $::xpartName[$from];
}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string

sub frontEndCode {
	my ($i) = @_;
        # print "Found EPICS REMOTE INTLK $::xpartName[$i] $::partInputType[$i][0] in loop\n";
	$temp = $::xpartName[$i];
	$temp =~ s/\-/\_/g;
        my $ret = "// RemoteIntlk:  $::xpartName[$i]\n";
        $ret .= "pLocalEpics->$::systemName\.$temp = ";
        $ret .= "$::fromExp[0];\n";
	return $ret;
}
