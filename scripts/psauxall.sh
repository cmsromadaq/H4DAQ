#!/bin/bash


# DATA RO 
for machine in pcethtb1 cms-h4-03 pcethtb2; do 
	echo "Checking processes on $machine"
	ssh ${machine}.cern.ch " ps aux | grep 'bin/datareadout \\|bin/runcontrol \\|bin/eventbuilder ' | grep -v 'grep' " | 
		sed 's/datareadout/\x1b[01;34m&\x1b[00m/g'  | # BLUE
		sed 's/runcontrol/\x1b[01;31m&\x1b[00m/g'   | # RED
		sed 's/eventbuilder/\x1b[01;32m&\x1b[00m/g'   # GREEN
done
# SLEEP IMPLICIT

## TODO EB
