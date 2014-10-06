#!/bin/bash


# DATA RO 
for machine in pcethtb1 cms-h4-03 ; do 
	echo "Terminating datareadout on $machine"
	ssh ${machine}.cern.ch "killall datareadout"
done
# RC/EB
for machine in pcethtb2 ; do 
	echo "Terminating runcontrol on $machine"
	ssh ${machine}.cern.ch "killall runcontrol"
	echo "Terminating eventbuilder on $machine"
	ssh ${machine}.cern.ch "killall eventbuilder"
done

# SLEEP IMPLICIT  kill -9
for machine in pcethtb1 cms-h4-03 ; do 
	echo "Killing datareadout on $machine"
	ssh ${machine}.cern.ch "killall -9 datareadout"
done
for machine in pcethtb2 ; do 
	echo "Killing runcontrol on $machine"
	ssh ${machine}.cern.ch "killall -9 runcontrol"
	echo "Killing eventbuilder on $machine"
	ssh ${machine}.cern.ch "killall -9 eventbuilder"
done

