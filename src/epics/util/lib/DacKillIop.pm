package CDS::DacKillIop;
use Exporter;
@ISA = ('Exporter');

#//     \page DacKillIop DacKillIop.pm
#// \b Description: \n
#// This part is used as part of a watchdog shutdown system. It is an enhancement of the basic DacKill part.\n 
#// The primary differences in the use of the DacKillIop (DKIOP) and the DacKill (DK) part are: \n
#// - Multiple DKIOP parts are allowed within a single model, whereas use of the DK part is limited to one per model.
#// - The DK part shuts down all connected DAC outputs on a trip. Each DKIOP part can be directed to shut down one or more individual DAC modules, as defined by the user in the part Description field.
#// - The DKIOP part has two additional input connections.
#//		- WD Trip Time: Time, in seconds, that a fault (0) indication at the Sig input must remain prior to the producing a fault (0) at the WD output.
#//		- DAC Trip Time: Time, in seconds, that a fault (0) indication at the Sig input must remain beyond the WD Trip Time before the DKIOP part shuts down the DAC module outputs.
#//
#// \n
#// \b Operation: \n
#// - A fault indication (0) must be present at Sig input consistently for the duration of time, in seconds, as set by the WD Trip Time input before any action is taken. 
#//		- If the fault indication is removed prior to this time, the timer is reset to start again on the next fault indication.
#// - Once the WD Trip timer expires:
#//		- WD output goes to zero (Fault). 
#//		- This output is latched in the fault condition ie a clearing of a fault indication at the Sig input at this point will not clear the fault nor reset the WD timer.
#//		- DAC outputs continue normal operation.
#//		- Second fault timer is started, based on time, in seconds, as defined at the DAC Trip Time Input.
#//		- Removal of a fault indication at Sig input will reset this timer to begin again on the next fault indication. Therefore, if the corrective action taken by code initiated by the WD fault output is successful, then the shutdown of DAC module outputs is prevented.
#// - Once the DAC Trip timer expires:
#//		- Outputs of all defined DAC modules are set, and latched, to zero.
#//		- Removal of fault indication at Sig input will not clear this, nor the WD fault output.
#//		- Requires DKIOP Reset command and clearing of fault indication at Sig input to clear the trip condition.
#//
#// \n
#// \b Restrictions: \n
#// - As presently coded, this part may only be used in an IOP model.
#//		- For use in a user application model, the controller.c code with 'ifdef ADC_SLAVE' must be changed in the DAC write code section.
#//
#// \n

#// \b SUBROUTINES ************************* \n\n

#// \b sub \b partType \n 
#// Required subroutine for RCG \n
#// - Checks that this part is only used in IOP model. \n
#// - Loads DAC modules to be controlled by this part. \n
#// 
#// Returns DacKillIop \n\n
sub partType {
	my ($i,$j) = @_;
	if($::adcMaster < 0) {
                die "***ERROR: DACKILL IOP parts can only be used in IOP models\n";
        }
	my $desc = ${$i->{FIELDS}}{"Description"};
	my ($num) = $desc =~ m/card_num=([^ ]+)/g;
	my @vals = split(',',$num);
	my $ii = 0;
	foreach (@vals) {
		$::dacKillMod[$::dacKillCnt][$ii] = int($_);
		print "\t\tDacKillIop $::dacKillCnt module $ii = $::dacKillMod[$::dacKillCnt][$ii] \n";
		$ii ++;
	}
	$::dacKillModCnt[$::dacKillCnt] = $ii;
	$::dacKillPartNum[$::dacKillCnt] = $j;
	$::dacKillCnt ++;
	return DacKillIop;
}

