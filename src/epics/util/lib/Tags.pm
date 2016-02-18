package CDS::Tags;
use Exporter;
@ISA = ('Exporter');

#//     \page Tags Tags.pm
#//     Documentation for Tags.pm
#//
#// \n

sub replaceGoto {
my ($totalParts) = @_;

# Loop thru all parts
for($ii=0;$ii<$totalParts;$ii++)
{
	# Loop thru all part output connections
        for($jj=0;$jj<$::partOutCnt[$ii];$jj++)
        {
$xp = 0;
	   # Loop thru all parts looking for part name that matches output name
           for($kk=0;$kk<$totalParts;$kk++)
           {
		# If name match
                if($::partOutput[$ii][$jj] eq $::xpartName[$kk])
                {
			# If output is to a GOTO part
                        if($::partType[$kk] eq "GOTO")
                        {
				# Search thru all parts looking for a matching FROM tag
                                for($mm=0;$mm<$totalParts;$mm++)
                                {
                                        if(($::partType[$mm] eq "FROM") && ($::partInput[$mm][3] eq $::partOutput[$kk][3]) )
                                        {
						# Remove parts input FROM tag name with actual originating part name
						#print "MATCHED GOTO $xpartName[$kk] with FROM $xpartName[$mm] $partInput[$ii][$jj] to $partInput[$mm][1] $partType[$ii] $partOutCnt[$mm] $partOutput[$mm][0]\n";
						$::partGoto[$mm] = $::xpartName[$ii];
	   					$totalPorts = $::partOutCnt[$ii];
						$xp = 0;
						for($yy=0;$yy<$::partOutCnt[$mm];$yy++)
						{
							if($yy > 0)
							{
								$port = $totalPorts;
								$totalPorts ++;
								$xp ++;
							} else {
								$port = $jj;
							}
							$::partOutput[$ii][$port] = $::partOutput[$mm][$yy];
							if($::partType[$ii] eq "OUTPUT")
							{
								$::partOutputPort[$ii][$port] = $::partOutputPort[$mm][$yy]+1;;
							} else {
								$::partOutputPort[$ii][$port] = $::partOutputPort[$mm][$yy];;
							}
						#print "\t $partOutput[$ii][$jj] $partOutput[$mm][0] $partOutputPort[$ii][$jj]\n";
						#print "\t $xpartName[$ii] $partOutput[$ii][$jj] $partOutputPort[$ii][$jj]\n";
						}
							$::partOutCnt[$mm] = 0;


                                        }
                                }
                        }
                }
           }
        }
	if($xp > 0) 
	{
		  $::partOutCnt[$ii]  = $totalPorts;
		print "\t **** $::xpartName[$ii] has new output count $totalParts $::partOutCnt[$ii]\n";
	}
}

}

sub replaceFrom {
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
                        # If input is from a FROM part
                        if($::partType[$kk] eq "FROM")
                        {
				 #print "Found FROM $partInput[$kk][1] in part $xpartName[$ii]\n";
                                # Search thru all parts looking for a matching GOTO tag
                                for($ll=0;$ll<$totalParts;$ll++)
                                {
                                        # If GOTO tag matches FROM tag
                                        if(($::partType[$ll] eq "GOTO") && ($::partOutput[$ll][3] eq $::partInput[$kk][3]) )
                                        {
                                                # Remove parts input FROM tag name with actual originating part name
                                                 #print "\t matched port $jj $partInCnt[$ii] $xpartName[$ii] $partInput[$kk][$jj] to $partOutput[$kk][1] $partInput[$kk][1]\n";
                                                $::partInput[$ii][$jj] = $::partGoto[$kk];
                                        }
                                }
                        }
                }
           }
        }
}

}
