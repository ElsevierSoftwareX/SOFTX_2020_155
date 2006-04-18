#!/usr/bin/perl

die "Usage: $PROGRAM_NAME <MDL file> <Output file name> <DCUID number>"
        #if (@ARGV != 1 && @ARGV != 2 && @ARGV != 3);
        if (@ARGV != 3);

$skeleton = $ARGV[1];
print "file out is $skeleton\n";
$cFile = "../../fe/";
$cFile .= $ARGV[1];
$cFile .= "/";
$cFile .= $ARGV[1];
$cFile .= ".c";
$hFile = "../../include/";
$hFile .= $ARGV[1];
$hFile .= ".h";
$mFile = "../../fe/";
$mFile .= $ARGV[1];
$mFile .= "/";
$mFile .= "Makefile";
$meFile = "../../../config/";
$meFile .= "Makefile\.";
$meFile .= $ARGV[1];
$meFile .= epics;
$dcuId = $ARGV[2];
print "DCUID = $dcuId\n";
if($dcuId < 16) {$rate = 60;}
if($dcuId > 16) {$rate = 480;}
if (@ARGV == 2) { $skeleton = $ARGV[1]; }
open(EPICS,">../fmseq/".$ARGV[1]) || die "cannot open output file for writing";
open(OUT,">./".$cFile) || die "cannot open c file for writing";
open(OUTM,">./".$mFile) || die "cannot open Makefile file for writing";
open(OUTME,">./".$meFile) || die "cannot open EPICS Makefile file for writing";
open(OUTH,">./".$hFile) || die "cannot open header file for writing";
open(IN,"<../simLink/".$ARGV[0]) || die "cannot open mdl file $ARGV[0]\n";
$diag = "diags\.txt";
open(OUTD,">./".$diag) || die "cannot open diag file for writing";

$mySeq = 0;
$connects = 0;
$ob = 0;
$subSys = 0;
$partCnt = 0;
$inBranch = 0;
$endBranch = 0;
$adcCnt = 0;
$dacCnt = 0;
$filtCnt = 0;
$firCnt = 0;

# Clear the part input and output counters
for($ii=0;$ii<300;$ii++)
{
	$partInCnt[$ii] = 0;
	$partOutCnt[$ii] = 0;
	$partInUsed[$ii] = 0;
	$conMade[$ii] = 0;
}
$openBrace = 0;
$openBlock = 0;
$nonSubCnt = 0;

