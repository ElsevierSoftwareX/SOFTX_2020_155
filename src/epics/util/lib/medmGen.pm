package CDS::medmGen;
use Exporter;
@ISA = ('Exporter');

#//     \page medmGen medmGen.pm
#//     medmGen.pm - Contains subroutines to create MEDM files and add MEDM parts to the files for auto screen generation.
#//
#// \n

#Package intended for use in generating MEDM screens

#// \b sub \b medmGenFile \n
#// This sub will create the basic MEDM file and add color map..\n
#// Arguments: \n
#//	0 = MEDM directory\n
#//	1 = File name \n
#//	2 = display width \n
#//	3 = display height \n\n
sub medmGenFile
{

my ($mdir,$mfile,$wid,$ht) = @_;
        return <<END;
file {
        name=$mfile
        version=030104
}
display {
        object {
                x=100
                y=100
                width=$wid
                height=$ht
        }
        clr=14
        bclr=4
        cmap=""
        gridSpacing=5
        gridOn=0
        snapToGrid=0
}
"color map" {
        ncolors=65
        colors {
                ffffff, ececec, dadada, c8c8c8, bbbbbb, aeaeae, 9e9e9e, 919191, 858585, 787878,
                696969, 5a5a5a, 464646, 2d2d2d, 000000, 00d800, 1ebb00, 339900, 2d7f00, 216c00,
                fd0000, de1309, be190b, a01207, 820400, 5893ff, 597ee1, 4b6ec7, 3a5eab, 27548d,
                fbf34a, f9da3c, eeb62b, e19015, cd6100, ffb0ff, d67fe2, ae4ebc, 8b1a96, 610a75,
                a4aaff, 8793e2, 6a73c1, 4d52a4, 343386, c7bb6d, b79d5c, a47e3c, 7d5627, 58340f,
                99ffff, 73dfff, 4ea5f9, 2a63e4, 0a00b8, ebf1b5, d4db9d, bbc187, a6a462, 8b8239,
                73ff6b, 52da3b, 3cb420, 289315, 1a7309,
        }
}

END
}

#// \b sub \b medmGenTextMon \n
#// This sub will create a text monitoring MEDM part ****************************************** \n
#// Arguments:\n
#//	0 = xpos \n
#//	1 = ypos \n
#//	2 = width \n
#//	3 = height \n
#//	4 = channel name \n
#//	5 = forground color \n
#//	6 = background color \n
#//	7 = clrmod \n\n
sub medmGenTextMon
{
my ($xpos,$ypos,$wid,$ht,$chan,$fgc,$bgc,$clrmod,$form) = @_;
        return <<END;
"text update" {
        object {
                x=$xpos
                y=$ypos
                width=$wid
                height=$ht
        }
        monitor {
                chan="$chan"
                clr="$fgc"
                bclr="$bgc"
        }
        clrmod="$clrmod"
        format="$form"
        align="horiz. centered"
        limits {
        }
}

END
}

#// \b sub \b medmGenText ($xpos,$ypos,$wid,$ht,$tix,$fgc) \n
#// This sub will create an MEDM text block ************************************************* \n
#//	0 = xpos \n
#//	1 = ypos \n
#//	2 = width \n
#//	3 = height \n
#//	4 = Text string \n
#//	5 = Text color \n\n
sub medmGenText
{
my ($xpos,$ypos,$wid,$ht,$tix,$fgc) = @_;
        return <<END;
text {
        object {
                x=$xpos
                y=$ypos
                width=$wid
                height=$ht
        }
	"basic attribute" {
                clr=$fgc
        }
        align="horiz. centered"
        textix="$tix"

}
END
}

