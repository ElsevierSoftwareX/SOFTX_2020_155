
package CDS::Parser;
use Exporter;
use Cwd;

@ISA = ('Exporter');

#//     \page Parser3 Parser3.pm
#//     Parser3.pm - Parses MATLAB file and flattens model into single subsystem.
#//
#// \n


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

#// \n \n \b sub \b print_node \n
#// Print block information, including all the fields \n\n
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

#// \b sub \b sortDacs \n
#// We need to sort DACs by placing 18bit frst and sorting on card number \n
# For example:
# 18bit card 0
# 18bit card 1
# 16bit card 0
# 16bit card 1
#
#// The following arrays will need to be shuffled (sorted) \n
#//  $::dacPartNum \n
#//  $::dacType \n
#//  $::dacNum n\
#// The following array will need to be reassigned with valid numbers based on the new order
#// from the information in $::daqcPartNum array: \n
#//  $::card2array \n\n
#
sub sortDacs {
        my @dacs;
        # Calculate relative positions of each DAC
        for ($_ = 0; $_ < $::dacCnt; $_++) {
                my $w = 0;
                if ($::dacType[$_] eq "GSC_18AO8") {
                        $w = 1;
                } elsif ($::dacType[$_] eq "GSC_20AO8") {
			print "Found 20 bit DAC \n";
                        $w = 100;
                } elsif ($::dacType[$_] eq "GSC_16AO16") {
                        $w = 200;
                } else {
                        die "Unsupported DAC board type " . $::dacType[$_] ;
                }
                $dacs[$_] =  $w  + $::dacNum[$_];
        }

        # Sort the dac weights
        my @sorted_dacs = sort {$a <=> $b} @dacs;

	#for (0 .. $::dacCnt-1) {
		#print "Dac weight $_ is " . $dacs[$_] . "\n";
		#print "Dac weight sorted $_ is " . $sorted_dacs[$_] . "\n";
	#}

        # Save old array values
        my @dacPartNumSave = @::dacPartNum;
        my @dacTypeSave = @::dacType;
        my @dacNumSave = @::dacNum;

	#print "dacCnt=" . $::dacCnt . "\n";
        # Find out shuffling indices and shuffle the data
        for ($_ = 0; $_< $::dacCnt; $_++) {
                my $w = $dacs[$_];
                my $i = 0;
		foo: {
                for ($i = 0; $i < $::dacCnt; $i++) {
                        if ($w == $sorted_dacs[$i]) {
                                last foo;
                        }
                }}

		print "DAC: moving " . $_ . " to " . $i . "\n";
                $::dacPartNum[$i] = $dacPartNumSave[$_];
                $::dacType[$i] = $dacTypeSave[$_];
                $::dacNum[$i] = $dacNumSave[$_];
                $::card2array[$::dacPartNum[$i]] = $i;
		print "\t" .$::dacPartNum[$i] . "\t" . $::dacType[$i] . "\t" . $::card2array[$::dacPartNum[$i]] . "\n";
        }
	return 1;
}

#// \b sub \b parse \n
#// Primary model parser routine \n\n
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

#// \b sub \b transform_part_name \n
#// Change Reference source name (CDS part) \n\n
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
        elsif ($r eq "cdsFiltBiQuad" ) { $r = "Filt"; $::biQuad[$::partCnt] = 1; print "Found biquad filter\n"; }
        elsif ($r eq "cdsFirFilt" ) { $r = "Filt"; $::useFIRs = 1; }
        elsif ($r =~ /^cds/) {
                # Getting rid of the leading "cds"
                ($r) = $r =~ m/^cds(.+)$/;
        } elsif ($r =~ /^SIMULINK/i) { next; }

        # Capitalize first character
        $r = uc(substr($r,0, 1)) . substr($r, 1);

	return $r;
}

#// \b sub \b transform_block_type \n
#// Change Block type name \n\n
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

#// \b sub \b process_line \n
#// Store line information \n\n
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
	  elsif ($::partType[$part_num] eq "Dac18") { $::partInCnt[$part_num] = 8; }
	  elsif ($::partType[$part_num] eq "Dac20") { $::partInCnt[$part_num] = 8; }
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

#// \b sub \b do_branches \n
#// There could be nested branching structure (1 to many connection) \n\n
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