# Read .mdl input file
while (<IN>) {
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
	# print "System found on line $lcntr\n";
	$mySeq = 1;
    }
    if(($mySeq == 1) && ($var1 eq "Name")){
	$systemName = $var2;
	#print "System Name $systemName on line $lcntr\n";
	print OUTH "\#ifndef \U$systemName";
	print OUTH "_H_INCLUDED\n\#define \U$systemName";
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
	if($blockType eq "Inport") {$partType[$partCnt] = INPUT;}
	if($blockType eq "Outport") {$partType[$partCnt] = OUTPUT;}
	if($blockType eq "Sum") {$partType[$partCnt] = SUM;}
	if($blockType eq "Ground") {$partType[$partCnt] = GROUND;}
	if($blockType eq "Terminator") {$partType[$partCnt] = TERM;}
	if($blockType eq "BusCreator") {$partType[$partCnt] = BUSC; $adcCnt ++;}
	if($blockType eq "BusSelector") {$partType[$partCnt] = BUSS;}
	# Need to find subsystems, as these contain internal parts and links
	# which need to be "flattened" to connect all parts together
	if($blockType eq "SubSystem") {
		$inSub = 1;
		#print "Start of subsystem ********************\n";
		$inBlock = 0;
		$lookingForName = 1;
		$openBrace = 2;
		$subSysPartStart[$subSys] = $partCnt;
		$openBlock --;
	}
	#print "BlockType $blockType ";
    }
    # Need to get the name of subsystem to annonate names of
    # parts within the subsystem
    if(($lookingForName == 1) && ($var1 eq "Name")){
	$subSysName[$subSys] = $var2;
	#print "$subSysName[$subSys]\n";
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
		$conSrc = $subSysName[$subSys];
		$conSrc .= "_";
		$conSrc .= $var2;
	    }
	    if(($inLine == 1) && ($var1 eq "SrcPort")) {
		$conSrcPort = $var2;
	    }
	    if(($inLine == 1) && ($var1 eq "DstBlock")) {
		$conDes = $subSysName[$subSys];
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
	for($ii=0;$ii<$partCnt;$ii++)
	{
		# If connection output name corresponds to a part name,
		# annotate part input information
		if($conDes eq $xpartName[$ii])
		{
			$partInput[$ii][($conDesPort-1)] = $conSrc;
			$partInputPort[$ii][($conDesPort-1)] = $conSrcPort-1;
			$partInCnt[$ii] ++;
		}
		# If connection input name corresponds to a part name,
		# annotate part output information
		if($conSrc eq $xpartName[$ii])
		{
			$partOutput[$ii][$partOutCnt[$ii]] = $conDes;
			$partOutputPort[$ii][$partOutCnt[$ii]] = $conDesPort-1;
			$partOutputPortUsed[$ii][$partOutCnt[$ii]] = $conSrcPort-1;
			$partOutCnt[$ii] ++;
		}
	}
    }
    if(($inLine == 1) && ($endBranch == 1) && ($inSub == 0)){
	$inLine = 0;
	$endBranch = 0;
	$srcType = 0;
	$desType = 0;
	for($ii=0;$ii<$subSys;$ii++)
	{
	if($conSrc eq $subSysName[$ii]) {
		$srcType = 1;
	}
	if($conDes eq $subSysName[$ii]) {
		$desType = 1;
	}
	}
	# print "Connection $conSrc $conSrcPort $conDes $conDesPort\n";
	# Compare block names with names in links
	if(($srcType == 0) && ($desType == 1))
	{
	#print "Looking for $conDes $conDesPort\n";
	for($ii=0;$ii<$partCnt;$ii++)
	{
		if($conSrc eq $xpartName[$ii])
		{
			$partOutput[$ii][$partOutCnt[$ii]] = $conDes;
			$partOutputPort[$ii][$partOutCnt[$ii]] = $conDesPort-1;
			$partOutputPortUsed[$ii][$partOutCnt[$ii]] = $conSrcPort-1;
			$partOutCnt[$ii] ++;
		}
	}
	}
	if(($srcType == 1) && ($desType == 0))
	{
	#print "Looking for $conDes $conDesPort\n";
	for($ii=0;$ii<$partCnt;$ii++)
	{
		if(($partType[$ii] eq "OUTPUT") && ($partOutput[$ii][0] eq $conSrc) && ($partOutputPort[$ii][0] == $conSrcPort) && ($conMade[$ii] == 0))
		{
	 	#print "Connection $xpartName[$ii] $ii\n";
		$partOutput[$ii][0] = $conDes;
		$partOutputPort[$ii][0] = $conDesPort;
		$conMade[$ii] = 1;
		}
		if($conDes eq $xpartName[$ii])
		{
			$partInput[$ii][$partInCnt[$ii]] = $conSrc;
			$partInputPort[$ii][$partInCnt[$ii]] = $conDesPort-1;
			$partInCnt[$ii] ++;
		}
	}
	}
	if(($srcType == 1) && ($desType == 1))
	{
	#print "Looking for $conDes $conDesPort\n";
	for($ii=0;$ii<$partCnt;$ii++)
	{
		if(($partType[$ii] eq "OUTPUT") && ($partOutput[$ii][0] eq $conSrc) && ($partOutputPort[$ii][0] == $conSrcPort)&& ($conMade[$ii] == 0))
		{
	 	#print "Connection 3 $xpartName[$ii] $ii\n";
		$partOutput[$ii][0] = $conDes;
		$partOutputPort[$ii][0] = $conDesPort;
		$conMade[$ii] = 1;
		}
	}
	}
	if(($srcType == 0) && ($desType == 0))
	{
	for($ii=0;$ii<$partCnt;$ii++)
	{
		# If connection input name corresponds to a part name,
		# annotate part output information
		if($conSrc eq $xpartName[$ii])
		{
			$partOutput[$ii][$partOutCnt[$ii]] = $conDes;
			$partOutputPort[$ii][$partOutCnt[$ii]] = $conDesPort-1;
			$partOutputPortUsed[$ii][$partOutCnt[$ii]] = $conSrcPort-1;
			$partOutCnt[$ii] ++;
		}
		# If connection output name corresponds to a part name,
		# annotate part input information
		if($conDes eq $xpartName[$ii])
		{
			$partInput[$ii][$partInCnt[$ii]] = $conSrc;
			$partInputPort[$ii][$partInCnt[$ii]] = $conSrcPort-1;
			if(substr($conDes,0,3) eq "DAC")
			{
				$partOutputPort[$ii][$partInCnt[$ii]] = $conDesPort-1;
				#print "$xpartName[$ii] $partInCnt[$ii] has link to $partInput[$ii][$partInCnt[$ii]] $partOutputPort[$ii][$partInCnt[$ii]]\n";
			}
			$partInCnt[$ii] ++;
		}
	}
	}
    }

    # Get block (part) names.
    if(($inBlock == 1) && ($var1 eq "Name") && ($inSub == 0) && ($openBlock == 1)){
	#print "$var2";
	$partName[$partCnt] = $var2;
	$xpartName[$partCnt] = $var2;
	$nonSubPart[$nonSubCnt] = $partCnt;
	$nonSubCnt ++;
    }
    # If in a subsystem block, have to annotate block names with subsystem name.
    if(($inBlock == 1) && ($var1 eq "Name") && ($inSub == 1)){
	$val = $subSysName[$subSys];
	$val .= "_";
	$val .= $var2;
	#print "$val";
	$xpartName[$partCnt] = $val;
	$partName[$partCnt] = $var2;
	$partSubNum[$partCnt] = $subSys;
	$partSubName[$partCnt] = $subSysName[$subSys];
	if($partType[$partCnt] eq "OUTPUT")
	{
		$partPort[$partCnt] = 1;
		$partOutput[$partCnt][0] = $subSysName[$subSys];
		$partOutputPort[$partCnt][0] = 1;
		$partOutCnt[$partCnt] ++;
	}
    }
    if(($inBlock == 1) && ($var1 eq "Port") && ($inSub == 1) && ($partType[$partCnt] eq "INPUT")){
	$partPort[$partCnt] = $var2;
	$partInput[$partCnt][0] = $subSysName[$subSys];
	$partInputPort[$partCnt][0] = $var2;
	#print "$xpartName[$partCnt] is input $partCnt with $partInput[$partCnt][0] $partInputPort[$partCnt][0]\n";
	$partInCnt[$partCnt] ++;
    }
    if(($inBlock == 1) && ($var1 eq "Port") && ($inSub == 1) && ($partType[$partCnt] eq "OUTPUT")){
	$partPort[$partCnt] = $var2;
	$partOutputPort[$partCnt][0] = $var2;
    }
    if(($inBlock == 1) && ($var1 eq "Port") && ($partType[$partCnt] eq "BUSS")){
	$openBlock ++;
    }
    if(($inBlock == 1) && ($var1 eq "PortNumber") && ($partType[$partCnt] eq "BUSS")){
	$busPort = $var2 - 1;
    }
    if(($inBlock == 1) && ($var1 eq "Name") && ($partType[$partCnt] eq "BUSS")){
	$partInput[$partCnt][$busPort] = $var2;
	$partInCnt[$partCnt] ++;
    }

    # Presently, 4 types of Reference blocks are supported.
    # Info on the part type is in the SourceBlock field.
    if(($inBlock == 1) && ($inRef == 1 ) && ($var1 eq "SourceBlock")){
	$partErr = 1;
	if (substr($var2,0,9) eq "cdsSwitch") {
		$partType[$partCnt] = MULTI_SW;
		#print "$partCnt is type MULTI_SW\n";
		$partErr = 0;
	}
	if (substr($var2,0,13) eq "cdsRampSwitch") {
		$partType[$partCnt] = RAMP_SW;
		# print "$partCnt is type RAMP_SW\n";
		$partErr = 0;
	}
	if (substr($var2,0,11) eq "cdsSubtract") {
		$partType[$partCnt] = DIFF_JUNC;
		#print "$partCnt is type DIFF_JUNCT\n";
		$partErr = 0;
	}
	if (substr($var2,0,10) eq "cdsProduct") {
		$partType[$partCnt] = PRODUCT;
		#print "$partCnt is type PRODUCT\n";
		$partErr = 0;
	}
	if (substr($var2,0,10) eq "cdsEpicsIn") {
		$partType[$partCnt] = EPICS_INPUT;
		$partErr = 0;
	}
	if (substr($var2,0,11) eq "cdsEpicsOut") {
		$partType[$partCnt] = EPICS_OUTPUT;
		$partErr = 0;
	}
	if (substr($var2,0,8) eq "cdsPPFIR") {
		$partType[$partCnt] = FIR_FILTER;
		#print "$partCnt is type FIR\n";
		$firName = $xpartName[$partCnt];
		print OUTH "#define $firName \t $filtCnt\n";
		print EPICS "$firName\n";
		$filtCnt ++;
		$firName = $xpartName[$partCnt];
		$firName .= _DF;
		print OUTH "#define $firName \t $filtCnt\n";
		print EPICS "$firName\n";
		$filtCnt ++;
		$firName = $xpartName[$partCnt];
		$firName .= _UF;
		print OUTH "#define $firName \t $filtCnt\n";
		print EPICS "$firName\n";
		$filtCnt ++;
		$firName = $xpartName[$partCnt];
		$firName .= _CF;
		print OUTH "#define $firName \t $filtCnt\n";
		print EPICS "$firName\n";
		$filtCnt ++;
		$firName = $xpartName[$partCnt];
		$firName .= _FIR;
		print OUTH "#define $firName \t $firCnt\n";
		$firCnt ++;
		$partErr = 0;
	}
	if (substr($var2,0,3) eq "adc") {
		$partType[$partCnt] = ADC;
		$adcPartNum[$adcCnt] = $partCnt;
		$adcCnt ++;
		$partUsed[$partCnt] = 1;
		$partErr = 0;
	}
	if (substr($var2,0,3) eq "dac") {
		$partType[$partCnt] = DAC;
		$dacPartNum[$dacCnt] = $partCnt;
		$dacCnt ++;
		$partErr = 0;
	}
	if (substr($var2,0,6) eq "Matrix") {
		$partType[$partCnt] = MATRIX;
		$partErr = 0;
	}
	if (substr($var2,0,8) eq "dsparch4") {
		$partType[$partCnt] = FILT;
		print OUTH "#define $xpartName[$partCnt] \t $filtCnt\n";
		print EPICS "$xpartName[$partCnt]\n";
		$filtCnt ++;
		$partErr = 0;
	}
	if ($partErr)
	{
		print "Unknow part type $var2\nExiting script\n";
		exit;
	}

    }

    # End of blocks/lines are denoted by } in .mdl file.
    if(($mySeq == 2) && ($var1 eq "}")){
	if($inSub == 1) {$openBrace --;}
	if($inBlock) {$openBlock --;}
	$ob ++;
	$inRef = 0;
    	if(($inBlock == 1) && ($openBlock == 0)){
	#print " $partCnt\n";
	$partCnt ++;
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
	$subSysPartStop[$subSys] = $partCnt;
	$subSys ++;
	#print "Think end of subsys $subSys is at $lcntr\n";
	#print "End of Subsystem *********** $subSys\n";
	$openBlock = 0;
    }
    # print "$var1\n";
}
# Done reading the input file. ********************************************
# Done reading the input file. ********************************************
#Lookup all connection names and matrix to part numbers
for($ii=0;$ii<$partCnt;$ii++)
{
	for($jj=0;$jj<$partOutCnt[$ii];$jj++)
	{
	   for($kk=0;$kk<$partCnt;$kk++)
	   {
		if($partOutput[$ii][$jj] eq $xpartName[$kk])
		{
			$partOutNum[$ii][$jj] = $kk;
			$partOutputType[$ii][$jj] = $partType[$kk];
		}
	   }
	}
}
for($ii=0;$ii<$partCnt;$ii++)
{
	if($partType[$ii] eq "BUSS") 
	{
		$partInCnt[$ii] -= 2;
	}
	for($jj=0;$jj<$partInCnt[$ii];$jj++)
	{
	   for($kk=0;$kk<$partCnt;$kk++)
	   {
		if($partInput[$ii][$jj] eq $xpartName[$kk])
		{
			$partInNum[$ii][$jj] = $kk;
			$partInputType[$ii][$jj] = $partType[$kk];
		}
	   }
	}
}
for($ii=0;$ii<$partCnt;$ii++)
{
$foundCon = 0;
	if($partType[$ii] eq "OUTPUT")
	{
	for($jj=0;$jj<$partOutCnt[$ii];$jj++)
	{
	   for($kk=0;$kk<$partCnt;$kk++)
	   {
		if($partType[$kk] eq "INPUT")
		{
	      for($ll=0;$ll<$partInCnt[$kk];$ll++)
        	{
		if(($partOutput[$ii][$jj] eq $partInput[$kk][$ll]) && ($partOutputPort[$ii][$jj] == $partInputPort[$kk][$ll]))
		{
			#$partOutput[$ii][$jj] = $xpartName[$kk];
			#$partOutNum[$ii][$jj] = $kk;
			$partSysFrom[$kk] = $partSubNum[$ii];
			# print "Connection from $xpartName[$ii] $fromNum $fromPort to $xpartName[$kk] $toNum $toPort\n";
			#print"\t$xpartName[$fromNum] $fromPort to $xpartName[$toNum] $toPort\n";
                       $fromNum = $partInNum[$ii][$jj];
                       $fromPort = $partInputPort[$ii][$jj];
                       $toNum = $partOutNum[$kk][$ll];
                       $toPort = $partOutputPort[$kk][$ll];
                       $partOutput[$fromNum][$fromPort] = $xpartName[$toNum];
                       $partOutputType[$fromNum][$fromPort] = $partType[$toNum];
                       $partOutNum[$fromNum][$fromPort] = $toNum;
                       $partOutputPort[$fromNum][$fromPort] = $toPort;
                       $partInput[$toNum][$toPort] = $xpartName[$fromNum];
                       $partInputType[$toNum][$toPort] = $partType[$fromNum];
                       $partInNum[$toNum][$toPort] = $fromNum;
                       $partInputPort[$toNum][$toPort] = $fromPort;
                       $partSysFrom[$kk] = $partSubNum[$ii];
                       $partInputType[$kk][0] = "SUBSYS";
                       $partInNum[$kk][0] = $partSubNum[$ii];

			$foundCon = 1;
		}
		}
		}
	   }
	}
	if($foundCon == 0){
	for($jj=0;$jj<$partOutCnt[$ii];$jj++)
	{
	   for($kk=0;$kk<$nonSubCnt;$kk++)
	   {
	      $xx = $nonSubPart[$kk];
			if($partOutput[$ii][$jj] eq $partName[$xx])
			{
			#print "\t Maybe $xpartName[$xx] port $partOutputPort[$ii][$jj]\n";
				$fromNum = $partInNum[$ii][$jj];
				$fromPort = $partInputPort[$ii][$jj];
				$partOutput[$fromNum][$fromPort] = $xpartName[$xx];
				$partOutputType[$fromNum][$fromPort] = $partType[$xx];
				$partOutNum[$fromNum][$fromPort] = $xx;
				$partOutputPort[$fromNum][$fromPort] = $partOutputPort[$ii][$jj];
                       		$partSysFromx[$xx][$fromCnt[$xx]] = $partSubNum[$ii];
				$fromCnt[$xx] ++;
				$foundCon = 1;
			}
	   }
	}

	}
	if($foundCon == 0){
	print "No connect for $xpartName[$ii]\n";
	}
	}
}
for($ii=0;$ii<$nonSubCnt;$ii++)
{
	$xx = $nonSubPart[$ii];
	if($partType[$xx] eq "BUSS")
	{
		for($jj=0;$jj<$partOutCnt[$xx];$jj++)
		{
		   $mm = $partOutputPort[$xx][$jj]+1;
		   for($kk=0;$kk<$partCnt;$kk++)
		   {
			if($partType[$kk] eq "INPUT")
			{
				if(($partOutput[$xx][$jj] eq $partInput[$kk][0]) && ($mm == $partInputPort[$kk][0]))
				{
				$fromNum = $xx;
				$fromPort = $partOutputPortUsed[$xx][$jj];
				# $fromPort = $partInputPort[$kk][0];
				$partInput[$kk][0] = $xpartName[$xx];
				$partInputType[$kk][0] = "ADC";
				$partInNum[$kk][0] = $xx;
				#$partInputPort[$kk][0] = $fromPort;
				$partSysFrom[$kk] = 100 + $xx;
				#print "Found ADC INPUT connect from $xpartName[$xx] to $xpartName[$kk] $partOutputPort[$xx][$jj] $jj  input $partInputPort[$kk][0]\n";
				for($ll=0;$ll<$partOutCnt[$kk];$ll++)
				{
					$adcName = $partInput[$xx][$fromPort];
					$adcNum = substr($adcName,4,1);
					$adcChan = substr($adcName,6,2);
					$toNum = $partOutNum[$kk][$ll];
				        $toPort = $partOutputPort[$kk][$ll];
					$partInNum[$toNum][$toPort] = $adcNum;
					#$partInput[$toNum][$toPort] = $xpartName[$xx];
					$partInput[$toNum][$toPort] = $adcName;
					$partInputPort[$toNum][$toPort] = $adcChan;
					$partInputType[$toNum][$toPort] = "ADC";
					#print "\tNew adc connect $xpartName[$toNum] $adcNum $adcChan to partnum $partInput[$toNum][$toPort]\n";
				}
				}
			}
			if($partType[$kk] ne "INPUT")
			{
				if(($partOutput[$xx][$jj] eq $xpartName[$kk]))
				{
				#print "Found ADC NP connect to $xpartName[$kk] $partOutputPort[$xx][$jj]\n";
				$fromNum = $xx;
				#$fromPort = $jj;
				$fromPort = $partInputPort[$kk][0];
				#$fromPort = $partOutputPort[$xx][$jj];
				$partInput[$kk][0] = $xpartName[$xx];
				$partInputType[$kk][0] = "ADC";
				$partInNum[$kk][0] = $xx;
				$partInputPort[$kk][0] = $fromPort;
					$adcName = $partInput[$xx][$fromPort];
					$adcNum = substr($adcName,4,1);
					$adcChan = substr($adcName,6,2);
					$partInput[$kk][0] = $adcName;
					$partInputPort[$kk][0] = $adcChan;
					$partInNum[$kk][0] = $adcNum;
				}
			}
		   }
		}
	}
	if($partType[$xx] ne "BUSS")
	{
		for($jj=0;$jj<$partOutCnt[$xx];$jj++)
		{
		   $mm = $partOutputPort[$xx][$jj]+1;
		   for($kk=0;$kk<$partCnt;$kk++)
		   {
			if($partType[$kk] eq "INPUT")
			{
				if(($partOutput[$xx][$jj] eq $partInput[$kk][0]) && ($mm == $partInputPort[$kk][0]))
				{
                       $partInputType[$kk][0] = "PART";
                       $partInNum[$kk][0] = $xx;
				
				for($ll=0;$ll<$partOutCnt[$kk];$ll++)
				{
					$toNum = $partOutNum[$kk][$ll];
					$toPort = $partOutputPort[$kk][$ll];
				#print "Found nonADC connect from $xpartName[$xx] port $mm to $xpartName[$toNum] $partInputPort[$xx][$jj]\n";
					$partInNum[$toNum][$toPort] = $xx;
					$partInput[$toNum][$toPort] = $xpartName[$xx];
					$partInputPort[$toNum][$toPort] = $mm;
					$partInputType[$toNum][$toPort] = $partType[$mm];

					$partOutput[$xx][$jj] = $xpartName[$toNum];
					$partOutputPort[$xx][$jj] = $toPort;
					$partOutNum[$xx][$jj] = $toNum;
					$partOutputType[$xx][$jj] = $partType[$toNum];
				}
				$partSysFrom[$kk] = 100 + $xx;
				}
			}
		   }
		}
	}
}


