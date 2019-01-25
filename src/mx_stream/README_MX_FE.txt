The program mx_fe has been added to provide open-mx DAQ data transfer
from FE computers to DAQ DC.  This done to provide data in the form
required by the new shared memory daqd. It is intended as a stop gap
measure until another suitable network protocol is found and tested.

This software may be run on an FE computer in parallel with the usual
mx_stream software if a second DAQ DC is provided, as is the case on
the LLO DTS, where testing is on going.  This allows for comparison
between present production daqd and new daqd_shmem DAQ systems.

The mx_fe code can be found in the src/mx_stream directories in both
trunk and branch-3.5.

To run the code on an FE computer, the command is:

mx_fe -e <local_endpoint> -r <remote_endpoint -b <local_shmem> -D
<trasmitDelay> -s <Fe_model_names> -t <down_stream_target>

where:
	-e = local mx endpoint (0-31): If running in parallel with mx_stream
	     this should be set to 1 as mx_stream uses 0
	-r = remote mx endpoint (0-31):  Each FE computer on this network 
	     should use a unique number as down stream receiver expects
	     a single FE on each end point.
	-D = transmit delay (0-15): Delays the transmission of data by
	     specified number of milliseconds between data ready to 
	     transmit and actual transmission.  This is used as a tuning
	     mechanism to prevent all FE transmitting at the same time
	     and causing data spikes which can lead to CRC errors at
	     the receive end. This is particularly true if running
	     both mx_stream and mx_fe on the same FE.
	-s = List of control models running on this FE computer. This
	     list must be in quotes if started from the command line.
	-t = target DAQ DC mx node, usually in the form of 
	     computer_host_name:card_number
