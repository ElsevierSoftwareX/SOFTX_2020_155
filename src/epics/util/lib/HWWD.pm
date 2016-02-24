package CDS::HWWD;
use Exporter;
@ISA = ('Exporter');

#//     \page SusHWWD HWWD.pm
#// \b Description: \n
#//
#// This part is used to monitor and control a SUS Hardware WatchDog (HWWD) chassis. \n
#// \n
#// \b Operation: \n
#// \n
#// - Input = Single word, with the lower four bits received via a binary input channel \n
#// 	from the HWWD.\n
#// - Outputs:
#//	- 1) HWWD Status, a reflection of the input signal.
#//	- 2) RESET/COMMAND output: To be connected to binary output attached to HWWD RESET input.
#// - Monitoring Functions:
#// This part monitors the 4 status bits from the HWWD and reports them to an EPICS record (see \n
#// _STATE definition in printEpics). Additionally, this part provides two timers (1 sec downcounters)\n
#// to provide an estimate of time until the HWWD will trip the connected SUS and SEI systems.
#// - Control Functions:
#// This part provides the capability to send control data to the HWWD via its RESET input line. This \n
#// RESET line is connected via a single binary output channel of a BIO card. \n
#//	- 1) HWWD Reset 
#//	- 2) Set HWWD RMS Trip Point (60 to 360 mV RMS)
#//	- 3) Set HWWD Time to Trip (T2T) (2 to 30 minutes)
#//	- 4) Read RMS and T2T settings from HWWD
#//
#// \n
#// \b SUBROUTINES ************************* \n\n

#// \b sub \b partType \n 
#// Required subroutine for RCG \n
#// 
#// Returns HWWD \n\n
sub partType {
	my ($i,$j) = @_;
	return HWWD;
}

#// \b sub \b printHeaderStruct \n 
#// Required subroutine for RCG \n
#// Print Epics communication structure into a header file \n
#// Current part number is passed as first argument \n\n
#// This part passes 8 double type values to/from the EPICS sequencer. All of these have corresponding \n
#// EPICS variables.
#// - _STATE = HWWD Status, bits defined as. \n
#// 	- 0 = SEI Trip\n
#// 	- 1 = Photodiode RMS Fault \n
#// 	- 2 = SUS Trip \n
#// 	- 3 = LED Fault \n
#// - _CMD = control command to be sent to HWWD \n
#// 	- 1 = Reset\n
#// 	- 2 = Read Settings\n
#// 	- 3 = Write RMS trip point setting\n
#// 	- 4 = Write T2T setting\n
#// - _RMS_REQ = Requested RMS trip point setting \n
#// - _TIME_REQ = Requested T2T setting \n
#// - _RMS_RD = RMS trip point setting read back from HWWD \n
#// - _TIME_RD = T2T setting read back from HWWD. \n
#// - _TTF_MIN = Estimated time to SEI trip in minutes.\n
#// - _TTF_SEC = Estimated time to SEI trip in seconds.\n
#// - _MODE = Present software state \n
#//