#DIAGNOSTIC
print "Found $subSys subsystems\n";
for($ll=0;$ll<$subSys;$ll++)
{
$xx = $subSysPartStop[$ll] - $subSysPartStart[$ll];
$subCntr[$ll] = 0;
	#print "SubSys $ll $subSysName[$ll] from $subSysPartStart[$ll] to $subSysPartStop[$ll]\n";
print OUTD "\nSubSystem $ll has $xx parts ************************************\n";
for($ii=$subSysPartStart[$ll];$ii<$subSysPartStop[$ll];$ii++)
{
	print OUTD "Part $ii $xpartName[$ii] is type $partType[$ii] with $partInCnt[$ii] inputs and $partOutCnt[$ii] outputs\n";
	print OUTD "INS FROM:\n";
	for($jj=0;$jj<$partInCnt[$ii];$jj++)
	{
                        #$from = $partInNum[$ii][$jj];
		print OUTD "\t$partInput[$ii][$jj]\t$partInputType[$ii][$jj]\t$partInNum[$ii][$jj]\t$partInputPort[$ii][$jj]\n";
		if($partType[$ii] eq "INPUT")
		{
			print OUTD "From Subsystem $partSysFrom[$ii]\n";
			$subInputs[$ll][$subCntr[$ll]] = $partSysFrom[$ii];
			$subInputsType[$ll][$subCntr[$ll]] = $partInputType[$ii][$jj];
			$subCntr[$ll] ++;
		}
	}
	print OUTD "OUT TO:\n";
	print OUTD "\tPart Name\tType\tNum\tPort\tPort Used\n";
	for($jj=0;$jj<$partOutCnt[$ii];$jj++)
	{
		#$to = $partOutNum[$ii][$jj];
		print OUTD "\t$partOutput[$ii][$jj]\t$partOutputType[$ii][$jj]\t$partOutNum[$ii][$jj]\t$partOutputPort[$ii][$jj]\t$partOutputPortUsed[$ii][$jj]\n";
	}
	print OUTD "\n****************************************************************\n";
}
}
print OUTD "Non sub parts ************************************\n";
for($ii=0;$ii<$nonSubCnt;$ii++)
{
	$xx = $nonSubPart[$ii];
	print OUTD "Part $xx $xpartName[$xx] is type $partType[$xx] with $partInCnt[$xx] inputs and $partOutCnt[$xx] outputs\n";
	print OUTD "INS FROM:\n";
	for($jj=0;$jj<$partInCnt[$xx];$jj++)
	{
                        #$from = $partInNum[$xx][0];
		print OUTD "\t$partInput[$xx][$jj]\t$partInputType[$xx][$jj]\t$partInNum[$xx][$jj]\t$partInputPort[$xx][$jj] subsys $partSysFromx[$xx][$jj]\n";
	}
	print OUTD "OUT TO:\n";
	for($jj=0;$jj<$partOutCnt[$xx];$jj++)
	{
		$to = $partOutNum[$xx][$jj];
		print OUTD "\t$partOutput[$xx][$jj]\t$partOutputType[$xx][$jj]\t$partOutNum[$xx][$jj]\t$partOutputPort[$xx][$jj]\t$partOutputPortUsed[$xx][$jj]\n";
#		print OUTD "\t$partOutput[$xx][$jj]\t$partOutputPort[$xx][$jj] type $partType[$to] $partOutNum[$xx][$jj]\n";
	}
	print OUTD "\n****************************************************************\n";
}
for($ii=0;$ii<$subSys;$ii++)
{
	print OUTD "\nSUBS $ii $subSysName[$ii] *******************************\n";

for($ll=0;$ll<$subCntr[$ii];$ll++)
{
	print OUTD "$ll $subInputs[$ii][$ll] $subInputsType[$ii][$ll]\n";
}
}
close(OUTD);
print "Found $adcCnt ADC modules part is $adcPartNum[0]\n";
print "Found $dacCnt DAC modules part is $dacPartNum[0]\n";


