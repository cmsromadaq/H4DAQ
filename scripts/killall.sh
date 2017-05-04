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

TEMP=`getopt -o t: --long tag: -n 'killall.sh' -- "$@"`

if [ $? != 0 ] ; then echo "Options are wrong..." >&2 ; exit 1 ; fi

# Note the quotes around `$TEMP': they are essential!
eval set -- "$TEMP"

tag=""

while true; do
  case "$1" in
    -t | --tag )
      tag="$2"; shift 2 ;;
    -- ) shift; break ;;
    * ) break ;;
  esac
done

if [ "${tag}" == "H4_2016" ]; then  
    machines="pcethtb1 pcethtb2 cms-h4-04 cms-h4-05" ;
elif [ "${tag}" == "T9_2016" ]; then
    machines="pcminn03 pccmsrmtb01" ;
elif [ "${tag}" == "T9_2017" ]; then
    machines="cmsmi5 pccmsrmtb01" ;
fi

# DRO/RC/EB
for machine in $machines; do 
    kill_machine $machine
done


