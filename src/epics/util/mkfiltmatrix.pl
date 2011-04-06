#!/usr/bin/perl -w
#mkmatrix --cols=9 --rows=9  --x=70 --y=45\
#         --chanbase=C1:ASS-YAW_CTRL_MTRX_ \
#         --rowlabels=PRM,BS,ITMX,ITMY,SRM,ETMX,ETMY,PZT1,PZT2 \
#         --collabels=DOF1,DOF2,DOF3,DOF4,DOF5,DOF6,DOF7,DOF8,DOF9 \
#         --filterbase=C1LSP-DOF2PD_MTRX \
#   > C1ASS_YAW_CTRL_MTRX.adl	

use Getopt::Long;

# Default options

my $W = 60;	# Width of each text box
my $H = 25;	# Height of each text box
my $rows = 5;	# Number of rows in the matrix
my $cols = 5;	# Number of columns in the matrix
my $x = 80;	# X-coordinate of left side of the matrix
my $y = 60;	# Y-coordinate of the top of the matrix
my $padx = 10;	# Horizontal padding between entries
my $pady = 10;  # Vertical padding between entries
my $channelbase = 'C1:ASS-PIT_SENMTRX_';
my $filterbase = 'C1LSP-DOF2PD_MTRX';

my $rowlabels = '';
my $collabels = '';

# c.f. http://perldoc.perl.org/Getopt/Long.html

GetOptions("rows=i"=>\$rows,
           "cols=i"=>\$cols,
           "width=i"=>\$W,
           "height=i"=>\$H,
           "x=i"=>\$x,
           "y=i"=>\$y,
           "padx=i"=>\$padx,
           "pady=i"=>\$pady,
           "chanbase=s"=>\$channelbase,
           "rowlabels=s"=>\$rowlabels,
           "collabels=s"=>\$collabels,
	   "filterbase=s"=>\$filterbase);

@rowlabels = split(/,/, $rowlabels);
@collabels = split(/,/, $collabels);

$matrix_width  = $cols * ($W + $padx) - $padx;
$matrix_height = $rows * ($H + $pady) - $pady + ($rows * $H);

$display_width = $x + $matrix_width  + $padx;
$display_height= $y + $matrix_height + $pady;

# Print out the header

make_adl_header();

# Make the matrix!

print <<END;
composite {
  "composite name" = "matrix"
  object {
    x = $x 
    y = $y
    height = $matrix_height
    width  = $matrix_width
  }
  children {
END

# Put in row and column labels
if ($#rowlabels > 0) {
  for ($row = 0; $row < $rows; $row ++) {
    $X = $x + ($W + $padx) * -1;
    $Y = $y + ($H + $pady) * $row + ($row * $H);
    make_text_box($X, $Y, $W, $H, $rowlabels[$row]);
  }
}

if ($#collabels > 0) {
  for ($col = 0; $col < $cols; $col ++) {
    $X = $x + ($W + $padx) * $col;
    $Y = $y + ($H + $pady) * -1;
    make_text_box($X, $Y, $W, $H, $collabels[$col]);
  }
}


for ($row = 0; $row < $rows; $row ++) {
  for ($col = 0; $col < $cols; $col ++) {

    $X = $x + ($W + $padx) * $col;
    $Y = $y + ($H + $pady) * $row + ($row * $H);
    $channel = $channelbase . sprintf('%i_%i_GAIN', $row,  $col);

    # Make the primary text box (for nonzero values)
    make_text_entry($X, $Y, $W, $H, $channel, 14, 60, "if not zero");

    # Make the secondary text box (for zero values)
    make_text_entry($X, $Y, $W, $H, $channel, 10, 5, "if zero");

    # Make the button to the corresponding filter module below the text box
    $filter = "$filterbase" . "/$filterbase" . sprintf('_%i_%i.adl', $row, $col);
    $filter_label = sprintf('%i_%i', $row, $col);
    $Y = $y + ($H + $pady) *  $row + (($row + 1) * $H);
    make_filter_button($X, $Y, $W, $H, $filter,$filter_label,14,25);

    # Make the highlight box around the text box
  }
}

print "    }\n  \n";


# Subroutines to generate ADL code
sub make_text_entry {
  my ($X, $Y, $W, $H, $channel, $c, $bg, $vis) = @_;
  print <<END;
composite {
  object {
    x=$X
    y=$Y
    width=$W
    height=$H
  }
  children {
  "text entry" {
    object {
      x=$X
      y=$Y
      width=$W
      height=$H
    }
    control {
      chan="$channel"
      clr=$c
      bclr=$bg
    }
    limits {
    }
  } 
  }
  "dynamic attribute" {
    vis="$vis"
    chan="$channel"
  }
}
END
}

sub make_text_box {
  my ($X, $Y, $W, $H, $text) = @_;
  print <<END;
text {
        object {
                x=$X
                y=$Y
                width=$W
                height=$H
        }
        "basic attribute" {
                clr=2
        }
        textix="$text"
        align="horiz. centered"
}
END
}

sub make_filter_button {
  my ($X, $Y, $W, $H, $chan, $text,$clr,$bclr) = @_;
  print <<END;
"related display" {
	object {
		x=$X
		y=$Y
		width=$W
		height=$H
	}
	display[0] {
		label="$text"
		name="$chan"
	}
	clr=$clr
	bclr=$bclr
	label="$text"
	visual="a row of buttons"
}
END
}

sub make_adl_header {
print <<END;
file {
        name="matrix.adl"
        version=030003
}

display {
        object {
                width = $display_width;
                height= $display_height;
        }
        clr=5
        bclr=12
        cmap=""
        gridSpacing=5
        gridOn=0
        snapToGrid=0
}

"color map" {
        ncolors=65 colors {
                ffffff, ececec, dadada, c8c8c8, bbbbbb, aeaeae, 9e9e9e,
                919191, 858585, 787878, 696969, 5a5a5a, 464646, 2d2d2d,
                000000, 00d800, 1ebb00, 339900, 2d7f00, 216c00, fd0000,
                de1309, be190b, a01207, 820400, 5893ff, 597ee1, 4b6ec7,
                3a5eab, 27548d, fbf34a, f9da3c, eeb62b, e19015, cd6100,
                ffb0ff, d67fe2, ae4ebc, 8b1a96, 610a75, a4aaff, 8793e2,
                6a73c1, 4d52a4, 343386, c7bb6d, b79d5c, a47e3c, 7d5627,
                58340f, 99ffff, 73dfff, 4ea5f9, 2a63e4, 0a00b8, ebf1b5,
                d4db9d, bbc187, a6a462, 8b8239, 73ff6b, 52da3b, 3cb420,
                289315, 1a7309,
        }
}
END
}