#// \b sub \b medmGenTextDyn ($xpos,$ypos,$wid,$ht,$tix,$fgc,$mcalc,$chan) \n
#// This sub will create an MEDM dynamic text block ************************************************* \n
#//	0 = xpos \n
#//	1 = ypos \n
#//	2 = width \n
#//	3 = height \n
#//	4 = Text string \n
#//	5 = Text color \n
#//	6 = Calc \n
#//	7 = Channel \n\n 
sub medmGenTextDyn
{
my ($xpos,$ypos,$wid,$ht,$tix,$fgc,$mcalc,$chan) = @_;
        return <<END;
text {
        object {
                x=$xpos
                y=$ypos
                width=$wid
                height=$ht
        }
	"basic attribute" {
                clr=$fgc
        }
	"dynamic attribute" {
		vis="calc"
		calc="$mcalc"
		chan="$chan"
        }
        align="horiz. centered"
        textix="$tix"

}
END
}

#// \b sub \b medmGenByte ($xpos,$ypos,$wid,$ht,$chan,$sb,$eb,$oclr,$zclr) \n
#// This sub will create an MEDM byte monitor ************************************************* \n
#//	0 = xpos \n
#//	1 = ypos \n
#//	2 = width \n
#//	3 = height \n
#//	4 = EPICS channel name \n
#//	5 = Starting bit \n
#//	6 = End bit \n
#//	7 = "1" color \n
#//	8= "0" color \n\n
sub medmGenByte
{
my ($xpos,$ypos,$wid,$ht,$chan,$sb,$eb,$oclr,$zclr) = @_;
        return <<END;
byte {
        object {
                x=$xpos
                y=$ypos
                width=$wid
                height=$ht
        }
        monitor {
                chan="$chan"
                clr=$oclr
                bclr=$zclr
        }
	sbit=$sb
	ebit=$eb
}



END
}


#// \b sub \b medmGenRectangle ($x,$y,$w,$h,$color,$vis,$calc,$chan) \n
#// This sub will create an MEDM rectangle  ************************************************* \n
#//	0 = xpos \n
#//	1 = ypos \n
#//	2 = width \n
#//	3 = height \n
#//	4 = rectangle color \n
#//	5 = visability \n
#//	6 = calc \n
#//	7 = chan \n\n
sub medmGenRectangle
{
my ($x,$y,$w,$h,$color,$vis,$calc,$chan) = @_;
        return <<END;
rectangle {
        object {
                x=$x
                y=$y
                width=$w
                height=$h
        }
	"basic attribute" {
                clr=$color
        }
	"dynamic attribute" {
                vis="$vis"
		calc="$calc"
		chan="$chan"
        }

}



END
}

#// \b sub \b medmGenMeter ($xpos,$ypos,$wid,$ht,$chan,$fgc,$bgc) \n
#// This sub will create an MEDM Meter ************************************************* \n
#//	0 = xpos \n
#//	1 = ypos \n
#//	2 = width \n
#//	3 = height \n
#//	4 = channel \n
#//	5 = color \n
#//	6= background color \n\n
sub medmGenMeter
{
my ($xpos,$ypos,$wid,$ht,$chan,$fgc,$bgc) = @_;
        return <<END;
meter {
        object {
                x=$xpos
                y=$ypos
                width=$wid
                height=$ht
        }
	monitor {
		chan="$chan";
		clr=$fgc
		bclr=$bgc
	}
	label="limits"
	limits {
	}
}



END
}

#// \b sub \b medmGenRelDisp ($xpos,$ypos,$wid,$ht,$disp,$fgc,$bgc,$label,$dargs) \n
#// This sub will create an MEDM related display  ************************************************* \n
#//	0 = xpos \n
#//	1 = ypos \n
#//	2 = width \n
#//	3 = height \n
#//	4 = Display name \n
#//	5 = color \n
#//	6= background color \n
#//	7 = Label \n\n
sub medmGenRelDisp
{
my ($xpos,$ypos,$wid,$ht,$disp,$fgc,$bgc,$label,$dargs) = @_;
        return <<END;
"related display" {
        object {
                x=$xpos
                y=$ypos
                width=$wid
                height=$ht
        }
        display[0] {
                name="$disp"
		args="$dargs"
        }
	clr=$fgc
	bclr=$bgc
	label="$label"
}



END
}

