package CDS::EpicsOut;
use Exporter;
@ISA = ('Exporter');

#//     \page EpicsOut EpicsOut.pm
#//     Documentation for EpicsOut.pm
#//
#// \n

sub partType {
        my ($node, $i) = @_;
	$desc = ${$node->{FIELDS}}{"Description"};
	# Pull out all Epics fields from the description
	$desc =~ s/\s//g;
	# Get rid of backslashes inserted by Matlab in field name around quotes.
	$desc =~ s/\\"/"/g;
	my @l = $desc =~ m/(field\([^\)]*\))/g;
	$::epics_fields[$i] = [@l];
	return EpicsOut;
}

# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub printHeaderStruct {
        my ($i) = @_;
        print ::OUTH "\tdouble $::xpartName[$i];\n";
}

# Print Epics variable definitions
# Current part number is passed as first argument
sub printEpics {
        my ($i) = @_;

        print ::EPICS "OUTVARIABLE $::xpartName[$i] $::systemName\.$::xpartName[$i] double ao 0 field(PREC,\"3\")";
	foreach $ef (@{$::epics_fields[$i]}) {
		print ::EPICS  " " . $ef;
        }
	print ::EPICS "\n";
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
        my $from = $::partInNum[$i][$j];
        return "pLocalEpics->" . $::systemName . "\." . $::xpartName[$from];
}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string

sub frontEndCode {
	my ($i) = @_;
        #print "Found EPICS OUTPUT $::xpartName[$i] $::partInputType[$i][0] in loop\n";
        my $ret = "// EpicsOut:  $::xpartName[$i]\n";
        $ret .= "pLocalEpics->$::systemName\.$::xpartName[$i] = ";
        $ret .= "$::fromExp[0];\n";
	return $ret;
}
