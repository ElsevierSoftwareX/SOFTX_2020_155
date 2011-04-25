
package CDS::Parser;
use Exporter;
use Cwd;
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
sub parse {
  my ($myroot, $desc, $dbg) =  @_;
  my @nodes;
  if (defined $myroot) {
	push @nodes, $myroot;
  } else {
	push @nodes, $root;
	$myroot = $root;
  }

  my $tagcntr = 0;

  my $top_level = 0;
  if (!defined $desc) {
	  $desc = ::IN;
	  $top_level = 1;
  }
  while (<$desc>) {
    if ($dbg) {
	    #print $_;
    }
    # Strip out quotes and blank spaces
    #tr/\"/ /;
    #tr/\</ /;
    #tr/\>/ /;
    s/^\s+//;
    s/\s+$//;
    my $lcntr = 0;
    $lcntr ++;

    my ($var1,$var2) = split(/\s+/,$_, 2);
    if ($var2 eq "{") { # This is new block
	my $node = {
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
		my $key = ${$cur_node->{LAST_FIELD_KEY}};
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

                if ($var2 =~ /^cdsIPCx_/) {
                   $::blockTag[$tagcntr] = $var2;
                   $tagcntr++;
                }
                else {
                   $::blockTag[$tagcntr] = undef;
                   $tagcntr++;
                }
	}
    }
  }

  #CDS::Tree::do_on_nodes($myroot, \&print_node);
  print "Lexically parsed the model file successfully\n";

  # Process parsed tree to fill in the required information
  #if ($top_level) {
  #print "Starting node processing\n";
  #process();
  #}

  #exit (0);
  return 1;
}