#// \b sub \b printHeaderStruct \n 
#// Required subroutine for RCG \n
#// Print Epics communication structure into a header file \n
#// Current part number is passed as first argument \n\n
#// This part passes 6 integer type values: \n
#// - _STATE = DacKill Status. \n
#// 	- 0 = Tripped or PANIC \n
#//		- WD output = 0 and DAC modules shutdown \n
#// 	- 1 = OK  \n
#//		- WD output = 1 and DAC modules outputs run normal.\n
#// 	- 2 = BYPASS Mode \n
#// 		- WD output = 1 and DAC modules outputs run normal. \n
#// 	- 3 = Fault at Sig and WD timer running \n
#// 		- WD output = 1 and DAC modules outputs run normal. \n
#// 	- 4 = Fault at Sig and DAC shutdown timer running \n
#//		- WD output = 0 and DAC modules outputs run normal. \n
#// - _RESET = DacKill reset. \n
#// - _BPSET = Starts Bypass Timer. \n
#// - _BPTIME = Reports Bypass Time remaining. \n
#// - _PANIC = Forces DacKill part into State 0. \n
#// - _WDTIME = Reports Watchdog Time remaining. \n
#// - _DTTIME = Reports time to DAC shutdown. \n
#//

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

#// \b sub \b printEpics \n 
#// Required subroutine for RCG \n
#// Current part number is passed as first argument \n
#// - Writes out EPICS variables for this part. \n
#// - Variable ordering MUST match list in printHeaderStruct above. \n
#//
sub printEpics {
        my ($i) = @_;
	print ::EPICS <<END;
OUTVARIABLE $::xpartName[$i]\_STATE $::systemName\.$::xpartName[$i]\_STATE int ai 0 field(HIGH,\"2\") field(HSV,\"MINOR\") field(HHSV,\"MAJOR\") field(HIHI,\"3\") field(LOW,\"0\") field(LSV,\"MAJOR\") \n
MOMENTARY $::xpartName[$i]\_RESET $::systemName\.$::xpartName[$i]\_RESET int ai 0
MOMENTARY $::xpartName[$i]\_BPSET $::systemName\.$::xpartName[$i]\_BPSET int ai 0
INVARIABLE $::xpartName[$i]\_PANIC $::systemName\.$::xpartName[$i]\_PANIC int bi 0 field(ZNAM,\"NORMAL\") field(ONAM,\"PANIC\") field(OSV,\"2\") \n
OUTVARIABLE $::xpartName[$i]\_BPTIME $::systemName\.$::xpartName[$i]\_BPTIME int ai 0 field(HIGH,\"1\") field(HSV,\"2\") \n
OUTVARIABLE $::xpartName[$i]\_WDTIME $::systemName\.$::xpartName[$i]\_WDTIME int ai 0 field(LOW,\"0\") field(LSV,\"MAJOR\") \n
OUTVARIABLE $::xpartName[$i]\_DTTIME $::systemName\.$::xpartName[$i]\_DTTIME int ai 0 field(LOW,\"0\") field(LSV,\"MAJOR\") \n
END
}


#// \b sub \b printFrontEndVars \n 
#// Required subroutine for RCG \n
#// Current part number is passed as first argument \n
#// -  Defines local variables for user C code.
#//
sub printFrontEndVars  {
        my ($i) = @_;
        print ::OUT "static int \L$::xpartName[$i]\[3\];\n";
        print ::OUT "static int \L$::xpartName[$i]_remainingTime;\n";
        print ::OUT "static int \L$::xpartName[$i]_wdTime;\n";
        print ::OUT "static int \L$::xpartName[$i]_dtTime;\n";
        print ::OUT "static int \L$::xpartName[$i]_dacState;\n";
        print ::OUT "static int \L$::xpartName[$i]_dkState;\n";
}

# Check inputs are connected
sub checkInputConnect {
        my ($i) = @_;
	if($::partInCnt[$i] < 4) {
                print ::CONN_ERRORS "***\n$::partType[$i] with name $::xpartName[$i] has missing inputs\nRequires 4; Only $::partInCnt[$i] provided.\n";
                return "ERROR";
        }
        return "";
}


# Figure out part input code
# Returns calculated input code
#// \b sub \b fromExp \n 
#// Required subroutine for RCG \n
#// - Argument 1 is the part number \n
#// - Argument 2 is the input number \n
#// - Returns code required for input by another part.
#//		- Either WD output or RESET output.
#//
sub fromExp {
        my ($i, $j) = @_;
        my $from = $::partInNum[$i][$j];
        my $fromPort = $::partInputPort[$i][$j];
        # return "\L$::xpartName[$from]";
	return "\L$::xpartName[$from]" . "\[" . $fromPort . "\]";
}

#// \b sub \b frontEndInitCode \n 
#// Required subroutine for RCG \n
#// - Argument 1 is the part number \n
#// - Returns C code initialization code\n
#//		- No init required for this part.
#//
sub frontEndInitCode {
	my ($i) = @_;
	return "";
}


# Return front end code
# Argument 1 is the part number
# Returns calculated code string
#// \b sub \b frontEndCode \n 
#// Required subroutine for RCG \n
#// - Argument 1 is the part number \n
#// - Returns C Code for this part. \n
#//
sub frontEndCode {
	my ($i) = @_;
	$ii = 0;
	$jj = 0;
	$::dkTimesCalled ++;
	#//	- Figure out which DKI part this is
	for($ii=0;$ii<$::dacKillCnt;$ii++) {
		if($::dacKillPartNum[$ii] == $i) {
			$jj = $ii;
			print "FOUND DACKKILL number $jj \n";
		}
	 }
	 #// _dacState maintains DAC kill value.
	 #// This value is sent back to controller.c to shutdown DAC modules.
	 #//	0 = OK 
	 #//	1 = Set DAC module outputs to zero. 
	 my $DACSTAT = "\L$::xpartName[$i]_dacState";
	 for ($ii=0;$ii < $::dacKillModCnt[$jj];$ii++) {
		$modNum = $::dacKillMod[$jj][$ii];
		if($::dacKillDko[$modNum] eq "x") {
	 		$::dacKillDko[$modNum] = "$DACSTAT";
		} else {
	 		$::dacKillDko[$modNum] .= " | $DACSTAT";
		}
	 }
	 my $DKERR = '';
	 #// SIGNAL = DKI Sig Input
	 my $SIGNAL = $::fromExp[0];
	 #// BPTIME = Bypass time input, in seconds.
	 my $BPTIME = $::fromExp[1];
	 #// WDTIME = Watchdog Timer input, in seconds.
	 my $WDTIME = $::fromExp[2];
	 #// DTTIME = DAC shutdown timer input, in seconds.
	 my $DTTIME = $::fromExp[3];
	 my $CYCLE = "cycle";
	 #// DKSTAT = DKI State 
	 #//	0 = WD output = 0 and DAC modules shutdown
	 #//	1 = OK - WD output = 1 and DAC modules outputs run normal.
	 #//	2 = BYPASS Mode 
	 #//		- WD output = 1 and DAC modules outputs run normal.
	 #//	3 = Fault at Sig and WD timer running
	 #//		- WD output = 1 and DAC modules outputs run normal.
	 #//	4 = Fault at Sig and DAC shutdown timer running
	 #//		- WD output = 0 and DAC modules outputs run normal.
	 my $DKSTAT = "\L$::xpartName[$i]_dkState";
	 #// DKI WD signal output (1 = OK, 0 = FAULT)
	 my $WDOUT = "\L$::xpartName[$i]\[0\]";
	 #// DKI RESET signal output (1 = RESET, 0 = NOOP)
	 my $RSETOUT = "\L$::xpartName[$i]\[1\]";
	 #// DKI STATE signal output (Same as EPICS channel)
         my $STATEOUT = "\L$::xpartName[$i]\[2\]";
	 #// BYPASS time remaining 
	 my $BPTIME_REMAINING = "\L$::xpartName[$i]_remainingTime";
	 #// Time remaining until WD output trips (goes to zero).
	 my $WDTIME_REMAINING = "\L$::xpartName[$i]_wdTime";
	 #// Time remaining until DAC modules are shutdown
	 my $DTTIME_REMAINING = "\L$::xpartName[$i]_dtTime";
	 #// Following are DKI variables sent to / received from EPICS
	 my $EPICS_STATE = "pLocalEpics->$::systemName\.$::xpartName[$i]\_STATE";
	 my $EPICS_BPTIME = "pLocalEpics->$::systemName\.$::xpartName[$i]\_BPTIME";
	 my $EPICS_WDTIME = "pLocalEpics->$::systemName\.$::xpartName[$i]\_WDTIME";
	 my $EPICS_DTTIME = "pLocalEpics->$::systemName\.$::xpartName[$i]\_DTTIME";
	 my $EPICS_BPSET = "pLocalEpics->$::systemName\.$::xpartName[$i]\_BPSET";
	 my $EPICS_RESET = "pLocalEpics->$::systemName\.$::xpartName[$i]\_RESET";
	 my $EPICS_PANIC = "pLocalEpics->$::systemName\.$::xpartName[$i]\_PANIC";

	 #// Set DAC Kill signals to be sent back to controller.c
	 #// Do not do this until code done for last DACKILL IOP part
	 #// has code written. This takes care of case where there are multiple
	 #// of these parts and some DAC modules are shared between the two.
	 if($::dkTimesCalled == $::dacKillCnt) {
		for ($ii=0;$ii<12;$ii++) {
			if($::dacKillDko[$ii] ne "x") {
				$DKERR .= "dacInfo.dacChanErr[$ii] = $::dacKillDko[$ii]; \n";
			}
		}
	 }

	# Write out the C Code
        return <<END;

// DACKILLER MODULE
// State 0 = killall
// State 1 = OK
// State 2 = Bypass
// State 3 = WD timer running
// State 4 = DAC timer running
/// Reset DAC Status word

/// Clear the Dackill reset output, if set
if ($RSETOUT) $RSETOUT = 0;

/// Check if reset sent from EPICS
if ($EPICS_RESET && !$EPICS_PANIC && ($SIGNAL || $BPTIME_REMAINING)) {
	/// Reset WD output line to OK
	$WDOUT = 1;
	/// Clear WD timer
	$WDTIME_REMAINING = $WDTIME;
	/// Clear DT timer
	$DTTIME_REMAINING = $DTTIME;
	/// Set DK status as OK
	$DKSTAT = 1;
	/// Set DAC enable mode as OK.
	$DACSTAT = 0;
	///  Clear Bypass timer
	$BPTIME_REMAINING = 0;
	/// Clear reset from EPICS as it is a momentary switch
	$EPICS_RESET = 0;
	/// Set the Dackill reset output line high (reset)
	$RSETOUT = 1;
	/// Clear Bypass mode.
	$EPICS_BPSET = 0;
} else if($EPICS_RESET) {
        /// Clear reset from EPICS as it is a momentary switch
        $EPICS_RESET = 0;
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
	/// Clear Bypass mode.
	$EPICS_BPSET = 0;
	/// Clear WD Timer
	$WDTIME_REMAINING = 0;
	/// Clear DT Timer
	$DTTIME_REMAINING = 0;
	/// Send dackill status to EPICS
	$DKSTAT = 0;
	/// Clear reset from EPICS as it is a momentary switch
	$EPICS_RESET = 0;
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
			default:
				$DACSTAT = 1;
				$WDOUT = 0;
				break;
		}
	/// If normal watchdog mode, OK at input
	} else if($SIGNAL)  {
		switch ($DKSTAT)
		{
			case 0:
				$DACSTAT = 1;
				$WDOUT = 0;
				break;
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
	if ($EPICS_BPSET && (0 == $BPTIME_REMAINING) && !$EPICS_PANIC) {
		/// Reset the bypass mode timer.
		$BPTIME_REMAINING = $BPTIME;
		/// Clear Bypass mode.
		$EPICS_BPSET = 0;
		/// Send dackill state with BYPASS cleared
		$DKSTAT = 2;
	} 
}

$STATEOUT = $DKSTAT;

/// On one second mark
if (!$CYCLE) {
	/// Decrement Bypass timer, if running.
	if($DKSTAT == 2) {
		$BPTIME_REMAINING -= 1;
		if($BPTIME_REMAINING == 0) {
			$DKSTAT = 1;
			$EPICS_BPSET = 0;
			/// Clear WD timer
			$WDTIME_REMAINING = $WDTIME;
			/// Clear DT timer
			$DTTIME_REMAINING = $DTTIME;
		}
	}
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

$DKERR
END
}

