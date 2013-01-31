package CDS::Util;
use Exporter;
@ISA = ('Exporter');

#//     \page Util Util.pm
#//     Documentation for Util.pm
#//
#// \n

# Open $fname (C header file) and locate each of the rest of the arguments
#   as a "define" name, return an array of numbers 
#
# @res_array = findDefine("my/header/file/name.h", "DEFINE1", "DEFINE1");
#
sub findDefine { 
   my($fname, @defs) = @_;

   # Determine allowed maximum IPC number
   open(CD2, "$::rcg_src_dir/" . $fname)
	|| die "***ERROR: could not open $fname header\n";
   my @inData=<CD2>;
   close CD2;
   foreach $i (@defs) {
   	my @res = grep /.*define.*$i.*/, @inData;
   	die "***ERROR: couldn't find $i in commData2.h\n" unless @res;
   	$res[0] =~ s/\D//g;
   	my $ires = 0 + $res[0];
   	die "**ERROR: unable to determine $i\n" unless $ires > 0;
   	printf "$i=$ires\n";
	push @res_array, $ires;
   }
   undef @inData;
   return @res_array;
}

return 1;
