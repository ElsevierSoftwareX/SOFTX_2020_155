package CDS::IPCx;
use Exporter;
@ISA = ('Exporter');
 
sub partType {
        return IPCx;
}
 
# Print Epics communication structure into a header file
# Current part number is passed as first argument
sub printHeaderStruct {
        my ($i) = @_;
        ;
}

# Print Epics variable definitions
# Current part number is passed as first argument
sub printEpics {
        my ($i) = @_;
        ;
}

# Print variable declarations into front-end file
# Current part number is passed as first argument
sub printFrontEndVars  {
        my ($i) = @_;

        if ($::ipcxDeclDone == 0) {
           $::ipcxDeclDone = 1;

           print ::OUT "#define COMMDATA_INLINE\n";
           print ::OUT "#include \"commData2.h\"\n";
           print ::OUT "static int myIpcCount;\n";
           print ::OUT "static CDS_IPC_INFO ipcInfo[$::ipcxCnt];\n\n";
        }
}

# Return front end initialization code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndInitCode {
        my ($i) = @_;
        my $found = 0;
 
        if ($::ipcxInitDone == 0) {
           $calcExp = "\nmyIpcCount = $::ipcxCnt;\n\n";
        }
        else {
           $calcExp = "";
        }

        $::ipcxRef[$::ipcxInitDone] = $i;
 
        for ($l = 0; $l < $::ipcxCnt; $l++) {
           if ($::ipcxParts[$l][6] == $i) {
              $found = 1;
 
              if ( ($::ipcxParts[$l][7] =~ /^Ground/) || ($::ipcxParts[$l][7] =~ /\_Ground/) ) {
                 $calcExp .= "ipcInfo[$::ipcxInitDone]\.mode = IRCV;\n";
              }
              else {
                 $calcExp .= "ipcInfo[$::ipcxInitDone]\.mode = ISND;\n";
              }
 
              my $subPart = substr($::ipcxParts[$l][1], 0, 5);
              $calcExp .= "ipcInfo[$::ipcxInitDone]\.netType = $subPart;\n";
              $calcExp .= "ipcInfo[$::ipcxInitDone]\.sendRate = $::ipcxParts[$l][2];\n";
              $calcExp .= "ipcInfo[$::ipcxInitDone]\.ipcNum = $::ipcxParts[$l][4];\n";
              $calcExp .= "ipcInfo[$::ipcxInitDone]\.errFlag = 0;\n";

              last;
           }
        }
 
        $::ipcxInitDone++;
 
        if ($found == 0) {
           die "***ERROR: Did not find IPCx part $i\n";
        }

        if ($::ipcxInitDone eq $::ipcxCnt) {
           $calcExp .= "\ncommData2Init(myIpcCount, FE_RATE, ipcInfo);\n\n";
        }

        return $calcExp;
}

# Figure out part input code
# Argument 1 is the part number
# Argument 2 is the input number
# Returns calculated input code
sub fromExp {
        my ($i, $j) = @_;
        my $from1 = $::partInNum[$i][$j];
        my $from2 = $::partInputPort[$i][$j];
        my $index = -999;

        for ($k = 0; $k < $::ipcxCnt; $k++) {
           if ($::ipcxRef[$k] == $from1) {
              $index = $k;
              last;
           }
        }

        if ($index < 0) {
           die "***ERROR: index-FE=$index\n";
        }

        if ($from2 == 0) {
           return "ipcInfo[$index]\.data";
        }
        elsif ($from2 == 1) {
           return "ipcInfo[$index]\.errTotal";
        }
        else {
           die "***ERROR: IPCx component with incorrect partInputPort = $from2\n";
        }
}

# Return front end code
# Argument 1 is the part number
# Returns calculated code string
sub frontEndCode {
        my ($i) = @_;

        if ($::partInputType[$i][0] eq "GROUND") {
           $calcExp = "";
        }
        else {
           my $index = -999;

           for ($k = 0; $k < $::ipcxCnt; $k++) {
              if ($::ipcxRef[$k] == $i) {
                 $index = $k;
                 last;
              }
           }
 
           if ($index < 0) {
              die "***ERROR: index-FEC=$index\n";
           }

           $calcExp = "// IPCx:  $::xpartName[$i]\n";
           $calcExp .= "ipcInfo[$index]\.data = ";
           $calcExp .= "$::fromExp[0];\n";
        }

        return $calcExp;
}

