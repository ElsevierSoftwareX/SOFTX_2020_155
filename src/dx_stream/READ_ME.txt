Code in this area is presently development for mx_stream replacement.

NOTE: ***************************************************************
It is hard coded for use on the Caltech test stand for now and would
require various changes to run elsewhere.  It also makes us of later
Dolphin Gen2 networking, so IX drivers instead of site DX drivers.

As the code progresses and becomes more generic, this file will be updated.
*********************************************************************

This code is designed to transmit DAQ data via the Dolphin Networks in 2 fashions:

1) dx_stream: Essentially a direct replacement for mx_stream in that it is designed
  	for point to point communication from many FE computers to a DAQ data
	concentrator. The prototype code as is just sends data from one FE model
	to another computer for testing. To run the prototype code:
		FE computer: 
			1) Must have x1ioplsc0 and x1lscaux models running, as 
			   prototype transmits data from x1lscaux DAQ buffer.
			2) Execute './dx_stream -rn 12 -client', where:
				-rn is the receiving computer Dolphin node number
				-client indicates code is to send data.  
		DAQ computer:
			1) Execute './dx_stream -rn 20 -server -size 7900000', where:
				-rn is the sending computer Dolphin node number.
				-server indicates this computer will be receiving data.
				-size is memory allocation in bytes. Note that system
				 is presently configured for maximum 8MBytes.
2) dx_broadcast: This code broadcasts data to all computers as reflective memory. The 
	prototype code presently has one node as a sender and one node as a receiver.
	To run the code:
		FE computer:
			1) Must have x1ioplsc0 and x1lscaux models running, as 
			   prototype transmits data from x1lscaux DAQ buffer.
			2) Execute './dx_broadcast -client -nodes 1 -size 8000000 -group 2 -loops 12',
			   where:
				-client indicates this is the sender
				-nodes indicates the number of receivers to expect for test purposes.
				-size is memory allocation in bytes. Note that system
				 	is presently configured for maximum 8MBytes.
				-group is the Dolphin broadcast group id
				-loops indicates number of transmissions to occur before exiting, again
					for test purposes.
		DAQ computer:
			1) Execute './dx_broadcast -server -rank 1 -size 8000000 -group 2 -loops 12', where:
				-server indicates this computer will be receiving data.
				-rank is simply a test check number
				-size is memory allocation in bytes. Note that system
				 	is presently configured for maximum 8MBytes.
				-group is the Dolphin broadcast group id
				-loops indicates number of transmissions to occur before exiting, again
					for test purposes.
