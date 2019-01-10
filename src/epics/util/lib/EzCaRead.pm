package CDS::EzCaRead;
use Exporter;
@ISA = ('Exporter');

#//     \page EzCaRead ExCaRead.pm
#//     Documentation for EzCaRead.pm
#//
#// \n

sub partType {
	return EzCaRead;
}

# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub printHeaderStruct {
        my ($i) = @_;
	$temp = $::xpartName[$i];
	$temp =~ s/\-/\_/g;
	$temp =~ s/\:/\_/g;
        print ::OUTH "\tdouble $temp;\n";
	$temp .= "_CONN";
        print ::OUTH "\tdouble $temp;\n";
}

# Print Epics variable definitions
# Current part number is passed as first argument
sub printEpics {
        my ($i) = @_;
	$::remoteGpsPart = $i;
	$temp = $::xpartName[$i];
	$temp =~ s/\-/\_/g;
	$temp =~ s/\:/\_/g;
        #print ::EPICS "EZ_CA_READ $::xpartName[$i] $::partName[$i] $::systemName\.$temp int ai 0 field(PREC,\"3\")\n";
	# description has the original part name
        print ::EPICS "EZ_CA_READ $::xpartName[$i] $::blockDescr[$i] $::systemName\.$temp\n";
}

# Print variable declarations int front-end file
# Current part number is passed as first argument
sub printFrontEndVars  {
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
        return "";
}

# Figure out part input code
# Argument 1 is the part number
# Argument 2 is the input number
# Returns calculated input code
sub fromExp {
        my ($i, $j) = @_;
        my $from = $::partInNum[$i][$j];
	my $fromPort = $::partInputPort[$i][$j];
	my $temp = $::xpartName[$from];
	$temp =~ s/\-/\_/g;
	$temp =~ s/\:/\_/g;
	if ($fromPort == 0) {
        	return "pLocalEpics->" . $::systemName . "\." . $temp;
        } else {;
		$temp .= "_ERR";
        	return "pLocalEpics->" . $::systemName . "\." . $temp;
        }
}
sub remoteGps {
	my $i = @_;
	my $temp = $::xpartName[$::remoteGpsPart];
	$temp =~ s/\-/\_/g;
	$temp =~ s/\:/\_/g;
	print "Called EZCAREAD  $temp $i $::remoteGpsPart\n";
	return "pLocalEpics->" . $::systemName . "\." . $temp;
}
	

# Return front end code
# Argument 1 is the part number
# Returns calculated code string

sub frontEndCode {
	return "";
}
