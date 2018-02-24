Code in this area is presently development for DAQ data transmission via
Dolphin IX adapters..

As the code progresses and becomes more generic, this file will be updated.
*********************************************************************

This code is designed to transmit DAQ data via the Dolphin Networks in 2 fashions:

1) ix_multi_stream:
	Function: Collect DAQ data from all models running on a single FE and 
		  transmit it as a single data block via Dolphin IX.
	Notes:
		- Code does not yet allow for multiple FEs, as data set to
		  only one data area.

	Usage: ./ix_multi_stream -n 1 -g 2 -m "x1iopsam
2) ix_rcvr: 
	Function: This code receives data from ix_multi_stream. 
	Notes:
		- Code does not yet allow for multiple FEs, as data set to
		  only one data area.
		- Code does not yet write out a FE data block ie receives data
		  only.


3) ix_rcvr_threads:

./ix_rcvr_threads -rank 1 -group 2 -nodes 1
