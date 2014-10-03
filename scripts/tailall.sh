#!/bin/bash

trap ctrl_c INT

pids=""

function ctrl_c(){
	echo "CLOSING: $pids"
	rm /tmp/myfifo
	
	kill $pids
	kill %%
	
	kill -9 $pids
	kill -9 %%
	echo "Force kill of $(ps aux | grep tail | grep ssh  | tr -s ' ' | cut -d ' ' -f 2)"
	ps aux | grep tail | grep ssh  | tr -s ' ' | cut -d ' ' -f 2  |xargs kill 
}

[ -e /tmp/myfifo ] && rm /tmp/myfifo
mkfifo /tmp/myfifo



# DATA RO 
for machine in pcethtb1 cms-h4-03 ; do 
	(ssh ${machine}.cern.ch " tail -f \$(ls -tr  /tmp/log_h4daq_start_dr_${machine}_*.log | tail -1 ) " | while read line ; do echo  "($machine|dr_std): ${line}" ; done >  /tmp/myfifo ) &
	## \$ -> \\\$, but it's only the date
	#bash -c "ssh ${machine}.cern.ch \" tail -f \$(ls -tr  /tmp/log_h4daq_start_dr_${machine}_*.log | tail -1 ) \" | while read line ; do echo  \"($machine|dr_std): ${line}\" ; done >  /tmp/myfifo"  &
	pids="$pids $!"
	(ssh ${machine}.cern.ch " tail -f \$(ls -tr  /tmp/log_h4daq_datareadout_*.log | tail -1 ) " | while read line ; do echo  "($machine|dr_log): ${line}" ; done >  /tmp/myfifo ) &
	#bash -c "ssh ${machine}.cern.ch \" tail -f \$(ls -tr  /tmp/log_h4daq_datareadout_*.log | tail -1 ) \" | while read line ; do echo  \"($machine|dr_log): ${line}\" ; done >  /tmp/myfifo " &
	pids="$pids $!"
done

## RC
for machine in pcethtb2 ; do 
	( ssh ${machine}.cern.ch " tail -f \$(ls -tr /tmp/log_h4daq_start_rc_${machine}_*.log | tail -1 ) " | while read line ; do echo  "($machine|rc_std): ${line}" ; done >  /tmp/myfifo ) &
	#bash -c " ssh ${machine}.cern.ch \" tail -f \$(ls -tr /tmp/log_h4daq_start_rc_${machine}_*.log | tail -1 ) \" | while read line ; do echo  \"($machine|rc_std): ${line}\" ; done >  /tmp/myfifo " &
	pids="$pids $!"
	( ssh ${machine}.cern.ch " tail -f \$(ls -tr /tmp/log_h4daq_runcontrol_*.log | tail -1 ) " | while read line ; do echo  "($machine|rc_log): ${line}" ; done >  /tmp/myfifo ) &
	#bash -c " ssh ${machine}.cern.ch \" tail -f \$(ls -tr /tmp/log_h4daq_runcontrol_*.log | tail -1 ) \" | while read line ; do echo  \"($machine|rc_log): ${line}\" ; done >  /tmp/myfifo " &
	pids="$pids $!"
done

## EB
for machine in pcethtb2 ; do 
	#( ssh ${machine}.cern.ch " tail -f \$(ls -tr /tmp/log_h4daq_start_rc_${machine}_*.log | tail -1 ) " | while read line ; do echo  "($machine|rc_std): ${line}" ; done >  /tmp/myfifo ) &
	#bash -c " ssh ${machine}.cern.ch \" tail -f \$(ls -tr /tmp/log_h4daq_start_eb_${machine}_*.log | tail -1 ) \" | while read line ; do echo  \"($machine|eb_std): ${line}\" ; done >  /tmp/myfifo " &
	pids="$pids $!"
	#( ssh ${machine}.cern.ch " tail -f \$(ls -tr /tmp/log_h4daq_runcontrol_*.log | tail -1 ) " | while read line ; do echo  "($machine|rc_log): ${line}" ; done >  /tmp/myfifo ) &
	#bash -c " ssh ${machine}.cern.ch \" tail -f \$(ls -tr /tmp/log_h4daq_eventbuilder_*.log | tail -1 ) \" | while read line ; do echo  \"($machine|eb_log): ${line}\" ; done >  /tmp/myfifo " &
	pids="$pids $!"
done

cat /tmp/myfifo
## TODO EB

