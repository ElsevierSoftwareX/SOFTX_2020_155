
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

#:TODO: lexical analyzer should check MDL format syntax
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
	$r =~ s/\/.*$//; # Delete everything after the slash

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
			# This is completely weird code, fitted here to match the existing code
			# generator... All this needs to be fixed
          	        $::partOutputPortUsed[$part_num][0] = $src_port-1;
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

# There could be nested branching structure (1 to many connection)
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

# Check name to contain only allowd characters
sub name_check {
	return @_[0] =~ /^[a-zA-Z0-9_]+$/;
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
	my $source_type = transform_block_type(${$node->{FIELDS}}{"SourceType"});
	my $source_block = transform_block_type(${$node->{FIELDS}}{"SourceBlock"});
	my $block_name = ${$node->{FIELDS}}{"Name"};
	#print "Part $block_name $block_type $in_sub \n";
	# Skip certain blocks
 	if ($source_type eq "DocBlock") {
		return 0;
	}

	# Bus creator part is the ADC board
	if ($block_type eq "BUSC") {
        	require "lib/Adc.pm";
        	CDS::Adc::initAdc($node);
        	$::partType[$::partCnt] = CDS::Adc::partType($node);
	} 
	if ($block_type eq "SubSystem") {
                die "Cannot handle nested subsystems\n" if $in_sub;
                $::subSysPartStart[$::subSys] = $::partCnt;
                $::subSysName[$::subSys] = $block_name;
		if (!name_check($block_name)) {
			die "Invalid subsystem name \"$block_name\"";
		}
		foreach (@{$node->{NEXT}}) {
  			CDS::Tree::do_on_nodes($_, \&node_processing, 1);
		}
                $::subSysPartStop[$::subSys] = $::partCnt;
                $::subSys++;
		return 1; # Do not call this function on leaves, we already did that
	} elsif ($block_type eq "Reference") {
		# Skip Parameters block
		#if ($source_block =~ /^cdsParameters/) {
		  #return 0;
	 	#}
		# This is CDS part

        	$::cdsPart[$::partCnt] = 1;
		$::xpartName[$::partCnt] = $::partName[$::partCnt] = $block_name;
		#print "CDS part $block_name type $source_block\n";
	} else {
		# Not a CDS part
		$::partType[$::partCnt] = $block_type;
        	$::cdsPart[$::partCnt] = 0;
		$::xpartName[$::partCnt] = $::partName[$::partCnt] = $block_name;
	}
	# Check names; pass ADC parts and Remote Interlinks
	if ($::partName[$::partCnt] !~ /^Bus\\n/ && $source_block !~ /^cdsRemoteIntlk/ && $source_block !~ /^cdsParameters/) {
	    if (!name_check($::partName[$::partCnt])) {
		die "Invalid part name \"$::partName[$::partCnt]\"; source_block \"$source_block\"; block  type \"$block_type\"";
	    }
	}
	if (!$in_sub) {
		$::nonSubPart[$::nonSubCnt] = $::partCnt;
		$::nonSubCnt++;
	} else {
		$::partSubNum[$::partCnt] = $::subSys;
		$::partSubName[$::partCnt] = $::subSysName[$::subSys];
		$::xpartName[$::partCnt] = $::subSysName[$::subSys] . "_" . $::xpartName[$::partCnt];
	}
	if ($::cdsPart[$::partCnt]) {
		my $part_name = transform_part_name(${$node->{FIELDS}}{"SourceBlock"});
        	require "lib/$part_name.pm";
		if ($part_name eq "Dac") {
        	  $::partType[$::partCnt] = CDS::Dac::initDac($node);
		}
        	$::partType[$::partCnt] = ("CDS::" . $part_name . "::partType") -> ();
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
	} elsif ($block_type eq "RelationalOperator") {
		$::partInputs[$::partCnt] = ${$node->{FIELDS}}{Operator};
	}
	${$node->{FIELDS}}{PartNumber} = $::partCnt; # Store our part number
	$::partCnt++;
   }
   return 0;
}

# subsystems queue
@subsys;

# subsystems level
@subsys_level;

# recursive branch processing
# annotate names
sub flatten_do_branches {
   ($_, $ant) = @_;
   if (${$_->{FIELDS}}{Parent} == 1) { return; } # Stop annotating if discovered parent's block
   foreach (@{$_->{NEXT}}) {
     if (${$_->{FIELDS}}{DstBlock} ne undef) {
       ${$_->{FIELDS}}{DstBlock} = $ant . ${$_->{FIELDS}}{DstBlock};
       #print ${$_->{FIELDS}}{DstBlock}, ":", ${$_->{FIELDS}}{DstPort}, " ";
     } else {
       flatten_do_branches($_, $ant);
     }
   }
}

