package CDS::Tree;
use Exporter;
@ISA = ('Exporter');

# Parts tree root name
$tree_root_name = "__root__";

# set debug level (0 - no debug messages)
$dbg_level = 2;

# Print debug message
# Example:
# debug (0, "debug test: openBrace=$openBrace");
#
sub debug {
  if ($dbg_level > shift @_) {
	print @_, "\n";
  }
}

# Parts "tree" root
#$root = {
#	NAME => $tree_root_name,
#	NEXT => [], # array of references to leaves
#};

# Run a function on all tree leaves
# Arguments:
#	tree root reference
#	function reference
#
sub do_on_leaves {
	my($tree, $f) = @_;
	if (0 == @{$tree->{NEXT}}) {
		&$f($tree);
	} else {
		for (@{$tree->{NEXT}}) {
                	do_on_leaves($_, $f);
        	}
	}
}

# Run a function on all tree nodes
# Arguments:
#	tree root reference
#	function reference
#	function argument
#
# :WARNING: the functionality of some code depends on $_ not declared local here
#
sub do_on_nodes {
	my($tree, $f, $arg, $parent) = @_;
	#print("do on nodes; node=$tree; parent=$parent\n");
	if (&$f($tree, $arg, $parent) == 0) {
	  for (@{$tree->{NEXT}}) {
               	do_on_nodes($_, $f, $arg, $tree);
	  }
	}
}

# Find graph node by name
#
sub find_node {
	my($tree, $name, $field, $name1, $field1) = @_;
	if ($field1 ne undef) {
	  if (${$tree->{FIELDS}}{$field} eq $name 
	  	&& ${$tree->{FIELDS}}{$field1} eq $name1) {
		return $tree;
	  } else {
		for (@{$tree->{NEXT}}) {
			#print ${$_->{FIELDS}}{$field}, "\n";
                	$res = find_node($_, $name, $field, $name1, $field1);
			if ($res) {
				return $res;
			}
        	}
	  }
  } elsif ($field ne undef) {
	  if (${$tree->{FIELDS}}{$field} eq $name) {
		return $tree;
	  } else {
		for (@{$tree->{NEXT}}) {
			#print ${$_->{FIELDS}}{$field}, "\n";
                	$res = find_node($_, $name, $field);
			if ($res) {
				return $res;
			}
        	}
	  }
	} else {
	  if ($tree->{NAME} eq $name) {
		return $tree;
	  } else {
		for (@{$tree->{NEXT}}) {
			#print $_->{NAME}, "\n";
                	$res = find_node($_, $name);
			if ($res) {
				return $res;
			}
        	}
	  }
	}
	return undef;
}

# Find all DAQ channel Annotations
sub find_daq_annotation {
	my($tree) = @_;
	if ($tree->{NAME} eq "Annotation" && ${$tree->{FIELDS}}{Name} =~ /^#\s*[dD][aA][qQ]\s+[cC]hannels/) {
		return ($tree, "");
	} else {
		for (@{$tree->{NEXT}}) {
			#print "Recursing into ", $_->{NAME}, " ", ${$_->{FIELDS}}{"Name"}, "\n";
                	($res, $prefix) = find_daq_annotation($_);
			if ($res) {
				#print $tree->{NAME}, " ", ${$tree->{FIELDS}}{"Name"}, " ", ${$tree->{FIELDS}}{"Type"}, "\n";
				# Annotate the prefix if this is block
				if ($tree->{NAME} eq "Block") {
				  $prefix = ${$tree->{FIELDS}}{"Name"}. "_" .$prefix;
				}
				return ($res, $prefix);
			}
        	}
	}
	return undef;
}

# Print all terminator nodes
#do_on_leaves($root, sub {if ($_->{PRINTED} != 1) {print $_->{NAME}, "\n"; $_->{PRINTED} = 1; }});

# Print the tree
$print_no_repeats = 1;
sub print_tree {
	my($tree, $level) = @_;
	my($space);
	if ($print_no_repeats && $tree->{PRINTED}) { return; }
	for (0 .. $level) { $space .= ' '; }
	#debug 0, $space, "{", $tree->{NAME}, "; name=", ${$tree->{FIELDS}}{Name}, "; src=", ${$tree->{FIELDS}}{SrcBlock}, "; src_port=", ${$tree->{FIELDS}}{SrcPort}, ";dst=", ${$tree->{FIELDS}}{DstBlock}, ";dts_port=", ${$tree->{FIELDS}}{DstPort}, " nref=", scalar @{$tree->{NEXT}}, "}";
	for (@{$tree->{NEXT}}) {
		print_tree($_, $level + 1);
	}
	if ($print_no_repeats) { $tree->{PRINTED} = 1; }
}

#do_on_nodes($root, sub {if ($_->{PRINTED} != 1) {print $_->{NAME}, " ", @{$_->{INPUTS}}, "\n"; $_->{PRINTED} = 1; }});

sub reset_visited { 
	my ($node) = @_;
	$node->{VISITED} = 0;
}


return 1;
