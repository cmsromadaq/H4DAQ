#!/bin/bash

dryrun=0
verbosity=3

#start_dr=1
#start_rc=1
#start_eb=0

daquser="cmsdaq"
daqhome="/home/cmsdaq/DAQ/H4DAQ"
norecompile=0
logdir="/tmp"
dr=""
rc=""
eb=""
nice=0

TEMP=`getopt -o dv:n: --long nice:,verbose:,logdir:,daquser:,daqhome:,dr:,eb:,rc:,norecompile,dryrun -n 'startall.sh' -- "$@"`

if [ $? != 0 ] ; then echo "Options are wrong..." >&2 ; exit 1 ; fi

# Note the quotes around `$TEMP': they are essential!
eval set -- "$TEMP"

while true; do

  case "$1" in
    -v | --verbose ) verbosity="$2"; shift 2 ;;
    -n | --nice ) nice="$2"; shift 2 ;;
    -d | --dryrun ) dryrun=1; shift;;
    --norecompile ) norecompile=1; shift;;
    --daquser )
      daquser="$2"; shift 2 ;;
    --daqhome )
      daqhome="$2"; shift 2 ;;
    --logdir )
      logdir="$2"; shift 2 ;;
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

echo "=================================================================="
echo "Starting H4DAQ installed @${daqhome} as ${daquser}"
[ "${dr}" == "" ] || echo "DR MACHINES: ${dr}"
[ "${rc}" == "" ] || echo "RC MACHINES: ${rc}"
[ "${eb}" == "" ] || echo "EB MACHINES: ${eb}"
echo "=================================================================="

## create repository if does not exists, otherwise update and compile
mycommand="cd ${daqhome}; mkdir -p DAQ ; cd DAQ ; [ -d H4DAQ ] || git clone git@github.com:cmsromadaq/H4DAQ.git ; cd H4DAQ ; git pull ; python configure.py --noroot ; make -j 4;  "
IFS=','

for machine in $dr ; do 
	
	mydataro="cd ${daqhome}; nohup nice -n +${nice} ./bin/datareadout  -c data/config_${machine}_DR.xml -v ${verbosity} -l ${logdir}/log_h4daq_datareadout_\$(date +%s)_${daquser}.log  > ${logdir}/log_h4daq_start_dr_${machine}_\$(date +%s)_${daquser}.log" 

	
	[ "${dryrun}" == "0" ] || {  echo "$mycommand" ; echo "$mydataro" ; continue; }
#	[ "${start_dr}" == "0" ] && continue;
	## compile
	[ "${norecompile}" == "1" ] || ssh ${daquser}@${machine} /bin/bash -c \'"${mycommand}"\' 2>&1 | tee /tmp/log_h4daq_update_$machine_${USER}.log ;
	## launch the daemon
	echo "-----------------------------"
	echo "START DATAREADOUT on $machine"
	echo "-----------------------------"
	ssh ${daquser}@${machine} /bin/bash -c \'"${mydataro}"\' 2>&1 | tee  /tmp/log_h4daq_start_dr_{machine}_$(date +%s)_${USER}.log ;

done

for machine in $rc ; do 

	myrc="cd ${daqhome}; nohup nice -n +${nice} ./bin/runcontrol  -c data/config_${machine}_RC.xml -v ${verbosity} -l ${logdir}/log_h4daq_runcontrol_\$(date +%s)_${daquser}.log >  ${logdir}/log_h4daq_start_rc_${machine}_\$(date +%s)_${daquser}.log " 
	[ "${dryrun}" == "0" ] || {  echo "$mycommand" ; echo "$mydatarc" ; continue; }
#	[ "${start_rc}" == "0" ] && continue;
	## compile
	[ "${norecompile}" == "1" ] || ssh ${daquser}@${machine} /bin/bash -c \'"${mycommand}"\' 2>&1 | tee /tmp/log_h4daq_update_$machine_${USER}.log ;
	## launch the daemon
	echo "-----------------------------"
	echo "START RUNCONTROL on $machine"
	echo "-----------------------------"
	ssh ${daquser}@${machine} /bin/bash -c \'"${myrc}"\' 2>&1 | tee /tmp/log_h4daq_start_rc_${machine}_$(date +%s)_${USER}.log ;

done

for machine in $eb ; do 

	myeb="cd ${daqhome}; nohup nice -n +${nice} ./bin/eventbuilder   -c data/config_${machine}_EB.xml -v ${verbosity} -l ${logdir}/log_h4daq_eventbuilder_\$(date +%s)_${daquser}.log >  ${logdir}/log_h4daq_start_eb_${machine}_\$(date +%s)_${daquser}.log " 
	[ "${dryrun}" == "0" ] || {  echo "$mycommand" ; echo "$mydatarc" ; continue; }
#	[ "${start_eb}" == "0" ] && continue;
	## compile
	[ "${norecompile}" == "1" ] || ssh ${daquser}@${machine} /bin/bash -c \'"${mycommand}"\' 2>&1 | tee /tmp/log_h4daq_update_$machine_${USER}.log ;
	## launch the daemon
	echo "-----------------------------"
	echo "START EVENTBUILDER on $machine"
	echo "-----------------------------"
	ssh ${daquser}@${machine} /bin/bash -c \'"${myeb}"\' 2>&1 | tee /tmp/log_h4daq_start_rc_${machine}_$(date +%s)_${USER}.log ;

done



