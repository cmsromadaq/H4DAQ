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

function kill_fedaq(){
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
    
    echo "Killing FEDAQ on $machine"
    ssh ${machine} "sudo systemctl stop run_fedaq.service"
    return 0
}

TEMP=`getopt -o m: --long machines:,fedaq: -n 'killall.sh' -- "$@"`

if [ $? != 0 ] ; then echo "Options are wrong..." >&2 ; exit 1 ; fi

# Note the quotes around `$TEMP': they are essential!
eval set -- "$TEMP"

machines="pcethtb2 cms-h4-04 cms-h4-05"
fedaq_machine=""

while true; do
  case "$1" in
    -m | --machines )
      machines="$2"; shift 2 ;;
    -t | --fepet )
      fedaq="$2"; shift 2 ;;
    -- ) shift; break ;;
    * ) break ;;
  esac
done

# kill on localhost
killall -9 runcontrol
killall -9 datareadout
killall -9 eventbuilder
killall -9 datareceiver

for machine in $machines; do 
    kill_machine $machine
done

kill_fedaq $fedaq

