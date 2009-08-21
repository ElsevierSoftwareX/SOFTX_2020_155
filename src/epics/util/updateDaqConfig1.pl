#!/usr/bin/perl 

use Getopt::Long;

# Update DAQ config file test point numbers
# with new values, print new DAQ config file out

# Old DAQ config file
my $daq_file = undef;

# Old testpoints file
my $old_tp_file = undef;

# New testpoints file
my $new_tp_file = undef;

my $ignore_errors = 0;

# c.f. http://perldoc.perl.org/Getopt/Long.html

GetOptions("daq_old=s"=>\$old_daq_file,
	   "daq=s"=>\$daq_file,
           "old_tp_file=s"=>\$old_tp_file,
           "new_tp_file=s"=>\$new_tp_file,
	   "f"=>\$ignore_errors);

if ($ignore_errors) {
	$SIG{__DIE__} = sub { exit (0); };
}

if ($daq_file eq undef || $old_daq_file eq undef) {
	die "Usage: $0 -daq_old=M1SYS.ini~ -daq=M1SYS.ini -old=tpchn_M1.par~ -new=tpchn_M1.par";
}

# Not sure this block is needed
#open(IN,"<" . $daq_file) || die "Couldn't open $daq_file for reading";
#if ($new_tp_file eq undef || $old_tp_file eq undef) {
#print <IN>;
#exit 0;
#}
#close(IN);
my $daq = readf($daq_file, 0, 1);
my $daq_old = readf($old_daq_file, 0, 1);
#print %$daq;

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


# Read the test point config files in
# Create numbers -> names map
my $old = readf($old_tp_file, 0, 0);
#print %$old;
# Create names to numbers map
my $new = readf($new_tp_file, 1, 0);
#print %$new;

#print_r $daq;


my $section = undef; # The whole section
my $chnname = undef; # Current section channel name
my $pf = 1;	     # Print flag


# Read the old DAQ file and store all fields for all sections
# Need to write a function that inputs an .ini file and stores everything
# for every section.
# Also need to input the new file and store all sections marked commented out.
# Next step: go through the list of uncommented sections in the old
# DAQ ini file and try finding each section in the new DAQ ini file.
# Print a message to the user if such a section is found.
# If such section is found, need to mark it uncommented and then patch in
# all the fields, except "chnnum". "chnnum" needs to be new, not old.
# Check that no old "datarate" field is greater then the new "datarate"
# in "default" section and if it is greater use the new datarate instead
# and print a message to the user.
# At the end check how many active channels are there in the new DAQ ini file
# and uncomment the first two OUT_DAQ channels in there is none printing a
# message to the user.

open(IN,"<" . $daq_file) || die "Couldn't open $daq_file for reading";
while (<IN>) {
    my $l = $_;
    chomp;
    #s/#.*//;	# no comments
    s/^\s+//;
    s/\s+$//;
    #next unless length;
    my ($var, $value) = split(/\s*=\s*/, $_, 2); 
    if ($var =~ /^#?\[/) {
	# end of section
	if ($chnname ne undef) {
	  print_section($section, $pf);
	  $section = undef;
	  $pf = 1;
	}
	$chnname = $var;
	$chnname =~ tr/\[\]#//d;
	#print $chnname;
    }
    if ($chnname eq undef) {
	# Not in section, just print
	print $l;
    } else {
     if ($var eq "chnnum" || $var eq "#chnnum") {
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
#print_section($section, $pf);

close(IN);
exit 0;

# Read testpoint config file and map names and numbers
sub readf {
  # $ignore -- ignore comments 
  my($fname, $dir, $ignore) = @_;
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
	s/^#//; 	# ignore comment sign
    } else {
    	s/#.*//;	# no comments
    }
    next unless length;
    my ($var, $value) = split(/\s*=\s*/, $_, 2); 
    if ($var =~ /^\[/) {
	$chnname = $var;
	$chnname =~ tr/\[\]//d;
	if ($chnname eq "default") {
		$tpnames->{"0"} = $chnname;
	}
    } else {
    if ($var eq "chnnum") {
	if ($dir) {
    	  $tpnames->{$chnname} = $value;
	} else { 
    	  $tpnames->{$value} = $chnname;
	}
	if ($commented_out) {
	  $tpnames->{$chnname . ":commented_out"} = 1;
	}
    } else {
	$tpnames->{$chnname . ":" . $var} = $value;
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
