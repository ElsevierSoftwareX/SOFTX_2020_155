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

sub partType {
	return RfmIoSender;
}

# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub printHeaderStruct {
    # This stuff appears in the middle of a struct definition
    my ($i) = @_;
    my $MYNAME = $::xpartName[$i];
    $MYNAME = 'RFMCOMM'; # HACK
    print ::OUTH <<END;
int ${MYNAME}\_WAIT;
int ${MYNAME}\_RESET;
int ${MYNAME}\_ERRORCOUNT;
END
}

# Print Epics variable definitions
# Current part number is passed as first argument
sub printEpics {
    my ($i) = @_;
    my $MYNAME = $::xpartName[$i];
    $MYNAME = 'RFMCOMM'; # HACK
    my $SYSNAME = $::systemName;   
    print ::EPICS <<END;
MOMENTARY   ${MYNAME}\_WAIT       ${SYSNAME}\.${MYNAME}\_WAIT           int ai 0
MOMENTARY   ${MYNAME}\_RESET      ${SYSNAME}\.${MYNAME}\_RESET          int ai 0
OUTVARIABLE ${MYNAME}\_ERRORCOUNT ${SYSNAME}\.${MYNAME}\_ERRORCOUNT     int ao 0
END
}

# Print variable declarations int front-end file
# Current part number is passed as first argument
sub printFrontEndVars  {
    my ($i) = @_;
    my $MYNAME = $::xpartName[$i];
    $MYNAME = 'RFMCOMM'; # HACK
    my $LOCALSTATE = "\L${MYNAME}";
    print ::OUT<<END;
#define COMMDATA_INLINE
#include "commData.h"
static struct CommState ${LOCALSTATE};
END
}

# Convert the part name to the RFM address
sub nameToAddress {
    my ($partName) = @_;
    die "RfmIO Part $partName invalid: its name must be the hex address\n" unless
	$partName =~ /^.*0x(\d|[abcdefABCDEF])+$/;
    my $rfmAddress =  hex $partName;
    my $card_num = ($rfmAddress - $rfmAddress % 0x4000000) / 0x4000000;
    $rfmAddress = $rfmAddress % 0x4000000;
    return $rfmAddress;
}


# Return front end initialization code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndInitCode {
    my ($i) = @_;
    my $partName = $::xpartName[$i];
    print "Reflective Memory Address is $partName\n";

    my $rfmBaseAddress = nameToAddress($partName);
    my $rfmSemaphoreAddress = $rfmBaseAddress;
    my $rfmDataAddress = $rfmBaseAddress + 4;

    if ($rfmDataAddress % 4 != 0) {
	die "RfmIoSynchronizedSender: data address must be 4-byte aligned\n";
    }
    if ($rfmDataAddress % 4 != 0) {
	die "RfmIoSynchronizedSender: semaphore address must be 4-byte aligned\n";
    }

    my $rfmSemaphoreAddressString = sprintf("0x%x", $rfmSemaphoreAddress);
    my $rfmDataAddressString = sprintf("0x%x", $rfmDataAddress);

    my $MYNAME = 'RFMCOMM'; # HACK
    my $LOCALSTATE = "\L${MYNAME}";

    my $card_num = 0; # HACK
    # Many of these values should really not be hard-coded, but
    # I don't know how to properly parametrize the block.
    return <<END;
commDataInit(&(${LOCALSTATE}),   
             COMM_SEND,  /* commType */
	     2,          /* Nsamp */
	     (unsigned int *)(((char *)cdsPciModules.pci_rfm[$card_num]) + $rfmSemaphoreAddressString), /* rfmCounter */
	     (double *)(((char *)cdsPciModules.pci_rfm[$card_num]) + $rfmDataAddressString), /* rfmData */
	     1);         /* dataLength */
END
}

# Figure out part input code
# Argument 1 is the part number
# Argument 2 is the input number
# Returns calculated input code
sub fromExp {
    my ($i, $j) = @_;
    my $from = $::partInNum[$i][$j];
    my $fromPort = $::partInputPort[$i][$j];
    # return "\L$::xpartName[$from]" . "\[" . $fromPort . "\]";
    my $MYNAME = 'RFMCOMM'; # HACK
    my $LOCALSTATE = "\L${MYNAME}";

    if ($fromPort == 0) {
	return "${LOCALSTATE}.errCounter";
    } elsif ($fromPort == 1) {
	return "${LOCALSTATE}.cycleCounter";
    } else {
	die "RfmIoSynchronizedSender has too many outputs";
    }
}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string

sub frontEndCode {
    my ($i) = @_;
    print "frontEndCode($i)\n";
    my $partName = $::xpartName[$i];
    my $fromType = $::partInputType[$i][$_];

    my $MYNAME = $::xpartName[$i];
    $MYNAME = 'RFMCOMM'; # HACK
    my $SYSNAME = $::systemName;   
    my $LOCALSTATE = "\L${MYNAME}"; # name of CommStruct

    my $INPUT =  $::fromExp[0];
    my $RESET = "pLocalEpics->${SYSNAME}\.${MYNAME}_RESET";
    my $WAIT  = "pLocalEpics->${SYSNAME}\.${MYNAME}_WAIT";
    my $ERRCOUNT = "pLocalEpics->${SYSNAME}\.${MYNAME}_ERRORCOUNT";
   
    return <<END;
if ($RESET) {
  ${LOCALSTATE}.errCounter = 0;
  $RESET = 0;
}
if ($WAIT) {
  ${LOCALSTATE}.waitCounter ++;
  $WAIT = 0;
}
commDataRecv(&${LOCALSTATE}, &(${INPUT})); /* FIXME: assuming that "${INPUT}" is something I can take address-of! */
${ERRCOUNT} = ${LOCALSTATE}.errCounter;
END

}
