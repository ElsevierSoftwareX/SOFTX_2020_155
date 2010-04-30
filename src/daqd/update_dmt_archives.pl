#!/bin/perl
# ------------- D M T      M I N U T E        T R E N D S
# DMT and other remote systems producing minute trend frame files.

use IO::Socket;

# Give it '-w' argument to force probing for server and waiting
# until is comes up

$do_wait = (@ARGV >= 1? ($ARGV[0] eq "-w"): 0);

# EDIT THE FOLLOWING HASH ARRAY TO UPDATE DMT INFO
#
# 22aug02 D.Barker Added SenseMonitor_H[1,2] and ServoMon[1,2]

if ($do_wait) {
#  print "probing frame builder server\n";
   while(!($socket = IO::Socket::INET->new(PeerAddr => "localhost",
					PeerPort => 8088,
					Proto	 => "tcp",
					Type	 => SOCK_STREAM))) {
	sleep 60;
#	print "waiting for server\n";
   }
   close($socket);
}

# Archive name => file prefix 
%archive=(
'IRIG-B'   => 'IRIGBMinuteTrend',
'RmsBands' => 'RmsBand',
'LineMon_H2-IOO-MC_F' => 'LineMon_H2xIOOxMC_F',
'LineMon_H2-LSC-CARM_CTRL' => 'LineMon_H2xLSCxCARM_CTRL',
'LineMon_H2-LSC-DARM_CTRL' => 'LineMon_H2xLSCxDARM_CTRL',
'TimeMon' => 'TimeMonMinuteTrend',
'ZGlitch' => 'ZGlitch',
'MultiVolt' => 'MultiVolt',
'LineMon_H1_gws' => 'LineMon_H1_gws',
'LineMon_H2_gws' => 'LineMon_H2_gws',
'LineMon_H1_ioo' => 'LineMon_H1_ioo',
'LineMon_H2_ioo' => 'LineMon_H2_ioo',
'LineMon_H1_lsc' => 'LineMon_H1_lsc',
'LineMon_H2_lsc' => 'LineMon_H2_lsc',
'LockLoss_H1' => 'LockLoss_H1',
'LockLoss_H2' => 'LockLoss_H2',
'glitchMon' => 'glitchMon',
'SenseMonitor_H1' => 'SenseMonitor_H1',
'SenseMonitor_H2' => 'SenseMonitor_H2',
'ServoMon_H1' => 'ServoMon_H1',
'ServoMon_H2' => 'ServoMon_H2',
'DataQual' => 'DataQual'
);

$daqcn = '/usr/local/bin/daqcn -c ';
$cmnd = 'delete archive "/usr1/minute-trend-frames/DMT/ARCHIVE/Data";scan archive "/usr1/minute-trend-frames/DMT/ARCHIVE/Data", "H-PREFIX_M-", ".gwf", 1;configure archive "/usr1/minute-trend-frames/DMT/ARCHIVE/Data" channels "/usr1/minute-trend-frames/DMT/ARCHIVE/channel.cfg"; ';

$cmds="";
$maxlen = 512;

while (($dir, $prefix) = each(%archive)) {
	($cmds .= $cmnd) =~ s/ARCHIVE/$dir/g;
	$cmds =~ s/PREFIX/$prefix/g;
	# do the command if command line is getting too large
	if (length $cmds > $maxlen) {
		#print "$daqcn '$cmds'\n";
		`$daqcn '$cmds'\n`;
		$cmds="";
	}
}
if (length $cmds) {
	#print "$daqcn '$cmds'";
	`$daqcn '$cmds'\n`;
}
