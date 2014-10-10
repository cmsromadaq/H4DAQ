#!/bin/bash

for machine in pcethtb1 pcethtb2 pcethtb3 cms-h4-03 ; do 
    echo "Syncronizing clock on $machine"
    ssh root@${machine}.cern.ch -i ~/.ssh/key_for_clock_sync/id_rsa_clock
done

