
package CDS::Parser;
use Exporter;
@ISA = ('Exporter');


require "lib/Tree.pm";


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

  exit (0);
  return 1;
}

sub count_adcs {
   ($node) =  @_;
   if ($node->{NAME} eq "Block" && ${$node->{FIELDS}}{"BlockType"} eq "BusCreator") {
	$::adcCnt++;
   }
}

sub process {

  # Find first System node, this is the top level subsystem
  my $system_node = CDS::Tree::find_node($root, "System");

  # Set system name
  $::systemName = ${$system_node->{FIELDS}}{"Name"};

  # There is really nothing needed below System node in the tree so set new root
  $root = $system_node;

  CDS::Tree::do_on_nodes($root, \&count_adcs);
  print "Found $::adcCnt ADCs\n";

  return 1;
}

