#!/bin/bash

[ -e /tmp/myfifo ] && rm /tmp/myfifo
mkfifo /tmp/myfifo


# DATA RO 
for machine in pcethtb1 cms-h4-03 ; do 
	(ssh ${machine}.cern.ch " tail -f \$(ls -tr  /tmp/log_h4daq_start_dr_${machine}_*.log | tail -1 ) " | while read line ; do echo  "($machine|dr): ${line}" ; done >  /tmp/myfifo ) &
done

for machine in pcethtb2 ; do 
	( ssh ${machine}.cern.ch " tail -f \$(ls -tr /tmp/log_h4daq_start_rc_${machine}_*.log | tail -1 ) " | while read line ; do echo  "($machine|rc): ${line}" ; done >  /tmp/myfifo ) &
done

cat /tmp/myfifo
## TODO EB

rm /tmp/myfifo

kill %%

