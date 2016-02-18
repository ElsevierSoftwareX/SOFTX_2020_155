package CDS::Input;
use Exporter;
@ISA = ('Exporter');

#//     \page Input Input.pm
#//     Documentation for Input.pm
#//
#// \n

# Open $fname (C header file) and locate each of the rest of the arguments
#   as a "define" name, return an array of numbers 
#
# @res_array = findDefine("my/header/file/name.h", "DEFINE1", "DEFINE1");
#
sub checkInputConnect {
my ($totalParts,$ckLength) = @_;
my $kk = 0;
my $mm = 0;

for($ii=0;$ii<$totalParts;$ii++)
{
        if($::partType[$ii] eq "INPUT")
        {
		if(($::partInCnt[$ii] == 0) || ($ckLength && length($::partInputType[$ii][0]) == 0))
                {
                print "\nINPUT $::xpartName[$ii] is NOT connected \n";
                $kk ++;
                }
        }
}

	print "Found $kk INPUT ERRORS in total parts = $mm\n";
    return $kk;
}