for($ii=0;$ii<$subSys;$ii++)
{
	$partsRemaining = $subSysPartStop[$ii] - $subSysPartStart[$ii];
	$counter = 0;
	$ssCnt = 0;
	for($jj=$subSysPartStart[$ii];$jj<$subSysPartStop[$ii];$jj++)
	{
		if(($partType[$jj] eq "INPUT") || ($partType[$jj] eq "GROUND") || ($partType[$jj] eq "EPICS_INPUT"))
		{
			$partsRemaining --;
			$partUsed[$jj] = 1;
			for($kk=0;$kk<$partOutCnt[$jj];$kk++)
			{
				$ll = $partOutNum[$jj][$kk];
				$seqNum[0][$counter] = $ll;
				$counter ++;
			}
		}
		if(($partType[$jj] eq "OUTPUT") || ($partType[$jj] eq "TERM") || ($partType[$jj] eq "BUSC") || ($partType[$jj] eq "BUSS"))
		{
			$partsRemaining --;
			$partUsed[$jj] = 1;
		}
	}
	#print "Found $counter Inputs for subsystem $ii *********************************\n";
	$xx = 0;
	$ts = 1;
	until(($partsRemaining < 1) || ($xx > 20))
	{
		$xx ++;
		$loop = $counter ++;
		$counter = 0;
		if($ts == 1) {
			$ts = 0;
			$ns = 1;
		}
		else {
			$ts = 1;
			$ns = 0;
		}
		for($jj=0;$jj<$loop;$jj++)
		{
			$mm = $seqNum[$ts][$jj];
			$partInUsed[$mm] ++;
			if(($partInUsed[$mm] >= $partInCnt[$mm]) && ($partUsed[$mm] == 0))
			{
				$partUsed[$mm] = 1;
				$partsRemaining --;
				$seq[$ii][$ssCnt] = $mm;
				$seqName[$ii][$ssCnt] = $xpartName[$mm];
				#print "Sub $ii part $ssCnt = $mm $xpartName[$mm]\n";
				$ssCnt ++;
				for($kk=0;$kk<$partOutCnt[$mm];$kk++)
				{
					$ll = $partOutNum[$mm][$kk];
					if(($ll >= $subSysPartStart[$ii]) && ($ll < $subSysPartStop[$ii]))
					{
						$seqNum[$ns][$counter] = $ll;
						$counter ++;
					}
				}
			}
		}

	}
	#print "Parts remaining = $partsRemaining\n";
	$seqParts[$ii] = $ssCnt;
    
}

$partsRemaining = 0;
$searchCnt = 0;
for($ii=0;$ii<$nonSubCnt;$ii++)
{
      	$xx = $nonSubPart[$ii];
	if(($partType[$xx] ne "BUSC") && ($partType[$xx] ne "DAC") && ($partType[$xx] ne "BUSS"))
	{
		$searchPart[$partsRemaining] = $xx;
		$searchCnt ++;
		$partsRemaining ++;
		#print "Part num $xx $partName[$xx] is remaining\n";
	}
}
$subRemaining = $subSys;
$seqCnt = 0;
for($ii=0;$ii<$subSys;$ii++)
{
$allADC = 1;
	for($jj=0;$jj<$subCntr[$ii];$jj++)
	{
		if($subInputsType[$ii][$jj] ne "ADC")
		{
			$allADC = 0;
		}
	}
	if($allADC == 1) 
	{
		$subUsed[$ii] = 1;
		$seqList[$seqCnt] = $ii;
		$seqType[$seqCnt] = "SUBSYS";
		$seqCnt ++;
		#print "Subsys $ii $subSysName[$ii] has all ADC inputs and can go $seqCnt\n";
		$subRemaining --;
	}
}
for($ii=0;$ii<$searchCnt;$ii++)
{
	$allADC = 1;
	$xx = $searchPart[$ii];
	if($partUsed[$xx] == 0)
	{
		for($jj=0;$jj<$partInCnt[$xx];$jj++)
		{
			if($partInputType[$jj] ne "ADC")

			{
					$allADC = 0;
			}
		}
		if($allADC == 1) {
			#print "Part $xx $xpartName[$xx] can go next\n";
			$partUsed[$xx] = 1;
			$partsRemaining --;
			$seqList[$seqCnt] = $xx;
			$seqType[$seqCnt] = "PART";
			$seqCnt ++;
		}
	}
}
until(($partsRemaining < 1) && ($subRemaining < 1))
{
	for($ii=0;$ii<$subSys;$ii++)
	{
		$allADC = 1;
		if($subUsed[$ii] == 0)
		{
			for($jj=0;$jj<$subCntr[$ii];$jj++)
			{
				$yy = $subInputs[$ii][$jj];
				if(($yy<100) && ($subUsed[$yy] != 1))
				{
					$allADC = 0;
						# print "FallIn $ii\n";
				}
				$yy = $subInputs[$ii][$jj] - 100;
				for($kk=0;$kk<$searchCnt;$kk++)
				{
					if(($partUsed[$yy] != 1) && ($subInputsType[$ii][$jj] ne "ADC"))
					{
						#print "Fallout $ii\n";
						$allADC = 0;
					}
				}
			}
			if($allADC == 1) {
				#print "Subsys $ii $subSysName[$ii] can go next\n";
				$subUsed[$ii] = 1;
				$subRemaining --;
				$seqList[$seqCnt] = $ii;
				$seqType[$seqCnt] = "SUBSYS";
				$seqCnt ++;
			}
		}
	}
	for($ii=0;$ii<$searchCnt;$ii++)
	{
		$allADC = 1;
		$xx = $searchPart[$ii];
		if($partUsed[$xx] == 0)
		{
			for($jj=0;$jj<$partInCnt[$xx];$jj++)
			{
				$yy = $partSysFromx[$xx][$jj];
				if(($subUsed[$yy] != 1) && ($partInputType[$xx][$jj] ne "ADC"))
				{
						$allADC = 0;
				}
			}
			if($allADC == 1) {
				#print "Part $xx $xpartName[$xx] can go next\n";
				$partUsed[$xx] = 1;
				$partsRemaining --;
				$seqList[$seqCnt] = $xx;
				$seqType[$seqCnt] = "PART";
				$seqCnt ++;
			}
		}
	}
}
$processCnt = 0;
$processSeqCnt = 0;
for($ii=0;$ii<$seqCnt;$ii++)
{
	print "$ii $seqList[$ii] $seqType[$ii] $seqParts[$seqList[$ii]]\n";
	if($seqType[$ii] eq "SUBSYS")
	{
		$processSeqStart[$processSeqCnt] = $processCnt;
		$processSeqCnt ++;
		$xx = $seqList[$ii];
		for($jj=0;$jj<$seqParts[$xx];$jj++)
		{
			$processName[$processCnt] = $seqName[$xx][$jj];
			$processPartNum[$processCnt] = $seq[$xx][$jj];
			#print OUT "$processCnt $processName[$processCnt] $processPartNum[$processCnt]\n";
			$processCnt ++;
		}
	}
	if($seqType[$ii] eq "PART")
	{
		$xx = $seqList[$ii];
		$processName[$processCnt] = $xpartName[$xx];
		$processPartNum[$processCnt] = $xx;
		# print OUT "$processCnt $processName[$processCnt] $processPartNum[$processCnt]\n";
		$processCnt ++;
	}
}
print "Total of $processCnt parts to process\n";

