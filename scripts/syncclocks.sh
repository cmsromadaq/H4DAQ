#!/bin/bash

for machine in pcethtb1 pcethtb2 pcethtb3 cms-h4-03 ; do 
    echo "Syncronizing clock on $machine"
    ssh ${machine}.cern.ch "ntpd -q"
    echo "After syncronization:"
    ssh ${machine}.cern.ch "ntpq -c peers"
done

