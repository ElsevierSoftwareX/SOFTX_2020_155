
package CDS::Parser;
use Exporter;
@ISA = ('Exporter');


require "lib/Tree.pm";

# Hash of subsystem-annotated part names into part numbers
%parts;

# Hash of subsystem names into an array of InPorts and OutPorts
%sys_outs;
%sys_ins;

$root = {
	NAME => $Tree::tree_root_name,
	NEXT => [], # array of references to leaves
};

# Print block information, including all the fields
sub print_node {
   ($node) =  @_;
   #if ($node->{PRINTED} != 1) {
	print $node->{NAME}, "\n";
	foreach (keys %{$node->{FIELDS}}) {
	#	%v = %{$node->{FIELDS}};
		print "\t", $_, "\t", ${$node->{FIELDS}}{$_}, "\n";
	}
	#$_->{PRINTED} = 1;
   #}
};

sub parse() {
  my @nodes;
  push @nodes, $root;

  while (<::IN>) {
    # Strip out quotes and blank spaces
    #tr/\"/ /;
    #tr/\</ /;
    #tr/\>/ /;
    s/^\s+//;
    s/\s+$//;
    $lcntr ++;
    ($var1,$var2) = split(/\s+/,$_, 2);
    if ($var2 eq "{") { # This is new block
	$node = {
		NAME => $var1,
		NEXT => [],
		FIELDS => {},
	};
	# Get current node
	my $cur_node = pop @nodes;
	push @nodes, $cur_node;
	# Add new node to it
	push @{$cur_node->{NEXT}}, $node;
	# New node becomes current node
	push @nodes, $node;
	# New block
	#print  " " x scalar @blocks; print "Block $var1 starts on $lcntr\n";
	push @blocks, $var1;
    } elsif ($var1 eq "}") { # This is end of block
	my $bn = pop @blocks;
	# Discard this node from stack
	pop @nodes;
	#print  " " x scalar @blocks; print "Block $bn ends on $lcntr\n";
    } else {	# This is block field
	# Get current node
	my $cur_node = pop @nodes;
	push @nodes, $cur_node;
	# See if this a string continuation line
	if ("\"" eq substr $var1, 0, 1) {
		# Add the whole line to the last field
		##print "$_\n";
		$key = ${$cur_node->{LAST_FIELD_KEY}};
		# Remove double quotes
		s/^"//;s/"$//;
		${$cur_node->{FIELDS}}{$key} .= $_;
	} else {
		# Remove double quotes
		$var2 =~ s/^"//;
		$var2 =~ s/"$//;
		# Add new field to it
		${$cur_node->{FIELDS}}{$var1} = $var2;
		${$cur_node->{LAST_FIELD_KEY}} = $var1;
		#print "Block ", $cur_node->{NAME}, " fields are $var1 $var2\n";
	}
    }
  }

  #CDS::Tree::print_tree($root);
  #CDS::Tree::do_on_nodes($root, \&print_node);
  print "Lexically parsed the model file successfully\n";

  # Process parsed tree to fill in the required information
  process();

  #exit (0);
  return 1;
}

# Change Reference source name (CDS part)
sub transform_part_name {
	($r) = @_;
	$r =~ s/\/.*$//; # Delte everything after the slash

        if ($r =~ /^cdsSwitch/ || $r eq "cdsSusSw2") { $r = "MultiSwitch"; }
        elsif ($r =~ /^Matrix/) { $r = "Matrix"; }
        elsif ($r =~ /^cdsSubtract/) { $r = "DiffJunc"; }
        elsif ($r eq "dsparch4" ) { $r = "Filt"; }
        elsif ($r eq "cdsFmodule" ) { $r = "Filt"; }
        elsif ($r eq "cdsWD" ) { $r = "Wd"; }
        elsif ($r eq "cdsSusWd" ) { $r = "SusWd"; }
        elsif ($r eq "cdsSWD1" ) { $r = "SeiWd"; }
        elsif ($r eq "cdsPPFIR" ) { $r = "FirFilt"; }
        elsif ($r =~ /^cds/) {
                # Getting rid of the leading "cds"
                ($r) = $r =~ m/^cds(.+)$/;
        } elsif ($r =~ /^SIMULINK/i) { next; }

        # Capitalize first character
        $r = uc(substr($r,0, 1)) . substr($r, 1);

	return $r;
}