$fpartCnt = 0;
$ftotal = $partCnt;
$inCnt = 0;

# Write Epics structs common to all CDS front ends to the .h file.
print OUTH "\n\n#define MAX_MODULES \t $filtCnt\n";
$filtCnt *= 10;
print OUTH "#define MAX_FILTERS \t $filtCnt\n\n";
print OUTH "#define MAX_FIR \t $firCnt\n";
print OUTH "#define MAX_FIR_POLY \t $firCnt\n\n";
print OUTH "typedef struct CDS_EPICS_IN {\n";
print OUTH "\tfloat vmeReset;\n";
print OUTH "\tint burtRestore;\n";
print OUTH "\tint dcuId;\n";
print OUTH "\tint diagReset;\n";
print OUTH "\tint syncReset;\n";
print OUTH "\tint overflowReset;\n";
print OUTH "} CDS_EPICS_IN;\n\n";
print OUTH "typedef struct CDS_EPICS_OUT {\n";
print OUTH "\tint onePps;\n";
print OUTH "\tint adcWaitTime;\n";
print OUTH "\tint diagWord;\n";
print OUTH "\tint cpuMeter;\n";
print OUTH "\tint cpuMeterMax;\n";
print OUTH "\tint gdsMon[32];\n";
print OUTH "\tint diags[4];\n";
print OUTH "\tint overflowAdc[4][32];\n";
print OUTH "\tint overflowDac[4][16];\n";
print OUTH "\tint ovAccum;\n";
print OUTH "} CDS_EPICS_OUT;\n\n";
print OUTH "typedef struct \U$systemName {\n";
print EPICS "\nEPICS CDS_EPICS dspSpace coeffSpace epicsSpace\n\n";
for($ii=0;$ii<$partCnt;$ii++)
{
	if($partType[$ii] eq "MATRIX") {
		print EPICS "MATRIX $xpartName[$ii]_ $partOutCnt[$ii]x$partInCnt[$ii] $systemName\.$xpartName[$ii]\n";
		print OUTH "\tfloat $xpartName[$ii]\[$partOutCnt[$ii]\]\[$partInCnt[$ii]\];\n";
	}
	if($partType[$ii] eq "RAMP_SW") {
		print OUTH "\tint $xpartName[$ii];\n";
	}
	if($partType[$ii] eq "MULTI_SW") {
		print OUTH "\tint $xpartName[$ii];\n";
	}
	if($partType[$ii] eq "EPICS_INPUT") {
		print OUTH "\tfloat $xpartName[$ii];\n";
	}
	if($partType[$ii] eq "EPICS_OUTPUT") {
		print OUTH "\tfloat $xpartName[$ii];\n";
	}
	if($partType[$ii] eq "PRODUCT") {
		print OUTH "\tfloat $xpartName[$ii];\n";
		print OUTH "\tint $xpartName[$ii]\_TRAMP;\n";
		print OUTH "\tint $xpartName[$ii]\_RMON;\n";
	}
}
print EPICS "\n\n";
print OUTH "} \U$systemName;\n\n";
print EPICS "MOMENTARY VME_RESET epicsInput.vmeReset int ai 0\n";
print EPICS "MOMENTARY DIAG_RESET epicsInput.diagReset int ai 0\n";
print EPICS "MOMENTARY SYNC_RESET epicsInput.syncReset int ai 0\n";
print EPICS "MOMENTARY OVERFLOW_RESET epicsInput.overflowReset int ai 0\n";
print EPICS "DAQVAR LOAD_CONFIG int ai 0\n";
print EPICS "DAQVAR CHAN_CNT int ai 0\n";
print EPICS "DAQVAR TOTAL int ai 0\n";
print EPICS "DAQVAR MSG int ai 0\n";
print EPICS "DAQVAR DCU_ID int ai 0\n";
print EPICS "OUTVARIABLE CPU_METER epicsOutput.cpuMeter int ai 0 field(HOPR,\"$rate\") field(LOPR,\"0\")\n";
print EPICS "OUTVARIABLE CPU_METER_MAX epicsOutput.cpuMeterMax int ai 0 field(HOPR,\"$rate\") field(LOPR,\"0\")\n";
print EPICS "OUTVARIABLE ADC_WAIT epicsOutput.adcWaitTime int ai 0 field(HOPR,\"$rate\") field(LOPR,\"0\")\n";
print EPICS "OUTVARIABLE ONE_PPS epicsOutput.onePps int ai 0\n";
print EPICS "OUTVARIABLE DIAG_WORD epicsOutput.diagWord int ai 0\n";
print EPICS "INVARIABLE BURT_RESTORE epicsInput.burtRestore int ai 0\n";
for($ii=0;$ii<32;$ii++)
{
	print EPICS "OUTVARIABLE GDS_MON_$ii epicsOutput.gdsMon\[$ii\] int ai 0\n";
}
print EPICS "OUTVARIABLE USR_TIME epicsOutput.diags[0] int ai 0\n";
print EPICS "OUTVARIABLE RESYNC_COUNT epicsOutput.diags[1] int ai 0\n";
print EPICS "OUTVARIABLE NET_ERR_COUNT epicsOutput.diags[2] int ai 0\n";
print EPICS "OUTVARIABLE DAQ_BYTE_COUNT epicsOutput.diags[3] int ai 0\n";
print EPICS "\n\n";
#Load EPICS I/O Parts
for($ii=0;$ii<$partCnt;$ii++)
{
	if($partType[$ii] eq "MULTI_SW") {
		print EPICS "INVARIABLE $xpartName[$ii] $systemName\.$xpartName[$ii] int bi 0 field(ZNAM,\"OFF\") field(ONAM,\"ON\")\n";
	}
	if($partType[$ii] eq "RAMP_SW") {
		print EPICS "INVARIABLE $xpartName[$ii] $systemName\.$xpartName[$ii] int bi 0 field(ZNAM,\"OFF\") field(ONAM,\"ON\")\n";
	}
	if($partType[$ii] eq "EPICS_INPUT") {
		print EPICS "INVARIABLE $xpartName[$ii] $systemName\.$xpartName[$ii] float ai 0 field(PREC,\"3\")\n";
	}
	if($partType[$ii] eq "EPICS_OUTPUT") {
		print EPICS "OUTVARIABLE $xpartName[$ii] $systemName\.$xpartName[$ii] float ai 0 field(PREC,\"3\")\n";
	}
	if($partType[$ii] eq "PRODUCT") {
		print EPICS "INVARIABLE $xpartName[$ii] $systemName\.$xpartName[$ii] float ai 0 field(PREC,\"3\")\n";
		print EPICS "INVARIABLE $xpartName[$ii]\_TRAMP $systemName\.$xpartName[$ii]\_TRAMP int ai 0 field(PREC,\"0\")\n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_RMON $systemName\.$xpartName[$ii]\_RMON int ai 0 field(PREC,\"0\")\n";
	}
}
print EPICS "\n\n";
for($ii=0;$ii<$adcCnt;$ii++)
{
	for($jj=0;$jj<32;$jj++)
	{
		print EPICS "OUTVARIABLE ADC_OVERFLOW_$ii\_$jj epicsOutput.overflowAdc\[$ii\]\[$jj\] int ai 0\n";
	}
}
print EPICS "\n\n";
for($ii=0;$ii<$dacCnt;$ii++)
{
	for($jj=0;$jj<16;$jj++)
	{
		print EPICS "OUTVARIABLE DAC_OVERFLOW_$ii\_$jj epicsOutput.overflowDac\[$ii\]\[$jj\] int ai 0\n";
	}
}
print EPICS "OUTVARIABLE ACCUM_OVERFLOW epicsOutput.ovAccum int ai 0\n";
print EPICS "\n\n";
print EPICS "systems \U$systemName\-\n";
$gdsXstart = ($dcuId - 5) * 1250;
$gdsTstart = $gdsXstart + 10000;
print EPICS "gds_config $gdsXstart $gdsTstart 1250 1250\n";
print EPICS "\n\n";

