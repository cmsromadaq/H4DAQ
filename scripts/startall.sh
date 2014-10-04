#!/bin/bash

dryrun=0
verbosity=3

#start_dr=1
#start_rc=1
#start_eb=0

dr=""
rc=""
eb=""

TEMP=`getopt -o dv: --long verbose:,dr:,eb:,rc:,dryrun -n 'startall.sh' -- "$@"`

if [ $? != 0 ] ; then echo "Options are wrong..." >&2 ; exit 1 ; fi

# Note the quotes around `$TEMP': they are essential!
eval set -- "$TEMP"

while true; do

  case "$1" in
    -v | --verbose ) verbosity="$2"; shift 2 ;;
    -d | --dryrun ) dryrun=1; shift;;
    --dr )
      dr="$2"; shift 2 ;;
    --rc )
      rc="$2"; shift 2 ;;
    --eb )
      eb="$2"; shift 2 ;;
    -- ) shift; break ;;
    * ) break ;;
  esac
done

echo "========================================"
[ "${dr}" == "" ] || echo "DR MACHINES: ${dr}"
[ "${rc}" == "" ] || echo "RC MACHINES: ${rc}"
[ "${eb}" == "" ] || echo "EB MACHINES: ${eb}"
echo "========================================"

## create repository if does not exists, otherwise update and compile
mycommand=" mkdir -p DAQ ; cd DAQ ; [ -d H4DAQ ] || git clone git@github.com:cmsromadaq/H4DAQ.git ; cd H4DAQ ; git pull ; python configure.py --noroot ; make -j 4;  "
IFS=','

for machine in $dr ; do 
	
	mydataro="cd DAQ/H4DAQ; nohup nice -n 0 ./bin/datareadout  -c data/config_${machine}_DR.xml -v ${verbosity} -l /tmp/log_h4daq_datareadout_\$(date +%s).log  > /tmp/log_h4daq_start_dr_${machine}_\$(date +%s).log" 
	
	[ "${dryrun}" == "0" ] || {  echo "$mycommand" ; echo "$mydataro" ; continue; }
#	[ "${start_dr}" == "0" ] && continue;
	## compile
	ssh ${machine}.cern.ch "${mycommand}" 2>&1 | tee /tmp/log_h4daq_update_$machine.log ;
	## launch the daemon
	echo "-----------------------------"
	echo "START DATAREADOUT on $machine"
	echo "-----------------------------"
	ssh ${machine}.cern.ch "${mydataro}" 2>&1 | tee  /tmp/log_h4daq_start_dr_${machine}_$(date +%s).log ;

done

for machine in $rc ; do 

	myrc="cd DAQ/H4DAQ; nohup nice -n 0 ./bin/runcontrol  -c data/config_${machine}_RC.xml -v ${verbosity} -l /tmp/log_h4daq_runcontrol_\$(date +%s).log >  /tmp/log_h4daq_start_rc_${machine}_\$(date +%s).log " 
	[ "${dryrun}" == "0" ] || {  echo "$mycommand" ; echo "$mydatarc" ; continue; }
#	[ "${start_rc}" == "0" ] && continue;
	## compile
	ssh ${machine}.cern.ch "${mycommand}" 2>&1 | tee /tmp/log_h4daq_update_$machine.log ;
	## launch the daemon
	echo "-----------------------------"
	echo "START RUNCONTROL on $machine"
	echo "-----------------------------"
	ssh ${machine}.cern.ch "${myrc}" 2>&1 | tee /tmp/log_h4daq_start_rc_${machine}_$(date +%s).log ;

done

for machine in $eb ; do 

	myeb="cd DAQ/H4DAQ; nohup nice -n 0 ./bin/eventbuilder   -c data/config_${machine}_EB.xml -v ${verbosity} -l /tmp/log_h4daq_eventbuilder_\$(date +%s).log >  /tmp/log_h4daq_start_eb_${machine}_\$(date +%s).log " 
	[ "${dryrun}" == "0" ] || {  echo "$mycommand" ; echo "$mydatarc" ; continue; }
#	[ "${start_eb}" == "0" ] && continue;
	## compile
	ssh ${machine}.cern.ch "${mycommand}" 2>&1 | tee /tmp/log_h4daq_update_$machine.log ;
	## launch the daemon
	echo "-----------------------------"
	echo "START EVENTBUILDER on $machine"
	echo "-----------------------------"
	ssh ${machine}.cern.ch "${myrc}" 2>&1 | tee /tmp/log_h4daq_start_rc_${machine}_$(date +%s).log ;

done



