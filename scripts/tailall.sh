#!/bin/bash

SSH="ssh -t -t "

trap ctrl_c INT

pids=""

function ctrl_c(){
	echo "CLOSING: $pids"
	rm /tmp/myfifo
	rm /tmp/myfifo_pids
	
	kill $pids
	kill %%
	
	kill -9 $pids
	kill -9 %%
	echo "Force kill of $(ps aux | grep tail | grep ssh  | tr -s ' ' | cut -d ' ' -f 2)"
	ps aux | grep tail | grep ssh  | tr -s ' ' | cut -d ' ' -f 2  | xargs kill 
	
	exit 0
}

[ -e /tmp/myfifo ] && rm /tmp/myfifo
mkfifo /tmp/myfifo
[ -e /tmp/myfifo_pids ] && rm /tmp/myfifo_pids
mkfifo /tmp/myfifo_pids



# DATA RO 
#for machine in pcethtb1 cms-h4-03 ; do 
#	{  ${SSH} ${machine}.cern.ch " tail -f \$(ls -tr  /tmp/log_h4daq_start_dr_${machine}_*.log | tail -1 ) " &   echo "$!" >/tmp/myfifo_pids&  }  |  while read line ; do echo  "($machine|dr_std): ${line}" ; done >  /tmp/myfifo & 
#	pids="$pids $!"
#	pids="$pids $( cat /tmp/myfifo_pids)"
#	#echo "PIDS ARE 2: $pids"
#	{ ${SSH} ${machine}.cern.ch " tail -f \$(ls -tr  /tmp/log_h4daq_datareadout_*.log | tail -1 ) " | while read line ; do echo  "($machine|dr_log): ${line}" ; done >  /tmp/myfifo ; } &
#	pids="$pids $!"
#done

## RC
for machine in pcethtb2 ; do 
	{ ${SSH} ${machine}.cern.ch " tail -f \$(ls -tr /tmp/log_h4daq_start_rc_${machine}_*.log | tail -1 ) " & echo "$!" >/tmp/myfifo_pids & } | while read line ; do echo  "($machine|rc_std): ${line}" ; done >  /tmp/myfifo  &
	pids="$pids $!"
	pids="$pids $( cat /tmp/myfifo_pids)"
	{ ${SSH} ${machine}.cern.ch " tail -f \$(ls -tr /tmp/log_h4daq_runcontrol_*.log | tail -1 ) " & echo "$!">/tmp/myfifo_pids & } | while read line ; do echo  "($machine|rc_log): ${line}" ; done >  /tmp/myfifo  &
	pids="$pids $!"
	pids="$pids $( cat /tmp/myfifo_pids)"
done

## EB
for machine in pcethtb2 ; do 
	{ ${SSH} ${machine}.cern.ch " tail -f \$(ls -tr /tmp/log_h4daq_start_eb_${machine}_*.log | tail -1 ) " & echo "$!" >/tmp/myfifo_pids & } | while read line ; do echo  "($machine|eb_std): ${line}" ; done >  /tmp/myfifo  &
	pids="$pids $!"
	pids="$pids $( cat /tmp/myfifo_pids)"
	{ ${SSH} ${machine}.cern.ch " tail -f \$(ls -tr /tmp/log_h4daq_eventbuilder_*.log | tail -1 ) " & echo "$!">/tmp/myfifo_pids & } | while read line ; do echo  "($machine|eb_log): ${line}" ; done >  /tmp/myfifo  &
	pids="$pids $!"
	pids="$pids $( cat /tmp/myfifo_pids)"
done

## DRCV
for machine in cms-h4-04 ; do 
	{ ${SSH} ${machine}.cern.ch " tail -f \$(ls -tr /tmp/log_h4daq_start_drcv_${machine}_*.log | tail -1 ) " & echo "$!" >/tmp/myfifo_pids & } | while read line ; do echo  "($machine|drcv_std): ${line}" ; done >  /tmp/myfifo  &
	pids="$pids $!"
	pids="$pids $( cat /tmp/myfifo_pids)"
	{ ${SSH} ${machine}.cern.ch " tail -f \$(ls -tr /tmp/log_h4daq_eventbuilder_*.log | tail -1 ) " & echo "$!">/tmp/myfifo_pids & } | while read line ; do echo  "($machine|drcv_log): ${line}" ; done >  /tmp/myfifo  &
	pids="$pids $!"
	pids="$pids $( cat /tmp/myfifo_pids)"
done

cat /tmp/myfifo | 
	sed  's:error:\x1b[01;31m&\x1b[00m:gI'  
	#sed  's:debug\|warn[^\ ]*:\x1b[01;33m&\x1b[00m:gI' 
	#sed  's:warn[^\ ]*:\x1b[01;32m&\x1b[00m:gI' |&
	#sed  's:debug:\x1b[01;33m&\x1b[00m:gI'


