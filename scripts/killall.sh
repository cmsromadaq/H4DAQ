#!/bin/bash

function kill_machine(){
    machine=$1
    #check if machine is alive
     if ! [ 'ping -c 1 -w 5 '$machine ' > /dev/null 2>&1' ]; then echo "$machine not reachable"; return -1; fi
     echo "Terminating runcontrol on $machine"
     ssh ${machine} "killall -9 runcontrol"
     echo "Terminating readout on $machine"
     ssh ${machine} "killall -9 datareadout"
     echo "Terminating eventbuilder on $machine"
     ssh ${machine} "killall -9 eventbuilder"
     echo "Killing datareceiver on $machine"
     ssh ${machine} "killall -9 datareceiver"

}
# DRO/RC/EB
for machine in pcethtb1 pcethtb2 pcminn03 cms-h4-03 cms-h4-04 cms-h4-05 pcethtb3; do 
    kill_machine $machine
done


