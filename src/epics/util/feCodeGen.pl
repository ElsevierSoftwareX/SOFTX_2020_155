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
$useWd = 0;
$gainCnt = 0;

# Clear the part input and output counters
for($ii=0;$ii<2000;$ii++)
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
	if($blockType eq "UnitDelay") {$partType[$partCnt] = DELAY;}
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
	if (substr($var2,0,6) eq "cdsRms") {
		$partType[$partCnt] = RMS;
		$partErr = 0;
	}
	if (substr($var2,0,7) eq "cdsSWD1") {
		$partType[$partCnt] = SEI_WD1;
		$useWdName[$useWd] = substr($xpartName[$partCnt],0,3);
		$useWd ++;
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
		#$firName = $xpartName[$partCnt];
		#$firName .= _FIR;
		#print OUTH "#define $firName \t $filtCnt\n";
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
	      for($ll=0;$ll<$partOutCnt[$kk];$ll++)
        	{
		if(($partOutput[$ii][$jj] eq $partInput[$kk][0]) && ($partOutputPort[$ii][$jj] == $partInputPort[$kk][0]))
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
			for($vv=0;$vv<$partOutCnt[$fromNum];$vv++)
			{
				if($partOutput[$fromNum][$vv] eq $xpartName[$ii])
				{
					$fromLink = $vv;
				}
			}
                       $partOutput[$fromNum][$fromLink] = $xpartName[$toNum];
                       $partOutputType[$fromNum][$fromLink] = $partType[$toNum];
                       $partOutNum[$fromNum][$fromLink] = $toNum;
                       $partOutputPort[$fromNum][$fromLink] = $toPort;
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
				$fromNum = $partInNum[$ii][$jj];
				$fromPort = $partInputPort[$ii][$jj];
			#print "\t Maybe $xpartName[$xx] port $partOutputPort[$ii][$jj] $xpartName[$fromNum] $partType[$fromNum] $fromPort\n";
				# Make output connection at source part
				$partOutput[$fromNum][$fromPort] = $xpartName[$xx];
				$partOutputType[$fromNum][$fromPort] = $partType[$xx];
				$partOutNum[$fromNum][$fromPort] = $xx;
				$partOutputPort[$fromNum][$fromPort] = $partOutputPort[$ii][$jj];
                       		$partSysFromx[$xx][$fromCnt[$xx]] = $partSubNum[$ii];
				# Make input connection at destination part
				$qq = $partOutputPort[$ii][$jj] - 1;
				$partInput[$xx][$qq] = $xpartName[$fromNum];
				$partInputType[$xx][$qq] = $partType[$fromNum];
				$partInNum[$xx][$qq] = $fromNum;
				$partInputPort[$xx][$qq] = $fromPort;
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
				#print "Found ADC INPUT connect from $xpartName[$xx] to $xpartName[$kk] $partOutputPort[$xx][$jj] $jj  input $fromPort $partInput[$xx][$fromPort]\n";
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
					$toPort1 = $partOutputPortUsed[$kk][$ll];
				#print "Found nonADC connect from $xpartName[$xx] port $mm to $xpartName[$toNum] $partInputPort[$xx][$jj]\n";
					$partInNum[$toNum][$toPort] = $xx;
					$partInput[$toNum][$toPort] = $xpartName[$xx];
					$partInputPort[$toNum][$toPort] = $toPort1;
					$partInputType[$toNum][$toPort] = $partType[$xx];

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
	print OUTD "\tPart Name\tType\tNum\tPort\tPort Used\n";
	for($jj=0;$jj<$partOutCnt[$xx];$jj++)
	{
		$to = $partOutNum[$xx][$jj];
		print OUTD "\t$partOutput[$xx][$jj]\t$partOutputType[$xx][$jj]\t$partOutNum[$xx][$jj]\t$partOutputPort[$xx][$jj]\t$partOutputPortUsed[$xx][$jj]\n";
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
	if(($partType[$xx] ne "BUSC") && ($partType[$xx] ne "BUSS"))
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
$subUsed[$ii] = 0;
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
		print "Subsys $ii $subSysName[$ii] has all ADC inputs and can go $seqCnt\n";
		$subRemaining --;
	}
}
#print "Searching parts $searchCnt\n";
for($ii=0;$ii<$searchCnt;$ii++)
{
	$allADC = 1;
	$xx = $searchPart[$ii];
	if($partUsed[$xx] == 0)
	{
		for($jj=0;$jj<$partInCnt[$xx];$jj++)
		{
			if(($partInputType[$xx][$jj] ne "ADC") && ($partInputType[$xx][$jj] ne "DELAY"))

			{
					$allADC = 0;
			}
		}
		if($allADC == 1) {
			print "Part $xx $xpartName[$xx] can go next\n";
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
				}
				$yy = $subInputs[$ii][$jj] - 100;
				for($kk=0;$kk<$searchCnt;$kk++)
				{
					if(($partUsed[$yy] != 1) && ($subInputsType[$ii][$jj] ne "ADC") && ($partType[$yy] ne "DELAY"))
					{
						$allADC = 0;
					}
				}
			}
			if($allADC == 1) {
				print "Subsys $ii $subSysName[$ii] can go next\n";
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
				print "Part $xx $xpartName[$xx] can go next\n";
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
if($useWd)
{
print OUTH "typedef struct SEI_WATCHDOG {\n";
print OUTH "\tint trip;\n";
print OUTH "\tint reset;\n";
print OUTH "\tint status\[3\];\n";
print OUTH "\tint senCount[20];\n";
print OUTH "\tint senCountHold[20];\n";
print OUTH "\tint filtMax[20];\n";
print OUTH "\tint filtMaxHold[20];\n";
print OUTH "\tint tripSetF[5];\n";
print OUTH "\tint tripSetR[5];\n";
print OUTH "\tint tripState;\n";
print OUTH "} SEI_WATCHDOG;\n\n";
}
print OUTH "typedef struct \U$systemName {\n";
print EPICS "\nEPICS CDS_EPICS dspSpace coeffSpace epicsSpace\n\n";
for($ii=0;$ii<$partCnt;$ii++)
{
	if($partType[$ii] eq "MATRIX") {
		for($qq=0;$qq<$partOutCnt[$ii];$qq++)
		{
			$portUsed[$qq] = 0;
		}
		$matOuts[$ii] = 0;
		for($qq=0;$qq<$partOutCnt[$ii];$qq++)
		{
			$fromPort = $partOutputPortUsed[$ii][$qq];
			if($portUsed[$fromPort] == 0)
			{
				$portUsed[$fromPort] = 1;
				$matOuts[$ii] ++;
			}
		}
		#print "$xpartName[$ii] has $matOuts[$ii] Outputs\n";
		print EPICS "MATRIX $xpartName[$ii]_ $matOuts[$ii]x$partInCnt[$ii] $systemName\.$xpartName[$ii]\n";
		print OUTH "\tfloat $xpartName[$ii]\[$matOuts[$ii]\]\[$partInCnt[$ii]\];\n";
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
	if($partType[$ii] eq "SEI_WD1") {
		print OUTH "\tSEI_WATCHDOG $xpartName[$ii];\n";
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
	if($partType[$ii] eq "SEI_WD1") {
		print EPICS "DUMMY $xpartName[$ii]\_STATUS int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_TRIP $systemName\.$xpartName[$ii]\.trip int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_S0 $systemName\.$xpartName[$ii]\.status[0] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_S1 $systemName\.$xpartName[$ii]\.status[1] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_S2 $systemName\.$xpartName[$ii]\.status[2] int ai 0 \n";
		print EPICS "DUMMY $xpartName[$ii]\_RESET int ai 0 \n";
		print EPICS "INVARIABLE $xpartName[$ii]\_MAX_S $systemName\.$xpartName[$ii]\.tripSetR\[0\] int ai 0 \n";
		print EPICS "INVARIABLE $xpartName[$ii]\_MAX_PV $systemName\.$xpartName[$ii]\.tripSetR\[1\] int ai 0 \n";
		print EPICS "INVARIABLE $xpartName[$ii]\_MAX_PH $systemName\.$xpartName[$ii]\.tripSetR\[2\] int ai 0 \n";
		print EPICS "INVARIABLE $xpartName[$ii]\_MAX_GV $systemName\.$xpartName[$ii]\.tripSetR\[3\] int ai 0 \n";
		print EPICS "INVARIABLE $xpartName[$ii]\_MAX_GH $systemName\.$xpartName[$ii]\.tripSetR\[4\] int ai 0 \n";
		print EPICS "INVARIABLE $xpartName[$ii]\_MAX_SF $systemName\.$xpartName[$ii]\.tripSetF\[0\] int ai 0 \n";
		print EPICS "INVARIABLE $xpartName[$ii]\_MAX_PVF $systemName\.$xpartName[$ii]\.tripSetF\[1\] int ai 0 \n";
		print EPICS "INVARIABLE $xpartName[$ii]\_MAX_PHF $systemName\.$xpartName[$ii]\.tripSetF\[2\] int ai 0 \n";
		print EPICS "INVARIABLE $xpartName[$ii]\_MAX_GVF $systemName\.$xpartName[$ii]\.tripSetF\[3\] int ai 0 \n";
		print EPICS "INVARIABLE $xpartName[$ii]\_MAX_GHF $systemName\.$xpartName[$ii]\.tripSetF\[4\] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_STXF $systemName\.$xpartName[$ii]\.filtMaxHold\[0\] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_STYF $systemName\.$xpartName[$ii]\.filtMaxHold\[1\] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_STZF $systemName\.$xpartName[$ii]\.filtMaxHold\[2\] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_PV1F $systemName\.$xpartName[$ii]\.filtMaxHold\[3\] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_PV2F $systemName\.$xpartName[$ii]\.filtMaxHold\[4\] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_PV3F $systemName\.$xpartName[$ii]\.filtMaxHold\[5\] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_PV4F $systemName\.$xpartName[$ii]\.filtMaxHold\[6\] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_PH1F $systemName\.$xpartName[$ii]\.filtMaxHold\[7\] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_PH2F $systemName\.$xpartName[$ii]\.filtMaxHold\[8\] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_PH3F $systemName\.$xpartName[$ii]\.filtMaxHold\[9\] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_PH4F $systemName\.$xpartName[$ii]\.filtMaxHold\[10\] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_GV1F $systemName\.$xpartName[$ii]\.filtMaxHold\[11\] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_GV2F $systemName\.$xpartName[$ii]\.filtMaxHold\[12\] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_GV3F $systemName\.$xpartName[$ii]\.filtMaxHold\[13\] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_GV4F $systemName\.$xpartName[$ii]\.filtMaxHold\[14\] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_GH1F $systemName\.$xpartName[$ii]\.filtMaxHold\[15\] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_GH2F $systemName\.$xpartName[$ii]\.filtMaxHold\[16\] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_GH3F $systemName\.$xpartName[$ii]\.filtMaxHold\[17\] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_GH4F $systemName\.$xpartName[$ii]\.filtMaxHold\[18\] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_STX $systemName\.$xpartName[$ii]\.senCountHold\[0\] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_STY $systemName\.$xpartName[$ii]\.senCountHold\[1\] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_STZ $systemName\.$xpartName[$ii]\.senCountHold\[2\] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_PV1 $systemName\.$xpartName[$ii]\.senCountHold\[3\] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_PV2 $systemName\.$xpartName[$ii]\.senCountHold\[4\] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_PV3 $systemName\.$xpartName[$ii]\.senCountHold\[5\] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_PV4 $systemName\.$xpartName[$ii]\.senCountHold\[6\] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_PH1 $systemName\.$xpartName[$ii]\.senCountHold\[7\] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_PH2 $systemName\.$xpartName[$ii]\.senCountHold\[8\] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_PH3 $systemName\.$xpartName[$ii]\.senCountHold\[9\] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_PH4 $systemName\.$xpartName[$ii]\.senCountHold\[10\] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_GV1 $systemName\.$xpartName[$ii]\.senCountHold\[11\] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_GV2 $systemName\.$xpartName[$ii]\.senCountHold\[12\] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_GV3 $systemName\.$xpartName[$ii]\.senCountHold\[13\] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_GV4 $systemName\.$xpartName[$ii]\.senCountHold\[14\] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_GH1 $systemName\.$xpartName[$ii]\.senCountHold\[15\] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_GH2 $systemName\.$xpartName[$ii]\.senCountHold\[16\] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_GH3 $systemName\.$xpartName[$ii]\.senCountHold\[17\] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_GH4 $systemName\.$xpartName[$ii]\.senCountHold\[18\] int ai 0 \n";
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
print OUT "void feCode(int cycle, double dWord[][32],\t\/* ADC inputs *\/\n";
print OUT "\t\tint dacOut[][16],\t\/* DAC outputs *\/\n";
print OUT "\t\tFILT_MOD *dspPtr,\t\/* Filter Mod variables *\/\n";
print OUT "\t\tCOEF *dspCoeff,\t\t\/* Filter Mod coeffs *\/\n";
print OUT "\t\tCDS_EPICS *pLocalEpics,\t\/* EPICS variables *\/\n";
print OUT "\t\tint feInit)\t\/* Initialization flag *\/\n";
print OUT "{\n\nint ii,jj;\n\n";
for($ii=0;$ii<$partCnt;$ii++)
{
	if($partType[$ii] eq "MATRIX") {
		print OUT "double \L$xpartName[$ii]\[$matOuts[$ii]\]\[$partInCnt[$ii]\];\n";
	}
	if($partType[$ii] eq "SUM") {
		$port = $partInCnt[$ii];
		print OUT "double \L$xpartName[$ii];\n";
	}
	if($partType[$ii] eq "DIFF_JUNC") {
		print OUT "double \L$xpartName[$ii]\[16\];\n";
	}
	if($partType[$ii] eq "FILT") {
		print OUT "double \L$xpartName[$ii];\n";
	}
	if($partType[$ii] eq "FIR_FILTER") {
		print OUT "double \L$xpartName[$ii];\n";
	}
	if($partType[$ii] eq "RAMP_SW") {
		print OUT "double \L$xpartName[$ii]\[4\];\n";
	}
	if($partType[$ii] eq "MULTI_SW") {
		print OUT "double \L$xpartName[$ii]\[$partOutCnt[$ii]\];\n";
	}
	if($partType[$ii] eq "DELAY") {
		print OUT "static double \L$xpartName[$ii];\n";
	}
	if($partType[$ii] eq "PRODUCT") {
		print OUT "double \L$xpartName[$ii]\[$partOutCnt[$ii]\];\n";
		print OUT "float $xpartName[$ii]\_CALC;\n";
	}
	if($partType[$ii] eq "RMS") {
		print OUT "float \L$xpartName[$ii];\n";
		print OUT "static float \L$xpartName[$ii]\_avg;\n";
	}
	if($partType[$ii] eq "SEI_WD1"){
		print OUT "float \L$xpartName[$ii]";
		print OUT "Filt\[20\];\n";
		print OUT "float \L$xpartName[$ii]";
		print OUT "Raw\[20\];\n";
	}
}
print OUT "\n\n";

print OUT "if(feInit)\n\{\n";
for($ii=0;$ii<$partCnt;$ii++)
{
	if($partType[$ii] eq "RMS") {
		print OUT "\L$xpartName[$ii]\_avg = 0\.0;\n";
	}
}
print OUT "\} else \{\n";

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
	$inCnt = $partInCnt[$mm];
	for($qq=0;$qq<$inCnt;$qq++)
	{
		$indone = 0;
		if($partInputType[$mm][$qq] eq "ADC")
		{
			$card = $partInNum[$mm][$qq];
			$chan = $partInputPort[$mm][$qq];
			$fromExp[$qq] = "dWord\[";
			$fromExp[$qq] .= $card;
			$fromExp[$qq] .= "\]\[";
			$fromExp[$qq] .= $chan;
			$fromExp[$qq] .= "\]";
			$indone = 1;
		}
		if($partInputType[$mm][$qq] eq "MATRIX")
		{
			$from = $partInNum[$mm][$qq];
			$fromPort = $partInputPort[$mm][$qq];
			$fromExp[$qq] = "\L$xpartName[$from]";
			$fromExp[$qq] .= "\[1\]\[";
			$fromExp[$qq] .= $fromPort;
			$fromExp[$qq] .= "\]";
			$indone = 1;
		}
		if(($partInputType[$mm][$qq] eq "EPICS_OUTPUT") || ($partInputType[$mm][$qq] eq "EPICS_INPUT"))
		{
			$from = $partInNum[$mm][$qq];
			$fromPort = $partInputPort[$mm][$qq];
			$fromExp[$qq] = "pLocalEpics->";
			$fromExp[$qq] .= $systemName;
			$fromExp[$qq] .= "\.";
			$fromExp[$qq] .= $xpartName[$from];
			$indone = 1;
		}
		if(($partInputType[$mm][$qq] eq "RAMP_SW") || ($partInputType[$mm][$qq] eq "MULTI_SW") || ($partInputType[$mm][$qq] eq "PRODUCT") || ($partInputType[$mm][$qq] eq "DIFF_JUNC"))
		{
			$from = $partInNum[$mm][$qq];
			$fromPort = $partInputPort[$mm][$qq];
			$fromExp[$qq] = "\L$xpartName[$from]";
			$fromExp[$qq] .= "\[";
			$fromExp[$qq] .= $fromPort;
			$fromExp[$qq] .= "\]";
			$indone = 1;
		}
		if($indone == 0)
		{
			$from = $partInNum[$mm][$qq];
			$fromExp[$qq] = "\L$xpartName[$from]";
			#print "$xpartName[$mm]  $fromExp[$qq]\n";
		}
	}
	# ******** FILTER *************************************************************************
	if($partType[$mm] eq "FILT")
	{
	   print OUT "// FILTER MODULE\n";
	   $calcExp = "\L$xpartName[$mm]";
	   $calcExp .= " = ";
	   $calcExp .= "filterModuleD(dspPtr,dspCoeff,";
	   $calcExp .= $xpartName[$mm];
	   $calcExp .= ",";
	   $calcExp .= $fromExp[0];
	   $calcExp .= ",0);\n";
	   print OUT "$calcExp";
	}
	# ******** SEI_WD1 *************************************************************************
	if($partType[$mm] eq "SEI_WD1")
	{
		$zz = 0;
		print OUT "// SEI WD GOES HERE ***\n\n";
		for($qq=0;$qq<$inCnt;$qq+=2)
		{
	   		$calcExp = "\L$xpartName[$mm]";
	   		$calcExp .= "Raw\[";
	   		$calcExp .= $zz;
	   		$calcExp .= "\] = ";
	   		$calcExp .= $fromExp[$qq];
	   		$calcExp .= ";\n";
			print OUT "$calcExp";
			$yy = $qq+1;
	   		$calcExp = "\L$xpartName[$mm]";
	   		$calcExp .= "Filt\[";
	   		$calcExp .= $zz;
	   		$calcExp .= "\] = ";
	   		$calcExp .= $fromExp[$yy];
	   		$calcExp .= ";\n";
			print OUT "$calcExp";
			$zz ++;
		}
		$calcExp = "seiwd1(cycle,\L$xpartName[$mm]";
		$calcExp .= "Raw, \L$xpartName[$mm]";
		$calcExp .= "Filt,\&pLocalEpics->";
		$calcExp .= $systemName;
		$calcExp .= "\.";
		$calcExp .= $xpartName[$mm];
		$calcExp .= ");\n";
			print OUT "$calcExp";
		
	}
	# ******** MATRIX *************************************************************************
	if($partType[$mm] eq "MATRIX")
	{
		print OUT "// MATRIX CALC\n";
		print OUT "for(ii=0;ii<$matOuts[$mm];ii++)\n{\n";
		print OUT "\L$xpartName[$mm]\[1\]\[ii\] = \n";
		
		for($qq=0;$qq<$partInCnt[$mm];$qq++)
		{
			$calcExp = "\tpLocalEpics->$systemName\.";
			$calcExp .= $xpartName[$mm];
			$calcExp .= "\[ii\]\[";
			$calcExp .= $qq;
			$calcExp .= "\] * ";
			$calcExp .= $fromExp[$qq];
			if($qq == ($partInCnt[$mm] - 1))
			{
				$calcExp .= ";\n";
			}
			else
			{
				$calcExp .= " +\n";
			}
			print OUT "$calcExp";
		}
		print OUT "}\n";
	}
	# ******** SUMMING JUNC ********************************************************************
	if($partType[$mm] eq "SUM")
	{
	   print OUT "// SUM\n";
		#print "\tUsed Sum $xpartName[$mm] $partOutCnt[$mm]\n";
		$calcExp = "\L$xpartName[$mm]";
		$calcExp .= " = ";
		for($qq=0;$qq<$inCnt;$qq++)
		{
		    $zz = $qq+1;
		    if(($zz - $inCnt) == 0)
	 	    {
			$calcExp .= $fromExp[$qq];
			$calcExp .= ";\n";
		    }
		    else {
			$calcExp .= $fromExp[$qq];
			$calcExp .= " + ";
		    }
		}
		print OUT "$calcExp";
	}
	# ******** DIFF JUNC ********************************************************************
	if($partType[$mm] eq "DIFF_JUNC")
	{
	   print OUT "// DIFF_JUNC\n";
	   $zz = 0;
           for($qq=0;$qq<16;$qq+=2)
           {

		$yy = $qq + 1;
		$calcExp = "\L$xpartName[$mm]";
		$calcExp .= "\[";
		$calcExp .= $zz;
		$calcExp .= "\] = ";
		$calcExp .= $fromExp[$yy];
		$calcExp .= " - ";
		$calcExp .= $fromExp[$qq];
		$calcExp .= ";\n";
		print OUT "$calcExp";
		$zz ++;
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
	   	print OUT "// EPICS_OUTPUT\n";
			print OUT "pLocalEpics->$systemName\.$xpartName[$mm] = ";
			print OUT "$fromExp[0];\n";
	}
	# ******** DAC OUTPUT ********************************************************************
	if($partType[$mm] eq "DAC")
	{
		$dacNum = substr($xpartName[$mm],4,1);
		print OUT "// DAC number is $dacNum\n";
           for($qq=0;$qq<$inCnt;$qq++)
           {
		$calcExp = "dacOut\[";
		$calcExp .= $dacNum;
		$calcExp .= "\]\[";
		$calcExp .= $partOutputPort[$mm][$qq];
		$calcExp .= "\] = ";
		$calcExp .= $fromExp[$qq];
		$calcExp .= ";\n";
		print OUT "$calcExp";
	   }
	}
	# ******** MULTI_SW ************************************************************************
	if($partType[$mm] eq "MULTI_SW")
	{
	   print OUT "// MULTI_SW\n";
           for($qq=0;$qq<$inCnt;$qq++)
           {
		$calcExp = "\L$xpartName[$mm]";
		$calcExp .= "\[";
		$calcExp .= $qq;
		$calcExp .= "\] = ";
		$calcExp .= $fromExp[$qq];
		$calcExp .= ";\n";
		print OUT "$calcExp";
	   }
		print OUT "if(pLocalEpics->$systemName\.$xpartName[$mm] == 0)\n";
		print OUT "{\n";
		print OUT "\tfor(ii=0;ii< $partOutCnt[$mm];ii++) \L$xpartName[$mm]\[ii\] = 0.0;\n";
		print OUT "}\n\n";
	}
	# ******** DELAY ************************************************************************
	if($partType[$mm] eq "DELAY")
	{
	   print OUT "// DELAY\n";
		$calcExp = "\L$xpartName[$mm]";
		$calcExp .= " = ";
		$calcExp .= $fromExp[0];
		$calcExp .= ";\n";
		print OUT "$calcExp";
	}
	# ******** FIR_FILTER ************************************************************************
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
	# ******** RAMP_SW ************************************************************************
	if($partType[$mm] eq "RAMP_SW")
	{
	   print OUT "// RAMP_SW\n";
		$calcExp = "\L$xpartName[$mm]";
		$calcExp .= " = ";
		$calcExp .= $fromExp[0];
		$calcExp .= ";\n";
	   for($qq=0;$qq<$inCnt;$qq++)
	   {
		$calcExp = "\L$xpartName[$mm]\[";
		$calcExp .= $qq;
		$calcExp .= "\] = ";
		$calcExp .= $fromExp[$qq];
		$calcExp .= ";\n";
		print OUT "$calcExp";
	   }
	   print OUT "if(pLocalEpics->$systemName\.$xpartName[$mm] == 0)\n";
		print OUT "{\n";
		print OUT "\t\L$xpartName[$mm]\[1\] = \L$xpartName[$mm]\[2\];\n";
		print OUT "}\n";
		print OUT "else\n";
		print OUT "{\n";
		print OUT "\t\L$xpartName[$mm]\[0\] = \L$xpartName[$mm]\[1\];\n";
		print OUT "\t\L$xpartName[$mm]\[1\] = \L$xpartName[$mm]\[3\];\n";
		print OUT "}\n\n";
	}
	# ******** PRODUCT ************************************************************************
	if($partType[$mm] eq "PRODUCT")
	{
	   	print OUT "// PRODUCT\n";
	   	print OUT "pLocalEpics->$systemName\.$xpartName[$mm]";
	   	print OUT "_RMON = \n\tgainRamp(pLocalEpics->$systemName\.$xpartName[$mm],";
	   	print OUT "pLocalEpics->$systemName\.$xpartName[$mm]";
	   	print OUT "_TRAMP,";
	   	print OUT "$gainCnt\,\&$xpartName[$mm]\_CALC);";
	   	print OUT "\n\n";
	   	for($qq=0;$qq<$inCnt;$qq++)
	   	{
			$calcExp = "\L$xpartName[$mm]";
			$calcExp .= "\[";
			$calcExp .= $qq;
			$calcExp .= "\] = ";
			$calcExp .= "$xpartName[$mm]";
			$calcExp .= "_CALC * ";
			$calcExp .= $fromExp[$qq];
			$calcExp .= ";\n";
			print OUT "$calcExp";
	   	}
		$gainCnt ++;
	}
	# ******** RMS ************************************************************************
	if($partType[$mm] eq "RMS")
	{
	   	print OUT "// RMS\n";
		$calcExp = "\L$xpartName[$mm]";
		$calcExp .= " = ";
		$calcExp .= $fromExp[0];
		$calcExp .= ";\n";
		print OUT "$calcExp";
		$calcExp = "if(\L$xpartName[$mm]\ \> 2000\) \L$xpartName[$mm] = 2000;\n";
		print OUT "$calcExp";
		$calcExp = "if(\L$xpartName[$mm]\ \< -2000\) \L$xpartName[$mm] = 2000;\n";
		print OUT "$calcExp";
		$calcExp = "\L$xpartName[$mm] = \L$xpartName[$mm] * \L$xpartName[$mm];\n";
		print OUT "$calcExp";
		$calcExp = "\L$xpartName[$mm]\_avg = \L$xpartName[$mm] * \.00005 + ";
		$calcExp .= "\L$xpartName[$mm]\_avg * 0\.99995;\n";
		print OUT "$calcExp";
		$calcExp = "\L$xpartName[$mm] = lsqrt(\L$xpartName[$mm]\_avg);\n";
		print OUT "$calcExp";
	}


print OUT "\n";
}
print OUT "  }\n";
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
if($systemName eq "sei")
{
print OUTM "CFLAGS += -DFIR_FILTERS\n";
}
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
for($ii=0;$ii<$useWd;$ii++)
{
	print OUTME "SRC += src/epics/seq/hepiWatchdog";
	print OUTME "\U$useWdName[$ii]";
	print OUTME "\L\.st\n";
}
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
for($ii=0;$ii<$useWd;$ii++)
{
print OUTME "SEQ += \'";
print OUTME "hepiWatchdog";
print OUTME "\U$useWdName[$ii]";
print OUTME ",(\"ifo=M1, sys=\U$systemName\,\Lsubsys=\U$useWdName[$ii]\")\'\n";
}
print OUTME "\n";
print OUTME "CFLAGS += -D";
print OUTME "\U$skeleton";
print OUTME "_CODE\n";
print OUTME "\n";
if($systemName eq "sei")
{
print OUTME "CFLAGS += -DFIR_FILTERS\n";
}
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
