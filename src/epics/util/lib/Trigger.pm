package CDS::Trigger;
use Exporter;
@ISA = ('Exporter');

# This module implements a "Trigger" component, which is a state machine with two
# states: ARMED (0), and TRIGGERED (1).   The trigger begins in the ARMED state
# and transitions to the TRIGGERED state when its input exceeds THRESHOLD.  The
# trigger returns to the ARMED state when the input subsides and the RESET epics
# variable is asserted (set to a nonzero value).
#
# Tobin Fricke <tfricke@ligo.caltech.edu>
# California Institute of Technology
#
# Described in the Livingston ilog:
# http://ilog.ligo-la.caltech.edu/ilog/pub/ilog.cgi?group=detector&date_to_view=05/04/2008&anchor_to_scroll_to=2008:05:04:16:38:04-fricke

sub partType {
	return Trigger;
}

# Print Epics communication structure into a header file
sub printHeaderStruct {
        my ($i) = @_; # current part number
	my $MYNAME = $::xpartName[$i];
	print ::OUTH <<END;
int $MYNAME\_STATE;
int $MYNAME\_RESET;
int $MYNAME\_THRESHOLD;
END
	return "\tchar $::xpartName[$i]_THRESHOLD_mask\n";
}

# Print Epics variable definitions
sub printEpics {
        my ($i) = @_;  # current part number
	print ::EPICS <<END;
MOMENTARY $::xpartName[$i]\_RESET $::systemName\.$::xpartName[$i]\_RESET int ai 0
OUTVARIABLE $::xpartName[$i]\_STATE $::systemName\.$::xpartName[$i]\_STATE int ai 0 \n
INVARIABLE $::xpartName[$i]\_THRESHOLD $::systemName\.$::xpartName[$i]\_THRESHOLD int ai 0 field(PREC,\"0\")
END
}

# LOCAL VARIABLES
# Print variable declarations int front-end file
# Current part number is passed as first argument
sub printFrontEndVars  {
        my ($i) = @_;
    	print ::OUT "static int \L$::xpartName[$i];\n";	
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
        return "\L$::xpartName[$from]";
}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string

sub frontEndCode {
    my ($i) = @_;
    
    my $INPUT = $::fromExp[$_];
    my $STATE = "\L$::xpartName[$i]";
    my $THRESHOLD = "pLocalEpics->$::systemName\.$::xpartName[$i]\_THRESHOLD";
    my $EPICS_STATE = "pLocalEpics->$::systemName\.$::xpartName[$i]\_STATE";
    my $RESET = "pLocalEpics->$::systemName\.$::xpartName[$i]\_RESET";
    
    return <<END;

// Trigger MODULE
$STATE = $EPICS_STATE;
if ($INPUT > $THRESHOLD) {
 $STATE = 1;  // Tripped
 $RESET = 0;
} else if ($RESET > 0) {
  $STATE = 0; // Armed
  $RESET = 0;
}
$EPICS_STATE = $STATE;
END
}





