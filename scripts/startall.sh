#!/bin/bash

dryrun=0
start_dr=1
start_rc=1
start_eb=0
verbosity=3


## create repository if does not exists, otherwise update and compile
mycommand=" mkdir -p DAQ ; cd DAQ ; [ -d H4DAQ ] || git clone git@github.com:cmsromadaq/H4DAQ.git ; cd H4DAQ ; git pull ; python configure.py --noroot ; make -j 4;  "



for machine in pcethtb1 cms-h4-03 ; do 
	
	mydataro="cd DAQ/H4DAQ; nohup ./bin/datareadout  -c data/config_${machine}_DR.xml -v ${verbosity} -l /tmp/log_h4daq_datareadout_\$(date +%s).log  > /tmp/log_h4daq_start_dr_${machine}_\$(date +%s).log" 
	
	[ "${dryrun}" == "0" ] || {  echo "$mycommand" ; echo "$mydataro" ; continue; }
	[ "${start_dr}" == "0" ] && continue;
	## compile
	ssh ${machine}.cern.ch "${mycommand}" 2>&1 | tee /tmp/log_h4daq_update_$machine.log ;
	## launch the daemon
	echo "-----------------------------"
	echo "START DATAREADOUT on $machine"
	echo "-----------------------------"
	ssh ${machine}.cern.ch "${mydataro}" 2>&1 | tee  /tmp/log_h4daq_start_dr_${machine}_$(date +%s).log ;

done

for machine in pcethtb2 ; do 

	myrc="cd DAQ/H4DAQ; nohup ./bin/runcontrol  -c data/config_${machine}_RC.xml -v ${verbosity} -l /tmp/log_h4daq_runcontrol_\$(date +%s).log >  /tmp/log_h4daq_start_rc_${machine}_\$(date +%s).log " 
	[ "${dryrun}" == "0" ] || {  echo "$mycommand" ; echo "$mydatarc" ; continue; }
	[ "${start_rc}" == "0" ] && continue;
	## compile
	ssh ${machine}.cern.ch "${mycommand}" 2>&1 | tee /tmp/log_h4daq_update_$machine.log ;
	## launch the daemon
	echo "-----------------------------"
	echo "START RUNCONTROL on $machine"
	echo "-----------------------------"
	ssh ${machine}.cern.ch "${myrc}" 2>&1 | tee /tmp/log_h4daq_start_rc_${machine}_$(date +%s).log ;

done

for machine in pcethtb2 ; do 

	myeb="cd DAQ/H4DAQ; nohup ./bin/eventbuilder   -c data/config_${machine}_EB.xml -v ${verbosity} -l /tmp/log_h4daq_eventbuilder_\$(date +%s).log >  /tmp/log_h4daq_start_eb_${machine}_\$(date +%s).log " 
	[ "${dryrun}" == "0" ] || {  echo "$mycommand" ; echo "$mydatarc" ; continue; }
	[ "${start_eb}" == "0" ] && continue;
	## compile
	ssh ${machine}.cern.ch "${mycommand}" 2>&1 | tee /tmp/log_h4daq_update_$machine.log ;
	## launch the daemon
	echo "-----------------------------"
	echo "START EVENTBUILDER on $machine"
	echo "-----------------------------"
	ssh ${machine}.cern.ch "${myrc}" 2>&1 | tee /tmp/log_h4daq_start_rc_${machine}_$(date +%s).log ;

done



