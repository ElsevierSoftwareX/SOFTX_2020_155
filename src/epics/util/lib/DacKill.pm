package CDS::DacKill;
use Exporter;
@ISA = ('Exporter');

#//     \page DacKill DacKill.pm
#//     Documentation for DacKill.pm
#//
#// \n

sub partType {
	$::dacKillCnt ++;
	return DacKill;
}

# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub printHeaderStruct {
        my ($i) = @_;
	my $MYNAME = $::xpartName[$i];
	print ::OUTH <<END;
int $MYNAME\_STATE;
int $MYNAME\_RESET;
int $MYNAME\_BPSET;
int $MYNAME\_BPTIME;
int $MYNAME\_PANIC;
END

}

# Print Epics variable definitions
# Current part number is passed as first argument
sub printEpics {
        my ($i) = @_;
	print ::EPICS <<END;
OUTVARIABLE $::xpartName[$i]\_STATE $::systemName\.$::xpartName[$i]\_STATE int ai 0 field(HIGH,\"1\") field(HSV,\"2\") \n
MOMENTARY $::xpartName[$i]\_RESET $::systemName\.$::xpartName[$i]\_RESET int ai 0
MOMENTARY $::xpartName[$i]\_BPSET $::systemName\.$::xpartName[$i]\_BPSET int ai 0
OUTVARIABLE $::xpartName[$i]\_BPTIME $::systemName\.$::xpartName[$i]\_BPTIME int ai 0 field(HIGH,\"1\") field(HSV,\"2\") \n
INVARIABLE $::xpartName[$i]\_PANIC $::systemName\.$::xpartName[$i]\_PANIC int bi 0 field(ZNAM,\"NORMAL\") field(ONAM,\"PANIC\") field(OSV,\"2\") \n
END
}


# Print variable declarations int front-end file
# Current part number is passed as first argument
sub printFrontEndVars  {
        my ($i) = @_;
        print ::OUT "static int \L$::xpartName[$i]\[2\];\n";
        print ::OUT "static int \L$::xpartName[$i]_remainingTime;\n";
}

# Figure out part input code
# Argument 1 is the part number
# Argument 2 is the input number
# Returns calculated input code
sub fromExp {
        my ($i, $j) = @_;
        my $from = $::partInNum[$i][$j];
        my $fromPort = $::partInputPort[$i][$j];
        # return "\L$::xpartName[$from]";
	return "\L$::xpartName[$from]" . "\[" . $fromPort . "\]";
}

# Return front end initialization code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndInitCode {
	my ($i) = @_;
	return "";
}


# Return front end code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndCode {
	my ($i) = @_;

	 my $SIGNAL = $::fromExp[0];
	 my $BPTIME = $::fromExp[1];
	 my $DACSTAT = "dacFault";
	 my $CYCLE = "cycle";
	 my $STATE = "\L$::xpartName[$i]\[0\]";
	 my $RSETOUT = "\L$::xpartName[$i]\[1\]";
	 my $BPTIME_REMAINING = "\L$::xpartName[$i]_remainingTime";
	 my $EPICS_STATE = "pLocalEpics->$::systemName\.$::xpartName[$i]\_STATE";
	 my $EPICS_BPTIME = "pLocalEpics->$::systemName\.$::xpartName[$i]\_BPTIME";
	 my $EPICS_BPSET = "pLocalEpics->$::systemName\.$::xpartName[$i]\_BPSET";
	 my $EPICS_RESET = "pLocalEpics->$::systemName\.$::xpartName[$i]\_RESET";
	 my $EPICS_PANIC = "pLocalEpics->$::systemName\.$::xpartName[$i]\_PANIC";

        return <<END;

// DACKILLER MODULE
if ($RSETOUT) $RSETOUT = 0;
if ($EPICS_RESET) {
$STATE = 1;
$BPTIME_REMAINING = 0;
$EPICS_RESET = 0;
$RSETOUT = 1;
} 
if ($EPICS_PANIC)
{
$STATE = 0;
$DACSTAT = 0;
$BPTIME_REMAINING = 0;
} else {
if ($BPTIME_REMAINING > 0) {
$STATE = 1;
$DACSTAT = 2;
} else {
$STATE = ($STATE & (int)$SIGNAL);
$DACSTAT = $STATE;
}
if ($EPICS_BPSET && (0 == $BPTIME_REMAINING)) {
$BPTIME_REMAINING = $BPTIME;
$EPICS_BPSET = 0;
} 
}
if (!$CYCLE) {
if($BPTIME_REMAINING) $BPTIME_REMAINING -= 1;
$EPICS_STATE = $DACSTAT;
$EPICS_BPTIME = $BPTIME_REMAINING;
} 
END
}

