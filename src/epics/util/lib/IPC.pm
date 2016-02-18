package CDS::IPC;
use Exporter;
@ISA = ('Exporter');

# node pointer and part counter passed
sub partType {
        my ($node, $i) = @_;
        my $desc = ${$node->{FIELDS}}{"Description"};
	my $partName = $::xpartName[$i];
	#print $partName, "\n";
	if (@h = $partName =~ /^(\w+:\d+):(\d+)$/g) {
	  die "Maximum $::maxRemoteIPCVars remote IPC vars allowed; exceeded in $partName\n" unless $h[1] < $::maxRemoteIPCVars;
	  print "Host name ", $h[0], "; address ", $h[1], "\n";
	  if (@{$::remoteIPC{$h[0]}} == 0) {
		# A new host; save its index
		$::remoteIPChostIdx{$h[0]} = $::remoteIPChosts;
		push @::remoteIPCnodes, $h[0];
		$::remoteIPChosts++;
		die "Maximum $::maxRemoteIPCHosts remote IPC nodes allowed; exceeded in $partName\n" unless $::remoteIPChosts <= $::maxRemoteIPCHosts;
		print "New remote IPC node $h[0] @ idx $::remoteIPChostIdx{$h[0]}\n";
	  }
	  # Save host and address information
	  push @{$::remoteIPC{$h[0]}}, $h[1];
	}

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
	# See if this is a remote IPC part
	my $is_remote = 0;
	my @h;
	if (@h = $partName =~ /^(\w+:\d+):(\d+)$/g) {
		die "Maximum $::maxRemoteIPCVars remote IPC vars allowed; exceeded in $partName\n" unless $h[1] < $::maxRemoteIPCVars;
		#print "Host name ", $h[0], "; address ", $h[1], "\n";
		$is_remote = 1;
	} else {
		die "IPC Part $partName invalid: its name must be the hex address or host:port:addr for remote IPC\n" unless
			$partName =~ /^.*0x(\d|[abcdefABCDEF])+$/;
	}

	if ($is_remote) {
	  #if (!$::remote_ipc_printed) {
            #print ::OUT "extern double remote_ipc_send[$::maxRemoteIPCHosts][$::maxRemoteIPCVars];\n";
            #print ::OUT "extern double remote_ipc_rcv[$::maxRemoteIPCHosts][$::maxRemoteIPCVars];\n";
	    #$::remote_ipc_printed = 1;
	  #}
	} else {
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
	my ($i) = @_;
	if ($remoteIPChosts && !$::remote_ipc_init_printed) {
          $code  = "for (int i = 0; i < $::remoteIPChosts; i++)\n";
	  $code .= "  for (int j = 0; j < $::maxRemoteIPCVars; j++)\n";
	  $code .= "    remote_ipc_send[i][j] = remote_ipc_rcv[i][j] = 0.0;\n";
	  $::remote_ipc_init_printed = 1;
          return $code;
	} else {
	  return "";
	}
}

# Figure out part input code
# Argument 1 is the part number
# Argument 2 is the input number
# Returns calculated input code
sub fromExp {
        my ($i, $j) = @_;
	my $partName = $::partInput[$i][$j];
	#print "IPC $partName\n";
	# Check remote host IPC spec
	if (@h = $partName =~ /^(\w+:\d+):(\d+)$/g) {
	  $n_host = $::remoteIPChostIdx{$h[0]};
	  return "remote_ipc_rcv[$n_host][$h[1]]";
	} else {
	  die "IPC Part $partName invalid: its name must be the hex address or host:port:addr for remote IPC\n"
		unless $partName =~ /^.*0x(\d|[abcdefABCDEF])+$/;
	  my $addressString = $partName;
	  $addressString =~ s/^.*(0x(\d|[abcdefABCDEF])+)$/\1/g;
          my $address =  hex $partName;
	  if ($address % 8 != 0) {
		die "IPC Part $::xpartName[$i] invalid: address must be 8-byte aligned\n";
	  }
	  #return "*((double *)(((void *)_ipc_shm) + $addressString))";
	  return "ipc_at_$addressString";
	}
}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string

sub frontEndCode {
	my ($i) = @_;
	my $partName = $::xpartName[$i];
	# print "IPC Address is $partName\n";
	# Check remote host IPC spec
	if (@h = $partName =~ /^(\w+:\d+):(\d+)$/g) {
	  $n_host = $::remoteIPChostIdx{$h[0]};
	  return "remote_ipc_send[$n_host][$h[1]] = $::fromExp[0];\n";
	} else {
	  die "IPC Part $partName invalid: its name must be the hex address or host:port:addr for remote IPC\n" unless
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
}