# Find a line in the $node, starting from $src_name and $src_port
sub find_line {
   my ($node, $src_name, $src_port) = @_;
   foreach (@{$node->{NEXT}}) {
	if ($_->{NAME} eq "Line") {
	  if (${$_->{FIELDS}}{SrcBlock} eq $src_name && ${$_->{FIELDS}}{SrcPort} == $src_port) {
		return $_;
	  }
	}
   }
   return undef;
}
# Find a branch (or a line) in the $node, leading to $dst_name and $dst_port
sub find_branch {
   my ($node, $dst_name, $dst_port) = @_;
   foreach (@{$node->{NEXT}}) {
	if ($_->{NAME} eq "Line" || $_->{NAME} eq "Branch") {
	  my $dprt = ${$_->{FIELDS}}{DstPort};
	  if ($dprt == undef) { $dprt = 1; }
	  #print "find_branch: ", ${$_->{FIELDS}}{DstBlock}, ":", $dprt, "\n";
	  if (${$_->{FIELDS}}{DstBlock} eq $dst_name && $dprt == $dst_port) {
		return $_;
	  }
	}
	my $block = find_branch($_, $dst_name, $dst_port);
	if ($block ne undef) {
	  return $block;
	}
   }
   return undef;
}

# This recursive function flattens the bottom most system
# first and then flattens the rest of the systems in ascending order.
sub flatten {
   my @inports;
   my @outports;
   my @lines;
   my ($node) =  @_;

   if ($node->{NAME} eq "Block"
       && ${$node->{FIELDS}}{BlockType} eq "SubSystem") {
     push @subsys, $node;
   }
   foreach (@{$node->{NEXT}}) {
	flatten($_);
   }
   if ($node->{NAME} eq "Block"
       && ${$node->{FIELDS}}{BlockType} eq "SubSystem") {

#print "Subsys=";
#foreach (@subsys) {
  #print ${$_->{FIELDS}}{Name}, ", ";
#}
#print "\n";

     $parent = pop @subsys;
     if ($parent == $node) {
        $parent = pop @subsys;
     }
     push @subsys, $parent;

     #print "Flattening ", ${$node->{FIELDS}}{Name}, "\n";
     #print "Parent ", ${$parent->{FIELDS}}{Name}, "\n";
     # Remove node from parent
     my $idx = 0;
     # Parent node has "System" node next, move down to it
     if ($parent->{NAME} ne "System") {
       $parent = ${$parent->{NEXT}}[0];
     }
     foreach (@{$parent->{NEXT}}) {
	if ($_ == $node) {
		#print "Found node at index $idx\n";
		last;
	}
	$idx++;
     }
     splice(@{$parent->{NEXT}}, $idx, 1,);

     # Annotate blocks in this node with its name
     $node = ${$node->{NEXT}}[0]; # Move down to the "System" node
     #print "Following blocks found in ", ${$node->{FIELDS}}{Name}, ":\n";
     foreach (@{$node->{NEXT}}) {
	if ($_->{NAME} eq "Block") {
	  if (${$_->{FIELDS}}{BlockType} eq "Inport") {
	    push @inports, $_; # skip all nput ports
	  } elsif (${$_->{FIELDS}}{BlockType} eq "Outport") {
	    push @outports, $_; # skip all output ports
	  } else {
	    ${$_->{FIELDS}}{Name} = ${$node->{FIELDS}}{Name} . "_" . ${$_->{FIELDS}}{Name};
	    #print "Block ", $_->{NAME}, ", type=", ${$_->{FIELDS}}{BlockType}, ", name=",${$_->{FIELDS}}{Name}, "\n";
	    unshift @{$parent->{NEXT}}, $_; # add to the parent's list (prepend, lines need to stay at the end)
	  }
 	} elsif ($_->{NAME} eq "Line") {
	  push @lines, $_;
	  # Do not annotate lines just yet
	  #${$_->{FIELDS}}{SrcBlock} = ${$node->{FIELDS}}{Name} . "_" . ${$_->{FIELDS}}{SrcBlock};
	  #${$_->{FIELDS}}{DstBlock} = ${$node->{FIELDS}}{Name} . "_" . ${$_->{FIELDS}}{DstBlock};
	}
     }
     #print "Following lines found in ", ${$node->{FIELDS}}{Name}, ":\n";
     foreach (@{$node->{NEXT}}) {
	if ($_->{NAME} eq "Line") {
	  my $branches = scalar @{$_->{NEXT}} != 0;
	  if ($branches) {
	    #print "Branched Source=", ${$_->{FIELDS}}{SrcBlock}, ":", ${$_->{FIELDS}}{SrcPort}, ", dst=";
	    flatten_do_branches($_);
	    #print "\n";
	  } else {
	    #print "Source=", ${$_->{FIELDS}}{SrcBlock}, ":", ${$_->{FIELDS}}{SrcPort}, ", dst=",
		${$_->{FIELDS}}{DstBlock}, ":", ${$_->{FIELDS}}{DstPort}, "\n";
	  }
 	}
     }

     # Hook up input lines
     foreach (@inports) {
	my $port_num = ${$_->{FIELDS}}{Port};
	if ($port_num eq undef) { $port_num = 1; }
	my $port_name = ${$_->{FIELDS}}{Name};
	#print "Processing input port #$port_num name=$port_name\n";
	# Find line connected to this input port (if any)
	foreach $line (@lines) {
	  if (${$line->{FIELDS}}{SrcBlock} eq $port_name) {
	    #print "Found line Source=", ${$line->{FIELDS}}{SrcBlock}, ":", ${$line->{FIELDS}}{SrcPort}, " dst=",  ${$line->{FIELDS}}{DstBlock}, ":", ${$line->{FIELDS}}{DstPort}, "\n";
	    # In the parent find the line or a branch connected to this $node and $port_num
	    my $branch = find_branch($parent, ${$node->{FIELDS}}{Name}, $port_num);
	    if ($branch eq undef) {
		die "Port $port_name disconnected\n";
	    } else {
		if (${$branch->{FIELDS}}{SrcBlock} ne undef) {
		  #print "Found parent line Source=", ${$branch->{FIELDS}}{SrcBlock}, ":", ${$branch->{FIELDS}}{SrcPort}, "\n";
		} else {
		  #print "Found parent branch\n";
		}
	    }
	    # Hook up parent branch
	    if (0 == scalar @{$line->{NEXT}}) {
	      ${$branch->{FIELDS}}{DstBlock} = ${$node->{FIELDS}}{Name} . "_" . ${$line->{FIELDS}}{DstBlock};
	      ${$branch->{FIELDS}}{DstPort} =  ${$line->{FIELDS}}{DstPort};

	    } else {
	      flatten_do_branches($line, ${$node->{FIELDS}}{Name} . "_");
              # change this line into a branch 
	      ${$line->{FIELDS}}{SrcBlock} = undef;
	      ${$line->{FIELDS}}{SrcPort} = undef;
	      $line->{NAME} = "Branch";
	      # Reset parent's destination
	      ${$branch->{FIELDS}}{DstBlock} = undef;
	      ${$branch->{FIELDS}}{DstPort} = undef;
	      push @{$branch->{NEXT}}, $line;
  	      #CDS::Tree::do_on_nodes($branch, \&print_node);
	    }

	    # Remove this line from the list in this node
	    $idx = 0;
	    foreach $block (@{$node->{NEXT}}) {
		if ($block == $line) {
			last;
		}
		$idx++;
	    }
     	    splice(@{$node->{NEXT}}, $idx, 1,);
	  }
	}
     }

     # Hook up output lines
     foreach (@outports) {
	my $port_num = ${$_->{FIELDS}}{Port};
	if ($port_num eq undef) { $port_num = 1; }
	my $port_name = ${$_->{FIELDS}}{Name};
	#print "Processing output port #$port_num name=$port_name\n";
	# Find line connected to this output port (if any)
	my $branch = find_branch($node, $port_name, 1);
	die "OutPort $port_name disconnected\n" if ($branch eq undef);
	# Find parent's line connected to this node, output port $port_num
	my $line = find_line($parent, ${$node->{FIELDS}}{Name}, $port_num);
	if ($line eq undef) {
     		print "Flattening ", ${$node->{FIELDS}}{Name}, "\n";
     		print "Parent ", ${$parent->{FIELDS}}{Name}, "\n";
		print "Processing output port #$port_num name=$port_name\n";
		die "Disconnected output port\n";
	}
	# Hook the line up
	if (${$branch->{FIELDS}}{SrcBlock} ne "") {
	  # There is no branching in the inside line, just a line
	  # Hook parent line's input up and remove the inside line
	  ${$line->{FIELDS}}{SrcBlock} = ${$node->{FIELDS}}{Name} . "_" . ${$branch->{FIELDS}}{SrcBlock};
	  ${$line->{FIELDS}}{SrcPort} =  ${$branch->{FIELDS}}{SrcPort};

	  # Remove this line from the list in this node
	  $idx = 0;
	  foreach $block (@{$node->{NEXT}}) {
	    if ($block == $branch) {
		last;
	    }
	    $idx++;
	  }
     	  splice(@{$node->{NEXT}}, $idx, 1,);
	} else {
	  #die "Unsupported line processing $port_name in ${$node->{FIELDS}}{Name}\n";
	  # There is some sort of branching structure in the inside subsystem
          # Change parent line into a branch 
	  ${$line->{FIELDS}}{SrcBlock} = undef;
	  ${$line->{FIELDS}}{SrcPort} = undef;
	  $line->{NAME} = "Branch";
	  # Remove former destination
	  ${$branch->{FIELDS}}{DstBlock} = undef;
	  ${$branch->{FIELDS}}{DstPort} = undef;
	  # Mark it here to stop annotating names in flatten_do_branches() later
	  ${$branch->{FIELDS}}{Parent} = 1;
	  # Insert parent's line into the branch
	  push @{$branch->{NEXT}}, $line;
	}
	
     }

     # Annotate and add all remaining lines to the parent's list
     #print "Following lines remaining in ", ${$node->{FIELDS}}{Name}, ":\n";
     my $ant = ${$node->{FIELDS}}{Name} . "_";
     foreach (@{$node->{NEXT}}) {
	if ($_->{NAME} eq "Line") {
	  my $branches = scalar @{$_->{NEXT}} != 0;
          ${$_->{FIELDS}}{SrcBlock} = $ant . ${$_->{FIELDS}}{SrcBlock};
	  if ($branches) {
	    #print "Branched Source=", ${$_->{FIELDS}}{SrcBlock}, ":", ${$_->{FIELDS}}{SrcPort}, ", dst=";
	    flatten_do_branches($_, $ant);
	    #print "\n";
	  } else {
            ${$_->{FIELDS}}{DstBlock} = $ant . ${$_->{FIELDS}}{DstBlock};
	    #print "Source=", ${$_->{FIELDS}}{SrcBlock}, ":", ${$_->{FIELDS}}{SrcPort}, ", dst=",
		#${$_->{FIELDS}}{DstBlock}, ":", ${$_->{FIELDS}}{DstPort}, "\n";
	  }
	  # Add to parent
	  push @{$parent->{NEXT}}, $_; # add to the parent's list
 	}
     }

if (0) {
     print "List of parent lines:\n";
     foreach (@{$parent->{NEXT}}) {
	if ($_->{NAME} eq "Line") {
	  my $branches = scalar @{$_->{NEXT}} != 0;
	  if ($branches) {
	    print "Branched Source=", ${$_->{FIELDS}}{SrcBlock}, ":", ${$_->{FIELDS}}{SrcPort}, ", dst=";
	    flatten_do_branches($_);
	    print "\n";
	  } else {
	    print "Source=", ${$_->{FIELDS}}{SrcBlock}, ":", ${$_->{FIELDS}}{SrcPort}, ", dst=",
		${$_->{FIELDS}}{DstBlock}, ":", ${$_->{FIELDS}}{DstPort}, "\n";
	  }
 	}
     }
     print "List of parent blocks:\n";
     foreach (@{$parent->{NEXT}}) {
	if ($_->{NAME} eq "Block") {
	  print "Block ", $_->{NAME}, ", type=", ${$_->{FIELDS}}{BlockType}, ", name=",${$_->{FIELDS}}{Name}, "\n";
	}
     }
}
   }
}

