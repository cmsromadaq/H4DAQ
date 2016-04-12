#!/bin/bash

# DRO/RC/EB
for machine in pcethtb1 pcethtb2 pcminn03; do 
	echo "Terminating runcontrol on $machine"
	ssh ${machine} "killall runcontrol"
	echo "Terminating readout on $machine"
	ssh ${machine} "killall datareadout"
	echo "Terminating eventbuilder on $machine"
	ssh ${machine} "killall eventbuilder"
done

#DRCV
for machine in cms-h4-03 cms-h4-04 cms-h4-05 pcethtb3 ; do 
	echo "Killing datareceiver on $machine"
	ssh ${machine} "killall -9 datareceiver"
done

