package CDS::DacKillIop;
use Exporter;
@ISA = ('Exporter');

#//     \page DacKillIop DacKillIop.pm
#//     Documentation for DacKillIop.pm
#//
#// \n

sub partType {
	my ($i,$j) = @_;
	print "DACKILL IOP **************** \n";
	my $desc = ${$i->{FIELDS}}{"Description"};
	my ($num) = $desc =~ m/card_num=([^,]+)/g;
	print "\t Num = $num $j\n";
	my @vals = split(' ',$num);
	my $ii = 0;
	foreach (@vals) {
		$::dacKillMod[$::dacKillCnt][$ii] = int($_);
		print "\t\tDacKill $::dacKillCnt module $ii = $::dacKillMod[$::dacKillCnt][$ii] \n";
		$ii ++;
	}
	$::dacKillModCnt[$::dacKillCnt] = $ii;
	$::dacKillPartNum[$::dacKillCnt] = $j;
	print "\tDACKILL CARD COUNT = $::dacKillModCnt[$::dacKillCnt]\n";
	$::dacKillCnt ++;
	return DacKillIop;
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
int $MYNAME\_WDTIME;
int $MYNAME\_DTTIME;
END

	return "\tchar $MYNAME\_PANIC_mask;\n";
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
OUTVARIABLE $::xpartName[$i]\_WDTIME $::systemName\.$::xpartName[$i]\_WDTIME int ai 0 field(HIGH,\"1\") field(HSV,\"2\") \n
OUTVARIABLE $::xpartName[$i]\_DTTIME $::systemName\.$::xpartName[$i]\_DTTIME int ai 0 field(HIGH,\"1\") field(HSV,\"2\") \n
INVARIABLE $::xpartName[$i]\_PANIC $::systemName\.$::xpartName[$i]\_PANIC int bi 0 field(ZNAM,\"NORMAL\") field(ONAM,\"PANIC\") field(OSV,\"2\") \n
END
}


# Print variable declarations int front-end file
# Current part number is passed as first argument
sub printFrontEndVars  {
        my ($i) = @_;
        print ::OUT "static int \L$::xpartName[$i]\[2\];\n";
        print ::OUT "static int \L$::xpartName[$i]_remainingTime;\n";
        print ::OUT "static int \L$::xpartName[$i]_wdTime;\n";
        print ::OUT "static int \L$::xpartName[$i]_dtTime;\n";
        print ::OUT "static int \L$::xpartName[$i]_dacState;\n";
        print ::OUT "static int \L$::xpartName[$i]_dkState;\n";
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
	$ii = 0;
	$jj = 0;
	for($ii=0;$ii<$::dacKillCnt;$ii++) {
		if($::dacKillPartNum[$ii] == $i) {
			$jj = $ii;
			print "FOUND DACKKILL number $jj \n";
		}
	 }
	 my $DKERR = '';
	 my $SIGNAL = $::fromExp[0];
	 my $BPTIME = $::fromExp[1];
	 my $WDTIME = $::fromExp[2];
	 my $DTTIME = $::fromExp[3];
	 my $CYCLE = "cycle";
	 my $DACSTAT = "\L$::xpartName[$i]_dacState";
	 my $DKSTAT = "\L$::xpartName[$i]_dkState";
	 my $WDOUT = "\L$::xpartName[$i]\[0\]";
	 my $RSETOUT = "\L$::xpartName[$i]\[1\]";
	 my $BPTIME_REMAINING = "\L$::xpartName[$i]_remainingTime";
	 my $WDTIME_REMAINING = "\L$::xpartName[$i]_wdTime";
	 my $DTTIME_REMAINING = "\L$::xpartName[$i]_dtTime";
	 my $EPICS_STATE = "pLocalEpics->$::systemName\.$::xpartName[$i]\_STATE";
	 my $EPICS_BPTIME = "pLocalEpics->$::systemName\.$::xpartName[$i]\_BPTIME";
	 my $EPICS_WDTIME = "pLocalEpics->$::systemName\.$::xpartName[$i]\_WDTIME";
	 my $EPICS_DTTIME = "pLocalEpics->$::systemName\.$::xpartName[$i]\_DTTIME";
	 my $EPICS_BPSET = "pLocalEpics->$::systemName\.$::xpartName[$i]\_BPSET";
	 my $EPICS_RESET = "pLocalEpics->$::systemName\.$::xpartName[$i]\_RESET";
	 my $EPICS_PANIC = "pLocalEpics->$::systemName\.$::xpartName[$i]\_PANIC";
	 for($ii=0;$ii <$::dacKillModCnt[$jj];$ii++) {
		$DKERR .= "dacChanErr[$::dacKillMod[$jj][$ii]] = $DACSTAT; \n";
	 }

        return <<END;

// DACKILLER MODULE
// State 0 = killall
// State 1 = OK
// State 2 = Bypass
// State 3 = WD timer running
// State 4 = DAC timer running
/// Reset DAC Status word

$DACSTAT = 1;
/// Clear the Dackill reset output, if set
if ($RSETOUT) $RSETOUT = 0;

/// Check if reset sent from EPICS
if ($EPICS_RESET) {
	/// Reset WD output line to OK
	$WDOUT = 1;
	/// Clear Bypass timer
	$BPTIME_REMAINING = 0;
	/// Clear WD timer
	$WDTIME_REMAINING = $WDTIME;
	/// Clear DT timer
	$DTTIME_REMAINING = $DTTIME;
	/// Clear reset from EPICS as it is a momentary switch
	$EPICS_RESET = 0;
	/// Set the Dackill reset output line high (reset)
	$RSETOUT = 1;
	/// Set DK status as OK
	$DKSTAT = 1;
} 

/// Check if in Panic state request
if ($EPICS_PANIC)
{
	/// Set WD output to FAULT
	$WDOUT = 0;
	/// Set return to one to force zero output from DAC modules.
	$DACSTAT = 1;
	/// Clear Bypass Timer
	$BPTIME_REMAINING = 0;
	/// Clear WD Timer
	$WDTIME_REMAINING = 0;
	/// Clear DT Timer
	$DTTIME_REMAINING = 0;
	/// Send dackill status to EPICS
	$DKSTAT = 0;
} else {
	/// If Bypass timer is running
	if ($BPTIME_REMAINING > 0) {
		/// Set WD output to OK.
		$WDOUT = 1;
		/// Set DAC enable mode as OK.
		$DACSTAT = 0;
		/// Send dackill state as BYPASS to EPICS
		$DKSTAT = 2;
	/// If normal watchdog mode, fault at input
	} else if(!$SIGNAL)  {
		switch ($DKSTAT)
		{
			case 1:	// OK STATE
				$DKSTAT = 3;
				$WDTIME_REMAINING = $WDTIME;
				break;
			case 3:	// WD TIMER RUNNING STATE
				if ($WDTIME_REMAINING <= 0) {
					/// Set WD output to FAULT
					$WDOUT = 0;
					/// Set DK state to WD trip, DAC timer running.
					$DKSTAT = 4;
				}
				break;
			case 4:	// DAC TIMER RUNNING STATE
				if ($DTTIME_REMAINING <= 0) {
					/// KILL DAC Modules
					$DACSTAT = 1;
					/// Set DK state to WD trip, DAC timer running.
					$DKSTAT = 0;
				}
				break;
		}
	/// If normal watchdog mode, OK at input
	} else if($SIGNAL)  {
		switch ($DKSTAT)
		{
			case 3:	// WD TIMER RUNNING STATE
				$WDTIME_REMAINING = $WDTIME;
				/// Set DK state to OK.
				$DKSTAT = 1;
				break;
			case 4:	// DAC TIMER RUNNING STATE
				$DTTIME_REMAINING = $DTTIME;
				/// Set DK state to WD trip, DAC timer running.
				$DKSTAT = 4;
				break;
		}
	}
	/// If in Bypass mode and timer has expired.
	if ($EPICS_BPSET && (0 == $BPTIME_REMAINING)) {
		/// Reset the bypass mode timer.
		$BPTIME_REMAINING = $BPTIME;
		/// Clear Bypass mode.
		$EPICS_BPSET = 0;
		/// Send dackill state with BYPASS cleared
		$DKSTAT = 1;
	} 
}

$DKERR

/// On one second mark
if (!$CYCLE) {
	/// Decrement Bypass timer, if running.
	if($DKSTAT == 2) $BPTIME_REMAINING -= 1;
	/// Decrement WD timer, if running.
	if($DKSTAT == 3) $WDTIME_REMAINING -= 1;
	/// Decrement DAC timer, if running.
	if($DKSTAT == 4) $DTTIME_REMAINING -= 1;
	/// Send dackill state to EPICS
	$EPICS_STATE = $DKSTAT;
	/// Send Bypass timer value to EPICS
	$EPICS_BPTIME = $BPTIME_REMAINING;
	/// Send WD timer value to EPICS
	$EPICS_WDTIME = $WDTIME_REMAINING;
	/// Send DAC timer value to EPICS
	$EPICS_DTTIME = $DTTIME_REMAINING;
} 
END
}

