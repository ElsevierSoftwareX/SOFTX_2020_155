#!/bin/bash

i=0;
while [ $i -le 128 ]
do
	echo $i;
	caput L1:DAQ-FEC_${i}_LOAD_CONFIG 1
	i=`expr $i + 1`
done