sub printHeaderStruct {
        my ($i) = @_;
	my $MYNAME = $::xpartName[$i];
	print ::OUTH <<END;
double $MYNAME\_STATE;
double $MYNAME\_CMD;
double $MYNAME\_RMS_REQ;
double $MYNAME\_TIME_REQ;
double $MYNAME\_RMS_RD;
double $MYNAME\_TIME_RD;
double $MYNAME\_TTF_MIN;
double $MYNAME\_TTF_SEC;
double $MYNAME\_MODE;
END

$here = <<END;
\tchar  $::xpartName[$i]\_RMS_REQ_mask;\n
\tchar  $::xpartName[$i]\_TIME_REQ_mask;\n
END
        return $here;
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
OUTVARIABLE $::xpartName[$i]\_STATE $::systemName\.$::xpartName[$i]\_STATE double ai 0 
MOMENTARY $::xpartName[$i]\_CMD $::systemName\.$::xpartName[$i]\_CMD double ai 0
INVARIABLE $::xpartName[$i]\_RMS_REQ $::systemName\.$::xpartName[$i]\_RMS_REQ double ai 110 field(PREC,\"1\") field(HOPR,\"3.0\") field(LOPR,\"0.5\")
INVARIABLE $::xpartName[$i]\_TIME_REQ $::systemName\.$::xpartName[$i]\_TIME_REQ double ai 20 field(PREC,\"1\") field(HOPR,\"30.0\") field(LOPR,\"10.0\")
OUTVARIABLE $::xpartName[$i]\_RMS_RD $::systemName\.$::xpartName[$i]\_RMS_RD double ao 110 field(PREC,\"1\")
OUTVARIABLE $::xpartName[$i]\_TIME_RD $::systemName\.$::xpartName[$i]\_TIME_RD double ao 20 field(PREC,\"1\") 
OUTVARIABLE $::xpartName[$i]\_TTF_MIN $::systemName\.$::xpartName[$i]\_TTF_MIN double ao 0 
OUTVARIABLE $::xpartName[$i]\_TTF_SEC $::systemName\.$::xpartName[$i]\_TTF_SEC double ao 0 
OUTVARIABLE $::xpartName[$i]\_MODE $::systemName\.$::xpartName[$i]\_MODE double ao 0 
END
}


#// \b sub \b printFrontEndVars \n 
#// Required subroutine for RCG \n
#// Current part number is passed as first argument \n
#// -  Defines local variables for user C code.
#//
sub printFrontEndVars  {
        my ($i) = @_;
        print ::OUT "static int \L$::xpartName[$i]\[2\];\n";
        print ::OUT "static int \L$::xpartName[$i]_remainingTime;\n";
        print ::OUT "static int \L$::xpartName[$i]_onesecpulse;\n";
        print ::OUT "static double \L$::xpartName[$i]_rmsTime;\n";
        print ::OUT "static double \L$::xpartName[$i]_wdTime;\n";
        print ::OUT "static int \L$::xpartName[$i]_mode;\n";
        print ::OUT "static int \L$::xpartName[$i]_modestep;\n";
        print ::OUT "static int \L$::xpartName[$i]_cmdack;\n";
        print ::OUT "static int \L$::xpartName[$i]_ackcnt;\n";
        print ::OUT "static int \L$::xpartName[$i]_nxtreq;\n";
        print ::OUT "static int \L$::xpartName[$i]_time2trip;\n";
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
#//	- Variable initialization of note: \n
#//		- _onesecpulse = FE_RATE: Number of code cycles per second for use by timers. \n.
#//
sub frontEndInitCode {
	my ($i) = @_;
$here = <<END;
\L$::xpartName[$i]_mode = 0;
\L$::xpartName[$i]_remainingTime = 0;
\L$::xpartName[$i]_onesecpulse = \UFE_RATE;
\L$::xpartName[$i]_modestep = 0;
\L$::xpartName[$i]_wdTime = 0;
\L$::xpartName[$i]_rmsTime = 0;
\L$::xpartName[$i]_cmdack = 0;
\L$::xpartName[$i]_ackcnt = 47;
\L$::xpartName[$i]_nxtreq = 0;
\L$::xpartName[$i]_time2trip = 0;
\L$::xpartName[$i]\[0\] = 0;
\L$::xpartName[$i]\[1\] = 0;
END
        return $here;
}


# Return front end code
# Argument 1 is the part number
# Returns calculated code string
#// \b sub \b frontEndCode \n 
#// Required subroutine for RCG \n
#// - Argument 1 is the part number \n
#// - Returns C Code for this part. \n
#//
#// The 'return << END' section writes out the C coded needed to support this part. It consists \n
#//	of three basic sections:\n
#//	- Monitoring: Reads in the HWWD status and updates EPICS information.
#//	- If not presently in a HHWD command sequence ($HWWD_MODE == 0), check for new commands.
#//		- Initiate the command sequence requested.
#//	- If in a command sequence ($HWWD_MODE != 0), continue the command sequence until complete.
#//
#// HHWD Commands. \n
#// Overview: \n
#// The HWWD chassis has a single connection, RESET, from a single binary output channel ( single bit, 0 or 1). \n
#// While initially designed for the sole purpose of allowing remote RESET, this input was later adapted to allow \n
#// the setting of HWWD trip points and times. In order for the HWWD to recognize these commands (listed below), this \n
#// software must send a precisely timed series of pulses to this HWWD input. This software must also be capable of \n
#// reading precisely timed pulse trains back from the HWWD status monitor to read and verify the settings.\n
#//
#// General Notes: \n
#//	- 1) RESET command is only recognized by the HWWD when in a tripped condition (SEI and/or SUS fault). Unless \n
#//	the status readback indicates that the HWWD is in that state, this command should NOT be sent.
#//	- 2) The READ, SET RMS, SET TIME commands are not recognized by the HWWD unless the HWWD status is \n
#//	NOT in a tripped state. Therefore, do NOT attempt to send these commands unless HWWD is not in tripped state. \n
#//	- 3) Upon receipt, and completion, of a 'SET RMS Trip Point' command, this code will continue a sequence to also \n
#//	set the T2T and readback the new settings from the HWWD. \n
#//	
#//
#// Supported Commands:
#//	- RESET: \n
#//		- Clears HWWD SUS and SEI trip if, and only if, PD and LED indicators are OK.
#//		- Command Sequence:
#//			- Set RESET output high (1) for 2 seconds, then return to zero (0).
#//	- READ: \n
#//		- Reads back the RMS trip point and T2T values set in the HHWD.
#//		- Command Sequence:
#//			- Set RESET output high (1) for 1 second \n
#//			- Set RESET output low (0) for 1 second \n
#//			- Set RESET output high (1) for 5 seconds \n
#//			- Set RESET output low (0) \n
#//			- Wait for STATUS = 0 \n
#//			- Wait for STATUS > 0 \n
#//			- Measure time duration of PD and LED status = 1 \n
#//				- Time duration of PD bit set high indicates RMS trip point setting.
#//				- Time duration of LED bit set high indicates T2T setting.
#//			- Convert signal durations to proper engineering units and write to EPICS channels.
#//			- Verify receipt of command acknowledgment signal from HHWD.
#//				- Series of five on/off (1/0), one second pulses on LED and PD status bits.
#//	- SET RMS TRIP POINT
#//		- Writes the RMS trip point setting to the HWWD in range of 60 to 360 mV RMS.
#//		- Command Sequence:
#//			- Set RESET output high (1) for 10 seconds \n
#//			- Set RESET output low (0) for 1 second \n
#//			- Set RESET output high (1) for 2 seconds * RMS request/110 \n
#//			- Set RESET output low (0) \n
#//			- Verify receipt of command acknowledgment signal from HHWD.
#//				- Series of ten on/off (1/0), one second pulses on LED and PD status bits.
#//	- SET T2T 
#//		- Writes the T2T setting to the HWWD in range of 2 to 30 minutes.
#//		- Command Sequence:
#//			- Set RESET output high (1) for 15 seconds \n
#//			- Set RESET output low (0) for 1 second \n
#//			- Set RESET output high (1) for 1 second * T2T request/5 \n
#//			- Set RESET output low (0) \n
#//			- Verify receipt of command acknowledgment signal from HHWD.
#//				- Series of fiftenn on/off (1/0), one second pulses on LED and PD status bits.
sub frontEndCode {
	my ($i) = @_;
	 #// SIGNAL = HWWD Sig Input
	 my $SIGNAL = $::fromExp[0];
	 my $CYCLE = "cycle";
	 #// HWWD signal output (1 = OK, 0 = FAULT)
	 my $WDOUT = "\L$::xpartName[$i]\[0\]";
	 #// HWWD reset signal output 
	 my $RSETOUT = "\L$::xpartName[$i]\[1\]";
	 #// HWWD Mode 
	 my $HWWD_MODE = "\L$::xpartName[$i]_mode";
	 #// HWWD Mode Step 
	 my $HWWD_MODE_STEP = "\L$::xpartName[$i]_modestep";
	 #// FAULT time remaining 
	 my $TIME_REMAINING = "\L$::xpartName[$i]_remainingTime";
	 my $ONE_SEC_PULSE = "\L$::xpartName[$i]_onesecpulse";
	 my $FIVE_SEC_PULSE = "\L$::xpartName[$i]_onesecpulse * 5";
	 my $CMD_ACK = "\L$::xpartName[$i]_cmdack";
	 my $ACK_CNT = "\L$::xpartName[$i]_ackcnt";
	 my $NXT_REQ = "\L$::xpartName[$i]_nxtreq";
	 #// Time remaining until WD output trips (goes to zero).
	 my $WDTIME = "\L$::xpartName[$i]_wdTime";
	 #// Time remaining until DAC modules are shutdown
	 my $RMSTIME = "\L$::xpartName[$i]_rmsTime";
	 my $TTF = "\L$::xpartName[$i]_time2trip";
	 #// Following are HWWD variables sent to / received from EPICS
	 my $EPICS_STATE = "pLocalEpics->$::systemName\.$::xpartName[$i]\_STATE";
	 my $EPICS_CMD_INT = "(int)pLocalEpics->$::systemName\.$::xpartName[$i]\_CMD";
	 my $EPICS_CMD = "pLocalEpics->$::systemName\.$::xpartName[$i]\_CMD";
	 my $EPICS_RMSREQ = "pLocalEpics->$::systemName\.$::xpartName[$i]\_RMS_REQ";
	 my $EPICS_TIMEREQ = "pLocalEpics->$::systemName\.$::xpartName[$i]\_TIME_REQ";
	 my $EPICS_RMSRD = "pLocalEpics->$::systemName\.$::xpartName[$i]\_RMS_RD";
	 my $EPICS_TIMERD = "pLocalEpics->$::systemName\.$::xpartName[$i]\_TIME_RD";
	 my $EPICS_TTF_MIN = "pLocalEpics->$::systemName\.$::xpartName[$i]\_TTF_MIN";
	 my $EPICS_TTF_SEC = "pLocalEpics->$::systemName\.$::xpartName[$i]\_TTF_SEC";
	 my $EPICS_MODE = "pLocalEpics->$::systemName\.$::xpartName[$i]\_MODE";

	# Write out the C Code
        return <<END;

// HWWD MODULE
$SIGNAL = (int)$SIGNAL ^ 15;
$EPICS_STATE = $SIGNAL;
$WDOUT = $SIGNAL;
if(!$CYCLE && !$SIGNAL) $TTF = $EPICS_TIMERD * 60;
if(!$CYCLE && $SIGNAL && ($TTF > 0)) $TTF --;
if(!$CYCLE && ((int)$SIGNAL & 1)) $TTF = 0;
$EPICS_TTF_MIN = $TTF / 60;
$EPICS_TTF_SEC = $TTF % 60;
if($NXT_REQ  && ($HWWD_MODE == 0))
{
	$EPICS_CMD = $NXT_REQ;
	$NXT_REQ = 0;
}
if($HWWD_MODE == 0)
{
	switch ($EPICS_CMD_INT)
	{
		case 1:	// RESET COMMAND
			/// Only reset if SEI or SEI&SUS tripped and LED and PD are OK
			if ($SIGNAL == 1 || $SIGNAL == 5)
			{
				/// Set output to HWWD reset high
				$RSETOUT = 1;
				/// Set time to hold reset in cycles
				$TIME_REMAINING = $ONE_SEC_PULSE * 2;
				$HWWD_MODE = 1;
			}
			break;
		case 2:	// READ COMMAND
			if (!((int)$SIGNAL & 5))
			{
				/// Set output to HWWD reset high
				$RSETOUT = 1;
				$TIME_REMAINING = $ONE_SEC_PULSE;
				$HWWD_MODE = 2;
				$HWWD_MODE_STEP = 1;
			}
			break;
		case 3:	// WRITE RMS COMMAND
			if(($EPICS_RMSREQ > 360.0) || ($EPICS_RMSREQ < 60))
			{
				$TIME_REMAINING = $ONE_SEC_PULSE * 4;
				$NXT_REQ = 4;
				$HWWD_MODE = 6;
				break;
		
			} else if (!((int)$SIGNAL & 5))
			{
				/// Set output to HWWD reset high
				$RSETOUT = 1;
				$TIME_REMAINING = $ONE_SEC_PULSE * 10;
				$HWWD_MODE = 3;
				$HWWD_MODE_STEP = 1;
			}
			break;
		case 4:	// WRITE TIME COMMAND
			if(($EPICS_TIMEREQ > 30.0) || ($EPICS_TIMEREQ < 2.0))
			{
				$TIME_REMAINING = $ONE_SEC_PULSE * 4;
				$HWWD_MODE = 6;
				$NXT_REQ = 2;
				break;
			} else if (!((int)$SIGNAL & 5))
			{
				/// Set output to HWWD reset high
				$RSETOUT = 1;
				$TIME_REMAINING = $ONE_SEC_PULSE * 15;
				$HWWD_MODE = 4;
				$HWWD_MODE_STEP = 1;
			}
			break;
		default:
			break;
	}
} else {
	switch ($HWWD_MODE)
	{
		case 1:	// RESET COMMAND
			$TIME_REMAINING --;
			if($TIME_REMAINING <= 0)
			{
				$RSETOUT = 0;
				$TIME_REMAINING = 0;
				$HWWD_MODE = 0;
			}
			break;
		case 2:	// READ COMMAND
			$TIME_REMAINING --;
			if(($TIME_REMAINING <= 0) && ($HWWD_MODE_STEP < 4))
			{
				$TIME_REMAINING = 0;
				if($HWWD_MODE_STEP == 1)
				{
					$RSETOUT = 0;
					$TIME_REMAINING = $ONE_SEC_PULSE;
					$HWWD_MODE_STEP = 2;
				} else if ($HWWD_MODE_STEP == 2) {
					$RSETOUT = 1;
					$TIME_REMAINING = $FIVE_SEC_PULSE;
					$HWWD_MODE_STEP = 3;
				} else if ($HWWD_MODE_STEP == 3) {
					$RSETOUT = 0;
					$HWWD_MODE_STEP = 4;
					$TIME_REMAINING = $FIVE_SEC_PULSE * 2;
				}
			}
			if(($TIME_REMAINING > 0) && ($HWWD_MODE_STEP == 4) && ($SIGNAL == 0))
				$HWWD_MODE_STEP = 5;
			if(($TIME_REMAINING > 0) && ($HWWD_MODE_STEP == 5) && ($SIGNAL != 0))
				$HWWD_MODE_STEP = 6;
			if(($TIME_REMAINING > 0) && ($HWWD_MODE_STEP == 6) && ($SIGNAL))
			{
				if((int)$SIGNAL & 2) $RMSTIME ++;
				if((int)$SIGNAL & 8) $WDTIME ++;
			}
			if((($TIME_REMAINING <= 0) || (!$SIGNAL)) && ($HWWD_MODE_STEP == 6))
			{
				$EPICS_RMSRD = $RMSTIME / FE_RATE / 2 * 110.0;
				$EPICS_TIMERD = $WDTIME / FE_RATE * 5;
				$RMSTIME = 0;
				$WDTIME = 0;
				$TIME_REMAINING = $ONE_SEC_PULSE * 8;
				$HWWD_MODE_STEP = 0;
				$HWWD_MODE = 5;
				$CMD_ACK = 0;
				$ACK_CNT = 5;
			}
			break;
		case 3:	// RMS SET COMMAND
			$TIME_REMAINING --;
			if(($TIME_REMAINING <= 0) && ($HWWD_MODE_STEP == 1))
			{
				$RSETOUT = 0;
				$TIME_REMAINING = $ONE_SEC_PULSE;
				$HWWD_MODE_STEP = 2;
			}
			if(($TIME_REMAINING <= 0) && ($HWWD_MODE_STEP == 2))
			{
				$RSETOUT = 1;
				$TIME_REMAINING = $ONE_SEC_PULSE * 2 * $EPICS_RMSREQ / 110;
				$HWWD_MODE_STEP = 3;
			}
			if(($TIME_REMAINING <= 0) && ($HWWD_MODE_STEP == 3))
			{
				$RSETOUT = 0;
				$TIME_REMAINING = $ONE_SEC_PULSE * 12;
				$HWWD_MODE_STEP = 0;
				$HWWD_MODE = 5;
				$CMD_ACK = 0;
				$ACK_CNT = 10;
				$NXT_REQ = 4;
			}
			break;
		case 4:	// TRIP TIME SET COMMAND
			$TIME_REMAINING --;
			if(($TIME_REMAINING <= 0) && ($HWWD_MODE_STEP == 1))
			{
				$RSETOUT = 0;
				$TIME_REMAINING = $ONE_SEC_PULSE;
				$HWWD_MODE_STEP = 2;
			}
			if(($TIME_REMAINING <= 0) && ($HWWD_MODE_STEP == 2))
			{
				$RSETOUT = 1;
				$TIME_REMAINING = $EPICS_TIMEREQ / 5 * $ONE_SEC_PULSE;
				$HWWD_MODE_STEP = 3;
			}
			if(($TIME_REMAINING <= 0) && ($HWWD_MODE_STEP == 3))
			{
				$RSETOUT = 0;
				$TIME_REMAINING = $ONE_SEC_PULSE * 20;
				$HWWD_MODE_STEP = 0;
				$HWWD_MODE = 5;
				$CMD_ACK = 0;
				$ACK_CNT = 15;
				$NXT_REQ = 2;
			}
			break;
		case 5:	// COMMAND ACK
			$TIME_REMAINING --;
			if(($CMD_ACK == 0) && ($SIGNAL))
			{
				$ACK_CNT --;
				$CMD_ACK = 1;
			}
			if(($CMD_ACK == 1) && (!$SIGNAL))
			{
				$CMD_ACK = 0;
			}
			if(($TIME_REMAINING <= 0) || ($ACK_CNT <= 0))
			{
				$TIME_REMAINING = 0;
				$HWWD_MODE = 0;
			}
			break;
		case 6:	// SETTING REQ NOT IN RANGE 
			$TIME_REMAINING --;
			if(($TIME_REMAINING <= 0))
			{
				$TIME_REMAINING = 0;
				$HWWD_MODE = 0;
			}
		default:
			break;
	}
}
/// Clear command
$EPICS_CMD = 0;
$EPICS_MODE = $HWWD_MODE;



END
}

