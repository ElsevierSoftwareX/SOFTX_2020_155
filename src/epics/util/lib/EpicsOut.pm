package CDS::EpicsOut;
use Exporter;
@ISA = ('Exporter');

sub partType {
	return EpicsOut;
}

# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub printHeaderStruct {
        my ($i) = @_;
        print ::OUTH "\tfloat $::xpartName[$i];\n";
}


# Print variable declarations int front-end file
# Current part number is passed as first argument
sub printFrontEndVars  {
        my ($i) = @_;
        ;
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
        #print "Found EPICS OUTPUT $::xpartName[$i] $::partInputType[$i][0] in loop\n";
        my $ret = "// EpicsOut\n";
        $ret .= "pLocalEpics->$::systemName\.$::xpartName[$i] = ";
        $ret .= "$::fromExp[0];\n";
	return $ret;
}
