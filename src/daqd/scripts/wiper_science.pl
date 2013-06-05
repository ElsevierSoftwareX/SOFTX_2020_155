#!/usr/bin/perl
use File::Basename;

print "\n" .  `date` . "\n";
# Dry run, do not delete anything
$dry_run = 1;

if ($ARGV[0] eq "--delete") { $dry_run = 0; }

print "Dry run, will not remove any files!!!\n" if $dry_run;
print "You need to rerun this with --delete argument to really delete frame files\n\n" if $dry_run;
beep;

# Print each file name we delete
$verbose = 0;

# Keep frame file system at this percent usage level
$percent_keep = 87.0;

# ######################################################
# Below keep values are percentages within $percent_keep
# minus the size of raw minute trend data, which we do not delete
# This script deletes full, second and minute trend files
# to bring the file system usage to the specified values.

# Keep full frames under this percentage
$full_frames_percent_keep = 79.7;

# Keep science data frames under this percentage
$science_frames_percent_keep = 10;

# Keep trend frames under this percentage
$second_frames_percent_keep = 10.2;

# Keep minute trend frames under this percentage
$minute_frames_percent_keep = 0.005;

# ######################################################

# Frame file system mount point
$frames_dir = "/frames";
# Full frame directory
$full_frames_dir = $frames_dir . "/full";
# Science frame directory
$science_frames_dir = $frames_dir . "/science";
# Second trend frame directory
$second_trend_frames_dir = $frames_dir . "/trend/second";
# Minute trend frame directory
$minute_trend_frames_dir = $frames_dir . "/trend/minute";
# Raw minute trend data directory
$raw_minute_trend_frames_dir = $frames_dir . "/trend/minute_raw";

# Directory disk usage in kilobytes
%du;

# Determine disk usage in kilobytes; single argument is a directory name
# Returns kbytes and sets value in %du
sub ddu {
   ($dname) =  @_;
   $_= `du -sk $dname`;
   my @l = split;
   die "Couldn't determine disk usage in $dname\n" unless (0+@l) == 2;
   $du{@l[1]} = @l[0];
   return @l[0];
}

# Determinue file system size and free space in kilobytes
# Takes file system name as a single argument
# returns an array of two values, first is files system size
# second is file system free space in kilobytes
sub ddf {
   ($dname) =  @_;
   $_= `df  -P -k $dname | tail -1`;
   my @l = split;
   die "Couldn't df filesystem $dname\n" unless (0+@l) == 6;
   return (@l[1], @l[2]);
}


# Determine usage for each directory
ddu $full_frames_dir;
sleep 2;
ddu $science_frames_dir;
sleep 2;
ddu $second_trend_frames_dir;
sleep 2;
ddu $minute_trend_frames_dir;
sleep 2;
ddu $raw_minute_trend_frames_dir;
sleep 2;


$combined = 0;
print "Directory disk usage:\n";
for (keys %du) {
	printf "$_ $du{$_}k\n";
	$combined += $du{$_};
}
printf "Combined %dk or %dm or %dGb\n\n", $combined, $combined/1024, $combined/1024/1024;

# file system overall size
($kbytes, $df_usage_kbytes) = ddf($frames_dir);

$usage_kbytes = $combined;

$percent_kbytes = $kbytes / 100;
$percent_usage = $usage_kbytes / $percent_kbytes;
printf "$frames_dir size %dk at %.2f\%\n", $kbytes, $percent_usage;

if ($percent_usage <= $percent_keep) {
	printf "$frames_dir is below keep value of %.2f\%\n", $percent_keep;
	printf "Will not delete any files\n";
	printf "df reported usage %.2f\%\n", $df_usage_kbytes/$percent_kbytes;
	exit 0;
}
printf "$frames_dir above keep value of %.2f\%\n", $percent_keep;


# TODO: make sure all allocated percentages are less than 100% summed

