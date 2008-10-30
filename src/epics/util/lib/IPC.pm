package CDS::IPC;
use Exporter;
@ISA = ('Exporter');

sub partType {
	return IPC;
}

# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub printHeaderStruct {
        my ($i) = @_;
        ;
}

# Print Epics variable definitions
# Current part number is passed as first argument
sub printEpics {
        my ($i) = @_;
        ;
}

# Print variable declarations int front-end file
# Current part number is passed as first argument
sub printFrontEndVars  {
        my ($i) = @_;
	my $partName = $::xpartName[$i];
	die "IPC Part $partName invalid: its name must be the hex address\n" unless
		$partName =~ /^.*0x(\d|[abcdefABCDEF])+$/;
	my $addressString = $partName;
	$addressString =~ s/^.*(0x(\d|[abcdefABCDEF])+)$/\1/g;
        my $address =  hex $partName;
	if ($address % 8 != 0) {
		die "IPC Part $::xpartName[$i] invalid: address must be 8-byte aligned\n";
	}
	if ($::partOutCnt[$i] > 0) {
          print ::OUT "double ipc_at_$addressString = *((double *)(((void *)_ipc_shm) + $addressString));\n";
	} else {
          print ::OUT "double ipc_at_$addressString;\n";
	}
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
	my $partName = $::partInput[$i][$j];
	#print "IPC $partName\n";
	die "IPC Part $partName invalid: its name must be the hex address\n" unless
		$partName =~ /^.*0x(\d|[abcdefABCDEF])+$/;
	my $addressString = $partName;
	$addressString =~ s/^.*(0x(\d|[abcdefABCDEF])+)$/\1/g;
        my $address =  hex $partName;
	if ($address % 8 != 0) {
		die "IPC Part $::xpartName[$i] invalid: address must be 8-byte aligned\n";
	}
#        return "*((double *)(((void *)_ipc_shm) + $addressString))";
	return "ipc_at_$addressString";
}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string

sub frontEndCode {
	my ($i) = @_;
	my $partName = $::xpartName[$i];
	# print "IPC Address is $partName\n";
	die "RfmIO Part $partName invalid: its name must be the hex address\n" unless
		$partName =~ /^.*0x(\d|[abcdefABCDEF])+$/;
	my $addressString = $partName;
	$addressString =~ s/^.*(0x(\d|[abcdefABCDEF])+)$/\1/g;
	my $address = hex $partName;
	if ($address % 8 != 0) {
		die "RfmIO Part $::xpartName[$i] invalid: address must be 8-byte aligned\n";
	}
        my $fromType = $::partInputType[$i][$_];
        if (($fromType ne "GROUND") && ($::partInput[$i][0] ne "NC") && ($fromType ne "")) {
#		return "if (_ipc_shm != 0) {\n"
#        		. "  // IPC output\n"
#                	. "  *((double *)(((char *)_ipc_shm) + $addressString)) = $::fromExp[0];\n"
#			. "}\n";
		$::ipcOutputCode .= "      *((double *)(((char *)_ipc_shm) + $addressString)) = ipc_at_$addressString;\n";
		return "ipc_at_$addressString = $::fromExp[0];\n";
        }
        return " ";
}