# C language reserved words
my @c_resv = ("auto", "break", "case", "char", "const", "continue", "default", "do", "double",
"else", "enum", "extern", "float", "for", "goto", "if", "int", "long", "register", "return",
"short", "signed", "sizeof", "static", "struct", "switch", "typedef", "union", "unsigned",
"void", "volatile", "while");

#// \b sub \b name_check \n
#// Check name to contain only allowed characters \n
#// Disallow C language reserved words \n\n
sub name_check {
	return 0 unless @_[0] =~ /^[a-zA-Z0-9_]+$/;
	return 0 if grep {$_ eq lc(@_[0])} @c_resv;
	return 1;
}

#// \b sub \b store_ezca_names \n
sub store_ezca_names {
   my ($node, $in_sub, $parent) =  @_;
   my $src_block = ${$node->{FIELDS}}{"SourceBlock"};
   my $name =  ${$node->{FIELDS}}{"Name"};
   if ($src_block =~ /^cdsParameters/) {
	# Parse cdsParameters now
       	require "lib/Parameters.pm";
   	CDS::Parameters::parseParams($name);
   }
   if ($node->{NAME} ne "Block") {
	   return 0;
   }
   if ($src_block !~ /^cdsEzCa/ || $src_block eq undef) {
	  return 0;
   }
   #print "name=", $name, "src=", $src_block, "\n";
   ${$node->{FIELDS}}{"Description"} = $name;
   return 0;
}
#// \b sub \b merge_references \n
#// Bring library references into the tree \n\n
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
   } else {
	my $adcStr = substr($part_name, 0, 3);
	return 0 if ($adcStr eq "Adc");
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

   my $model_file_found = 0;
   $fname .= ".mdl";
   foreach $i (@::rcg_lib_path) {
	   my $f = $i ."/".$fname;
           if (-r $f) {
                   print "Library model file found $f", "\n";
                   print "RCG_LIB_PATH=". join(":", @::rcg_lib_path)."\n";
                   $model_file_found = 1;
		   $fname = $f;
		   push @::sources, $fname;
   		   last;
	   }
   }

   #$fname = "../simLink/$fname.mdl";
   die "Can't find $fname; RCG_LIB_PATH=". join(":", @::rcg_lib_path). "\n" unless $model_file_found;
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
   if ($mynode->{NAME} eq "Library") {
	# file name is same as subsystem name, so need to skip Library and System
        ${$mynode->{FIELDS}}{"Name"} .= "~";
   	$mynode = CDS::Tree::find_node($myroot, $system_name, "Name");
	#print "NAME=$mynode->{NAME}\n";
        if ($mynode->{NAME} eq "System") {
           ${$mynode->{FIELDS}}{"Name"} .= "~";
   	   $mynode = CDS::Tree::find_node($myroot, $system_name, "Name");
	   #print "NAME=$mynode->{NAME}\n";
        }
   }
   die "Couldn't find $system_name in $fname\n" unless defined $mynode;
   #CDS::Tree::print_tree($mynode);
   #${$node->{FIELDS}}{"__merge_references__"} = $mynode;
   # Replace subsystem name
   ${$mynode->{FIELDS}}{"Name"} = $name;
   my $next = $mynode->{NEXT}[0];
   foreach (@{$mynode->{NEXT}}) {
      if ($_->{NAME} eq "System") {
            $next = $_;
            last;
      }
   }
   die "Failed to parse the MDL file merging references at System", $name, "\n" if $next->{NAME} ne "System";
   ${$next->{FIELDS}}{"Name"} = $name;
   $_ = $mynode; # replace the current node (do_on_nodes)
   $n_merged++;
   return 0;
}

