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
open(OUT,">./".$cFile) || die "cannot open c file for writing $cFile";
open(OUTM,">./".$mFile) || die "cannot open Makefile file for writing";
open(OUTME,">./".$meFile) || die "cannot open EPICS Makefile file for writing";
open(OUTH,">./".$hFile) || die "cannot open header file for writing";
open(IN,"<../simLink/".$ARGV[0]) || die "cannot open mdl file $ARGV[0]\n";
$diag = "diags\.txt";
$filtFile = "gds\.txt";
open(OUTD,">./".$diag) || die "cannot open diag file for writing";
open(OUTG,">./".$filtFile) || die "cannot open diag file for writing";

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
$busPort = -1;
$oscUsed = 0;

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

# set debug level (0 - no debug messages)
$dbg_level = 2;

# Print debug message
# Example:
# debug (0, "debug test: openBrace=$openBrace");
#
sub debug {
  if ($dbg_level > shift @_) {
	print @_, "\n";
  }
}

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
	#print "System found on line $lcntr\n";
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
	if($blockType eq "Product") {$partType[$partCnt] = MULTIPLY;}
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
                       	$partSysFromx[$ii][($conDesPort-1)] = -1;
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
		$subNum = $ii;
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
		$partOutput[$ii][0] = $conDes;
		$partOutputPort[$ii][0] = $conDesPort;
		$conMade[$ii] = 1;
		}
		if($conDes eq $xpartName[$ii])
		{
			$qq = $conDesPort - 1;
			$partInput[$ii][$qq] = $conSrc;
			$partInputPort[$ii][$qq] = $conDesPort - 1;
			# $partInput[$ii][$partInCnt[$ii]] = $conSrc;
			# $partInputPort[$ii][$partInCnt[$ii]] = $conDesPort - 1;
                       	# $partSysFromx[$ii][($conDesPort - 1)] = $subNum;
			if(substr($conDes,0,3) eq "Dac")
			{
				$partOutputPort[$ii][$partInCnt[$ii]] = $conDesPort - 1;
				#print "$xpartName[$ii] $partInCnt[$ii] has link to $partInput[$ii][$partInCnt[$ii]] $partOutputPort[$ii][$partInCnt[$ii]]\n";
			}
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
		if(($conDes eq $xpartName[$ii]) && ($partType[$ii] ne "BUSS"))
		{
			$qq = $conDesPort-1;
			$partInput[$ii][$qq] = $conSrc;
			$partInputPort[$ii][$qq] = $conSrcPort-1;
                       	$partSysFromx[$ii][$qq] = -1;
			# $partInput[$ii][$partInCnt[$ii]] = $conSrc;
			# $partInputPort[$ii][$partInCnt[$ii]] = $conSrcPort-1;
                       	# $partSysFromx[$ii][$partInCnt[$ii]] = -1;
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
    #Check if MULT function is really a DIVIDE function. 
    if(($inBlock == 1) && ($var1 eq "Inputs") && ($var2 eq "*\/") && ($partType[$partCnt] eq "MULTIPLY")){
	#print "$var2";
	$partType[$partCnt] = "DIVIDE";
	#print "Found a DIVIDE with name $xpartName[$partCnt]\n";
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
    if(($inBlock == 1) && ($var1 eq "Name") && ($partType[$partCnt] eq "BUSS") && ($busPort >= 0)){
	$partInput[$partCnt][$busPort] = $var2;
	# print "BUSS X $partInCnt[$partCnt] Y $busPort $partInput[$partCnt][$busPort]\n";
	$partInCnt[$partCnt] ++;
	# print "BUSS X $partCnt $partInCnt[$partCnt]\n";
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
	if ($r eq "cdsSwitch" || $r eq "cdsSusSw2") { $r = "MultiSwitch"; }
	elsif ($r =~ /^Matrix/) { $r = "Matrix"; }
	elsif ($r eq "dsparch4" ) { $r = "Filt"; }
	elsif ($r eq "cdsWD" ) { $r = "Wd"; }
	elsif ($r =~ /^cds/) {
		# Getting rid of the leading "cds"
		($r) = $r =~ m/^cds(.+)$/;
	}
	# Capitalize first character
	$r = uc(substr($r,0, 1)) . substr($r, 1);
	# END part name transformation code
	   
	require "lib/$r.pm";
	$partType[$partCnt] = ("CDS::" . $r . "::partType") -> ();
	$cdsPart[$partCnt] = 1;
	$partErr = 0;

if (0) {
	if (substr($var2,0,8) eq "cdsSusWd") {
		$partType[$partCnt] = SUS_WD;
		$partErr = 0;
	}
	if (substr($var2,0,5) eq "cdsWD") {
		$partType[$partCnt] = SUS_WD1;
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
	if (substr($var2,0,6) eq "cdsOsc") {
		$partType[$partCnt] = Osc;
		print "$partCnt is type Osc\n";
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
	if (substr($var2,0,8) eq "cdsPhase") {
		$partType[$partCnt] = PHASE;
		print "$partCnt is type PHASE\n";
		$partErr = 0;
	}
	if (substr($var2,0,11) eq "cdsWfsPhase") {
		$partType[$partCnt] = WFS_PHASE;
		print "$partCnt is type WFS_PHASE\n";
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
		for($qq=0;$qq<16;$qq++)
		{
			$partInput[$partCnt][$qq] = "NC";
		}
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
		$filterName[$filtCnt] = $xpartName[$partCnt];
		$filtCnt ++;
		$partErr = 0;
	}
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
for($ii=0;$ii<$dacCnt;$ii++)
{
	$partInCnt[$dacPartNum[$ii]] = 16;
}
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
		# $partInCnt[$ii] -= 2;
	}
	for($jj=0;$jj<$partInCnt[$ii];$jj++)
	{
	   for($kk=0;$kk<$partCnt;$kk++)
	   {
		if($partInput[$ii][$jj] eq $xpartName[$kk])
		{
			$qq = $partOutputPort[$kk][$partInputPort[$ii][$jj]];
			$partInNum[$ii][$jj] = $kk;
			$partInputType[$ii][$jj] = $partType[$kk];
                       	$partSysFromx[$xx][$jj] = -1;
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
			# print "\t Maybe $xpartName[$xx] port $partOutputPort[$ii][$jj] $xpartName[$fromNum] $partType[$fromNum] $fromPort\n";
				# Make output connection at source part
				$partOutput[$fromNum][$fromPort] = $xpartName[$xx];
				$partOutputType[$fromNum][$fromPort] = $partType[$xx];
				$partOutNum[$fromNum][$fromPort] = $xx;
				$partOutputPort[$fromNum][$fromPort] = $partOutputPort[$ii][$jj];
                       		# $partSysFromx[$xx][$fromCnt[$xx]] = $partSubNum[$ii];
				# Make input connection at destination part
				$qq = $partOutputPort[$ii][$jj] - 1;
				$partInput[$xx][$qq] = $xpartName[$fromNum];
				$partInputType[$xx][$qq] = $partType[$fromNum];
				$partInNum[$xx][$qq] = $fromNum;
				$partInputPort[$xx][$qq] = $fromPort;
                       		$partSysFromx[$xx][$qq] = $partSubNum[$ii];
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
				$partInputType[$kk][0] = "Adc";
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
					$partInputType[$toNum][$toPort] = "Adc";
					#print "\tNew adc connect $xpartName[$toNum] $adcNum $adcChan to partnum $partInput[$toNum][$toPort]\n";
				}
				}
			}
			if($partType[$kk] ne "INPUT")
			{
				if(($partOutput[$xx][$jj] eq $xpartName[$kk]))
				{
				# print "Found ADC NP connect $xpartName[$xx] to $xpartName[$kk] $partOutputPort[$xx][$jj]\n";
				print "$jj $partOutputPort[$xx][$jj] $partOutputPortUsed[$xx][$jj]\n";
				$fromNum = $xx;
				#$fromPort = $jj;
				#$fromPort = $partInputPort[$kk][0];
				#$fromPort = $partOutputPort[$xx][$jj];
				$fromPort = $partOutputPortUsed[$xx][$jj];
				$partInput[$kk][0] = $xpartName[$xx];
				$partInputType[$kk][0] = "Adc";
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
	   if($partSysFromx[$xx][$jj] == -1)
	   {
		print OUTD "\t$partInput[$xx][$jj]\t$partInputType[$xx][$jj]\t$partInNum[$xx][$jj]\t$partInputPort[$xx][$jj] subsys NONE\n";
	   }
	   else
	   {
		print OUTD "\t$partInput[$xx][$jj]\t$partInputType[$xx][$jj]\t$partInNum[$xx][$jj]\t$partInputPort[$xx][$jj] subsys $partSysFromx[$xx][$jj]\n";
	   }
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
		if(($partType[$jj] eq "INPUT") || ($partType[$jj] eq "GROUND") || ($partType[$jj] eq "EpicsIn"))
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

#
$old_style_multiprocessing = 1;

# Total number of CPUs available to us
$cpus = 2;

# subSysName -> step*10 + cpu
# 'step' is the processing step from one  sync point to the next
# 'cpu' is the processor number: 0, 1 ... $cpus
%sys_cpu_step;

# Current processing step (a running counter)
$cur_step = 1;

# How many processors are available on the current processing step
# Running counter
$cur_step_cpus = $cpus;

# Parts tree root name
$tree_root_name = "__root__";

# Parts "tree" root
# It points to ADC input groups (they are grouped for each subsystem)
$root = {
	NAME => $tree_root_name,
	NEXT => [], # array of references to leaves
};

# Run a function on all tree leaves
# Arguments:
#	tree root reference
#	function reference
#
sub do_on_leaves {
	my($tree, $f) = @_;
	if (0 == @{$tree->{NEXT}}) {
		&$f($tree);
	} else {
		for (@{$tree->{NEXT}}) {
                	do_on_leaves($_, $f);
        	}
	}
}

# Run a function on all tree nodes
# Arguments:
#	tree root reference
#	function reference
#	function argument
sub do_on_nodes {
	my($tree, $f, $arg) = @_;
	&$f($tree, $arg);
	for (@{$tree->{NEXT}}) {
               	do_on_nodes($_, $f, $arg);
	}
}

# Find graph node by name
#
sub find_node {
	my($tree, $name) = @_;
	if ($tree->{NAME} eq $name) {
		return $tree;
	} else {
		for (@{$tree->{NEXT}}) {
			#print $_->{NAME}, "\n";
                	$res = find_node($_, $name);
			if ($res) {
				return $res;
			}
        	}
	}
	return undef;
}

# Construct parts linked list
#
foreach $i (0 .. $subSys-1) {
	debug(0, "Subsystem $i ", $subSysName[$i]);
}

if ($cpus > 2) {
foreach $i (0 .. $subSys-1) {
	debug(2, "Subsystem $i (", $subSysName[$i], "):");
	debug(2, " inputs=", $subCntr[$i]);
	debug(2, "  parts=", $seqParts[$i]);
	# See if this subsystem has ADC inputs
	$has_inputs = 0;
	for ($subSysPartStart[$i] .. $subSysPartStop[$i]) {
		if ($partType[$_] eq "INPUT" && $partInputType[$_][0] eq "Adc") {
			$has_inputs = 1;
			break;
		}
	}

	# insert new tree node for the group of inputs
	# and the input parts afterwards
	if ($has_inputs) {
		$inputs_node = {
			NAME => $subSysName[$i] . "_INPUTS",
			TYPE => "input_group",
			NEXT => [],
			PART => -1,
			SUBSYS => $i,
		};
		push @{$root->{NEXT}}, $inputs_node;

		# Insert individual ADC inputs after the input group
		for ($subSysPartStart[$i] .. $subSysPartStop[$i]) {
			if ($partInput[$_][0] =~ m/adc_(\d+)_(\d+)/
			    && $partInputType[$_][0] eq "Adc") {
				$node = {
					NAME => $xpartName[$_],
					TYPE => $partType[$_],
					NEXT => [],
					PART => $_,
					SUBSYS => $i,
				};
				push @{$inputs_node->{NEXT}}, $node;
			}
		}
	}

	# Insert all ground inputs into the root
	for ($subSysPartStart[$i] .. $subSysPartStop[$i]) {
		if ($partType[$_] eq "GROUND") {
			$node = {
				NAME => $xpartName[$_],
				TYPE => $partType[$_],
				NEXT => [],
				PART => $_,
				SUBSYS => $i,
			};
			push @{$root->{NEXT}}, $node;
		}
	}

	debug(2, "  {");
	foreach $j ($subSysPartStart[$i] .. $subSysPartStop[$i]) {
		debug(2, "\tPart $j ", $xpartName[$j], " type=", $partType[$j], " inputs=", $partInCnt[$j], " outputs=", $partOutCnt[$j]);
		
		#debug(2, "\tPart Name ", $seqName[$i][$j]);
		#debug(2, "\tPart Type ", $partType[$i][$j]);
		#debug(2, "\t    seq=", $seq[$i][$j]);
	}
	debug(2, "  }");
}
}



# Recursively insert a node into the tree
#
sub insert_node {
my($tree, $pnum) = @_;

# See if the node is in the tree already and return
if (find_node($root, $xpartName[$pnum])) {
	return;
}

#debug 1, "Node ", $xpartName[$pnum], " #", $pnum, " type=", $partType[$pnum], " not in the tree";

# Make sure each input node is in the tree before inserting current node
for (0 .. $partInCnt[$pnum] - 1) {
	if ($partType[$pnum] eq "INPUT") {
		debug 1, "Input Node";
	} elsif ($partInput[$pnum][$_] =~ m/adc_(\d+)_(\d+)/) {
		debug 1, "\tadc input node";
		debug 1, "\t\t $partInput[$pnum][$_]\t input_type=$partInputType[$pnum][$_]\t$partInNum[$pnum][$_]";
		# create a node for this adc input and then create a link to it from
		# the corresponding second level node
	} else {
		#debug 1, "\t Recursively insert node $partInput[$pnum][$_]\t input_type=$partInputType[$pnum][$_]\t$partInNum[$pnum][$_]";
		insert_node($tree, $partInNum[$pnum][$_]);
	}
}

# At this point all input nodes are alredy resolved
# Insert this node then.
# Find in which subsystem this node is
#
my $subsys = -1;

foreach $i (0 .. $subSys-1) {
	if ($subSysPartStart[$i] <= $pnum && $pnum <  $subSysPartStop[$i]) {
		$subsys = $i;
		break;
	}
}

$my_node = {
	NAME => $xpartName[$pnum],
	TYPE => $partType[$pnum],
	NEXT => [],
	PART => $pnum,
	SUBSYS => $subsys,
};

# Find each input in the tree and update its NEXT field with the pointer to the node

for (0 .. $partInCnt[$pnum] - 1) {
	my $node;
	if (($node = find_node($tree, $partInput[$pnum][$_]))) {
		push @{$node->{NEXT}}, $my_node;
	} else {
		die "Node not found: $partInput[$pnum][$_]\n";
	}
}
}

if ($cpus > 2) {

# Insert all parts into the tree
#
foreach $i (0 .. $subSys-1) {
	for ($subSysPartStart[$i] .. $subSysPartStop[$i]-1) {
	  if ($partType[$_] eq "INPUT") {
		next;
	  }
	  insert_node($root, $_);
	}
}

# Print all terminator nodes
#do_on_leaves($root, sub {if ($_->{PRINTED} != 1) {print $_->{NAME}, "\n"; $_->{PRINTED} = 1; }});

# Print the tree
$print_no_repeats = 0;
sub print_tree {
	my($tree, $level) = @_;
	my($space);
	if ($print_no_repeats && $tree->{PRINTED}) { return; }
	for (0 .. $level) { $space .= ' '; }
	debug 0, $space, "{", $tree->{NAME}, " nref=", scalar @{$tree->{NEXT}}," subsys=", $tree->{SUBSYS}," type=", $tree->{TYPE}, "}";
	for (@{$tree->{NEXT}}) {
		print_tree($_, $level + 1);
	}
	if ($print_no_repeats) { $tree->{PRINTED} = 1; }
}

#debug(0, "Tree:\n");
#print_tree($root);
#debug(0, "\n");


# Tag each node with the subsystem number
sub tag_node {
	my ($node, $subsys) = @_;

	# see if this node is already tagged
	#
	for (@{$node->{INPUTS}}) {
		if ($_ == $subsys) {
			return; # already tagged
		}
	}

	push @{$node->{INPUTS}}, $subsys;
	#print "Tagged ", $node->{NAME}, " with $subsys\n";
}

for (@{$root->{NEXT}}) {
	if ($_->{NAME} =~ m/.*INPUTS/) {
		my $subsys = $_->{SUBSYS};
		#print "Marking ", $_->{NAME}, " with $subsys\n";
		do_on_nodes($_, \&tag_node, $subsys);
	}
}

#do_on_nodes($root, sub {if ($_->{PRINTED} != 1) {print $_->{NAME}, " ", @{$_->{INPUTS}}, "\n"; $_->{PRINTED} = 1; }});

# See how many different groups we have got
%prog_groups;
%prog_group_fmods; # How many filter modules in a group

sub find_prog_groups {
	my ($node) = @_;
	my $nm;
	if ($node->{VISITED}) { return; }
	$node->{VISITED} = 1;
	for (sort @{$node->{INPUTS}}) {
		if (length $nm) { $nm .= "_"; }
		$nm .= $_;
	}
	$prog_groups{$nm}++;
	if ($node->{TYPE} eq "Filt") {
		$prog_group_fmods{$nm}++;
	}
	$node->{GROUP} = $nm;
}

sub reset_visited { 
	my ($node) = @_;
	$node->{VISITED} = 0;
}


do_on_nodes($root,\&reset_visited);
do_on_nodes($root,\&find_prog_groups);


sub processing_order {
	my $idx = index $a, $b;
	#print "processing_order $a $b\n";
	if ($a eq $b) { return 0; }
	if ($idx >= 0) { return 1; }
	$idx = index $b, $a;
	if ($idx >= 0) { return -1; }
	# independent, go by length
	return length $a <=> length $b;
}

print "\n";
for (sort processing_order keys %prog_groups) {
	#print "cluster=$_ -> parts=", $prog_groups{$_}, " fmods=", $prog_group_fmods{$_}, "\n";
}


# Construct cluster tree
$root1 = {
	NAME => $tree_root_name,
	NEXT => [], # array of references to leaves
};

sub get_nums_array {
	my ($str) = @_;
	return @ret = $str =~ m/(\d+)/g;
}

sub add_refs {
	my ($node, $ref_node) = @_;
	if ($node->{VISITED}) { return; }
	if ($ref_node->{NAME} eq $node->{NAME}) { return; }
	#@ref_node_nums = $ref_node->{NAME} =~ m/(\d+)/g;
	#@node_nums =  $node->{NAME} =~ m/(\d+)/g;
	@ref_node_nums =  get_nums_array($ref_node->{NAME});
	@node_nums =  get_nums_array($node->{NAME});
	#print "ref node '", $ref_node->{NAME} ,"' nums: ", @ref_node_nums, "\n";
	#print "node '", $node->{NAME}, "' nums: ", @node_nums, "\n";
	$pass = 1;
	for (@node_nums) {
		$in_it = 0;
		$cur_num = $_;
		for (@ref_node_nums) {
			if ($cur_num == $_) {
				$in_it = 1;
				break;
			}
		}
		if (!$in_it) {
			$pass = 0;
			break;
		}
	}
	if ($pass) {
		push @{$node->{NEXT}}, $ref_node;
		push @{$ref_node->{PARENTS}}, $node;
	}
	$node->{VISITED} = 1;
}

for (sort processing_order keys %prog_groups) {
	if (length($_) == 0) { next; }
	$node = {
		NAME => $_,
		NEXT => [],
		PARTS => $prog_groups{$_},
		FILTERS => $prog_group_fmods{$_},
	};
	# initial node
	if (index($_, "_") == -1) {
		#print "Initial node $_\n";
		push @{$root1->{NEXT}}, $node;
		push @{$node->{PARENTS}}, $root1;
	} else {
		do_on_nodes($root1,\&reset_visited);
		$root1->{VISITED} = 1;
		do_on_nodes($root1, \&add_refs, $node);
	}
}


$exit_node_name = "__exit__";
$exit_node = {
	NAME => $exit_node_name,
	NEXT => [], # array of references to leaves
};

# Insert exit node
for (sort processing_order keys %prog_groups) {
	if (length($_) == 0) { next; }
	$node = find_node($root1, $_);
	die "Node $_ not found\n" unless $node != undef;
	if (scalar @{$node->{NEXT}} == 0) {
		push @{$node->{NEXT}}, $exit_node;
		push @{$exit_node->{PARENTS}}, $node;
	}
}

# Calculate t-levels for each node
for ($tree_root_name, sort processing_order keys %prog_groups, $exit_node_name) {
	if (length($_) == 0) { next; }
	$node = find_node($root1, $_);
	die "Node $_ not found\n" unless $node != undef;
	#print "Node $_ has parents: ";
	$max_t_level = 0;
	for (@{$node->{PARENTS}}) {
		#print $_->{NAME}, " ";
		$pt = $_->{T_LEVEL} + $_->{FILTERS};
		if ($pt > $max_t_level) {
			$max_t_level = $pt;
		}
	}
	$node->{T_LEVEL} = $max_t_level;
	#print "t_level=", $node->{T_LEVEL}, "\n";
}

# Calculate b-levels for each node
for ($exit_node_name, reverse(sort processing_order keys %prog_groups), $tree_root_name) {
	if (length($_) == 0) { next; }
	$node = find_node($root1, $_);
	die "Node $_ not found\n" unless $node != undef;
	$max_b_level = 0;
	for (@{$node->{NEXT}}) {
		if ($_->{B_LEVEL} > $max_b_level) {
			$max_b_level = $_->{B_LEVEL};
		}
	}
	$node->{B_LEVEL} = $max_b_level + $node->{FILTERS};
	#print "Node ", $node->{NAME} ," b_level=", $node->{B_LEVEL}, "\n";
}

sub b_level_order {
	if (length($a) == 0) { return 0; }
	if (length($b) == 0) { return 0; }
	$anode = find_node($root1, $a);
	$bnode = find_node($root1, $b);
	die "Node $_ not found\n" unless $anode != undef && $bnode != undef;
	#print $anode->{B_LEVEL}, " ", $bnode->{B_LEVEL}, "\n";
	return $bnode->{B_LEVEL} <=> $anode->{B_LEVEL};
}

print "-----------------------\n";
print "CPU allocation schedule\n";
print "---------------------------------------------------\n";
print "Node\tB-level\tcpu\tlevel\tweight\tlevel time\n";
print "---------------------------------------------------";
# Assign clusters to CPUs (b-level based list algorithm)
$level = 0;
$cpu = 1; # goes from 1 to $cpus
$max_level_time = 0;
$total_time = 0;
$cum_time = 0;
@sorted_clusters = sort b_level_order keys %prog_groups;
@running_clusters = (); # currently running clusters (on CPUs)
@saved_nodes = ();
while(scalar @sorted_clusters) {
	$_ = shift @sorted_clusters;
	if (length($_) == 0) { next; }
	$node = find_node($root1, $_);
	die "\nNode $_ not found\n" unless $node != undef;
	# See if just popped cluster can be run on this processing level
	$repeat = 0;
	for (@running_clusters) {
		if (find_node(find_node($root1, $_), $node->{NAME}) != undef) {
			# Current node depends on this running cluster...
			unshift @saved_nodes, $node->{NAME};
			$repeat = 1;
			break;
		}
	}
	if ($repeat) {
		next;
	} else {
		for (@saved_nodes) {
			unshift @sorted_clusters, $_;
		}
		@saved_nodes = ();
	}
	push @{$cpu_clusters[$cpu]}, $_;
	$node->{CPU} = $cpu;
	$node->{LEVEL} = $level;
	push @running_clusters, $_;
	print "\n", $node->{NAME} ,"\t", $node->{B_LEVEL}, "\t", $cpu, "\t", $level, "\t", $node->{FILTERS};
	$cpu++;
	$cum_time += $node->{FILTERS};
	if ($node->{FILTERS} > $max_level_time) {
		$max_level_time = $node->{FILTERS};
	}
	if ($cpu == $cpus) {
		$cpu = 1;
		print "\t", $max_level_time;
		@running_clusters = ();
		$total_time += $max_level_time,;
		$max_level_time = 0;
		$level++;
	}
}

if ($cpu != 1) {
	print "\t", $max_level_time;
	$total_time += $max_level_time,;
}
print "\n---------------------------------------------------\n";
print "Total time is ", $total_time, "\n";
print "Cumulative time is ", $cum_time, "(", $cum_time/($cpus - 1), " per cpu)\n";
print "TODO: need to eliminate unnecessary CPU sync points (merge levels)\n";
#$print_no_repeats = 1;
#print_tree($root1);

# Check all nodes are scheduled
for (sort processing_order keys %prog_groups) {
	if (length($_) == 0) { next; }
	$node = find_node($root1, $_);
	die "Node $_ not found\n" unless $node != undef;
	die "Node $_ was not scheduled for execution\n" unless $node->{CPU};
}

} # If $cpus > 2

# First pass defines processing step 0
# It finds all input sybsystems

for($ii=0;$ii<$subSys;$ii++)
{
$subUsed[$ii] = 0;
$allADC = 1;
	for($jj=0;$jj<$subCntr[$ii];$jj++)
	{
		if($subInputsType[$ii][$jj] ne "Adc")
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
		if ($cur_step_cpus == 1) { $cur_step_cpus = $cpus; }
		$sys_cpu_step{$subSysName[$ii]} = --$cur_step_cpus;
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
			if(($partInputType[$xx][$jj] ne "Adc") && ($partInputType[$xx][$jj] ne "DELAY"))

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
			if ($cur_step_cpus == 1) { $cur_step_cpus = $cpus; }
			$sys_cpu_step{$xpartName[$xx]} = --$cur_step_cpus;
		}
	}
}
print "first pass done $partsRemaining $subRemaining\n";

# Second multiprocessing step
$cur_step = 1;
$cur_step_cpus = $cpus;

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
					if(($partUsed[$yy] != 1) && ($subInputsType[$ii][$jj] ne "Adc") && ($partType[$yy] ne "DELAY"))
					{
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
				if ($cur_step_cpus == 1) { $cur_step_cpus = $cpus; }
				$sys_cpu_step{$subSysName[$ii]} = --$cur_step_cpus + 10 * $cur_step;
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
				if($yy < 0)
				{
					
					$zz = $partInNum[$xx][$jj];
					if(!$partUsed[$zz])
					{
						$allADC = 0;
					}
				}
				else {
				if(($subUsed[$yy] != 1) && ($partInputType[$xx][$jj] ne "Adc"))
				{
						$allADC = 0;
				}
				}
			}
			if($allADC == 1) {
				#print "Part $xx $xpartName[$xx] can go next\n";
				$partUsed[$xx] = 1;
				$partsRemaining --;
				$seqList[$seqCnt] = $xx;
				$seqType[$seqCnt] = "PART";
				$seqCnt ++;
				if ($cur_step_cpus == 1) { $cur_step_cpus = $cpus; }
				$sys_cpu_step{$xpartName[$xx]} = --$cur_step_cpus + 10* $cur_step;
			}
		}
	}
}
$processCnt = 0;
$processSeqCnt = 0;
for($ii=0;$ii<$seqCnt;$ii++)
{
	#print "$ii $seqList[$ii] $seqType[$ii] $seqParts[$seqList[$ii]]\n";
	$processSeqStart[$processSeqCnt] = $processCnt;
	$processSeqCnt ++;
	if($seqType[$ii] eq "SUBSYS")
	{
		$xx = $seqList[$ii];
		# Save the name for subsystem for later use in code gennerator
		$processSeqSubsysName[$processCnt] = $subSysName[$xx];
		$processSeqType{$subSysName[$xx]} = $seqType[$ii];
		for($jj=0;$jj<$seqParts[$xx];$jj++)
		{
			$processName[$processCnt] = $seqName[$xx][$jj];
			$processPartNum[$processCnt] = $seq[$xx][$jj];
			#print "$processCnt $processName[$processCnt] $processPartNum[$processCnt]\n";
			$processCnt ++;
		}
	}
	if($seqType[$ii] eq "PART")
	{
		$xx = $seqList[$ii];
		$processName[$processCnt] = $xpartName[$xx];
		$processSeqType{$xpartName[$xx]} = $seqType[$ii];
		$processPartNum[$processCnt] = $xx;
		#print "$processCnt $processName[$processCnt] $processPartNum[$processCnt]\n";
		$processSeqSubsysName[$processCnt] = "__PART__";
		$processCnt ++;
	}
	$processSeqEnd[$processSeqCnt] = $processCnt;
}
print "Total of $processCnt parts to process\n";

#print "CPU_STEPS:\n";
#while (($k, $v) = each %sys_cpu_step) {
#	print "$k => $v\n";
#}

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
	if ($cdsPart[$ii]) {
	  ("CDS::" . $partType[$ii] . "::printHeaderStruct") -> ($ii);
	}

if (0) {
	if($partType[$ii] eq "Matrix") {
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
	if($partType[$ii] eq "Osc") {
		print OUTH "\tfloat $xpartName[$ii]\_FREQ;\n";
		print OUTH "\tfloat $xpartName[$ii]\_CLKGAIN;\n";
		print OUTH "\tfloat $xpartName[$ii]\_SINGAIN;\n";
		print OUTH "\tfloat $xpartName[$ii]\_COSGAIN;\n";
	}
	if($partType[$ii] eq "MultiSwitch") {
		print OUTH "\tint $xpartName[$ii];\n";
	}
	if($partType[$ii] eq "EpicsIn") {
		print OUTH "\tfloat $xpartName[$ii];\n";
	}
	if($partType[$ii] eq "EpicsOut") {
		print OUTH "\tfloat $xpartName[$ii];\n";
	}
	if($partType[$ii] eq "PHASE") {
		print OUTH "\tfloat $xpartName[$ii]\[2\];\n";
	}
	if($partType[$ii] eq "WfsPhase") {
		print OUTH "\tfloat $xpartName[$ii]\[2\]\[2\];\n";
	}
	if($partType[$ii] eq "PRODUCT") {
		print OUTH "\tfloat $xpartName[$ii];\n";
		print OUTH "\tint $xpartName[$ii]\_TRAMP;\n";
		print OUTH "\tint $xpartName[$ii]\_RMON;\n";
	}
	if($partType[$ii] eq "Wd") {
		print OUTH "\tint $xpartName[$ii];\n";
		print OUTH "\tint $xpartName[$ii]_STAT;\n";
		print OUTH "\tint $xpartName[$ii]\_MAX;\n";
		print OUTH "\tfloat $xpartName[$ii]\_VAR\[$partInCnt[$ii]\];\n";
	}
	if($partType[$ii] eq "SUS_WD") {
		print OUTH "\tint $xpartName[$ii];\n";
		print OUTH "\tint $xpartName[$ii]\_MAX;\n";
		print OUTH "\tfloat $xpartName[$ii]\_VAR\[20\];\n";
	}
	if($partType[$ii] eq "SEI_WD1") {
		print OUTH "\tSEI_WATCHDOG $xpartName[$ii];\n";
	}
} # if (0)
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
	if ($cdsPart[$ii]) {
	  ("CDS::" . $partType[$ii] . "::printEpics") -> ($ii);
	}

if (0) {
	if($partType[$ii] eq "MultiSwitch") {
		print EPICS "INVARIABLE $xpartName[$ii] $systemName\.$xpartName[$ii] int bi 0 field(ZNAM,\"OFF\") field(ONAM,\"ON\")\n";
	}
}

	if($partType[$ii] eq "RAMP_SW") {
		print EPICS "INVARIABLE $xpartName[$ii] $systemName\.$xpartName[$ii] int bi 0 field(ZNAM,\"OFF\") field(ONAM,\"ON\")\n";
	}
if (0) {
	if($partType[$ii] eq "EpicsIn") {
		print EPICS "INVARIABLE $xpartName[$ii] $systemName\.$xpartName[$ii] float ai 0 field(PREC,\"3\")\n";
	}
}

	if($partType[$ii] eq "Osc") {
		print EPICS "INVARIABLE $xpartName[$ii]\_FREQ $systemName\.$xpartName[$ii]\_FREQ float ai 0 field(PREC,\"1\")\n";
		print EPICS "INVARIABLE $xpartName[$ii]\_CLKGAIN $systemName\.$xpartName[$ii]\_CLKGAIN float ai 0 field(PREC,\"1\")\n";
		print EPICS "INVARIABLE $xpartName[$ii]\_SINGAIN $systemName\.$xpartName[$ii]\_SINGAIN float ai 0 field(PREC,\"1\")\n";
		print EPICS "INVARIABLE $xpartName[$ii]\_COSGAIN $systemName\.$xpartName[$ii]\_COSGAIN float ai 0 field(PREC,\"1\")\n";
	}
	if($partType[$ii] eq "PHASE") {
		print EPICS "PHASE $xpartName[$ii] $systemName\.$xpartName[$ii] float ai 0 field(PREC,\"3\")\n";
	}
if (0) {
	if($partType[$ii] eq "WfsPhase") {
		print EPICS "WFS_PHASE $xpartName[$ii] $systemName\.$xpartName[$ii] float ai 0 field(PREC,\"3\")\n";
	}
	if($partType[$ii] eq "EpicsOut") {
		print EPICS "OUTVARIABLE $xpartName[$ii] $systemName\.$xpartName[$ii] float ai 0 field(PREC,\"3\")\n";
	}
}
	if($partType[$ii] eq "PRODUCT") {
		print EPICS "INVARIABLE $xpartName[$ii] $systemName\.$xpartName[$ii] float ai 0 field(PREC,\"3\")\n";
		print EPICS "INVARIABLE $xpartName[$ii]\_TRAMP $systemName\.$xpartName[$ii]\_TRAMP int ai 0 field(PREC,\"0\")\n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_RMON $systemName\.$xpartName[$ii]\_RMON int ai 0 field(PREC,\"0\")\n";
	}
if (0) {
	if($partType[$ii] eq "Wd") {
	#	print EPICS "INVARIABLE $xpartName[$ii] $systemName\.$xpartName[$ii] int bi 0 field(ZNAM,\"OFF\") field(ONAM,\"ON\")\n";
		print EPICS "MOMENTARY $xpartName[$ii] $systemName\.$xpartName[$ii] int ai 0\n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_STAT $systemName\.$xpartName[$ii]_STAT int ai 0 \n";
		for (0 .. $partInCnt[$ii]-1) {
		  my $a = 1 + $_;
		  print EPICS "OUTVARIABLE $xpartName[$ii]\_VAR_$a $systemName\.$xpartName[$ii]\_VAR\[$_\] float ai 0 field(PREC,\"1\")\n";
		}
		print EPICS "INVARIABLE $xpartName[$ii]\_MAX $systemName\.$xpartName[$ii]\_MAX int ai 0 field(PREC,\"0\")\n";
	}
}
	if($partType[$ii] eq "SUS_WD") {
		print EPICS "OUTVARIABLE $xpartName[$ii] $systemName\.$xpartName[$ii] int ai 0 \n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_VAR_M0_F1 $systemName\.$xpartName[$ii]\_VAR\[0\] float ai 0 field(PREC,\"1\")\n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_VAR_M0_F2 $systemName\.$xpartName[$ii]\_VAR\[1\] float ai 0 field(PREC,\"1\")\n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_VAR_M0_F3 $systemName\.$xpartName[$ii]\_VAR\[2\] float ai 0 field(PREC,\"1\")\n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_VAR_M0_L $systemName\.$xpartName[$ii]\_VAR\[3\] float ai 0 field(PREC,\"1\")\n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_VAR_M0_R $systemName\.$xpartName[$ii]\_VAR\[4\] float ai 0 field(PREC,\"1\")\n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_VAR_M0_S $systemName\.$xpartName[$ii]\_VAR\[5\] float ai 0 field(PREC,\"1\")\n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_VAR_R0_F1 $systemName\.$xpartName[$ii]\_VAR\[6\] float ai 0 field(PREC,\"1\")\n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_VAR_R0_F2 $systemName\.$xpartName[$ii]\_VAR\[7\] float ai 0 field(PREC,\"1\")\n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_VAR_R0_F3 $systemName\.$xpartName[$ii]\_VAR\[8\] float ai 0 field(PREC,\"1\")\n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_VAR_R0_L $systemName\.$xpartName[$ii]\_VAR\[9\] float ai 0 field(PREC,\"1\")\n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_VAR_R0_R $systemName\.$xpartName[$ii]\_VAR\[10\] float ai 0 field(PREC,\"1\")\n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_VAR_R0_S $systemName\.$xpartName[$ii]\_VAR\[11\] float ai 0 field(PREC,\"1\")\n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_VAR_L1_UL $systemName\.$xpartName[$ii]\_VAR\[12\] float ai 0 field(PREC,\"1\")\n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_VAR_L1_LL $systemName\.$xpartName[$ii]\_VAR\[13\] float ai 0 field(PREC,\"1\")\n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_VAR_L1_UR $systemName\.$xpartName[$ii]\_VAR\[14\] float ai 0 field(PREC,\"1\")\n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_VAR_L1_LR $systemName\.$xpartName[$ii]\_VAR\[15\] float ai 0 field(PREC,\"1\")\n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_VAR_L2_UL $systemName\.$xpartName[$ii]\_VAR\[16\] float ai 0 field(PREC,\"1\")\n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_VAR_L2_LL $systemName\.$xpartName[$ii]\_VAR\[17\] float ai 0 field(PREC,\"1\")\n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_VAR_L2_UR $systemName\.$xpartName[$ii]\_VAR\[18\] float ai 0 field(PREC,\"1\")\n";
		print EPICS "OUTVARIABLE $xpartName[$ii]\_VAR_L2_LR $systemName\.$xpartName[$ii]\_VAR\[19\] float ai 0 field(PREC,\"1\")\n";
		print EPICS "INVARIABLE $xpartName[$ii]\_MAX $systemName\.$xpartName[$ii]\_MAX int ai 0 field(PREC,\"0\")\n";
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
print OUT "\#ifdef SERVO32K\n";
print OUT "\t\#define FE_RATE\t32768\n";
print OUT "\#endif\n";
print OUT "\#ifdef SERVO16K\n";
print OUT "\t\#define FE_RATE\t16382\n";
print OUT "\#endif\n";
print OUT "\n\n";

sub printVariables {
for($ii=0;$ii<$partCnt;$ii++)
{
	if ($cdsPart[$ii]) {
	  ("CDS::" . $partType[$ii] . "::printFrontEndVars") -> ($ii);
	}

if (0) {
	if($partType[$ii] eq "Matrix") {
		print OUT "double \L$xpartName[$ii]\[$matOuts[$ii]\]\[$partInCnt[$ii]\];\n";
	}
}
	if($partType[$ii] eq "SUM") {
		$port = $partInCnt[$ii];
		print OUT "double \L$xpartName[$ii];\n";
	}
	if($partType[$ii] eq "MULTIPLY") {
		$port = $partInCnt[$ii];
		print OUT "double \L$xpartName[$ii];\n";
	}
	if($partType[$ii] eq "DIVIDE") {
		$port = $partInCnt[$ii];
		print OUT "double \L$xpartName[$ii];\n";
	}
	if($partType[$ii] eq "DIFF_JUNC") {
		print OUT "double \L$xpartName[$ii]\[16\];\n";
	}
if (0) {
	if($partType[$ii] eq "Filt") {
		print OUT "double \L$xpartName[$ii];\n";
	}
}
	if($partType[$ii] eq "FIR_FILTER") {
		print OUT "double \L$xpartName[$ii];\n";
	}
	if($partType[$ii] eq "RAMP_SW") {
		print OUT "double \L$xpartName[$ii]\[4\];\n";
	}
if (0) {
	if($partType[$ii] eq "MultiSwitch") {
		print OUT "double \L$xpartName[$ii]\[$partOutCnt[$ii]\];\n";
	}
}
	if($partType[$ii] eq "DELAY") {
		print OUT "static double \L$xpartName[$ii];\n";
	}
	if($partType[$ii] eq "Osc") {
		print OUT "static double \L$xpartName[$ii]\[3\];\n";
		print OUT "static double \L$xpartName[$ii]\_freq;\n";
		print OUT "static double \L$xpartName[$ii]\_delta;\n";
		print OUT "static double \L$xpartName[$ii]\_alpha;\n";
		print OUT "static double \L$xpartName[$ii]\_beta;\n";
		print OUT "static double \L$xpartName[$ii]\_cos_prev;\n";
		print OUT "static double \L$xpartName[$ii]\_sin_prev;\n";
		print OUT "static double \L$xpartName[$ii]\_cos_new;\n";
		print OUT "static double \L$xpartName[$ii]\_sin_new;\n";
		if($oscUsed == 0)
		{
			print OUT "double lsinx, lcosx, valx;\n";
			$oscUsed = 1;
		}
	}
	if($partType[$ii] eq "PHASE") {
		print OUT "static double \L$xpartName[$ii]\[2\];\n";
	}
if (0) {
	if($partType[$ii] eq "WfsPhase") {
		print OUT "static double \L$xpartName[$ii]\[2\];\n";
	}
}
	if($partType[$ii] eq "PRODUCT") {
		print OUT "double \L$xpartName[$ii]\[$partOutCnt[$ii]\];\n";
		print OUT "float $xpartName[$ii]\_CALC;\n";
	}
	if($partType[$ii] eq "RMS") {
		print OUT "float \L$xpartName[$ii];\n";
		print OUT "static float \L$xpartName[$ii]\_avg;\n";
	}
	if($partType[$ii] eq "GROUND") {
		print OUT "static float \L$xpartName[$ii];\n";
	}
if (0) {
	if($partType[$ii] eq "Wd") {
	   print OUT "static double \L$xpartName[$ii];\n";
	   print OUT "static float \L$xpartName[$ii]\_avg\[$partInCnt[$ii]\];\n";
	   print OUT "static float \L$xpartName[$ii]\_var\[$partInCnt[$ii]\];\n";
	   print OUT "float \L$xpartName[$ii]\_vabs;\n";
	}
}
	if($partType[$ii] eq "SUS_WD") {
	   print OUT "float \L$xpartName[$ii];\n";
	   print OUT "static float \L$xpartName[$ii]\_avg\[20\];\n";
	   print OUT "static float \L$xpartName[$ii]\_var\[20\];\n";
	   print OUT "float vabs;\n";
	}
	if($partType[$ii] eq "SEI_WD1"){
		print OUT "float \L$xpartName[$ii]";
		print OUT "Filt\[20\];\n";
		print OUT "float \L$xpartName[$ii]";
		print OUT "Raw\[20\];\n";
	}
}
print OUT "\n\n";
}

if ($cpus > 2) {

# Output multiprocessing functions
print OUT "/* Multi-cpu synchronization primitives */\n";
for ($i = 2; $i < $cpus; $i++) {
	print OUT "volatile int go$i = 0;\n";
	print OUT "volatile int done$i = 0;\n";
}
print OUT "\n";


# Print common to all functions variables as globals
printVariables();

for ($i = 2; $i < $cpus; $i++) {
    print OUT "/* CPU $i code */\n";
    print OUT "void cpu${i}_start(){\n";
    print OUT "\tint ii, jj;\n";
    print OUT "\twhile(!stop_working_threads) {\n";

if ($old_style_multiprocessing) {
    # Processing steps
    for ($step = 0; $step <= $cur_step; $step++) {
      # Wait for signal from main thread
      print OUT "\t\twhile(!go$i && !stop_working_threads);\n";
      print OUT "\t\tgo$i = 0;\n";

      # Processing for this step
      while (($k, $v) = each %sys_cpu_step) {
	#printf "feCode: $k => $v  %d %d\n", $v % 10, $v / 10;
	if ($v == $step*10 + $i) {
  	  &printSubsystem("$k");
	}
      }
      print OUT "\t\tdone$i = 1;\n";
    }
} else {
    for (@{$cpu_clusters[$i]}) {
      # Wait for signal from main thread
      print OUT "\t\twhile(!go$i && !stop_working_threads);\n";
      print OUT "\t\tgo$i = 0;\n";
      &printCluster($_);
      print OUT "\t\tdone$i = 1;\n";
    }
}

    print OUT "\t}\n";
    print OUT "\tdone$i = 1;\n";
    print OUT "}\n\n";
}

sub printCluster {
	my ($cluster) = @_;
	print OUT "/* Do $cluster cluster */\n";
        &printSubsystem(".", $cluster);
}

print OUT "/* CPU 1 code */\n";
}

print OUT "void feCode(int cycle, double dWord[][32],\t\/* ADC inputs *\/\n";
print OUT "\t\tint dacOut[][16],\t\/* DAC outputs *\/\n";
print OUT "\t\tFILT_MOD *dsp_ptr,\t\/* Filter Mod variables *\/\n";
print OUT "\t\tCOEF *dspCoeff,\t\t\/* Filter Mod coeffs *\/\n";
print OUT "\t\tCDS_EPICS *pLocalEpics,\t\/* EPICS variables *\/\n";
print OUT "\t\tint feInit)\t\/* Initialization flag *\/\n";
print OUT "{\n\nint ii,jj;\n\n";
if ($cpus < 3) {
  printVariables();
}
print OUT "if(feInit)\n\{\n";
for($ii=0;$ii<$partCnt;$ii++)
{
	if ( -e "lib/$partType[$ii].pm" ) {
	   print OUT ("CDS::" . $partType[$ii] . "::frontEndInitCode") -> ($ii);
	}	


	if($partType[$ii] eq "RMS") {
		print OUT "\L$xpartName[$ii]\_avg = 0\.0;\n";
	}
	if($partType[$ii] eq "GROUND") {
		print OUT "\L$xpartName[$ii] = 0\.0;\n";
	}


if (0) {
	if($partType[$ii] eq "Osc") {
	   	$calcExp =  "\L$xpartName[$ii]_freq = ";
	   	$calcExp .=  "pLocalEpics->$systemName\.$xpartName[$ii]\_FREQ;\n";
	   	print OUT "$calcExp";
	   	$calcExp =  "printf(\"OSC Freq = \%f\\n\",\L$xpartName[$ii]_freq\);\n";
	   	print OUT "$calcExp";
		$calcExp = "\L$xpartName[$ii]\_delta = 2.0 * 3.1415926535897932384626 * ";
	   	$calcExp .=  "\L$xpartName[$ii]_freq / \UFE_RATE;\n";
	   	print OUT "$calcExp";
		$calcExp = "valx = \L$xpartName[$ii]\_delta \/ 2.0;\n";
	   	print OUT "$calcExp";
		$calcExp = "sincos\(valx, \&lsinx, \&lcosx\);\n";
	   	print OUT "$calcExp";
		$calcExp = "\L$xpartName[$ii]\_alpha = 2.0 * lsinx * lsinx;\n";
	   	print OUT "$calcExp";
		$calcExp = "valx = \L$xpartName[$ii]\_delta\;\n";
	   	print OUT "$calcExp";
		$calcExp = "sincos\(valx, \&lsinx, \&lcosx\);\n";
	   	print OUT "$calcExp";
		$calcExp = "\L$xpartName[$ii]\_beta = lsinx;\n";
	   	print OUT "$calcExp";
		$calcExp = "\L$xpartName[$ii]\_cos_prev = 1.0;\n";
	   	print OUT "$calcExp";
		$calcExp = "\L$xpartName[$ii]\_sin_prev = 0.0;\n";
	   	print OUT "$calcExp";
	}
	if($partType[$ii] eq "Wd") {
	   print OUT "\L$xpartName[$ii] = 0.0;\n";
	   print OUT "for\(ii=0;ii<$partInCnt[$ii];ii++\) {\n";
	   print OUT "\t\L$xpartName[$ii]\_avg\[ii\] = 0.0;\n";
	   print OUT "\t\L$xpartName[$ii]\_var\[ii\] = 0.0;\n";
	   print OUT "}\n";
	   print OUT "pLocalEpics->$systemName\." . $xpartName[$ii] . " = 1;\n";
	}
}
	if($partType[$ii] eq "SUS_WD") {
	   print OUT "for\(ii=0;ii<20;ii++\) {\n";
	   print OUT "\t\L$xpartName[$ii]\_avg\[ii\] = 0.0;\n";
	   print OUT "\t\L$xpartName[$ii]\_var\[ii\] = 0.0;\n";
	   print OUT "}\n";
	}
}
print OUT "\} else \{\n";

#print "*****************************************************\n";

# Print subsystems processing step by step
# along with signalling primitives to other threads
if ($cpus > 2) {
if ($old_style_multiprocessing) {
  for ($step = 0; $step <= $cur_step; $step++) {
    # Signal other threads to go
    for ($i = 2; $i < $cpus; $i++) {
	print OUT "go$i = 1;\n";
    }

    # Print cpu 1 subsystems for $step
    while (($k, $v) = each %sys_cpu_step) {
	#printf "feCode: $k => $v  %d %d\n", $v % 10, $v / 10;
	if ($v == $step*10 + 1) {
  	  &printSubsystem("$k");
	}
    }

    # Wait for other threads to finish this processing step
    for ($i = 2; $i < $cpus; $i++) {
	print OUT "while(!done$i && !stop_working_threads);\n";
	print OUT "done$i = 0;\n";
    }
  }
} else {
    for (@{$cpu_clusters[1]}) {
      # Signal other threads to go
      for ($i = 2; $i < $cpus; $i++) {
	  print OUT "go$i = 1;\n";
      }

      &printCluster($_);

      # Wait for other threads to finish this processing step
      for ($i = 2; $i < $cpus; $i++) {
	print OUT "while(!done$i && !stop_working_threads);\n";
	print OUT "done$i = 0;\n";
      }
    }
}

  # Output standalone parts (DAC output)
  &printSubsystem("__PART__");
} else {
  &printSubsystem(".");
}

sub printSubsystem {
my ($subsys, $cluster) = @_;

$do_cluster = 0;

if (scalar(@_) > 1) {
	$do_cluster = 1;
}

$ts = 0;
$xx = 0;
$do_print = 0;
@cluster_parts = ();

if ($do_cluster) {
	# get all cluster part names
	do_on_nodes($root, \&reset_visited);
	do_on_nodes($root, sub{ if (!$_->{VISITED} && $_->{GROUP} eq $cluster) { push @cluster_parts, $_->{NAME}; $_->{VISITED} = 1;}});

	#print ("Cluster $cluster has these parts: ");
	for (@cluster_parts) {
		#print $_, " ";
	}
	#print "\n";
}

for($xx=0;$xx<$processCnt;$xx++) 
{
	if($xx == $processSeqEnd[$ts]) {
		if ($do_print && ($processSeqType{$cur_sys_name} eq "SUBSYS")) {
		    print OUT "\n\/\/End of subsystem   $cur_sys_name **************************************************\n\n\n";
		}
		$do_print = 0;
	}
	if($xx == $processSeqStart[$ts])
	{
		if ($processSeqSubsysName[$xx] =~ /$subsys/) {
		  $do_print = 1; 
		  $cur_sys_name = $processSeqSubsysName[$xx];
		  if ($processSeqType{$cur_sys_name} eq "SUBSYS") {
		    print OUT "\n\/\/Start of subsystem $cur_sys_name **************************************************\n\n";
		  }
		}
		$ts ++;
	}

	if (! $do_print)  { next; };
	$mm = $processPartNum[$xx];
	#print "looking for part $mm type=$partType[$mm] name=$xpartName[$mm]\n";
	if ($do_cluster) {
		my $do_print = 0;
		for (@cluster_parts) {
			if ($xpartName[$mm] eq $_) {
				$do_print = 1;
				break;
			}
		}
		if (! $do_print)  { next; };
	}
	$inCnt = $partInCnt[$mm];
	for($qq=0;$qq<$inCnt;$qq++)
	{
		$indone = 0;
		if ( -e "lib/$partInputType[$mm][$qq].pm" ) {
		  require "lib/$partInputType[$mm][$qq].pm";
	  	  $fromExp[$qq] = ("CDS::" . $partInputType[$mm][$qq] . "::fromExp") -> ($mm, $qq);
	    	  $indone = $fromExp[$qq] ne "";
		}

if (0) {
		if($partInputType[$mm][$qq] eq "Adc")
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
		if($partInputType[$mm][$qq] eq "Matrix")
		{
			$from = $partInNum[$mm][$qq];
			$fromPort = $partInputPort[$mm][$qq];
			$fromExp[$qq] = "\L$xpartName[$from]";
			$fromExp[$qq] .= "\[1\]\[";
			$fromExp[$qq] .= $fromPort;
			$fromExp[$qq] .= "\]";
			$indone = 1;
		}
}

		if($partInputType[$mm][$qq] eq "OSC")
		{
			$from = $partInNum[$mm][$qq];
			$fromPort = $partInputPort[$mm][$qq];
			$fromExp[$qq] = "\L$xpartName[$from]";
			$fromExp[$qq] .= "\[";
			$fromExp[$qq] .= $fromPort;
			$fromExp[$qq] .= "\]";
			$indone = 1;
		}
if (0) {
		if(($partInputType[$mm][$qq] eq "EpicsOut") || ($partInputType[$mm][$qq] eq "EpicsIn"))
		{
			$from = $partInNum[$mm][$qq];
			$fromPort = $partInputPort[$mm][$qq];
			$fromExp[$qq] = "pLocalEpics->";
			$fromExp[$qq] .= $systemName;
			$fromExp[$qq] .= "\.";
			$fromExp[$qq] .= $xpartName[$from];
			$indone = 1;
		}
}

		if(($partInputType[$mm][$qq] eq "RAMP_SW") || ($partInputType[$mm][$qq] eq "PRODUCT") || ($partInputType[$mm][$qq] eq "DIFF_JUNC"))
		{
			$from = $partInNum[$mm][$qq];
			$fromPort = $partInputPort[$mm][$qq];
			$fromExp[$qq] = "\L$xpartName[$from]";
			$fromExp[$qq] .= "\[";
			$fromExp[$qq] .= $fromPort;
			$fromExp[$qq] .= "\]";
			$indone = 1;
		}
		if($partInputType[$mm][$qq] eq "PHASE")
		{
			$from = $partInNum[$mm][$qq];
			$fromPort = $partInputPort[$mm][$qq];
			$fromExp[$qq] = "\L$xpartName[$from]";
			$fromExp[$qq] .= "\[";
			$fromExp[$qq] .= $fromPort;
			$fromExp[$qq] .= "\]";
			$indone = 1;
		}
if (0) {
		if($partInputType[$mm][$qq] eq "Wd")
		{
			$from = $partInNum[$mm][$qq];
			$fromPort = $partInputPort[$mm][$qq];
			$fromExp[$qq] = "\L$xpartName[$from]";
			$indone = 1;
		}
}
		if($indone == 0)
		{
			$from = $partInNum[$mm][$qq];
			$fromExp[$qq] = "\L$xpartName[$from]";
			#print "$xpartName[$mm]  $fromExp[$qq]\n";
		}
	}
	# ******** FILTER *************************************************************************



	if ( -e "lib/$partType[$mm].pm" ) {
	  	  print OUT ("CDS::" . $partType[$mm] . "::frontEndCode") -> ($mm);
	}	

if (0) {
	if($partType[$mm] eq "Filt")
	{
	   print OUT "// FILTER MODULE\n";
	   $calcExp = "\L$xpartName[$mm]";
	   $calcExp .= " = ";
	   if ($cpus > 2) {
	     $calcExp .= "filterModuleD(dspPtr[0],dspCoeff,";
	   } else {
	     $calcExp .= "filterModuleD(dsp_ptr,dspCoeff,";
	   }
	   $calcExp .= $xpartName[$mm];
	   $calcExp .= ",";
	   $calcExp .= $fromExp[0];
	   $calcExp .= ",0);\n";
	   print OUT "$calcExp";
	}
	if($partType[$mm] eq "Wd")
	{
	   print OUT "// Wd (Watchdog) MODULE\n";
		print OUT "if((clock16K \% 16) == 0) {\n";
		$calcExp = "if (pLocalEpics->$systemName\.";
		$calcExp .= $xpartName[$mm];
		$calcExp .= " == 1) {\n";
		$calcExp .= "\t\L$xpartName[$mm] = 1;\n";
		$calcExp .= "\tpLocalEpics->$systemName\." . $xpartName[$mm] . " = 0;\n";
		$calcExp .= "};\n";
		print OUT "$calcExp";
		print OUT "double ins[$partInCnt[$mm]]= {\n";
		for (0 .. $partInCnt[$mm]-1) {
			print OUT "\t$fromExp[$_],\n";
		}
		print OUT "};\n";
	        print OUT "   for\(ii=0;ii<$partInCnt[$mm];ii++\) {\n";
		$calcExp = "\t\L$xpartName[$mm]\_avg\[ii\]";
		$calcExp .= " = ins[ii] * \.00005 + ";
		$calcExp .= "\L$xpartName[$mm]\_avg\[ii\] * 0\.99995;\n";
		print OUT "$calcExp";
		print OUT "\t\L$xpartName[$mm]\_vabs = ins[ii] - \L$xpartName[$mm]\_avg\[ii\];\n";
		print OUT "\tif\(\L$xpartName[$mm]\_vabs < 0) \L$xpartName[$mm]\_vabs *= -1.0;\n";
		$calcExp = "\t\L$xpartName[$mm]\_var\[ii\] = \L$xpartName[$mm]\_vabs * \.00005 + ";
		$calcExp .= "\L$xpartName[$mm]\_var\[ii\] * 0\.99995;\n";
		print OUT "$calcExp";
		$calcExp = "\tpLocalEpics->$systemName\.";
		$calcExp .= $xpartName[$mm];
		$calcExp .= "_VAR\[ii\] = ";
		$calcExp .= "\L$xpartName[$mm]\_var\[ii\];\n";
		print OUT "$calcExp";

		$calcExp = "\tif(\L$xpartName[$mm]\_var\[ii\] \> ";
		$calcExp .= "pLocalEpics->$systemName\.";
		$calcExp .= $xpartName[$mm];
		$calcExp .= "_MAX\) ";
		$calcExp .= "\L$xpartName[$mm] = 0;\n";
		print OUT "$calcExp";
		print OUT "   }\n";
		$calcExp = "\tpLocalEpics->$systemName\.";
		$calcExp .= $xpartName[$mm];
		$calcExp .= "_STAT = \L$xpartName[$mm];\n";
		print OUT "$calcExp";
		print OUT "}\n";
	}
}

	if($partType[$mm] eq "SUS_WD")
	{
	   print OUT "// SUS_WD MODULE\n";
		print OUT "if((cycle \% 16) == 0) {\n";
		$calcExp = "\L$xpartName[$mm] = 16384;\n";
		print OUT "$calcExp";
	        print OUT "   for\(ii=0;ii<20;ii++\) {\n";
	        print OUT "\tif\(ii\<16\) jj = ii;\n";
	        print OUT "\telse jj = ii+2;\n";
		$calcExp = "\t\L$xpartName[$mm]\_avg\[ii\]";
		$calcExp .= " = dWord\[0\]\[jj\] * \.00005 + ";
		$calcExp .= "\L$xpartName[$mm]\_avg\[ii\] * 0\.99995;\n";
		print OUT "$calcExp";
		print OUT "\tvabs = dWord\[0\]\[jj\] - \L$xpartName[$mm]\_avg\[ii\];\n";
		print OUT "\tif\(vabs < 0) vabs *= -1.0;\n";
		$calcExp = "\t\L$xpartName[$mm]\_var\[ii\] = vabs * \.00005 + ";
		$calcExp .= "\L$xpartName[$mm]\_var\[ii\] * 0\.99995;\n";
		print OUT "$calcExp";
		$calcExp = "\tpLocalEpics->$systemName\.";
		$calcExp .= $xpartName[$mm];
		$calcExp .= "_VAR\[ii\] = ";
		$calcExp .= "\L$xpartName[$mm]\_var\[ii\];\n";
		print OUT "$calcExp";

		$calcExp = "\tif(\L$xpartName[$mm]\_var\[ii\] \> ";
		$calcExp .= "pLocalEpics->$systemName\.";
		$calcExp .= $xpartName[$mm];
		$calcExp .= "_MAX\) ";
		$calcExp .= "\L$xpartName[$mm] = 0;\n";
		print OUT "$calcExp";
		print OUT "   }\n";
		$calcExp = "\tpLocalEpics->$systemName\.";
		$calcExp .= $xpartName[$mm];
		$calcExp .= " = \L$xpartName[$mm] \/ 16384;\n";
		print OUT "$calcExp";
		print OUT "}\n";
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
if (0) {
	# ******** MATRIX *************************************************************************
	if($partType[$mm] eq "Matrix")
	{
		print OUT "// Matrix\n";
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
	# ******** PHASE ************************************************************************
	if($partType[$mm] eq "PHASE")
	{
	   	print OUT "// PHASE\n";
	   	$calcExp = "\L$xpartName[$mm]\[0\] = \(";
	   	$calcExp .=  "$fromExp[0]";
	   	$calcExp .=  " * ";
	   	$calcExp .=  "pLocalEpics->$systemName\.$xpartName[$mm]";
	   	$calcExp .=  "\[1\]\) + \(";
	   	$calcExp .=  "$fromExp[1]";
	   	$calcExp .=  " * ";
	   	$calcExp .=  "pLocalEpics->$systemName\.$xpartName[$mm]";
	   	$calcExp .=  "\[0\]);\n";
		print OUT "$calcExp";
	   	$calcExp = "\L$xpartName[$mm]\[1\] = \(";
	   	$calcExp .=  "$fromExp[1]";
	   	$calcExp .=  " * ";
	   	$calcExp .=  "pLocalEpics->$systemName\.$xpartName[$mm]";
	   	$calcExp .=  "\[1\]\) - \(";
	   	$calcExp .=  "$fromExp[0]";
	   	$calcExp .=  " * ";
	   	$calcExp .=  "pLocalEpics->$systemName\.$xpartName[$mm]";
	   	$calcExp .=  "\[0\]);\n";
		print OUT "$calcExp";
	}

if (0) {
	if($partType[$mm] eq "WfsPhase")
	{
	   	print OUT "// WFS PHASE\n";
	   	$calcExp = "\L$xpartName[$mm]\[0\] = \(";
	   	$calcExp .=  "$fromExp[0]";
	   	$calcExp .=  " * ";
	   	$calcExp .=  "pLocalEpics->$systemName\.$xpartName[$mm]";
	   	$calcExp .=  "\[0\]\[0\]\) - \(";
	   	$calcExp .=  "$fromExp[1]";
	   	$calcExp .=  " * ";
	   	$calcExp .=  "pLocalEpics->$systemName\.$xpartName[$mm]";
	   	$calcExp .=  "\[1\]\[0\]);\n";
		print OUT "$calcExp";
	   	$calcExp = "\L$xpartName[$mm]\[1\] = \(";
	   	$calcExp .=  "$fromExp[1]";
	   	$calcExp .=  " * ";
	   	$calcExp .=  "pLocalEpics->$systemName\.$xpartName[$mm]";
	   	$calcExp .=  "\[1\]\[1\]\) - \(";
	   	$calcExp .=  "$fromExp[0]";
	   	$calcExp .=  " * ";
	   	$calcExp .=  "pLocalEpics->$systemName\.$xpartName[$mm]";
	   	$calcExp .=  "\[0\]\[1\]);\n";
		print OUT "$calcExp";
	}
}

	# ******** OSC ************************************************************************
	if($partType[$mm] eq "OSC")
	{
	   	print OUT "// OSC\n";
	   	$calcExp = "\L$xpartName[$mm]\_cos_new = \(1.0 - ";
	   	$calcExp .=  "\L$xpartName[$mm]\_alpha\) * \L$xpartName[$mm]\_cos_prev - ";
	   	$calcExp .=  "\L$xpartName[$mm]\_beta * \L$xpartName[$mm]\_sin_prev;\n";
		print OUT "$calcExp";
	   	$calcExp = "\L$xpartName[$mm]\_sin_new = \(1.0 - ";
	   	$calcExp .=  "\L$xpartName[$mm]\_alpha\) * \L$xpartName[$mm]\_sin_prev + ";
	   	$calcExp .=  "\L$xpartName[$mm]\_beta * \L$xpartName[$mm]\_cos_prev;\n";
		print OUT "$calcExp";
	   	$calcExp = "\L$xpartName[$mm]\_sin_prev = \L$xpartName[$mm]\_sin_new;\n";
		print OUT "$calcExp";
	   	$calcExp = "\L$xpartName[$mm]\_cos_prev = \L$xpartName[$mm]\_cos_new;\n";
		print OUT "$calcExp";
	   	$calcExp = "\L$xpartName[$mm]\[0\] = \L$xpartName[$mm]\_sin_new * ";
	   	$calcExp .=  "pLocalEpics->$systemName\.$xpartName[$mm]\_CLKGAIN;\n";
		print OUT "$calcExp";
	   	$calcExp = "\L$xpartName[$mm]\[1\] = \L$xpartName[$mm]\_sin_new * ";
	   	$calcExp .=  "pLocalEpics->$systemName\.$xpartName[$mm]\_SINGAIN;\n";
		print OUT "$calcExp";
	   	$calcExp = "\L$xpartName[$mm]\[2\] = \L$xpartName[$mm]\_cos_new * ";
	   	$calcExp .=  "pLocalEpics->$systemName\.$xpartName[$mm]\_COSGAIN;\n";
		print OUT "$calcExp";

	   	$calcExp =  "if((\L$xpartName[$mm]_freq \!= ";
	   	$calcExp .=  "pLocalEpics->$systemName\.$xpartName[$mm]\_FREQ) \&\& ";
	   	$calcExp .=  "((cycle + 1) == \UFE_RATE))\n";
	   	print OUT "$calcExp";
	   	print OUT "{\n";
	   	$calcExp =  "\L$xpartName[$mm]_freq = ";
	   	$calcExp .=  "pLocalEpics->$systemName\.$xpartName[$mm]\_FREQ;\n";
	   	print OUT "\t$calcExp";
	   	$calcExp =  "printf(\"OSC Freq = \%f\\n\",\L$xpartName[$mm]_freq\);\n";
	   	print OUT "\t$calcExp";
		$calcExp = "\L$xpartName[$mm]\_delta = 2.0 * 3.1415926535897932384626 * ";
	   	$calcExp .=  "\L$xpartName[$mm]_freq / \UFE_RATE;\n";
	   	print OUT "\t$calcExp";
		$calcExp = "valx = \L$xpartName[$mm]\_delta \/ 2.0;\n";
	   	print OUT "\t$calcExp";
		$calcExp = "sincos\(valx, \&lsinx, \&lcosx\);\n";
	   	print OUT "\t$calcExp";
		$calcExp = "\L$xpartName[$mm]\_alpha = 2.0 * lsinx * lsinx;\n";
	   	print OUT "\t$calcExp";
		$calcExp = "valx = \L$xpartName[$mm]\_delta\;\n";
	   	print OUT "\t$calcExp";
		$calcExp = "sincos\(valx, \&lsinx, \&lcosx\);\n";
	   	print OUT "\t$calcExp";
		$calcExp = "\L$xpartName[$mm]\_beta = lsinx;\n";
	   	print OUT "\t$calcExp";
		$calcExp = "\L$xpartName[$mm]\_cos_prev = 1.0;\n";
	   	print OUT "\t$calcExp";
		$calcExp = "\L$xpartName[$mm]\_sin_prev = 0.0;\n";
	   	print OUT "\t$calcExp";
	   	print OUT "}\n";
	}
	# ******** MULTIPLY ********************************************************************
	if($partType[$mm] eq "MULTIPLY")
	{
	   print OUT "// MULTIPLY\n";
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
			$calcExp .= " * ";
		    }
		}
		print OUT "$calcExp";
	}
	# ******** DIVIDE ********************************************************************
	if($partType[$mm] eq "DIVIDE")
	{
	   print OUT "// DIVIDE\n";
		# print "\tUsed Divide $xpartName[$mm] $partOutCnt[$mm]\n";
		$calcExp = "if\(";
		$calcExp .= "$fromExp[1] \!= 0.0)\n";
		print OUT "$calcExp";
		print OUT "\{\n";
		$calcExp = "\t\L$xpartName[$mm]";
		$calcExp .= " = ";
		$calcExp .= $fromExp[0];
		$calcExp .= " \/ ";
		$calcExp .= $fromExp[1];
		$calcExp .= ";\n";
		print OUT "$calcExp";
		print OUT "\}\n";
		print OUT "else\{\n";
		$calcExp = "\t\L$xpartName[$mm] = 0.0;\n";
		print OUT "$calcExp";
		print OUT "\}\n";
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

if (0) {
	# ******** EPICS OUTPUT ********************************************************************
	if($partType[$mm] eq "EpicsOut")
	{
	   #print "Found EPICS OUTPUT $xpartName[$mm] $partInputType[$mm][0] in loop\n";
	   	print OUT "// EpicsOut\n";
			print OUT "pLocalEpics->$systemName\.$xpartName[$mm] = ";
			print OUT "$fromExp[0];\n";
	}
	# ******** DAC OUTPUT ********************************************************************
	if($partType[$mm] eq "Dac")
	{
		$dacNum = substr($xpartName[$mm],4,1);
		print OUT "// DAC number is $dacNum\n";
           for($qq=0;$qq<16;$qq++)
           {
	     $fromType = $partInputType[$mm][$qq];
	     if(($fromType ne "GROUND") && ($partInput[$mm][$qq] ne "NC"))
	     {
		$calcExp = "dacOut\[";
		$calcExp .= $dacNum;
		$calcExp .= "\]\[";
		# $calcExp .= $partOutputPort[$mm][$qq];
		$calcExp .= $qq;
		$calcExp .= "\] = ";
		$calcExp .= $fromExp[$qq];
		$calcExp .= ";\n";
		print OUT "$calcExp";
	     }
	   }
	}
	# ******** MULTI_SW ************************************************************************
	if($partType[$mm] eq "MultiSwitch")
	{
	   print OUT "// MultiSwitch\n";
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
	   	if ($cpus > 2) {
	   	  $calcExp = "filterPolyPhase(dspPtr[0],firCoeff,$xpartName[$mm],$firName,";
	 	} else {
	   	  $calcExp = "filterPolyPhase(dsp_ptr,firCoeff,$xpartName[$mm],$firName,";
		}
		if($toType eq "Matrix")
		{
			$toPort = $partOutputPort[$mm][0];
			print "\t$xpartName[$to]\[0\]\[$toPort\] = ";
		}
		if($fromType eq "Adc")
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
if ($cpus > 2) {
print OUTM "CFLAGS += -DRESERVE_CPU2\n";
}
if ($cpus > 3) {
print OUTM "CFLAGS += -DRESERVE_CPU3\n";
}
print OUTM "CFLAGS += -DNO_SYNC\n";
print OUTM "CFLAGS += -DNO_DAQ\n";
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
$jj = $filtCnt / 40;
$jj ++;
print OUTG "$jj lines to print\n";
for($ii=0;$ii<$jj;$ii++)
{
	$kk = $ii * 4;
	print OUTG "\# MODULES $filterName[$kk] ";
	$kk ++;
	print OUTG "$filterName[$kk] ";
	$kk ++;
	print OUTG "$filterName[$kk] ";
	$kk ++;
	print OUTG "$filterName[$kk]\n";
}
close OUTG;
