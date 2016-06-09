#!/bin/bash

function kill_machine(){
    machine=$1
    #check if machine is alive
    ping -q -c 2 -w 5 ${machine} > /dev/null
    if  [ "$?" != 0 ]; then
        echo "+++ ${machine} is NOT alive +++"
        return 1
    fi
    ssh ${machine} ls > /dev/null
    if  [ "$?" != 0 ]; then
        echo "+++ CanNOT connect to ${machine} +++"
        return 1
    fi
     echo "Terminating runcontrol on $machine"
     ssh ${machine} "killall -9 runcontrol"
     echo "Terminating readout on $machine"
     ssh ${machine} "killall -9 datareadout"
     echo "Terminating eventbuilder on $machine"
     ssh ${machine} "killall -9 eventbuilder"
     echo "Killing datareceiver on $machine"
     ssh ${machine} "killall -9 datareceiver"
     return 0
}
# DRO/RC/EB
for machine in pcethtb1 pcethtb2 pcminn03 cms-h4-03 cms-h4-04 cms-h4-05 pcethtb3; do 
    kill_machine $machine
done


