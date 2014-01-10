package CDS::HWWD;
use Exporter;
@ISA = ('Exporter');

#//     \page SusHWWD HWWD.pm
#// \b Description: \n
#//
#// \n
#// \b Operation: \n
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
#// This part passes 8 double type values: \n
#// - _STATE = HWWD Status, bits defined as. \n
#// 	- 0 = SEI Trip\n
#// 	- 1 = Photodiode RMS  \n
#// 	- 2 = SUS Trip \n
#// 	- 3 = LED Fault \n
#// - _CMD =  \n
#// - _RMS_REQ =  \n
#// - _TIME_REQ =  \n
#// - _RMS_RD = . \n
#// - _TIME_RD = . \n
#// - _TTF = . \n
#// - _MODE = . \n
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
double $MYNAME\_TTF_SEI;
double $MYNAME\_TTF_SUS;
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
INVARIABLE $::xpartName[$i]\_RMS_REQ $::systemName\.$::xpartName[$i]\_RMS_REQ double ai 1 field(PREC,\"1\") field(HOPR,\"3.0\") field(LOPR,\"0.5\")
INVARIABLE $::xpartName[$i]\_TIME_REQ $::systemName\.$::xpartName[$i]\_TIME_REQ double ai 20 field(PREC,\"1\") field(HOPR,\"30.0\") field(LOPR,\"10.0\")
OUTVARIABLE $::xpartName[$i]\_RMS_RD $::systemName\.$::xpartName[$i]\_RMS_RD double ao 1 field(PREC,\"1\")
OUTVARIABLE $::xpartName[$i]\_TIME_RD $::systemName\.$::xpartName[$i]\_TIME_RD double ao 20 field(PREC,\"1\") 
OUTVARIABLE $::xpartName[$i]\_TTF_SEI $::systemName\.$::xpartName[$i]\_TTF_SEI double ao 0 
OUTVARIABLE $::xpartName[$i]\_TTF_SUS $::systemName\.$::xpartName[$i]\_TTF_SUS double ao 0 
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
$here = <<END;
\L$::xpartName[$i]_mode = 0;
\L$::xpartName[$i]_remainingTime = 0;
\L$::xpartName[$i]_onesecpulse = \UFE_RATE;
\L$::xpartName[$i]_modestep = 0;
\L$::xpartName[$i]_wdTime = 0;
\L$::xpartName[$i]_rmsTime = 0;
\L$::xpartName[$i]_cmdack = 0;
\L$::xpartName[$i]_ackcnt = 47;
\L$::xpartName[$i]_nxtreq = 3;
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
	 #// Following are DKI variables sent to / received from EPICS
	 my $EPICS_STATE = "pLocalEpics->$::systemName\.$::xpartName[$i]\_STATE";
	 my $EPICS_CMD_INT = "(int)pLocalEpics->$::systemName\.$::xpartName[$i]\_CMD";
	 my $EPICS_CMD = "pLocalEpics->$::systemName\.$::xpartName[$i]\_CMD";
	 my $EPICS_RMSREQ = "pLocalEpics->$::systemName\.$::xpartName[$i]\_RMS_REQ";
	 my $EPICS_TIMEREQ = "pLocalEpics->$::systemName\.$::xpartName[$i]\_TIME_REQ";
	 my $EPICS_RMSRD = "pLocalEpics->$::systemName\.$::xpartName[$i]\_RMS_RD";
	 my $EPICS_TIMERD = "pLocalEpics->$::systemName\.$::xpartName[$i]\_TIME_RD";
	 my $EPICS_TTF_SEI = "pLocalEpics->$::systemName\.$::xpartName[$i]\_TTF_SEI";
	 my $EPICS_TTF_SUS = "pLocalEpics->$::systemName\.$::xpartName[$i]\_TTF_SUS";
	 my $EPICS_MODE = "pLocalEpics->$::systemName\.$::xpartName[$i]\_MODE";

	# Write out the C Code
        return <<END;

// HWWD MODULE
$EPICS_STATE = $SIGNAL;
$WDOUT = $SIGNAL;
if(!$CYCLE && !$SIGNAL) $EPICS_TTF_SEI = $EPICS_TIMERD * 60;
if(!$CYCLE && $SIGNAL && ($EPICS_TTF_SEI > 0)) $EPICS_TTF_SEI --;
if(!$CYCLE && ((int)$SIGNAL & 1)) $EPICS_TTF_SEI = 0;
if(!$CYCLE && !((int)$SIGNAL & 10)) $EPICS_TTF_SUS = $EPICS_TIMERD * 60 + $EPICS_TTF_SEI;
if(!$CYCLE && ((int)$SIGNAL & 10) && ($EPICS_TTF_SUS > 0)) $EPICS_TTF_SUS --;
if(!$CYCLE && ((int)$SIGNAL & 4)) $EPICS_TTF_SUS = 0;
if(($EPICS_TIMERD > ($EPICS_TIMEREQ * 1.2)) || ($EPICS_TIMERD < ($EPICS_TIMEREQ * .8)))
	$EPICS_STATE += 16;
if(($EPICS_RMSRD > ($EPICS_RMSREQ * 1.1)) || ($EPICS_RMSRD < ($EPICS_RMSREQ * .9)))
	$EPICS_STATE += 32;
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
			if(($EPICS_RMSREQ > 600.0) || ($EPICS_RMSREQ < 100))
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
				$EPICS_RMSRD = $RMSTIME / FE_RATE / 2 * 200.0;
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
				$TIME_REMAINING = $ONE_SEC_PULSE * 2 * $EPICS_RMSREQ / 200;
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

