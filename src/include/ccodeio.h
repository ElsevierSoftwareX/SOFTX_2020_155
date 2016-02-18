# Header file used by RCG to verify number of inputs and outputs to/from user C code.
# All columns should be tab and/or space delimited.
# NOTE: A -1 in the Ins or Outs column indicates that number of ins/outs is variable ie
#	code uses the numins and numouts input to calculate how many variables to use.

# ======================================================        	==========                    =====   ======
#       C code Source File                                       	C Function                     Ins     Outs
# ======================================================        	==========                    =====   ======
# CDS COMMON ****************************
/opt/rtcds/userapps/release/cds/common/src/BLRMSFILTER.c          	BLRMS                            2       8
/opt/rtcds/userapps/release/cds/common/src/ODC_MASKING_MATRIX.c   	MASKING_MATRIX                 	32       2
# ISC COMMON ****************************
/opt/rtcds/userapps/release/isc/common/src/RtCommunication.c      	RtComm                         	14      24
# SYS COMMON ****************************
/opt/rtcds/userapps/release/sys/common/src/MULTIPLE_BITWISE_AND.c 	MULTIPLE_BITWISE_AND		-1	1
# PSL COMMON ****************************
/opt/rtcds/userapps/release/psl/common/src/dbb/DBB_QPD_MULTICALI.c 	DBB_QPD_MULTICALI		10	12
/opt/rtcds/userapps/release/psl/common/src/dbb/DBB_CTRL_SELECT.c		DBB_CTRL_SELECT			-1	1
/opt/rtcds/userapps/release/psl/common/src/dbb/DBB_CTRL_HOLD.c		DBB_CTRL_HOLD			-1	-1
/opt/rtcds/userapps/release/psl/common/src/dbb/DBB_CTRL_DESELECT.c	DBB_CTRL_DESELECT		-1	-1
/opt/rtcds/userapps/release/psl/common/src/dbb/DBB_AO_SLEWLIMIT.c	DBB_AO_SLEWLIMIT		5	5
/opt/rtcds/userapps/release/psl/common/src/fss/FSS_TEMPSELECT.c		FSS_TEMPSELECT			-1	1
/opt/rtcds/userapps/release/psl/common/src/fss/FSS_PPDET.c		FSS_PPDET			-1	-1