# Start process of writing .c file.
print OUT "// ******* This is a computer generated file *******\n";
print OUT "// ******* DO NOT HAND EDIT ************************\n";
print OUT "\n\n";
print OUT "void feCode(double dWord[][32],\t\/* ADC inputs *\/\n";
print OUT "\t\tint dacOut[][16],\t\/* DAC outputs *\/\n";
print OUT "\t\tFILT_MOD *dspPtr,\t\/* Filter Mod variables *\/\n";
print OUT "\t\tCOEF *dspCoeff,\t\t\/* Filter Mod coeffs *\/\n";
print OUT "\t\tCDS_EPICS *pLocalEpics)\t\/* EPICS variables *\/\n";
print OUT "{\n\nint ii,jj;\n\n";
for($ii=0;$ii<$partCnt;$ii++)
{
	if($partType[$ii] eq "MATRIX") {
		print OUT "double $xpartName[$ii]\[$partOutCnt[$ii]\]\[$partInCnt[$ii]\];\n";
	}
	if($partType[$ii] eq "SUM") {
		print OUT "double $xpartName[$ii];\n";
	}
	if($partType[$ii] eq "DIFF_JUNC") {
		print OUT "double $xpartName[$ii]\[16\];\n";
	}
	if($partType[$ii] eq "RAMP_SW") {
		print OUT "double $xpartName[$ii]\[4\];\n";
	}
	if($partType[$ii] eq "MULTI_SW") {
		print OUT "double $xpartName[$ii]\[$partOutCnt[$ii]\];\n";
	}
	if($partType[$ii] eq "PRODUCT") {
		print OUT "double $xpartName[$ii]\[$partOutCnt[$ii]\];\n";
		print OUT "float $xpartName[$ii]\_CALC;\n";
	}
}
print OUT "\n\n";
for($ii=0;$ii<$partCnt;$ii++)
{
	if($partType[$ii] eq "SUM") {
		print OUT "$xpartName[$ii] = 0.0;\n";
	}
}

print "*****************************************************\n";

