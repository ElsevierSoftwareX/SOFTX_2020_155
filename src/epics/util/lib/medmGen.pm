package CDS::medmGen;
use Exporter;
@ISA = ('Exporter');

#//     \file medmGen.dox
#//     \brief Documentation for medmGen.pm
#//
#// \n
#//     \subpage devguidercg2 "<<-- Parts Library"
#// \n

#Package intended for use in generating MEDM screens

# This sub will create the basic MEDM file and add color map..
# Arguments:
#	0 = MEDM directory
#	1 = File name
#	2 = display width
#	3 = display height
sub medmGenFile
{

my ($mdir,$mfile,$wid,$ht) = @_;
print "creating file $mdir\/$mfile \n";
        open(OUTMEDM, ">$mdir/$mfile") || die "cannot open $mdir/$mfile for writing ";
        print OUTMEDM <<END;
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
                ffffff,
                ececec,
                dadada,
                c8c8c8,
                bbbbbb,
                aeaeae,
                9e9e9e,
                919191,
                858585,
                787878,
                696969,
                5a5a5a,
                464646,
                2d2d2d,
                000000,
                00d800,
                1ebb00,
                339900,
                2d7f00,
                216c00,
                fd0000,
                de1309,
                be190b,
                a01207,
                820400,
                5893ff,
                597ee1,
                4b6ec7,
                3a5eab,
                27548d,
                fbf34a,
                f9da3c,
                eeb62b,
                e19015,
                cd6100,
                ffb0ff,
                d67fe2,
                ae4ebc,
                8b1a96,
                610a75,
                a4aaff,
                8793e2,
                6a73c1,
                4d52a4,
                343386,
                c7bb6d,
                b79d5c,
                a47e3c,
                7d5627,
                58340f,
                99ffff,
                73dfff,
                4ea5f9,
                2a63e4,
                0a00b8,
                ebf1b5,
                d4db9d,
                bbc187,
                a6a462,
                8b8239,
                73ff6b,
                52da3b,
                3cb420,
                289315,
                1a7309,
        }
}

END
close OUTMEDM;
}

# This sub will create a text monitoring MEDM part ******************************************
# Arguments:
#	0 = MEDM directory
#	1 = File name
#	2 = xpos
#	3 = ypos
#	4 = width
#	5 = height
#	6 = channel name
#	7 = forground color
#	8 = background color
#	9 = clrmod
sub medmGenTextMon
{
my ($mdir,$mfile,$xpos,$ypos,$wid,$ht,$chan,$fgc,$bgc,$clrmod) = @_;
        open(OUTMEDM, ">>$mdir/$mfile") || die "cannot open $mdir/$mfile for writing ";
        print OUTMEDM <<END;
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
        align="horiz. centered"
        limits {
        }
}



END
close OUTMEDM;
}

# This sub will create an MEDM text block *************************************************
#	0 = MEDM directory
#	1 = File name
#	2 = xpos
#	3 = ypos
#	4 = width
#	5 = height
#	6 = Text string
#	7 = Text color
sub medmGenText
{
my ($mdir,$mfile,$xpos,$ypos,$wid,$ht,$tix,$fgc) = @_;
        open(OUTMEDM, ">>$mdir/$mfile") || die "cannot open $mdir/$mfile for writing ";
        print OUTMEDM <<END;
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
close OUTMEDM;
}
# This sub will create an MEDM dynamic text block *************************************************
#	0 = MEDM directory
#	1 = File name
#	2 = xpos
#	3 = ypos
#	4 = width
#	5 = height
#	6 = Text string
#	7 = Text color
#	8 = Calc
#	9 = Channel 
sub medmGenTextDyn
{
my ($mdir,$mfile,$xpos,$ypos,$wid,$ht,$tix,$fgc,$mcalc,$chan) = @_;
        open(OUTMEDM, ">>$mdir/$mfile") || die "cannot open $mdir/$mfile for writing ";
        print OUTMEDM <<END;
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
close OUTMEDM;
}

