#!/usr/bin/perl -w
# Get online data for one float (datatype 4) channel
use IO::Socket;
#unless (@ARGV > 1) { die "usage: $0 host document ..." }
$host = shift(@ARGV) || 'localhost';
#$EOL = "\015\012";
#$BLANK = $EOL x 2;
$remote = IO::Socket::INET->new( Proto => "tcp",
 				 PeerAddr => $host,
    				 PeerPort => "8088",
       			);
unless ($remote) { die "cannot connect to daqd on $host" }
$remote->autoflush(1);
print $remote "start net-writer {\"C1:SUS-ETMY_OPLEV_SUM\"};\n";
#while ( <$remote> ) { print }
#close $remote;

#read($remote, $length. 4);
#read($new_sock, $data, $length);


# Initial responses "0000"
read($remote, $resp, 4);
#print $resp. "\n\n";
read($remote, $resp, 4);
#print $resp. "\n";
read($remote, $resp, 4);
#print $resp. "\n\n";

my $len = 0;
read($remote, $len, 4);
$len = unpack( 'i', $len );
die unless (!$len);
#received 0 response (online data)

while(read($remote, $len, 4)) {
  $len = unpack( 'N', $len );
  #print "$len\n";
  read($remote, $seconds, 4);
  $seconds = unpack( 'N', $seconds );
  read($remote, $gps, 4);
  $gps = unpack( 'N', $gps );
  read($remote, $gps_n, 4);
  $gps_n = unpack( 'N', $gps_n );
  read($remote, $seq, 4);
  $seq = unpack( 'N', $seq );
  $len -= 16;
  read($remote, $data, $len);
  if ($gps == 0xffffffff) {
	#reconfig block
  } else {
  	print "gps=$gps; seconds=$seconds; gps_n=$gps_n; seq=$seq\n";
  	#print "data length=$len\n";
	@data_array = unpack( '(a4)*', $data );
	my $as = @data_array;
	#print $as, "\n";
	for (@data_array) { $_ = unpack 'f', reverse; }
	for (@data_array) { printf "%.1f ", $_; }
	print "\n";
  }
}