sub flatten_nested_subsystems {
   my ($node) =  @_;

# This code flattens all subsystems
# It is not working properly
if (0) {
   foreach (@{$node->{NEXT}}) {
     if ($_->{NAME} eq "Block" && ${$_->{FIELDS}}{BlockType} eq "SubSystem") {
	print "Top-level subsystem ", ${$_->{FIELDS}}{Name}, "\n";
	@subsys = ($node);
	flatten($_);
     }
   }
}

# This code flattens only second-level subsystems
if (1) {
   # Find all top-level subsystems
   foreach (@{$node->{NEXT}}) {
     if ($_->{NAME} eq "Block" && ${$_->{FIELDS}}{BlockType} eq "SubSystem") {
	print "Top-level subsystem ", ${$_->{FIELDS}}{Name}, "\n";
	# Flatten all second-level subsystems
	my $system = $_->{NEXT}[0];
	foreach $ssub (@{$system->{NEXT}}) {
          if ($ssub->{NAME} eq "Block" && ${$ssub->{FIELDS}}{BlockType} eq "SubSystem") {
	    print "Second-level subsystem ", ${$ssub->{FIELDS}}{Name}, "\n";
	    @subsys = ($_);
            flatten($ssub);
	  }
	}
     }
   }
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

  print "Flattening the model\n";
  flatten_nested_subsystems($root);
  print "Finished flattening the model\n";
  CDS::Tree::do_on_nodes($root, \&node_processing, 0);
  print "Found $::adcCnt ADCs $::partCnt parts $::subSys subsystems\n";

  # See to it that ADC is on the top level
  # This is needed because the main script can't handle ADCs in the subsystems
  # :TODO: fix main script to handle ADC parts in subsystems
  foreach (0 ... $::partCnt) {
    if ($::partType[$_] eq "BUSS" || $::partType[$_] eq "BUSC" || $::partType[$_] eq "Dac") {
      if ($::partSubName[$_] ne "") {
	die "All ADCs and DACs must be on the top level in the model";
      }
    }
  }

  return 1;
}

return 1;