#// \b sub \b medmGenMessage ($xpos,$ypos,$wid,$ht,$chan,$fgc,$bgc,$label,$message) \n
#// This sub will create an MEDM Message Button  ************************************************* \n
#//	0 = xpos \n
#//	1 = ypos \n
#//	2 = width \n
#//	3 = height \n
#//	4 = channel name \n
#//	5 = color \n
#//	6= background color \n
#//	7 = Label \n
#//	8 = Message \n\n
sub medmGenMessage
{
my ($xpos,$ypos,$wid,$ht,$chan,$fgc,$bgc,$label,$message) = @_;
        return <<END;
"message button" {
        object {
                x=$xpos
                y=$ypos
                width=$wid
                height=$ht
        }
        control {
                chan="$chan"
		clr=$fgc
		bclr=$bgc
        }
	label="$label"
	release_msg="$message"
}



END
}

#// \b sub \b medmGenChoice ($xpos,$ypos,$wid,$ht,$chan,$fgc,$bgc) \n
#// This sub will create a choice button MEDM part ****************************************** \n
#// Arguments: \n
#//	0 = xpos \n
#//	1 = ypos \n
#//	2 = width \n
#//	3 = height \n
#//	4 = channel name \n
#//	5 = forground color \n
#//	6 = background color \n\n
sub medmGenChoice
{
my ($xpos,$ypos,$wid,$ht,$chan,$fgc,$bgc) = @_;
        return <<END;
"choice button" {
        object {
                x=$xpos
                y=$ypos
                width=$wid
                height=$ht
        }
        control {
                chan="$chan"
                clr="$fgc"
                bclr="$bgc"
        }
}

END
}

#// \b sub \b medmGenTextEntry ($xpos,$ypos,$wid,$ht,$chan,$fgc,$bgc,$clrmod) \n
#// This sub will create a text entry MEDM part ****************************************** \n
#// Arguments: \n
#//	0 = xpos \n
#//	1 = ypos \n
#//	2 = width \n
#//	3 = height \n
#//	4 = channel name \n
#//	5 = forground color \n
#//	6 = background color \n
#//	7 = clrmod \n\n
sub medmGenTextEntry
{
my ($xpos,$ypos,$wid,$ht,$chan,$fgc,$bgc,$clrmod) = @_;
        return <<END;
"text entry" {
        object {
                x=$xpos
                y=$ypos
                width=$wid
                height=$ht
        }
        control {
                chan="$chan"
                clr="$fgc"
                bclr="$bgc"
        }
        clrmod="$clrmod"
        align="horiz. centered"
        limits {
        }
}

END
}

#// \b sub \b medmGenTriangle ($xpos,$ypos,$wid,$ht,$lwide,$fgc,@xpts) \n
#// This sub will create an MEDM static triangle block ************************************** \n
#//	0 = xpos \n
#//	1 = ypos \n
#//	2 = width \n
#//	3 = height \n
#//	4 = Line width \n
#//	5 = Text color \n
#//	6 = Triangle points (4xpos + 4ypos). \n\n
sub medmGenTriangle
{
my ($xpos,$ypos,$wid,$ht,$lwide,$fgc,@xpts) = @_;
        return <<END;
polygon {
        object {
                x=$xpos
                y=$ypos
                width=$wid
                height=$ht
        }
	"basic attribute" {
                clr=$fgc
		width=$lwide
        }
	points {
		($xpts[0],$xpts[1])
		($xpts[2],$xpts[3])
		($xpts[4],$xpts[5])
		($xpts[6],$xpts[7])
	}

}
END
}