# This sub will create an MEDM byte monitor *************************************************
#	0 = MEDM directory
#	1 = File name
#	2 = xpos
#	3 = ypos
#	4 = width
#	5 = height
#	6 = EPICS channel name
#	7 = Starting bit
#	8 = End bit
#	9 = "1" color
#	10= "0" color
sub medmGenByte
{
my ($mdir,$mfile,$xpos,$ypos,$wid,$ht,$chan,$sb,$eb,$oclr,$zclr) = @_;
        open(OUTMEDM, ">>$mdir/$mfile") || die "cannot open $mdir/$mfile for writing ";
        print OUTMEDM <<END;
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
close OUTMEDM;
}


# This sub will create an MEDM rectangle  *************************************************
#	0 = MEDM directory
#	1 = File name
#	2 = xpos
#	3 = ypos
#	4 = width
#	5 = height
#	6 = rectangle color
#	7 = visability
#	8 = calc
#	9 = chan
sub medmGenRectangle
{
my ($mdir,$mfile,$x,$y,$w,$h,$color,$vis,$calc,$chan) = @_;
        open(OUTMEDM, ">>$mdir/$mfile") || die "cannot open $mdir/$mfile for writing ";
        print OUTMEDM <<END;
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
close OUTMEDM;
}
# This sub will create an MEDM Meter *************************************************
#	0 = MEDM directory
#	1 = File name
#	2 = xpos
#	3 = ypos
#	4 = width
#	5 = height
#	6 = channel
#	7 = color
#	8= background color
sub medmGenMeter
{
my ($mdir,$mfile,$xpos,$ypos,$wid,$ht,$chan,$fgc,$bgc) = @_;
        open(OUTMEDM, ">>@_[0]/@_[1]") || die "cannot open @_[0]/@_[1] for writing ";
        print OUTMEDM <<END;
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
close OUTMEDM;
}
# This sub will create an MEDM related display  *************************************************
#	0 = MEDM directory
#	1 = File name
#	2 = xpos
#	3 = ypos
#	4 = width
#	5 = height
#	6 = Display name
#	7 = color
#	8= background color
#	9 = Label
sub medmGenRelDisp
{
my ($mdir,$mfile,$xpos,$ypos,$wid,$ht,$disp,$fgc,$bgc,$label) = @_;
        open(OUTMEDM, ">>$mdir/$mfile") || die "cannot open $mdir/$mfile for writing ";
        print OUTMEDM <<END;
"related display" {
        object {
                x=$xpos
                y=$ypos
                width=$wid
                height=$ht
        }
        display[0] {
                name="$disp"
        }
	clr=$fgc
	bclr=$bgc
	label="$label"
}



END
close OUTMEDM;
}
# This sub will create an MEDM Message Button  *************************************************
#	0 = MEDM directory
#	1 = File name
#	2 = xpos
#	3 = ypos
#	4 = width
#	5 = height
#	6 = channel name
#	7 = color
#	8= background color
#	9 = Label
#	10 = Message
sub medmGenMessage
{
my ($mdir,$mfile,$xpos,$ypos,$wid,$ht,$chan,$fgc,$bgc,$label,$message) = @_;
        open(OUTMEDM, ">>$mdir/$mfile") || die "cannot open $mdir/$mfile for writing ";
        print OUTMEDM <<END;
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
close OUTMEDM;
}
# This sub will create a choice button MEDM part ******************************************
# Arguments:
#	0 = MEDM directory
#	1 = File name
#	2 = xpos
#	3 = ypos
#	4 = width
#	5 = height
#	6 = channel name
#	7 = forground color
#	8 = background color
sub medmGenChoice
{
my ($mdir,$mfile,$xpos,$ypos,$wid,$ht,$chan,$fgc,$bgc) = @_;
        open(OUTMEDM, ">>$mdir/$mfile") || die "cannot open $mdir/$mfile for writing ";
        print OUTMEDM <<END;
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
close OUTMEDM;
}