# Change Block type name
sub transform_block_type {
	my ($blockType) = @_;
        if ($blockType eq "Inport")	{ return "INPUT"; }
        elsif ($blockType eq "Outport") { return "OUTPUT"; }
        elsif ($blockType eq "Sum")	{ return "SUM"; }
        elsif ($blockType eq "Product") { return "MULTIPLY"; }
        elsif ($blockType eq "Ground")	{ return "GROUND"; }
        elsif ($blockType eq "Terminator") { return "TERM"; }
        elsif ($blockType eq "BusCreator") { return "BUSC"; }
        elsif ($blockType eq "BusSelector") { return "BUSS"; }
        elsif ($blockType eq "UnitDelay") { return "DELAY"; }
        elsif ($blockType eq "Logic")	{ return "AND"; }
        elsif ($blockType eq "Mux")	{ return "MUX"; }
        elsif ($blockType eq "Demux")	{ return "DEMUX"; }
	else { return $blockType; }
}

# Store line information
sub process_line {
	my ($src, $src_port, $dst, $dst_port, $node) = @_;
	my $part_num = $parts{$dst};
	if ($part_num ne undef) { 
	  if ($::partType[$part_num] eq "BUSS") {
		return; # Don't patch in any Bus Selector input links
	  }
	  #print " dst part " . $part_num . "\n";
          $::partInput[$part_num][$dst_port - 1] = $src;
          $::partInput[$part_num][$dst_port - 1] = $src;
          $::partInputPort[$part_num][$dst_port - 1] = $src_port - 1;
	  $::partInCnt[$part_num]++;
	} else {
	  # This line connected to a subsystem
	  foreach (@{$sys_ins{$dst}}) {
		my $port = ${$_->{FIELDS}}{Port};
		if ($port eq undef) { $port = 1; } # Default value for the port is one
		if ($port == $dst_port) {
			$part_num = ${$_->{FIELDS}}{PartNumber};
                	$::partInput[$part_num][0] = $dst;
                	$::partInputPort[$part_num][0] = $dst_port;
	  		$::partInCnt[$part_num] = 1;
		}
	  }
	}

	$part_num =  $parts{$src};

	# Patch in Bus Selector information
	if ($::partType[$part_num] eq "BUSS") {
             $::partInput[$part_num][$src_port - 1] = ${$node->{FIELDS}}{Name};
	     $::partInput[$part_num][$src_port - 1] =~ tr/<>//d;
	     $::partInputPort[$part_num][$src_port - 1] = $src_port - 1;
	     #print "partInputPort[$part_num][" . ($src_port - 1) . "]=", $src_port - 1 . "\n";
	     $::partInCnt[$part_num]++;
	     #print "BUSS $part_num ",  $::partInput[$part_num][$src_port - 1], "input port $src_port\n";
	}

	if ($part_num ne undef) {
	  #print " src part " . $part_num , "\n";
          $::partOutput[$part_num][$::partOutCnt[$part_num]] = $dst;
          $::partOutputPort[$part_num][$::partOutCnt[$part_num]] = $dst_port - 1;
          $::partOutputPortUsed[$part_num][$::partOutCnt[$part_num]] = $src_port-1;
	  $::partOutCnt[$part_num]++;
	} else {
	  # This line is drawn from a subsystem
	  # Find corresponding OUTPUT part in this subsystem and connect
	  # Its "Port" field (1 if missing) must correspond to $src_port
	  #print "Subsystem output " . $src . "\n";
	  foreach (@{$sys_outs{$src}}) {
		my $port = ${$_->{FIELDS}}{Port};
		if ($port eq undef) { $port = 1; } # Default value for the port is one
		if ($port == $src_port) {
			$part_num = ${$_->{FIELDS}}{PartNumber};
                	$::partOutput[$part_num][0] = $dst;
                	$::partOutputPort[$part_num][0] = $dst_port; # FIXME: main script is not using this variable correctly, it needs to be fixed
	  		$::partOutCnt[$part_num] = 1;
			#print "Connected OutPort #" . $part_num . " part " . ${$_->{FIELDS}}{Name} . " to " . $dst . "\n";
		}
	  }
	}
}

sub do_branches {
	my ($node, $in_sub, $branch) = @_;
	foreach (@{$branch->{NEXT}}) {
		if (${$_->{FIELDS}}{DstBlock} ne undef) {
		  #print ${$_->{FIELDS}}{DstBlock} . ":" . ${$_->{FIELDS}}{DstPort} . " ";
		 process_line(($in_sub? $::subSysName[$::subSys] . "_": "")
				. ${$node->{FIELDS}}{SrcBlock},
		             ${$node->{FIELDS}}{SrcPort},
			     ($in_sub? $::subSysName[$::subSys] . "_": "")
			        . ${$_->{FIELDS}}{DstBlock},
			     ${$_->{FIELDS}}{DstPort}, $node);
		} else {
			do_branches($node, $in_sub, $_);
		}
	}
	return 0;
}

