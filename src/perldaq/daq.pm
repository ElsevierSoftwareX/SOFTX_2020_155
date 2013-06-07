package DAQ;
use Exporter;
@ISA = ('Exporter');

use IO::Socket;

%channels = ();
$host = "";
$port = 8088;


# get the channel list from the server, store in %channels
sub connect {
my ($host, $port) = @_;

if (! defined $host) {
	$host = 'localhost';
}
if (! defined $port) {
	$port = 8088;
}
$DAQ::host = $host;
$DAQ::port = $port;

$remote = IO::Socket::INET->new( Proto => "tcp", PeerAddr => $host, PeerPort => $port);
unless ($remote) { die "cannot connect to daqd on $host:$port" }
$remote->autoflush(1);

# Get the channel list and store the values
#

print $remote "status channels 3;\n";
$len = 0 + <$remote>;
#print "$len channels configured\n";
for my $i (1 .. $len) {
	#print $i, "\n";
	my $name = <$remote>;
	my $rate = 0 + <$remote>;
	my $type = 0 + <$remote>;
	my $tpnum = 0 + <$remote>;
	my $grpnum = 0 + <$remote>;
	my $units = <$remote>;
	my $gain = .0 + <$remote>;
	my $slope = .0 + <$remote>;
	my $offset = .0 + <$remote>;
	chomp $name;
	chomp $units;
	$channels{$name} = {rate => $rate,
				type => $type,
				tpnum => $tpnum,
				grpnum => $grpnum,
				units => $units,
				gain => $gain,
				slope => $slope,
				offset => $offset};
	#print $channels{$name};
} 
close $remote;
return 0;
}

# print the while channel list or just a single channel, given an argument (channel name)
sub print_channel {
my ($cn) = @_;

if (!defined $cn) {
for my $chname (keys %channels) {
	print $chname, "\n";
	for (keys %{$channels{$chname}}) {
		print $_, "=", ${$channels{$chname}}{$_}, "\n";
	}
}
} else {
	print $cn, "\n";
	for (keys %{$channels{$cn}}) {
		print $_, "=", ${$channels{$cn}}{$_}, "\n";
	}
}
}

# get current time
sub gps {
my $remote = IO::Socket::INET->new( Proto => "tcp", PeerAddr => $DAQ::host, PeerPort => $DAQ::port);
unless ($remote) { die "cannot connect to daqd on $DAQ::host:$DAQ::port" }
$remote->autoflush(1);
my $gps = 0;

# This loop here is to fix a frame builder bug
# sometimes "gps" commands returns strange number (0 or a number much less than the current GPS time)
# TODO: need to fix this in the frame builder
do {
print $remote "gps;\n";
read($remote, $resp, 24);
read($remote, $gps, 4);
$gps = unpack( 'N', $gps );
#print "gps=$gps\n";
} while ($gps <= 986074331);
close $remote;
return $gps;
}

# get data starting at (optional) $gps for $req_seconds (default 1 second)
# for the channel $chname
sub acquire {
my ($chname, $req_seconds, $gps) = @_;
die "Need to connect first\n" unless defined $remote;
die "Unspecified channel name\n" unless defined $chname;
die "Bad channel name\n" unless defined $channels{$chname};
$req_seconds = 1 if ! defined $req_seconds;

#print "Acquiring $chname for $req_seconds from $gps\n";
my $remote = IO::Socket::INET->new( Proto => "tcp", PeerAddr => $DAQ::host, PeerPort => $DAQ::port);
unless ($remote) { die "cannot connect to daqd on $DAQ::host:$DAQ::port" }
$remote->autoflush(1);

if (defined $gps) {
	print $remote "start net-writer $gps $req_seconds {\"$chname\"};\n";
} else {
	print $remote "start net-writer {\"$chname\"};\n";
}
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
$len = unpack( 'N', $len );
#if ($len) { 
#print "Getting offline data\n";
#} else {
#print "received 0 response (online data)\n";
#}

my $accum_seconds = 0;

undef @result_array;
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
  	#print "gps=$gps; seconds=$seconds; gps_n=$gps_n; seq=$seq\n";
  	#print "data length=$len\n";
	@data_array = unpack( '(a4)*', $data );
	my $as = @data_array;
	#print $as, "\n";
	for (@data_array) { $_ = unpack 'f', reverse; }
	#for (@data_array) { printf "%.1f ", $_; }
	#print "\n";
	push (@result_array, @data_array);
	$accum_seconds += $seconds;
	if ($accum_seconds >= $req_seconds) {last};
  }
}
close $remote;
return @result_array;
}

return 1;
