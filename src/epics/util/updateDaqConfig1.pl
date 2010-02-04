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

# Read the old DAQ file and store all fields for all sections
# Also need to input the new file and store all sections marked commented out.

my $daq = readf($daq_file, 0, 1, 1);
my $daq_old = readf($old_daq_file, 0, 1, 0);
#print_r($daq_old);

# Read the test point config files in
# Create numbers -> names map
my $old = readf($old_tp_file, 0, 0);
#print %$old;

# Create names to numbers map
my $new = readf($new_tp_file, 1, 0);

#print %$new;
#print_r $daq;



# Go through the list of uncommented sections in the old
# DAQ ini file and try finding each section in the new DAQ ini file.
# Print a message to the user if such a section is found.
# If such section is found, need to mark it uncommented and then patch in
# all the fields, except "chnnum". "chnnum" needs to be new, not old.
# Check that no old "datarate" field is greater then the new "datarate"
# in "default" section and if it is greater use the new datarate instead
# and print a message to the user.

my $myrate = $daq->{"default:datarate"};
print STDERR "New default DAQ rate is $myrate\n";

foreach $item (sort { $a <=> $b } keys %$daq_old) {
	# Numeric keys are channel numbers to name
	if ($item =~ /^(\d+\.?\d*|\.\d+)$/) {
	  my $name = $daq_old->{$item};
 	  if ($name eq "default") { next; }
	  if ($daq_old->{$name . ":commented_out"} == 0) { # Uncommented only
	    #printf "%s -> %s\n", $item, $name;
	    #foreach $field (keys %$daq_old) {
		  #if ($field =~ /^$name/) {
		#	  	printf "\t%s -> %s\n", $field, $daq_old->{$field};
	          #}
	    #}
	    # TRy finding this section in the new DAQ init file
	    if (length $daq->{$name . ":acquire"}) {
		# Found the old channel name in the DAQ ini file
		$daq->{$name . ":commented_out"} = 0;	# Uncomment this chan
		# PAtch in all the fields excluding the channel number
		# The channel number will be the new one
	        foreach $field (keys %$daq_old) {
		  if ($field =~ /^$name/) {
		    # Skip chnnum
		    if ($field eq "$name:chnnum") { next; }
		    $daq->{$field} = $daq_old->{$field};
		    #printf STDERR "%s -> %s\n", $field, $daq->{$field};
		    # Make sure the old datarate is not greater
		    # than the new default datarate (new maximum)
		    if ($field eq "$name:datarate") {
			if ($myrate < $daq_old->{$field}) {
			   printf STDERR "Warning: adjusting $name darate to $myrate; it was %d before\n", $daq_old->{$field};
			   $daq->{$field} = $myrate;
			}
		    }
	          }
	        }
	    }
	  }
        }
}

# Check how many active channels are there in the new DAQ ini file
# and uncomment the first two OUT_DAQ channels in there is none printing a
# message to the user.

my $n_active_chans = 0;

foreach $item (sort { $a <=> $b } keys %$daq) {
	# Numeric keys are channel numbers to name
	if ($item =~ /^(\d+\.?\d*|\.\d+)$/) {
	  my $name = $daq->{$item};
 	  if ($name eq "default") { next; }
	  if ($daq->{$name . ":commented_out"} == 0) { # Uncommented only
		$n_active_chans++;
	  }
        }
}

print STDERR "There are $n_active_chans active channels in the new DAQ ini file\n";

# TODO: uncomment one or two _OUT channels to make no less than 2 active chans

# Print out the new ini file
print "[default]\n";
# Print default section first
foreach $field (keys %$daq) {
	  if ($field =~ /^default/) {
	   	my ($fv) = $field =~ m/default:([^:]+)/g;
	  	printf "%s=%s\n", $fv, $daq->{$field};
          }
}
print "\n";

# Print all the rest of sections later
foreach $item (sort { $a <=> $b } keys %$daq) {
	# Numeric keys are channel numbers to name
	if ($item =~ /^(\d+\.?\d*|\.\d+)$/) {
	  if ($item == 0) { next; } # Skip default section
	  my $name = $daq->{$item};
	  my $cmnt = $daq->{$name . ":commented_out"};
	  if ($cmnt) { $cmnt = "#"; } else { $cmnt = ""; };
	  printf "%s[%s]\n", $cmnt, $name;
	  foreach $field (sort keys %$daq) {
	    if ($field =~ /^$name/) {
	   	my ($fv) = $field =~ m/$name:([^:]+)/g;
		if ($fv eq "commented_out") { next; };
	  	printf "%s%s=%s\n", $cmnt, $fv, $daq->{$field};
	    }
          }
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
	if ($commented_out || $all_commented_out) {
	  $tpnames->{$chnname . ":commented_out"} = 1;
	}
    }
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

