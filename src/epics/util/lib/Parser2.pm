
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
   if ($_->{PRINTED} != 1) {
	print $_->{NAME}, " ", @{$_->{INPUTS}}, "\n";
	foreach (@{$_->{FIELDS}}) {
		print "\t", $_->{KEY}, "\t", $_->{VALUE}, "\n";
	}
	$_->{PRINTED} = 1;
   }
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
		FIELDS => [],
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
		#print "$_\n";
		$fields = pop @{$cur_node->{FIELDS}};
		$fields->{VALUE} .= $_;
		push @{$cur_node->{FIELDS}}, $fields;
	} else {
		# Add new field to it
		push @{$cur_node->{FIELDS}}, {KEY => $var1, VALUE => $var2};
		#print "Block ", $cur_node->{NAME}, " fields are $var1 $var2\n";
	}
	# New node becomes current node
    }
}

#CDS::Tree::print_tree($root);
#CDS::Tree::do_on_nodes($root, \&print_node);
print "Lexically parsed the model file successfully\n";

# Process parsed tree to fill in the required information
process();

return 1;
}


sub process {
print "PROCESS\n";
return 1;
}