#// \b sub \b node_processing \n
#// Find parts and sybsystems \n\n
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

	# Save logical operator for Logic blocks
	if ($block_type eq "AND") {
                $::blockDescr[$::partCnt] = ${$node->{FIELDS}}{"Operator"};
		$::partInputs[$::partCnt] = 2;
		if(${$node->{FIELDS}}{Inputs} ne undef)
		{
			$::partInputs[$::partCnt] = ${$node->{FIELDS}}{Inputs};
		}
		if(${$node->{FIELDS}}{"Operator"} eq "NOT") {
			$::partInputs[$::partCnt] = 1;
		}
	}
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
	   $::partInputs[$::partCnt] = $math_op;
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
                die "Cannot handle nested subsystems in $block_name\n" if $in_sub;
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
		#print "CDS part $block_name type $source_block\n";
        	$::cdsPart[$::partCnt] = 1;
		$::xpartName[$::partCnt] = $::partName[$::partCnt] = $block_name;
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
		    die "Invalid part name \"$::partName[$::partCnt]\"; source_block \"$source_block\"; block  type \"$block_type\"" unless ("$::partName[$::partCnt]");
	    }
	}
	if (!$in_sub) {
		$::nonSubPart[$::nonSubCnt] = $::partCnt;
		$::nonSubCnt++;
	} else {
		$::partSubNum[$::partCnt] = $::subSys;
		$::partSubName[$::partCnt] = $::subSysName[$::subSys];
	     	if($source_block !~ /^cdsEzCa/) {
		$::xpartName[$::partCnt] = $::subSysName[$::subSys] . "_" . $::xpartName[$::partCnt];
		} else {
		$::xpartName[$::partCnt] = $::subSysName[$::subSys] . "_" . $::xpartName[$::partCnt];
		print "Found ezca part $::xpartName[$::partCnt] $::partName[$::partCnt]\n";
		}
	}
	if ($::cdsPart[$::partCnt]) {
		my $part_name = transform_part_name(${$node->{FIELDS}}{"SourceBlock"});
                if ($block_type eq "FCN") {                                # ===  MA  ===
                   $part_name = "Fcn";                                     # ===  MA  ===
                }                                                          # ===  MA  ===
		if (! -e "lib/$part_name.pm") {
			my $adcStr = substr($part_name, 0, 3);
			if ($adcStr eq "Adc") {
				require "lib/Adc.pm";
				CDS::Adc::initAdc($node);
				$::partType[$::partCnt] = CDS::Adc::partType($node);
			} else {
				die "Can't find part code in lib/$part_name.pm\n";
			}
		} else {

       		require "lib/$part_name.pm";
		if ($part_name eq "Dac") {
        	  $::partType[$::partCnt] = CDS::Dac::initDac($node);
		}
		if ($part_name eq "Dac18") {                              
        	  $::partType[$::partCnt] = CDS::Dac18::initDac($node); 
		}                                                     
		if ($part_name eq "Dac20") {                       
        	  $::partType[$::partCnt] = CDS::Dac20::initDac($node);   
		}                                                       
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
		if ($part_name eq "CDO64") {
        	  $::partType[$::partCnt] = CDS::CDO64::initCDO64($node);
                  if ($::boCnt > $::maxDioMod) {
                     die "Too many Digital I/O modules \(max is $::maxDioMod\)\n";
                  }
		}
		if ($part_name eq "CDI64") {
        	  $::partType[$::partCnt] = CDS::CDI64::initCDI64($node);
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

        	 $::partType[$::partCnt] = ("CDS::" . $part_name . "::partType") -> ($node, $::partCnt);

	 #print "PNAME = $part_name \n";
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
        	$::cdsPart[$::partCnt] = 1;
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
	} elsif ($block_type eq "AND") {
        	$::cdsPart[$::partCnt] = 1;
	} elsif ($block_type eq "MUX") {
        	$::cdsPart[$::partCnt] = 1;
		if(${$node->{FIELDS}}{Inputs} eq undef)
		{
			$::partInputs[$::partCnt] = 4;
		} else {
			$::partInputs[$::partCnt] = ${$node->{FIELDS}}{Inputs};
		}
	} elsif ($block_type eq "DEMUX") {
        	$::cdsPart[$::partCnt] = 1;
	} elsif ($block_type eq "SUM") {
        	$::cdsPart[$::partCnt] = 1;
		$::partInputs[$::partCnt] = ${$node->{FIELDS}}{Inputs};
		$::partInputs[$::partCnt] =~ tr/+-//cd; # delete other characters
	} elsif ($block_type eq "MATH") {
        	$::cdsPart[$::partCnt] = 1;
	} elsif ($block_type eq "MULTIPLY" &&
		 index(${$node->{FIELDS}}{Inputs} , "*" ) != -1 &&
		 index(${$node->{FIELDS}}{Inputs} , "\/" ) != -1) {
		$::partType[$::partCnt] = "DIVIDE";
		$::partInputs[$::partCnt] = ${$node->{FIELDS}}{Inputs};
        	$::cdsPart[$::partCnt] = 1;
	} elsif ($block_type eq "MULTIPLY") {
		if(${$node->{FIELDS}}{Inputs} eq undef) {
			$::partInputs[$::partCnt] = 2;
		}
		if(${$node->{FIELDS}}{Inputs} =~ /^[0-9,.E]+$/ ) {
			$::partInputs[$::partCnt] = ${$node->{FIELDS}}{Inputs};
		}
		if (index(${$node->{FIELDS}}{Inputs}, "*") != -1) {
			my $tmpstring = ${$node->{FIELDS}}{Inputs};
			$tmpstring =~ tr/*//cd; # delete other characters
			$::partInputs[$::partCnt] = length($tmpstring);
		} 
        	$::cdsPart[$::partCnt] = 1;
	} elsif ($block_type eq "Abs") {
        	$::cdsPart[$::partCnt] = 1;
	} elsif ($block_type eq "Gain") {
        	$::cdsPart[$::partCnt] = 1;
		  $::partInputs[$::partCnt] = ${$node->{FIELDS}}{Gain};
		  if ($::partInputs[$::partCnt] eq undef) {
			$::partInputs[$::partCnt] = 1;
		  }
	} elsif ($block_type eq "RelationalOperator") {
        	$::cdsPart[$::partCnt] = 1;
		if (${$node->{FIELDS}}{Operator} eq undef) {
		  $::partInputs[$::partCnt] = ">=";
		} else {
		  $::partInputs[$::partCnt] = ${$node->{FIELDS}}{Operator};
		}
	} elsif ($block_type eq "Switch") {
        	$::cdsPart[$::partCnt] = 1;
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

#// \b sub \b check_tags \n
#// Gather tag names \n\n
sub check_tags {
   my ($node, $in_sub, $parent) =  @_;
   return 0 if ($node->{NAME} ne "Block");
   my $block_type = transform_block_type(${$node->{FIELDS}}{"BlockType"});
   my $block_name = ${$node->{FIELDS}}{"Name"};
   return 0 if ($block_type ne "FROM" && $block_type ne "GOTO");
   my $tag = ${$parent->{FIELDS}}{"Name"} . ${$node->{FIELDS}}{"GotoTag"};
   #print "TAG: ", ${$parent->{FIELDS}}{"Name"}, " ",  $tag , " counter=", $::goto_tags{$tag}, "\n";
   if ($block_type eq "FROM") {
   	$::from_tags{$tag}++;
   } else {
        $::goto_tags{$tag}++;
   }
   return 0;
}

#// \b sub \b remove_tags \n
#// Remove tags, replace'm with lines \n\n
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
   die "*** ERROR: Can't find a Goto for a From $tag\n*** ERROR: The Goto must be in the same subsystem\n" if !defined $goto_name;
   #print "GotoName is $goto_name\n";
   # Find the line leading to the Goto
   my $goto_line = find_branch($parent, $goto_name, 1, 1, $parent);
   #print "Found goto line names ", ${$goto_line->{FIELDS}}{"Name"}, "\n";
   #print_node($goto_line);
   my $src_name = ${$goto_line->{FIELDS}}{"SrcBlock"};
   my $src_port = ${$goto_line->{FIELDS}}{"SrcPort"};
   #print "Source $src_name\n";
   print "Couldn't find tag source on $goto_name\n" unless $src_name;
   #die if ($goto_name eq "YARM_TRIG_From");
   # Find line originating at the "From"
   my $from_line = find_line($parent, $block_name, 1);
   #print_node($from_line);
   ${$from_line->{FIELDS}}{"SrcBlock"} = $src_name;
   ${$from_line->{FIELDS}}{"SrcPort"} = $src_port;
   ${$from_line->{FIELDS}}{"Name"} = ${$goto_line->{FIELDS}}{"Name"};
   #print_node($from_line);
   # Rename the tags so they are not picked up by the code upstream...
   $node->{NAME} = "Removed";
   $goto->{NAME} = "Removed";
   #$goto_line->{NAME} = "Removed";
   return 0;
}

#// \b sub \b remove_busses \n
#// Remove buses, replace'm with lines \n\n
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
		if(($outsize = @out_signals) == 0) { 
			die "Failed to find output signals - No OutputSignals field in .mdl file for $block_name\n";
		}
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
			# :TODO: we may want to fix this case (bus coming out
			# of a top-level subsystem), but it may not be easy
			#print "Failed to find BusCreator for $block_name\n";
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
				  die "Failed to find line leading to bus selector $block_name"  if $outline == undef;
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
				  ${$outline->{FIELDS}}{"SrcPort"} = ${$in_line->{FIELDS}}{"SrcPort"};
				  #print_node($outline);
				  $found = 1;
				  last;
				}
			}
			if (!$found) {
				print "Couldn't find a signal $out_signals[$index] in the bus\n";
				print "Available signals: $bus_creator_inputs\n";
				$::time_to_die = 1;
			}
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

#// \b sub \b flatten_do_branches \n
#// recursive branch processing \n
#// annotate names \n\n
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

#// \b sub \b find_line \n
#// Find a line in the $node, starting from $src_name and $src_port \n\n
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
#// \b sub \b find_branch \n
#// Find a branch (or a line) in the $node, leading to $dst_name and $dst_port 
#// if $flag is 1, then it returns a reference to Line for a branch. \n
#// Line is a parent node for a branch. \n\n
sub find_branch {
   my ($node, $dst_name, $dst_port, $flag, $prnt) = @_;
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
			#print "Returning 1:\n";
			#print_node($node);
			#print_node($prnt);
			# Do not return if the $node is not Line, keep searching
			if ($node->{NAME} eq "Branch") {
				# This code ignores broken off branches
				# which somehow get introduced into the tree.
				# There are branches without Line (no source).
				if ($prnt->{NAME} eq "System") {
					next;
				} else {
					return $prnt;
				}
			}
			if ($node->{NAME} ne "Line") {
				next;
			}
			return $node;
		}
		else {
			#print "Returning 2:\n";
			#print_node($_);
		       	return $_; 
		}
	  }
	  my $block = find_branch($_, $dst_name, $dst_port, $flag, $node);
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

#// \b sub \b flatten \n
#// This recursive function flattens the bottom most system
#// first and then flattens the rest of the systems in ascending order. \n\n
sub flatten {
   my @inports;
   my @outports;
   my @lines;
   my ($node) =  @_;

   #print "flatten called: ", $node->{NAME}, " name: ", ${$node->{FIELDS}}{Name}, "\n";

   if ($node->{NAME} eq "Block"
       && ${$node->{FIELDS}}{BlockType} eq "SubSystem") {
     push @subsys, $node;
   }
   foreach (@{$node->{NEXT}}) {
  	#print ${$_->{FIELDS}}{Name}, ", ";
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
       foreach (@{$parent->{NEXT}}) {
	if ($_->{NAME} eq "System") {
       		$parent = $_;
		last;
	}
       }
     }
     die "Failed to parse the MDL file at System\n" if $parent->{NAME} ne "System";
     foreach (@{$parent->{NEXT}}) {
	if ($_ == $node) {
		#print "Found node at index $idx\n";
		last;
	}
	$idx++;
     }
     splice(@{$parent->{NEXT}}, $idx, 1,);

     # Annotate blocks in this node with its name
     # Move down to the "System" node
     foreach (@{$node->{NEXT}}) {
	if ($_->{NAME} eq "System") {
		$node = $_;
		last;
	}
     }
     die "Failed to parse the MDL file at System ", ${$node->{FIELDS}}{Name}, "\n" if $node->{NAME} ne "System";
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
	   # print "Source=", ${$_->{FIELDS}}{SrcBlock}, ":", ${$_->{FIELDS}}{SrcPort}, ", dst=",
		#${$_->{FIELDS}}{DstBlock}, ":", ${$_->{FIELDS}}{DstPort}, "\n";
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
	# print "Processing output port #$port_num name=$port_name\n";
	# Find line connected to this output port (if any)
	my $branch = find_branch($node, $port_name, 1);
	# print_node($branch);
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

#// \b sub \b flatten_nested_subsystems \n
sub flatten_nested_subsystems {
   my ($node) =  @_;

# This code flattens all subsystems \n
# It is not working properly \n\n
if (0) {
   foreach (@{$node->{NEXT}}) {
     if ($_->{NAME} eq "Block" && ${$_->{FIELDS}}{BlockType} eq "SubSystem") {
	print "Top-level subsystem ", ${$_->{FIELDS}}{Name}, "\n";
	@subsys = ($node);
	flatten($_);
     }
   }
}

#// This code flattens only second-level subsystems \n\n
if (1) {
   # Find all top-level subsystems
   foreach (@{$node->{NEXT}}) {
     if ($_->{NAME} eq "Block" && ${$_->{FIELDS}}{BlockType} eq "SubSystem") {
	print "Top-level subsystem ", ${$_->{FIELDS}}{Name}, "\n";
	if (${$_->{FIELDS}}{Tag} eq "top_names") {
		push @::top_names, ${$_->{FIELDS}}{Name};
	}
	# Flatten all second-level subsystems
	my $system = $_->{NEXT}[0];
        foreach (@{$_->{NEXT}}) {
           if ($_->{NAME} eq "System") {
                $system = $_;
                last;
           }
        }
        die "Failed to parse the MDL file flattening System", ${$_->{FIELDS}}{Name}, "\n" if $system->{NAME} ne "System";
	foreach $ssub (@{$system->{NEXT}}) {
	  #print "0; Second-level subsystem ", ${$ssub->{FIELDS}}{Name}, "\n";
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


#// \b sub \b process \n
#// Start node processing \n\n
sub process {

  print "Starting node processing\n";

  # Find first System node, this is the top level subsystem
  my $system_node = CDS::Tree::find_node($root, "System");

  # Find block parameter defaults
  $block_parameter_defaults_node = CDS::Tree::find_node($root, "BlockParameterDefaults");
  $node = CDS::Tree::find_node($block_parameter_defaults_node, "Fcn", "BlockType");
  $expr = ${$node->{FIELDS}}{"Expr"};
  #print "expr=$expr\n"; 
  $::defFcnExpr = $expr;

  # Set system name
  $::systemName = ${$system_node->{FIELDS}}{"Name"};

  # There is really nothing needed below System node in the tree so set new root
  $root = $system_node;

  #print "TREE\n";
  #CDS::Tree::print_tree($root);

  #CDS::Tree::do_on_nodes($root, \&remove_tags, 0, $root);
  #CDS::Tree::do_on_nodes($root, \&remove_busses, 0, $root);
  #print "Removed Busses\n";
  do {
  	$n_merged = 0;
  	CDS::Tree::do_on_nodes($root, \&merge_references, 0, $root);
	print "Merged $n_merged references\n";
  } while ($n_merged != 0);
  print "Merged library referenes\n";

  #CDS::Tree::print_tree($root);
  #die;
  # Find all annotations starting with keyword "#DAQ Channels"
  # They represent the list of DAQ channels the user want
  #
  my $annot, $prefix;
  do {
    ($annot, $prefix) = CDS::Tree::find_daq_annotation($root);
    if ($annot ne undef) {
      my @sp = split(/\\n/, ${$annot->{FIELDS}}{Name});
      # See if the channel name and rate specified
      foreach $i (@sp) {
	      my $rate = "";
	      my $type = "";
	      my $science = "";
	      my $egu = "";

	      next if ($i =~ "^#");
	      my @nr = split(/\s+/, $i);
	      next unless length $nr[0];
	      my $pn = $prefix . $nr[0];
	      # See if this is a science mode channel (ends with an asterisk)
	      if ($pn =~ /\*$/) {
		chop $pn;
		$science ="science";
	      }
	      die "Bad DAQ channel name specified: $pn\n" unless name_check($pn);
	      # There are up to three extra fields allowed
	      # rate, if it is a number
	      # "uint32" specified data type
	      # or "science" to indicate a science run mode channel
	      shift @nr;
	      foreach $f (@nr) {
		if ($f eq "science") {
		  $science = $f;
		} elsif ($f eq "uint32") {
		  $type = "uint32";
		} elsif ($f eq "double") {
		  $type = "double";
		} elsif ($f eq "int32") {
		  $type = "int32";
	        } elsif ($f =~ /^\d+$/) { # An integer
		  my @rates = qw(32 64 128 256 512 1024 2048 4096 8192 16384 32768 65536);
		  my @res = grep {$f == $_} @rates;
		  #print $f, " ", @res, "\n";
		  die "Bad DAQ channel rate specified: $pn, $f\n" unless @res;
		  #print $pn," ", $f, "\n";
		  $rate = $f;
	        } else {
		  $egu = $f;
		}
	      }
      	      # Add channel name and rate into the hash and print into the _daq file
	      # fmseq.pl then will open and process the DAQ channel data
	      die "Duplicated DAQ channel name $pn\n" if defined $::DAQ_Channels{$pn};
	      $::DAQ_Channels{$pn} = $rate;
	      print ::DAQ $pn, " $rate $type $science $egu\n";
      }
      ${$annot->{FIELDS}}{Name} = "Removed";
    }
  } while($annot ne undef);
  close ::DAQ;

  # Store ezca block names in description field, so that they are accessible later
  # without the subsystems prefix
  CDS::Tree::do_on_nodes($root, \&store_ezca_names, 0, $root);

  print "Flattening the model\n";
  flatten_nested_subsystems($root);
  print "Finished flattening the model\n";
  $::goto_tags = undef;
  CDS::Tree::do_on_nodes($root, \&check_tags, 0, $root);
  foreach (keys %::goto_tags) {
         # See if there is one or more corresponding FROM tags
         die "*** ERROR: Goto tag ", $_, " has no coresponding From\n" if ($::from_tags{$_} < 1);
         # See if there multiple Goto tags with the same name
         die "*** ERROR: Goto tag ", $_, " specified " , $::goto_tags{$_}, " times\n" if ($::goto_tags{$_} > 1);
  }
  CDS::Tree::do_on_nodes($root, \&remove_tags, 0, $root);
  print "Removed Tags\n";
  $::time_to_die = 0;
  CDS::Tree::do_on_nodes($root, \&remove_busses, 0, $root);
  die if $::time_to_die;
  print "Removed Busses\n";

  #CDS::Tree::print_tree($root);
  CDS::Tree::do_on_nodes($root, \&node_processing, 0);
  print "Found $::adcCnt ADCs $::partCnt parts $::subSys subsystems\n";

  # Check the ADC and DAC numbers are unique

  my @adc_names;
  my @adc_card_nums;
  my @dac_card_nums;

  foreach (0 ... $::partCnt) {
  	#if ($::partType[$_] eq "Dac" || $::partType[$_] eq "Dac18" || $::partType[$_] eq "Dac20") {
  	if ($::partType[$_] eq "Dac" || $::partType[$_] eq "Dac18") {
		my $card_num = $::dacNum[$::card2array[$_]];
		print "Dac ", $::xpartName[$_], " ", $card_num, " ", $::partType[$_], "\n";
		push @dac_card_nums, $card_num;

	} elsif ($::partType[$_] eq "Adc") {
		my $an = substr($::xpartName[$_],3,2);
		my $card_num = $::adcNum[$an];
		#print "Adc ", $::xpartName[$_], " ", $card_num, "\n";
		push @adc_card_nums, $card_num;
		push @adc_names, $an;
	}
  }
  print "DAC card numbs = @dac_card_nums \n";
  # Check that card numbers are unique
  my %hash   = map { $_, 1 } @dac_card_nums;
  my @unique = keys %hash;
  die "DAC card numbers must be unique\n" unless $#dac_card_nums == $#unique;

  my %hash   = map { $_, 1 } @adc_card_nums;
  my @unique = keys %hash;
  # die "ADC card numbers must be unique\n" unless $#adc_card_nums == $#unique;

  # Go through the array and see if the numbers are all consequtive for ADC names
  @adc_names = sort { $a <=> $b } @adc_names;
  foreach (0 .. $#adc_names) {
	#printf "ADC #%s\n", $adc_names[$_];
	if ($_ && $adc_names[$_-1] + 1 != $adc_names[$_]) {
		die "ADC names must be unique and consecutive\n";
	}
  }

  # See to it that ADC is on the top level
  # This is needed because the main script can't handle ADCs in the subsystems
  # :TODO: fix main script to handle ADC parts in subsystems
  foreach (0 ... $::partCnt) {
    #if ($::partType[$_] eq "BUSS" || $::partType[$_] eq "BUSC" || $::partType[$_] eq "Dac") {
    if ($::partType[$_] eq "Dac" || $::partType[$_] eq "Dac18" || $::partType[$_] eq "Dac20") {  
      if ($::partSubName[$_] ne "") {
	die "All ADCs and DACs must be on the top level in the model";
      }
    }
  }

  # Check we have at least two filter modules
  die "Need to have at least two filter modules in the model" unless $::filtCnt > 1;

  return 1;
}

return 1;
