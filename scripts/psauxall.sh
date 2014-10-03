#!/bin/bash


# DATA RO 
for machine in pcethtb1 cms-h4-03 pcethtb2; do 
	echo "Checking processes on $machine"
	ssh ${machine}.cern.ch " ps aux | grep 'datareadout\\|runcontrol' "
done
# SLEEP IMPLICIT

## TODO EB