$ts = 0;
$xx = 0;
for($xx=0;$xx<$processCnt;$xx++) 
{
	if($xx == $processSeqStart[$ts])
	{
		if($ts != 0)
		{
		print OUT "\n\/\/End of subsystem **************************************************\n\n\n";
		}
		print OUT "\n\/\/Start of subsystem **************************************************\n\n";
		$ts ++;
	}
	$mm = $processPartNum[$xx];
	#print "looking for part $mm\n";
	if($partType[$mm] eq "FILT")
	{
	   $calcExp = "filterModuleD(dspPtr,dspCoeff,";
	   # print "Found Filt $xpartName[$mm] in loop $partOutCnt[$mm]\n";
	   for($jj=0;$jj<$partOutCnt[$mm];$jj++)
	   {
		$fromType = 0;
		$toType = 0;
		$to = $partOutNum[$mm][$jj];
		$from = $partInNum[$mm][0];
		if($jj == 1)
		{
			$calcExp = $outExp;
			$calcExp .= ";\n";
		}

		# Figure OUTPUT connection type **************************************
		if($partType[$to] eq "MATRIX")
		{
			$toType = 1;
			$toPort = $partOutputPort[$mm][0];
			$toName = $xpartName[$to];
			#print "FILT has output to matrix $toName port $toPort\n";
			$outExp = $toName;
			$outExp .= "\[0\]\[";
			$outExp .= $toPort;
			$outExp .= "\]";
			#print "MATRIX $xpartName[$mm]  $outExp\n";
		}
		if($partType[$to] eq "SUM")
		{
			$toType = 3;
			$toPort = $partOutputPort[$mm][0];
			$toName = $xpartName[$to];
			$outExp = $toName;
			#print "SUM $xpartName[$mm]  $outExp\n";
		}
		if($partType[$to] eq "FILT")
		{
			$toType = 4;
			$toPort = $partOutputPort[$mm][0];
			$toName = $xpartName[$to];
			$outExp = "filtIn\[";
			$outExp .= $toName;
			$outExp .= "\]";
			#print "FILT $xpartName[$mm]  $outExp\n";
		}
		if($partType[$to] eq "DAC")
		{
			$toPort = substr($partName[$to],4,1);
			$toType = 2;
			$toPort1 = $partOutputPort[$mm][0] - 1;
			$outExp = "dacOut\[";
			$outExp .= $toPort;
			$outExp .= "\]\[";
			$outExp .= $toPort1;
			$outExp .= "\]";
			#print "DAC $xpartName[$mm]  $outExp\n";
		}
		if($partType[$to] eq "MULTI_SW")
		{
			$toType = 6;
			$toPort = $partOutputPort[$mm][0];
			$toName = $xpartName[$to];
			$outExp = "$toName\[";
			$outExp .= $toPort;
			$outExp .= "\]";
			#print "FILT $xpartName[$mm]  $outExp\n";
		}
		if($partType[$to] eq "PRODUCT")
		{
			$toType = 6;
			$toPort = $partOutputPort[$mm][0];
			$toName = $xpartName[$to];
			$outExp = "$toName\[";
			$outExp .= $toPort;
			$outExp .= "\]";
			#print "FILT $xpartName[$mm]  $outExp\n";
		}
		if($partType[$to] eq "DIFF_JUNC")
		{
			$toType = 7;
			$toPort = $partOutputPort[$mm][0];
			$toName = $xpartName[$to];
			$outExp = "$toName\[";
			$outExp .= $toPort;
			$outExp .= "\]";
			#print "FILT $xpartName[$mm]  $outExp\n";
		}
		if($partType[$to] eq "RAMP_SW")
		{
			$toType = 8;
			$toPort = $partOutputPort[$mm][0];
			$toName = $xpartName[$to];
			$outExp = "$toName\[";
			$outExp .= $toPort;
			$outExp .= "\]";
			#print "FILT $xpartName[$mm]  $outExp\n";
		}
		if($partType[$to] eq "TERM")
		{
			$toType = 0;
			$outExp = "";
		}
		if($jj == 0)
		{
		# Figure INPUT connection type **************************************
		if($partInputType[$mm][0] eq "MATRIX")
		{
			$fromType = 1;
			$fromPort = $partInputPort[$mm][0];
			$fromName = $xpartName[$from];
			#print "FILT has input from matrix $fromName port $fromPort\n";
			$calcExp .= $xpartName[$mm];
			$calcExp .= ",";
			$calcExp .= $fromName;
			$calcExp .= "\[1\]\[";
			$calcExp .= $fromPort;
			$calcExp .= "\],0);\n";
			#print "FROM MATRIX $xpartName[$mm]  $calcExp\n";
		}
		if($partType[$from] eq "SUM")
		{
			$fromType = 3;
			$fromPort = $partInputPort[$mm][0];
			$fromName = $xpartName[$from];
			#print "FILT has input from SUM $fromName port $fromPort\n";
			$calcExp .= $xpartName[$mm];
			$calcExp .= ",";
			$calcExp .= $xpartName[$from];
			$calcExp .= ",0);\n";
			#print "FROM SUM $xpartName[$mm]  $calcExp\n";
		}
		if($partType[$from] eq "FILT")
		{
			$fromType = 4;
			$fromPort = $partInputPort[$mm][0];
			$fromName = $xpartName[$from];
			#print "FILT has input from FILT $fromName port $fromPort\n";
			$calcExp .= $xpartName[$mm];
			$calcExp .= ",filtIn\[";
			$calcExp .= $xpartName[$mm];
			$calcExp .= "\],0); \n";
			#print "FROM FILTER $xpartName[$mm]  $calcExp\n";
		}
		if($partType[$from] eq "GROUND")
		{
			$calcExp .= $xpartName[$mm];
			$calcExp .= ",0.0,0);\n";
			# print "FROM GROUND $xpartName[$mm]  $calcExp\n";
		}
		if($partInputType[$mm][0] eq "ADC")
		{
			$fromType = 5;
			$fromPort = $partInNum[$mm][0];
			$fromPort1 = $partInputPort[$mm][0];
			$calcExp .= $xpartName[$mm];
			$calcExp .= ",dWord\[";
			$calcExp .= $fromPort;
			$calcExp .= "\]\[";
			$calcExp .= $fromPort1;
			$calcExp .= "\],0); \n";
			#print "ADC $xpartName[$mm]  $calcExp\n";
		}
		if($partType[$from] eq "RAMP_SW")
		{
			$fromType = 8;
			$fromPort = $partInputPort[$mm][0];
			$fromName = $xpartName[$from];
			#print "FILT has input from FILT $fromName port $fromPort\n";
			$calcExp .= $xpartName[$mm];
			$calcExp .= ",";
			$calcExp .= $fromName;
			$calcExp .= "\[";
			$calcExp .= $fromPort;
			$calcExp .= "\],0); \n";
			#print "FROM FILTER $xpartName[$mm]  $calcExp\n";
		}
		if($partType[$from] eq "MULTI_SW")
		{
			$fromType = 6;
			$fromPort = $partInputPort[$mm][0];
			$fromName = $xpartName[$from];
			#print "FILT has input from FILT $fromName port $fromPort\n";
			$calcExp .= $xpartName[$mm];
			$calcExp .= ",";
			$calcExp .= $fromName;
			$calcExp .= "\[";
			$calcExp .= $fromPort;
			$calcExp .= "\],0); \n";
		}
		if($partType[$from] eq "EPICS_OUTPUT")
		{
			$fromType = 9;
			$fromPort = $partInputPort[$mm][0];
			$fromName = $xpartName[$from];
			$calcExp .= $xpartName[$mm];
			$calcExp .= ",";
			$calcExp .= "pLocalEpics->";
			$calcExp .= $systemName;
			$calcExp .= "\.";
			$calcExp .= $xpartName[$from];
			$calcExp .= ",0); \n";
		}
		}

		$codeLine = $outExp;
		if(($toType != 3) && ($toType != 0))
		{
			$codeLine .= " = ";
		}
		if($toType == 3)
		{
			$codeLine .= " += ";
		}
		$codeLine .= $calcExp;
		print OUT "$codeLine";

	   }
	}
	# ******** MATRIX *************************************************************************
	if($partType[$mm] eq "MATRIX")
	{
	   	#print "Found Matrix $xpartName[$mm] in loop\n";
		
		for($qq=0;$qq<$partOutCnt[$mm];$qq++)
		{
			$portUsed[$qq] = 0;
		}
		$matOuts = 0;
#FIX
		for($qq=0;$qq<$partOutCnt[$mm];$qq++)
		{
			$fromPort = $partOutputPortUsed[$mm][$qq];
			if($portUsed[$fromPort] == 0)
			{
				$portUsed[$fromPort] = 1;
				$matOuts ++;
			}
		}
		#print "$xpartName[$mm] has $matOuts Outputs\n";
		print OUT "\n";
		print OUT "\/\/ Perform Matrix Calc **********************\n\n";
		print OUT "for(ii=0;ii<$matOuts;ii++)\n{\n";
		print OUT "$xpartName[$mm]\[1\]\[ii\] = \n";
		for($qq=0;$qq<$partInCnt[$mm];$qq++)
		{
			$done = 0;
			$to = $partInNum[$mm][$qq];
			$fromType = $partInputType[$mm][$qq];
			$toType = $partType[$to];
			$toName = $xpartName[$to];
			$fromPort1 = $partInputPort[$mm][$qq];
			if($fromType eq "ADC")
			{
				#print "Found MATRIX with ADC input on $qq\n";
				$card = $partInNum[$mm][$qq];
				$chan = $partInputPort[$mm][$qq];
				print OUT "\tpLocalEpics->$systemName\.$xpartName[$mm]\[ii\]\[$qq\] * dWord\[$card\]\[$chan\]";
				#print "\tpLocalEpics->$systemName\.$xpartName[$mm]\[ii\]\[$qq\] * dWord\[$card\]\[$chan\]\n";
				$done = 1;
			}
			if ($toType eq "EPICS_INPUT")
                        {
				print OUT "\tpLocalEpics->$systemName\.$xpartName[$mm]\[ii\]\[$qq\] * pLocalEpics->$systemName\.$xpartName[$to]";
				$done = 1;
			}
			if ($toType eq "RAMP_SW")
                        {
				print OUT "\tpLocalEpics->$systemName\.$xpartName[$mm]\[ii\]\[$qq\] * $xpartName[$to]\[$fromPort1\]";
				#print "\tpLocalEpics->$systemName\.$xpartName[$mm]\[ii\]\[$qq\] * $xpartName[$to]\[$fromPort1\]\n";
				$done = 1;
			}
			if(!$done)
			{
				print OUT "\tpLocalEpics->$systemName\.$xpartName[$mm]\[ii\]\[$qq\] * $xpartName[$mm]\[0\]\[$qq\]";
				#print "\tpLocalEpics->$systemName\.$xpartName[$mm]\[ii\]\[$qq\] * $xpartName[$mm]\[0\]\[$qq\];\n";
			}
			if($qq == ($partInCnt[$mm] - 1))
			{
				print OUT ";\n";
			}
			else
			{
				print OUT " +\n";
			}
		}
		print OUT "}\n";
		for($qq=0;$qq<$partOutCnt[$mm];$qq++)
		{
			if($partOutputType[$mm][$qq] eq "SUM")
			{
				print OUT "$partOutput[$mm][$qq] += $xpartName[$mm]\[1\]\[$partOutputPortUsed[$mm][$qq]\];\n";
			}
		}
		print OUT "\/\/ End Matrix Calc ***************************\n\n\n";
	}
	# ******** SUMMING JUNC ********************************************************************
	if($partType[$mm] eq "SUM")
	{
	   #print "Found SUM $xpartName[$mm] in loop\n";
		#print "\tUsed Sum $xpartName[$mm] $partOutCnt[$mm]\n";
		#print OUT "pLocalEpics->$systemName\.$xpartName[$mm] = $xpartName[$mm]\[0\] + $xpartName[$mm]\[1\]\n";
	}
	# ******** DIFF JUNC ********************************************************************
	if($partType[$mm] eq "DIFF_JUNC")
	{
	   $zz = 0;
	   for($qq=0;$qq<16;$qq+=2)
	   {
		$to = $partOutNum[$mm][$zz];
		$toPort = $partOutputPort[$mm][$zz];
		$from = $partInNum[$mm][$qq];
		$from1 = $partInNum[$mm][$qq+1];
		if($partType[$to] eq "MATRIX")
		{
			print OUT "$xpartName[$to]\[0\]\[$toPort\] = ";
		}
		else
		{
			print "Error in DIFF JUNC  $xpartName[$mm] - Unsupported output type\n";
			exit;
		}
		if($partType[$from1] eq "MATRIX")
		{
			print OUT "$xpartName[$to]\[1\]\[$toPort\] = ";
		}
		else
		{
			$yy = $qq+1;
			print OUT "$xpartName[$mm]\[$yy\] - ";
		}
		if($partType[$from] eq "MATRIX")
		{
			print OUT "$xpartName[$from]\[1\]\[$toPort\];\n";
		}
		else
		{
			print OUT "$xpartName[$mm];\n";
		}
		$zz++;
	   }
	}
	# ******** GROUND INPUT ********************************************************************
	if(($partType[$mm] eq "GROUND") && ($partUsed[$mm] == 0))
	{
	   #print "Found GROUND $xpartName[$mm] in loop\n";
	}
	# ******** EPICS OUTPUT ********************************************************************
	if($partType[$mm] eq "EPICS_OUTPUT")
	{
	   #print "Found EPICS OUTPUT $xpartName[$mm] $partInputType[$mm][0] in loop\n";
		$done = 0;
		$fromType = $partInputType[$mm][0];
		$to = $partOutNum[$mm][0];
		$toType = $partType[$to];
		$toName = $xpartName[$to];
		if($fromType eq "ADC")
		{
			$fromCard = $partInNum[$mm][0];
			$fromChan = $partInputPort[$mm][0];
			print OUT "pLocalEpics->$systemName\.$xpartName[$mm] = ";
			print OUT "dWord\[$fromCard\]\[$fromChan\];\n";
			$done = 1;
		}
		if($toType eq "DAC")
		{
			$toPort = substr($partName[$to],4,1);
			$toPort1 = $partOutputPort[$mm][0];
			print OUT "dacOut\[$toPort\]\[$toPort1\] = ";
			print OUT "pLocalEpics->$systemName\.$xpartName[$mm];\n";
		}
	}
	# ******** MULTI_SW ************************************************************************
	if($partType[$mm] eq "MULTI_SW")
	{
		print OUT "if(pLocalEpics->$systemName\.$xpartName[$mm] == 0)\n";
		print OUT "{\n";
		print OUT "\tfor(ii=0;ii< $partOutCnt[$mm];ii++) $xpartName[$mm]\[ii\] = 0.0;\n";
		print OUT "}\n\n";
	   $from = $partInNum[$mm][0];
	   $fromType = $partType[$from];
	   if($fromType eq "MATRIX")
	   {
		print OUT "else {\n";
		print OUT "\tfor(ii=0;ii< $partOutCnt[$mm];ii++) $xpartName[$mm]\[ii\] = $xpartName[$from]\[1\]\[ii\];\n";
		print OUT "}\n";
	   }
	   for($jj=0;$jj<$partOutCnt[$mm];$jj++)
	   {
		$fromType = 0;
		$to = $partOutNum[$mm][$jj];
		$toType = $partType[$to];
		$toName = $xpartName[$to];
		if($toType eq "FILT")
		{
		}
		if($toType eq "DAC")
		{
			$toPort = substr($partName[$to],4,1);
			$toPort1 = $partOutputPort[$mm][$jj] - 1;
			print OUT "\tdacOut\[$toPort\]\[$toPort1\] = $xpartName[$mm]\[$jj\];\n";
		}
		if($toType eq "MATRIX")
		{
			$toPort = $partOutputPort[$mm][$jj];
			print OUT "\t$xpartName[$to]\[0\]\[$jj\] = $xpartName[$mm]\[$jj\];\n";
		}
		if($toType eq "SUM")
		{
			print OUT "\t$xpartName[$to] += $xpartName[$mm]\[$jj\];\n";
		}
		if($toType eq "EPICS_OUTPUT")
		{
			print OUT "pLocalEpics->$systemName\.$xpartName[$to] = ";
			print OUT "$xpartName[$mm]\[$partOutputPortUsed[$mm][$jj]\];\n";
		}
	   }
	}
	if($partType[$mm] eq "FIR_FILTER")
	{
		$from = $partInNum[$mm][0];
		$fromType = $partInputType[$mm][0];
		$fromPort = $partInputPort[$mm][0];
		$to = $partOutNum[$mm][0];
		$toType = $partType[$to];
		$toName = $xpartName[$to];
		print "Found FIR $from $fromType $fromPort\n";
		$firName = $xpartName[$mm];
		$firName .= _FIR;
	   	$calcExp = "filterPolyPhase(dspPtr,firCoeff,$xpartName[$mm],$firName,";
		if($toType eq "MATRIX")
		{
			$toPort = $partOutputPort[$mm][0];
			print "\t$xpartName[$to]\[0\]\[$toPort\] = ";
		}
		if($fromType eq "ADC")
		{
			$card = $from;
			$chan = $fromPort;
			$calcExp .= "dWord\[";
			$calcExp .= $card;
			$calcExp .= "\]\[";
			$calcExp .= $chan;
			$calcExp .= "\],0);\n";
			print "$calcExp";
		}
	}
	if($partType[$mm] eq "RAMP_SW")
	{
	   for($jj=0;$jj<$partInCnt[$mm];$jj++)
	   {
		$from = $partInNum[$mm][$jj];
		$fromType = $partInputType[$mm][$jj];
		$fromPort = $partInputPort[$mm][$jj];
#print "\nFound RAMP_SW from $xpartName[$from] type $fromType port $fromPort\n";
		if($fromType eq "ADC")
		{
			print OUT "$xpartName[$mm]\[$jj\] = dWord\[$from\]\[$fromPort\];\n";
		}
	   }
	   print OUT "if(pLocalEpics->$systemName\.$xpartName[$mm] == 0)\n";
		print OUT "{\n";
		print OUT "\t$xpartName[$mm]\[1\] = $xpartName[$mm]\[2\];\n";
		print OUT "}\n";
		print OUT "else\n";
		print OUT "{\n";
		print OUT "\t$xpartName[$mm]\[0\] = $xpartName[$mm]\[1\];\n";
		print OUT "\t$xpartName[$mm]\[1\] = $xpartName[$mm]\[3\];\n";
		print OUT "}\n\n";
	}
	if($partType[$mm] eq "PRODUCT")
	{
	print OUT "pLocalEpics->$systemName\.$xpartName[$mm]";
	print OUT "_RMON = \n\tgainRamp(pLocalEpics->$systemName\.$xpartName[$mm],";
	print OUT "pLocalEpics->$systemName\.$xpartName[$mm]";
	print OUT "_TRAMP,0,\&$xpartName[$mm]\_CALC);";
	print OUT "\n";
	   for($jj=0;$jj<$partInCnt[$mm];$jj++)
	   {
		$from = $partInNum[$mm][$jj];
		$fromType = $partType[$from];
		$fromPort = $partInputPort[$mm][$jj];
		$to = $partOutNum[$mm][$jj];
		$toType = $partType[$to];
		$toPort = $partOutputPort[$mm][$jj];
		if(($fromType eq "MATRIX") && ($toType eq "MULTI_SW"))
		{
			print OUT "$xpartName[$to]\[$jj\] = $xpartName[$mm]\_CALC * $xpartName[$from]\[1\]\[$fromPort\];\n";
		}
		if(($fromType eq "FILT") && ($toType eq "MULTI_SW"))
		{
			print OUT "$xpartName[$to]\[$toPort\] = $xpartName[$mm]\_CALC * $xpartName[$mm]\[$jj\];\n";
		}
	   }
	}


print OUT "\n";
}
print OUT "}\n\n";
print OUTH "typedef struct CDS_EPICS {\n";
print OUTH "\tCDS_EPICS_IN epicsInput;\n";
print OUTH "\tCDS_EPICS_OUT epicsOutput;\n";
print OUTH "\t\U$systemName \L$systemName;\n";
print OUTH "} CDS_EPICS;\n";