# Change Reference source name (CDS part)
sub transform_part_name {
        $::ppFIR[$::partCnt] = 0;         # Set to zero initially; change to 1 below for PPFIR
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
        elsif ($r eq "cdsPPFIR" ) { $r = "Filt"; $::useFIRs = 1; $::ppFIR[$::partCnt] = 1; }
        elsif ($r eq "cdsFirFilt" ) { $r = "Filt"; $::useFIRs = 1; }
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
        elsif ($blockType eq "Math")	{ return "MATH"; }                 # ===  MA  ===
        elsif ($blockType eq "Fcn")	{ return "FCN"; }                  # ===  MA  ===
        elsif ($blockType eq "Ground")	{ return "GROUND"; }
        elsif ($blockType eq "Constant")	{ return "CONSTANT"; }
        elsif ($blockType eq "Saturate")	{ return "SATURATE"; }
        elsif ($blockType eq "Terminator") { return "TERM"; }
        elsif ($blockType eq "BusCreator") { return "BUSC"; }
        elsif ($blockType eq "BusSelector") { return "BUSS"; }
        elsif ($blockType eq "UnitDelay") { return "DELAY"; }
        elsif ($blockType eq "Logic")	{ return "AND"; }
        elsif ($blockType eq "Mux")	{ return "MUX"; }
        elsif ($blockType eq "Demux")	{ return "DEMUX"; }
        elsif ($blockType eq "From")	{ return "FROM"; }
        elsif ($blockType eq "Goto")	{ return "GOTO"; }
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
	  #print " dst part " . $part_num . "type = $::partType[$part_num]\n";
          $::partInput[$part_num][$dst_port - 1] = $src;
          $::partInputPort[$part_num][$dst_port - 1] = $src_port - 1;
	  if ($::partType[$part_num] eq "Dac") { $::partInCnt[$part_num] = 16; }
	  elsif ($::partType[$part_num] eq "Dac18") { $::partInCnt[$part_num] = 8; }  # ===  MA-2011  ===
	  else { $::partInCnt[$part_num]++; }
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
                	$::partOutput[$part_num][$::partOutCnt[$part_num]] = $dst;
                	$::partOutputPort[$part_num][$::partOutCnt[$part_num]] = $dst_port; # FIXME: main script is not using this variable correctly, it needs to be fixed
	  		$::partOutCnt[$part_num]++;
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

# Check name to contain only allowed characters
sub name_check {
	return @_[0] =~ /^[a-zA-Z0-9_]+$/;
}

# Bring library references into the tree
sub merge_references {
   my ($node, $in_sub, $parent) =  @_;
   if ($node->{NAME} ne "Block") {
	   return 0;
   }
   my $name =  ${$node->{FIELDS}}{"Name"};
   my $src_block = ${$node->{FIELDS}}{"SourceBlock"};
   if ($src_block =~ /^cdsParameters/ || $src_block eq undef) {
	  return 0;
   }
   #print "name=", $name, "src=", $src_block, "\n";
   my $part_name = transform_part_name(${$node->{FIELDS}}{"SourceBlock"});
   if (-e "lib/$part_name.pm") {
	   return 0; # Library part
   }
   my $ref = ${$node->{FIELDS}}{"SourceBlock"};
   print "Found a library reference $ref\n";
   #my $cwd = getcwd;
   #print $cwd," ", $part_name, "\n";
   my $fname = $ref;
   $fname =~ s/\/.*$//; # Delete everything after the slash
   my $system_name = $ref;
   $system_name =~ s/^.*\///; # Strip everything before last slash
   #print $system_name, "\n";
   $fname = "../simLink/$fname.mdl";
   die "Can't find $fname\n" unless -e $fname;
   print "Found the library model $fname\n";
   my $myroot = {
	NAME => $Tree::tree_root_name,
	NEXT => [], # array of references to leaves
   };

   #CDS::Tree::print_tree($parent);
   # Parse the library file
   open(in, "<$fname") || die "***ERROR: $fname not found\n";
   parse($myroot, in, 1);
   #print "After", "\n";
   #CDS::Tree::print_tree($parent);
   #exit(1);
   #return 0;
   close in;
   my $mynode = CDS::Tree::find_node($myroot, $system_name, "Name");
   die "Couldn't find $system_name in $fname\n" unless defined $mynode;
   #CDS::Tree::print_tree($mynode);
   #${$node->{FIELDS}}{"__merge_references__"} = $mynode;
   # Replace subsystem name
   ${$mynode->{FIELDS}}{"Name"} = $name;
   my $next = $mynode->{NEXT}[0];
   ${$next->{FIELDS}}{"Name"} = $name;
   $_ = $mynode; # replace the current node (do_on_nodes)
   $n_merged++;
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
	my $source_type = transform_block_type(${$node->{FIELDS}}{"SourceType"});
	my $source_block = transform_block_type(${$node->{FIELDS}}{"SourceBlock"});
	my $block_name = ${$node->{FIELDS}}{"Name"};

        if ($block_type ne "SubSystem") {
           my $block_tag = transform_block_type(${$node->{FIELDS}}{"Tag"});
           #$::ipcxBlockTags[$::ipcxTagCount++] = $block_tag;
           $::ipcxBlockTags[$::partCnt] = $block_tag;
	   #print "IPC block tag $::partCnt is $block_tag \n";
        }

	#print "Part $block_name $block_type $in_sub \n";
	# Skip certain blocks
 	if ($source_type eq "DocBlock") {
		return 0;
	}
        # Process Math Function blocks  ========================================  MA  ===
        if ($block_type eq "MATH") {                                       # ===  MA  ===
           my $math_op = ${$node->{FIELDS}}{"Operator"};                   # ===  MA  ===
           if ($math_op eq "square") {                                     # ===  MA  ===
              $block_type = "M_SQR";                                       # ===  MA  ===
           }                                                               # ===  MA  ===
           elsif ($math_op eq "sqrt") {                                    # ===  MA  ===
              $block_type = "M_SQT";                                       # ===  MA  ===
           }                                                               # ===  MA  ===
           elsif ($math_op eq "reciprocal") {                              # ===  MA  ===
              $block_type = "M_REC";                                       # ===  MA  ===
           }                                                               # ===  MA  ===
           elsif ($math_op eq "mod") {                                     # ===  MA  ===
              $block_type = "M_MOD";                                       # ===  MA  ===
           }                                                               # ===  MA  ===
           else {                                                          # ===  MA  ===
              die "*** ERROR: Math operator not supported: $math_op \n";   # ===  MA  ===
           }                                                               # ===  MA  ===
        }                                                                  # ===  MA  ===
        # Process User-defined Inline Function block  ==========================  MA  ===
        if ($block_type eq "FCN") {                                        # ===  MA  ===
           my $expr = ${$node->{FIELDS}}{"Expr"};                          # ===  MA  ===
           if ($expr =~ /acos|asin|cosh|sinh/) {                           # ===  MA  ===
              my $errmsg = "Inverse trig/Hyperbolic math function";        # ===  MA  ===
              die "*** ERROR: $errmsg not suppported:\n\t   $expr \n";     # ===  MA  ===
           }                                                               # ===  MA  ===
           elsif ($expr =~ /tan/) {                                        # ===  MA  ===
              my $errmsg = "Tangent math function";                        # ===  MA  ===
              die "*** ERROR: $errmsg not suppported:\n\t   $expr \n";     # ===  MA  ===
           }                                                               # ===  MA  ===
           elsif ($expr =~ /ceil|floor/) {                                 # ===  MA  ===
              my $errmsg = "Ceiling/Floor math function";                  # ===  MA  ===
              die "*** ERROR: $errmsg not suppported:\n\t   $expr \n";     # ===  MA  ===
           }                                                               # ===  MA  ===
           elsif ($expr =~ /hypot|pow|rem|sgn/) {                          # ===  MA  ===
              my $errmsg = "Hypotenuse/Power/Remainder/Signum";            # ===  MA  ===
              $errmsg .= " math function";                                 # ===  MA  ===
              die "*** ERROR: $errmsg not suppported:\n\t   $expr \n";     # ===  MA  ===
           }                                                               # ===  MA  ===
           elsif ($expr =~ /exp|ln|log\s*\(/) {                            # ===  MA  ===
              my $errmsg = "Exponent/Natural log math function";           # ===  MA  ===
              die "*** ERROR: $errmsg not suppported:\n\t   $expr \n";     # ===  MA  ===
           }                                                               # ===  MA  ===
           elsif ($expr =~ /[^f]{1}abs/) {                                 # ===  MA  ===
              my $errmsg = "Integer abs math function";                    # ===  MA  ===
              die "*** ERROR: $errmsg not suppported:\n\t   $expr \n";     # ===  MA  ===
           }                                                               # ===  MA  ===
           $::functionExpr = $expr;                                        # ===  MA  ===
        }                                                                  # ===  MA  ===

	$::partOutputs[$::partCnt] = ${$node->{FIELDS}}{Outputs};

	# Account for default Demux 4 outputs (missing # outs number)
	if ($block_type eq "DEMUX" && ${$node->{FIELDS}}{Outputs} eq undef) {
		$::partOutputs[$::partCnt] = 4;
	}

	# Bus creator part is the ADC board
	#if ($block_type eq "BUSC") {
        #	require "lib/Adc.pm";
        #	CDS::Adc::initAdc($node);
        #	$::partType[$::partCnt] = CDS::Adc::partType($node);
	#} 
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
                if ($source_block =~ /cdsIPCx/) {
                   if ($block_name !~ /[CGHKLMSX]\d\:/) {
                      die "***ERROR: Signal name of IPCx module must include IFO: $block_name\n";
                   }
                }

                $descr=(${$node->{FIELDS}}{"Description"});
                #  print "DESCR=$descr\n";
                $::blockDescr[$::partCnt] = $descr;
		# Skip Parameters block
		#if ($source_block =~ /^cdsParameters/) {
		  #return 0;
	 	#}
		# This is CDS part

        	$::cdsPart[$::partCnt] = 1;
		$::xpartName[$::partCnt] = $::partName[$::partCnt] = $block_name;
		#print "CDS part $block_name type $source_block\n";
        } elsif ($block_type eq "FCN") {                                   # ===  MA  ===
        	$::cdsPart[$::partCnt] = 1;                                # ===  MA  ===
		$::xpartName[$::partCnt] = $::partName[$::partCnt] = $block_name; #= MA =
	} else {
		# Not a CDS part
		$::partType[$::partCnt] = $block_type;
        	$::cdsPart[$::partCnt] = 0;
		$::xpartName[$::partCnt] = $::partName[$::partCnt] = $block_name;
		# If a GOTO tag, have to put its tag name in the output location for later connection of parts.
		if ($block_type eq "GOTO") {
		     $::partOutput[$::partCnt][3] = ${$node->{FIELDS}}{GotoTag};
          		$::partOutCnt[$::partCnt] = 0;
			#print "PROCESSED GOTO $::partCnt $::xpartName[$::partCnt] $::partOutput[$::partCnt][0]***************\n";
		}
		# If a FROM tag, need to get the tag name and put it into input parameters for later connection of parts.
		if ($block_type eq "FROM") {
		     $::partInput[$::partCnt][3] = ${$node->{FIELDS}}{GotoTag};
		     $::partInCnt[$::partCnt] = 0;
			#print "PROCESSED FROM $::partCnt $::partInput[$::partCnt][0] ***************\n";
		}
	}
	#if ($::partName[$::partCnt] ~~ /^Bus\\n/)
	#{
		#print("BUS found \"$::partName[$::partCnt]\" $source_block\n");
	#}

	# Check names; pass ADC parts and Remote Interlinks
	# Allow IPC part through
	if ($::partName[$::partCnt] !~ /Bus\\n/
	    && $source_block !~ /^cdsRemoteIntlk/
	    && $source_block !~ /^cdsParameters/
	    && $source_block !~ /^cdsIPC/
	    && $source_block !~ /^cdsEzCa/) {
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
                if ($block_type eq "FCN") {                                # ===  MA  ===
                   $part_name = "Fcn";                                     # ===  MA  ===
                }                                                          # ===  MA  ===
		print ${$node->{FIELDS}}{"SourceBlock"}, "\n";
		if (! -e "lib/$part_name.pm") {
			die "Can't find part code in lib/$part_name.pm\n";
			if (0) {
			my $ref = ${$node->{FIELDS}}{"SourceBlock"};
			print "Found a library reference $ref\n";
			#my $cwd = getcwd;
			#print $cwd," ", $part_name, "\n";
			my $fname = $ref;
			$fname =~ s/\/.*$//; # Delete everything after the slash
			my $system_name = $ref;
			$system_name =~ s/^.*\///; # Strip everything before last shash
			#print $system_name, "\n";
			$fname = "../simLink/$fname.mdl";
			die "Can't find $fname\n" unless -e $fname;
			print "Found the library model $fname\n";
my $myroot = {
	NAME => $Tree::tree_root_name,
	NEXT => [], # array of references to leaves
};

			# Parse the library file
   			open(in, "<$fname") || die "***ERROR: $fname not found\n";
			parse($myroot, in, 1);
			close in;
			#CDS::Tree::print_tree($myroot);
   			my $mynode = CDS::Tree::find_node($myroot, $system_name, "Name");
			die "Couldn't find $system_name in $fname\n" unless defined $mynode;
			CDS::Tree::print_tree($mynode);
			exit(1);
		}
		} else {
       		require "lib/$part_name.pm";
		if ($part_name eq "Dac") {
        	  $::partType[$::partCnt] = CDS::Dac::initDac($node);
		}
		if ($part_name eq "Dac18") {                               # ===  MA-2011  ===
        	  $::partType[$::partCnt] = CDS::Dac18::initDac($node);    # ===  MA-2011  ===
		}                                                          # ===  MA-2011  ===
		if ($part_name eq "Contec1616DIO") {
        	  $::partType[$::partCnt] = CDS::Contec1616DIO::initCDIO1616($node);
                  if ($::boCnt > $::maxDioMod) {
                     die "Too many Digital I/O modules \(max is $::maxDioMod\)\n";
                  }
		}
		if ($part_name eq "Contec6464DIO") {
        	  $::partType[$::partCnt] = CDS::Contec6464DIO::initCDIO6464($node);
                  if ($::boCnt > $::maxDioMod) {
                     die "Too many Digital I/O modules \(max is $::maxDioMod\)\n";
                  }
		}
		if ($part_name eq "CDO32") {
        	  $::partType[$::partCnt] = CDS::CDO32::initCDO32($node);
                  if ($::boCnt > $::maxDioMod) {
                     die "Too many Digital I/O modules \(max is $::maxDioMod\)\n";
                  }
		}
		if ($part_name eq "Dio") {
        	  $::partType[$::partCnt] = CDS::Dio::initDio($node);
                  if ($::boCnt > $::maxDioMod) {
                     die "Too many Digital I/O modules \(max is $::maxDioMod\)\n";
                  }
		}
		if ($part_name eq "Rio") {
        	  $::partType[$::partCnt] = CDS::Rio::initRio($node);
                  if ($::boCnt > $::maxDioMod) {
                     die "Too many Digital I/O modules \(max is $::maxDioMod\)\n";
                  }
		}
		if ($part_name eq "Rio1") {
        	  $::partType[$::partCnt] = CDS::Rio1::initRio1($node);
                  if ($::boCnt > $::maxDioMod) {
                     die "Too many Digital I/O modules \(max is $::maxDioMod\)\n";
                  }
		}
# Added for ADC PART CHANGE *****************
		if ($part_name eq "Adcx0") {
			require "lib/Adc.pm";
			#CDS::Adc::initAdc($node);
			CDS::Adcx0::initAdc($node);
			$::partType[$::partCnt] = CDS::Adcx0::partType($node);
		} 
		if ($part_name eq "Adcx1") {
			require "lib/Adc.pm";
			#CDS::Adcx1::initAdc($node);
			CDS::Adcx1::initAdc($node);
			$::partType[$::partCnt] = CDS::Adcx1::partType($node);
		} 
		if ($part_name eq "Adcx2") {
			require "lib/Adc.pm";
			#CDS::Adcx1::initAdc($node);
			CDS::Adcx2::initAdc($node);
			$::partType[$::partCnt] = CDS::Adcx2::partType($node);
		} 
		if ($part_name eq "Adcx3") {
			require "lib/Adc.pm";
			#CDS::Adcx1::initAdc($node);
			CDS::Adcx3::initAdc($node);
			$::partType[$::partCnt] = CDS::Adcx3::partType($node);
		} 
		if ($part_name eq "Adcx4") {
			require "lib/Adc.pm";
			#CDS::Adcx1::initAdc($node);
			CDS::Adcx4::initAdc($node);
			$::partType[$::partCnt] = CDS::Adcx4::partType($node);
		} 
		if ($part_name eq "Adcx5") {
			require "lib/Adc.pm";
			#CDS::Adcx1::initAdc($node);
			CDS::Adcx5::initAdc($node);
			$::partType[$::partCnt] = CDS::Adcx5::partType($node);
		} 
		if ($part_name eq "Adcx6") {
			require "lib/Adc.pm";
			#CDS::Adcx1::initAdc($node);
			CDS::Adcx6::initAdc($node);
			$::partType[$::partCnt] = CDS::Adcx6::partType($node);
		} 
		if ($part_name eq "Adcx7") {
			require "lib/Adc.pm";
			#CDS::Adcx1::initAdc($node);
			CDS::Adcx7::initAdc($node);
			$::partType[$::partCnt] = CDS::Adcx7::partType($node);
		} 
		if ($part_name eq "Adcx8") {
			require "lib/Adc.pm";
			#CDS::Adcx1::initAdc($node);
			CDS::Adcx8::initAdc($node);
			$::partType[$::partCnt] = CDS::Adcx8::partType($node);
		} 
# End of ADC PART CHANGE **********************************

        	 $::partType[$::partCnt] = ("CDS::" . $part_name . "::partType") -> ($node, $::partCnt);
	 }
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
	if ($block_type eq "SATURATE") {
		$::partInputs[$::partCnt] = ${$node->{FIELDS}}{UpperLimit};
		if ($::partInputs[$::partCnt] eq undef) {
			$::partInputs[$::partCnt] = 0.5;
		}
		$::partInputs1[$::partCnt] = ${$node->{FIELDS}}{LowerLimit};
		if ($::partInputs1[$::partCnt] eq undef) {
			$::partInputs1[$::partCnt] = -0.5;
		}
	} elsif ($block_type eq "CONSTANT") {
		$::partInputs[$::partCnt] = ${$node->{FIELDS}}{Value};
		if ($::partInputs[$::partCnt] eq undef) {
			$::partInputs[$::partCnt] = 1;
		}
	} elsif ($block_type eq "SUM") {
		$::partInputs[$::partCnt] = ${$node->{FIELDS}}{Inputs};
		$::partInputs[$::partCnt] =~ tr/+-//cd; # delete other characters
	} elsif ($block_type eq "MULTIPLY" &&
		 ${$node->{FIELDS}}{Inputs} eq "*\/") {
		$::partType[$::partCnt] = "DIVIDE";
		$::partInputs[$::partCnt] = ${$node->{FIELDS}}{Inputs};
	} elsif ($block_type eq "Gain") {
		  $::partInputs[$::partCnt] = ${$node->{FIELDS}}{Gain};
		  if ($::partInputs[$::partCnt] eq undef) {
			$::partInputs[$::partCnt] = 1;
		  }
	} elsif ($block_type eq "RelationalOperator") {
		if (${$node->{FIELDS}}{Operator} eq undef) {
		  $::partInputs[$::partCnt] = ">=";
		} else {
		  $::partInputs[$::partCnt] = ${$node->{FIELDS}}{Operator};
		}
	} elsif ($block_type eq "Switch") {
		my $op = ${$node->{FIELDS}}{Criteria};
		if ($op eq undef) { $op = ">="; }
		$thresh = ${$node->{FIELDS}}{Threshold};
		if ($thresh eq undef) { $thresh = "0"; }
	  	if ($op  =~ />=/) { $op = ">= $thresh"; }
	  	elsif ($op  =~ />/) { $op = "> $thresh"; }
	  	elsif ($op  =~ /~=/) { $op = "!= 0"; }
		else { die "Invalid \"Choice\" block \"$block_name\" criteria"; }
		$::partInputs[$::partCnt] = $op;
	}
	${$node->{FIELDS}}{PartNumber} = $::partCnt; # Store our part number
	$::partCnt++;
   }
   return 0;
}

# Remove tags, replace'm with lines
sub remove_tags {
   my ($node, $in_sub, $parent) =  @_;
   return 0 if ($node->{NAME} ne "Block");
   my $block_type = transform_block_type(${$node->{FIELDS}}{"BlockType"});
   my $block_name = ${$node->{FIELDS}}{"Name"};
   return 0 if ($block_type ne "FROM");
   #print_node($node);
   my $tag = ${$node->{FIELDS}}{"GotoTag"};
   my $goto = CDS::Tree::find_node($parent, $tag, "GotoTag", "Goto", "BlockType");
   #print_node($goto);
   my $goto_name = ${$goto->{FIELDS}}{"Name"};
   #print "GotoName is $goto_name\n";
   # Find the line leading to the Goto
   my $goto_line = find_branch($parent, $goto_name, 1, 1);
   #print_node($goto_line);
   my $src_name = ${$goto_line->{FIELDS}}{"SrcBlock"};
   my $src_port = ${$goto_line->{FIELDS}}{"SrcPort"};
   #print "Source $src_name\n";
   # Find line originating at the "From"
   my $from_line = find_line($parent, $block_name, 1);
   #print_node($from_line);
   ${$from_line->{FIELDS}}{"SrcBlock"} = $src_name;
   ${$from_line->{FIELDS}}{"SrcPort"} = $src_port;
   #print_node($from_line);
   # Rename the tags so they are not picked up by the code upstream...
   ${$node->{FIELDS}}{"Name"} .= "_Removed";
   ${$goto->{FIELDS}}{"Name"} .= "_Removed";
   return 0;
}

# Remove buses, replace'm with lines
sub remove_busses {
   my ($node, $in_sub, $parent) =  @_;
   #print("node=$node, parent=$parent\n");
   if ($node->{NAME} eq "Block") {
	my $block_type = transform_block_type(${$node->{FIELDS}}{"BlockType"});
	my $block_name = ${$node->{FIELDS}}{"Name"};

	#print "Part $block_name $block_type $in_sub \n";
	if ($block_type eq "BUSS") {
		#print("BUSS found; name=$block_name; parent=$parent\n");
		#print_node($node);
		## Determine output port numbers (remove brackets, split on non-digits)
		#my $ports = ${$node->{FIELDS}}{"Ports"};
		#$ports =~ tr/[]//d;
		#my @out_ports = split(/\D*/,$ports);
		# Output signal names, comma separated
   		my @out_signals = split(/,/,${$node->{FIELDS}}{"OutputSignals"});
		# find the outgoing linesPorts
		#for my $index (0 .. $#out_ports) {
		#print "port=$out_ports[$index]; signal=$out_signals[$index]\n";
		#my $outline = find_line($parent, $block_name, $out_ports[$index]);
		#print_node($outline);
		#}
		# Find the line leading to this bus selector
		my $bus_line = find_branch($parent, $block_name, 1, 1);
		#print_node($bus_line);
		if ($bus_line == undef || $bus_line->{NAME} ne "Line") {
			die "Failed to find line leading to $block_name\n";
		}
		# See if the source block is a Bus Creator
		my $src_blk_name = ${$bus_line->{FIELDS}}{"SrcBlock"};
		#print "Source block name:" . $src_blk_name . "\n";
		my $bus_creator = CDS::Tree::find_node($parent, $src_blk_name, "Name");
		#print_node($bus_creator);
		if (${$bus_creator->{FIELDS}}{"BlockType"} ne "BusCreator") {
			# Do not continue processing this selector (for now)
			# ADCs are represented as subsystems with bus
			# selector coming out of it
			return 0;
		}
		my $bus_creator_inputs = ${$bus_creator->{FIELDS}}{"Inputs"};
		$bus_creator_inputs =~ tr/'//d;
		my @inputs = split(",", $bus_creator_inputs);
		#print @inputs;
		# Go through the list of signal names in bus selector
		for my $index (0 .. $#out_signals) {
			#print "signal=$out_signals[$index]\n";
			# Find the name in bus creator
			my $found = 0;
			for my $index1 (0 .. $#inputs) {
				#print "\tsignal=$inputs[$index1]\n";
				if ($inputs[$index1] eq $out_signals[$index]) {
				  # Found the signal; now find the output 
				  # line in the bus selector
		                  my $outline = find_line($parent, $block_name, 1+$index);
				  die if $outline == undef;
				  #print_node($outline);

				  # Find the line leading to the bus creator port
				  my $port = 1+$index1;
				  #print "find_branch on $src_blk_name; port=$port\n";
				  $in_line = find_branch($parent, $src_blk_name, 1+$index1, 1);
				  die if $in_line == undef;
				  #print_node($in_line);
				  die if ($in_line->{NAME} ne "Line");

				  # Find the block the line originates at
				  my $sig_src = CDS::Tree::find_node($parent, ${$in_line->{FIELDS}}{"SrcBlock"}, "Name");
				  die if $sig_src == undef;
				  #print_node($sig_src);

				  # Now connect the bus selector output line to originate
				  # at the found block
				  ${$outline->{FIELDS}}{"SrcBlock"} = ${$sig_src->{FIELDS}}{"Name"};
				  #print_node($outline);
				  $found = 1;
				  last;
				}
			}
			die "Couldn't find a signal $out_signals[$index] in Bus Creator" if !$found;
		}
	} elsif ($block_type eq "BUSC") {
		#print("BUSC found; name=$block_name\n");
		#print_node($node);
	}
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
   #if (${$_->{FIELDS}}{Parent} == 1) { return; } # Stop annotating if discovered parent's block
   if ("${$_->{FIELDS}}{nname}_" eq $ant) { return; }

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
# if $flag is 1, then it returns a reference to Line for a branch.
# Line is a parent node for a branch.
sub find_branch {
   my ($node, $dst_name, $dst_port, $flag) = @_;
   #print "find_branch: ". $_->{NAME} . "+++". ${$node->{FIELDS}}{Name} . ", $dst_name, $dst_port, $flag\n";
   foreach (@{$node->{NEXT}}) {
	if ($_->{NAME} eq "Line" || $_->{NAME} eq "Branch") {
	  my $dprt = ${$_->{FIELDS}}{DstPort};
	  if ($dprt == undef) { $dprt = 1; }
	  #print "find_branch: ", ${$_->{FIELDS}}{DstBlock}, ":", $dprt, "\n";
	  if (${$_->{FIELDS}}{DstBlock} eq $dst_name && $dprt == $dst_port) {
	  	#print "found it; " . $_->{NAME} . "\n";
		#print_node($_);
		if ($flag && $_->{NAME} eq "Branch") {
			#print "Returning:\n";
			#print_node($node);
			# Do not return if the $node is not Line, keep searching
			if ($node->{NAME} ne "Line" && $node->{NAME} ne "Branch") {
				next;
			}
			return $node;
		}
		else { return $_; }
	  }
	  my $block = find_branch($_, $dst_name, $dst_port, $flag);
	  if ($block ne undef) {
		  # See if this is the line and return IT instead if $flag
		  if ($flag && $node->{NAME} eq "Line") {
		  	#print "return line " . ${node->{FIELDS}}{DstBlock} . "\n";
		  	return $node;
		  } else { return $block; }
	  }
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
	    # Annotate tags
	    if (${$_->{FIELDS}}{GotoTag}) {
	    	${$_->{FIELDS}}{GotoTag} = ${$node->{FIELDS}}{Name} . "_" . ${$_->{FIELDS}}{GotoTag};
	    }
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
	#print_node($branch);
	die "OutPort $port_name disconnected\n" if ($branch eq undef);
	# Find parent's line connected to this node, output port $port_num
	my $line = find_line($parent, ${$node->{FIELDS}}{Name}, $port_num);
	#print_node($line);
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
	  #printf "Changing Line into branch\n";
	  #print_node($line);
	  CDS::Tree::print_tree($line);
	  ${$line->{FIELDS}}{SrcBlock} = undef;
	  ${$line->{FIELDS}}{SrcPort} = undef;
	  $line->{NAME} = "Branch";
	  # Remove former destination
	  ${$branch->{FIELDS}}{DstBlock} = undef;
	  ${$branch->{FIELDS}}{DstPort} = undef;
	  # Mark it here to stop annotating names in flatten_do_branches() later
	  #${$branch->{FIELDS}}{Parent} = 1;
     	  ${$branch->{FIELDS}}{nname} = ${$node->{FIELDS}}{Name};
	  #print_node($line);
	  # Insert parent's line into the branch
	  push @{$branch->{NEXT}}, $line;
	  #print_node($branch);
	  CDS::Tree::print_tree($branch);
  	  #CDS::Tree::do_on_nodes($branch, \&print_node);
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
;  print "Block ", $_->{NAME}, ", type=", ${$_->{FIELDS}}{BlockType}, ", name=",${$_->{FIELDS}}{Name}, "\n";
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
	#print "Top-level subsystem ", ${$_->{FIELDS}}{Name}, "\n";
	if (${$_->{FIELDS}}{Tag} eq "top_names") {
		push @::top_names, ${$_->{FIELDS}}{Name};
	}
	# Flatten all second-level subsystems
	my $system = $_->{NEXT}[0];
	foreach $ssub (@{$system->{NEXT}}) {
          if ($ssub->{NAME} eq "Block" && ${$ssub->{FIELDS}}{BlockType} eq "SubSystem") {
	    #print "Second-level subsystem ", ${$ssub->{FIELDS}}{Name}, "\n";
	    @subsys = ($_);
            flatten($ssub);
	  }
	}
     }
   }
   #print ::EPICS "test_points ONE_PPS $::extraTestPoints\n";
   if (@::top_names) {
   	print ::EPICS "top_names FEC";
   	foreach $item (@::top_names) {
		print ::EPICS " ", $item ;
	}
	print ::EPICS "\n";
   } else {
   	print ::EPICS "top_names FEC\n";
   }
}

   return 0;
}


sub process {

  print "Starting node processing\n";

  # Find first System node, this is the top level subsystem
  my $system_node = CDS::Tree::find_node($root, "System");

  # Find block parameter defaults
  #$block_parameter_defaults_node = CDS::Tree::find_node($root, "BlockParameterDefaults");

  # Set system name
  $::systemName = ${$system_node->{FIELDS}}{"Name"};

  # There is really nothing needed below System node in the tree so set new root
  $root = $system_node;

  #print "TREE\n";
  #CDS::Tree::print_tree($root);

#  CDS::Tree::do_on_nodes($root, \&remove_busses, 0, $root);
#  print "Removed Busses\n";
  do {
  	$n_merged = 0;
  	CDS::Tree::do_on_nodes($root, \&merge_references, 0, $root);
	print "Merged $n_merged references\n";
  } while ($n_merged != 0);
  #CDS::Tree::print_tree($root);
  print "Merged library referenes\n";

  print "Flattening the model\n";
  flatten_nested_subsystems($root);
  print "Finished flattening the model\n";
  CDS::Tree::do_on_nodes($root, \&remove_tags, 0, $root);
  print "Removed Tags\n";

  CDS::Tree::do_on_nodes($root, \&node_processing, 0);
  print "Found $::adcCnt ADCs $::partCnt parts $::subSys subsystems\n";



  # See to it that ADC is on the top level
  # This is needed because the main script can't handle ADCs in the subsystems
  # :TODO: fix main script to handle ADC parts in subsystems
  foreach (0 ... $::partCnt) {
    #if ($::partType[$_] eq "BUSS" || $::partType[$_] eq "BUSC" || $::partType[$_] eq "Dac") {
    if ($::partType[$_] eq "Dac" || $::partType[$_] eq "Dac18") {                        # ===  MA-2011  +++
      if ($::partSubName[$_] ne "") {
	die "All ADCs and DACs must be on the top level in the model";
      }
    }
  }

  return 1;
}

return 1;