#// \b sub \b medmGenLine ($xpos,$ypos,$wid,$ht,$lwide,$fgc) \n
#// This sub will create an MEDM Line  ************************************************* \n
#//	0 = xpos \n
#//	1 = ypos \n
#//	2 = width \n
#//	3 = height \n
#//	4 = Line width \n
#//	5 = Color \n\n
sub medmGenLine
{
my ($xpos,$ypos,$wid,$ht,$lwide,$fgc) = @_;
if($ht > $wid) {
	$sx = $xpos;
	$ex = $xpos;
	$sy = $ypos;
	$ey = $ypos + $ht;
} else {
	$sx = $xpos;
	$ex = $xpos + $wid;
	$sy = $ypos;
	$ey = $ypos;
}
        return <<END;
polyline {
        object {
                x=$xpos
                y=$ypos
                width=$wid
                height=$ht
        }
	"basic attribute" {
                clr=$fgc
		width=$lwide
        }
	points {
		($sx,$sy)
		($ex,$ey)
	}

}
END
}

#// \b sub \b medmGenTriangleDyn ($xpos,$ypos,$wid,$ht,$lwide,$fgc,$mcalc,$chan,@xpts) \n
#// This sub will create an MEDM Triangle block with dynamic attibutes *********************************** \n
#//	0 = xpos \n
#//	1 = ypos \n
#//	2 = width \n
#//	3 = height \n
#//	4 = line width \n
#//	5 = color \n
#//	6 = visibility calculation \n
#//	7 = Channel name \n
#//	8 = Triangle points (4xpos + 4ypos). \n\n
sub medmGenTriangleDyn
{
my ($xpos,$ypos,$wid,$ht,$lwide,$fgc,$mcalc,$chan,@xpts) = @_;
        return <<END;
polygon {
        object {
                x=$xpos
                y=$ypos
                width=$wid
                height=$ht
        }
	"basic attribute" {
                clr=$fgc
		width=$lwide
        }
	"dynamic attribute" {
		vis="calc"
		calc="$mcalc"
		chan="$chan"
        }
	points {
		($xpts[0],$xpts[1])
		($xpts[2],$xpts[3])
		($xpts[4],$xpts[5])
		($xpts[6],$xpts[7])
	}

}
END
}

#// \b sub \b medmGenShellCmd ($xpos,$ypos,$wid,$ht,$fgc,$bgc,$numCmds,$label,@sargs) \n
#// This sub will create an MEDM Shell Command block ************************************************* \n
#//	0 = xpos \n
#//	1 = ypos \n
#//	2 = width \n
#//	3 = height \n
#//	4 = Text color \n
#//	5 = Background color \n
#//	6 = Number of commands \n
#//	7 = Label \n
#//	8 = List of commands, in order: \n
#//		- Label \n
#//		- Command \n
#//		- Command Arguments \n\n
sub medmGenShellCmd
{
my ($xpos,$ypos,$wid,$ht,$fgc,$bgc,$numCmds,$label,@sargs) = @_;

	$rstr = "\"shell command\" { \n";
	$rstr .=  "\tobject  { \n";
	$rstr .=  "\t\tx=$xpos \n";
	$rstr .=  "\t\ty=$ypos \n";
	$rstr .=  "\t\twidth=$wid \n";
	$rstr .=  "\t\theight=$ht \n";
	$rstr .= "\t}\n";
for(my $ii=0;$ii<$numCmds;$ii++)
{
	$rstr .= "\tcommand[$ii] { \n";
	$rstr .= "\t\tlabel=\"$sargs[$ii]\" \n";
	$rstr .= "\t\tname=\"$sargs[$ii+ $numCmds]\" \n";
	$rstr .= "\t\targs=\"$sargs[$ii+ 2*$numCmds]\" \n";
	$rstr .= "\t}\n";
}

	$rstr .=  "\tclr=$fgc \n";
	$rstr .=  "\tbclr=$bgc \n";
	$rstr .=  "\tlabel=\"$label\" \n";
	$rstr .= "}\n";

	return $rstr;
}