$frame_area_size =  $percent_kbytes * $percent_keep - $du{$raw_minute_trend_frames_dir};
#printf "File system size is %dk\n", $kbytes;
printf "Frame area size is %dk\n", $frame_area_size;
$frame_area_percent = $frame_area_size / 100.0;

$full_frames_keep = $full_frames_percent_keep * $frame_area_percent;
printf "$full_frames_dir size %dk keep %dk\n", $du{$full_frames_dir}, $full_frames_keep;

if ($du{$full_frames_dir} > $full_frames_keep) { $do_full = 1; };

$science_frames_keep = $science_frames_percent_keep * $frame_area_percent;
printf "$science_frames_dir size %dk keep %dk\n", $du{$science_frames_dir}, $science_frames_keep;

if ($du{$science_frames_dir} > $science_frames_keep) { $do_science = 1; };

$second_frames_keep = $second_frames_percent_keep * $frame_area_percent;
printf "$second_trend_frames_dir size %dk keep %dk\n", $du{$second_trend_frames_dir}, $second_frames_keep;

if ($du{$second_trend_frames_dir} > $second_frames_keep) { $do_sec = 1; };

$minute_frames_keep = $minute_frames_percent_keep * $frame_area_percent;
printf "$minute_trend_frames_dir size %dk keep %dk\n", $du{$minute_trend_frames_dir}, $minute_frames_keep;

if ($du{$minute_trend_frames_dir} > $minute_frames_keep) { $do_min = 1; };

# Delete frame files in $dir to free $ktofree Kbytes of space
# This one reads file names in $dir/*/*.gwf sorts them by GPS time
# and progressively deletes them up to $ktofree limit
sub delete_frames {
	($dir, $ktofree) = @_;

	# Read file names; Could this be inefficient?
        sub byGPSTime {
                my $c = basename $a; $c =~ s/\D+(\d+)\D+(\d+)\D+/$1/g;
                my $d = basename $b; $d =~ s/\D+(\d+)\D+(\d+)\D+/$1/g;
                $c <=> $d;
        }
        @a = sort byGPSTime <$dir/*/*.gwf>;

	$dacc = 0; # How many kilobytes we deleted
	$fnum = @a;
	$dnum = 0;
	foreach $file (@a) {
	  $fs = -s $file;
	  $fs /= 1024;
	  #print $file . " " . $fs . "k " . $dacc . "\n";
	  $dacc += $fs;
	  if ($dacc >= $ktofree)  { last; }
	  $dnum ++;
	  # Delete $file here
	  if (!$dry_run) {	
	    unlink($file);
	    sleep 1;
	  }
	  if ($verbose) {
		print "Unlink $file\n";
	  }
	}
	printf "Overall %d; deleted %d\n", $fnum, $dnum;
}

# See what we need to remove in full frames
if ($do_full) {
	$k_need_to_free = $du{$full_frames_dir} -$full_frames_keep;
	printf "Deleting some full frames to free %dk\n", $k_need_to_free;

	delete_frames($full_frames_dir, $k_need_to_free);
}

# See what we need to remove in science frames
if ($do_science) {
	$k_need_to_free = $du{$science_frames_dir} -$science_frames_keep;
	printf "Deleting some science frames to free %dk\n", $k_need_to_free;

	delete_frames($science_frames_dir, $k_need_to_free);
}

# See if anythings needs to go in second trends
if ($do_sec) {
	$k_need_to_free = $du{$second_trend_frames_dir} -$second_frames_keep;
	printf "Deleting some second trend frames to free %dk\n", $k_need_to_free;

	delete_frames($second_trend_frames_dir, $k_need_to_free);
}

if ($do_min) {
	$k_need_to_free = $du{$minute_trend_frames_dir} -$minute_frames_keep;
	printf "Deleting some minute trend frames to free %dk\n", $k_need_to_free;

	delete_frames($minute_trend_frames_dir, $k_need_to_free);
}


