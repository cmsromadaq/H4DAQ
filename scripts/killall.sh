#!/bin/bash


# RC/EB
for machine in localhost pcethtb2; do 
	echo "Terminating runcontrol on $machine"
	ssh ${machine} "killall runcontrol"
	echo "Terminating eventbuilder on $machine"
	ssh ${machine} "killall eventbuilder"
done

for machine in localhost; do 
	echo "Killing runcontrol on $machine"
	ssh ${machine} "killall -9 runcontrol"
	echo "Killing eventbuilder on $machine"
	ssh ${machine} "killall -9 eventbuilder"
done

