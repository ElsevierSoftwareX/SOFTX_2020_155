
package CDS::Parser;
use Exporter;
@ISA = ('Exporter');

sub parse() {
while (<::IN>) {
    # Strip out quotes and blank spaces
    tr/\"/ /;
    tr/\</ /;
    tr/\>/ /;
    s/^\s+//;
    s/\s+$//;
    $lcntr ++;
    ($var1,$var2,$var3,$var4) = split(/\s+/,$_);

    # Find the System keyword. This is where real stuff starts
    if(($mySeq == 0) && ($var1 eq "System")){
	print "System found on line $lcntr\n";
	$mySeq = 1;
    }
    if(($mySeq == 1) && ($var1 eq "Name")){
	$::systemName = $var2;
	#print "System Name $::systemName on line $lcntr\n";
	print OUTH "\#ifndef \U$::systemName";
	print OUTH "_H_INCLUDED\n\#define \U$::systemName";
	print OUTH "_H_INCLUDED\n";
	$mySeq = 2;
    }

    # Start searching for blocks (parts) and determine Type
    if(($mySeq == 2) && ($var1 eq "BlockType")){
	if($inSub == 1) {$openBrace ++;}
	$openBlock ++;
	$inBlock = 1;
	$blockType = $var2;
	# Reference blocks are parts defined in separate files.
	# Code now only supports DAC, ADC and IIR Filters
	if($blockType eq "Reference") {$inRef = 1;}
	# Need to find input/outport ports to link parts together
	# and determine processing order.
	if($blockType eq "Inport") {$::partType[$::partCnt] = INPUT;}
	if($blockType eq "Outport") {$::partType[$::partCnt] = OUTPUT;}
	if($blockType eq "Sum") {$::partType[$::partCnt] = SUM;}
	if($blockType eq "Product") {$::partType[$::partCnt] = MULTIPLY;}
	if($blockType eq "Ground") {$::partType[$::partCnt] = GROUND;}
	if($blockType eq "Terminator") {$::partType[$::partCnt] = TERM;}
	if($blockType eq "BusCreator") {$::partType[$::partCnt] = BUSC; $::adcCnt ++;}
	if($blockType eq "BusSelector") {$::partType[$::partCnt] = BUSS;}
	if($blockType eq "UnitDelay") {$::partType[$::partCnt] = DELAY;}
	if($blockType eq "Logic") {$::partType[$::partCnt] = AND;}
	if($blockType eq "Mux") {$::partType[$::partCnt] = MUX;}
	if($blockType eq "Demux") {$::partType[$::partCnt] = DEMUX;}
	# Need to find subsystems, as these contain internal parts and links
	# which need to be "flattened" to connect all parts together
	if($blockType eq "SubSystem") {
		$inSub = 1;
		#print "Start of subsystem ********************\n";
		$inBlock = 0;
		$lookingForName = 1;
		$openBrace = 2;
		$::subSysPartStart[$::subSys] = $::partCnt;
		$openBlock --;
	}
	#print "BlockType $blockType ";
    }
    # Need to get the name of subsystem to annonate names of
    # parts within the subsystem
    if(($lookingForName == 1) && ($var1 eq "Name")){
	$::subSysName[$::subSys] = $var2;
	#print "$::subSysName[$::subSys]\n";
	$lookingForName = 0;
    }

    # Line parts contain link information
    if(($mySeq == 2) && (($var1 eq "Line") || ($var1 eq "Branch"))){
	if($inSub == 1) {$openBrace ++;}
	$inLine = 1;
    }

    # If in a Subsystem block, need to annotate link names with subsystem name
    if($inSub == 1)
    {
	    if(($inLine == 1) && ($var1 eq "SrcBlock")) {
		$conSrc = $::subSysName[$::subSys];
		$conSrc .= "_";
		$conSrc .= $var2;
	    }
	    if(($inLine == 1) && ($var1 eq "SrcPort")) {
		$conSrcPort = $var2;
	    }
	    if(($inLine == 1) && ($var1 eq "DstBlock")) {
		$conDes = $::subSysName[$::subSys];
		$conDes .= "_";
		$conDes .= $var2;
	    }
	    if(($inLine == 1) && ($var1 eq "DstPort")) {
		$conDesPort = $var2;
		$endBranch = 1;
	    }
    }

    # If not in a Subsystem block, use link names directly.
    if($inSub == 0)
    {
	    if(($inLine == 1) && ($var1 eq "SrcBlock")) {
		$conSrc = $var2;
	    }
	    if(($inLine == 1) && ($var1 eq "SrcPort")) {
		$conSrcPort = $var2;
	    }
	    if(($inLine == 1) && ($var1 eq "DstBlock")) {
		$conDes = $var2;
	    }
	    if(($inLine == 1) && ($var1 eq "DstPort")) {
		$conDesPort = $var2;
		$endBranch = 1;
	    }
    }

    # At end of line block, make connection table
    if(($inLine == 1) && ($endBranch == 1) && ($inSub == 1)){
	$inLine = 0;
	$endBranch = 0;
	# print "Connection $conSrc $conSrcPort $conDes $conDesPort\n";
	# Compare block names with names in links
	for($ii=0;$ii<$::partCnt;$ii++)
	{
		# If connection output name corresponds to a part name,
		# annotate part input information
		if($conDes eq $::xpartName[$ii])
		{
			$::partInput[$ii][($conDesPort-1)] = $conSrc;
			$::partInputPort[$ii][($conDesPort-1)] = $conSrcPort-1;
                       	#$::partSysFromx[$ii][($conDesPort-1)] = -1;
			$::partInCnt[$ii] ++;
		}
		# If connection input name corresponds to a part name,
		# annotate part output information
		if($conSrc eq $::xpartName[$ii])
		{
			$::partOutput[$ii][$::partOutCnt[$ii]] = $conDes;
			$::partOutputPort[$ii][$::partOutCnt[$ii]] = $conDesPort-1;
			$::partOutputPortUsed[$ii][$::partOutCnt[$ii]] = $conSrcPort-1;
			$::partOutCnt[$ii] ++;
		}
	}
    }
    if(($inLine == 1) && ($endBranch == 1) && ($inSub == 0)){
	$inLine = 0;
	$endBranch = 0;
	$srcType = 0;
	$desType = 0;
	for($ii=0;$ii<$::subSys;$ii++)
	{
	if($conSrc eq $::subSysName[$ii]) {
		$srcType = 1;
		$subNum = $ii;
	}
	if($conDes eq $::subSysName[$ii]) {
		$desType = 1;
	}
	}
	# print "Connection $conSrc $conSrcPort $conDes $conDesPort\n";
	# Compare block names with names in links
	if(($srcType == 0) && ($desType == 1))
	{
	#print "Looking for $conDes $conDesPort\n";
	for($ii=0;$ii<$::partCnt;$ii++)
	{
		if($conSrc eq $::xpartName[$ii])
		{
			$::partOutput[$ii][$::partOutCnt[$ii]] = $conDes;
			$::partOutputPort[$ii][$::partOutCnt[$ii]] = $conDesPort-1;
			$::partOutputPortUsed[$ii][$::partOutCnt[$ii]] = $conSrcPort-1;
			$::partOutCnt[$ii] ++;
		}
	}
	}
	if(($srcType == 1) && ($desType == 0))
	{
	#print "Looking for $conDes $conDesPort\n";
	for($ii=0;$ii<$::partCnt;$ii++)
	{
		if(($::partType[$ii] eq "OUTPUT") && ($::partOutput[$ii][0] eq $conSrc) && ($::partOutputPort[$ii][0] == $conSrcPort) && ($conMade[$ii] == 0))
		{
			# print "Made OUT connect $::xpartName[$ii] $conDes $conDesPort\n";
			$::partOutput[$ii][0] = $conDes;
			$::partOutputPort[$ii][0] = $conDesPort;
			$conMade[$ii] = 1;
		}
		if($conDes eq $::xpartName[$ii])
		{
			$qq = $conDesPort - 1;
			$::partInput[$ii][$qq] = $conSrc;
			$::partInputPort[$ii][$qq] = $conDesPort - 1;
			# $::partInput[$ii][$::partInCnt[$ii]] = $conSrc;
			# $::partInputPort[$ii][$::partInCnt[$ii]] = $conDesPort - 1;
                       	# $::partSysFromx[$ii][($conDesPort - 1)] = $subNum;
			if(substr($conDes,0,3) eq "Dac")
			{
				$::partOutputPort[$ii][$::partInCnt[$ii]] = $conDesPort - 1;
				#print "$::xpartName[$ii] $::partInCnt[$ii] has link to $::partInput[$ii][$::partInCnt[$ii]] $::partOutputPort[$ii][$::partInCnt[$ii]]\n";
			}
			$::partInCnt[$ii] ++;
		}
	}
	}
	if(($srcType == 1) && ($desType == 1))
	{
	#print "Looking for $conDes $conDesPort\n";
	for($ii=0;$ii<$::partCnt;$ii++)
	{
		if(($::partType[$ii] eq "OUTPUT") && ($::partOutput[$ii][0] eq $conSrc) && ($::partOutputPort[$ii][0] == $conSrcPort)&& ($conMade[$ii] == 0))
		{
	 	#print "Connection 3 $::xpartName[$ii] $ii\n";
		$::partOutput[$ii][0] = $conDes;
		$::partOutputPort[$ii][0] = $conDesPort;
		$conMade[$ii] = 1;
		}
	}
	}
	if(($srcType == 0) && ($desType == 0))
	{
	for($ii=0;$ii<$::partCnt;$ii++)
	{
		# If connection input name corresponds to a part name,
		# annotate part output information
		if($conSrc eq $::xpartName[$ii])
		{
			$::partOutput[$ii][$::partOutCnt[$ii]] = $conDes;
			$::partOutputPort[$ii][$::partOutCnt[$ii]] = $conDesPort-1;
			$::partOutputPortUsed[$ii][$::partOutCnt[$ii]] = $conSrcPort-1;
			$::partOutCnt[$ii] ++;
		}
		# If connection output name corresponds to a part name,
		# annotate part input information
		if(($conDes eq $::xpartName[$ii]) && ($::partType[$ii] ne "BUSS"))
		{
			$qq = $conDesPort-1;
			$::partInput[$ii][$qq] = $conSrc;
			$::partInputPort[$ii][$qq] = $conSrcPort-1;
                       	#$::partSysFromx[$ii][$qq] = -1;
			# $::partInput[$ii][$::partInCnt[$ii]] = $conSrc;
			# $::partInputPort[$ii][$::partInCnt[$ii]] = $conSrcPort-1;
                       	# $::partSysFromx[$ii][$::partInCnt[$ii]] = -1;
			$::partInCnt[$ii] ++;
		}
	}
	}
    }

    # Get block (part) names.
    if(($inBlock == 1) && ($var1 eq "Name") && ($inSub == 0) && ($openBlock == 1)){
	#print "$var2";
	$::partName[$::partCnt] = $var2;
	$::xpartName[$::partCnt] = $var2;
	$::nonSubPart[$::nonSubCnt] = $::partCnt;
	$::nonSubCnt ++;
    }
    #Check if MULT function is really a DIVIDE function. 
    if(($inBlock == 1) && ($var1 eq "Inputs") && ($var2 eq "*\/") && ($::partType[$::partCnt] eq "MULTIPLY")){
	#print "$var2";
	$::partType[$::partCnt] = "DIVIDE";
	#print "Found a DIVIDE with name $::xpartName[$::partCnt]\n";
    }
    if (($inBlock == 1) && ($var1 eq "Inputs") && ($::partType[$::partCnt] eq "SUM")) {
	$::partInputs[$::partCnt] = $var2;
	$::partInputs[$::partCnt] =~ tr/+-//cd; # delete other characters
    }
    # If in a subsystem block, have to annotate block names with subsystem name.
    if(($inBlock == 1) && ($var1 eq "Name") && ($inSub == 1)){
	$val = $::subSysName[$::subSys];
	$val .= "_";
	$val .= $var2;
	#print "$val";
	$::xpartName[$::partCnt] = $val;
	$::partName[$::partCnt] = $var2;
	$::partSubNum[$::partCnt] = $::subSys;
	$::partSubName[$::partCnt] = $::subSysName[$::subSys];
	if($::partType[$::partCnt] eq "OUTPUT")
	{
		$::partOutput[$::partCnt][0] = $::subSysName[$::subSys];
		$::partOutputPort[$::partCnt][0] = 1;
		$::partOutCnt[$::partCnt] ++;
	}
	if($::partType[$::partCnt] eq "INPUT"){
        	$::partInput[$::partCnt][0] = $::subSysName[$::subSys];
        	$::partInputPort[$::partCnt][0] = 1;
		$::partInCnt[$::partCnt] ++;
	}
    }
    if(($inBlock == 1) && ($var1 eq "Port") && ($inSub == 1) && ($::partType[$::partCnt] eq "INPUT")){
	$::partInputPort[$::partCnt][0] = $var2;
	#print "$::xpartName[$::partCnt] is input $::partCnt with $::partInput[$::partCnt][0] $::partInputPort[$::partCnt][0]\n";
    }
    if(($inBlock == 1) && ($var1 eq "Port") && ($inSub == 1) && ($::partType[$::partCnt] eq "OUTPUT")){
	$::partOutputPort[$::partCnt][0] = $var2;
    }
    if(($inBlock == 1) && ($var1 eq "Port") && ($::partType[$::partCnt] eq "BUSS")){
	$openBlock ++;
    }
    if(($inBlock == 1) && ($var1 eq "PortNumber") && ($::partType[$::partCnt] eq "BUSS")){
	$busPort = $var2 - 1;
    }
    if(($inBlock == 1) && ($var1 eq "Name") && ($::partType[$::partCnt] eq "BUSS") && ($busPort >= 0)){
	$::partInput[$::partCnt][$busPort] = $var2;
	# print "BUSS X $::partInCnt[$::partCnt] Y $busPort $::partInput[$::partCnt][$busPort]\n";
	$::partInCnt[$::partCnt] ++;
	# print "BUSS X $::partCnt $::partInCnt[$::partCnt]\n";
	$busPort = -1;
    }

    # Presently, 4 types of Reference blocks are supported.
    # Info on the part type is in the SourceBlock field.
    if(($inBlock == 1) && ($inRef == 1 ) && ($var1 eq "SourceBlock")){
	$partErr = 1;

	($r) = $var2 =~ m%(^[^/]+)/.*%g;
	#print "CDS part: ", $r, "\n";

	#
	# These names need a thorough cleanup !!!
	#
	# START part name transformation code
	if ($r =~ /^cdsSwitch/ || $r eq "cdsSusSw2") { $r = "MultiSwitch"; }
	elsif ($r =~ /^Matrix/) { $r = "Matrix"; }
	elsif ($r =~ /^cdsSubtract/) { $r = "DiffJunc"; }
	elsif ($r eq "dsparch4" ) { $r = "Filt"; }
	elsif ($r eq "cdsFmodule" ) { $r = "Filt"; }
	elsif ($r eq "cdsWD" ) { $r = "Wd"; }
	elsif ($r eq "cdsSusWd" ) { $r = "SusWd"; }
	elsif ($r eq "cdsSWD1" ) { $r = "SeiWd"; }
	elsif ($r eq "cdsPPFIR" ) { $r = "FirFilt"; }
	elsif ($r =~ /^cds/) {
		# Getting rid of the leading "cds"
		($r) = $r =~ m/^cds(.+)$/;
	} elsif ($r =~ /^SIMULINK/i) { next; }

	# Capitalize first character
	$r = uc(substr($r,0, 1)) . substr($r, 1);
	# END part name transformation code
	   
	require "lib/$r.pm";
	$::partType[$::partCnt] = ("CDS::" . $r . "::partType") -> ();
	$::cdsPart[$::partCnt] = 1;
	$partErr = 0;

	if ($partErr)
	{
		print "Unknow part type $var2\nExiting script\n";
		exit;
	}

    }

    # End of blocks/lines are denoted by \} in .mdl file.
    if(($mySeq == 2) && ($var1 eq "}")){
	if($inSub == 1) {$openBrace --;}
	if($inBlock) {$openBlock --;}
	$ob ++;
	$inRef = 0;
    	if(($inBlock == 1) && ($openBlock == 0)){
	#print " $::partCnt\n";
	$::partCnt ++;
	$inBlock = 0;
	}
    }
    if(($mySeq == 2) && ($var1 ne "}")){
	$ob = 0;
    }
    if(($inSub == 1) && ($openBrace == 0))
    {
	#print "Think end of subsys is at $lcntr\n";
    }

    #Look for end of subsystem 
    if(($mySeq == 2) && ($openBrace == 0) && ($inSub == 1)){
	$ob = 0;
	$inSub = 0;
	$::subSysPartStop[$::subSys] = $::partCnt;
	$::subSys ++;
	#print "Think end of subsys $::subSys is at $lcntr\n";
	#print "End of Subsystem *********** $::subSys\n";
	$openBlock = 0;
    }
    # print "$var1\n";
}
return 1;
}
