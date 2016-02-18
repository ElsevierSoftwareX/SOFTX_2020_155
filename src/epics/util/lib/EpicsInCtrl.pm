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
unsigned int $::xpartName[$i]_gps;   // GPS time of the last value update
unsigned int $::xpartName[$i]_cycle; // Cycle number of the last valid value update
double $::xpartName[$i]_last; // Last valid value received from the FE
END
}

# Check inputs are connected
sub checkInputConnect {
        my ($i) = @_;
	if($::partInCnt[$i] < 2) {
                print ::CONN_ERRORS "***\n$::partType[$i] with name $::xpartName[$i] has missing inputs\nRequires 2; Only $::partInCnt[$i] provided:  Please ground any unused inputs\n";
        return "ERROR";
        }
        return "";
}

# Return front end initialization code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndInitCode {
        my ($i) = @_;
return  <<END
$::xpartName[$i]_gps = 0;
$::xpartName[$i]_cycle = 0;
$::xpartName[$i]_last = 0;
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
//
// mask==1 : means the value is under the FE control
// mask==0 : means the value is under the EPICS control
//
// See if mask transitions from 0 to 1, if so do it right away
// If mask transitions from 1 to 0, then need to delay this transition by one second
// to allow the sequencer code to pick up the new value.
// Code keeps sending the last valid value received from the second input for at least one second.
//
{
	unsigned char mask = $::fromExp[0]; // Part's mask input (from the FE)
	unsigned char seq_mask = pLocalEpics->$::systemName\.$::xpartName[$i]_mask; // Shared memory mask variable (to EPICS), EPICS never writes it
	if (!seq_mask) pLocalEpics->$::systemName\.$::xpartName[$i]_mask = mask; // if seq_mask == 0, transition right away (if mask == 1)
	else if (!mask) { // if mask == 0 && seq_mask == 1 then delay transition from 1 to 0 if needed	
		if ($::xpartName[$i]_gps) { // Currently waiting for transition from 1 to 0
			if ($::xpartName[$i]_gps < cycle_gps_time // We are far in the future
			    || ($::xpartName[$i]_gps == cycle_gps_time && $::xpartName[$i]_cycle <= cycleNum)) {
				// Done waiting for a transition, finish it up
				$::xpartName[$i]_gps = 0;
				pLocalEpics->$::systemName\.$::xpartName[$i]_mask = 0;
			} else {
				// Keep sending the last good value
				pLocalEpics->$::systemName\.$::xpartName[$i] = $::xpartName[$i]_last;
			}
		} else {
			// Setup transition
			$::xpartName[$i]_gps = cycle_gps_time + 1;
			$::xpartName[$i]_cycle = cycleNum;
		}
	}
	if (pLocalEpics->$::systemName\.$::xpartName[$i]_mask && mask) { // Case when we are sending the value to the EPICS
		// See if the value is changing and remember the time when the mask is allowed to be set to 0
		if (pLocalEpics->$::systemName\.$::xpartName[$i] != $::fromExp[1]) {
			$::xpartName[$i]_gps = cycle_gps_time + 1;
			$::xpartName[$i]_cycle = cycleNum;
		}
		// Assign the value from the second input 
		pLocalEpics->$::systemName\.$::xpartName[$i] = $::fromExp[1];
		// Remember the last known good value from the input
		$::xpartName[$i]_last = $::fromExp[1];
	}
}
END
}
