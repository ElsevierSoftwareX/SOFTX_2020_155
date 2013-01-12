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

print "opening file @_[0]\/@_[1] \n";
        open(OUTMEDM, ">@_[0]/@_[1]") || die "cannot open @_[0]/@_[1] for writing ";
        print OUTMEDM <<END;
file {
        name=@_[1]
        version=030104
}
display {
        object {
                x=100
                y=100
                width=@_[2]
                height=@_[3]
        }
        clr=14
        bclr=2
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
sub medmGenTextMon
{
print "opening file @_[0]\/@_[1] \n";
        open(OUTMEDM, ">>@_[0]/@_[1]") || die "cannot open @_[0]/@_[1] for writing ";
        print OUTMEDM <<END;
"text update" {
        object {
                x=@_[2]
                y=@_[3]
                width=@_[4]
                height=@_[5]
        }
        monitor {
                chan="@_[6]"
                clr="@_[7]"
                bclr="@_[8]"
        }
        clrmod="static"
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
print "opening file @_[0]\/@_[1] \n";
        open(OUTMEDM, ">>@_[0]/@_[1]") || die "cannot open @_[0]/@_[1] for writing ";
        print OUTMEDM <<END;
text {
        object {
                x=@_[2]
                y=@_[3]
                width=@_[4]
                height=@_[5]
        }
	"basic attribute" {
                clr=@_[7]
        }
        align="horiz. centered"
        textix="@_[6]"

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
print "opening file @_[0]\/@_[1] \n";
        open(OUTMEDM, ">>@_[0]/@_[1]") || die "cannot open @_[0]/@_[1] for writing ";
        print OUTMEDM <<END;
byte {
        object {
                x=@_[2]
                y=@_[3]
                width=@_[4]
                height=@_[5]
        }
        monitor {
                chan="@_[6]"
                clr=@_[9]
                bclr=@_[10]
        }
	sbit=@_[7]
	ebit=@_[8]
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
sub medmGenRectangle
{
print "opening file @_[0]\/@_[1] \n";
        open(OUTMEDM, ">>@_[0]/@_[1]") || die "cannot open @_[0]/@_[1] for writing ";
        print OUTMEDM <<END;
rectangle {
        object {
                x=@_[2]
                y=@_[3]
                width=@_[4]
                height=@_[5]
        }
	"basic attribute" {
                clr=@_[6]
        }

}



END
close OUTMEDM;
}
