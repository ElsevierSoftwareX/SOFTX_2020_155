package CDS::RfmIoSender;
use Exporter;
@ISA = ('Exporter');

# This part implements an improved means of communication over RFM
#
# Based on RfmIO
#
# Tobin Fricke 2009-10-06 <tfricke@ligo.caltech.edu>
# Louisiana State University and Agricultural and Mechanical College

# Changelog:
# 
# 2009-10-09 Now uses Matt's 'commDataInit' routine
# 2009-10-15 Now doing CommDataRecv instead of the semaphore thing

# The following file handles are defined:
#
# ::OUT    --> advLigo/src/fe/om1/om1.c     - the front-end main program loop
# ::OUTH   --> advLigo/src/include/om1.h    - Defines a struct containing storage for all EPICS variables
# ::EPICS  --> advLigo/src/epics/fmseq/om1  - Some kind of list of EPICS channels?

####################### special helper for RfmIoSender
# Convert the part name to the RFM address
#   the name format is "<part name>:<data rate>:<hex address>"
sub parseName {
    my $partName = shift(@_);
    my ($MYNAME, $dataRate, $fullAddress) = split(/x/, $partName);

    # convert full address to card number and address
    ($fullAddress =~ /^(\d|[abcdefABCDEF])+$/) or
      die "RfmIoSender $partName invalid: name must be " .
          " \"part name:data rate:hex address\"\n";
    $fullAddress =  hex $fullAddress;

    my $cardNum = ($fullAddress - $fullAddress % 0x4000000) / 0x4000000;
    my $rfmAddress = $fullAddress % 0x4000000;

    ($rfmAddress % 8 == 0) or
	die "RfmIoSender: address must be 8-byte aligned\n";

    return ($MYNAME, $dataRate, $cardNum, $rfmAddress);
}

####################### special helper for RfmIoSender
# Return the names of local variables
#   MYNAME is first argument
sub getLocalVars {
    my $MYNAME = shift(@_);
    my $MYSTATE = "\L${MYNAME}";
    my $MYINPUT = "${MYSTATE}_inputValue";
    my $MYCYCLE = "${MYSTATE}_cycleCount";

    return ($MYSTATE, $MYINPUT, $MYCYCLE);
}

#####################################################

# return part type name
sub partType {
    return RfmIoSender;
}

# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub printHeaderStruct {
  # no Epics
}

# Print Epics variable definitions
# Current part number is passed as first argument
sub printEpics {
  # no Epics
}

# Print variable declarations int front-end file
# Current part number is passed as first argument
sub printFrontEndVars  {
    my $partNum = shift(@_);
    my $partName = $::xpartName[$partNum];
    my ($MYNAME, $dataRate, $cardNum, $rfmAddress) = parseName($partName);
    my ($MYSTATE, $MYINPUT, $MYCYCLE) = getLocalVars($MYNAME);

    my $rfmAddressString = sprintf("0x%x", $rfmAddress);

    print ::OUT<<END;
/* RfmIoSender $MYNAME: $dataRate Hz at $rfmAddressString ($cardNum) */
#define COMMDATA_INLINE
#include "commData.h"
static struct CommDataState ${MYSTATE};
static cycle_cd ${MYCYCLE}; /* local cycle counter */
double ${MYINPUT};   /* temporary space for the input value */

END
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
    my $partNum = shift(@_);
    my $partName = $::xpartName[$partNum];
    my ($MYNAME, $dataRate, $cardNum, $rfmAddress) = parseName($partName);
    my ($MYSTATE, $MYINPUT, $MYCYCLE) = getLocalVars($MYNAME);

    my $rfmAddressString = sprintf("0x%x", $rfmAddress);

    print "RfmIoSender $MYNAME $dataRate Hz at " .
          "$rfmAddressString ($cardNum)\n";

    # return init code
    #   this assumes one double as input
    return <<END;
/* RfmIoSender $MYNAME: $dataRate Hz at $rfmAddressString ($cardNum) */

// init state struct $MYSTATE
commDataInit(&${MYSTATE}, $dataRate,   // state struct, dataRate
  sizeof(double), (void*)&${MYINPUT},  // dataBytes, input address
  (void*)(((char*)cdsPciModules.pci_rfm[$cardNum]) + $rfmAddressString));

END
}

# Figure out part input code
#   Argument 1 is the part number
#   Argument 2 is the input number
# Returns calculated input code
#
# "input code" means the expressing that serves as the input
# to a part that takes output from this part.  Typically this
# is just the name of this part's output variable.
#
# For example, if the part has an array of output values
# this would just return "\L$MYNAME.out\[" . $fromPort . "\]";

sub fromExp {
    # destNum and destInput refer to the caller (data destination)
    my ($destNum, $destInput) = @_;

    # partNum and srcPort refer to this part (data source)
    my $partNum = $::partInNum[$destNum][$destInput];
    my $srcPort = $::partInputPort[$destNum][$destInput];

    my $partName = $::xpartName[$partNum];
    my ($MYNAME, $dataRate, $cardNum, $rfmAddress) = parseName($partName);
    my ($MYSTATE, $MYINPUT, $MYCYCLE) = getLocalVars($MYNAME);

    # RfmIoSender outputs are:
    #   cycle counter
    if ($srcPort == 0) {
	return "${MYCYCLE}";
    } else {
	die "RfmIoSender $MYNAME has too many outputs (4 max)";
    }
}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string

sub frontEndCode {
    my $partNum = shift(@_);
    print "frontEndCode($partNum)\n";

    my $partName = $::xpartName[$partNum];
    my ($MYNAME, $dataRate, $cardNum, $rfmAddress) = parseName($partName);
    my ($MYSTATE, $MYINPUT, $MYCYCLE) = getLocalVars($MYNAME);

    my $rfmAddressString = sprintf("0x%x", $rfmAddress);

    # input expressions
    my $INPUT =  $::fromExp[0];

    return <<END;
// ==============================================================
// RfmIoSender $MYNAME: $dataRate Hz at $rfmAddressString ($cardNum)

// HACK: write only on even cycles (should be odd)
if( !(cycle & 1) )
{
  // set cycle counter
  //  int cycle is an argument to feCode, counting at FE_RATE
  $MYCYCLE = cycle * (COMMDATA_MAX_RATE / FE_RATE);

  // copy data to shared memory
  $MYINPUT = $INPUT;
  commDataSend(&${MYSTATE}, $MYCYCLE);
}

END

}
