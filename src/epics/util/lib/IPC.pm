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
        ;
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
	if ($address % 4 != 0) {
		die "IPC Part $::xpartName[$i] invalid: address must be 4-byte aligned\n";
	}
        return "_ipc_shm? *((float *)(((void *)_ipc_shm) + $addressString)) : 0.0";
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
	if ($address % 4 != 0) {
		die "RfmIO Part $::xpartName[$i] invalid: address must be 4-byte aligned\n";
	}
        my $fromType = $::partInputType[$i][$_];
        if (($fromType ne "GROUND") && ($::partInput[$i][0] ne "NC") && ($fromType ne "")) {
		return "if (_ipc_shm != 0) {\n"
        		. "  // IPC output\n"
                	. "  *((float *)(((char *)_ipc_shm) + $addressString)) = $::fromExp[0];\n"
			. "}\n";
        }
        return " ";
}
