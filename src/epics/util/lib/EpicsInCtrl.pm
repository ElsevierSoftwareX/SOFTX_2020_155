package CDS::EpicsInCtrl;
use Exporter;
@ISA = ('Exporter');

#//     \page EpicsInCtrl EpicsInCtrl.pm
#//     Documentation for EpicsInCtrl.pm
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
	return EpicsInCtrl;
}

# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub printHeaderStruct {
        my ($i) = @_;
        print ::OUTH "\tdouble $::xpartName[$i];\n";
	return "\tchar $::xpartName[$i]_mask;\n";
}

# Print Epics variable definitions
# Current part number is passed as first argument
sub printEpics {
        my ($i) = @_;
	print ::EPICS "INVARIABLE $::xpartName[$i] $::systemName\.$::xpartName[$i] double ai 0 field(PREC,\"3\")";
        foreach $ef (@{$::epics_fields[$i]}) {
                print ::EPICS  " " . $ef;
        }
        print ::EPICS "\n";

}

# Print variable declarations int front-end file
# Current part number is passed as first argument
sub printFrontEndVars  {
        my ($i) = @_;
print ::OUT  <<END
unsigned int $::xpartName[$i]_gps;
unsigned int $::xpartName[$i]_cycle;
END
}

# Return front end initialization code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndInitCode {
        my ($i) = @_;
return  <<END
$::xpartName[$i]_gps = 0;
$::xpartName[$i]_cycle = 0;
END
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
return <<END
// See if mask transitions from 0 to 1, if so do it right away
// If mask transitions from 1 to 0, then need to delay this transition by one second
// to allow the sequencer code to pick up the new value.
{
	unsigned char mask = $::fromExp[0];
	unsigned char seq_mask = pLocalEpics->$::systemName\.$::xpartName[$i]_mask;
	if (seq_mask == 0) {
		if (mask == 1) {
			pLocalEpics->$::systemName\.$::xpartName[$i]_mask = mask;
			// Remember when the mask is allowed to get changed back to 0
			$::xpartName[$i]_gps = cycle_gps_time + 1;
			$::xpartName[$i]_cycle = cycleNum;
		}
	} else if (mask == 0) { // Delay transition from 1 to 0 if needed
		if ($::xpartName[$i]_gps) { // Currently waiting for transition from 1 to 0
			if ($::xpartName[$i]_gps < cycle_gps_time // We are far in the future
			    || ($::xpartName[$i]_gps == cycle_gps_time && $::xpartName[$i]_cycle <= cycleNum)) {
				// Done waiting for a transition, finish it up
				$::xpartName[$i]_gps = 0;
				pLocalEpics->$::systemName\.$::xpartName[$i]_mask = mask;
			}
		} else {
			// Setup transition
			$::xpartName[$i]_gps = cycle_gps_time + 1;
			$::xpartName[$i]_cycle = cycleNum;
		}
	}
	// Assign the value from the second input if masked
	if (pLocalEpics->$::systemName\.$::xpartName[$i]_mask) pLocalEpics->$::systemName\.$::xpartName[$i] = $::fromExp[1];
}
END
}
