
package CDS::Parser;
use Exporter;
@ISA = ('Exporter');


require "lib/Tree.pm";

sub parse() {
while (<::IN>) {
    # Strip out quotes and blank spaces
    #tr/\"/ /;
    #tr/\</ /;
    #tr/\>/ /;
    s/^\s+//;
    s/\s+$//;
    $lcntr ++;
    ($var1,$var2,$var3,$var4) = split(/\s+/,$_);
    if ($var2 eq "{") {
	# New block
	#print  " " x scalar @blocks; print "Block $var1 starts on $lcntr\n";
	push @blocks, $var1;
    } elsif ($var1 eq "}") {
	my $bn = pop @blocks;
	#print  " " x scalar @blocks; print "Block $bn ends on $lcntr\n";
    }
}

exit (0);
}