# Find parts and sybsystems
sub node_processing {
   my ($node, $in_sub) =  @_;
   if ($node->{NAME} eq "Line") {
	if (${$node->{FIELDS}}{SrcBlock} eq "") {
		# Empty line?
		return 0;
	}
	my $branches = scalar @{$node->{NEXT}} != 0;
	if (1) {
	  #print "Line connecting " . ${$node->{FIELDS}}{SrcBlock} . ":" . ${$node->{FIELDS}}{SrcPort} . " and ";
	  if ($branches) {
		do_branches($node, $in_sub, $node);
	  } else {
		# Point to point connection
		#print ${$node->{FIELDS}}{DstBlock} . ":" . ${$node->{FIELDS}}{DstPort};
		process_line(($in_sub? $::subSysName[$::subSys] . "_": "")
				. ${$node->{FIELDS}}{SrcBlock},
		             ${$node->{FIELDS}}{SrcPort},
			     ($in_sub? $::subSysName[$::subSys] . "_": "")
			        . ${$node->{FIELDS}}{DstBlock},
			     ${$node->{FIELDS}}{DstPort}, $node);
	  }
	  #print "\n";
	}
   } elsif ($node->{NAME} eq "Block") {
	my $block_type = transform_block_type(${$node->{FIELDS}}{"BlockType"});
	my $block_name = ${$node->{FIELDS}}{"Name"};
	#print "Part $block_name $block_type $in_sub \n";
	# Bus creator part is the ADC board
	if ($block_type eq "BUSC") {
		$::adcCnt++;
	} 
	if ($block_type eq "SubSystem") {
                die "Cannot handle nested subsystems\n" if $in_sub;
                $::subSysPartStart[$::subSys] = $::partCnt;
                $::subSysName[$::subSys] = $block_name;
		foreach (@{$node->{NEXT}}) {
  			CDS::Tree::do_on_nodes($_, \&node_processing, 1);
		}
                $::subSysPartStop[$::subSys] = $::partCnt;
                $::subSys++;
		return 1; # Do not call this function on leaves, we already did that
	} elsif ($block_type eq "Reference") {
		# This is CDS part
		my $part_name = transform_part_name(${$node->{FIELDS}}{"SourceBlock"});

        	$::cdsPart[$::partCnt] = 1;
		$::xpartName[$::partCnt] = $::partName[$::partCnt] = $block_name;
		#print "CDS part $r\n";
        	require "lib/$part_name.pm";
        	$::partType[$::partCnt] = ("CDS::" . $part_name . "::partType") -> ();
	} else {
		# Not a CDS part
		$::partType[$::partCnt] = $block_type;
        	$::cdsPart[$::partCnt] = 0;
		$::xpartName[$::partCnt] = $::partName[$::partCnt] = $block_name;
	}
	if (!$in_sub) {
		$::nonSubPart[$::nonSubCnt] = $::partCnt;
		$::nonSubCnt++;
	} else {
		$::partSubNum[$::partCnt] = $::subSys;
		$::partSubName[$::partCnt] = $::subSysName[$::subSys];
		$::xpartName[$::partCnt] = $::subSysName[$::subSys] . "_" . $::xpartName[$::partCnt];
	}
	# For easy access
	#print "Part ". $::xpartName[$::partCnt] . "\n";
	$parts{$::xpartName[$::partCnt]} = $::partCnt;
	#$nodes{$::xpartName[$::partCnt]} = $node;
	if ($block_type eq "INPUT") {
		if ($in_sub) {
			push @{$sys_ins{$::subSysName[$::subSys]}},  $node;
		} else {
			# do not put these ports into top-level system
			die "Input ports are only supported in subsystems\n";
		}
	}
	if ($block_type eq "OUTPUT") {
		if ($in_sub) {
			push @{$sys_outs{$::subSysName[$::subSys]}},  $node;
		} else {
			# do not put these ports into top-level system
			die "Output ports are only supported in subsystems\n";
		}
	}
	if ($block_type eq "SUM") {
		$::partInputs[$::partCnt] = ${$node->{FIELDS}}{Inputs};
		$::partInputs[$::partCnt] =~ tr/+-//cd; # delete other characters
	} elsif ($block_type eq "MULTIPLY" &&
		 ${$node->{FIELDS}}{Inputs} eq "*\/") {
		$::partType[$::partCnt] = "DIVIDE";
		$::partInputs[$::partCnt] = ${$node->{FIELDS}}{Inputs};
	}
	${$node->{FIELDS}}{PartNumber} = $::partCnt; # Store our part number
	$::partCnt++;
   }
   return 0;
}

sub process {

  # Find first System node, this is the top level subsystem
  my $system_node = CDS::Tree::find_node($root, "System");

  # Set system name
  $::systemName = ${$system_node->{FIELDS}}{"Name"};

  # There is really nothing needed below System node in the tree so set new root
  $root = $system_node;

  CDS::Tree::do_on_nodes($root, \&node_processing, 0);
  print "Found $::adcCnt ADCs $::partCnt parts $::subSys subsystems\n";

  return 1;
}
