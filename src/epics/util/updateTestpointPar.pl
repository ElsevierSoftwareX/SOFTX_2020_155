#!/usr/bin/perl 

use Getopt::Long;

my $par_file = undef;
my $gds_node = undef;
my $site_letter = undef;
my $system = undef;
my $ignore_errors = undef;
my $hostname = undef;
my $remove = undef;

# c.f. http://perldoc.perl.org/Getopt/Long.html
GetOptions("par_file=s"=>\$par_file,
	   "gds_node=s"=>\$gds_node,
           "site_letter=s"=>\$site_letter,
           "system=s"=>\$system,
           "host=s"=>\$hostname,
	   "f"=>\$ignore_errors,
	   "remove"=>\$remove);

if ($ignore_errors) {
	$SIG{__DIE__} = sub { exit (0); };
}

if ($par_file eq undef || $gds_node eq undef || $site_letter eq undef || $system eq undef || $hostname eq undef) {
	die "Usage: $0 -par_file=/opt/rtcds/geo/g1/target/gds/param/testpoint.par -gds_node=0 -site_letter=G -system=g1x11 -host=scipe17\n";
}

# Read the old DAQ file and store all fields for all sections
# Also need to input the new file and store all sections marked commented out.

my $par = readf($par_file, 0, 0, 0);

if ($remove) {
 delete $par->{$gds_node};
} else {
  if (exists $par->{$gds_node}) {
	# See if this is the same system/host pair
	# Otherwise warn and fail
	if ($par->{$site_letter . "-node" . $gds_node . ":hostname"} ne $hostname
	    || $par->{$site_letter . "-node" . $gds_node . ":system"} ne $system) {
		warn "ERROR: This node $gds_node is already installed as:\n";
		warn "	hostname=", $par->{$site_letter . "-node" . $gds_node . ":hostname"}, "\n";
		warn "	system=", $par->{$site_letter . "-node" . $gds_node . ":system"}, "\n";
		warn "The new entry you are trying to write is as follows:\n";
		warn "	hostname=$hostname\n";
		warn "	system=$system\n";
		warn "This script will not overwrite existing entries in testpoint.par\n";
		warn "If this is an attempt to move an existing system from one host to another,\n";
		warn "please remove conflicting entry from testpoint.par file.\n";
		exit 1;
	}
  }
  $par->{$site_letter . "-node" . $gds_node . ":hostname"} = $hostname;
  $par->{$site_letter . "-node" . $gds_node . ":system"} = $system;
  $par->{$gds_node} = $site_letter . "-node" . $gds_node;
}

#print_r($par);

foreach $item (sort { $a <=> $b } keys %$par) {
        # Numeric keys are channel numbers to name
        if ($item =~ /^(\d+\.?\d*|\.\d+)$/) {
          my $name = $par->{$item};
	  print "[$name]\n";
	  print "hostname=" . $par->{$name . ":hostname"} . "\n";
	  print "system=" . $par->{$name . ":system"} . "\n";
        }
}

exit (0);

# Read testpoint config file and map names and numbers
sub readf {
  # $ignore -- ignore comments 
  my($fname, $dir, $ignore, $all_commented_out) = @_;
  my $tpnames = {};

  open(IN,"<" . $fname) || die "Couldn't open $fname for reading";
  my $chnname;
  while (<IN>) {
    my $commented_out = 0;

    chomp;
    s/^\s+//;
    s/\s+$//;
    if (/^#/) { $commented_out = 1; }
    if ($ignore) {
	s/^#\s*//; 	# ignore comment sign
    } else {
    	s/#.*//;	# no comments
    }
    next unless length;
    my ($var, $value) = split(/\s*=\s*/, $_, 2); 
    if ($var =~ /^\[/) {
	$chnname = $var;
	$chnname =~ tr/\[\]//d;
	$gdsnode = $chnname;
	$gdsnode =~ s/[^\d]+(\d+)/$1/;
	$tpnames->{$gdsnode} = $chnname;
    } else {
    	$tpnames->{$chnname . ":" . $var} = $value;
    }
  }
  close(IN);
  return $tpnames;
}

sub print_section() {
  my($section, $pf) = @_;
  # Here is the start of new section. print old section
  if ($pf) { print $section; }
  else {
    # Print this section commented out
    print "# This channel removed by system reinstall\n";
    for (split(/\n/, $section)) {
	print "# $_\n";
    }
  }
}

# Recursive print 
sub print_r {
  my($daq, $all ) = @_;
  if ($all) {
     foreach $item (sort keys %$daq) {
	  my $name = $daq->{$item};
	  printf "%s -> %s\n", $item, $name;
     }
  } else {
     foreach $item (sort { $a <=> $b } keys %$daq) {
	# Numeric keys are channel numbers to name
	if ($item =~ /^(\d+\.?\d*|\.\d+)$/) {
	  my $name = $daq->{$item};
	  printf "%s -> %s\n", $item, $name;
	  foreach $field (keys %$daq) {
		  if ($field =~ /^$name/) {
			  	printf "\t%s -> %s\n", $field, $daq->{$field};
	          }
	  }
        }
      }
  }
}

