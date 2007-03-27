#!/usr/bin/perl 

use Getopt::Long;

# Update DAQ config file test point numbers
# with new values, print new DAQ config file out

# Old DAQ config file
my $daq_file = "/dev/null";

# Old testpoints file
my $old_tp_file = "/dev/null";

# New testpoints file
my $new_tp_file = "/dev/null";

# c.f. http://perldoc.perl.org/Getopt/Long.html

GetOptions("daq=s"=>\$daq_file,
           "old_tp_file=s"=>\$old_tp_file,
           "new_tp_file=s"=>\$new_tp_file);

# Read the test point config files in
# Create numbers -> names map
my $old = readf($old_tp_file, 0);
#print %$old;
# Create names to numbers map
my $new = readf($new_tp_file, 1);
#print %$new;

my $section = undef; # The whole section
my $chnname = undef; # Current section channel name
my $pf = 1;	     # Print flag

open(IN,"<" . $daq_file) || die "Couldn't open $daq_file for reading";
while (<IN>) {
    my $l = $_;
    chomp;
    s/#.*//;	# no comments
    s/^\s+//;
    s/\s+$//;
    #next unless length;
    my ($var, $value) = split(/\s*=\s*/, $_, 2); 
    if ($var =~ /^\[/) {
	if ($chnname ne undef) {
	  print_section($section, $pf);
	  $section = undef;
	  $pf = 1;
	}
	$chnname = $var;
	$chnname =~ tr/\[\]//d;
	#print $chnname;
    }
    if ($chnname eq undef) {
	# Not in section, just print
	print $l;
    } else {
     if ($var eq "chnnum") {
#	print "$chnname $value ", $old->{$value}, " ", $new->{$old->{$value}}, "\n";
	my $newval = $new->{$old->{$value}};
	if ($newval ne undef) {
	  $l =~ s/$value/$newval/g;
	} else {
	  $pf = 0;
	}
     }
     $section .= $l;
    }
}
print_section($section, $pf);

close(IN);
exit 0;

# Read testpoint config file and map names and numbers
sub readf {
  my($fname, $dir) = @_;
  $tpnames = {};

  open(IN,"<" . $fname) || die "Couldn't open $fname for reading";
  my $chnname;
  while (<IN>) {
    chomp;
    s/#.*//;	# no comments
    s/^\s+//;
    s/\s+$//;
    next unless length;
    my ($var, $value) = split(/\s*=\s*/, $_, 2); 
    if ($var =~ /^\[/) {
	$chnname = $var;
	$chnname =~ tr/\[\]//d;
    }
    if ($var eq "chnnum") {
	if ($dir) {
    	  $tpnames->{$chnname} = $value;
	} else { 
    	  $tpnames->{$value} = $chnname;
	}
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
