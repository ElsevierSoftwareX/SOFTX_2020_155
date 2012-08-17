#!/usr/bin/perl -w 
use strict;
use File::Basename;

# this program is meant to be run by the MEDM "Shell Command Controller"
# and will take or display a snapshot of the calling MEDM screen 
#  First Argument - Action 
#   u/U - update current snapshot
#   v/V - view current snapshot
#   p/P - view previous snapshot
#
#  Second Argument - full path of the calling MEDM screen (&A in MEDM language) 
# 
#  Third Argument - X Window ID of the calling MEDM screen (&X in MEDM language)
#                    ( only needed for update) 
#  so commands are 
#   medmsnap.pl U &A &W
#   medmsnap.pl V &A
#   medmsnap.pl P &A

# ARGV[0] is Action
my $snap_act = $ARGV[0];
$snap_act = uc(substr($snap_act,0,1));

# ARGV[1] is full path to adl file
my $adlfile = $ARGV[1];

# parse path to get directory put snaps in sub-directory 
my $adldir;
my $adlpre;
my $adlext;
($adlpre,$adldir,$adlext) = fileparse($adlfile, qr/\..*/);
my $snapdir = $adldir."snap";

# Strip off _Main if found in adl file
my $mainpos = rindex($adlpre,"\_Main");
if ($mainpos gt 0) {
    $adlpre = substr($adlpre,0,$mainpos);
}
my $currfile = $snapdir."/".$adlpre."\_0.png";
my $prefile = $snapdir."/".$adlpre."\_1.png";

if ($snap_act eq "U") {
    my $window = $ARGV[2]; # X Window ID of the MEDM screen

    if (! -e $snapdir) { # if the snapshot directory doesn't exist, make it
	system "mkdir -p $snapdir";
    }

    if (-e  $currfile ) { # copy to the previous file
	system "cp $currfile $prefile";
    }
    system  "import -window $window $currfile &";
}
elsif ($snap_act eq "V") {
    if (-e  $currfile ) { 
        system "display $currfile &";
    } 
}
elsif ($snap_act eq "P") {
    if (-e  $prefile ) { 
        system "display $prefile &";
    } 
}