if($partsRemaining != 0) {
	print "WARNING -- NOT ALL PARTS CONNECTED !!!!\n";
for($ii=0;$ii<$partCnt;$ii++)
{
	if($partUsed[$ii] == 0)
	{
	print "$ii $xpartName[$ii] $partOutCnt[$ii] $partUsed[$ii]\n";
	}
}
}

close IN;
close OUT;
	print OUTH "\#endif\n";
close OUTH;
close OUTD;
close EPICS;
print OUTM "# RTLinux makefile\n";
print OUTM "include /opt/rtldk-2.2/rtlinuxpro/rtl.mk\n";
print OUTM "\n";
print OUTM "\n";
print OUTM "TARGET_RTL := $skeleton";
print OUTM "fe\.rtl\n";
print OUTM "LIBRARY_OBJS := map.o\n";
print OUTM "LDFLAGS_\$(TARGET_RTL) := \$(LIBRARY_OBJS)\n";
print OUTM "\n";
print OUTM "\$(TARGET_RTL): \$(LIBRARY_OBJS)\n";
print OUTM "\n";
print OUTM "$skeleton";
print OUTM "fe\.o: ../controller.c\n";
print OUTM "\t\$(CC) \$(CFLAGS) -c \$< -o \$\@\n";
print OUTM "map.o: ../map.c\n";
print OUTM "\t\$(CC) \$(CFLAGS) -D__KERNEL__ -c \$<\n";
print OUTM "fm10Gen.o: fm10Gen.c\n";
if($rate == 60)
{
print "SERVO IS 16K\n";
print OUTM "\t\$(CC) \$(CFLAGS) -D__KERNEL__ -DSERVO16K -c \$<\n";
}
else
{
print "SERVO IS 2K\n";
print OUTM "\t\$(CC) \$(CFLAGS) -D__KERNEL__ -DSERVO2K -c \$<\n";
}
print OUTM "crc.o: crc.c\n";
print OUTM "\t\$(CC) \$(CFLAGS) -D__KERNEL__ -c \$<\n";
print OUTM "\n";
print OUTM "ALL \+= user_mmap \$(TARGET_RTL)\n";
print OUTM "CFLAGS += -I../../include\n";
print OUTM "CFLAGS += -I/opt/gm/include\n";
if($rate == 60)
{
print OUTM "CFLAGS += -DSERVO16K\n";
}
else
{
print OUTM "CFLAGS += -DSERVO2K\n";
}
print OUTM "CFLAGS += -D";
print OUTM "\U$skeleton";
print OUTM "_CODE\n";
print OUTM "\n";
print OUTM "all: \$(ALL)\n";
print OUTM "\n";
print OUTM "clean:\n";
print OUTM "\trm -f \$(ALL) *.o\n";
print OUTM "\n";
print OUTM "include /opt/rtldk-2.2/rtlinuxpro/Rules.make\n";
print OUTM "\n";
close OUTM;

print OUTME "\n";
print OUTME "# Define Epics system name. It should be unique.\n";
print OUTME "TARGET = $skeleton";
print OUTME "epics\n";
print OUTME "\n";
print OUTME "SRC = build/\$(TARGET)/";
print OUTME "$skeleton";
print OUTME "\.st\n";
print OUTME "\n";
print OUTME "SRC += src/drv/rfm.c\n";
print OUTME "SRC += src/drv/param.c\n";
print OUTME "SRC += src/drv/crc.c\n";
print OUTME "SRC += src/drv/fmReadCoeff.c\n";
print OUTME "\n";
print OUTME "DB += build/\$(TARGET)/";
print OUTME "$skeleton";
print OUTME "1\.db\n";
print OUTME "\n";
print OUTME "IFO = M1\n";
print OUTME "SITE = mit\n";
print OUTME "\n";
print OUTME "SEQ += \'";
print OUTME "$skeleton";
print OUTME ",(\"ifo=M1, site=mit, sys=\U$systemName\, \Lsysnum= $dcuId\")\'\n";
print OUTME "\n";
print OUTME "CFLAGS += -D";
print OUTME "\U$skeleton";
print OUTME "_CODE\n";
print OUTME "\n";
print OUTME "include config/Makefile.linux\n";
print OUTME "\n";
print OUTME "build/\$(TARGET)/";
print OUTME "$skeleton";
print OUTME "1\.db: build/\$(TARGET)/";
print OUTME "$skeleton";
print OUTME "\.db\n";
print OUTME "\tsed 's/%SYS%/";
print OUTME "\U$systemName";
print OUTME "/g;s/%SUBSYS%//g' \$< > \$\@\n";
print OUTME "\n";
print OUTME "\n";
close OUTME;
