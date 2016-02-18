package CDS::BusSelect;
use Exporter;
@ISA = ('Exporter');

#//     \page BusSelect BusSelect.pm
#//     Documentation for BusSelect.pm
#//
#// \n
sub linkBusSelects {
my ($totalParts) = @_;
# Loop thru all parts
for($ii=0;$ii<$totalParts;$ii++)
{
	# Loop thru all part input connections
        for($jj=0;$jj<$::partInCnt[$ii];$jj++)
        {
	   # Loop thru all parts looking for part name that matches input name
           for($kk=0;$kk<$totalParts;$kk++)
           {
		# If name match
                if($::partInput[$ii][$jj] eq $::xpartName[$kk])
                {
			# If input is from a BUSS part
                        if($::partType[$kk] eq "BUSS")
                        {
				$gfrom = $::partInputPort[$ii][$jj];
				$adcName = $::partInput[$kk][$gfrom];
				$adcTest = substr($adcName,0,3);
				# Check if BUSS is actually an ADC part.
				if($adcTest eq "adc")	# Make ADC channel connection.
				{
    					($var1,$var2) = split(' ',$adcName);
					$::partInputType[$kk][$gfrom] = "Adc";

				} else {	#If BUSS is not ADC part.
    					($var1,$var2) = split(' ',$adcName);
					$::partInput[$ii][$jj] = $var1;
        				for($ll=0;$ll<$::partInCnt[$kk];$ll++)
					{
                				if($::partInput[$kk][$ll] eq $adcName)
						{
							$port = $::partInputPort[$kk][$ll];
							$ports = $::partOutputPort[$kk][$ll];
							$ports = $var2;
							$::partInputPort[$ii][$jj] = $ports;
							for($xx=0;$xx<$totalParts;$xx++)
							{
								if($::xpartName[$xx] eq $var1)
								{
									for($q1=0;$q1<$::partOutCnt[$xx];$q1++)
									{
										
										if($::partOutput[$xx][$q1] =~ m/Bus/)
										{
											# Redefine part output from BUSS to real part.
											$::partOutput[$xx][$q1] = $xpartName[$ii];
											$::partOutputPort[$xx][$q1] = $jj;
										}
									}
								}
							}
						}
					}
				}
                        }
                }
           }
        }
}

}
